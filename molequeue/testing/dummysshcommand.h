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

#ifndef DUMMYSSHCOMMAND_H
#define DUMMYSSHCOMMAND_H

#include "opensshcommandconnection.h"
#include "sshcommand.h"
#include "sshoperation.h"

#include <QtCore/QVariant>
#include <qdebug.h>

class DummySshFileUploadOperation : public MoleQueue::SshFileUploadCommand
{
  Q_OBJECT

public:
  DummySshFileUploadOperation(MoleQueue::SshCommandConnection *connection,
                              const QString &localFile,
                              const QString &remoteFile);

  QString getDummyCommand() const { return m_dummyCommand; }
  QStringList getDummyArgs() const { return m_dummyArgs; }
  void setDummyExitCode(int exitCode) {
    m_errorCode = exitCode;
  };
  void setDummyOutput(const QString &out) {
    m_output = out;
  }
  void emitDummyComplete() { emit complete(); }

protected:
  void sendRequest(const QString &command, const QStringList &args)
  {
    m_dummyCommand = command;
    m_dummyArgs = args;
  }

  QString m_dummyCommand;
  QStringList m_dummyArgs;
};

class DummySshFileDownloadOperation : public MoleQueue::SshFileDownloadCommand
{
  Q_OBJECT

public:
  DummySshFileDownloadOperation(MoleQueue::SshCommandConnection *connection,
                                const QString &remoteFile,
                                const QString &localFile);

  QString getDummyCommand() const { return m_dummyCommand; }
  QStringList getDummyArgs() const { return m_dummyArgs; }
  void setDummyExitCode(int exitCode) {
    m_errorCode = exitCode;
  };
  void setDummyOutput(const QString &out) {
    m_output = out;
  }
  void emitDummyComplete() { emit complete(); }

protected:
  void sendRequest(const QString &command, const QStringList &args)
  {
    m_dummyCommand = command;
    m_dummyArgs = args;
  }

  QString m_dummyCommand;
  QStringList m_dummyArgs;
};

class DummySshDirUploadOperation : public MoleQueue::SshDirUploadCommand
{
  Q_OBJECT

public:
  DummySshDirUploadOperation(MoleQueue::SshCommandConnection *connection,
                              const QString &localDir,
                              const QString &remoteDir);

  QString getDummyCommand() const { return m_dummyCommand; }
  QStringList getDummyArgs() const { return m_dummyArgs; }
  void setDummyExitCode(int exitCode) {
    m_errorCode = exitCode;
  };
  void setDummyOutput(const QString &out) {
    m_output = out;
  }
  void emitDummyComplete() { emit complete(); }

protected:
  void sendRequest(const QString &command, const QStringList &args)
  {
    m_dummyCommand = command;
    m_dummyArgs = args;
  }

  QString m_dummyCommand;
  QStringList m_dummyArgs;
};

class DummySshDirDownloadOperation : public MoleQueue::SshDirDownloadCommand
{
  Q_OBJECT

public:
  DummySshDirDownloadOperation(MoleQueue::SshCommandConnection *connection,
                                const QString &remoteDir,
                                const QString &localDir);

  QString getDummyCommand() const { return m_dummyCommand; }
  QStringList getDummyArgs() const { return m_dummyArgs; }
  void setDummyExitCode(int exitCode) {
    m_errorCode = exitCode;
  };
  void setDummyOutput(const QString &out) {
    m_output = out;
  }
  void emitDummyComplete() { emit complete(); }

protected:
  void sendRequest(const QString &command, const QStringList &args)
  {
    m_dummyCommand = command;
    m_dummyArgs = args;
  }

  QString m_dummyCommand;
  QStringList m_dummyArgs;
};


class DummySshExecOperation : public MoleQueue::SshExecCommand
{
  Q_OBJECT

public:
  DummySshExecOperation(MoleQueue::SshCommandConnection *connection,
                                const QString &command);

  QString getDummyCommand() const { return m_dummyCommand; }
  QStringList getDummyArgs() const { return m_dummyArgs; }
  void setDummyExitCode(int exitCode) {
    m_errorCode = exitCode;
  };
  void setDummyOutput(const QString &out) {
    m_output = out;
  }
  void emitDummyComplete() { emit complete(); }

protected:
  void sendRequest(const QString &command, const QStringList &args)
  {
    qDebug() << "sendRequest";
    m_dummyCommand = command;
    m_dummyArgs = args;
  }

  QString m_dummyCommand;
  QStringList m_dummyArgs;
};

/// SshConnection implementation that doesn't actually call external processes.
class DummySshConnection : public MoleQueue::OpenSshCommandConnection
{
  Q_OBJECT
public:
  DummySshConnection(QObject *parentObject = NULL);

  ~DummySshConnection();

  MoleQueue::SshOperation *newCommand(const QString &command);
  MoleQueue::SshOperation *newFileUpload(const QString &localFile,
                                         const QString &remoteFile);
  MoleQueue::SshOperation *newFileDownLoad(const QString &remoteFile,
                                           const QString &localFile);
  MoleQueue::SshOperation *newDirUpload(const QString &localDir,
                                        const QString &remoteDir);
  MoleQueue::SshOperation *newDirDownload(const QString &remoteDir,
                                          const QString &localDir);

  MoleQueue::SshOperation *getDummyOperation() {
    return m_dummyOperation;
  };

protected:
  MoleQueue::SshOperation *m_dummyOperation;

  //QString m_dummyCommand;
  //QStringList m_dummyArgs;
};

#endif // DUMMYSSHCOMMAND_H
