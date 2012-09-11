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

#include "sshcommandconnection.h"
#include "sshoperation.h"
#include "sshcommand.h"

namespace MoleQueue
{

SshCommandConnection::SshCommandConnection(const QString &sshCommand,
                                           const QString &scpCommand,
                                           QObject *parentObject) :
  SshConnection(parentObject), m_sshCommand(sshCommand),
  m_scpCommand(scpCommand)
{

}

SshOperation *SshCommandConnection::newCommand(const QString &command)
{
  return new SshExecCommand(this, command);
}

SshOperation *SshCommandConnection::newFileUpload(const QString &localFile,
                                                  const QString &remoteFile)
{
  return new SshFileUploadCommand(this, localFile, remoteFile);
}

SshOperation *SshCommandConnection::newFileDownload(const QString &remoteFile,
                                                    const QString &localFile)
{
  return new SshFileDownloadCommand(this, remoteFile, localFile);
}

SshOperation *SshCommandConnection::newDirUpload(const QString &localDir,
                                                 const QString &remoteDir)
{
  return new SshDirUploadCommand(this, localDir, remoteDir);
}

SshOperation *SshCommandConnection::newDirDownload(const QString &remoteDir,
                                                   const QString &localDir)
{
  return new SshDirDownloadCommand(this, remoteDir, localDir);
}

bool SshCommandConnection::isValid() const
{
  if (m_hostName.isEmpty())
    return false;
  else
    return true;
}


} /* namespace MoleQueue */
