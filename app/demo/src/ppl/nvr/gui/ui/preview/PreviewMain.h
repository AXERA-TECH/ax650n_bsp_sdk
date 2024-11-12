#ifndef PREVIEWMAIN_H
#define PREVIEWMAIN_H

#include <QWidget>
#include <QLabel>
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QThread>

#include "ax_base_type.h"
#include "global/UiGlobalDef.h"
#include "utils/SplitWidgetMgr.h"
#include "PreviewPip.h"
#include "PreviewTopToolbar.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PreviewMain; }
QT_END_NAMESPACE


class PreviewMain : public QWidget
{
    Q_OBJECT

public:
    PreviewMain(QWidget *parent = nullptr);
    ~PreviewMain();
    bool TestChangePrevNext(PREV_NEXT_TYPE enPrevNextType);

    PreviewTopToolbar *m_pPreviewTopToolbar = nullptr;
    SplitWidgetMgr *m_pSplitWidgetMgr = nullptr;

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    Ui::PreviewMain *ui;
    ScaleLabel* m_pScaleLabelSeled = nullptr;

public slots:
    void OnChangeSplitVideo(SPLIT_TYPE enSplitType);
    void OnChangePrevNext(PREV_NEXT_TYPE enPrevNextType);
    void OnChangeMainSub1(void);
    void OnEnablePip(bool bEnable);
    void OnPlaybackStopStatusChanged(int nStatus); /* 0: stopped; 1: stopping */

signals:
    void signal_result_feedback(const AX_NVR_ACTION_RES_T& enActionType);

private:
    PreviewPip *m_pPreviewPip = nullptr;
    QFutureWatcher<AX_NVR_ACTION_RES_T> m_watcherPreview;
    CAXEvent  m_hPlaybackStopEvent;

    int m_nLeftMargin = 0;
    int m_nTopMargin = 0;
    int m_nRightMargin = 0;
    int m_nBottomMargin = 0;
    int m_nWidth = 0;
    int m_nHeight = 0;
    int m_nXOffset = 0;
    int m_nYOffset = 0;

    bool m_bEnablePip = false;

private:
    void update_display(const ax_nvr_channel_vector &vecViewChn, const vector<AX_NVR_RECT_T> &vecVoRect,
                    AX_NVR_VIEW_CHANGE_TYPE enChange, SPLIT_TYPE enSplitType);

};
#endif // NVRMAINWINDOW_H