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

#include "dummysshcommand.h"


class SshCommandTest : public QObject
{
  Q_OBJECT

private:
  DummySshConnection m_ssh;

private slots:
  /// Called before the first test function is executed.
  void initTestCase();
  /// Called after the last test function is executed.
  void cleanupTestCase();
  /// Called before each test function is executed.
  void init();
  /// Called after every test function.
  void cleanup();

  void sanityCheck();
  void testExecute();
  void testCopyTo();
  void testCopyFrom();
  void testCopyDirTo();
  void testCopyDirFrom();
};

void SshCommandTest::initTestCase()
{
}

void SshCommandTest::cleanupTestCase()
{
}

void SshCommandTest::init()
{
  m_ssh.setSshCommand("ssh");
  m_ssh.setScpCommand("scp");
  m_ssh.setHostName("host");
  m_ssh.setUserName("user");
  m_ssh.setPortNumber(22);
}

void SshCommandTest::cleanup()
{
}

void SshCommandTest::sanityCheck()
{
  m_ssh.setSshCommand("mySsh");
  QCOMPARE(m_ssh.sshCommand(), QString("mySsh"));
  m_ssh.setScpCommand("myScp");
  QCOMPARE(m_ssh.scpCommand(), QString("myScp"));

  MoleQueue::SshOperation *op = m_ssh.newCommand("test");

  op->setData(QVariant("Test"));
  QCOMPARE(op->data().toString(), QString("Test"));
}

void SshCommandTest::testExecute()
{
  MoleQueue::SshOperation *op = m_ssh.newCommand("ls ~");

  DummySshExecOperation *execCommand
       = qobject_cast<DummySshExecOperation *>(op);

  op->execute();

  QCOMPARE(execCommand->getDummyCommand(), QString("ssh"));
  QCOMPARE(execCommand->getDummyArgs(), QStringList ()
           << QString("-q")
           << QString("user@host")
           << QString("ls ~"));
}

void SshCommandTest::testCopyTo()
{
  MoleQueue::SshOperation *op = m_ssh.newFileUpload("C:/local/path",
                                                    "/remote/path");

  DummySshFileUploadOperation *fileUpload
        = qobject_cast<DummySshFileUploadOperation *>(op);

  op->execute();

  QCOMPARE(fileUpload->getDummyCommand(), QString("scp"));
  QCOMPARE(fileUpload->getDummyArgs(), QStringList ()
           << QString("-q")
           << QString("-S") << QString("ssh")
           << QString("C:/local/path")
           << QString("user@host:/remote/path"));
}

void SshCommandTest::testCopyFrom()
{
  MoleQueue::SshOperation *op = m_ssh.newFileDownLoad("/remote/path",
                                                      "C:/local/path");

  DummySshFileDownloadOperation *fileDownload
      = qobject_cast<DummySshFileDownloadOperation *>(op);

  op->execute();

  QCOMPARE(fileDownload->getDummyCommand(), QString("scp"));
  QCOMPARE(fileDownload->getDummyArgs(), QStringList ()
           << QString("-q")
           << QString("-S") << QString("ssh")
           << QString("user@host:/remote/path")
           << QString("C:/local/path"));
}

void SshCommandTest::testCopyDirTo()
{

  MoleQueue::SshOperation *op = m_ssh.newDirUpload("C:/local/path",
                                                   "/remote/path");

  DummySshDirUploadOperation *dirUpload
    = qobject_cast<DummySshDirUploadOperation *>(op);

  op->execute();

  QCOMPARE(dirUpload->getDummyCommand(), QString("scp"));
  QCOMPARE(dirUpload->getDummyArgs(), QStringList ()
           << QString("-q")
           << QString("-S") << QString("ssh")
           << QString("-r")
           << QString("C:/local/path")
           << QString("user@host:/remote/path"));
}

void SshCommandTest::testCopyDirFrom()
{
  MoleQueue::SshOperation *op = m_ssh.newDirDownload("/remote/path",
                                                     "C:/local/path");

  DummySshDirDownloadOperation *dirDownload
    = qobject_cast<DummySshDirDownloadOperation *>(op);

  op->execute();

  qDebug() << "op: " << dirDownload->getDummyArgs();


  QCOMPARE(dirDownload->getDummyCommand(), QString("scp"));
  QCOMPARE(dirDownload->getDummyArgs(), QStringList ()
           << QString("-q")
           << QString("-S") << QString("ssh")
           << QString("-r")
           << QString("user@host:/remote/path")
           << QString("C:/local/path"));
}

QTEST_MAIN(SshCommandTest)

#include "sshcommandtest.moc"
