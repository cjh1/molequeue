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

#include <QtTest>
#include "testserver.h"

#include "serverconnection.h"

#include "job.h"
#include "jobmanager.h"
#include "molequeueglobal.h"
#include "program.h"
#include "queue.h"
#include "queuemanager.h"
#include "server.h"
#include "zeromqclient.h"
#include "transport/localsocketconnection.h"

#include <QtNetwork/QLocalSocket>

class QueueDummy : public MoleQueue::Queue
{
  Q_OBJECT
public:
  QueueDummy(MoleQueue::QueueManager *parentManager)
    : MoleQueue::Queue ("Dummy", parentManager)
  {
  }
public slots:
  bool submitJob(const MoleQueue::Job *);
};

bool QueueDummy::submitJob(const MoleQueue::Job *)
{
  return false;
}

class ZeroMqConnectionTest : public QObject
{
  Q_OBJECT

private:
  MoleQueue::Server *m_server;
  MoleQueue::PacketType m_packet;

private slots:
  MoleQueue::PacketType readReferenceString(const QString &filename);

  /// Called before the first test function is executed.
  void initTestCase();
  /// Called after the last test function is executed.
  void cleanupTestCase();
  /// Called before each test function is executed.
  void init();
  /// Called after every test function.
  void cleanup();

  // ServerConnection slots
  void testSendQueueList();
  void testSendSuccessfulSubmissionResponse();
//  void testSendFailedSubmissionResponse();
//  void testSendSuccessfulCancellationResponse();
//  void testJobStateChangeNotification();
//
//  // ServerConnection signals
//  void testQueueListRequested();
//  void testJobSubmissionRequested();
//  void testJobCancellationRequested();
};


MoleQueue::PacketType ZeroMqConnectionTest::readReferenceString(const QString &filename)
{
  QString realFilename = TESTDATADIR + filename;
  QFile refFile (realFilename);
  if (!refFile.open(QFile::ReadOnly)) {
    qDebug() << "Cannot access reference file" << realFilename;
    return "";
  }
  MoleQueue::PacketType contents = refFile.readAll();
  refFile.close();
  return contents;
}

void ZeroMqConnectionTest::initTestCase()
{
  m_server = new MoleQueue::Server ();
  m_server->start();

  // Let the event loop run a bit to handle the connections
  qApp->processEvents(QEventLoop::AllEvents, 1000);
}

void ZeroMqConnectionTest::cleanupTestCase()
{
  delete m_server;
}

void ZeroMqConnectionTest::init()
{
  m_packet.clear();
}

void ZeroMqConnectionTest::cleanup()
{
}

void ZeroMqConnectionTest::testSendQueueList()
{
  MoleQueue::QueueListType testQueues;

  QString queueName = "Some big ol' cluster";
  QStringList progNames;
  progNames << "Quantum Tater" << "Crystal Math" << "Nebulous Nucleus";
  testQueues[queueName] = progNames;


  queueName = "Puny local queue";
  progNames.clear();
  progNames << "SpectroCrunch" << "FastFocker" << "SpeedSlater";
  testQueues[queueName] = progNames;

  // Create phony queues
  MoleQueue::QueueManager* qmanager = m_server->queueManager();

  foreach(QString qn, testQueues.keys()) {
    MoleQueue::Queue *queue = new QueueDummy (qmanager);
    queue->setName(qn);
    qmanager->addQueue(queue);

    foreach(QString progName, testQueues.value(qn)) {
      MoleQueue::Program *prog = new MoleQueue::Program (NULL);
      prog->setName(progName);
      queue->addProgram(prog);
    }
  }

  MoleQueue::ZeroMqClient client;
  client.connectToServer("MoleQueue2");

  QSignalSpy spy (&client,
                  SIGNAL(queueListUpdated(const MoleQueue::QueueListType&)));

  client.requestQueueListUpdate();

  QTimer timer;
  timer.setSingleShot(true);
  timer.start(10000);
  while (timer.isActive() && spy.isEmpty()) {
    qApp->processEvents(QEventLoop::AllEvents, 500);
  }

  QCOMPARE(spy.count(), 1);

  // Verify we get the same queue list back
  MoleQueue::QueueListType queueList = spy.first().first().value<MoleQueue::QueueListType>();

  foreach(QString qn, testQueues.keys()) {

    QVERIFY2(queueList.contains(qn), "Missing queue");

    foreach(QString pn, testQueues.value(qn)) {
      QVERIFY2(queueList.value(qn).contains(pn), "Missing program");
    }
  }
}

void ZeroMqConnectionTest::testSendSuccessfulSubmissionResponse()
{
  MoleQueue::Job req;

  req.setLocalWorkingDirectory("/tmp/some/path");
  req.setMolequeueId(1);
  req.setClientId(2);
  req.setQueueJobId(1439932);

  MoleQueue::ZeroMqClient client;
  client.connectToServer("MoleQueue2");

  QSignalSpy spy (&client,
                    SIGNAL(jobSubmitted(const MoleQueue::Job*, bool,
                                        const QString&)));

  client.submitJobRequest(&req);

  QTimer timer;
    timer.setSingleShot(true);
    timer.start(10000);
    while (timer.isActive() && spy.isEmpty()) {
      qApp->processEvents(QEventLoop::AllEvents, 500);
    }

  QCOMPARE(spy.count(), 1);


}

//void ZeroMqConnectionTest::testSendFailedSubmissionResponse()
//{
//  MoleQueue::Job req;
//
//  // Fake the request
//  m_serverConnection->jobSubmissionRequestReceived(92, req.hash());
//
//  // Get the molequeue id of the submitted job
//  MoleQueue::IdType mqId =
//      m_serverConnection->m_server->jobManager()->jobs().last()->moleQueueId();
//  req.setMolequeueId(mqId);
//
//  // Send the reply
//  m_serverConnection->sendFailedSubmissionResponse(&req, MoleQueue::Success,
//                                                   "Not a real error!");
//
//  QVERIFY2(m_testServer->waitForPacket(), "Timeout waiting for reply.");
//
//  MoleQueue::PacketType refPacket =
//      this->readReferenceString("serverconnection-ref/submit-failure.json");
//
//  QCOMPARE(QString(m_packet), QString(refPacket));
//}
//
//void ZeroMqConnectionTest::testSendSuccessfulCancellationResponse()
//{
//  MoleQueue::Job req;
//  req.setMolequeueId(21);
//
//  // Fake the request
//  m_serverConnection->jobCancellationRequestReceived(93, req.moleQueueId());
//
//  // Send the reply
//  m_serverConnection->sendSuccessfulCancellationResponse(&req);
//
//  QVERIFY2(m_testServer->waitForPacket(), "Timeout waiting for reply.");
//
//  MoleQueue::PacketType refPacket =
//      this->readReferenceString("serverconnection-ref/cancel-success.json");
//
//  QCOMPARE(QString(m_packet), QString(refPacket));
//}
//
//void ZeroMqConnectionTest::testJobStateChangeNotification()
//{
//  MoleQueue::Job req;
//  req.setMolequeueId(15);
//
//  m_serverConnection->sendJobStateChangeNotification(
//        &req, MoleQueue::RunningLocal, MoleQueue::Finished);
//
//
//  QVERIFY2(m_testServer->waitForPacket(), "Timeout waiting for reply.");
//
//  MoleQueue::PacketType refPacket =
//      this->readReferenceString("serverconnection-ref/state-change.json");
//
//  QCOMPARE(QString(m_packet), QString(refPacket));
//}
//
//void ZeroMqConnectionTest::testQueueListRequested()
//{
//  QSignalSpy spy (m_serverConnection, SIGNAL(queueListRequested(MoleQueue::IdType)));
//
//  MoleQueue::PacketType response =
//      this->readReferenceString("serverconnection-ref/queue-list-request.json");
//
//  m_testServer->sendPacket(response);
//
//  qApp->processEvents(QEventLoop::AllEvents, 1000);
//  QCOMPARE(spy.count(), 1);
//}
//
//void ZeroMqConnectionTest::testJobSubmissionRequested()
//{
//  QSignalSpy spy (m_serverConnection,
//                  SIGNAL(jobSubmissionRequested(const MoleQueue::Job*)));
//
//  MoleQueue::PacketType response =
//      this->readReferenceString("serverconnection-ref/job-request.json");
//
//  m_testServer->sendPacket(response);
//
//  qApp->processEvents(QEventLoop::AllEvents, 1000);
//  QCOMPARE(spy.count(), 1);
//  QCOMPARE(spy.first().size(), 1);
//
//  const MoleQueue::Job *req = spy.first().first().value<const MoleQueue::Job*>();
//  QCOMPARE(req->description(), QString("spud slicer 28"));
//}
//
//void ZeroMqConnectionTest::testJobCancellationRequested()
//{
//  QSignalSpy spy (m_serverConnection,
//                  SIGNAL(jobCancellationRequested(MoleQueue::IdType)));
//
//  MoleQueue::PacketType response =
//      this->readReferenceString("serverconnection-ref/job-cancellation.json");
//
//  m_testServer->sendPacket(response);
//
//  qApp->processEvents(QEventLoop::AllEvents, 1000);
//  QCOMPARE(spy.count(), 1);
//  QCOMPARE(spy.first().size(), 1);
//
//  MoleQueue::IdType moleQueueId = spy.first().first().value<MoleQueue::IdType>();
//  QCOMPARE(moleQueueId, static_cast<MoleQueue::IdType>(0));
//}

QTEST_MAIN(ZeroMqConnectionTest)

#include "moc_zeromqconnectiontest.cxx"
