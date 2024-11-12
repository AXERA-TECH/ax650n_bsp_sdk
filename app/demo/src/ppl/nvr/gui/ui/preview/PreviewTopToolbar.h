#ifndef PREVIEWTOPTOOLBAR_H
#define PREVIEWTOPTOOLBAR_H

#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QThread>

#include "ax_base_type.h"

#include "global/UiGlobalDef.h"


QT_BEGIN_NAMESPACE
namespace Ui { class PreviewTopToolbar; }
QT_END_NAMESPACE

class PreviewTopToolbar : public QWidget
{
    Q_OBJECT

public:
    PreviewTopToolbar(QWidget *parent = nullptr);
    ~PreviewTopToolbar();
    void CheckPip(bool bChecked);
    void SetCurSplit(SPLIT_TYPE type);
    void SetPrevEnabled(bool bEnabled);
    void SetNextEnabled(bool bEnabled);
    void EnableWidgets(bool bEnabled);

protected:
    virtual void hideEvent(QHideEvent *event) override;

private slots:
    void OnChangeSplit();
    void OnChangePrevNext();
    void OnChangeMainSub1();
    void OnEnablePip();
    // void OnStartPolling();

public:
    Ui::PreviewTopToolbar *ui;

signals:
    void signal_change_split(SPLIT_TYPE enSpliteType);
    void signal_change_mainsub1(void);
    void signal_change_prev_next(PREV_NEXT_TYPE enPrevNextType);
    void signal_enable_pip(bool bPip);

    // void signal_enable_polling(bool bPolling);

};
#endif // PREVIEWTOPTOOLBAR_H
