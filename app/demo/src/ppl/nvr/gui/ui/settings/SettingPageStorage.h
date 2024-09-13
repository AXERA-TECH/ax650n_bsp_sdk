#ifndef SETTINGPAGESTORAGE_H
#define SETTINGPAGESTORAGE_H

#include <QWidget>

#include "SettingPageBase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingPageStorage; }
QT_END_NAMESPACE

class SettingPageStorage : public QWidget, public SettingPageBase
{
    Q_OBJECT

public:
    SettingPageStorage(QWidget *parent = nullptr);
    ~SettingPageStorage();

public:
    virtual int OnLoad();
    virtual int OnSave();

private:
    Ui::SettingPageStorage *ui;

};
#endif // SETTINGPAGESTORAGE_H
