#include <filemanagerplugin.h>

#include <KPluginFactory>

#include <QDBusConnection>

#include <core/networkpacket.h>
#include <core/device.h>
#include <core/daemon.h>

#include "plugin_filemanager_debug.h"

#define PACKET_TYPE_FILEMANAGER QStringLiteral("kdeconnect.filemanager")

K_PLUGIN_CLASS_WITH_JSON(FileManagerPlugin, "kdeconnect_filemanager.json")



FileManagerPlugin::FileManagerPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
     qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "FILEMANAGER plugin constructor for device" << device()->name();
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
  return true;
}

void FileManagerPlugin::connected() {
  sendListing();
}

void FileManagerPlugin::sendListing() {
  sendListing(QDir::homePath());
}

void FileManagerPlugin::sendListing(const QString& path) {

  QJsonObject* listing = new QJsonObject();
  QDir directory(path);

  QList<QFileInfo> files = directory.entryInfoList(QDir::NoDot | QDir::Hidden | QDir::AllEntries);

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
  netpacket.set<QString>(QStringLiteral("directoryPath"), path);

  sendPacket(netpacket);

  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "constructed JSON=" << listingJSON;
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "sent JSON packet for " << path;

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

#include "filemanagerplugin.moc"
