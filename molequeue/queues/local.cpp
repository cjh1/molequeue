/******************************************************************************

  This source file is part of the MoleQueue project.

  Copyright 2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "local.h"

#include "../job.h"
#include "../jobmanager.h"
#include "../localqueuewidget.h"
#include "../logentry.h"
#include "../logger.h"
#include "../program.h"
#include "../queue.h"
#include "../queuemanager.h"
#include "../server.h"

#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimerEvent>
#include <QtCore/QThread> // For ideal thread count

#include <QtGui/QFormLayout>
#include <QtGui/QSpinBox>

#include <QtCore/QDebug>

#ifdef WIN32
#include <Windows.h> // For _PROCESS_INFORMATION (PID parsing)
#endif

namespace MoleQueue {

QueueLocal::QueueLocal(QueueManager *parentManager) :
  Queue("Local", parentManager),
  m_checkJobLimitTimerId(-1),
  m_cores(-1)
{
#ifdef WIN32
  m_launchTemplate = "@echo off\n\n$$programExecution$$\n";
  m_launchScriptName = "MoleQueueLauncher.bat";
#else // WIN32
  m_launchTemplate = "#!/bin/bash\n\n$$programExecution$$\n";
  m_launchScriptName = "MoleQueueLauncher.sh";
#endif // WIN32

  // Check if new jobs need starting every 10 seconds
  m_checkJobLimitTimerId = startTimer(10000);
}

QueueLocal::~QueueLocal()
{
}

void QueueLocal::readSettings(QSettings &settings)
{
  Queue::readSettings(settings);
  m_cores = settings.value("cores", -1).toInt();

  // use all cores if no value set
  if(m_cores == -1){
    m_cores = QThread::idealThreadCount();
  }

  int numPendingJobs = settings.beginReadArray("PendingJobs");
  m_pendingJobQueue.reserve(numPendingJobs);
  for (int i = 0; i < numPendingJobs; ++i) {
    settings.setArrayIndex(i);
    IdType mqId = settings.value("moleQueueId", 0).value<IdType>();
    if (mqId == 0)
      continue;
    m_pendingJobQueue.append(mqId);
  }
  settings.endArray(); // "PendingJobs"
}

void QueueLocal::writeSettings(QSettings &settings) const
{
  Queue::writeSettings(settings);
  settings.setValue("cores", m_cores);

  QList<IdType> jobsToResume;
  jobsToResume.append(m_runningJobs.keys());
  jobsToResume.append(m_pendingJobQueue);
  settings.beginWriteArray("PendingJobs", jobsToResume.size());
  for (int i = 0; i < jobsToResume.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("moleQueueId", jobsToResume.at(i));
  }
  settings.endArray(); // "PendingJobs"
}

void QueueLocal::exportConfiguration(QSettings &exporter,
                                     bool includePrograms) const
{
  Queue::exportConfiguration(exporter, includePrograms);
  exporter.setValue("cores", m_cores);
}

void QueueLocal::importConfiguration(QSettings &importer, bool includePrograms)
{
  Queue::importConfiguration(importer, includePrograms);
  if (importer.contains("cores"))
    m_cores = importer.value("cores").toInt();
}

AbstractQueueSettingsWidget *QueueLocal::settingsWidget()
{
  LocalQueueWidget *widget = new LocalQueueWidget(this);
  return widget;
}

bool QueueLocal::submitJob(Job job)
{
  if (job.isValid()) {
    Job(job).setJobState(MoleQueue::Accepted);
    return prepareJobForSubmission(job);;
  }
  return false;
}

void QueueLocal::killJob(Job job)
{
  if (!job.isValid())
    return;

  int pendingIndex = m_pendingJobQueue.indexOf(job.moleQueueId());
  if (pendingIndex >= 0) {
    m_pendingJobQueue.removeAt(pendingIndex);
    job.setJobState(MoleQueue::Killed);
    return;
  }

  QProcess *process = m_runningJobs.take(job.moleQueueId());
  if (process != NULL) {
    m_jobs.remove(job.queueId());
    process->disconnect(this);
    process->terminate();
    process->deleteLater();
    job.setJobState(MoleQueue::Killed);
    return;
  }

  job.setJobState(MoleQueue::Killed);
}

bool QueueLocal::prepareJobForSubmission(Job &job)
{
  if (!writeInputFiles(job)) {
    Logger::logError(tr("Error while writing input files."), job.moleQueueId());
    job.setJobState(Error);
    return false;
  }
  if (!addJobToQueue(job))
    return false;

  return true;
}

void QueueLocal::processStarted()
{
  QProcess *process = qobject_cast<QProcess*>(sender());
  if (!process)
    return;

  IdType moleQueueId = m_runningJobs.key(process, 0);
  if (moleQueueId == 0)
    return;

  IdType queueId;
#ifdef WIN32
  queueId = static_cast<IdType>(process->pid()->dwProcessId);
#else // WIN32
  queueId = static_cast<IdType>(process->pid());
#endif // WIN32

  // Get pointer to jobmanager to lookup job
  if (!m_server) {
    Logger::logError(tr("Queue '%1' cannot locate Server instance!")
                     .arg(m_name), moleQueueId);
    return;
  }
  Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
  if (!job.isValid()) {
    Logger::logError(tr("Queue '%1' Cannot update invalid Job reference!")
                     .arg(m_name), moleQueueId);
    return;
  }
  job.setQueueId(queueId);
  job.setJobState(MoleQueue::RunningLocal);
}

void QueueLocal::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitCode);
  Q_UNUSED(exitStatus);

  QProcess *process = qobject_cast<QProcess*>(sender());
  if (!process)
    return;

  IdType moleQueueId = m_runningJobs.key(process, 0);
  if (moleQueueId == 0)
    return;

  // Remove and delete QProcess from queue
  m_runningJobs.take(moleQueueId)->deleteLater();

  // Get pointer to jobmanager to lookup job
  if (!m_server) {
    Logger::logError(tr("Queue '%1' cannot locate Server instance!")
                     .arg(m_name), moleQueueId);
    return;
  }
  Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
  if (!job.isValid()) {
    Logger::logDebugMessage(tr("Queue '%1' Cannot update invalid Job "
                               "reference!").arg(m_name), moleQueueId);
    return;
  }

  if (!job.outputDirectory().isEmpty() &&
      job.outputDirectory() != job.localWorkingDirectory()) {
    // copy function logs errors if needed
    if (!recursiveCopyDirectory(job.localWorkingDirectory(),
                                job.outputDirectory())) {
      job.setJobState(MoleQueue::Error);
      return;
    }
  }

  if (job.cleanLocalWorkingDirectory())
    cleanLocalDirectory(job);

  job.setJobState(MoleQueue::Finished);
}

int QueueLocal::maxNumberOfCores() const
{
  if (m_cores > 0)
    return m_cores;
  else
    return QThread::idealThreadCount();
}


bool QueueLocal::addJobToQueue(const Job &job)
{
  m_pendingJobQueue.append(job.moleQueueId());

  Job(job).setJobState(MoleQueue::LocalQueued);

  return true;
}

void QueueLocal::connectProcess(QProcess *proc)
{
  connect(proc, SIGNAL(started()),
          this, SLOT(processStarted()));
  connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)),
          this, SLOT(processFinished(int,QProcess::ExitStatus)));
  connect(proc, SIGNAL(error(QProcess::ProcessError)),
          this, SLOT(processError(QProcess::ProcessError)));
}

void QueueLocal::checkJobQueue()
{
  int coresInUse = 0;
  foreach(IdType moleQueueId, m_runningJobs.keys()) {
    const Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
    if (job.isValid())
      coresInUse += job.numberOfCores();
  }

  int totalCores = maxNumberOfCores();
  int coresAvailable = totalCores - coresInUse;

  // Keep submitting jobs (FIFO) until we hit one we can't afford to start.
  while (!m_pendingJobQueue.isEmpty() && coresAvailable > 0) {
    IdType nextMQId = m_pendingJobQueue.first();
    Job nextJob = m_server->jobManager()->lookupJobByMoleQueueId(nextMQId);
    if (!nextJob.isValid()) {
      m_pendingJobQueue.removeFirst();
      continue;
    }
    else if (nextJob.numberOfCores() <= coresAvailable) {
      m_pendingJobQueue.removeFirst();
      if (startJob(nextJob.moleQueueId()))
        coresAvailable -= nextJob.numberOfCores();
      continue;
    }

    // Cannot start next job yet!
    break;
  }
}

bool QueueLocal::startJob(IdType moleQueueId)
{
  // Get pointers to job, server, etc
  if (!m_server) {
    Logger::logError(tr("Queue '%1' cannot locate Server instance!")
                     .arg(m_name), moleQueueId);
    return false;
  }
  const Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
  if (!job.isValid()) {
    Logger::logError(tr("Queue '%1' cannot locate Job with MoleQueue id %2.")
                     .arg(m_name).arg(moleQueueId), moleQueueId);
    return false;
  }
  const Program *program = lookupProgram(job.program());
  if (!program) {
    Logger::logError(tr("Queue '%1' cannot locate Program '%2'.")
                     .arg(m_name).arg(job.program()), moleQueueId);
    return false;
  }

  // Create and setup process
  QProcess *proc = new QProcess (this);
  QDir dir (job.localWorkingDirectory());
  proc->setWorkingDirectory(dir.absolutePath());

  QStringList arguments;
  if (!program->arguments().isEmpty())
    arguments << program->arguments();

  QString command;

  // Set default command. May be overwritten later.
  if (program->useExecutablePath())
    command = program->executablePath() + "/" + program->executable();
  else
    command = program->executable();

  switch (program->launchSyntax()) {
  case Program::CUSTOM:
#ifdef WIN32
    command = "cmd.exe /c " + launchScriptName();
#else // WIN32
    command = "./" + launchScriptName();
#endif // WIN32
    break;
  case Program::PLAIN:
    break;
  case Program::INPUT_ARG:
    arguments << program->inputFilename();
    break;
  case Program::INPUT_ARG_NO_EXT:
    arguments << program->inputFilenameNoExtension();
    break;
  case Program::REDIRECT:
    proc->setStandardInputFile(dir.absoluteFilePath(program->inputFilename()));
    proc->setStandardOutputFile(dir.absoluteFilePath(program->outputFilename()));
    break;
  case Program::INPUT_ARG_OUTPUT_REDIRECT:
    arguments << program->inputFilename();
    proc->setStandardOutputFile(dir.absoluteFilePath(program->outputFilename()));
    break;
  case Program::SYNTAX_COUNT:
  default:
    Logger::logError(tr("Unknown launcher syntax for program %1: %2.")
                     .arg(job.program()).arg(program->launchSyntax()),
                     moleQueueId);
    return false;
  }

  connectProcess(proc);

  // Handle any keywords in the arguments
  QString args = arguments.join(" ");
  replaceLaunchScriptKeywords(args, job, false);

  proc->start(command + " " + args);
  Logger::logNotification(tr("Executing '%1 %2' in %3", "command, args, dir")
                          .arg(command).arg(args)
                          .arg(proc->workingDirectory()),
                          job.moleQueueId());
  m_runningJobs.insert(job.moleQueueId(), proc);

  return true;
}

void QueueLocal::timerEvent(QTimerEvent *theEvent)
{
  if (theEvent->timerId() == m_checkJobLimitTimerId) {
    checkJobQueue();
    theEvent->accept();
    return;
  }

  QObject::timerEvent(theEvent);
}

void QueueLocal::processError(QProcess::ProcessError error)
{
  QProcess *process = qobject_cast<QProcess*>(sender());
  if (!process)
    return;

  IdType moleQueueId = m_runningJobs.key(process, 0);
  if (moleQueueId == 0)
    return;

  // Remove and delete QProcess from queue
  m_runningJobs.take(moleQueueId)->deleteLater();

  if (!m_server) {
    Logger::logError(tr("Queue '%1' cannot locate Server instance!")
                     .arg(m_name), moleQueueId);
    return;
  }

  Job job = m_server->jobManager()->lookupJobByMoleQueueId(moleQueueId);
  if (!job.isValid()) {
    Logger::logDebugMessage(tr("Queue '%1' Cannot update invalid Job "
                               "reference!").arg(m_name), moleQueueId);
    return;
  }

  QString errorString = QueueLocal::processErrorToString(error);
  Logger::logError(tr("Execution of \'%1\' failed with process \'%2\': %3")
                      .arg(job.program()).arg(errorString)
                      .arg(process->errorString()), moleQueueId);

  job.setJobState(MoleQueue::Error);
}


/**
 * Convert a ProcessError value to a string.
 *
 * @param error ProcessError
 * @return C string
 */
QString QueueLocal::processErrorToString(QProcess::ProcessError error)
{
  switch(error)
  {
  case QProcess::FailedToStart:
    return tr("Failed to start");
  case QProcess::Crashed:
    return tr("Crashed");
  case QProcess::Timedout:
    return tr("Timed out");
  case QProcess::WriteError:
    return tr("Write error");
  case QProcess::ReadError:
    return tr("Read error");
  case QProcess::UnknownError:
    return tr("Unknown error");
  }

  Logger::logError(tr("Unrecognized Process Error: %1").arg(error));

  return tr("Unrecognized process error");
}

} // End namespace
