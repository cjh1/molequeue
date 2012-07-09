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
#include "molequeueglobal.h"
#include "message.h"

namespace MoleQueue
{

Message::Message(PacketType data)
  : m_data(data), m_to(), m_replyTo()
{

}

Message::Message(EndpointId to, PacketType data)
  : m_to(to), m_replyTo(), m_data(data)
{

}

Message::Message(EndpointId to, EndpointId replyTo, PacketType data)
  : m_to(to), m_replyTo(replyTo), m_data(data)
{

}

EndpointId Message::to() const
{
  return m_to;
}


EndpointId Message::replyTo() const
{
  return m_replyTo;
}

PacketType Message::data() const
{
  return m_data;
}

}
