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

#ifndef SSHCOMMANDCONNECTION_H_
#define SSHCOMMANDCONNECTION_H_

#include "sshconnection.h"

namespace MoleQueue
{

class SshCommandConnection : public SshConnection
{
  Q_OBJECT
public:
  SshCommandConnection(const QString &sshCommand,
                       const QString &scpCommand,
                       QObject *parentObject = 0);

  /** \return The SSH command that will be run. */
  QString sshCommand() { return m_sshCommand; }

  /** \return The SCP command that will be run. */
  QString scpCommand() { return m_scpCommand; }

  SshOperation *newCommand(const QString &command);

  SshOperation *newFileUpload(const QString &localFile,
                              const QString &remoteFile);

  SshOperation *newFileDownload(const QString &remoteFile,
                                const QString &localFile);

  SshOperation *newDirUpload(const QString &localDir,
                             const QString &remoteDir);

  SshOperation *newDirDownload(const QString &remoteDir,
                               const QString &localDir);

  virtual bool isValid() const;

  /// @return the arguments to be passed to the SSH command.
  virtual QStringList sshArgs() = 0;

  /// @return the arguments to be passed to the SCP command.
  virtual QStringList scpArgs() = 0;

  /**
   * Set the SSH command for the class. Defaults to 'ssh', and would execute
   * the SSH commnand in the user's path.
   */
  void setSshCommand(const QString &command) { m_sshCommand = command; }

  /**
   * Set the SCP command for the class. Defaults to 'scp', and would execute
   * the SCP commnand in the user's path.
   */
  void setScpCommand(const QString &command) { m_scpCommand = command; }

protected:

  QString m_sshCommand;
  QString m_scpCommand;

};

} /* namespace MoleQueue */

#endif /* SSHCOMMANDCONNECTION_H_ */
