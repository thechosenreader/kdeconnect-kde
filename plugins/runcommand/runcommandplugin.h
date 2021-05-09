/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RUNCOMMANDPLUGIN_H
#define RUNCOMMANDPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMap>
#include <QPair>
#include <QString>

#include <QDBusConnection>
#include <QProcess>
#include <QDir>
#include <QSettings>
#include <QJsonDocument>

class Q_DECL_EXPORT RunCommandPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit RunCommandPlugin(QObject* parent, const QVariantList& args);
    ~RunCommandPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

    static QProcess* commandProcess(const QString& cmd);

private Q_SLOTS:
    void configChanged();

private:
    void sendConfig();

};

#endif
