/******************************************************************************

  This source file is part of the Avogadro project.

  Copyright 2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "sshcommandconnectionfactory.h"
#include "puttycommandconnection.h"
#include "opensshcommandconnection.h"

#include <QtCore/QMutex>
#include <QtCore/QCoreApplication>

namespace MoleQueue
{

namespace
{
  static SshCommandConnectionFactory *factoryInstance;
}

SshCommandConnectionFactory::SshCommandConnectionFactory(QObject *parentObject)
  : QObject(parentObject)
{
}

SshCommandConnectionFactory *SshCommandConnectionFactory::instance()
{
  static QMutex mutex;
  if (!factoryInstance) {
    mutex.lock();
    if (!factoryInstance)
      factoryInstance = new SshCommandConnectionFactory(QCoreApplication::instance());
    mutex.unlock();
  }
  return factoryInstance;
}

SshCommandConnection *SshCommandConnectionFactory::newSshCommandConnection(
  QObject *parentObject)
{
#ifdef WIN32
  return newSshCommandConnection(Putty, parentObject);
#else
  return newSshCommandConnection(OpenSsh, parentObject);
#endif
}

SshCommandConnection *SshCommandConnectionFactory::newSshCommandConnection(
  SshClient sshClient,
  QObject *parentObject)
{
    switch(sshClient) {
    case OpenSsh:
      return new OpenSshCommandConnection(parentObject);
#ifdef WIN32
    case Putty:
      return new PuttyCommandConnection(parentObject);
#endif
    default:
      qFatal("Can not create ssh command connection: " + sshClient);
      return NULL;
    }
}

QString SshCommandConnectionFactory::defaultSshCommand()
{
#ifdef WIN32
  return QString("plink");
#else
  return QString("ssh");
#endif
}

QString SshCommandConnectionFactory::defaultScpCommand()
{
#ifdef WIN32
  return QString("pscp");
#else
  return QString("scp");
#endif
}

} // End MoleQueue namespace
