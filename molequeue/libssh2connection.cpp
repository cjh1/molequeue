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

#include <QDebug>
#include <QHostInfo>
#include <QList>
#include <QtCore/QObject>


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

LibSsh2Connection::LibSsh2Connection(const QString &hostName,
                                     const QString &userName,
                                     int port,
                                     QObject *parentObject) :
  SshConnection(parentObject), m_hostName(hostName), m_userName(userName),
  m_port(port),m_password("test")
{

}

void LibSsh2Connection::openSession()
{
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

  const char *username = m_userName.toLocal8Bit().data();
  const char *password = m_password.toLocal8Bit().data();


  while ((rc = libssh2_userauth_password(m_session, username, password)) ==
      LIBSSH2_ERROR_EAGAIN);

    if (rc) {
      fprintf(stderr, "Authentication by password failed.\n");
      //goto shutdown;
      return;
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
  LibSsh2Operation *op = new DirDownload(this, localDir, remoteDir);
  return op;
}

SshOperation *LibSsh2Connection::newDirRemove(const QString &remoteDir)
{
  LibSsh2Operation *op = new CommandOperation(this, "rm -rf  " + remoteDir);
  return op;

}


} /* namespace MoleQueue */
