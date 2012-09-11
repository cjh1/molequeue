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

#include "sshcommand.h"
#include "sshcommandconnection.h"


#include "logger.h"
#include "terminalprocess.h"

#include <QtCore/QProcessEnvironment>
#include <QtCore/QDir>
#include <QtCore/QDebug>

namespace MoleQueue {

SshCommand::SshCommand(SshCommandConnection *connection) :
  SshOperation(connection),
  m_process(0),
  m_isComplete(true),
  m_commandConnection(connection)
{
  m_errorCode = -1;
}

SshCommand::~SshCommand()
{
  delete m_process;
  m_process = 0;
}

bool SshCommand::waitForCompletion(int msecs)
{
  if (!m_process)
    return false;

  if (m_process->state() == QProcess::Starting)
    m_process->waitForStarted(msecs);

  if (m_isComplete)
    return true;

  return m_process->waitForFinished(msecs);
}

bool SshCommand::isComplete() const
{
  return m_isComplete;
}

SshTransferCommand::SshTransferCommand(SshCommandConnection *connection,
                                       const QString &localPath,
                                       const QString &remotePath) :
  SshCommand(connection), m_localPath(localPath), m_remotePath(remotePath)
{

}


SshFileUploadCommand::SshFileUploadCommand(SshCommandConnection *connection,
                                           const QString &localFile,
                                           const QString &remoteFile) :
  SshTransferCommand(connection, localFile, remoteFile)
{

}

bool SshFileUploadCommand::copyTo(const QString &localFile, const QString &remoteFile)
{
  if (!m_commandConnection->isValid())
    return false;

  QStringList args = m_commandConnection->scpArgs();
  QString remoteFileSpec = remoteSpec() + ":" + remoteFile;
  args << localFile << remoteFileSpec;

  qDebug() << "args: " << args;

  sendRequest(m_commandConnection->scpCommand(), args);

  return true;
}

bool SshFileUploadCommand::execute()
{
  return copyTo(m_localPath, m_remotePath);
}


SshFileDownloadCommand::SshFileDownloadCommand(SshCommandConnection *connection,
                                               const QString &remoteFile,
                                               const QString &localFile) :
  SshTransferCommand(connection, localFile, remoteFile)
{

}

bool SshFileDownloadCommand::copyFrom(const QString &remoteFile, const QString &localFile)
{
  if (!m_commandConnection->isValid())
    return false;

  QStringList args = m_commandConnection->scpArgs();
  QString remoteFileSpec = remoteSpec() + ":" + remoteFile;
  args << remoteFileSpec << localFile;

  sendRequest(m_commandConnection->scpCommand(), args);

  return true;
}

bool SshFileDownloadCommand::execute()
{
  return copyFrom(m_remotePath, m_localPath);
}


SshDirUploadCommand::SshDirUploadCommand(SshCommandConnection *connection,
                                         const QString &localDir,
                                         const QString &remoteDir) :
  SshTransferCommand(connection, localDir, remoteDir)
{

}


bool SshDirUploadCommand::copyDirTo(const QString &localDir, const QString &remoteDir)
{
  if (!m_commandConnection->isValid())
    return false;

  QStringList args = m_commandConnection->scpArgs();
  QString remoteDirSpec = remoteSpec() + ":" + remoteDir;
  args << "-r" << localDir << remoteDirSpec;

  sendRequest(m_commandConnection->scpCommand(), args);

  qDebug() << "args: " << args;

  return true;
}

bool SshDirUploadCommand::execute()
{
  return copyDirTo(m_localPath, m_remotePath);

}

SshDirDownloadCommand::SshDirDownloadCommand(SshCommandConnection *connection,
                                             const QString &remoteDir,
                                             const QString &localDir) :
  SshTransferCommand(connection, localDir, remoteDir)
{

}

bool SshDirDownloadCommand::copyDirFrom(const QString &remoteDir, const QString &localDir)
{
  if (!m_commandConnection->isValid())
    return false;

  QDir local(localDir);
  if (!local.exists())
    local.mkpath(localDir); /// @todo Check for failure of mkpath

  QStringList args = m_commandConnection->scpArgs();
  QString remoteDirSpec = remoteSpec() + ":" + remoteDir;
  args << "-r" << remoteDirSpec << localDir;

  sendRequest(m_commandConnection->scpCommand(), args);

  return true;
}

bool SshDirDownloadCommand::execute()
{
  return copyDirFrom(m_remotePath, m_localPath);
}

void SshCommand::processStarted()
{
  m_process->closeWriteChannel();
}

void SshCommand::processFinished()
{
  m_output = m_process->readAll();
  m_errorCode = m_process->exitCode();
  m_process->close();

  if (debug()) {
    Logger::logDebugMessage(tr("SSH finished (%1) Exit code: %2\n%3")
                            .arg(reinterpret_cast<quint64>(this))
                            .arg(m_errorCode).arg(m_output));
  }

  m_isComplete = true;
  emit complete();
}

void SshCommand::sendRequest(const QString &command, const QStringList &args)
{
  qDebug() << "SshCommand::sendRequest(" << command;
  if (!m_process)
    initializeProcess();

  m_isComplete = false;

  if (debug()) {
    Logger::logDebugMessage(tr("SSH request (%1): %2 %3")
                            .arg(reinterpret_cast<quint64>(this))
                            .arg(command).arg(args.join((" "))));
  }

  m_process->start(command, args);
}

void SshCommand::initializeProcess()
{
  // Initialize the environment for the process, set merged channels.
  if (!m_process)
    m_process = new TerminalProcess(this);
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  QProcessEnvironment sshEnv;
  if (env.contains("DISPLAY"))
    sshEnv.insert("DISPLAY", env.value("DISPLAY"));
  if (env.contains("EDITOR"))
    sshEnv.insert("EDITOR", env.value("EDITOR"));
  if (env.contains("SSH_AUTH_SOCK"))
    sshEnv.insert("SSH_AUTH_SOCK", env.value("SSH_AUTH_SOCK"));
  if (env.contains("KRB5CCNAME"))
    sshEnv.insert("KRB5CCNAME", env.value("KRB5CCNAME"));
  if (env.contains("SSH_ASKPASS"))
    sshEnv.insert("SSH_ASKPASS", env.value("SSH_ASKPASS"));
  m_process->setProcessEnvironment(sshEnv);
  m_process->setProcessChannelMode(QProcess::MergedChannels);

  connect(m_process, SIGNAL(started()), this, SLOT(processStarted()));
  connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
          this, SLOT(processFinished()));
}

QString SshCommand::remoteSpec()
{
  return m_commandConnection->userName().isEmpty() ?
         m_commandConnection->hostName() :
         m_commandConnection->userName() + "@"
         + m_commandConnection->hostName();
}

bool SshCommand::debug()
{
  const char *val = qgetenv("MOLEQUEUE_DEBUG_SSH");
  return (val != NULL && val[0] != '\0');
}

SshExecCommand::SshExecCommand(SshCommandConnection *connection,
                           const QString &command) :
  SshCommand(connection), m_command(command)
{

}

bool SshExecCommand::execute(const QString &command)
{
  if (!m_commandConnection->isValid())
    return false;

  QStringList args = m_commandConnection->sshArgs();
  args << remoteSpec() << command;

  sendRequest(m_commandConnection->sshCommand(), args);

  return true;
}

bool SshExecCommand::execute()
{
  return execute(m_command);
}

} // End namespace
