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

#include "libssh2operations.h"
#include "libssh2connection.h"
#include "logger.h"

#include <QDebug>
#include <QHostInfo>
#include <QList>
#include <QTimer>
#include <QtGui/QApplication>


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

SocketNotifier::SocketNotifier(int sock, QSocketNotifier::Type type,
                               QObject *parentObject) :
  m_notifier(new QSocketNotifier(sock, type, parentObject))
{
  connect(m_notifier, SIGNAL(activated(int)),
          this, SLOT(activate(int)));
}

void  SocketNotifier::setEnabled(bool enable)
{
  m_notifier->setEnabled(enable);
}

void SocketNotifier::activate(int sock)
{
  m_notifier->setEnabled(false);
  emit activated(sock);
}

LibSsh2Operation::LibSsh2Operation(QObject *parentObject, LibSsh2Connection *connection) :
  SshOperation(parentObject),
  m_connection(connection),
  m_readNotifier(new SocketNotifier(connection->socket(),
                                     QSocketNotifier::Read, this)),
  m_writeNotifier(new SocketNotifier(connection->socket(),
                                      QSocketNotifier::Write, this))
{

}

LibSsh2Operation::LibSsh2Operation(LibSsh2Connection *connection) :
  SshOperation(connection), m_connection(connection),
  m_readNotifier(new SocketNotifier(connection->socket(),
                                     QSocketNotifier::Read, this)),
  m_writeNotifier(new SocketNotifier(connection->socket(),
                                      QSocketNotifier::Write, this)),
  m_errorCode(0),
  m_errorString(QString())

{

}

void CommandOperation::closeChannel()
{
  int rc = libssh2_channel_close(m_channel);

  if(rc == LIBSSH2_ERROR_EAGAIN) {
    waitForSocket(SLOT(closeChannel()));
  }
  else {
    if(rc != 0) {
      Logger::logError(tr("libssh2_channel_close rc: %1").arg(rc));

      char *exitSignal=(char *)"none";
      int exitCode = libssh2_channel_get_exit_status(m_channel);

       libssh2_channel_get_exit_signal(m_channel, &exitSignal,
                                       NULL, NULL, NULL, NULL, NULL);
       if (exitSignal)
         Logger::logError(tr("libssh2_channel_close error exit code: %1"
                             ", exit signal=%2").arg(exitCode).arg(exitSignal));
    }
    libssh2_channel_free(m_channel);
  }
}

void LibSsh2Operation::disableNotifiers()
{
  m_readNotifier->setEnabled(false);
  m_writeNotifier->setEnabled(false);

  disconnect(m_readNotifier, SIGNAL(activated(int)), 0, 0);
  disconnect(m_writeNotifier, SIGNAL(activated(int)), 0, 0);
}

void LibSsh2Operation::waitForSocket(const char *slot)
{
  int dir = libssh2_session_block_directions(m_connection->session());
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

void LibSsh2Operation::completeWithError()
{
  emit error(m_errorCode, m_errorString);
  emit complete();
}

CommandOperation::CommandOperation(LibSsh2Connection *connection, QString cmd) :
  LibSsh2Operation(connection), m_command(cmd)
{

}

bool CommandOperation::execute()
{
  sftpChannelOpen();

  return true;
}

void CommandOperation::sftpChannelOpen() {

/* Exec non-blocking on the remote host */
  if( (m_channel = libssh2_channel_open_session(m_connection->session())) == NULL &&
        libssh2_session_last_error(m_connection->session(),NULL,NULL,0) ==
       LIBSSH2_ERROR_EAGAIN ) {
    waitForSocket(SLOT(sftpChannelOpen()));
  }
  else {
    if( m_channel == NULL )
    {
      m_errorCode = libssh2_session_last_error(m_connection->session(),NULL,NULL,0);
      m_errorString = tr("libssh2_channel_open_session error %1").arg(m_errorCode);
      Logger::logError(m_errorString);
      completeWithError();
      return;
    }

    execCommand();
  }
}

void CommandOperation::execCommand()
{

  int rc = libssh2_channel_exec(m_channel, m_command.toLocal8Bit().data());

  if(rc == LIBSSH2_ERROR_EAGAIN) {
    waitForSocket(SLOT(execCommand()));
  }
  else if(rc == 0) {
    LibSsh2ChannelReader *reader = new LibSsh2ChannelReader(this,
                                                            m_connection,
                                                            m_channel);

    connect(reader, SIGNAL(complete()),
            this, SLOT(readComplete()));

    reader->read();
  }
  else {
    m_errorCode = rc;
    m_errorString = tr("libssh2_channel_exec error %1").arg(rc);
    Logger::logError(m_errorString);
    closeChannel();
    completeWithError();
  }
}


void CommandOperation::readComplete() {

  LibSsh2ChannelReader *reader = qobject_cast<LibSsh2ChannelReader*>(sender());

  m_output = reader->data();

  reader->deleteLater();

  closeChannel();

  if(reader->errorCode() != 0) {
    m_errorCode = reader->errorCode();
    m_errorString = reader->errorString();

    completeWithError();
  }
  else  {
    emit complete();
  }
}

LibSsh2ChannelReader::LibSsh2ChannelReader(QObject *parentObject,
                                           LibSsh2Connection *connection,
                                           LIBSSH2_CHANNEL *channel) :
  LibSsh2Operation(parentObject, connection),
  m_channel(channel)
{

}

bool LibSsh2ChannelReader::execute()
{
  read();

  return true;
}

void LibSsh2ChannelReader::read()
{

  /* loop until we block */
  int rc;
  do
  {
    char buffer[0x4000];
    rc = libssh2_channel_read(m_channel, buffer, sizeof(buffer) );

    if( rc > 0 ) {
      m_data.append(buffer, rc);
    }
    else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN){
      m_errorCode = libssh2_session_last_error(m_connection->session(),NULL,NULL,0);
      m_errorString = tr("libssh2_channel_read error %1").arg(rc);
      Logger::logError(m_errorString);
      break;
    }
  }
  while( rc > 0 );

  if( rc == LIBSSH2_ERROR_EAGAIN )
  {
    waitForSocket(SLOT(read()));
  }
  else {
    emit complete();
  }
}

} /* namespace MoleQueue */
