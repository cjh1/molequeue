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
#include <QtCore/QBuffer>
#include <QtCore/QTextStream>


namespace MoleQueue
{

class LibSsh2Connection;
class LibSsh2ChannelReader;

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

  void disableNotifiers();
  void waitForSocket(const char *slot);
  void completeWithError();

private:
  void init();

};

class CommandOperation: public LibSsh2Operation
{
  Q_OBJECT
public:
  CommandOperation(LibSsh2Connection *connection, QString command);

  bool execute();

  static const char *prompt;

protected slots:
  void closeChannel();

private slots:
  void openChannel();
  void execCommand();
  void exitCode();
  void readComplete();
  void requestPty();
  void openShell();
  void setPrompt();
  void exitCodeComplete();
  QString cleanOutput(QString output);
  //void startBash();

private:
  LIBSSH2_CHANNEL *m_channel;
  QString m_command;
  LibSsh2ChannelReader *m_reader;
  //static const char * redirectStdErr = "2>&1";



};
//class CopyOperation: public Operation {};


class LibSsh2ChannelWriter: public libSsh2Operation
{
  Q_OBJECT
public:

  LibSsh2ChannelWriter(QObject *parentObject,
                       LibSsh2Connection *connection,
                       LIBSSH2_CHANNEL *channel);

  void write(const QString &data, const char *completeSlot);

private slots:
  void write();

  /// For writing to a channel
};


class LibSsh2ChannelReader: public LibSsh2Operation
{
  Q_OBJECT
public:
  LibSsh2ChannelReader(QObject *parentObject,
                       LibSsh2Connection *connection,
                       LIBSSH2_CHANNEL *channel);

signals:
  void readComplete();


public slots:
  bool execute();
  void read();
  void readToPrompt();
  void readStdOut();
  void readStdErr();
  void reset(int promptCount);


private:
  QByteArray m_buffer;
  int m_socket;
  LIBSSH2_CHANNEL *m_channel;
  QBuffer *mq_buffer;
  QTextStream *m_stream;
  QString out;
  int m_promptCount;

};

} /* namespace MoleQueue */


#endif /* LIBSSH2OPERATIONS_H_ */
