#include "PlaybackMain.h"
#include "ui_PlaybackMain.h"
#include "AppLogApi.h"
#include <QDebug>
#include <QShowEvent>
#include <QTableWidget>
#include <unistd.h>
#include "AXNVRFramework.h"

#define TAG "PLAYBACK"

PlaybackMain::PlaybackMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackMain)
{
    ui->setupUi(this);
    ui->widget_playbackctrl->setStyleSheet(CSS_WIDGET_1);
    // ui->tableDevList->setAlternatingRowColors(true);
    // ui->tableDevList->setStyleSheet(CSS_TABLE);
    // ui->tableDevList->horizontalHeader()->setStyleSheet(CSS_TABLE_HEAD);

    m_pSplitWidgetMgr = new SplitWidgetMgr(AX_NVR_VIEW_TYPE::PLAYBACK, ui->gridLayout, this);
    m_pPlayBackTimeLine = new PlaybackTimeLine(this);
    m_pPlayBackCtrl = new PlaybackCtrl(this);
    ui->verticalLayout_3->addWidget(m_pPlayBackCtrl);
    ui->verticalLayout_3->addWidget(m_pPlayBackTimeLine);
    ui->verticalLayout_3->setStretch(1, 1);

    connect(&m_watcherPreview, &QFutureWatcher<AX_NVR_ACTION_RES_T>::finished, [=]{
        AX_NVR_ACTION_RES_T res = m_watcherPreview.result();
        if (res.enPlaybackActionType == PLAYBACK_ACTION_TYPE::STOP) {
            SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
            const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
            m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::STOP, vecViewChn);

            emit signal_stop_status_changed(0); /* stop finished */
        } else if (res.enPlaybackActionType == PLAYBACK_ACTION_TYPE::FAST_SPEED) {
            AX_NVR_ACTION_RES_T res = m_watcherPreview.result();
            if (AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK == res.enResult) {
                SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
                ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);

                if (m_fCurrentSpeed > 1.0) {
                    m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::FAST_SPEED, vecViewChn, QString("(Speed X%1)").arg(m_nSpeedFactor[m_nCurrentIndex]));
                } else {
                    m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::FAST_SPEED, vecViewChn);
                }
            }
        } else if (res.enPlaybackActionType == PLAYBACK_ACTION_TYPE::SLOW_SPEED) {
            AX_NVR_ACTION_RES_T res = m_watcherPreview.result();
            if (AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK == res.enResult) {
                SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
                ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);

                if (m_fCurrentSpeed < 1.0) {
                    m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::SLOW_SPEED, vecViewChn, QString("(Speed 1/%1)").arg(m_nSpeedFactor[m_nCurrentIndex]));
                } else {
                    m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::SLOW_SPEED, vecViewChn);
                }
            }
        }
        LOG_M_D(TAG, "[%s][%d] Preview Action Finished, res=0x%x.", __func__, __LINE__, res.enResult);
    });

    connect(&m_watcherPreview, &QFutureWatcher<AX_NVR_ACTION_RES_T>::started, [=]{
        LOG_M_D(TAG, "[%s][%d] Preview Action Started.", __func__, __LINE__);
    });

    connect(m_pPlayBackCtrl, &PlaybackCtrl::signal_playback_action, this, &PlaybackMain::OnDoPlaybackAction);
}

PlaybackMain::~PlaybackMain()
{
    delete ui;
    DEL_PTR(m_pSplitWidgetMgr)
    DEL_PTR(m_pPlayBackCtrl)
    DEL_PTR(m_pPlayBackTimeLine)
}

void PlaybackMain::SetCtrls(PlaybackLeftToolbar *pPlaybackLeftToolbar) {
    m_pPlaybackLeftToolbar = pPlaybackLeftToolbar;
}

void PlaybackMain::resizeEvent(QResizeEvent *event) {

    auto __INIT_FUNC__ = [this]() {
        QPoint pt = ui->widget_playback->mapToGlobal(QPoint(0, 0));
        m_nLeftMargin   = pt.x()%2;
        m_nTopMargin    = pt.y()%2;
        m_nRightMargin  = 0;
        m_nBottomMargin = 0;
        m_nWidth        = ui->widget_playback->geometry().width() - m_nLeftMargin - m_nRightMargin;
        m_nHeight       = ui->widget_playback->geometry().height() - m_nTopMargin - m_nBottomMargin;
    };

    __INIT_FUNC__();

    m_pSplitWidgetMgr->ChangeSplitWidgets(m_pSplitWidgetMgr->GetCurrentSplitType());

    return QWidget::resizeEvent(event);
}

void PlaybackMain::showEvent(QShowEvent *event) {

    do {
        m_pScaleLabelSeled = nullptr;
        if (m_watcherPreview.isRunning()) {
            LOG_MM_W(TAG, "Last operation still active.");
            break;
        }

        SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();

        // change view split layout
        m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::SHOW;

        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);
        this->update_playctrl(enSplitType);

        m_pScaleLabelSeled = (ScaleLabel*)m_pSplitWidgetMgr->GetCurrentStartWidget();
    } while(0);

    return QWidget::showEvent(event);
}

void PlaybackMain::hideEvent(QHideEvent *event) {
    do {

        if (m_watcherPreview.isRunning()) {
            LOG_MM_W(TAG, "Last operation still active.");
            break;
        }

        emit signal_stop_status_changed(1); /* stop started */

        SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            emit signal_stop_status_changed(0); /* stop finished */
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            emit signal_stop_status_changed(0); /* stop finished */
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::HIDE;
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);

    } while(0);
    return QWidget::hideEvent(event);
}

bool PlaybackMain::eventFilter(QObject *watched, QEvent *event) {
    AX_S32 nEventType = 0; /* Not concerned */
    do {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *ev = static_cast<QMouseEvent *>(event);
            if (ev->button() == Qt::LeftButton) {
                m_pScaleLabelSeled = (ScaleLabel*)watched;
                break;
            }
        }

        if (event->type() != QEvent::MouseButtonDblClick) {
            break;
        }

        QMouseEvent *ev = static_cast<QMouseEvent *>(event);
        if (ev->button() != Qt::LeftButton) {
            break;
        }

        nEventType = 1; /* Double click event */

        const SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
        if (enSplitType == SPLIT_TYPE::ONE) {
            LOG_MM_W(TAG, "SPLIT_TYPE::ONE actived, ignore MAX/MIN operation.");
            break;
        }

        if (m_watcherPreview.isRunning()) {
            LOG_MM_W(TAG, "Last operation still active.");
            break;
        }

        // get current selected label index
        int nChnSeled = m_pSplitWidgetMgr->GetWidgetIndex((ScaleLabel*)watched);
        if (nChnSeled == -1) {
            LOG_MM_E(TAG, "No channel selected.");
            break;
        }

        // min max selected label
        m_pSplitWidgetMgr->MinMaxSelected(nChnSeled);

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect;
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::MIN;
        SPLIT_TYPE enSplitTypeForPlayCtrl = enSplitType;
        if (vecViewChn.size() == 1) {
            enChangeType = AX_NVR_VIEW_CHANGE_TYPE::MAX;
            enSplitTypeForPlayCtrl = SPLIT_TYPE::MAX;
            vecVoRect = m_pSplitWidgetMgr->CalcLayouts(SPLIT_TYPE::MAX,
                                                    m_nWidth, m_nHeight,
                                                    m_nLeftMargin, m_nTopMargin,
                                                    m_nRightMargin, m_nBottomMargin);
        } else {
            vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                    m_nWidth, m_nHeight,
                                                    m_nLeftMargin, m_nTopMargin,
                                                    m_nRightMargin, m_nBottomMargin);
        }

        if (vecVoRect.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);
        this->update_playctrl(enSplitTypeForPlayCtrl);
        nEventType = 2; /* Double click actually processed */

    } while(0);

    if (1 == nEventType) { /* Double clicked not processed should still give feedback to testsuite */
        AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
        if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
            AX_NVR_ACTION_RES_T res;
            res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
            emit signal_result_feedback(res);
        }
    }

    return QWidget::eventFilter(watched, event);
}

void PlaybackMain::OnChangeSplitVideo(SPLIT_TYPE enSplitType) {
    do {
        SPLIT_TYPE enCurrSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
        if (enSplitType == enCurrSplitType) {
            LOG_M_D(TAG, "[%s][%d] split_type=%d not changed", __func__, __LINE__, enSplitType);
            break;
        }

        if (m_watcherPreview.isRunning()) {
            LOG_MM_W(TAG, "Last operation still active.");
            break;
        }

        // change view split layout
        m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::UPDATE;

        // change split layout, 1<-->4<-->8<-->16
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);
        this->update_playctrl(enSplitType);

        m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::STOP, vecViewChn);

        m_pScaleLabelSeled = (ScaleLabel*)m_pSplitWidgetMgr->GetCurrentStartWidget();

    } while(0);
}

void PlaybackMain::OnChangeMainSub1(void) {
    do {
        if (m_watcherPreview.isRunning()) {
            LOG_MM_W(TAG, "Last operation still active.");
            break;
        }

        // get current label selected
        int nIndexSeled = m_pSplitWidgetMgr->GetWidgetIndex(m_pScaleLabelSeled);
        int nDevidSeled = m_pSplitWidgetMgr->GetWidgetDevid(m_pScaleLabelSeled);
        ax_nvr_channel_vector vecViewChn;
        vecViewChn.emplace_back(make_tuple(nIndexSeled, nDevidSeled));

        const SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_D(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // Quit zoom mode
        m_pScaleLabelSeled->Reset();

        AX_U32 nYYYYMMDD = m_pPlaybackLeftToolbar->GetCurrentIntYMD();
        AX_U32 nHHMMSS = m_pPlayBackTimeLine->GetCurrentHMS();

        PLAYBACK_ACTION_TYPE enActionType = PLAYBACK_ACTION_TYPE::MAINSUB1;

        AX_NVR_ACTION_RES_T res;
        res.enSplitType = enSplitType;
        QFutureInterface<AX_NVR_ACTION_RES_T> interface;
        interface.reportStarted();
        m_watcherPreview.setFuture(interface.future());

        std::thread thread([this, interface, vecViewChn, enActionType, nYYYYMMDD, nHHMMSS, res]() mutable {
            LOG_MM_W(TAG, "Thread Action %d +++ ", enActionType);
            pthread_setname_np(pthread_self(), "AppPBChangeMS");

            res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_ERR;
            res.enPlaybackActionType = enActionType;

            do {
                CAXNVRPlaybakCtrl *pPlaybak = CAXNVRFramework::GetInstance()->PlaybakCtrl();
                if (!pPlaybak) {
                    LOG_MM_E(TAG, "invalid point");
                    break;
                }

                if (!pPlaybak->SwitchMainSub1(vecViewChn, nYYYYMMDD, nHHMMSS)) {
                    LOG_MM_E(TAG, "Playback Action=%d failed.", enActionType);
                    break;
                }

                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
            } while (0);

            interface.reportResult(res, 0);
            interface.reportFinished();

            AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
            if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
                emit signal_result_feedback(res);
            }

            LOG_MM_W(TAG, "Thread Action %d --- ", enActionType);
        });

        // detach thred, Use Future to ensure exit
        thread.detach();

    } while (0);
}

void PlaybackMain::OnDoPlaybackActionUpdate(PLAYBACK_ACTION_TYPE enActionType) {
    LOG_M_D(TAG, "[%s][%d] ActionType=%d +++", __func__, __LINE__, enActionType);
    if (m_pPlayBackTimeLine != nullptr) {
        ax_nvr_channel_vector vecViewChn = m_pPlaybackLeftToolbar->GetChannelsSelected();
        m_pPlayBackTimeLine->UpdateTimeline(vecViewChn, m_pPlaybackLeftToolbar->GetCurrentYMD());
    }
    LOG_M_D(TAG, "[%s][%d] ActionType=%d ---", __func__, __LINE__, enActionType);
}

void PlaybackMain::OnDoPlaybackAction(PLAYBACK_ACTION_TYPE enActionType) {
    LOG_MM_I(TAG, "ActionType=%d +++", enActionType);

    AX_BOOL bCurrOprStarted = AX_FALSE;
    AX_NVR_ACTION_RES_T res;
    res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_ERR;
    res.enPlaybackActionType = enActionType;
    QFutureInterface<AX_NVR_ACTION_RES_T> interface;

    do {
        if (m_watcherPreview.isRunning()) {
            LOG_MM_W(TAG, "Last operation still active.");
            break;
        }

        interface.reportStarted();
        m_watcherPreview.setFuture(interface.future());
        bCurrOprStarted = AX_TRUE;

        // set label dev-id after started
        if (PLAYBACK_ACTION_TYPE::PLAY == enActionType || PLAYBACK_ACTION_TYPE::REVERSE == enActionType) {
            SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
            ax_nvr_channel_vector vecViewChnPlayed = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
            ax_nvr_channel_vector vecViewChnSelected = m_pPlaybackLeftToolbar->GetChannelsSelected();
            if (m_vecViewChnSelected.size() > 0 && (vecViewChnSelected != m_vecViewChnSelected || m_bPlaying)) {
                AX_BOOL bIgnoreOpr = AX_FALSE;
                for (auto &m : vecViewChnPlayed) {
                    if (std::get<1>(m) != -1) {
                        LOG_MM_W(TAG, "Playback is running, please STOP first.");
                        res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
                        bIgnoreOpr = AX_TRUE;
                        break;
                    }
                }

                if (bIgnoreOpr) {
                    break;
                }
            }

            ax_nvr_channel_vector vecSelectedChn = m_pPlaybackLeftToolbar->GetChannelsSelected();
            m_pSplitWidgetMgr->SetChannelsText(PLAYBACK_ACTION_TYPE::PLAY, vecSelectedChn);
            m_vecViewChnSelected = vecSelectedChn;

            /* Reset speed */
            const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
            m_nCurrentIndex = 0;
            m_fCurrentSpeed = 1.0f;
            CAXNVRPlaybakCtrl *pPlaybak = CAXNVRFramework::GetInstance()->PlaybakCtrl();
            pPlaybak->SetSpeed(vecViewChn, m_fCurrentSpeed);
        }
        else if (PLAYBACK_ACTION_TYPE::STOP == enActionType) {
            m_pSplitWidgetMgr->Reset();
        }
        else if (enActionType == PLAYBACK_ACTION_TYPE::NEXT) {
            m_pPlaybackLeftToolbar->SetCurrentYMD(true);
        } else if (enActionType == PLAYBACK_ACTION_TYPE::PREV) {
            m_pPlaybackLeftToolbar->SetCurrentYMD(false);
        }

        const SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_C(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        AX_U32 nYYYYMMDD = m_pPlaybackLeftToolbar->GetCurrentIntYMD();
        AX_U32 nHHMMSS = m_pPlayBackTimeLine->GetCurrentHMS();
        std::thread thread([this, interface, vecViewChn, enActionType, nYYYYMMDD, nHHMMSS, res]() mutable {
            pthread_setname_np(pthread_self(), "AppPBAction");
            LOG_MM_W(TAG, "Thread Action %d +++ ", enActionType);

            do {
                CAXNVRPlaybakCtrl *pPlaybak = CAXNVRFramework::GetInstance()->PlaybakCtrl();
                if (!pPlaybak) {
                    LOG_M_E(TAG, "invalid point");
                    break;
                }

                AX_BOOL bRet = AX_FALSE;
                switch (enActionType) {
                    case PLAYBACK_ACTION_TYPE::PLAY:
                        bRet = pPlaybak->Start(vecViewChn, nYYYYMMDD, nHHMMSS, AX_FALSE);
                        m_bPlaying = AX_TRUE;
                        break;
                    case PLAYBACK_ACTION_TYPE::REVERSE:
                        bRet = pPlaybak->Start(vecViewChn, nYYYYMMDD, nHHMMSS, AX_TRUE);
                        m_bPlaying = AX_TRUE;
                        break;
                    case PLAYBACK_ACTION_TYPE::STOP:
                        bRet = pPlaybak->Stop(vecViewChn);
                        m_bPlaying = AX_FALSE;
                        break;
                    case PLAYBACK_ACTION_TYPE::PAUSE:
                        bRet = pPlaybak->PauseResume(vecViewChn);
                        break;
                    case PLAYBACK_ACTION_TYPE::SLOW_SPEED: {
                        int nDevidSeled = m_pSplitWidgetMgr->GetWidgetDevid(m_pScaleLabelSeled);
                        if (nDevidSeled == -1) {
                            LOG_MM_W(TAG, "Playback not started, please start first.");
                            bRet = AX_TRUE;
                            break;
                        }

                        if (m_fCurrentSpeed > 1.0) {
                            m_nCurrentIndex = 0;
                        } else {
                            m_nCurrentIndex ++;
                            if (m_nCurrentIndex > m_nMaxIndex) {
                                m_nCurrentIndex = 0;
                            }
                        }
                        m_fCurrentSpeed = 1.0 / m_nSpeedFactor[m_nCurrentIndex];
                        bRet = pPlaybak->SetSpeed(vecViewChn, m_fCurrentSpeed);
                        break;
                    }
                    case PLAYBACK_ACTION_TYPE::FAST_SPEED: {
                        int nDevidSeled = m_pSplitWidgetMgr->GetWidgetDevid(m_pScaleLabelSeled);
                        if (nDevidSeled == -1) {
                            LOG_MM_W(TAG, "Playback not started, please start first.");
                            bRet = AX_TRUE;
                            break;
                        }

                        if (m_fCurrentSpeed < 1.0) {
                            m_nCurrentIndex = 0;
                        } else {
                            m_nCurrentIndex ++;
                            if (m_nCurrentIndex > m_nMaxIndex) {
                                m_nCurrentIndex = 0;
                            }
                        }
                        m_fCurrentSpeed = m_nSpeedFactor[m_nCurrentIndex] * 1.0;
                        bRet = pPlaybak->SetSpeed(vecViewChn, m_fCurrentSpeed);
                        break;
                    }
                    case PLAYBACK_ACTION_TYPE::PREV_FRAME:
                        bRet = pPlaybak->Step(vecViewChn, AX_TRUE);
                        break;
                    case PLAYBACK_ACTION_TYPE::NEXT_FRAME:
                        bRet = pPlaybak->Step(vecViewChn, AX_FALSE);
                        break;
                    case PLAYBACK_ACTION_TYPE::MAINSUB1:
                        bRet = pPlaybak->SwitchMainSub1(vecViewChn);
                        break;
                    default:
                        bRet = AX_TRUE;
                        break;
                }
                if (!bRet) {
                    LOG_MM_E(TAG, "Thread Action=%d failed.", enActionType);
                    break;
                }
                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
            } while (0);

            interface.reportResult(res, 0);
            interface.reportFinished();

            AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
            if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
                if (PLAYBACK_ACTION_TYPE::PLAY == enActionType
                    || PLAYBACK_ACTION_TYPE::REVERSE == enActionType
                    || PLAYBACK_ACTION_TYPE::STOP == enActionType
                    || PLAYBACK_ACTION_TYPE::PAUSE == enActionType
                    || PLAYBACK_ACTION_TYPE::SLOW_SPEED == enActionType
                    || PLAYBACK_ACTION_TYPE::FAST_SPEED == enActionType
                    || PLAYBACK_ACTION_TYPE::PREV_FRAME == enActionType
                    || PLAYBACK_ACTION_TYPE::NEXT_FRAME == enActionType
                    || PLAYBACK_ACTION_TYPE::MAINSUB1 == enActionType) {

                    emit signal_result_feedback(res);
                }
            }

            LOG_MM_W(TAG, "Thread Action %d ---", enActionType);
        });

        // detach thred, Use Future to ensure exit
        thread.detach();

        LOG_MM_I(TAG, "ActionType=%d ---", enActionType);
        return;
    } while (0);

    if (bCurrOprStarted) {
        interface.reportResult(res, 0);
        interface.reportFinished();
    }

    AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
    if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
        if (PLAYBACK_ACTION_TYPE::PLAY == enActionType
            || PLAYBACK_ACTION_TYPE::REVERSE == enActionType
            || PLAYBACK_ACTION_TYPE::STOP == enActionType
            || PLAYBACK_ACTION_TYPE::PAUSE == enActionType
            || PLAYBACK_ACTION_TYPE::SLOW_SPEED == enActionType
            || PLAYBACK_ACTION_TYPE::FAST_SPEED == enActionType
            || PLAYBACK_ACTION_TYPE::PREV_FRAME == enActionType
            || PLAYBACK_ACTION_TYPE::NEXT_FRAME == enActionType
            || PLAYBACK_ACTION_TYPE::MAINSUB1 == enActionType) {
            emit signal_result_feedback(res);
        }
    }

    LOG_MM_I(TAG, "ActionType=%d ---", enActionType);
}

void PlaybackMain::OnLocateTime(int nHHMM) {
    if (m_pPlayBackTimeLine) {
        m_pPlayBackTimeLine->Locate(nHHMM);
    }
}

void PlaybackMain::update_display(const ax_nvr_channel_vector &vecViewChn,
                                const vector<AX_NVR_RECT_T> &vecVoRect,
                                AX_NVR_VIEW_CHANGE_TYPE enChangeType,
                                SPLIT_TYPE enSplitType) {
    // Create thead to update VO channels and stop-start PPL(if record, RTSP not stop)
    AX_NVR_ACTION_RES_T res;
    res.enSplitType = enSplitType;

    QFutureInterface<AX_NVR_ACTION_RES_T> interface;
    interface.reportStarted();
    m_watcherPreview.setFuture(interface.future());

    std::thread thread([this, interface, vecViewChn, vecVoRect, enChangeType, res]() mutable {
        pthread_setname_np(pthread_self(), "AppPBUpdDisp");
        LOG_MM_W(TAG, "Thread Action %d +++ ", enChangeType);
        res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_ERR;
        do {
            AX_BOOL bRet = AX_FALSE;
            CAXNVRDisplayCtrl *pPrimaryDispCtrl = CAXNVRFramework::GetInstance()->PrimaryDispCtrl();
            CAXNVRPlaybakCtrl *pPlaybak = CAXNVRFramework::GetInstance()->PlaybakCtrl();
            if (pPrimaryDispCtrl == nullptr || pPlaybak == nullptr) {
                LOG_MM_E(TAG, "invalid point");
                break;
            }

            // do stop
            switch (enChangeType)
            {
            case AX_NVR_VIEW_CHANGE_TYPE::UPDATE:
            case AX_NVR_VIEW_CHANGE_TYPE::HIDE:
                res.enPlaybackActionType = PLAYBACK_ACTION_TYPE::STOP;
                pPlaybak->StopAll();
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::MIN:
            case AX_NVR_VIEW_CHANGE_TYPE::MAX:
            case AX_NVR_VIEW_CHANGE_TYPE::SHOW:
            default:
                break;
            }

            // Update VO attr
            if (!pPrimaryDispCtrl->UpdateView(vecViewChn, vecVoRect, AX_NVR_VIEW_TYPE::PLAYBACK, enChangeType)) {
                LOG_MM_E(TAG, "UpdateView changetype=%d", enChangeType);
                break;
            }

            // start PPL display
            switch (enChangeType)
            {
            case AX_NVR_VIEW_CHANGE_TYPE::MAX:
                bRet = pPlaybak->Update(vecViewChn);
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::MIN:
                if (!pPlaybak->PauseResume(vecViewChn, AX_TRUE)) {
                    bRet = AX_FALSE;
                    break;
                }
                m_nCurrentIndex = 0;
                m_fCurrentSpeed = 1.0f;
                if (!pPlaybak->SetSpeed(vecViewChn, m_fCurrentSpeed)) {
                    bRet = AX_FALSE;
                    break;
                }
                bRet = pPlaybak->Update(vecViewChn);
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::HIDE:
                /* Mark as STOP to enter SetChannelsText() when signal finished triggered */
                res.enPlaybackActionType = PLAYBACK_ACTION_TYPE::STOP;
                bRet = AX_TRUE;
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::SHOW:
            case AX_NVR_VIEW_CHANGE_TYPE::UPDATE:
            default:
                bRet = AX_TRUE;
                break;
            }

            if (!bRet) {
                LOG_MM_E(TAG, "Thread Action=%d failed.", enChangeType);
                break;
            }

            res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
        } while (0);

        interface.reportResult(res, 0);
        interface.reportFinished();

        AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
        if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
            emit signal_result_feedback(res);
        }

        LOG_MM_W(TAG, "Thread Action %d ---", enChangeType);
    });

    // detach thread, Use Future to ensure exit
    thread.detach();
}

void PlaybackMain::update_playctrl(SPLIT_TYPE enChange) {
    if (SPLIT_TYPE::ONE == enChange || SPLIT_TYPE::MAX == enChange) {
        m_pPlayBackCtrl->EnableSingleLabelCtrl(true);
    } else {
        m_pPlayBackCtrl->EnableSingleLabelCtrl(false);
    }
}