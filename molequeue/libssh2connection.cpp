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

#include "libssh2connection.h"
#include "libssh2operations.h"
#include "libssh2copyoperation.h"
#include "askpassword.h"

#include <QDebug>
#include <QHostInfo>
#include <QList>
#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>

//#include "libssh2_config.h"


#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_ARPA_INET_H 1

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

namespace MoleQueue
{


extern "C" LibSsh2Connection *conn = NULL;

extern "C" void kbd_callback(const char *name, int name_len,
                             const char *instruction, int instruction_len, int num_prompts,
                             const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                             LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                             void **abstract)
{
  (void)abstract;

    printf("callback\n");

    int i;
    size_t n;
    char buf[1024];
    (void)abstract;

    if(prompts == NULL)
      return;

    fwrite(prompts[0].text, 1, prompts[0].length, stdout);
    printf("\n");

    conn->askKeyboardInterative(prompts, responses);

    QEventLoop loop;

    loop.connect(conn, SIGNAL(responseSet()),
                 SLOT(quit()));
    loop.exec();
}



LibSsh2Connection::LibSsh2Connection(const QString &hostName,
                                     const QString &userName,
                                     int port,
                                     AskPassword *askPass,
                                     QObject *parentObject) :
  SshConnection(parentObject), m_hostName(hostName), m_userName(userName),
  m_port(port), sock(NULL), m_session(NULL), m_askPassword(askPass),
  m_readNotifier(NULL), m_writeNotifier(NULL), m_responses(NULL),
  m_sftp_session(NULL)
{

}

void LibSsh2Connection::openSession()
{
  if(m_session != NULL) {
    emit sessionOpen();
    return;
  }

  int i, auth_pw = 1;
  struct sockaddr_in sin;
  const char *fingerprint;
  struct timeval start;
  int rc;


#ifdef WIN32
  WSADATA wsadata;

  WSAStartup(MAKEWORD(2,0), &wsadata);
#endif

  rc = libssh2_init(0);

  if (rc != 0) {
    fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
    return;
  }

  /* Ultra basic "connect to port 22 on localhost"
   * Your code is responsible for creating the socket establishing the
   * connection
   */
  QList<QHostAddress> addresses = QHostInfo::fromName(m_hostName).addresses();
  // Check for error ...
  QByteArray addr = addresses[0].toString().toLocal8Bit();
  unsigned long hostaddr = inet_addr(addr.constData());
  sock = ::socket(AF_INET, SOCK_STREAM, 0);


  sin.sin_family = AF_INET;
  sin.sin_port = htons(m_port);
  sin.sin_addr.s_addr = hostaddr;
  if (::connect(sock, (struct sockaddr*) (&sin), sizeof(struct sockaddr_in))
      != 0) {
    fprintf(stderr, "failed to connect!\n");
    return;
  }

  qDebug() << "libssh2_session_init";
  /* Create a session instance */
  m_session = libssh2_session_init();


  if (!m_session) {
    qDebug() << "can't create session";
    return;
  }

  //libssh2_trace(m_session, LIBSSH2_TRACE_CONN | LIBSSH2_TRACE_ERROR | LIBSSH2_TRACE_SFTP);

  /* Since we have set non-blocking, tell libssh2 we are non-blocking */
  libssh2_session_set_blocking(m_session, 0);

  gettimeofday(&start, NULL);

  /* ... start it up. This will trade welcome banners, exchange keys,
   * and setup crypto, compression, and MAC layers
   */
  while ((rc = libssh2_session_handshake(m_session, sock)) ==

  LIBSSH2_ERROR_EAGAIN)
    ;
  if (rc) {
    fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
    return;
  }

  /* At this point we havn't yet authenticated.  The first thing to do
   * is check the hostkey's fingerprint against our known hosts Your app
   * may have it hard coded, may go to a file, may present it to the
   * user, that's your call
   */
  fingerprint = libssh2_hostkey_hash(m_session, LIBSSH2_HOSTKEY_HASH_SHA1);

  fprintf(stderr, "Fingerprint: ");
  for (i = 0; i < 20; i++) {
    fprintf(stderr, "%02X ", (unsigned char) fingerprint[i]);
  }
  fprintf(stderr, "\n");

  userAuth();
}

void LibSsh2Connection::userAuth()
{
  qDebug() << "userAuth";

  QByteArray user = m_userName.toLocal8Bit();
  const char *username = user.constData();

  int rc ;
  char *userauthlist;
  while((userauthlist = libssh2_userauth_list(m_session, username,
                                              strlen(username))) == NULL &&
        (rc = libssh2_session_last_errno(m_session)) ==  LIBSSH2_ERROR_EAGAIN);

  if (rc != LIBSSH2_ERROR_EAGAIN) {
    qDebug() << "Error: " << rc;
    // shutdown
    return;
  }

  m_authList = QString(userauthlist);
  qDebug() << "auth" << m_authList;

  if(m_authList.contains("keyboard-interactive")) {

    userAuthKeyboardInterative();

  } else if(m_authList.contains("password")) {
    userAuthPassword("");
  }
}

void LibSsh2Connection::userAuthPassword(QString password)
{
  int rc;

  if(password.length() == 0) {
    connect(m_askPassword, SIGNAL(entered(const QString&)),
            this, SLOT(userAuthPassword(QString)));

    m_askPassword->ask(tr("%1@%2").arg(m_userName).arg(m_hostName),
                       "Password:");
    return;
  }

  QByteArray pass = password.toLocal8Bit();
  QByteArray user = m_userName.toLocal8Bit();

  while ((rc = libssh2_userauth_password(m_session,
                                         user.constData(),
                                         pass.constData())) ==
         LIBSSH2_ERROR_EAGAIN);

  if (rc) {
     qDebug() << "error : " << rc;
     m_askPassword->incorrect();
     fprintf(stderr, "Authentication by password failed.\n");
     // shutdown session here ...
     m_session = NULL;
     openSession();
      //goto shutdown;
     return;
  }

  m_askPassword->correct();

  emit sessionOpen();

}


void LibSsh2Connection::userAuthKeyboardInterative()
{
  qDebug() << "interative";

  if(libssh2_userauth_authenticated(m_session))
    return;

  disableNotifiers();

  conn = this;

  QByteArray user = m_userName.toLocal8Bit();
  int rc = libssh2_userauth_keyboard_interactive(m_session,
                                                 user.data(),
                                                 &kbd_callback);

  qDebug() << "rc: " << rc;

  qDebug() << "auth:" << libssh2_userauth_authenticated(m_session);

  if(rc == LIBSSH2_ERROR_EAGAIN) {
    qDebug() << "wait for socket";
    waitForSocket(SLOT(userAuthKeyboardInterative()));
    return;
  }


  if(rc != 0){
    if(rc == LIBSSH2_ERROR_AUTHENTICATION_FAILED) {
      qDebug() << "auth falled!";
      userAuthKeyboardInterative();
    }
    else {
      qDebug() << "error:" << rc;
    }

    return;
  }

  m_askPassword->correct();

  emit sessionOpen();
}

SshOperation *LibSsh2Connection::newCommand(const QString &command)
{
  LibSsh2Operation *op = new CommandOperation(this, command);
  return op;
}

SshOperation *LibSsh2Connection::newFileDownload(const QString &remoteFile,
                                              const QString &localFile)
{
  LibSsh2Operation *op = new FileDownload(this, localFile, remoteFile);
  return op;
}

SshOperation *LibSsh2Connection::newFileUpload(const QString &localFile,
                                            const QString &remoteFile)
{
  LibSsh2Operation *op = new FileUpload(this, localFile, remoteFile);
  return op;
}

SshOperation *LibSsh2Connection::newDirUpload(const QString &localDir,
                                           const QString &remoteDir)
{
  LibSsh2Operation *op = new DirUpload(this, localDir, remoteDir);
  return op;
}

SshOperation *LibSsh2Connection::newDirDownload(const QString &remoteDir,
                                             const QString &localDir)
{
  QString fullLocalPath = localDir + "/" + QDir(remoteDir).dirName();
  LibSsh2Operation *op = new DirDownload(this, fullLocalPath, remoteDir);
  return op;
}

SshOperation *LibSsh2Connection::newDirRemove(const QString &remoteDir)
{
  LibSsh2Operation *op = new CommandOperation(this, "rm -rf  " + remoteDir);
  return op;

}

LIBSSH2_SESSION * LibSsh2Connection::session()
{
  return m_session;
}

void LibSsh2Connection::disableNotifiers()
{
  if(m_readNotifier) {
    m_readNotifier->setEnabled(false);
    disconnect(m_readNotifier, SIGNAL(activated(int)), 0, 0);
  }

  if(m_writeNotifier) {
    m_writeNotifier->setEnabled(false);
    disconnect(m_writeNotifier, SIGNAL(activated(int)), 0, 0);
  }
}

void LibSsh2Connection::waitForSocket(const char *slot)
{
  if(!m_readNotifier) {
    m_readNotifier = new SocketNotifier(sock,
                                        QSocketNotifier::Read, this);
  }
  if(!m_writeNotifier) {
    m_writeNotifier = new SocketNotifier(sock,
                                         QSocketNotifier::Write, this);
  }

  int dir = libssh2_session_block_directions(m_session);
  if(dir & LIBSSH2_SESSION_BLOCK_INBOUND) {
    disconnect(m_readNotifier, SIGNAL(activated(int)), 0, 0);
    connect(m_readNotifier, SIGNAL(activated(int)),
            this, slot);
    m_readNotifier->setEnabled(true);
  }

  if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) {
     disconnect(m_writeNotifier, SIGNAL(activated(int)), 0, 0);
     connect(m_writeNotifier, SIGNAL(activated(int)),
             this, slot);
     m_writeNotifier->setEnabled(true);
  }
}

void LibSsh2Connection::askKeyboardInterative(const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                                              LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses)
{
  m_responses = responses;
  QString prompt = QString::fromLocal8Bit(prompts[0].text, prompts[0].length);

  connect(m_askPassword, SIGNAL(entered(const QString&)),
          this, SLOT(askKeyboardInterative(const QString&)));

  m_askPassword->ask(tr("%1@%2").arg(m_userName).arg(m_hostName),
                     prompt);
}

void LibSsh2Connection::askKeyboardInterative(const QString &passcode)
{
  QByteArray code = passcode.toLocal8Bit();
  // What about clean of text? Does libssh2 do it?

  if(m_responses != NULL) {

    m_responses[0].text = strdup(code.constData());
    m_responses[0].length = strlen(code.constData());
    m_responses = NULL;
  }

  emit responseSet();
}



void LibSsh2Connection::close() {

  libssh2_sftp_shutdown(m_sftp_session);
}




} /* namespace MoleQueue */
