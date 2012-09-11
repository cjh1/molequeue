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

#ifndef OPENSSHCOMMANDCONNECTION_H
#define OPENSSHCOMMANDCONNECTIOn_H

#include "sshcommandconnection.h"

namespace MoleQueue {

class TerminalProcess;


/**
 * @class OpenSshCommandConnection opensshcommandconnection.h
 * <molequeue/opensshcommandconnection.h>
 * @brief Concrete implementation of SshCommandConnection using commandline
 * open ssh/scp.
 * @author Marcus D. Hanwell, David C. Lonie, Chris Harris
 *
 * The OpenSshCommandConnection provides an implementation of the
 * SshCommandConnection interface that calls the commandline ssh and
 * scp executables in a TerminalProcess.
 *
 * When writing code that needs ssh functionality, the code should use the
 * SshConnection interface instead.
 */
class OpenSshCommandConnection : public SshCommandConnection
{
  Q_OBJECT

public:
  OpenSshCommandConnection(QObject *parentObject = 0);
  ~OpenSshCommandConnection();

protected:

  /// @return the arguments to be passed to the SSH command.
  QStringList sshArgs();

  /// @return the arguments to be passed to the SCP command.
  QStringList scpArgs();
};

} // End namespace

#endif
