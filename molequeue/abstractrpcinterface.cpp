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

#include "abstractrpcinterface.h"

#include "jsonrpc.h"
#include "connection.h"
#include "message.h"

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtGlobal>

#define DEBUGOUT(title) \
  if (this->m_debug)    \
    qDebug() << QDateTime::currentDateTime().toString() \
             << "AbstractRpcInterface" << title <<

namespace MoleQueue
{

AbstractRpcInterface::AbstractRpcInterface(QObject *parentObject) :
  QObject(parentObject),
  m_jsonrpc(new JsonRpc (this)),
  m_packetCounter(0),
  m_debug(false)
{
  // Randomize the packet counter's starting value.
  qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));
  m_packetCounter = static_cast<IdType>(qrand());

  connect(m_jsonrpc, SIGNAL(invalidPacketReceived(MoleQueue::Connection*,
                                                  MoleQueue::EndpointId,
                                                  Json::Value,Json::Value)),
          this, SLOT(replyToInvalidPacket(MoleQueue::Connection*,
                                          MoleQueue::EndpointId,
                                          Json::Value,Json::Value)));
  connect(m_jsonrpc, SIGNAL(invalidRequestReceived(MoleQueue::Connection*,
                                                   MoleQueue::EndpointId,
                                                   Json::Value,Json::Value)),
          this, SLOT(replyToInvalidRequest(MoleQueue::Connection*,
                                           MoleQueue::EndpointId,
                                           Json::Value,Json::Value)));
  connect(m_jsonrpc, SIGNAL(unrecognizedRequestReceived(MoleQueue::Connection*,
                                                        MoleQueue::EndpointId,
                                                        Json::Value,Json::Value)),
          this, SLOT(replyToUnrecognizedRequest(MoleQueue::Connection*,
                                                MoleQueue::EndpointId,
                                                Json::Value,Json::Value)));
  connect(m_jsonrpc, SIGNAL(invalidRequestParamsReceived(MoleQueue::Connection*,
                                                         MoleQueue::EndpointId,
                                                         Json::Value,Json::Value)),
          this, SLOT(replyToinvalidRequestParams(MoleQueue::Connection*,
                                                 MoleQueue::EndpointId,
                                                 Json::Value,Json::Value)));
  connect(m_jsonrpc, SIGNAL(internalErrorOccurred(MoleQueue::Connection*,
                                                  MoleQueue::EndpointId,
                                                  Json::Value,Json::Value)),
          this, SLOT(replyWithInternalError(MoleQueue::Connection*,
                                            MoleQueue::EndpointId,
                                            Json::Value,Json::Value)));
}

AbstractRpcInterface::~AbstractRpcInterface()
{
  delete m_jsonrpc;
  m_jsonrpc = NULL;
}

void AbstractRpcInterface::readPacket(const MoleQueue::Message msg)
{
  Connection *conn = qobject_cast<Connection*>(this->sender());
  DEBUGOUT("readPacket") "Interpreting new packet.";
  m_jsonrpc->interpretIncomingPacket(conn, msg);
}
// TODO rename connection => replyConnection?
void AbstractRpcInterface::replyToInvalidPacket(MoleQueue::Connection *connection,
                                                const MoleQueue::EndpointId replyTo,
                                                const Json::Value &packetId,
                                                const Json::Value &errorDataObject)
{
  DEBUGOUT("replyToInvalidPacket") "replying to an invalid packet.";
  PacketType packet = m_jsonrpc->generateErrorResponse(
        -32700, "Parse error", errorDataObject, packetId);

  Message msg(replyTo, packet);

  connection->send(msg);
}

void AbstractRpcInterface::replyToInvalidRequest(MoleQueue::Connection *connection,
                                                 const MoleQueue::EndpointId replyTo,
                                                 const Json::Value &packetId,
                                                 const Json::Value &errorDataObject)
{
  DEBUGOUT("replyToInvalidRequest") "replying to an invalid request.";
  PacketType packet = m_jsonrpc->generateErrorResponse(
        -32600, "Invalid request", errorDataObject, packetId);

  Message msg(replyTo, packet);

  connection->send(msg);
}

void AbstractRpcInterface::replyToUnrecognizedRequest(MoleQueue::Connection *connection,
                                                      const MoleQueue::EndpointId replyTo,
                                                      const Json::Value &packetId,
                                                      const Json::Value &errorDataObject)
{
  DEBUGOUT("replyToUnrecognizedRequest") "replying to an unrecognized method.";
  PacketType packet = m_jsonrpc->generateErrorResponse(
        -32601, "Method not found", errorDataObject, packetId);

  Message msg(replyTo, packet);

  connection->send(msg);
}

void AbstractRpcInterface::replyToinvalidRequestParams(MoleQueue::Connection *connection,
                                                       const MoleQueue::EndpointId replyTo,
                                                       const Json::Value &packetId,
                                                       const Json::Value &errorDataObject)
{
  DEBUGOUT("replyToInvalidRequestParam") "replying to an ill-formed request.";
  PacketType packet = m_jsonrpc->generateErrorResponse(
        -32602, "Invalid params", errorDataObject, packetId);

  Message msg(replyTo, packet);

  connection->send(msg);
}

void AbstractRpcInterface::replyWithInternalError(MoleQueue::Connection *connection,
                                                  const MoleQueue::EndpointId replyTo,
                                                  const Json::Value &packetId,
                                                  const Json::Value &errorDataObject)
{
  DEBUGOUT("replyWithInternalError") "Notifying peer of internal error.";
  PacketType packet = m_jsonrpc->generateErrorResponse(
        -32603, "Internal error", errorDataObject, packetId);

  Message msg(replyTo, packet);

  connection->send(msg);
}

IdType AbstractRpcInterface::nextPacketId()
{
  return m_packetCounter++;
}

} // end namespace MoleQueue
