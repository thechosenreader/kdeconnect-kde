#include <filemanagerplugin.h>

#include <KPluginFactory>

#include <QDBusConnection>
#include <core/networkpacket.h>
#include <core/device.h>
#include <core/daemon.h>
#include "interfaces/dbushelpers.h"
#include "plugin_filemanager_debug.h"
#include <dbushelper.h>

#define PACKET_TYPE_FILEMANAGER QStringLiteral("kdeconnect.filemanager")

K_PLUGIN_CLASS_WITH_JSON(FileManagerPlugin, "kdeconnect_filemanager.json")



FileManagerPlugin::FileManagerPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
     qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "FILEMANAGER plugin constructor for device" << device()->name();

     connect(this, &FileManagerPlugin::listingNeedsUpdate, this, &FileManagerPlugin::updateListing);
     connect(this, &FileManagerPlugin::errorNeedsSending, this, &FileManagerPlugin::sendError);
     connect(this, &FileManagerPlugin::fileDeleted, this, &FileManagerPlugin::replaceFile);
}


FileManagerPlugin::~FileManagerPlugin()
{
     qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "FILEMANAGER plugin destructor for device" << device()->name();
}


QString FileManagerPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/filemanager");
}


bool FileManagerPlugin::receivePacket(NetworkPacket const& np){
  if (np.get<bool>(QStringLiteral("requestDirectoryListing"), false)) {
    if (np.has(QStringLiteral("directoryPath")))
      sendListing(np.get<QString>(QStringLiteral("directoryPath")));

    else if (np.has(QStringLiteral("home")) && np.get<bool>(QStringLiteral("home")))
      sendListing(QDir::homePath());

    else
      sendListing();
  }

  else if (np.has(QStringLiteral("path"))) {
    QString filepath = np.get<QString>(QStringLiteral("path"));
    if (np.has(QStringLiteral("requestFileDownload")) && np.get<bool>(QStringLiteral("requestFileDownload"))) {
      sendFile(filepath);
    }

    else if (np.has(QStringLiteral("requestDirectoryDownload")) && np.get<bool>(QStringLiteral("requestDirectoryDownload"))) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "got a request for directory download" << filepath;
      QFuture<void> future = QtConcurrent::run(this, &FileManagerPlugin::sendArchive, filepath);
    }

    else if (np.has(QStringLiteral("requestDelete")) && np.get<bool>(QStringLiteral("requestDelete"))) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "got delete request for" << filepath;
      QFuture<void> future = QtConcurrent::run(this, &FileManagerPlugin::deleteFile, filepath);
    }

    else if (np.has(QStringLiteral("requestRename")) && np.get<bool>(QStringLiteral("requestRename"))) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "got rename request for" << filepath;
      if (np.has(QStringLiteral("newname"))) {
        QString newname = np.get<QString>(QStringLiteral("newname"));
        rename(filepath, newname);
      }
    }

    else if (np.has(QStringLiteral("requestDownloadForViewing")) && np.get<bool>(QStringLiteral("requestDownloadForViewing"))) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "got viewing download request for" << filepath;
      QString dest;
      if (np.has(QStringLiteral("dest"))) {
        dest = np.get<QString>(QStringLiteral("dest"));
      } else { sendMsgPacket(QStringLiteral("failed; no dest received")); return true; }
      shareFileForViewing(filepath, dest);
    }
  }

  else if (np.has(QStringLiteral("requestMakeDirectory")) && np.get<bool>(QStringLiteral("requestMakeDirectory"))) {
    if (np.has(QStringLiteral("dirname"))) {
      QString dirname = np.get<QString>(QStringLiteral("dirname"));
      QFuture<void> future = QtConcurrent::run(this, &FileManagerPlugin::makeDirectory, dirname);
    }
  }

  else if (np.has(QStringLiteral("requestRunCommand")) && np.get<bool>(QStringLiteral("requestRunCommand"))) {
    if (np.has(QStringLiteral("command"))) {
      QString cmd = np.get<QString>(QStringLiteral("command"));
      QString wd;
      if (np.has(QStringLiteral("wd"))) {
        wd = np.get<QString>(QStringLiteral("wd"));
      } else { wd = directory->absolutePath(); }
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "starting thread to run" << cmd << "in" << wd;
      QFuture<void> future = QtConcurrent::run(this, &FileManagerPlugin::runCommand, cmd, wd);
    }
  }

  else if (np.has(QStringLiteral("togglehidden")) && np.get<bool>(QStringLiteral("togglehidden"))) {
    showHidden = !showHidden;
    sendListing();
  }

  else if (np.hasPayload() || np.has(QStringLiteral("filename"))) {
    const QString filename = np.get<QString>(QStringLiteral("filename"));
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "got payload for" << filename;

    QUrl destination = QUrl::fromLocalFile(filename);
    if (np.hasPayload()) {
      FileTransferJob* job = np.createPayloadTransferJob(destination);
      // this will emit fileDeleted, which will trigger replaceFile
      QFuture<void> future = QtConcurrent::run(this, &FileManagerPlugin::deleteFileForReplacement, filename, job);
      // connect(job, &KJob::result, this, [this](KJob* job) { finished(job); });
      // job->start();

    } else {
        // create empty file
        QFile file(destination.toLocalFile());
        file.open(QIODevice::WriteOnly);
        file.close();
    }



  }

  return true;
}


void FileManagerPlugin::connected() {
  // sendListing();
}


void FileManagerPlugin::sendListing(const QString& path) {
  QFileInfo finfo(directory->filePath(path));
  QJsonObject* listing = new QJsonObject();

  if (finfo.isDir()) {
    directory->cd(path);
  } else {
    directory->cd(finfo.absoluteDir().absolutePath());
    if (finfo.exists()) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "sending file" << path;
      sendFile(finfo.absoluteFilePath());
    }
  }

  directory->setPath(directory->absolutePath());
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "cwd is" << directory->absolutePath();

  QList<QFileInfo> files = directory->entryInfoList(QDir::NoDot | QDir::AllEntries | (showHidden ? QDir::Hidden : QFlags<QDir::Filter>(0x0)));

  QList<QFileInfo>::iterator i;
  for (i = files.begin(); i != files.end(); i++) {
    QJsonObject* file = new QJsonObject();
    QFileInfo f = *i;

    QString abspath = f.absoluteFilePath();
    QString filename = f.fileName();
    if (f.isDir())
      filename += QStringLiteral("/");

    QString permissions = permissionsString(f.permissions());
    QString owner       = f.owner();
    QString group       = f.group();
    QString lastmod     = f.lastModified().toString(QStringLiteral("MMM dd hh:mm"));
    qint64 bytesize     = f.size();
    bool readable       = f.isReadable();

    file->insert(QStringLiteral("filename"), filename);
    file->insert(QStringLiteral("permissions"), permissions);
    file->insert(QStringLiteral("owner"), owner);
    file->insert(QStringLiteral("group"), group);
    file->insert(QStringLiteral("lastmod"), lastmod);
    file->insert(QStringLiteral("size"), bytesize);
    file->insert(QStringLiteral("readable"), readable);

    listing->insert(abspath, *file);

  }


  QJsonDocument doc(*listing);
  QString listingJSON = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

  NetworkPacket netpacket(PACKET_TYPE_FILEMANAGER);
  netpacket.set<QString>(QStringLiteral("directoryListing"), listingJSON);
  QString temp = directory->cleanPath(directory->absolutePath());
  netpacket.set<QString>(QStringLiteral("directoryPath"), temp);

  sendPacket(netpacket);

  // qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "constructed JSON=" << listingJSON;
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "sent JSON packet for " << path;

}


void FileManagerPlugin::sendFile(const QString& path) {
  QUrl url = QUrl::fromLocalFile(path);
  QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                      QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/share"),
                      QStringLiteral("org.kde.kdeconnect.device.share"), QStringLiteral("shareUrl"));

  msg.setArguments(QVariantList() << QVariant(url.toString()));

  blockOnReply(DBusHelper::sessionBus().asyncCall(msg));

}


QString FileManagerPlugin::permissionsString(QFileDevice::Permissions permissions) {
  // first three are rwx for owner, then group, then any user
  // the second digit is unused because it defines permissions for the current user
  // bitshifting was inconvenient because the first bit of every hex digit is unused
  // see Qt docs for QFileDevice::Permissions

  qint16 values[] = { 0x4000, 0x2000, 0x1000, 0x0040, 0x0020, 0x0010, 0x0004, 0x0002, 0x0001 };
  QString perms[] = { QStringLiteral("r"), QStringLiteral("w"), QStringLiteral("x")};
  QString permstring = QStringLiteral("");

  for (qint8 i=0; i < 9; i++) {
    if (values[i] & permissions)
      permstring += perms[i % 3];

    else
      permstring += QStringLiteral("-");
  }

  return permstring;
}


QList<QString> FileManagerPlugin::recurseAddDir(const QDir& dir) {
  QList<QString> sourceList;
  QList<QString> entryList = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden);

  for (QList<QString>::iterator i = entryList.begin(); i != entryList.end(); i++) {
    QString entry  = *i;

    // qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "found entry" << entry;
    QFileInfo finfo(QString(QStringLiteral("%1/%2")).arg(dir.path()).arg(entry));
    // qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "made finfo for" << finfo.filePath();

    if (finfo.isSymLink()) {
      continue;
    }

    if (finfo.isDir()) {
      QDir sd(finfo.filePath());
      sourceList += recurseAddDir(sd);
    } else {
      // qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "adding to sourceList" << QDir::toNativeSeparators(finfo.filePath());
      sourceList << QDir::toNativeSeparators(finfo.filePath());
    }
  }

  return sourceList;
}

bool FileManagerPlugin::_archive(const QString& outPath, const QDir& dir) {
  // initializing archive
  QuaZip zip(outPath);
  zip.setFileNameCodec("IBM866");

  // basic error checking, confirming permissions and that dir exists
  if (!zip.open(QuaZip::mdCreate)) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("testCreate(): zip.open(): %1")).arg(zip.getZipError());
      return false;
  }

  if (!dir.exists()) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("dir.exists(%1)=FALSE")).arg(dir.absolutePath());
      return false;
  }


  QList<QString> sourceList = recurseAddDir(dir);

  QList<QFileInfo> files;
  for (QList<QString>::iterator i = sourceList.begin(); i != sourceList.end(); i++){
    // qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "adding file" << *i << "to list";
    files << QFileInfo(*i);
  }

  QuaZipFile outFile(&zip);

  QFile inFile;

  char c;
  for (QList<QFileInfo>::iterator i = files.begin(); i != files.end(); i++) {
      QFileInfo fileinfo = *i;

      if (!fileinfo.isFile())
          continue;

      QString fileNameWithRelativePath = fileinfo.filePath().remove(0, dir.absolutePath().length() + 1);

      inFile.setFileName(fileinfo.filePath());

      if (!inFile.open(QIODevice::ReadOnly)) {
          qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("testCreate(): inFile.open(): ")) << inFile.errorString().toLocal8Bit().constData();
          return false;
      }

      if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileinfo.filePath()))) {
          qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("testCreate(): outFile.open(): %1")).arg(outFile.getZipError());
          return false;
      }

      // qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "zipping" << fileinfo.filePath();
      while (inFile.getChar(&c) && outFile.putChar(c));

      if (outFile.getZipError() != UNZ_OK) {
          qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("testCreate(): outFile.putChar(): %1")).arg(outFile.getZipError());
          return false;
      }

      outFile.close();

      if (outFile.getZipError() != UNZ_OK) {
          qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("testCreate(): outFile.close(): %1")).arg(outFile.getZipError());
          return false;
      }

      inFile.close();
  }

  zip.close();

  if (zip.getZipError() != 0) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << QString(QStringLiteral("testCreate(): zip.close(): %1")).arg(zip.getZipError());
      return false;
  }

  return true;
}

QString FileManagerPlugin::genRandomString(const qint16 maxLength) {
  QString out;

  for (qint16 i = 0; i < maxLength; i++) {
    qint16 randIndex = random->generate() % possibleChars.length();
    out.append(possibleChars.at(randIndex));
  }

  return out;
}

void FileManagerPlugin::sendArchive(const QString& directoryPath) {
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "archiving" << directoryPath;
    if (zippedFiles.contains(directoryPath)) {
      QString zipLocation = zippedFiles.value(directoryPath);
      if (QFile::exists(zipLocation)) {
        sendFile(zipLocation);
        qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "archive already exists, sending" << zipLocation;
        return;
      } else {
        zippedFiles.remove(directoryPath);
      }
    }

    QDir targetDir(directoryPath);
    QString newLocation = QString(QStringLiteral("%1/%2_%3%4")).arg(QDir::tempPath()).arg(targetDir.dirName()).arg(genRandomString()).arg(QStringLiteral(".zip"));
    zippedFiles.insert(directoryPath, newLocation);

    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "creating new archive at" << newLocation;
    if(_archive(newLocation, targetDir)) {
      sendFile(newLocation);
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "sent newly created archive";
    }
    else {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "could not create archive";
      sendMsgPacket(QStringLiteral("Could not create archive!"));
    }
}


bool FileManagerPlugin::_delete(const QString& path) {
  QFileInfo finfo(path);

  if (finfo.isDir()) {
    QDir dir(path);
    return dir.removeRecursively();
  }

  else {
    return directory->remove(path);
  }

}


bool FileManagerPlugin::deleteFile(const QString& path) {
  bool success = _delete(path);

  if (success) {
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "successfully deleted" << path;
    Q_EMIT listingNeedsUpdate();

  } else {
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "could not delete" << path;
    Q_EMIT errorNeedsSending(QString(QStringLiteral("Error deleting file %1").arg(path)));
  }

  return success;

}


void FileManagerPlugin::rename(const QString& path, const QString& newname) {
  QFileInfo current(directory->absoluteFilePath(path));
  QFileInfo target(directory->absoluteFilePath(newname));
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "current" << current.fileName();
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "target" << target.absoluteFilePath();
  QString targetName;
  if (target.exists() && target.isDir()) {
    // by default, rename fails whenever target exists
    // this way, we can mimic the behaviour of mv: mv file dir -> dir/file
    targetName = QString(QStringLiteral("%1/%2")).arg(target.absoluteFilePath()).arg(current.fileName());
  } else { targetName = newname; }
  bool success = directory->rename(path, targetName);

  if (success) {
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "successfully renamed" << path << "to" << targetName;
    Q_EMIT updateListing();
  } else {
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "failed to rename" << path << "to" << targetName;
    Q_EMIT errorNeedsSending(QString(QStringLiteral("Could not rename %1 to %2")).arg(path).arg(newname));
  }

}


void FileManagerPlugin::updateListing() {
  sendListing();
}


void FileManagerPlugin::sendError(const QString& msg) {
  sendMsgPacket(msg);
}


void FileManagerPlugin::sendMsgPacket(const QString& errorMsg) {
  NetworkPacket np(PACKET_TYPE_FILEMANAGER);
  np.set<QString>(QStringLiteral("Error"), errorMsg);
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "created msg packet with msg" << errorMsg;
  sendPacket(np);
}


void FileManagerPlugin::runCommand(const QString& cmd, const QString& wd) {
  QProcess* proc = commandProcess(cmd);
  proc->setWorkingDirectory(wd);
  proc->start();
  proc->waitForFinished(-1);
  Q_EMIT errorNeedsSending(QString(QStringLiteral("exit code %1")).arg(proc->exitCode()));
  Q_EMIT listingNeedsUpdate();
}


void FileManagerPlugin::runCommand(const QString& cmd) {
  runCommand(cmd, directory->absolutePath());
}

void FileManagerPlugin::makeDirectory(const QString& path) {
  bool success = directory->mkpath(path);
  if (success) {
    Q_EMIT updateListing();
  } else {
    Q_EMIT errorNeedsSending(QString(QStringLiteral("Could not make %1")).arg(path));
  }
}

QProcess* FileManagerPlugin::commandProcess(const QString& cmd) {
  qCInfo(KDECONNECT_PLUGIN_FILEMANAGER) << "Running:" << COMMAND << ARGS << cmd;
  QProcess* proc = new QProcess();
  proc->setProgram(QStringLiteral(COMMAND));
  proc->setArguments(QStringList() << QStringLiteral(ARGS) << cmd);
  return proc;
}


void FileManagerPlugin::shareFileForViewing(const QString& path, const QString& dest)
{
    NetworkPacket packet(PACKET_TYPE_FILEMANAGER);
    QSharedPointer<QFile> ioFile(new QFile(path));

    if (!ioFile->exists()) {
        Daemon::instance()->reportError(i18n("Could not share file"), i18n("%1 does not exist", path));
        return;
    } else {
        packet.setPayload(ioFile, ioFile->size());
        packet.set<QString>(QStringLiteral("filename"), dest);
        packet.set<bool>(QStringLiteral("open"), false);
    }

    sendPacket(packet);
}


void FileManagerPlugin::deleteFileForReplacement(const QString& path, FileTransferJob* job) {
  bool success = deleteFile(path);

  if (success) {
    Q_EMIT fileDeleted(path, job);
  }
}


void FileManagerPlugin::replaceFile(const QString& path, FileTransferJob* job) {
  connect(job, &KJob::result, this, [this](KJob* kjob) { finished(kjob); });
  job->start();
  // QUrl destination = QUrl::fromLocalFile(path);
  // if (np->hasPayload()) {
  //   FileTransferJob* job = np->createPayloadTransferJob(destination);
  //   connect(job, &KJob::result, this, [this](KJob* job) { finished(job); });
  //   job->start();
  //
  // } else {
  //     // create empty file
  //     QFile file(destination.toLocalFile());
  //     file.open(QIODevice::WriteOnly);
  //     file.close();
  // }
}


void FileManagerPlugin::finished(KJob* job) {
  FileTransferJob* ftjob = qobject_cast<FileTransferJob*>(job);
  if (ftjob && !job->error()) {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "File transfer finished." << ftjob->destination();
  } else {
      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "File transfer failed." << (ftjob ? ftjob->destination() : QUrl()) << job->errorText();
      sendMsgPacket(QString(QStringLiteral("upload failed for %1\nerror: %2")).arg(ftjob->destination().toString()).arg(job->errorText()));
  }
}



#include "filemanagerplugin.moc"
