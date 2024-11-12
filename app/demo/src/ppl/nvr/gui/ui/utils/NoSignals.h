#pragma once
#include <QWidget>
#include <QPushButton>
#include <QString>

class NoSignals: public QObject {
    Q_OBJECT
public:
    static void setChecked(QPushButton* widget, bool isChecked) {
        widget->blockSignals(true);
        widget->setChecked(isChecked);
        widget->blockSignals(false);
    }
};
