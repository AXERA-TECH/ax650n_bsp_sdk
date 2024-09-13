#ifndef SETTINGSLEFTTOOLBAR_H
#define SETTINGSLEFTTOOLBAR_H

#include <QWidget>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QModelIndex>


QT_BEGIN_NAMESPACE
namespace Ui { class SettingsLeftToolbar; }
QT_END_NAMESPACE

class SettingsLeftToolbar : public QWidget
{
    Q_OBJECT

public:
    SettingsLeftToolbar(QWidget *parent = nullptr);
    ~SettingsLeftToolbar();

    // void InitPageList();

private:
    Ui::SettingsLeftToolbar *ui;
    // QStandardItemModel *m_ItemModel;


// private slots:
//     void OnPageIndexChange(int index);

signals:
    void signal_change_page(int page_index);
};
#endif // SETTINGSLEFTTOOLBAR_H
