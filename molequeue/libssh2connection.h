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

#ifndef LIBSSH2CONNECTION_H_
#define LIBSSH2CONNECTION_H_

#include "sshconnection.h"
#include "libssh2operations.h"
#include "sshoperation.h"
#include <libssh2.h>
#include <QtCore/QObject>
#include <QTcpSocket>
#include <QtCore/QSocketNotifier>


namespace MoleQueue
{

//class AskPassword: public QObject
//{
//  Q_OBJECT
//public:
//  AskPassword(QObject *parentObject);
//
//signals:
//  void credentials(QString password);
//
//public slots:
//  virtual void ask() = 0;
//
//};

class AskPassword;

class LibSsh2Connection: public MoleQueue::SshConnection
{
  Q_OBJECT
public:
  LibSsh2Connection(const QString &hostName,
                    const QString &userName,
                    int port = 22,
                    AskPassword *askPass = 0,
                    QObject *parentObject = 0);

  void openSession();

  SshOperation *newCommand(const QString &commandString);
  SshOperation *newFileDownload(const QString &remoteFile,
                             const QString &localFile);
  SshOperation *newFileUpload(const QString &localFile,
                           const QString &remoteFile);
  SshOperation *newDirUpload(const QString &localDir,
                          const QString &remoteDir);
  SshOperation *newDirDownload(const QString &remoteDir,
                            const QString &localDir);
  SshOperation *newDirRemove(const QString &remoteDir);

  LIBSSH2_SESSION * session();

  int socket() {
    return sock;
  };

  void askKeyboardInterative(const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                             LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses);
signals:
  void sessionOpen();
  void responseSet();

private:
  QString m_hostName;
  int m_port;
  QString m_userName;
  LIBSSH2_SESSION *m_session;
  int sock;
  AskPassword *m_askPassword;
  SocketNotifier *m_readNotifier;
  SocketNotifier *m_writeNotifier;
  QString m_authList;
  LIBSSH2_USERAUTH_KBDINT_RESPONSE *m_responses;

private slots:
  void userAuth();
  void userAuthPassword(QString password);
  void userAuthKeyboardInterative();

  void askKeyboardInterative(const QString &passcode);
  void disableNotifiers();
  void waitForSocket(const char *slot);









};


}
#endif /* LIBSSH2CONNECTION_H_ */
