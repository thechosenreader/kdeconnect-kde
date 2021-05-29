#ifndef KDECONNECT_FILEMANAGER_CONFIG_H
#define KDECONNECT_FILEMANAGER_CONFIG_H

#include "kcmplugin/kdeconnectpluginkcm.h"
#include <QLineEdit>

class QMenu;
class QStandardItemModel;

class FileManagerConfig
    : public KdeConnectPluginKcm
{
    Q_OBJECT

public:
    FileManagerConfig(QWidget* parent, const QVariantList&);
    ~FileManagerConfig() override;

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

// private Q_SLOTS:
    // void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    // void onTextChanged(const QString& text);

private:
    QLineEdit* commandEdit;
    QLineEdit* argsEdit;

};

#endif // KDECONNECT_FILEMANAGER_CONFIG_H
