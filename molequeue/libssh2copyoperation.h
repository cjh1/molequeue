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

#ifndef LIBSSH2COPYOPERATION_H_
#define LIBSSH2COPYOPERATION_H_

#include "sshconnection.h"
#include "libssh2operations.h"
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QTcpSocket>
#include <QtCore/QSocketNotifier>
#include <QtCore/QStringList>

namespace MoleQueue
{

class LibSsh2Connection;

class SftpOperation: public LibSsh2Operation
{
  Q_OBJECT
public:
  SftpOperation(QObject *parentObject,
                LibSsh2Connection *connection);

  SftpOperation(LibSsh2Connection *connection);

  SftpOperation(QObject *parentObject,
                LibSsh2Connection *connection,
                LIBSSH2_SFTP *sftp_session);

  SftpOperation(LibSsh2Connection *connection,
                LIBSSH2_SFTP *sftp_session);

public slots:
  bool execute();

protected slots:
  virtual void sftpOpen() = 0;

protected:
  LIBSSH2_SFTP *m_sftp_session;

private slots:
  void openSftpSession();

};

class RemoveRemoteDir : public SftpOperation
{
  Q_OBJECT

public:
  RemoveRemoteDir(LibSsh2Connection *connection,
                  const QString &remoteDir);

protected slots:
  void sftpOpen();

private slots:

  void removeDir();

private:
  QString m_remoteDir;
};

class TransferOperation: public SftpOperation
{
  Q_OBJECT
public:

  TransferOperation(QObject *parentObject,
                    LibSsh2Connection *connection,
                    const QString &localFile,
                    const QString &remoteFile);

  TransferOperation(QObject *parentObject,
                    LibSsh2Connection *connection,
                    LIBSSH2_SFTP *sftp_session,
                    LIBSSH2_SFTP_HANDLE *sftp_handle,
                    const QString &localFile,
                    const QString &remoteFile);

  TransferOperation(LibSsh2Connection *connection,
                    const QString &localFile,
                    const QString &remoteFile);

  TransferOperation(LibSsh2Connection *connection,
                    LIBSSH2_SFTP *sftp_session,
                    LIBSSH2_SFTP_HANDLE *sftp_handle,
                    const QString &localFile,
                    const QString &remoteFile);

public slots:
  void completeSlot();

private slots:
  void sftpCleanup();
  void cleanUpOperation();

protected:
  LIBSSH2_SFTP_HANDLE *m_sftp_handle;
  QString m_localFile;
  QString m_remoteFile;
};

class FileTransfer : public TransferOperation
{
  Q_OBJECT
public:

  FileTransfer(QObject *parentObject,
               LibSsh2Connection *connection,
               const QString &localFile,
               const QString &remoteFile);

  FileTransfer(QObject *parentObject,
               LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localFile,
               const QString &remoteFile);

  FileTransfer(LibSsh2Connection *connection,
               const QString &localFile,
               const QString &remoteFile);

  FileTransfer(LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localFile,
               const QString &remoteFile);

protected slots:
  virtual void sftpTransfer() = 0;
  void sftpRead();
  void sftpWrite();
  void sftpOpen();

protected:
  int m_flags;
  int m_mode;

private:
  QFile *m_file;
};


class FileUpload : public FileTransfer
{
  Q_OBJECT

public:
  FileUpload(QObject *parentObject,
               LibSsh2Connection *connection,
               const QString &localFile,
               const QString &remoteFile);

  FileUpload(QObject *parentObject,
               LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localFile,
               const QString &remoteFile);

  FileUpload(LibSsh2Connection *connection,
               const QString &localFile,
               const QString &remoteFile);

  FileUpload(LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localFile,
               const QString &remoteFile);

protected slots:
  void sftpTransfer();

};

class FileDownload : public FileTransfer
{
  Q_OBJECT
public:
  FileDownload(QObject *parentObject,
               LibSsh2Connection *connection,
               const QString &localFile,
               const QString &remoteFile);

  FileDownload(QObject *parentObject,
               LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localFile,
               const QString &remoteFile);

  FileDownload(LibSsh2Connection *connection,
               const QString &localFile,
               const QString &remoteFile);

  FileDownload(LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localFile,
               const QString &remoteFile);

protected slots:
  void sftpTransfer();


};

class DirTransfer : public TransferOperation
{
  Q_OBJECT

public:
  DirTransfer(QObject *parentObject,
               LibSsh2Connection *connection,
               const QString &localDir,
               const QString &remoteDir);

  DirTransfer(QObject *parentObject,
               LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localDir,
               const QString &remoteDir);

  DirTransfer(LibSsh2Connection *connection,
               const QString &localDir,
               const QString &remoteDir);

  DirTransfer(LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localDir,
               const QString &remoteDir);

protected slots:
  void copyNextFile();
  void copyNextDir();
  void copyNext();

  virtual TransferOperation  *newFileTransfer(const QString &localFile,
                                              const QString &remoteFile) = 0;

  virtual TransferOperation *newDirTransfer(const QString &localDir,
                                            const QString &remoteDir) = 0;

protected:
  QStringList files;
  QStringList dirs;

private slots:
  void subOpError(int errorCode, const QString &errorString);

};

class DirUpload : public DirTransfer
{
  Q_OBJECT

public:
  DirUpload(QObject *parentObject,
               LibSsh2Connection *connection,
               const QString &localDir,
               const QString &remoteDir);

  DirUpload(QObject *parentObject,
               LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localDir,
               const QString &remoteDir);

  DirUpload(LibSsh2Connection *connection,
               const QString &localDir,
               const QString &remoteDir);

  DirUpload(LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localDir,
               const QString &remoteDir);

private slots:
  TransferOperation  *newFileTransfer(const QString &localFile,
                                      const QString &remoteFile);

  TransferOperation  *newDirTransfer(const QString &localDir,
                                     const QString &remoteDir);
  void sftpOpen();

private:
  void uploadDir();

};

class DirDownload : public DirTransfer
{
  Q_OBJECT

public:
  DirDownload(QObject *parentObject,
               LibSsh2Connection *connection,
               const QString &localDir,
               const QString &remoteDir);

  DirDownload(QObject *parentObject,
               LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localDir,
               const QString &remoteDir);

  DirDownload(LibSsh2Connection *connection,
               const QString &localDir,
               const QString &remoteDir);

  DirDownload(LibSsh2Connection *connection,
               LIBSSH2_SFTP *sftp_session,
               LIBSSH2_SFTP_HANDLE *sftp_handle,
               const QString &localDir,
               const QString &remoteDir);
private slots:
  TransferOperation  *newFileTransfer(const QString &localFile,
                                      const QString &remoteFile);

  TransferOperation  *newDirTransfer(const QString &localDir,
                                     const QString &remoteDir);
  void sftpOpen();

private:
  void downloadDir();

};


} /* namespace MoleQueue */

#endif /* LIBSSH2COPYOPERATION_H_ */
