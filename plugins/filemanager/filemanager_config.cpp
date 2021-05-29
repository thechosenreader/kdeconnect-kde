#include "filemanager_config.h"

#include <QLabel>
#include <QVBoxLayout>
// #include <QDebug>
#include <KPluginFactory>

#ifdef Q_OS_WIN
#define COMMAND "cmd"
#define ARGS "/c"

#else
#define COMMAND "/bin/sh"
#define ARGS "-c"

#endif


K_PLUGIN_FACTORY(FileManagerConfigFactory, registerPlugin<FileManagerConfig>();)


FileManagerConfig::FileManagerConfig(QWidget* parent, const QVariantList& args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_filemanager_config"))
{

    QVBoxLayout* layout = new QVBoxLayout(this);

    commandEdit = new QLineEdit(this);
    QLabel* l1 = new QLabel(QStringLiteral("Shell program (default: %1)").arg(QStringLiteral(COMMAND)));
    layout->addWidget(l1);
    layout->addWidget(commandEdit);

    argsEdit = new QLineEdit(this);
    QLabel* l2 = new QLabel(QStringLiteral("Options (default: %1)").arg(QStringLiteral(ARGS)));
    layout->addWidget(l2);
    layout->addWidget(argsEdit);

    setLayout(layout);

}

FileManagerConfig::~FileManagerConfig()
{
}


void FileManagerConfig::defaults()
{
    KCModule::defaults();

    commandEdit->setText(QStringLiteral(COMMAND));
    argsEdit->setText(QStringLiteral(ARGS));

    Q_EMIT changed(true);
}

void FileManagerConfig::load()
{
    KCModule::load();

    commandEdit->setText(config()->getString(QStringLiteral("shellExe"), QStringLiteral(COMMAND)));
    argsEdit->setText(config()->getString(QStringLiteral("shellExeArgs"), QStringLiteral(ARGS)));

    // qDebug() << "CHILLING" << "load()";
    connect(commandEdit, &QLineEdit::textEdited, this, [=](QString text) {save();});
    connect(argsEdit, &QLineEdit::textEdited, this, [=](QString text) {save();});

    Q_EMIT changed(false);
}

void FileManagerConfig::save()
{

    config()->set(QStringLiteral("shellExe"), commandEdit->text());
    config()->set(QStringLiteral("shellExeArgs"), argsEdit->text());
    // qDebug() << "CHILLING" << "setting shellExe and args to" << commandEdit->text() << argsEdit->text();

    KCModule::save();

    Q_EMIT changed(false);
}

#include "filemanager_config.moc"
