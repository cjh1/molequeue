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

#include "libssh2copyoperation.h"
#include "libssh2connection.h"

#include <QDebug>
#include <QHostInfo>
#include <QList>
#include <QTimer>
#include <QtGui/QApplication>
#include <QtCore/QFile>
#include <QtCore/QDir>


//#include "libssh2_config.h"


#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_ARPA_INET_H 1

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

namespace MoleQueue
{

SftpOperation::SftpOperation(QObject *parentObject,
                             LibSsh2Connection *connection) :
  LibSsh2Operation(parentObject, connection), m_sftp_session(NULL)
{

}

SftpOperation::SftpOperation(LibSsh2Connection *connection) :
  LibSsh2Operation(connection, connection), m_sftp_session(NULL)
{

}

SftpOperation::SftpOperation(QObject *parentObject,
                             LibSsh2Connection *connection,
                             LIBSSH2_SFTP *sftp_session) :
  LibSsh2Operation(parentObject, connection), m_sftp_session(sftp_session)
{

}

SftpOperation::SftpOperation(LibSsh2Connection *connection,
                             LIBSSH2_SFTP *sftp_session) :
  LibSsh2Operation(connection, connection), m_sftp_session(sftp_session)
{

}



bool SftpOperation::execute()
{
  openSftpSession();

  return true;
}

void SftpOperation::openSftpSession()
{

  if(m_sftp_session == NULL) {
    qDebug() << "Open session: ";
    m_sftp_session = libssh2_sftp_init(m_connection->session());
    qDebug() << "Opened session: ";
  }

  qDebug() << "session " <<  m_sftp_session;
  if(!m_sftp_session) {
     if(libssh2_session_last_errno(m_connection->session()) == LIBSSH2_ERROR_EAGAIN)
     {
       waitForSocket(SLOT(openSftpSession()));
     } else {
       fprintf(stderr, "Unable to init SFTP session\n");
       return;
       //goto shutdown;
     }
  }
  else {
    qDebug() << "open " << m_sftp_session;
    sftpOpen();
  }
}


RemoveRemoteDir::RemoveRemoteDir(LibSsh2Connection *connection,
                                 const QString &remoteDir) :
  SftpOperation(connection), m_remoteDir(remoteDir)
{

}

void RemoveRemoteDir::sftpOpen()
{
  removeDir();
}

void RemoveRemoteDir::removeDir()
{

  int rc = libssh2_sftp_rmdir(m_sftp_session, m_remoteDir.toLocal8Bit().data());

  if(rc == LIBSSH2_ERROR_EAGAIN ) {
    waitForSocket(SLOT(removeDir()));
  }
  else if( rc == 0 ) {
    emit complete();
  }
  else {
    qWarning("libssh2_sftp_rmdir error: " + rc);
  }
}

TransferOperation::TransferOperation(QObject *parentObject,
                                     LibSsh2Connection *connection,
                                     const QString &localFile,
                                     const QString &remoteFile) :
  SftpOperation(parentObject, connection),
  m_sftp_handle(NULL),
  m_localFile(localFile),
  m_remoteFile(remoteFile)
{

}


TransferOperation::TransferOperation(QObject *parentObject,
                                     LibSsh2Connection *connection,
                                     LIBSSH2_SFTP *sftp_session,
                                     LIBSSH2_SFTP_HANDLE *sftp_handle,
                                     const QString &localFile,
                                     const QString &remoteFile) :
    SftpOperation(parentObject, connection, sftp_session),
    m_sftp_handle(sftp_handle),
    m_localFile(localFile),
    m_remoteFile(remoteFile)
{


}


TransferOperation::TransferOperation(LibSsh2Connection *connection,
                                     const QString &localFile,
                                     const QString &remoteFile) :
  SftpOperation(connection),
  m_sftp_handle(NULL),
  m_localFile(localFile),
  m_remoteFile(remoteFile)
{

}


TransferOperation::TransferOperation(LibSsh2Connection *connection,
                             LIBSSH2_SFTP *sftp_session,
                             LIBSSH2_SFTP_HANDLE *sftp_handle,
                             const QString &localFile,
                             const QString &remoteFile) :
    SftpOperation(connection, sftp_session),
    m_sftp_handle(sftp_handle),
    m_localFile(localFile),
    m_remoteFile(remoteFile)
{


}

void DirUpload::sftpOpen()
{
  uploadDir();
}


void DirDownload::sftpOpen()
{
  qDebug() << "open dir" << m_remoteFile;
  m_sftp_handle = libssh2_sftp_opendir(m_sftp_session,
                                       m_remoteFile.toLocal8Bit().data());

  if (!m_sftp_handle) {
    if (libssh2_session_last_errno(m_connection->session()) == LIBSSH2_ERROR_EAGAIN) {
      waitForSocket(SLOT(sftpOpen()));
    }
    else {
      fprintf(stderr, "Unable to open file with SFTP\n");
      //goto shutdown;
      return;
    }
  }
  else {
    downloadDir();
    }
}


void FileTransfer ::sftpOpen()
{

  qDebug() << "remote: " << m_remoteFile;

  m_sftp_handle = libssh2_sftp_open(m_sftp_session,
                                    m_remoteFile.toLocal8Bit().data(),
                                    m_flags, m_mode);

  if (!m_sftp_handle) {
    if (libssh2_session_last_errno(m_connection->session()) == LIBSSH2_ERROR_EAGAIN) {
      waitForSocket(SLOT(sftpOpen()));
    }
    else {
      fprintf(stderr, "Unable to open file with SFTP\n");
      //goto shutdown;
      return;
    }
  }
  else {
    sftpTransfer();
  }
}

void FileUpload::sftpTransfer()
{
  sftpWrite();
}

/*** FileDownload ***/

FileDownload::FileDownload(QObject *parentObject,
    LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(parentObject, connection, localFile, remoteFile)
{

  m_flags = LIBSSH2_FXF_READ;
}

FileDownload::FileDownload(QObject *parentObject,
    LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(parentObject, connection, sftp_session,
                      sftp_handle, localFile, remoteFile)
{
  m_flags = LIBSSH2_FXF_READ;

}

FileDownload::FileDownload(LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(connection, localFile, remoteFile)
{
m_flags = LIBSSH2_FXF_READ;

}

FileDownload::FileDownload(LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(connection, sftp_session, sftp_handle,
                      localFile, remoteFile)
{
  m_flags = LIBSSH2_FXF_READ;

}

void FileDownload::sftpTransfer()
{
  sftpRead();
}


/*** FileTransfer ***/

FileTransfer::FileTransfer(QObject *parentObject,
    LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(parentObject, connection, localFile, remoteFile),
    m_file(NULL)
{

}

FileTransfer::FileTransfer(QObject *parentObject,
    LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(parentObject, connection, sftp_session,
                      sftp_handle, localFile, remoteFile),
    m_file(NULL)
{

}

FileTransfer::FileTransfer(LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(connection, localFile, remoteFile),
    m_file(NULL)

{

}

FileTransfer::FileTransfer(LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(connection, sftp_session, sftp_handle,
                      localFile, remoteFile),
    m_file(NULL)
{

}

void FileTransfer::sftpRead()
{

  char buffer[1024*24];

  int rc = libssh2_sftp_read(m_sftp_handle, buffer, sizeof(buffer));

  if(rc == LIBSSH2_ERROR_EAGAIN) {
    waitForSocket(SLOT(sftpRead()));
    return;
  }
  else {
    if(m_file == NULL) {
      m_file = new QFile(m_localFile);
      m_file->open(QIODevice::WriteOnly);
    }
    qDebug() << "rc: " << rc;

    if (rc > 0) {
      qDebug() << "m_file" << m_file;
        m_file->write(buffer, rc);
      sftpRead();
    }
    // Finished
    else if(rc == 0 ) {
      emit complete();
    }
    else {
      qWarning("libssh2_sftp_read error: " + rc);
    }

    m_file->close();
  }

  libssh2_sftp_close(m_sftp_handle);
}

void FileTransfer::sftpWrite()
{

  char mem[1024 * 100];

  if(m_file == NULL) {
    m_file = new QFile(m_localFile);
    m_file->open(QIODevice::ReadOnly);
  }

  qint64 bytesRead = m_file->read(mem, 1024 * 100);

  if (bytesRead <= 0) {
    /* end of file */
    emit complete();
    return;
  }

  /* write data in a loop until we block */
  int rc = libssh2_sftp_write(m_sftp_handle, mem, bytesRead);

  if( rc == LIBSSH2_ERROR_EAGAIN) {
     waitForSocket(SLOT(sftpWrite()));
  }
}

/*** DirTransfer ***/

DirTransfer::DirTransfer(QObject *parentObject,
    LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(parentObject, connection, localFile, remoteFile)
{

}

DirTransfer::DirTransfer(QObject *parentObject,
    LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(parentObject, connection, sftp_session,
                      sftp_handle, localFile, remoteFile)
{

}

DirTransfer::DirTransfer(LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(connection, localFile, remoteFile)
{

}

DirTransfer::DirTransfer(LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    TransferOperation(connection, sftp_session, sftp_handle,
                      localFile, remoteFile)
{

}

void DirTransfer::subOpError(int errorCode, const QString &errorString)
{
  m_errorCode = errorCode;
  m_errorString = errorString;

  emit error(m_errorCode, m_errorString);
  emit complete();
}

void DirTransfer::copyNextFile()
{
  if(!files.empty()) {

    QString file = files.takeFirst();

    QString localPath = m_localFile + "/" + file;
    QString remotePath = m_remoteFile + "/" + file;


    TransferOperation *op = newFileTransfer(localPath,
                                            remotePath);
    connect(op, SIGNAL(complete()),
            this, SLOT(copyNext()));
    //connect errors signal

    connect(op, SIGNAL(error(int, const QString&)),
           this, SLOT(subOpError(int, const QString&)));

    op->execute();
  }
}

void DirTransfer::copyNextDir()
{
  if(!dirs.empty()) {
    QString dir = dirs.takeFirst();

    QString localPath = m_localFile + "/" + dir;
    QString remotePath = m_remoteFile + "/" + dir;

    qDebug() << "copy dir: " << "local: " <<localPath << "remote: " <<remotePath;


    TransferOperation *op = newDirTransfer(localPath,
                                           remotePath);
    connect(op, SIGNAL(complete()),
            this, SLOT(copyNext()));

    op->execute();
  }
}

void DirTransfer::copyNext()
{
  qDebug() << "copyNext: " << dirs << " " << files;

  if(!dirs.empty() && m_errorCode == 0) {
    qDebug() << "copyNextDir";
    copyNextDir();
  }
  else if(!files.empty()) {
    qDebug() << "copyNextFile";
    copyNextFile();
  }
  else {
    qDebug() << "complete";
    emit complete();
  }
}

/*** FileUpload ***/

FileUpload::FileUpload(QObject *parentObject,
    LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(parentObject, connection, localFile, remoteFile)
{
  m_flags = LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC;
  m_mode = LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
              LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH;
}

FileUpload::FileUpload(QObject *parentObject,
    LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(parentObject, connection, sftp_session,
                      sftp_handle, localFile, remoteFile)
{
  m_flags = LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC;
  m_mode = LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
            LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH;

}

FileUpload::FileUpload(LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(connection, localFile, remoteFile)
{
  m_flags = LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC;
  m_mode = LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
            LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH;

}

FileUpload::FileUpload(LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    FileTransfer(connection, sftp_session, sftp_handle,
                 localFile, remoteFile)
{
  m_flags = LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC;
  m_mode = LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|
            LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH;

}

/*** DirDownload ***/

DirDownload::DirDownload(QObject *parentObject,
    LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(parentObject, connection, localFile, remoteFile)
{

}

DirDownload::DirDownload(QObject *parentObject,
    LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(parentObject, connection, sftp_session,
                      sftp_handle, localFile, remoteFile)
{

}

DirDownload::DirDownload(LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(connection, localFile, remoteFile)
{

}

DirDownload::DirDownload(LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(connection, m_sftp_session, m_sftp_handle,
                      localFile, remoteFile)
{

}

TransferOperation *DirDownload::newDirTransfer(const QString &localDir,
                                                const QString &remoteDir)
{
  return new DirDownload(this, m_connection, m_sftp_session, m_sftp_handle,
                         localDir, remoteDir);
}

TransferOperation *DirDownload::newFileTransfer(const QString &localDir,
                                                const QString &remoteDir)
{
  return new FileDownload(this, m_connection, m_sftp_session, m_sftp_handle,
                         localDir, remoteDir);
}

/*** DirUpload ***/

DirUpload::DirUpload(QObject *parentObject,
    LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(parentObject, connection, localFile, remoteFile)
{

}

DirUpload::DirUpload(QObject *parentObject,
    LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(parentObject, connection, sftp_session,
                      sftp_handle, localFile, remoteFile)
{

}

DirUpload::DirUpload(LibSsh2Connection *connection,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(connection, localFile, remoteFile)
{

}

DirUpload::DirUpload(LibSsh2Connection *connection,
    LIBSSH2_SFTP *sftp_session,
    LIBSSH2_SFTP_HANDLE *sftp_handle,
    const QString &localFile,
    const QString &remoteFile) :
    DirTransfer(connection, sftp_session, sftp_handle,
                      localFile, remoteFile)
{

}

TransferOperation *DirUpload::newDirTransfer(const QString &localDir,
                               const QString &remoteDir)
{
  return new DirUpload(this, m_connection, m_sftp_session, m_sftp_handle,
                       localDir, remoteDir);
}

TransferOperation *DirUpload::newFileTransfer(const QString &localFile,
                               const QString &remoteFile)
{
  return new FileUpload(this, m_connection, m_sftp_session, m_sftp_handle,
                       localFile, remoteFile);
}

void DirUpload::uploadDir()
{
  // Add trailing slashes:
  QString localpath = m_localFile + "/";
  QString remotepath = m_remoteFile + "/";

  // Open local dir
  QDir locdir(localpath);
  if (!locdir.exists()) {
    qWarning() << "Could not open local directory " << localpath;
    return;
  }

  // Get listing of all items to copy
  dirs = locdir.entryList(QDir::AllDirs |
                          QDir::NoDotAndDotDot,
                          QDir::Name);

  files = locdir.entryList(QDir::Files, QDir::Name);

  // Create remote directory
  while (libssh2_sftp_mkdir(m_sftp_session, remotepath.toLocal8Bit().data(),
                            LIBSSH2_SFTP_S_IRWXU|
                            LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IXGRP|
                            LIBSSH2_SFTP_S_IROTH|LIBSSH2_SFTP_S_IXOTH)
         == LIBSSH2_ERROR_EAGAIN);

  copyNext();

}

void TransferOperation::sftpCleanup()
{
  libssh2_sftp_shutdown(m_sftp_session);

}

void TransferOperation::cleanUpOperation()
{
  TransferOperation *note = qobject_cast<TransferOperation*>(sender());
  if(note != NULL) {
    note->deleteLater();
  }
}

void DirDownload::downloadDir()
{
  char mem[512];
  LIBSSH2_SFTP_ATTRIBUTES attrs;

  int rc;

  QString localpath = m_localFile + "/";
  QString remotepath = m_remoteFile + "/";


  qDebug() <<"downloadDitr localPath: " << localpath;

  // Create local directory
   QDir locdir;
   if (!locdir.mkpath(localpath)) {
     qWarning() << "Could not create local directory " << localpath;
     //sftp_free(sftp);
     return;
   }

  do
  {
    while ((rc = libssh2_sftp_readdir(m_sftp_handle, mem, sizeof(mem),&attrs)) ==
        LIBSSH2_ERROR_EAGAIN) {
             ;
    }

    if(rc > 0) {

      if (strcmp(mem, ".") == 0 ||
          strcmp(mem, "..") == 0 ) {
          continue;
      }

      if(LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
        dirs.push_back(QString(mem));
      }
      else {
        files.push_back(QString(mem));
      }
      //printf("%s\n", mem);

      qDebug() << "dirs: " << dirs;


    }
    else {
      break;
    }
  }
  while(1);

  copyNext();

  qDebug() << files << dirs;

}

void TransferOperation::completeSlot()
{
  qDebug() << "copy complete";
}

} /* namespace MoleQueue */
