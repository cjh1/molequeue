/******************************************************************************

  This source file is part of the MoleQueue project.

  Copyright 2011-2012 Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#ifndef PUTTYCOMMANDCONNECTION_H
#define PUTTYCOMMANDCONNECTION_H

#include "sshcommand.h"

namespace MoleQueue {

class TerminalProcess;


/**
 * @class PuttyCommandConnnection puttycommandconnection.h
 * <molequeue/puttycommandconnection.h>
 *
 * @brief Concrete implementation of SshCommandConnection using
 * commandline plink/pscp.
 * @author Marcus D. Hanwell, David C. Lonie, Chris Harris
 *
 * The PuttyCommandConnection provides an implementation of the
 * SshCommandConnection interface  * that calls the commandline plink and
 * pscp executables in a TerminalProcess.
 *
 * When writing code that needs ssh functionality, the code should use the
 * SshConnection interface instead.
 */
class PuttyCommandConnection : public SshCommandConnection
{
  Q_OBJECT

public:
  PuttyCommandConnection(QObject *parentObject = 0);
  ~PuttyCommandConnection();

protected:

  /// @return the arguments to be passed to the SSH command.
  QStringList sshArgs();

  /// @return the arguments to be passed to the SCP command.
  QStringList scpArgs();
};

} // End namespace

#endif
