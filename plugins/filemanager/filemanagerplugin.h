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
#include <QFlags>
#include <QSharedPointer>
#include <quazip.h>
#include <quazipfile.h>

#ifdef Q_OS_WIN
#define COMMAND "cmd"
#define ARGS "/c"

#else
#define COMMAND "/bin/sh"
#define ARGS "-c"

#endif

class Q_DECL_EXPORT FileManagerPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

bool showHidden = false;
QRandomGenerator* random = new QRandomGenerator();
const QString possibleChars = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
QDir* directory = new QDir(QDir::homePath());
QMap<QString, QString> zippedFiles;

public:
    explicit FileManagerPlugin(QObject* parent, const QVariantList& args);
    ~FileManagerPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

    QString dbusPath() const override;

private Q_SLOTS:
    void updateListing();
    void sendError(const QString& errorMsg);

Q_SIGNALS:
    void listingNeedsUpdate();
    void errorNeedsSending(const QString& errorMsg);

private:
    void sendListing(){ sendListing(directory->cleanPath(directory->absolutePath())); }
    void sendListing(const QString& path);
    QString permissionsString(QFileDevice::Permissions permissions);
    void sendFile(const QString& path);
    QList<QString> recurseAddDir(const QDir& dir);
    bool _archive(const QString& outPath, const QDir& dir);
    void sendArchive(const QString& directoryPath);
    bool _delete(const QString& path);
    void deleteFile(const QString& path);
    void rename(const QString& path, const QString& newname);
    void sendErrorPacket(const QString& errorMsg);
    QString genRandomString(const qint16 maxLength = 8);
    void runCommand(const QString& cmd);
    void runCommand(const QString& cmd, const QString& wd);
    void makeDirectory(const QString& path);
    QProcess* commandProcess(const QString& cmd);
    void shareFileForViewing(const QString& path, const QString& dest);
};

#endif
