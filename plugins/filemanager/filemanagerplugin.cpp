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
     qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "testing random string" << genRandomString();
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

    else
      sendListing();
  }

  else if (np.has(QStringLiteral("path"))) {
    QString filepath = np.get<QString>(QStringLiteral("path"));
    qDebug() << "got a possible download request for" << filepath;
    if (np.has(QStringLiteral("requestFileDownload")) && np.get<bool>(QStringLiteral("requestFileDownload"))) {
      sendFile(filepath);
    }

    else if (np.has(QStringLiteral("requestDirectoryDownload")) && np.get<bool>(QStringLiteral("requestDirectoryDownload"))) {
      qDebug() << "got a request for directory download" << filepath;
      sendArchive(filepath);
    }
  }

  return true;
}


void FileManagerPlugin::connected() {
  sendListing();
}


void FileManagerPlugin::sendListing(const QString& path) {

  QJsonObject* listing = new QJsonObject();
  directory->cd(path);
  directory->setPath(directory->absolutePath());
  qDebug() << "cwd is" << directory->absolutePath();

  QList<QFileInfo> files = directory->entryInfoList(QDir::NoDot | QDir::Hidden | QDir::AllEntries);

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
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "directoryPath=" << temp;

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

  qDebug() << "boutta call dbus share method on" << url.toString();
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
  QList<QString> entryList = dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

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

bool FileManagerPlugin::_archive(const QString& outPath, const QDir& dir, const QString& comment) {
  // initializing archive
  QuaZip zip(outPath);
  zip.setFileNameCodec("IBM866");

  // basic error checking, confirming permissions and that dir exists
  if (!zip.open(QuaZip::mdCreate)) {
      qDebug() << QString(QStringLiteral("testCreate(): zip.open(): %1")).arg(zip.getZipError());
      return false;
  }

  if (!dir.exists()) {
      qDebug() << QString(QStringLiteral("dir.exists(%1)=FALSE")).arg(dir.absolutePath());
      return false;
  }


  QList<QString> sourceList = recurseAddDir(dir);

  QList<QFileInfo> files;
  for (QList<QString>::iterator i = sourceList.begin(); i != sourceList.end(); i++){
    qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "adding file" << *i << "to list";
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
          qDebug() << QString(QStringLiteral("testCreate(): inFile.open(): ")) << inFile.errorString().toLocal8Bit().constData();
          return false;
      }

      if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileNameWithRelativePath, fileinfo.filePath()))) {
          qDebug() << QString(QStringLiteral("testCreate(): outFile.open(): %1")).arg(outFile.getZipError());
          return false;
      }

      qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "zipping" << fileinfo.filePath();
      while (inFile.getChar(&c) && outFile.putChar(c));

      if (outFile.getZipError() != UNZ_OK) {
          qDebug() << QString(QStringLiteral("testCreate(): outFile.putChar(): %1")).arg(outFile.getZipError());
          return false;
      }

      outFile.close();

      if (outFile.getZipError() != UNZ_OK) {
          qDebug() << QString(QStringLiteral("testCreate(): outFile.close(): %1")).arg(outFile.getZipError());
          return false;
      }

      inFile.close();
  }

  // add comment if needed
  if (!comment.isEmpty())
      zip.setComment(comment);

  zip.close();

  if (zip.getZipError() != 0) {
      qDebug() << QString(QStringLiteral("testCreate(): zip.close(): %1")).arg(zip.getZipError());
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
    if (zippedFiles.contains(directoryPath)) {
      QString zipLocation = zippedFiles.value(directoryPath);
      if (QFile::exists(zipLocation)) {
        sendFile(zipLocation);
        return;
      } else {
        zippedFiles.remove(directoryPath);
      }
    }

    QString newLocation = QString(QStringLiteral("%1/%2%3")).arg(QDir::tempPath()).arg(genRandomString()).arg(QStringLiteral(".zip"));
    zippedFiles.insert(directoryPath, newLocation);
    if(_archive(newLocation, QDir(directoryPath))) {
      sendFile(newLocation);
    }
    else {
      sendErrorPacket(QStringLiteral("Could not create archive!"));
    }
}

void FileManagerPlugin::sendErrorPacket(const QString& errorMsg) {
  NetworkPacket np(PACKET_TYPE_FILEMANAGER);
  np.set<QString>(QStringLiteral("Error"), errorMsg);
  sendPacket(np);
}


#include "filemanagerplugin.moc"
