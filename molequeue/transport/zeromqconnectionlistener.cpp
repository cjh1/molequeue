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

#include "zeromqconnectionlistener.h"
#include "../message.h"

#include <assert.h>

#include <QtCore/QTimer>

namespace MoleQueue
{

ZeroMqConnectionListener::ZeroMqConnectionListener(QObject *parentObject,
                                                   const QString &address)
  : ConnectionListener(parentObject),
    m_connectionString(address),
    m_Context(1),
    m_Socket(m_Context, ZMQ_ROUTER),
    m_listener(new QTimer(this))
{
  connect(m_listener, SIGNAL(timeout()),
          this, SLOT(listen()));}

ZeroMqConnectionListener::~ZeroMqConnectionListener()
{
  // TODO Auto-generated destructor stub
}

void ZeroMqConnectionListener::start()
{
  qDebug() << "Connecting to " << m_connectionString;

  QString ipcString = "ipc://" + m_connectionString;

  QByteArray ba = ipcString.toLocal8Bit();

  qDebug() << "ba: " << ba.data();

  m_Socket.bind(ba.data());
  m_listener->start(100);

  // TODO Should this just be part of the class?
  m_connection = new ZeroMqConnection(this, &m_Context, &m_Socket);
  //      m_ConnectionMap.insert(identity, connection);
  emit newConnection(m_connection);

}

void ZeroMqConnectionListener::stop(bool force)
{
  m_Socket.close();
  m_listener->stop();
}

void ZeroMqConnectionListener::stop()
{
  this->stop(false);
}

QString ZeroMqConnectionListener::connectionString()
{
  return m_connectionString;
}

void ZeroMqConnectionListener::listen()
{
  qDebug() << "listen2";
  zmq::message_t address;

  if (m_Socket.recv(&address, ZMQ_NOBLOCK)) {

    int size = address.size();
    QString replyTo = QString::fromLocal8Bit(static_cast<char*>(address.data()),
                                              size);

    qDebug() << "message received: " << replyTo;

//    // Receive the empty message
//    zmq::message_t empty;
//
//
//    if(!m_Socket.recv(&empty, ZMQ_NOBLOCK)) {
//      qDebug() << "Error no empty message";
//      return;
//    }
//
//    qDebug() << empty.size();
//
//
//    assert(empty.size() == 0);

    // Now receive the message
    zmq::message_t message;
    if(!m_Socket.recv(&message, ZMQ_NOBLOCK)) {
      qDebug() << "Error not message body";
      return;
    }

    size = message.size();
    QString packetString = QString::fromLocal8Bit(static_cast<char*>(message.data()),
                                                  size);

    PacketType packet;
    packet.append(packetString);

    char to_id[255];
    size_t sz = 255;
    m_Socket.getsockopt(ZMQ_IDENTITY, to_id, &sz);
    QString to = QString::fromLocal8Bit(static_cast<char*>(to_id),
                                                sz);

    qDebug() << "replyTo: " << replyTo;
    Message msg(to, replyTo, packet);

    m_connection->onMessage(msg);
  }
}



} /* namespace MoleQueue */
