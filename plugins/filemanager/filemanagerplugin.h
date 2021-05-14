#ifndef FILEMANAGERPLUGIN_H
#define FILEMANAGERPLUGIN_H

#include <QObject>
#include <KJob>

#include "core/filetransferjob.h"
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



public:
    explicit FileManagerPlugin(QObject* parent, const QVariantList& args);
    ~FileManagerPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

    QString dbusPath() const override;

private Q_SLOTS:
    void updateListing();
    void sendError(const QString& errorMsg);
    void replaceFile(const QString& filename, FileTransferJob* job);

Q_SIGNALS:
    void fileDeleted(const QString& filename, FileTransferJob* job);
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
    bool deleteFile(const QString& path);
    void deleteFileForReplacement(const QString& path, FileTransferJob* job);
    void rename(const QString& path, const QString& newname);
    void sendMsgPacket(const QString& msg);
    QString genRandomString(const qint16 maxLength = 8);
    void runCommand(const QString& cmd);
    void runCommand(const QString& cmd, const QString& wd);
    void makeDirectory(const QString& path);
    QProcess* commandProcess(const QString& cmd);
    void shareFileForViewing(const QString& path, const QString& dest);
    void finished(KJob* job);

    bool showHidden = false;
    bool isConnected = false;
    QRandomGenerator* random = new QRandomGenerator();
    const QString possibleChars = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    QDir* directory = new QDir(QDir::homePath());
    QMap<QString, QString> zippedFiles;

};

#endif
