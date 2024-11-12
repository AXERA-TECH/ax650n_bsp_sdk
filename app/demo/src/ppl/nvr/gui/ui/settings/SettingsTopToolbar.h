#ifndef SETTINGSTOPTOOLBAR_H
#define SETTINGSTOPTOOLBAR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsTopToolbar; }
QT_END_NAMESPACE

class SettingsTopToolbar : public QWidget
{
    Q_OBJECT

public:
    SettingsTopToolbar(QWidget *parent = nullptr);
    ~SettingsTopToolbar();

private:
    Ui::SettingsTopToolbar *ui;
};
#endif // SETTINGSTOPTOOLBAR_H
