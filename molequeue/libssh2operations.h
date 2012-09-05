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

#ifndef LIBSSH2OPERATIONS_H_
#define LIBSSH2OPERATIONS_H_

#include "sshconnection.h"
#include "sshoperation.h"
#include <libssh2.h>
#include <QtCore/QObject>
#include <QTcpSocket>
#include <QtCore/QSocketNotifier>


namespace MoleQueue
{

class LibSsh2Connection;

class SocketNotifier: public QObject
{
  Q_OBJECT
public:
  SocketNotifier(int sock, QSocketNotifier::Type type, QObject *parentObject);

public slots:
  void  setEnabled(bool enable);

signals:
  void  activated (int socket);

private slots:
  void activate(int socket);

private:
  QSocketNotifier *m_notifier;
};

class LibSsh2Operation: public SshOperation
{
  Q_OBJECT
public:
  LibSsh2Operation(QObject *parentObject, LibSsh2Connection *connection);
  LibSsh2Operation(LibSsh2Connection *connection);

protected:
  LibSsh2Connection *m_connection;
  SocketNotifier *m_readNotifier;
  SocketNotifier *m_writeNotifier;
  QString m_output;
  QString m_errorString;
  int m_errorCode;

  void disableNotifiers();
  void waitForSocket(const char *slot);
  void completeWithError();

private:


};

class CommandOperation: public LibSsh2Operation
{
  Q_OBJECT
public:
  CommandOperation(LibSsh2Connection *connection, QString command);

  bool execute();

protected slots:
  void closeChannel();

private slots:
  void sftpChannelOpen();
  void execCommand();
  void readComplete();

private:
  LIBSSH2_CHANNEL *m_channel;
  QString m_command;




};
//class CopyOperation: public Operation {};



class LibSsh2ChannelReader: public LibSsh2Operation
{
  Q_OBJECT
public:
  LibSsh2ChannelReader(QObject *parentObject,
                       LibSsh2Connection *connection,
                       LIBSSH2_CHANNEL *channel);

  QByteArray data() {
    return m_data;
  }

signals:
  void readComplete();


public slots:
  bool execute();
  void read();


private:
  QByteArray m_data;
  int m_socket;
  LIBSSH2_CHANNEL *m_channel;
};

} /* namespace MoleQueue */


#endif /* LIBSSH2OPERATIONS_H_ */
