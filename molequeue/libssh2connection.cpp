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


extern "C" {

static char* passwd;


static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len, int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract)
{
    printf("callback");
    int i;
    size_t n;
    char buf[1024];
    (void)abstract;

    responses[i].text = strdup(passwd);
    responses[i].length = strlen(passwd);

    fwrite(prompts[0].text, 1, prompts[0].length, stdout);


}

}



LibSsh2Connection::LibSsh2Connection(const QString &hostName,
                                     const QString &userName,
                                     int port,
                                     AskPassword *askPass,
                                     QObject *parentObject) :
  SshConnection(parentObject), m_hostName(hostName), m_userName(userName),
  m_port(port), sock(NULL), m_session(NULL), m_askPassword(askPass),
  m_readNotifier(NULL)
{
  connect(m_askPassword, SIGNAL(entered(const QString&)),
          this, SLOT(userAuth(const QString&)));
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
  unsigned long hostaddr = inet_addr(addresses[0].toString().toLocal8Bit().data());
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


  if (!m_session)
    return;

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

  m_askPassword->ask(tr("%1@%2").arg(m_userName).arg(m_hostName));
}

void LibSsh2Connection::userAuth(const QString &password)
{

  int rc;

  const char *username = m_userName.toLocal8Bit().data();
  char *pass = password.toLocal8Bit().data();

  qDebug() << m_session;
  qDebug() << m_userName;

  char *userauthlist = libssh2_userauth_list(m_session, username, strlen(username));

  if(userauthlist == NULL)
    qDebug() << "NULL";

  rc = libssh2_session_last_errno(m_session);

  if(rc ==  LIBSSH2_ERROR_EAGAIN) {
    qDebug() << "wait";
    waitForSocket(SLOT(userAuth));
    return;
  }
  else {
    qDebug() << "rc: " << rc;
    return;
  }

  qDebug() << userauthlist;

  if (strstr(userauthlist, "password") != NULL) {
    while ((rc = libssh2_userauth_password(m_session, username, pass)) ==
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
  else if(strstr(userauthlist, "keyboard-interactive") != NULL) {
    passwd = pass;

    rc = libssh2_userauth_keyboard_interactive(m_session, username, &kbd_callback);

    if(rc)
    {
      qDebug() << "error";
    }

    m_askPassword->correct();

    emit sessionOpen();
 }
}




SshOperation *LibSsh2Connection::newCommand(const QString &command)
{
  LibSsh2Operation *op = new CommandOperation(this, command);
  return op;
}

SshOperation *LibSsh2Connection::newFileDownload(const QString &remoteFile,
                                              const QString &localFile)
{
  qDebug() << "download";
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
  m_readNotifier->setEnabled(false);

  disconnect(m_readNotifier, SIGNAL(activated(int)), 0, 0);
}

void LibSsh2Connection::waitForSocket(const char *slot)
{
  if(!m_readNotifier) {
    m_readNotifier = new SocketNotifier(sock,
                                        QSocketNotifier::Read, this);
  }

  m_readNotifier->setEnabled(true);
}
} /* namespace MoleQueue */
