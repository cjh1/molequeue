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

#include "queue.h"

#include "filespecification.h"
#include "job.h"
#include "jobmanager.h"
#include "logentry.h"
#include "logger.h"
#include "program.h"
#include "queues/local.h"
#include "queues/remote.h"
#include "queuemanager.h"
#include "server.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>

namespace MoleQueue {

Queue::Queue(const QString &queueName, QueueManager *parentManager) :
  QObject(parentManager), m_queueManager(parentManager),
  m_server((m_queueManager) ? m_queueManager->server() : NULL),
  m_name(queueName)
{
  qRegisterMetaType<Program*>("MoleQueue::Program*");
  qRegisterMetaType<const Program*>("const MoleQueue::Program*");
  qRegisterMetaType<IdType>("MoleQueue::IdType");
  qRegisterMetaType<JobState>("MoleQueue::JobState");

  if (m_server) {
    connect(m_server->jobManager(),
            SIGNAL(jobAboutToBeRemoved(const MoleQueue::Job&)),
            this, SLOT(jobAboutToBeRemoved(const MoleQueue::Job&)));
  }
}

Queue::~Queue()
{
  QList<Program*> programList = m_programs.values();
  m_programs.clear();
  qDeleteAll(programList);
}

void Queue::readSettings(QSettings &settings)
{
  m_launchTemplate = settings.value("launchTemplate").toString();
  m_launchScriptName = settings.value("launchScriptName").toString();

  int jobIdMapSize = settings.beginReadArray("JobIdMap");
  for (int i = 0; i < jobIdMapSize; ++i) {
    settings.setArrayIndex(i);
    m_jobs.insert(static_cast<IdType>(settings.value("queueId").toInt()),
                  static_cast<IdType>(settings.value("moleQueueId").toInt()));
  }
  settings.endArray(); // JobIdMap

  QStringList progNames = settings.value("programs").toStringList();

  settings.beginGroup("Programs");
  foreach (const QString &progName, progNames) {
    settings.beginGroup(progName);

    Program *program = new Program (this);
    program->setName(progName);
    program->readSettings(settings);

    if (!addProgram(program)) {
      Logger::logDebugMessage(tr("Cannot add program '%1' to queue '%2': "
                                 "program name already exists!")
                              .arg(progName).arg(name()));
      delete program;
      program = NULL;
    }

    settings.endGroup(); // progName
  }
  settings.endGroup(); // "Programs"
}

void Queue::writeSettings(QSettings &settings) const
{
  settings.setValue("launchTemplate", m_launchTemplate);
  settings.setValue("launchScriptName", m_launchScriptName);

  QList<IdType> keys = m_jobs.keys();
  settings.beginWriteArray("JobIdMap", keys.size());
  for (int i = 0; i < keys.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("queueId", keys[i]);
    settings.setValue("moleQueueId", m_jobs[keys[i]]);
  }
  settings.endArray(); // JobIdMap

  settings.setValue("programs", programNames());
  settings.beginGroup("Programs");
  foreach (const Program *prog, programs()) {
    settings.beginGroup(prog->name());
    prog->writeSettings(settings);
    settings.endGroup(); // prog->name()
  }
  settings.endGroup(); // "Programs"
}

void Queue::exportConfiguration(QSettings &exporter, bool includePrograms) const
{
  exporter.setValue("type", typeName());
  exporter.setValue("launchTemplate", m_launchTemplate);
  exporter.setValue("launchScriptName", m_launchScriptName);

  if (includePrograms) {
    exporter.setValue("programs", programNames());
    exporter.beginGroup("Programs");
    foreach (const Program *prog, programs()) {
      exporter.beginGroup(prog->name());
      prog->exportConfiguration(exporter);
      exporter.endGroup(); // prog->name()
    }
    exporter.endGroup(); // "Programs"
  }
}

void Queue::importConfiguration(QSettings &importer, bool includePrograms)
{
  m_launchTemplate = importer.value("launchTemplate").toString();
  m_launchScriptName = importer.value("launchScriptName").toString();

  if (includePrograms) {
    QStringList progNames = importer.value("programs").toStringList();

    importer.beginGroup("Programs");
    foreach (const QString &progName, progNames) {
      importer.beginGroup(progName);

      Program *program = new Program (this);
      program->setName(progName);
      program->importConfiguration(importer);

      if (!addProgram(program)) {
        Logger::logDebugMessage(tr("Cannot add program '%1' to queue '%2': "
                                   "program name already exists!")
                                .arg(progName).arg(name()));
        delete program;
        program = NULL;
      }

      importer.endGroup(); // progName
    }
    importer.endGroup(); // "Programs"
  }
}

AbstractQueueSettingsWidget* Queue::settingsWidget()
{
  return NULL;
}

bool Queue::addProgram(Program *newProgram, bool replace)
{
  // Check for duplicates, unless we are replacing, and return false if found.
  if (m_programs.contains(newProgram->name())) {
    if (replace)
      m_programs.take(newProgram->name())->deleteLater();
    else
      return false;
  }

  m_programs.insert(newProgram->name(), newProgram);

  if (newProgram->parent() != this)
    newProgram->setParent(this);

  emit programAdded(newProgram->name(), newProgram);
  return true;
}

bool Queue::removeProgram(Program* programToRemove)
{
  return removeProgram(programToRemove->name());
}

bool Queue::removeProgram(const QString &programName)
{
  if (!m_programs.contains(programName))
    return false;

  Program *program = m_programs.take(programName);

  emit programRemoved(programName, program);
  return true;
}

void Queue::replaceLaunchScriptKeywords(QString &launchScript, const Job &job,
                                        bool addNewline)
{
  launchScript.replace("$$moleQueueId$$", QString::number(job.moleQueueId()));

  launchScript.replace("$$numberOfCores$$",
                       QString::number(job.numberOfCores()));

  job.replaceLaunchScriptKeywords(launchScript);

  // Remove any unreplaced keywords
  QRegExp expr("[^\\$]?(\\${2,3}[^\\$\\s]+\\${2,3})[^\\$]?");
  while (expr.indexIn(launchScript) != -1) {
    Logger::logWarning(tr("Unhandled keyword in launch script: %1. Removing.")
                       .arg(expr.cap(1)), job.moleQueueId());
    launchScript.remove(expr.cap(1));
  }

  // Add newline at end if not present
  if (addNewline && !launchScript.isEmpty() &&
      !launchScript.endsWith(QChar('\n'))) {
    launchScript.append(QChar('\n'));
  }
}

bool Queue::writeInputFiles(const Job &job)
{
  QString workdir = job.localWorkingDirectory();

  // Lookup program.
  if (!m_server) {
    Logger::logError(tr("Queue '%1' cannot locate Server instance!")
                     .arg(m_name),
                     job.moleQueueId());
    return false;
  }
  const Program *program = lookupProgram(job.program());
  if (!program) {
    Logger::logError(tr("Queue '%1' cannot locate program '%2'!")
                     .arg(m_name).arg(job.program()),
                     job.moleQueueId());
    return false;
  }

  // Create directory
  QDir dir (workdir);

  /// Send a warning but don't bail if the path already exists.
  if (dir.exists()) {
    Logger::logWarning(tr("Directory already exists: %1")
                       .arg(dir.absolutePath()),
                       job.moleQueueId());
  }
  else {
    if (!dir.mkpath(dir.absolutePath())) {
      Logger::logError(tr("Cannot create directory: %1")
                       .arg(dir.absolutePath()),
                       job.moleQueueId());
      return false;
    }
  }

  // Create input files
  FileSpecification inputFile = job.inputFile();
  if (!program->inputFilename().isEmpty() && inputFile.isValid()) {
    /// @todo Allow custom file names, only specify extension in program.
    /// Use $$basename$$ keyword replacement.
    inputFile.writeFile(dir, program->inputFilename());
  }

  // Write additional input files
  QList<FileSpecification> additionalInputFiles = job.additionalInputFiles();
  foreach (const FileSpecification &filespec, additionalInputFiles) {
    if (!filespec.isValid()) {
      Logger::logError(tr("Writing additional input files...invalid FileSpec:\n"
                          "%1").arg(filespec.asJsonString()),
                       job.moleQueueId());
      return false;
    }
    QFileInfo target(dir.absoluteFilePath(filespec.filename()));
    switch (filespec.format()) {
    default:
    case FileSpecification::InvalidFileSpecification:
      Logger::logWarning(tr("Cannot write input file. Invalid filespec:\n%1")
                         .arg(filespec.asJsonString()), job.moleQueueId());
      continue;
    case FileSpecification::PathFileSpecification: {
      QFileInfo source(filespec.filepath());
      if (!source.exists()) {
        Logger::logError(tr("Writing additional input files...Source file "
                            "does not exist! %1")
                         .arg(source.absoluteFilePath()), job.moleQueueId());
        return false;
      }
      if (source == target) {
        Logger::logWarning(tr("Refusing to copy additional input file...source "
                              "and target refer to the same file!\nSource: %1"
                              "\nTarget: %2").arg(source.absoluteFilePath())
                           .arg(target.absoluteFilePath()), job.moleQueueId());
        continue;
      }
    }
    case FileSpecification::ContentsFileSpecification:
      if (target.exists()) {
        Logger::logWarning(tr("Writing additional input files...Overwriting "
                              "existing file: '%1'")
                           .arg(target.absoluteFilePath()), job.moleQueueId());
        QFile::remove(target.absoluteFilePath());
      }
      filespec.writeFile(dir);
      continue;
    }
  }

  // Do we need a driver script?
  const QueueLocal *localQueue = qobject_cast<const QueueLocal*>(this);
  const QueueRemote *remoteQueue = qobject_cast<const QueueRemote*>(this);
  if ((localQueue && program->launchSyntax() == Program::CUSTOM) ||
      remoteQueue) {
    QFile launcherFile (dir.absoluteFilePath(launchScriptName()));
    if (!launcherFile.open(QFile::WriteOnly | QFile::Text)) {
      Logger::logError(tr("Cannot open file for writing: %1.")
                       .arg(launcherFile.fileName()),
                       job.moleQueueId());
      return false;
    }
    QString launchString = program->launchTemplate();

    replaceLaunchScriptKeywords(launchString, job);

    launcherFile.write(launchString.toLatin1());
    if (!launcherFile.setPermissions(
          launcherFile.permissions() | QFile::ExeUser)) {
      Logger::logError(tr("Cannot set executable permissions on file: %1.")
                       .arg(launcherFile.fileName()),
                       job.moleQueueId());
      return false;
    }
    launcherFile.close();
  }

  return true;
}

bool Queue::recursiveRemoveDirectory(const QString &p)
{
  QString path = QDir::cleanPath(p);
  if (path.isEmpty() || path.simplified() == "/") {
    Logger::logError(tr("Refusing to remove directory '%1'.").arg(path));
    return false;
  }

  bool result = true;
  QDir dir;
  dir.setPath(path);

  if (dir.exists()) {
    foreach (QFileInfo info, dir.entryInfoList(
               QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
               QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir())
        result = recursiveRemoveDirectory(info.absoluteFilePath());
      else
        result = QFile::remove(info.absoluteFilePath());

      if (!result) {
        Logger::logError(tr("Cannot remove '%1' from local filesystem.")
                         .arg(info.absoluteFilePath()));
        return false;
      }
    }
    result = dir.rmdir(path);
  }

  if (!result) {
    Logger::logError(tr("Cannot remove '%1' from local filesystem.").arg(path));
    return false;
  }

  return true;
}

bool Queue::recursiveCopyDirectory(const QString &from, const QString &to)
{
  bool result = true;

  QDir fromDir;
  fromDir.setPath(from);
  if (!fromDir.exists()) {
    Logger::logError(tr("Cannot copy '%1' --> '%2': source directory does not "
                        "exist.").arg(from, to));
    return false;
  }

  QDir toDir;
  toDir.setPath(to);
  if (!toDir.exists()) {
    if (!toDir.mkdir(toDir.absolutePath())) {
      Logger::logError(tr("Cannot copy '%1' --> '%2': cannot mkdir target "
                          "directory.").arg(from, to));
      return false;
    }
  }

  foreach (QFileInfo info, fromDir.entryInfoList(
             QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
             QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
    QString newTargetPath = QString ("%1/%2")
    .arg(toDir.absolutePath(),
         fromDir.relativeFilePath(info.absoluteFilePath()));
    if (info.isDir()) {
      result = recursiveCopyDirectory(info.absoluteFilePath(),
                                            newTargetPath);
    }
    else {
      result = QFile::copy(info.absoluteFilePath(),
                           newTargetPath);
    }

    if (!result) {
      Logger::logError(tr("Cannot copy '%1' --> '%2'.")
                       .arg(info.absoluteFilePath(), newTargetPath));
      return false;
    }
  }

  return true;
}

bool Queue::addJobFailure(IdType moleQueueId)
{
  if (!m_failureTracker.contains(moleQueueId)) {
    m_failureTracker.insert(moleQueueId, 1);
    return true;
  }

  int failures = ++m_failureTracker[moleQueueId];

  if (failures > 3) {
    Logger::logError(tr("Maximum number of retries for job %1 exceeded.")
                     .arg(moleQueueId), moleQueueId);
    clearJobFailures(moleQueueId);
    return false;
  }

  return true;
}

void Queue::jobAboutToBeRemoved(const Job &job)
{
  m_failureTracker.remove(job.moleQueueId());
  m_jobs.remove(job.queueId());
}

void Queue::cleanLocalDirectory(const Job &job)
{
  recursiveRemoveDirectory(job.localWorkingDirectory());
}

} // End namespace
