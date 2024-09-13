#ifndef SETTINGPAGENETWORK_H
#define SETTINGPAGENETWORK_H

#include <QWidget>

#include "SettingPageBase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingPageNetwork; }
QT_END_NAMESPACE

class SettingPageNetwork : public QWidget, public SettingPageBase
{
    Q_OBJECT

public:
    SettingPageNetwork(QWidget *parent = nullptr);
    ~SettingPageNetwork();

public:
    virtual int OnLoad();
    virtual int OnSave();

private:
    Ui::SettingPageNetwork *ui;

};
#endif // SETTINGPAGENETWORK_H
