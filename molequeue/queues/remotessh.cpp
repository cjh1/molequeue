/******************************************************************************

  This source file is part of the MoleQueue project.

  Copyright 2011-2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "remotessh.h"

#include "../job.h"
#include "../jobmanager.h"
#include "../logentry.h"
#include "../logger.h"
#include "../program.h"
#include "../remotequeuewidget.h"
#include "../server.h"
#include "../sshcommandconnectionfactory.h"
#include "../libssh2connection.h"
#include "../askpassword.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <QtGui>

namespace MoleQueue {

QueueRemoteSsh::QueueRemoteSsh(const QString &queueName, QueueManager *parentObject)
  : QueueRemote(queueName, parentObject),
    m_isCheckingQueue(false)
{
  // Check for jobs to submit every 5 seconds
  m_checkForPendingJobsTimerId = startTimer(5000);

  // Always allow m_requestQueueCommand to return 0
  m_allowedQueueRequestExitCodes.append(0);

  createConnection();
}

QueueRemoteSsh::~QueueRemoteSsh()
{
}

void QueueRemoteSsh::readSettings(QSettings &settings)
{
  QueueRemote::readSettings(settings);

  m_submissionCommand = settings.value("submissionCommand").toString();
  m_requestQueueCommand = settings.value("requestQueueCommand").toString();
  m_killCommand = settings.value("killCommand").toString();
  setSshExecutable(settings.value("sshExecutable", "ssh").toString());
  setScpExecutable(settings.value("scpExecutable", "scp").toString());
  setHostName(settings.value("hostName").toString());
  setUserName(settings.value("userName").toString());
  setIdentityFile(settings.value("identityFile").toString());
  setSshPort(settings.value("sshPort").toInt());
  setUseLibSsh2(settings.value("useLibSsh2").toBool());
}

void QueueRemoteSsh::writeSettings(QSettings &settings) const
{
  QueueRemote::writeSettings(settings);

  settings.setValue("submissionCommand", m_submissionCommand);
  settings.setValue("requestQueueCommand", m_requestQueueCommand);
  settings.setValue("killCommand", m_killCommand);
  settings.setValue("sshExecutable", m_sshExecutable);
  settings.setValue("scpExecutable", m_scpExecutable);
  settings.setValue("hostName", m_hostName);
  settings.setValue("userName", m_userName);
  settings.setValue("identityFile", m_identityFile);
  settings.setValue("sshPort",  m_sshPort);
  settings.setValue("useLibSsh2", m_useLibSsh2);
}

void QueueRemoteSsh::exportConfiguration(QSettings &exporter,
                                         bool includePrograms) const
{
  QueueRemote::exportConfiguration(exporter, includePrograms);

  exporter.setValue("submissionCommand", m_submissionCommand);
  exporter.setValue("requestQueueCommand", m_requestQueueCommand);
  exporter.setValue("killCommand", m_killCommand);
  exporter.setValue("hostName", m_hostName);
  exporter.setValue("sshPort",  m_sshPort);
}

void QueueRemoteSsh::importConfiguration(QSettings &importer,
                                      bool includePrograms)
{
  QueueRemote::importConfiguration(importer, includePrograms);

  m_submissionCommand = importer.value("submissionCommand").toString();
  m_requestQueueCommand = importer.value("requestQueueCommand").toString();
  m_killCommand = importer.value("killCommand").toString();
  setHostName(importer.value("hostName").toString());
  setSshPort(importer.value("sshPort").toInt());
}

AbstractQueueSettingsWidget* QueueRemoteSsh::settingsWidget()
{
  RemoteQueueWidget *widget = new RemoteQueueWidget (this);
  return widget;
}

void QueueRemoteSsh::createRemoteDirectory(Job job)
{
  // Note that this is just the working directory base -- the job folder is
  // created by scp.
  QString remoteDir = QString("%1").arg(m_workingDirectoryBase);

  SshOperation *op = m_sshConnection->newCommand(QString("mkdir -p %1").
                                                  arg(remoteDir));

  op->setData(QVariant::fromValue(job));
  connect(op, SIGNAL(complete()), this, SLOT(remoteDirectoryCreated()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()), job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::remoteDirectoryCreated()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    return;
  }
  op->deleteLater();

  Job job = op->data().value<Job>();

  if (!job.isValid()) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender does not have an associated job!"));
    return;
  }

  if (op->errorCode() != 0) {
    Logger::logWarning(tr("Cannot create remote directory '%1@%2:%3'.\n"
                          "Exit code (%4) %5")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_workingDirectoryBase).arg(op->errorCode())
                     .arg(op->output()), job.moleQueueId());
    // Retry submission:
    if (addJobFailure(job.moleQueueId()))
      m_pendingSubmission.append(job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    return;
  }

  copyInputFilesToHost(job);
}

void QueueRemoteSsh::copyInputFilesToHost(Job job)
{
  QString localDir = job.localWorkingDirectory();
  QString remoteDir = QDir::cleanPath(QString("%1/%2")
                                      .arg(m_workingDirectoryBase)
                                      .arg(job.moleQueueId()));

  SshOperation *op = m_sshConnection->newDirUpload(localDir, remoteDir);
  op->setData(QVariant::fromValue(job));
  connect(op, SIGNAL(complete()), this, SLOT(inputFilesCopied()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()), job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::inputFilesCopied()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    return;
  }
  op->deleteLater();

  Job job = op->data().value<Job>();

  if (!job.isValid()) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender does not have an associated job!"));
    return;
  }

  if (op->errorCode() != 0) {
    // Check if we just need to make the parent directory
    if (op->errorCode() == 1 &&
        op->output().contains("No such file or directory")) {
      Logger::logDebugMessage(tr("Remote working directory missing on remote "
                                 "host. Creating now..."), job.moleQueueId());
      createRemoteDirectory(job);
      return;
    }

    Logger::logWarning(tr("Error while copying input files to remote host:\n"
                          "'%1' --> '%2/'\nExit code (%3) %4")
                       .arg(job.localWorkingDirectory())
                       .arg(m_workingDirectoryBase)
                       .arg(op->errorCode()).arg(op->output()),
                       job.moleQueueId());
    // Retry submission:
    if (addJobFailure(job.moleQueueId()))
      m_pendingSubmission.append(job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    return;
  }

  submitJobToRemoteQueue(job);
}

void QueueRemoteSsh::submitJobToRemoteQueue(Job job)
{
  const QString command = QString("cd %1/%2 && %3 %4")
      .arg(m_workingDirectoryBase)
      .arg(job.moleQueueId())
      .arg(m_submissionCommand)
      .arg(m_launchScriptName);

  qDebug() << "submit command: " << command;

  SshOperation *op = m_sshConnection->newCommand(command);
  op->setData(QVariant::fromValue(job));
  connect(op, SIGNAL(complete()),
          this, SLOT(jobSubmittedToRemoteQueue()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()), job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::jobSubmittedToRemoteQueue()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    return;
  }
  op->deleteLater();

  IdType queueId;

  parseQueueId(op->output(), &queueId);
  Job job = op->data().value<Job>();

  if (!job.isValid()) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender does not have an associated job!"));
    return;
  }

  if (op->errorCode() != 0) {
    Logger::logWarning(tr("Could not submit job to remote queue on %1@%2:%3\n"
                          "%4 %5/%6/%7\nExit code (%8) %9")
                       .arg(m_userName)
                       .arg(m_hostName)
                       .arg(m_sshPort).arg(m_submissionCommand)
                       .arg(m_workingDirectoryBase).arg(job.moleQueueId())
                       .arg(m_launchScriptName).arg(op->errorCode())
                       .arg(op->output()), job.moleQueueId());
    // Retry submission:
    if (addJobFailure(job.moleQueueId()))
      m_pendingSubmission.append(job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    return;
  }

  job.setJobState(MoleQueue::Submitted);
  clearJobFailures(job.moleQueueId());
  job.setQueueId(queueId);
  m_jobs.insert(queueId, job.moleQueueId());
}

void QueueRemoteSsh::requestQueueUpdate()
{
  if (m_isCheckingQueue)
    return;

  if (m_jobs.isEmpty())
    return;

  m_isCheckingQueue = true;

  const QString command = generateQueueRequestCommand();

  SshOperation *op = m_sshConnection->newCommand(command);
  connect(op, SIGNAL(complete()),
          this, SLOT(handleQueueUpdate()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()));
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::handleQueueUpdate()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    m_isCheckingQueue = false;
    return;
  }
  op->deleteLater();

  if (!m_allowedQueueRequestExitCodes.contains(op->errorCode())) {
    Logger::logWarning(tr("Error requesting queue data (%1 -u %2) on remote "
                          "host %3@%4:%5. Exit code (%6) %7")
                       .arg(m_requestQueueCommand)
                       .arg(m_sshConnection->userName())
                       .arg(m_sshConnection->userName())
                       .arg(m_sshConnection->hostName())
                       .arg(m_sshConnection->portNumber())
                       .arg(op->errorCode()).arg(op->output()));
    m_isCheckingQueue = false;
    return;
  }

  QStringList output = op->output().split("\n", QString::SkipEmptyParts);

  // Get list of submitted queue ids so that we detect when jobs have left
  // the queue.
  QList<IdType> queueIds = m_jobs.keys();

  MoleQueue::JobState state;
  foreach (QString line, output) {
    IdType queueId;
    if (parseQueueLine(line, &queueId, &state)) {
      IdType moleQueueId = m_jobs.value(queueId, InvalidId);
      if (moleQueueId != InvalidId) {
        queueIds.removeOne(queueId);
        // Get pointer to jobmanager to lookup job
        if (!m_server) {
          Logger::logError(tr("Queue '%1' cannot locate Server instance!")
                           .arg(m_name), moleQueueId);
          m_isCheckingQueue = false;
          return;
        }
        Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
        if (!job.isValid()) {
          Logger::logError(tr("Queue '%1' Cannot update invalid Job reference!")
                           .arg(m_name), moleQueueId);
          continue;
        }
        job.setJobState(state);
      }
    }
  }

  // Now copy back any jobs that have left the queue
  foreach (IdType queueId, queueIds)
    beginFinalizeJob(queueId);

  m_isCheckingQueue = false;
}

void QueueRemoteSsh::beginFinalizeJob(IdType queueId)
{
  IdType moleQueueId = m_jobs.value(queueId, InvalidId);
  if (moleQueueId == InvalidId)
    return;

  m_jobs.remove(queueId);

  // Lookup job
  if (!m_server)
    return;
  Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
  if (!job.isValid())
    return;

  finalizeJobCopyFromServer(job);
}

void QueueRemoteSsh::finalizeJobCopyFromServer(Job job)
{
  if (!job.retrieveOutput() ||
      (job.cleanLocalWorkingDirectory() && job.outputDirectory().isEmpty())
      ) {
    // Jump to next step
    finalizeJobCopyToCustomDestination(job);
    return;
  }

  QString localDir = job.localWorkingDirectory() + "/..";
  QString remoteDir =
      QString("%1/%2").arg(m_workingDirectoryBase).arg(job.moleQueueId());

  qDebug() << "Dir download: " << remoteDir << "local: " <<localDir;
  SshOperation *op = m_sshConnection->newDirDownload(remoteDir, localDir);
  op->setData(QVariant::fromValue(job));
  connect(op, SIGNAL(complete()),
          this, SLOT(finalizeJobOutputCopiedFromServer()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()), job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::finalizeJobOutputCopiedFromServer()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    return;
  }
  op->deleteLater();

  Job job = op->data().value<Job>();

  if (!job.isValid()) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender does not have an associated job!"));
    return;
  }

  if (op->errorCode() != 0) {
    Logger::logError(tr("Error while copying job output from remote server:\n"
                        "%1@%2:%3 --> %4\nExit code (%5) %6")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber())
                     .arg(job.localWorkingDirectory())
                     .arg(op->errorCode()).arg(op->output()),
                     job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    return;
  }

  finalizeJobCopyToCustomDestination(job);
}

void QueueRemoteSsh::finalizeJobCopyToCustomDestination(Job job)
{
  // Skip to next step if needed
  if (job.outputDirectory().isEmpty() ||
      job.outputDirectory() == job.localWorkingDirectory()) {
    finalizeJobCleanup(job);
    return;
  }

  // The copy function will throw errors if needed.
  if (!recursiveCopyDirectory(job.localWorkingDirectory(),
                              job.outputDirectory())) {
    job.setJobState(MoleQueue::Error);
    return;
  }

  finalizeJobCleanup(job);
}

void QueueRemoteSsh::finalizeJobCleanup(Job job)
{
  if (job.cleanLocalWorkingDirectory())
    cleanLocalDirectory(job);

  if (job.cleanRemoteFiles())
    cleanRemoteDirectory(job);

  job.setJobState(MoleQueue::Finished);
}

void QueueRemoteSsh::cleanRemoteDirectory(Job job)
{
  QString remoteDir = QDir::cleanPath(
        QString("%1/%2").arg(m_workingDirectoryBase).arg(job.moleQueueId()));

  // Check that the remoteDir is not just "/" due to another bug.
  if (remoteDir.simplified() == "/") {
    Logger::logError(tr("Refusing to clean remote directory %1 -- an internal "
                        "error has occurred.").arg(remoteDir),
                     job.moleQueueId());
    return;
  }

  QString command = QString ("rm -rf %1").arg(remoteDir);

  SshOperation *op = m_sshConnection->newCommand(command);
  op->setData(QVariant::fromValue(job));
  connect(op, SIGNAL(complete()),
          this, SLOT(remoteDirectoryCleaned()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()), job.moleQueueId());
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::remoteDirectoryCleaned()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    return;
  }
  op->deleteLater();

  Job job = op->data().value<Job>();

  if (!job.isValid()) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender does not have an associated job!"));
    return;
  }

  if (op->errorCode() != 0) {
    Logger::logError(tr("Error clearing remote directory '%1@%2:%3/%4'.\n"
                        "Exit code (%5) %6")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_workingDirectoryBase).arg(job.moleQueueId())
                     .arg(op->errorCode()).arg(op->output()),
                     job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    return;
  }
}

void QueueRemoteSsh::beginKillJob(Job job)
{
  const QString command = QString("%1 %2")
      .arg(m_killCommand)
      .arg(job.queueId());

  SshOperation *op = m_sshConnection->newCommand(command);
  op->setData(QVariant::fromValue(job));
  connect(op, SIGNAL(complete()),
          this, SLOT(endKillJob()));

  if (!op->execute()) {
    Logger::logError(tr("Could not initialize ssh resources: user= '%1'\nhost ="
                        " '%2' port = '%3'")
                     .arg(m_sshConnection->userName())
                     .arg(m_sshConnection->hostName())
                     .arg(m_sshConnection->portNumber()), job.moleQueueId());
    job.setJobState(MoleQueue::Error);
    op->deleteLater();
    return;
  }
}

void QueueRemoteSsh::endKillJob()
{
  SshOperation *op = qobject_cast<SshOperation*>(sender());
  if (!op) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender is not an SshConnection!"));
    return;
  }
  op->deleteLater();

  Job job = op->data().value<Job>();
  if (!job.isValid()) {
    Logger::logError(tr("Internal error: %1\n%2").arg(Q_FUNC_INFO)
                     .arg("Sender does not have an associated job!"));
    return;
  }

  if (op->errorCode() != 0) {
    Logger::logWarning(tr("Error cancelling job (mqid=%1, queueid=%2) on "
                          "%3@%4:%5 (queue=%6)\n(%7) %8")
                       .arg(job.moleQueueId()).arg(job.queueId())
                       .arg(m_sshConnection->userName())
                       .arg(m_sshConnection->hostName())
                       .arg(m_sshConnection->portNumber()).arg(m_name)
                       .arg(op->errorCode()).arg(op->output()));
    return;
  }

  job.setJobState(MoleQueue::Killed);
}


QString QueueRemoteSsh::generateQueueRequestCommand()
{
  QList<IdType> queueIds = m_jobs.keys();
  QString queueIdString;
  foreach (IdType id, queueIds) {
    queueIdString += QString::number(id) + " ";
  }

  return QString ("%1 %2").arg(m_requestQueueCommand).arg(queueIdString);
}

void QueueRemoteSsh::setSshExecutable(const QString &exe)
{
  m_sshExecutable = exe;
}

QString QueueRemoteSsh::sshExecutable() const
{
  return m_sshExecutable;
}

void QueueRemoteSsh::setScpExecutable(const QString &exe)
{
  m_scpExecutable = exe;
}

QString QueueRemoteSsh::scpExectuable() const
{
  return m_scpExecutable;
}

void QueueRemoteSsh::recreateConnection() {
  delete m_sshConnection;
  createConnection();
}

void QueueRemoteSsh::createConnection() {

  if(!m_useLibSsh2) {
    SshCommandConnection *commandConn = SshCommandConnectionFactory::instance()->
      newSshCommandConnection(this);

    commandConn->setSshCommand(SshCommandConnectionFactory::defaultSshCommand());
    commandConn->setScpCommand(SshCommandConnectionFactory::defaultScpCommand());
    m_sshConnection = commandConn;
    m_sshConnection->setHostName(m_hostName);
    m_sshConnection->setUserName(m_userName);
    m_sshConnection->setPortNumber(m_sshPort);
    m_sshConnection->setIdentityFile(m_identityFile);

  }
  else {
    AskPassword *askPassword = new QDialogAskPassword(this);
    m_sshConnection = new LibSsh2Connection(m_hostName,
                                            m_userName,
                                            m_sshPort,
                                            askPassword);
  }
}

} // End namespace
