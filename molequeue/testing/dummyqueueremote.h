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

#ifndef DUMMYQUEUEREMOTE_H
#define DUMMYQUEUEREMOTE_H

#include "queues/remotessh.h"

#include "dummysshcommand.h"


#include <QtCore/QPointer>

class QueueRemoteTest;

class DummyQueueRemote : public MoleQueue::QueueRemoteSsh
{
  Q_OBJECT
public:
  DummyQueueRemote(const QString &queueName,
                   MoleQueue::QueueManager *parentObject);

  ~DummyQueueRemote();

  QString typeName() const { return "Dummy"; }

  MoleQueue::SshOperation *getDummySshOperation();

  friend class QueueRemoteTest;

protected:
  bool parseQueueId(const QString &submissionOutput,
                    MoleQueue::IdType *queueId);

  bool parseQueueLine(const QString &queueListOutput,
                      MoleQueue::IdType *queueId, MoleQueue::JobState *state);

};

#endif // DUMMYQUEUEREMOTE_H
