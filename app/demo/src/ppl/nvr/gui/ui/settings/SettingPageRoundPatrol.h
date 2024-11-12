#ifndef SETTINGPAGEROUNDPATROL_H
#define SETTINGPAGEROUNDPATROL_H

#include <QWidget>

#include "SettingPageBase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingPageRoundPatrol; }
QT_END_NAMESPACE

class SettingPageRoundPatrol : public QWidget, public SettingPageBase
{
    Q_OBJECT

public:
    SettingPageRoundPatrol(QWidget *parent = nullptr);
    ~SettingPageRoundPatrol();

public:
    virtual int OnLoad() override {return 0;};
    virtual int OnSave() override {return 0;};

private:
    Ui::SettingPageRoundPatrol *ui;

};
#endif // SETTINGPAGEROUNDPATROL_H
