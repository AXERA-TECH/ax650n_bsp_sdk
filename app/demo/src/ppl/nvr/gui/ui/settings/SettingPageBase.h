#ifndef SETTINGPAGEBASE_H
#define SETTINGPAGEBASE_H


class SettingPageBase
{
public:
    virtual int OnLoad() = 0;
    virtual int OnSave() = 0;
};
#endif // SETTINGPAGEBASE_H
