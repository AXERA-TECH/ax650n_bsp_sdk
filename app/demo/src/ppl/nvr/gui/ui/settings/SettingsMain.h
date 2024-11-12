#ifndef SETTINGSMAIN_H
#define SETTINGSMAIN_H

#include <QWidget>
#include <QShowEvent>

#include "SettingPageSystem.h"
#include "SettingPageNetwork.h"
#include "SettingPageDevList.h"
#include "SettingPageStorage.h"


QT_BEGIN_NAMESPACE
namespace Ui { class SettingsMain; }
QT_END_NAMESPACE

class SettingsMain : public QWidget
{
    Q_OBJECT

public:
    SettingsMain(QWidget *parent = nullptr);
    ~SettingsMain();

    virtual void showEvent(QShowEvent *) override;

public slots:
    void OnChangePage(int page_index);

private:
    Ui::SettingsMain *ui;

    int m_nCurPageIndex{-1};
    QVector<SettingPageBase*> m_vecPage;

    SettingPageSystem  *m_pSettingPageSystem;
    SettingPageDevList *m_pSettingPageDevList;
    SettingPageStorage *m_pSettingPageStorage;
};
#endif // SETTINGSMAIN_H
