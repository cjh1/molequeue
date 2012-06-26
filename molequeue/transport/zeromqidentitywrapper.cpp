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

#include "zeromqidentitywrapper.h"

namespace MoleQueue
{

ZeroMqIdentityWrapper::ZeroMqIdentityWrapper(ZeroMqConnection *connection,
                                             QString identity)
  : m_connection(connection),
    m_identity(identity)
{

}

ZeroMqIdentityWrapper::~ZeroMqIdentityWrapper()
{

}

void ZeroMqIdentityWrapper::send(const PacketType& packet)
{
  m_connection->send(m_identity, packet);
}

bool ZeroMqIdentityWrapper::isOpen()
{
  return m_connection->isOpen();
}

QString ZeroMqIdentityWrapper::connectionString()
{
  return m_identity;
}

void ZeroMqIdentityWrapper::onMessage(const PacketType &message)
{
    emit newMessage(message);
}


} /* namespace MoleQueue */
