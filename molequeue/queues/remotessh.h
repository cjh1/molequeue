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

#ifndef QUEUEREMOTESSH_H
#define QUEUEREMOTESSH_H

#include "remote.h"
#include "sshcommandconnection.h"

class QTimer;

namespace MoleQueue
{
class QueueManager;
class SshConnection;


/// @brief QueueRemote subclass for interacting with a generic Remote queue
///        over SSH.
class QueueRemoteSsh : public QueueRemote
{
  Q_OBJECT
public:
  explicit QueueRemoteSsh(const QString &queueName = "AbstractRemoteSsh",
                       QueueManager *parentManager = 0);
  ~QueueRemoteSsh();

  /**
   * Read settings for the queue, done early on at startup.
   */
  void readSettings(QSettings &settings);

  /**
   * Write settings for the queue, done just before closing the server.
   */
  void writeSettings(QSettings &settings) const;

  /**
   * Populate the passed QSettings object with this queue's configuration.
   * Sensitive data (such as usernames, etc) and mutatable state data (like
   * current jobs) are not written, see writeSettings() if these are needed.
   * @param includePrograms Export this queue's programs as well. Default: true
   */
  void exportConfiguration(QSettings &exporter,
                           bool includePrograms = true) const;

  /**
   * Set this Queue's configuration from the passed QSettings object.
   * Sensitive data (such as usernames, etc) and mutatable state data (like
   * current jobs) are not read, see writeSettings() if these are needed.
   * @param includePrograms Import any programs contained in the importer.
   * Default: true
   */
  void importConfiguration(QSettings &importer,
                           bool includePrograms = true);

  void setSshExecutable(const QString &exe);
  QString sshExecutable() const;
  void setScpExecutable(const QString &exe);
  QString scpExectuable() const;

  void setHostName(const QString &host)
  {
    bool recreate = host != m_hostName;
    m_hostName = host;

    if(recreate)
      recreateConnection();
  }

  QString hostName() const
  {
    return m_hostName;
  }

  void setUserName(const QString &user)
  {
    bool recreate = user != m_userName;
    m_userName = user;

    if(recreate)
      recreateConnection();
  }

  QString userName() const
  {
    return m_userName;
  }

  void setIdentityFile(const QString &identity)
  {
    bool recreate = identity != m_identityFile;
    m_identityFile = identity;

    if(recreate)
      recreateConnection();
  }

  QString identityFile() const
  {
    return m_identityFile;
  }

  void setSshPort(int port)
  {
    bool recreate = port != m_sshPort;
    m_sshPort = port;

    if(recreate)
      recreateConnection();
  }

  int sshPort() const
  {
    return m_sshPort;
  }

  void setSubmissionCommand(const QString &command)
  {
    m_submissionCommand = command;
  }

  QString submissionCommand() const
  {
    return m_submissionCommand;
  }

  void setKillCommand(const QString &command)
  {
    m_killCommand = command;
  }

  QString killCommand() const
  {
    return m_killCommand;
  }

  void setRequestQueueCommand(const QString &command)
  {
    m_requestQueueCommand = command;
  }

  QString requestQueueCommand() const
  {
    return m_requestQueueCommand;
  }

  void setUseLibSsh2(bool use) {

    bool recreate = use != m_useLibSsh2;
    m_useLibSsh2 = use;

    if(recreate)
      recreateConnection();
  }

  bool useLibSsh2() {
    return m_useLibSsh2;
  }

  virtual AbstractQueueSettingsWidget* settingsWidget();

  void createConnection();
  void recreateConnection();

public slots:
  void requestQueueUpdate();

protected slots:

  void createRemoteDirectory(MoleQueue::Job job);
  void remoteDirectoryCreated();
  void copyInputFilesToHost(MoleQueue::Job job);
  void inputFilesCopied();
  void submitJobToRemoteQueue(MoleQueue::Job job);
  void jobSubmittedToRemoteQueue();
  void handleQueueUpdate();

  void beginFinalizeJob(MoleQueue::IdType queueId);
  void finalizeJobCopyFromServer(MoleQueue::Job job);
  void finalizeJobOutputCopiedFromServer();
  void finalizeJobCopyToCustomDestination(MoleQueue::Job job);
  void finalizeJobCleanup(MoleQueue::Job job);

  void cleanRemoteDirectory(MoleQueue::Job job);
  void remoteDirectoryCleaned();

  void beginKillJob(MoleQueue::Job job);
  void endKillJob();

protected:
  /**
   * Extract the job id from the submission output. Reimplement this in derived
   * classes.
   * @param submissionOutput Output from m_submissionCommand
   * @param queueId The queuing system's job id.
   * @return True if parsing successful, false otherwise.
   */
  virtual bool parseQueueId(const QString &submissionOutput, IdType *queueId) = 0;

  /**
   * Prepare the command to check the remote queue. The default implementation
   * is m_requestQueueCommand followed by the owned job ids separated by
   * spaces.
   */
  virtual QString generateQueueRequestCommand();

  /**
   * Extract the queueId and JobState from a single line of the the queue list
   * output. Reimplement this in derived classes.
   * @param queueListOutput Single line of output from m_requestQueueCommand
   * @param queueId The queuing system's job id.
   * @param state The state of the job with id queueId
   * @return True if parsing successful, false otherwise.
   */
  virtual bool parseQueueLine(const QString &queueListOutput, IdType *queueId,
                              MoleQueue::JobState *state) = 0;

  QString m_sshExecutable;
  QString m_scpExecutable;
  QString m_hostName;
  QString m_userName;
  QString m_identityFile;
  int m_sshPort;

  bool m_isCheckingQueue;

  QString m_submissionCommand;
  QString m_killCommand;
  QString m_requestQueueCommand;

  /// List of allowed exit codes for m_requestQueueCommand. This is required for
  /// e.g. PBS/Torque, which return 153 if you request the status of a job that
  /// has completed.
  QList<int> m_allowedQueueRequestExitCodes;

  SshConnection *m_sshConnection;
  bool m_useLibSsh2;

};

} // End namespace

#endif // QUEUEREMOTESSH_H
