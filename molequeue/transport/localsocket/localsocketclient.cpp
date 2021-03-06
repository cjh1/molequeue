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

#include "localsocketclient.h"
#include "localsocketconnection.h"

namespace MoleQueue
{

LocalSocketClient::LocalSocketClient(QObject *parentObject)
  : Client(parentObject)
{

}



void LocalSocketClient::connectToServer(const QString &serverName)
{

  if (m_connection && m_connection->isOpen()) {
    if (m_connection->connectionString() == serverName) {
      return;
    }
    else {
      m_connection->close();
      delete m_connection;
      m_connection = NULL;
    }
  }

  // New connection
  if (m_connection == NULL) {
    if (serverName.isEmpty()) {
      return;
    }
    else {
      LocalSocketConnection *connection = new LocalSocketConnection(this, serverName);
      setConnection(connection);
      connection->open();
      connection->start();
    }
  }
}


} /* namespace MoleQueue */
