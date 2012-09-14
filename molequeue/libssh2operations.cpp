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
  m_readNotifier(NULL),
  m_writeNotifier(NULL)
{

}

LibSsh2Operation::LibSsh2Operation(LibSsh2Connection *connection) :
  SshOperation(connection),
  m_connection(connection),
  m_readNotifier(NULL),
  m_writeNotifier(NULL)
{

}

void LibSsh2Operation::init()
{

}

void CommandOperation::closeChannel()
{
  qDebug() << ">>> closeChannel";
  int rc = libssh2_channel_close(m_channel);

  if(rc == LIBSSH2_ERROR_EAGAIN) {
    qDebug() << "closeChannel : waiting for socket";
    waitForSocket(SLOT(closeChannel()));
  } else {
    if(rc == 0) {
      char *exitSignal=(char *)"";
      m_errorCode = libssh2_channel_get_exit_status(m_channel);

      if(m_errorCode != 0) {
        libssh2_channel_get_exit_signal(m_channel, &exitSignal,
                                             NULL, NULL, NULL, NULL, NULL);
        if (exitSignal) {
           Logger::logError(tr("libssh2_channel_close error exit code: %1"
                               ", exit signal=%2").arg(m_errorCode)
                              .arg(exitSignal));
           m_errorString = exitSignal;
        }

        emit error(m_errorCode, m_errorString);
      }
    }
    else {
      QString msg
        = tr("libssh2_channel_close error exit code: %1").arg(rc);
      Logger::logError(msg);
      emit error(m_errorCode, msg);
    }
  }

  complete();

  libssh2_channel_free(m_channel);

  qDebug() << ">>> closeChannel";
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

  if(!m_readNotifier) {
    m_readNotifier = new SocketNotifier(m_connection->socket(),
                                        QSocketNotifier::Read, this);
  }
  if(!m_writeNotifier) {
    m_writeNotifier = new SocketNotifier(m_connection->socket(),
                                         QSocketNotifier::Write, this);
  }

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
  connect(m_connection, SIGNAL(sessionOpen()),
          this, SLOT(openChannel()));
}

bool CommandOperation::execute()
{
  m_connection->openSession();
  return true;
}

void CommandOperation::openChannel()
{

  qDebug() << ">>> openChannel";
  // Disconnection from sessionOpen or we will get call again
  LibSsh2Connection *con = qobject_cast<LibSsh2Connection *>(sender());
  if(con) {
    disconnect(con, SIGNAL(sessionOpen()), this, 0);
  }

/* Exec non-blocking on the remote host */
  if( (m_channel = libssh2_channel_open_session(m_connection->session())) == NULL &&
        libssh2_session_last_error(m_connection->session(),NULL,NULL,0) ==
       LIBSSH2_ERROR_EAGAIN ) {
    qDebug() << "openChannel : waiting for socket";
    waitForSocket(SLOT(openChannel()));
  }
  else {
    if( m_channel == NULL )
    {
      qDebug() << "openChannel : error: " << m_errorCode;
      m_errorCode = libssh2_session_last_error(m_connection->session(),NULL,NULL,0);
      m_errorString = tr("libssh2_channel_open_session error %1").arg(m_errorCode);

      char *errmsg;
      int errmsg_len;

      int e = libssh2_session_last_error(m_connection->session(), &errmsg, &errmsg_len, 0);

      qDebug() << "openChannel : libssh2_session_last_error return : " << e;
      qDebug() << "openChannel : last error: " << errmsg;


      Logger::logError(m_errorString);
      completeWithError();
      return;
    }

    execCommand();
  }

  qDebug() << "<<< openChannel";

}

void CommandOperation::execCommand()
{
  qDebug() << ">>> execCommand";

  int rc = libssh2_channel_exec(m_channel, m_command.toLocal8Bit().data());

  if(rc == LIBSSH2_ERROR_EAGAIN) {
    qDebug() << "execCommand : waiting for socket";
    waitForSocket(SLOT(execCommand()));
  }
  else if(rc == 0) {
    qDebug() << "execCommand : creating reader" ;
    LibSsh2ChannelReader *reader = new LibSsh2ChannelReader(this,
                                                            m_connection,
                                                            m_channel);

    connect(reader, SIGNAL(readComplete()),
            this, SLOT(readComplete()));

    reader->read();
  }
  else {
    qDebug() << "execCommand : error :" << rc;
    m_errorCode = rc;
    m_errorString = tr("libssh2_channel_exec error %1").arg(rc);
    Logger::logError(m_errorString);
    closeChannel();
    completeWithError();
  }

  qDebug() << "<<< execCommand";
}


void CommandOperation::readComplete() {

  qDebug() << ">>> readComplete";

  LibSsh2ChannelReader *reader = qobject_cast<LibSsh2ChannelReader*>(sender());

  m_output = reader->output();

  reader->deleteLater();

  if(reader->errorCode() != 0) {
    m_errorCode = reader->errorCode();
    m_errorString = reader->errorString();
  }

  closeChannel();

  qDebug() << "<<< readComplete";
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
  qDebug() << ">>> read";
  while(!libssh2_channel_eof(m_channel)) {
    readStdOut();
    readStdErr();
  }

  m_output = m_buffer;
  emit readComplete();
  qDebug() << "<<< read";
}

void LibSsh2ChannelReader::readStdOut()
{

  /* loop until we block */
  int rc;
  do
  {
    char buffer[0x4000];
    rc = libssh2_channel_read(m_channel, buffer, sizeof(buffer) );

    if( rc > 0 ) {
      m_buffer.append(buffer, rc);
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
    waitForSocket(SLOT(readStdOut()));
  }
}

void LibSsh2ChannelReader::readStdErr()
{
  /* loop until we block */
  int rc;
  do
  {
    char buffer[0x4000];
    rc = libssh2_channel_read_stderr(m_channel, buffer, sizeof(buffer) );

    if( rc > 0 ) {
      m_buffer.append(buffer, rc);
    }
    else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN){
      m_errorCode = rc;
      m_errorString = tr("libssh2_channel_read error %1").arg(rc);
      Logger::logError(m_errorString);
      break;
    }
  }
  while( rc > 0 );

  if( rc == LIBSSH2_ERROR_EAGAIN )
  {
    waitForSocket(SLOT(readStdErr()));
  }
}

} /* namespace MoleQueue */
