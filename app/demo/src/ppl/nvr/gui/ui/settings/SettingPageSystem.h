#ifndef SETTINGPAGESYSTEM_H
#define SETTINGPAGESYSTEM_H

#include <QWidget>

#include "SettingPageBase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingPageSystem; }
QT_END_NAMESPACE

class SettingPageSystem : public QWidget, public SettingPageBase
{
    Q_OBJECT

public:
    SettingPageSystem(QWidget *parent = nullptr);
    ~SettingPageSystem();

public:
    virtual int OnLoad();
    virtual int OnSave();

private:
    Ui::SettingPageSystem *ui;

};
#endif // SETTINGPAGESYSTEM_H
