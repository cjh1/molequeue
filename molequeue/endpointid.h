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

#ifndef ENDPOINTID_H_
#define ENDPOINTID_H_

#include <QtCore/QObject>

namespace MoleQueue
{

class EndpointId : public QObject
{
  Q_OBJECT
public:
  explicit EndpointId() {};
  explicit EndpointId(EndpointId &endpointId) : QObject() {};
  explicit EndpointId(const MoleQueue::EndpointId &endpointId) : QObject() {};
  //virtual void send(const PacketType &packet) = 0;
};

} /* namespace MoleQueue */

#endif /* ENDPOINTID_H_ */
