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
    sendListing(QStringLiteral("d"));
  }
  return true;
}

void FileManagerPlugin::connected() {
  sendListing(QStringLiteral("/home/chosen"));
}

void FileManagerPlugin::sendListing(const QString& path) {
  NetworkPacket netpacket(PACKET_TYPE_FILEMANAGER);
  netpacket.set<QString>(QStringLiteral("directoryListing"), QStringLiteral("{\"/home/chosen/test\": {\"filename\": \"test\", \"fileinfo\": \"poop\"}, \"/home/chosen/test2\": {\"filename\": \"test2\", \"fileinfo\": \"drwxrwxr-x  2 chosen chosen 4096 Apr 27 01:05\"}}"));
  sendPacket(netpacket);
  qCDebug(KDECONNECT_PLUGIN_FILEMANAGER) << "sent test JSON packet";
}

#include "filemanagerplugin.moc"
