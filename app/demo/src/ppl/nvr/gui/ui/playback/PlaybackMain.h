#ifndef PLAYBACKMAIN_H
#define PLAYBACKMAIN_H

#include <QWidget>
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QThread>

#include "global/UiGlobalDef.h"
#include "utils/SplitWidgetMgr.h"
#include "PlaybackCtrl.h"
#include "PlaybackTimeLine.h"
#include "PlaybackLeftToolbar.h"


QT_BEGIN_NAMESPACE
namespace Ui { class PlaybackMain; }
QT_END_NAMESPACE


class PlaybackMain : public QWidget
{
    Q_OBJECT

public:
    PlaybackMain(QWidget *parent = nullptr);
    ~PlaybackMain();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::PlaybackMain *ui;

public slots:
    void OnChangeSplitVideo(SPLIT_TYPE enSplitType);
    void OnChangeMainSub1(void);
    void OnDoPlaybackAction(PLAYBACK_ACTION_TYPE enActionType);
    void OnDoPlaybackActionUpdate(PLAYBACK_ACTION_TYPE enActionType);
    void OnLocateTime(int nHHMM);

public:
    void SetCtrls(PlaybackLeftToolbar *pPlaybackLeftToolbar);

signals:
    void signal_result_feedback(const AX_NVR_ACTION_RES_T& enActionType);
    void signal_stop_status_changed(int nStatus);

public:
    SplitWidgetMgr *m_pSplitWidgetMgr = nullptr;

private:
    QFutureWatcher<AX_NVR_ACTION_RES_T> m_watcherPreview;
    ScaleLabel* m_pScaleLabelSeled = nullptr;

    PlaybackCtrl *m_pPlayBackCtrl = nullptr;
    PlaybackTimeLine *m_pPlayBackTimeLine = nullptr;
    PlaybackLeftToolbar *m_pPlaybackLeftToolbar = nullptr;

    ax_nvr_channel_vector m_vecViewChnSelected;
    AX_BOOL m_bPlaying {AX_FALSE};

    int m_nLeftMargin = 0;
    int m_nTopMargin = 0;
    int m_nRightMargin = 0;
    int m_nBottomMargin = 0;
    int m_nWidth = 0;
    int m_nHeight = 0;
    int m_nXOffset = 0;
    int m_nYOffset = 0;

    AX_U32 m_nMaxIndex = 4;
    AX_U32 m_nSpeedFactor[5] = {1, 2, 4, 8, 16};
    AX_U32 m_nCurrentIndex = 0;
    AX_F32 m_fCurrentSpeed = 1.0f;
private:
    void update_display(const ax_nvr_channel_vector &vecViewChn, const vector<AX_NVR_RECT_T> &vecVoRect,
                    AX_NVR_VIEW_CHANGE_TYPE enChange, SPLIT_TYPE enSplitType);

    void update_playctrl(SPLIT_TYPE enSplitType);
};
#endif // PLAYBACKMAIN_H
