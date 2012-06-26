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

#ifndef MESSAGEID_H_
#define MESSAGEID_H_

namespace MoleQueue
{

class MessageId
{
public:
  MessageId(IdType id) : m_id(id) {};
  virtual ~MessageId();

  virtual IdType id() { return m_id; };

private:
  IdType m_id;
};

  } /* namespace MoleQueue */
#endif /* MESSAGEID_H_ */
