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

#ifndef SSHCOMMAND_H
#define SSHCOMMAND_H

#include "sshcommandconnection.h"
#include "sshoperation.h"

#include <QtCore/QStringList>

namespace MoleQueue {

class TerminalProcess;


/**
 * @class SshCommand sshcommand.h <molequeue/sshcommand.h>
 * @brief Abstract subclass of SshOperation providing base implementaton using
 * commandline ssh/scp.
 *
 * @author Marcus D. Hanwell, David C. Lonie, Chris Harris
 *
 * The SshCommand provides an base implementation of the SshOperation interface
 * that calls the commandline ssh and scp executables in a TerminalProcess.
 *
 * When writing code that needs ssh functionality, the code should use the
 * SshConnection interface instead.
 */
class SshCommand : public SshOperation
{
  Q_OBJECT

public:
  SshCommand(SshCommandConnection *connection);
  ~SshCommand();

  /**
   * Wait until the request has been completed.
   *
   * @param msecs Timeout in milliseconds. Default is 30 seconds.
   *
   * @return True if request finished, false on timeout.
   */
  bool waitForCompletion(int msecs = 30000);

  /** @return True if the request has completed. False otherwise. */
  bool isComplete() const;

protected slots:

  /// Called when the TerminalProcess enters the Running state.
  void processStarted();

  /// Called when the TerminalProcess exits the Running state.
  void processFinished();

protected:

  /// Send a request. This launches the process and connects the completion
  /// signals
  virtual void sendRequest(const QString &command, const QStringList &args);

  /// Initialize the TerminalProcess object.
  void initializeProcess();

  /// @return the remote specification, e.g. "user@host" or "host"
  QString remoteSpec();

  static bool debug();

  TerminalProcess *m_process;
  bool m_isComplete;
  SshCommandConnection *m_commandConnection;
};

class SshTransferCommand : public SshCommand
{
  Q_OBJECT

public:
  SshTransferCommand(SshCommandConnection *connection,
                     const QString &localPath,
                     const QString &remotePath);

protected:
  QString m_localPath;
  QString m_remotePath;
};


class SshFileUploadCommand : public SshTransferCommand
{
  Q_OBJECT
public:
  SshFileUploadCommand(SshCommandConnection *connection,
                       const QString &localFile,
                       const QString &remoteFile);

public slots:

  /**
   * Copy a local file to the remote system.
   *
   * \note The command is executed asynchronously, see requestComplete() or
   * waitForCompletion() for results.
   *
   * \sa requestSent() requestCompleted() waitForCompeletion()
   *
   * \param localFile The path of the local file.
   * \param remoteFile The path of the file on the remote system.
   * \return True on success, false on failure.
   */
  virtual bool copyTo(const QString &localFile, const QString &remoteFile);
  bool execute();

};

class SshFileDownloadCommand : public SshTransferCommand
{
  Q_OBJECT
public:
  SshFileDownloadCommand(SshCommandConnection *connection,
                         const QString &remoteFile,
                         const QString &localFile);

public slots:
  /**
   * Copy a remote file to the local system.
   *
   * \note The command is executed asynchronously, see requestComplete() or
   * waitForCompletion() for results.
   *
   * \sa requestSent() requestCompleted() waitForCompeletion()
   *
   * \param remoteFile The path of the file on the remote system.
   * \param localFile The path of the local file.
   * \return True on success, false on failure.
   */
  virtual bool copyFrom(const QString &remoteFile, const QString &localFile);
  bool execute();
};

class SshDirUploadCommand : public SshTransferCommand
{
  Q_OBJECT
public:
  SshDirUploadCommand(SshCommandConnection *connection,
                      const QString &localDir,
                      const QString &remoteDir);

public slots:
  /**
   * Copy a local directory recursively to the remote system.
   *
   * \note The command is executed asynchronously, see requestComplete() or
   * waitForCompletion() for results.
   *
   * \sa requestSent() requestCompleted() waitForCompeletion()
   *
   * \param localDir The path of the local directory.
   * \param remoteDir The path of the directory on the remote system.
   * \return True on success, false on failure.
   */
  virtual bool copyDirTo(const QString &localDir, const QString &remoteDir);
  bool execute();
};

class SshDirDownloadCommand : public SshTransferCommand
{
  Q_OBJECT
public:
  SshDirDownloadCommand(SshCommandConnection *connection,
                        const QString &remoteDir,
                        const QString &localDir);

public slots:
  /**
   * Copy a remote directory recursively to the local system.
   *
   * \note The command is executed asynchronously, see requestComplete() or
   * waitForCompletion() for results.
   *
   * \sa requestSent() requestCompleted() waitForCompeletion()
   *
   * \param remoteDir The path of the directory on the remote system.
   * \param localFile The path of the local directory.
   * \return True on success, false on failure.
   */
  virtual bool copyDirFrom(const QString &remoteDir, const QString &localDir);
  bool execute();
};

class SshExecCommand : public SshCommand
{
  Q_OBJECT
public:
  SshExecCommand(SshCommandConnection *connection, const QString &command);

public slots:
  /**
   * Execute the supplied command on the remote host.
   *
   * \note The command is executed asynchronously, see requestComplete() or
   * waitForCompletion() for results.
   *
   * \sa requestSent() requestCompleted() waitForCompeletion()
   *
   * \param command The command to execute.
   * \return True on success, false on failure.
   */
  virtual bool execute(const QString &command);
  bool execute();

private:
  QString m_command;
};

} // End namespace

#endif
