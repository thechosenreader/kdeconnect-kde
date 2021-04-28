#ifndef FILEMANAGERPLUGIN_H
#define FILEMANAGERPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>
#include <QFileSystemWatcher>

#include <QString>
#include <QDir>
#include <QList>
#include <QFileInfo>
#include <QtGlobal>
#include <QFileDevice>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

class Q_DECL_EXPORT FileManagerPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit FileManagerPlugin(QObject* parent, const QVariantList& args);
    ~FileManagerPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

    QString dbusPath() const override;

private:
    void sendListing();
    void sendListing(const QString& path);
    QString permissionsString(QFileDevice::Permissions permissions);
};

#endif
