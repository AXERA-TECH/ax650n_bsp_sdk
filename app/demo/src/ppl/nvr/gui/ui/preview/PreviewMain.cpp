#include "PreviewMain.h"
#include "ui_PreviewMain.h"
#include <QDebug>
#include <QShowEvent>
#include <unistd.h>
#include "AppLogApi.h"
#include "AXNVRFramework.h"

#define TAG "PREVIEW"

PreviewMain::PreviewMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PreviewMain) {

    ui->setupUi(this);
    m_pSplitWidgetMgr = new SplitWidgetMgr(AX_NVR_VIEW_TYPE::PREVIEW, ui->gridLayout, this);
    m_pPreviewPip = new PreviewPip();
    m_hPlaybackStopEvent.SetEvent();

    connect(&m_watcherPreview, &QFutureWatcher<AX_NVR_ACTION_RES_T>::finished, [=]{
        AX_NVR_ACTION_RES_T res = m_watcherPreview.result();
        if (m_pPreviewTopToolbar) {
            /* Recover all grayed widgets */
            m_pPreviewTopToolbar->EnableWidgets(AX_TRUE);
            /* Update Prev/Next widgets status */
            m_pPreviewTopToolbar->SetPrevEnabled(TestChangePrevNext(PREV_NEXT_TYPE::PREV));
            m_pPreviewTopToolbar->SetNextEnabled(TestChangePrevNext(PREV_NEXT_TYPE::NEXT));
        }
        LOG_M_I(TAG, "[%s][%d] Preview Action Finished, res=0x%x.", __func__, __LINE__, res.enResult);
    });

    connect(&m_watcherPreview, &QFutureWatcher<AX_NVR_ACTION_RES_T>::started, [=]{
        if (m_pPreviewTopToolbar) {
            /* Grayed all top toolbar widgets and recover when finished signal emitted */
            m_pPreviewTopToolbar->EnableWidgets(AX_FALSE);
        }
        LOG_M_I(TAG, "[%s][%d] Preview Action Started.", __func__, __LINE__);
    });
}

PreviewMain::~PreviewMain() {
    delete ui;
    DEL_PTR(m_pSplitWidgetMgr)
    DEL_PTR(m_pPreviewPip)
}

void PreviewMain::resizeEvent(QResizeEvent *event) {
    auto __INIT_FUNC__ = [this]() {
        QPoint pt = this->mapToGlobal(QPoint(0, 0));
        m_nLeftMargin   = pt.x()%2;
        m_nTopMargin    = pt.y()%2;
        m_nRightMargin  = 0;
        m_nBottomMargin = 0;
        m_nWidth        = this->geometry().width() - m_nLeftMargin - m_nRightMargin;
        m_nHeight       = this->geometry().height() - m_nTopMargin - m_nBottomMargin;
    };

    __INIT_FUNC__();
    const SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
    m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

    return QWidget::resizeEvent(event);
}

void PreviewMain::showEvent(QShowEvent *event) {
    do {
        m_pScaleLabelSeled = nullptr;

        if (m_watcherPreview.isRunning()) {
            LOG_M_I(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
        if (m_pPreviewTopToolbar) {
            m_pPreviewTopToolbar->SetCurSplit(enSplitType);
        }

        // change view split layout
        m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::SHOW;
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);

        m_pScaleLabelSeled = (ScaleLabel*)m_pSplitWidgetMgr->GetCurrentStartWidget();
    } while (0);

    return QWidget::showEvent(event);
}

void PreviewMain::hideEvent(QHideEvent *event) {
    do {
        if (m_watcherPreview.isRunning()) {
            /* Ensure that stop process will be processed */
            m_watcherPreview.waitForFinished();
        }

        SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::HIDE;
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);

        if (m_pPreviewPip && m_pPreviewPip->isActiveWindow()) {

            AX_NVR_ACTION_RES_T res;
            res.enSplitType = SPLIT_TYPE::PIP;
            QFutureInterface<AX_NVR_ACTION_RES_T> interface;

            m_watcherPreview.setFuture(interface.future());
            interface.reportStarted();

            AX_BOOL bEnable = AX_FALSE;

            std::thread thread([this, interface, bEnable, res]() mutable {
                pthread_setname_np(pthread_self(), "AppPreviewHide");
                LOG_MM_W(TAG, "HideEvent Action +++");
                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_ERR;

                do {
                    AX_BOOL bRet = AX_FALSE;
                    CAXNVRDisplayCtrl *pPrimaryDispCtrl = CAXNVRFramework::GetInstance()->PrimaryDispCtrl();
                    CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                    if (pPrimaryDispCtrl == nullptr || pPreview == nullptr) {
                        LOG_M_E(TAG, "[%s][%d] invalid point", __func__, __LINE__);
                        break;
                    }

                    if (!bEnable) {

                        if (!pPreview->StopPip()) {
                            LOG_M_E(TAG, "[%s][%d] stop pip channel display failed.", __func__, __LINE__);
                            break;
                        }

                        if (!pPrimaryDispCtrl->DisablePip()) {
                            LOG_M_E(TAG, "[%s][%d] disable pip channel failed.", __func__, __LINE__);
                            break;
                        }
                    }

                } while(0);

                interface.reportResult(res, 0);
                interface.reportFinished();

                LOG_MM_W(TAG, "HideEvent Action ---");
            });

            // detach thred, Use Future to ensure exit
            thread.detach();

            m_pPreviewPip->close();
        }
    } while(0);

    return QWidget::hideEvent(event);
}

bool PreviewMain::eventFilter(QObject *watched, QEvent *event) {
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

        if (m_watcherPreview.isRunning()) {
            LOG_M_I(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        const SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
        if (enSplitType == SPLIT_TYPE::ONE) {
            break;
        }

        // get current selected label index
        int nChnSeled = m_pSplitWidgetMgr->GetWidgetIndex((ScaleLabel*)watched);
        LOG_M_D(TAG, "[%s][%d] channel %d selected.", __func__, __LINE__, nChnSeled);
        if (nChnSeled == -1) {
            LOG_M_E(TAG, "[%s][%d] no channel selected.", __func__, __LINE__);
            break;
        }
        // min max selected label
        m_pSplitWidgetMgr->MinMaxSelected(nChnSeled);

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect;
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::MIN;
        if (vecViewChn.size() == 1) {
            enChangeType = AX_NVR_VIEW_CHANGE_TYPE::MAX;
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
            LOG_M_E(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);
    } while(0);

    return QWidget::eventFilter(watched, event);
}

void PreviewMain::OnChangeSplitVideo(SPLIT_TYPE enSplitType) {
    do {
        if (enSplitType == m_pSplitWidgetMgr->GetCurrentSplitType()) {
            AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
            if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
                AX_NVR_ACTION_RES_T res;
                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
                emit signal_result_feedback(res);
            }

            LOG_M_D(TAG, "[%s][%d] split_type=%d not changed", __func__, __LINE__, enSplitType);
            break;
        }

        if (m_watcherPreview.isRunning()) {
            LOG_M_E(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        // change view split layout
        m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::UPDATE;

        // change split layout, 1<-->4<-->8<-->16<-->64
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);
        m_pScaleLabelSeled = (ScaleLabel*)m_pSplitWidgetMgr->GetCurrentStartWidget();
    } while (0);
}

void PreviewMain::OnEnablePip(bool bEnable) {

    do {
        if (m_watcherPreview.isRunning()) {
            LOG_M_E(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        if (m_pPreviewPip == nullptr) {
           return;
        }

        // for test
        m_bEnablePip = false;

        AX_NVR_RECT_T rect;
        if (bEnable) {
            m_pPreviewPip->ShowPip(m_pPreviewTopToolbar);
            rect = m_pPreviewPip->GetDispRect();
        }

        AX_NVR_ACTION_RES_T res;
        res.enSplitType = SPLIT_TYPE::PIP;
        QFutureInterface<AX_NVR_ACTION_RES_T> interface;

        m_watcherPreview.setFuture(interface.future());
        interface.reportStarted();

        std::thread thread([this, interface, rect, bEnable, res]() mutable {
            pthread_setname_np(pthread_self(), "AppPreviewPip");
            LOG_MM_W(TAG, "EnablePIP Action +++");
            res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_ERR;

            do {
                AX_BOOL bRet = AX_FALSE;
                CAXNVRDisplayCtrl *pPrimaryDispCtrl = CAXNVRFramework::GetInstance()->PrimaryDispCtrl();
                CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                if (pPrimaryDispCtrl == nullptr || pPreview == nullptr) {
                    LOG_M_E(TAG, "[%s][%d] invalid point", __func__, __LINE__);
                    break;
                }

                if (bEnable) {
                    if (!pPrimaryDispCtrl->EnablePip(rect)) {
                        LOG_M_E(TAG, "[%s][%d] enable pip channel failed.", __func__, __LINE__);
                        break;
                    }

                    if (!pPreview->StartPip(0, AX_NVR_CHN_IDX_TYPE::MAIN)) {
                        LOG_M_E(TAG, "[%s][%d] start pip channel display failed.", __func__, __LINE__);
                        break;
                    }

                    pPrimaryDispCtrl->StartPipFBChannel(rect);

                } else {
                    pPrimaryDispCtrl->StopPipFBChannel();

                    if (!pPreview->StopPip()) {
                        LOG_M_E(TAG, "[%s][%d] stop pip channel display failed.", __func__, __LINE__);
                        break;
                    }

                    if (!pPrimaryDispCtrl->DisablePip()) {
                        LOG_M_E(TAG, "[%s][%d] disable pip channel failed.", __func__, __LINE__);
                        break;
                    }
                }

                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
            } while(0);

            interface.reportResult(res, 0);
            interface.reportFinished();

            AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
            if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
                emit signal_result_feedback(res);
            }

            LOG_MM_W(TAG, "EnablePIP Action ---");
        });

        // detach thred, Use Future to ensure exit
        thread.detach();

        if (!bEnable) {
            m_pPreviewPip->ClosePip();
        }

    } while(0);
}

void PreviewMain::OnChangeMainSub1(void) {
    do {
        if (m_watcherPreview.isRunning()) {
            LOG_M_D(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
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
            LOG_M_E(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // Quit zoom mode
        m_pScaleLabelSeled->Reset();

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::MAINSUB;

        // change split layout, 1<-->4<-->8<-->16<-->64
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);

    } while (0);
}

void PreviewMain::OnChangePrevNext(PREV_NEXT_TYPE enPrevNextType) {
    do {
        if (m_watcherPreview.isRunning()) {
            LOG_M_I(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        if (!m_pSplitWidgetMgr->ChangePrevNextWidgets(enPrevNextType)) {
            AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
            if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
                AX_NVR_ACTION_RES_T res;
                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
                emit signal_result_feedback(res);
            }

            LOG_M_D(TAG, "[%s][%d] Change Prev/Next=%d failed. ", __func__, __LINE__, enPrevNextType);
            break;
        }

        SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();

        // get current labels index list
        const ax_nvr_channel_vector vecViewChn = m_pSplitWidgetMgr->GetViewChannels(enSplitType);
        if (vecViewChn.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout channel invalid", __func__, __LINE__);
            break;
        }

        // get current labels rect list
        vector<AX_NVR_RECT_T> vecVoRect = m_pSplitWidgetMgr->CalcLayouts(enSplitType,
                                                                        m_nWidth, m_nHeight,
                                                                        m_nLeftMargin, m_nTopMargin,
                                                                        m_nRightMargin, m_nBottomMargin);
        if (vecVoRect.size() == 0) {
            LOG_M_E(TAG, "[%s][%d] layout rect invalid", __func__, __LINE__);
            break;
        }

        // change type
        AX_NVR_VIEW_CHANGE_TYPE enChangeType = AX_NVR_VIEW_CHANGE_TYPE::UPDATE;
        this->update_display(vecViewChn, vecVoRect, enChangeType, enSplitType);
        m_pScaleLabelSeled = (ScaleLabel*)m_pSplitWidgetMgr->GetCurrentStartWidget();
    } while(0);
}

void PreviewMain::OnPlaybackStopStatusChanged(int nStatus) {
    if (1 == nStatus) {
        m_hPlaybackStopEvent.ResetEvent();
    } else {
        m_hPlaybackStopEvent.SetEvent();
    }
}

void PreviewMain::update_display(const ax_nvr_channel_vector &vecViewChn, const vector<AX_NVR_RECT_T> &vecVoRect,
                            AX_NVR_VIEW_CHANGE_TYPE enChangeType, SPLIT_TYPE enSplitType) {
    // Create thead to update VO channels and stop-start PPL(if record, RTSP not stop)
    AX_NVR_ACTION_RES_T res;
    res.enSplitType = enSplitType;
    QFutureInterface<AX_NVR_ACTION_RES_T> interface;

    m_watcherPreview.setFuture(interface.future());
    interface.reportStarted();

    std::thread thread([this, interface, vecViewChn, vecVoRect, enChangeType, res]() mutable {
        pthread_setname_np(pthread_self(), "AppPreviewUpd");
        LOG_MM_W(TAG, "Thread Action %d +++ ", enChangeType);
        res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_ERR;

        do {
            AX_BOOL bRet = AX_FALSE;
            CAXNVRDisplayCtrl *pPrimaryDispCtrl = CAXNVRFramework::GetInstance()->PrimaryDispCtrl();
            CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
            if (pPrimaryDispCtrl == nullptr || pPreview == nullptr) {
                LOG_MM_E(TAG, "invalid point");
                break;
            }

            if (enChangeType == AX_NVR_VIEW_CHANGE_TYPE::MAINSUB) {
                if (!pPreview->SwitchPreviewMainSub(vecViewChn)) {
                    LOG_MM_E(TAG, "Switch Preview Main/Sub failed");
                }

                res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
                res.enPreviewActionType = PREVIEW_ACTION_TYPE::PREVIEW_MAINSUB;
                break;
            } else if (AX_NVR_VIEW_CHANGE_TYPE::UPDATE == enChangeType) {
                res.enPreviewActionType = PREVIEW_ACTION_TYPE::PREVIEW_SPLIT;
            } else if (AX_NVR_VIEW_CHANGE_TYPE::MIN == enChangeType || AX_NVR_VIEW_CHANGE_TYPE::MAX == enChangeType) {
                res.enPreviewActionType = PREVIEW_ACTION_TYPE::PREVIEW_MIN_MAX;
            } else if (AX_NVR_VIEW_CHANGE_TYPE::SHOW == enChangeType) {
                res.enPreviewActionType = PREVIEW_ACTION_TYPE::PREVIEW_SHOW;
            }

            /* Wait for playback stop process finished */
            m_hPlaybackStopEvent.WaitEvent();

            switch (enChangeType)
            {
            case AX_NVR_VIEW_CHANGE_TYPE::MIN:
            case AX_NVR_VIEW_CHANGE_TYPE::MAX:
            case AX_NVR_VIEW_CHANGE_TYPE::SHOW:
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::UPDATE:
            case AX_NVR_VIEW_CHANGE_TYPE::HIDE:
                if (m_bEnablePip) {
                    pPrimaryDispCtrl->StopPipFBChannel();

                    if (!pPreview->StopPip()) {
                        LOG_MM_E(TAG, "stop pip channel display failed.");
                        break;
                    }

                    if (!pPrimaryDispCtrl->DisablePip()) {
                        LOG_MM_E(TAG, "disable pip channel failed.");
                        break;
                    }
                }

                pPreview->StopPreview();
                break;
            default:
                break;
            }

            // Update VO attr
            if (!pPrimaryDispCtrl->UpdateView(vecViewChn, vecVoRect, AX_NVR_VIEW_TYPE::PREVIEW, enChangeType)) {
                LOG_MM_E(TAG, "UpdateView changetype=%d", enChangeType);
                break;
            }

            // start PPL display
            switch (enChangeType)
            {
            case AX_NVR_VIEW_CHANGE_TYPE::MIN:
            case AX_NVR_VIEW_CHANGE_TYPE::MAX:
                bRet = pPreview->UpdatePreview(vecViewChn);
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::UPDATE:
            case AX_NVR_VIEW_CHANGE_TYPE::SHOW:
                bRet = pPreview->StartPreview(vecViewChn);
                break;
            case AX_NVR_VIEW_CHANGE_TYPE::HIDE:
            default:
                bRet = AX_TRUE;
                break;
            }

            if (!bRet) {
                LOG_MM_E(TAG, "Preview Action=%d failed.", enChangeType);
                break;
            }

            if (AX_NVR_VIEW_CHANGE_TYPE::SHOW == enChangeType
                || AX_NVR_VIEW_CHANGE_TYPE::UPDATE == enChangeType
                || AX_NVR_VIEW_CHANGE_TYPE::MAX == enChangeType) {
                for (auto m : vecViewChn) {
                    AX_NVR_DEV_ID nDeviceID = std::get<1>(m);
                    AX_U32 nWidth = 0;
                    AX_U32 nHeight = 0;
                    if (pPreview->GetResolution(nDeviceID, nWidth, nHeight) && nWidth > 0 && nHeight > 0) {
                        pPrimaryDispCtrl->UpdateChnResolution(std::get<0>(m), nWidth, nHeight);
                    }
                }
            }

            res.enResult = AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK;
        } while (0);

        interface.reportResult(res, 0);
        interface.reportFinished();

        AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
        if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
            if (AX_NVR_VIEW_CHANGE_TYPE::MAINSUB == enChangeType
                || AX_NVR_VIEW_CHANGE_TYPE::UPDATE == enChangeType
                || AX_NVR_VIEW_CHANGE_TYPE::MIN == enChangeType
                || AX_NVR_VIEW_CHANGE_TYPE::MAX == enChangeType
                || AX_NVR_VIEW_CHANGE_TYPE::SHOW == enChangeType) {

                emit signal_result_feedback(res);
            }
        }

        LOG_MM_W(TAG, "Thread Action %d ---", enChangeType);
    });

    // detach thred, Use Future to ensure exit
    thread.detach();

    if (m_bEnablePip) {
        m_pPreviewPip->ClosePip();
        m_bEnablePip = false;
    }
}

bool PreviewMain::TestChangePrevNext(PREV_NEXT_TYPE enPrevNextType) {
    if (m_pSplitWidgetMgr) {
        return m_pSplitWidgetMgr->ChangePrevNextWidgets(enPrevNextType, true);
    }
    return false;
}