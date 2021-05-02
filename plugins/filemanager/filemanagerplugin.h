#ifndef FILEMANAGERPLUGIN_H
#define FILEMANAGERPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>
#include <QFileSystemWatcher>

#include <QString>
#include <QDir>
#include <QMap>
#include <QList>
#include <QFile>
#include <QFileInfo>
#include <QtGlobal>
#include <QFileDevice>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariant>
#include <QVariantList>
#include <QUrl>
#include <QDBusMessage>
#include <QRandomGenerator>
#include <QFuture>
#include <QtConcurrent>

#include <quazip.h>
#include <quazipfile.h>

class Q_DECL_EXPORT FileManagerPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

QRandomGenerator* random = new QRandomGenerator();
QString possibleChars = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
QDir* directory = new QDir(QDir::homePath());
QMap<QString, QString> zippedFiles;

public:
    explicit FileManagerPlugin(QObject* parent, const QVariantList& args);
    ~FileManagerPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

    QString dbusPath() const override;

private:
    void sendListing(){ sendListing(directory->cleanPath(directory->absolutePath())); }
    void sendListing(const QString& path);
    QString permissionsString(QFileDevice::Permissions permissions);
    void sendFile(const QString& path);
    bool _archive(const QString& outPath, const QDir& dir, const QString& comment = QString(QStringLiteral("")));
    void sendArchive(const QString& directoryPath);
    void sendErrorPacket(const QString& errorMsg);
    QList<QString> recurseAddDir(const QDir& dir);
    QString genRandomString(const qint16 maxLength = 8);
};

#endif
