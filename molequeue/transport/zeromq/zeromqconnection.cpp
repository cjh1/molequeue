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
  m_connected(true),
  m_listener(NULL)
{

}


ZeroMqConnection::ZeroMqConnection(QObject *parentObject, const QString &address)
: Connection(parentObject),
  m_connectionString(address),
  m_Context(new zmq::context_t(1)),
  m_Socket(new zmq::socket_t(*m_Context, ZMQ_DEALER)),
  m_connected(false),
  m_listener(new QTimer())
{
  connect(m_listener, SIGNAL(timeout()),
          this, SLOT(listen()));
}

ZeroMqConnection::~ZeroMqConnection()
    {
    }

  void ZeroMqConnection::onMessage(Message msg)
    {
    emit newMessage(msg);
    }

  void ZeroMqConnection::open()
    {
    if (m_Socket) {
    QByteArray ba = m_connectionString.toLocal8Bit();
    qDebug() << "client ba: " << ba.data();
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

void ZeroMqConnection::send(Message msg)
{
  qDebug() << msg.data().size();
  qDebug() << msg.data();
  zmq::message_t message(msg.data().size());
  memcpy(message.data(), msg.data().constData(), msg.data().size());

  // If on the server side send the endpoint id first
  if (!m_listener) {
    qDebug() << "Server sending message to: " << msg.to();
    zmq::message_t identity(msg.to().size());
    memcpy(identity.data(), msg.to().data(), msg.to().size());
    bool rc  = m_Socket->send(identity,ZMQ_SNDMORE | ZMQ_NOBLOCK);

    qDebug() << "rc=" << rc;
  }

  // Send message body
  bool rc = m_Socket->send(message, ZMQ_NOBLOCK);
  qDebug() << "rc=" << rc;
}

void ZeroMqConnection::listen()
{
  qDebug() << "Connection:listen";
  zmq::message_t message;

  if(m_Socket->recv(&message, ZMQ_NOBLOCK)) {

    qDebug() << "Client recieved message: " << message.size();

    int size = message.size();
    PacketType messageBuffer;

    // Doe we need to this convertion?
    messageBuffer.append(QString::fromLocal8Bit(static_cast<char*>(message.data()),
                                                size));

    Message msg(messageBuffer);

    emit newMessage(msg);
  }
}





} /* namespace MoleQueue */
