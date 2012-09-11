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

#include "dummysshcommand.h"
#include "sshcommandconnection.h"

using namespace MoleQueue;

DummySshFileUploadOperation::DummySshFileUploadOperation(
    MoleQueue::SshCommandConnection *connection,
    const QString &localFile,
    const QString &remoteFile) :
  SshFileUploadCommand(connection, localFile, remoteFile)
{

}

DummySshFileDownloadOperation::DummySshFileDownloadOperation(
    MoleQueue::SshCommandConnection *connection,
    const QString &remoteFile,
    const QString &localFile) :
  SshFileDownloadCommand(connection, remoteFile, localFile)
{

}

DummySshDirUploadOperation::DummySshDirUploadOperation(
    MoleQueue::SshCommandConnection *connection,
    const QString &localDir,
    const QString &remoteDir) :
  SshDirUploadCommand(connection, localDir, remoteDir)
{

}

DummySshDirDownloadOperation::DummySshDirDownloadOperation(
    MoleQueue::SshCommandConnection *connection,
    const QString &remoteDir,
    const QString &localDir) :
  SshDirDownloadCommand(connection, remoteDir, localDir)
{

}

DummySshExecOperation::DummySshExecOperation(
    MoleQueue::SshCommandConnection *connection, const QString &command) :
  SshExecCommand(connection, command)
{

}

DummySshConnection::DummySshConnection(QObject *parentObject)
  : MoleQueue::OpenSshCommandConnection(parentObject)
{
}

DummySshConnection::~DummySshConnection()
{
}


SshOperation *DummySshConnection::newCommand(const QString &command)
{
  m_dummyOperation = new DummySshExecOperation(this, command);
  return m_dummyOperation;
}

SshOperation *DummySshConnection::newFileUpload(const QString &localFile,
                                                const QString &remoteFile)
{
  m_dummyOperation = new DummySshFileUploadOperation(this, localFile,
                                                     remoteFile);
  return m_dummyOperation;
}

SshOperation *DummySshConnection::newFileDownLoad(const QString &remoteFile,
                                                  const QString &localFile)
{
  m_dummyOperation = new DummySshFileDownloadOperation(this, remoteFile,
                                                       localFile);
  return m_dummyOperation;
}

SshOperation *DummySshConnection::newDirUpload(const QString &localDir,
                                               const QString &remoteDir)
{
  m_dummyOperation = new DummySshDirUploadOperation(this, localDir, remoteDir);
  return m_dummyOperation;
}

SshOperation *DummySshConnection::newDirDownload(const QString &remoteDir,
                                                 const QString &localDir)
{
  m_dummyOperation = new DummySshDirDownloadOperation(this, remoteDir,
                                                      localDir);
  return m_dummyOperation;
}
