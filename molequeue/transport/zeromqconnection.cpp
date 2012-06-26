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

#include "zeromqconnection.h"

#include <QtCore/QTimer>

namespace MoleQueue
{

ZeroMqConnection::ZeroMqConnection(QObject *parentObject,
                                   zmq::context_t *context,
                                   zmq::socket_t *socket)
: Connection(parentObject),
  m_Context(context),
  m_Socket(socket),
  m_connected(true)
{

}


ZeroMqConnection::ZeroMqConnection(QObject *parentObject, const QString &address)
: Connection(parentObject),
  m_connectionString(address),
  m_Context(new zmq::context_t(1)),
  m_Socket(new zmq::socket_t(*m_Context, ZMQ_ROUTER)),
  m_connected(false),
  m_listener(new QTimer())
{

}

ZeroMqConnection::~ZeroMqConnection()
    {
    }

  void ZeroMqConnection::onMessage(const Message msg)
    {
    newMessage(msg);
    }

  void ZeroMqConnection::open()
    {
    if (m_Socket) {
    QByteArray ba = m_connectionString.toLocal8Bit();
    m_Socket->connect(ba.data());
    m_connected = true;
    }
    }

  void ZeroMqConnection::start()
    {
    if (m_listener && !m_listener->isActive()) {
    m_listener->start(100);
    }
    }

  void ZeroMqConnection::close()
    {
    if (m_listener) {
    m_listener->stop();
    m_Socket->close();
    }
    }

  bool ZeroMqConnection::isOpen()
    {
    return m_connected;
    }

  QString ZeroMqConnection::connectionString()
    {
    return m_connectionString;
    }

void ZeroMqConnection::send(const Message msg)
{
  zmq::message_t message(msg.data().size());
  memcpy(message.data(), msg.data().constData(), msg.data().size());

  // If on the server side send the endpoint id first
  if (m_listener) {


    zmq::message_t identity(msg.to().size());
    memcpy(identity.data(), msg.to().data(), msg.to().size());
    m_Socket->send(identity, ZMQ_SNDMORE | ZMQ_NOBLOCK);
    zmq::message_t empty;
    bool rc  = m_Socket->send(empty, ZMQ_NOBLOCK);

    //TODO check rc.

  }

  // Send message body
  bool rc = m_Socket->send(message, ZMQ_NOBLOCK);
}

void ZeroMqConnection::listen()
{
  zmq::message_t message;
  zmq::pollitem_t items[1] = { { *m_Socket, 0, ZMQ_POLLIN, 0 } };

  zmq::poll (&items[0], 1, 10);

  if(items[0].revents & ZMQ_POLLIN) {

    m_Socket->recv(&message, ZMQ_NOBLOCK);

    int size = message.size();
    PacketType messageBuffer;

    // Doe we need to this convertion?
    messageBuffer.append(QString::fromLocal8Bit(static_cast<char*>(message.data()),
                                                size));

    emit newMessage(messageBuffer);
  }
}





} /* namespace MoleQueue */
