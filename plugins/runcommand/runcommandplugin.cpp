/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "runcommandplugin.h"

#include <KPluginFactory>

#ifdef SAILFISHOS
#define KCMUTILS_VERSION 0
#else
#include <KShell>
#include <kcmutils_version.h>
#endif

#include <core/networkpacket.h>
#include <core/device.h>
#include <core/daemon.h>

#include "plugin_runcommand_debug.h"

#define PACKET_TYPE_RUNCOMMAND QStringLiteral("kdeconnect.runcommand")

// these are used as defaults when fetching from config
#ifdef Q_OS_WIN
#define _COMMAND "cmd"
#define _ARGS "/c"

#else
#define _COMMAND "/bin/sh"
#define _ARGS "-c"

#endif

K_PLUGIN_CLASS_WITH_JSON(RunCommandPlugin, "kdeconnect_runcommand.json")

RunCommandPlugin::RunCommandPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    qCDebug(KDECONNECT_PLUGIN_RUNCOMMAND) << "RUNCOMMAND plugin constructor for device" << device()->name();
    connect(config(), &KdeConnectPluginConfig::configChanged, this, &RunCommandPlugin::configChanged);
    updateCMDandARGS();
}

RunCommandPlugin::~RunCommandPlugin()
{
}

bool RunCommandPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("requestCommandList"), false)) {
        sendConfig();
        return true;
    }

    if (np.has(QStringLiteral("key"))) {
        QJsonDocument commandsDocument = QJsonDocument::fromJson(config()->getByteArray(QStringLiteral("commands"), "{}"));
        QJsonObject commands = commandsDocument.object();
        QString key = np.get<QString>(QStringLiteral("key"));
        QJsonValue value = commands[key];

    if (value == QJsonValue::Undefined) {
        qCWarning(KDECONNECT_PLUGIN_RUNCOMMAND) << key << "is not a configured command";
    }

	else if (np.has(QStringLiteral("rawCommand"))) {
		QString rawCommand = np.get<QString>(QStringLiteral("rawCommand"));
    commandProcess(rawCommand)->startDetached();
	}

	else {
		const QJsonObject commandJson = value.toObject();
    commandProcess(commandJson[QStringLiteral("command")].toString())->startDetached();
		return true;
	     }
    } else if (np.has(QStringLiteral("setup"))) {
        Daemon::instance()->openConfiguration(device()->id(), QStringLiteral("kdeconnect_runcommand"));
    }

    return false;
}

void RunCommandPlugin::connected()
{

    sendConfig();
}

void RunCommandPlugin::sendConfig()
{
    QString commands = config()->getString(QStringLiteral("commands"),QStringLiteral("{}"));
    NetworkPacket np(PACKET_TYPE_RUNCOMMAND, {{QStringLiteral("commandList"), commands}});

    #if KCMUTILS_VERSION >= QT_VERSION_CHECK(5, 45, 0)
        np.set<bool>(QStringLiteral("canAddCommand"), true);
    #endif

    np.set<bool>(QStringLiteral("canAddCommand"), true);
    sendPacket(np);
}

void RunCommandPlugin::updateCMDandARGS() {

    COMMAND = config()->getString(QStringLiteral("shellExe"), QStringLiteral(_COMMAND));
    ARGS = config()->getString(QStringLiteral("shellExeArgs"), QStringLiteral(_ARGS));
}

void RunCommandPlugin::configChanged() {
    updateCMDandARGS();
    sendConfig();
}

QProcess* RunCommandPlugin::commandProcess(const QString& cmd) {
  qCInfo(KDECONNECT_PLUGIN_RUNCOMMAND) << "Running:" << COMMAND << ARGS << cmd;
  QProcess* proc = new QProcess();
  proc->setProgram(COMMAND);
  proc->setArguments(QStringList() << ARGS << cmd);
  return proc;
}

#include "runcommandplugin.moc"
