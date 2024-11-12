/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ts_preview.hpp"
#include "AXNVRFrameworkDefine.h"
#include "preview/PreviewTopToolbar.h"
#include "preview/PreviewMain.h"
#include "ElapsedTimer.hpp"
#include <QApplication>
#include <QMouseEvent>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#define TAG "TS_PREVIEW"
#define MAX_LAYOUT_STREAM_COUNT (64)
#define MAX_PREVIEW_RANDOM_ACTION_COUNT (10)

CTestPreview::CTestPreview() {
    m_strModuleName = TAG;
}

AX_BOOL CTestPreview::EmitUIOprSignal(const TS_OPERATION_INFO_T& tOprInfo) {
    if (tOprInfo.target) {
        emit signal_ts_widget_opr(tOprInfo);
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CTestPreview::Start() {
    if (!initTestCase()) {
        return AX_FALSE;
    }

    m_bRunning = AX_TRUE;

    AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
    if (AX_NVR_TS_RUN_MODE::STABILITY == tTsCfg.eMode) {
        testRandom();
    } else if (AX_NVR_TS_RUN_MODE::UT == tTsCfg.eMode) {
        if (init()) {
            testChangeMainSub();
        }

        if (init()) {
            testMinMax();
        }

        if (init()) {
            testChangeSplit();
        }

        if (init()) {
            testChangePrevNext();
        }

        if (init()) {
            testPIP();
        }
    } else {
        LOG_MM_E(TAG, "Unknown test suite run mode: %d.", tTsCfg.eMode);
        return AX_FALSE;
    }

    cleanupTestCase();
    m_bRunning = AX_FALSE;

    return AX_TRUE;
}


AX_BOOL CTestPreview::initTestCase() {
    if (Enter()) {
        LOG_M_C(TAG, "<===== Start test for module PREVIEW =====>");
        return AX_TRUE;
    }

    return AX_FALSE;
}

void CTestPreview::cleanupTestCase() {
    LOG_M_C(TAG, "<===== End test for module PREVIEW =====>");
}

AX_BOOL CTestPreview::init() {
    if (!m_bRunning) {
        return AX_FALSE;
    }

    if (!CheckEntry()) {
        return AX_FALSE;
    }

    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    if (pModuleCfg) {
        CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
    }

    return AX_TRUE;
}

void CTestPreview::testRandom() {
    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    AX_S32 nRepeatCount = pModuleCfg->nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        for (AX_U32 i = 0; i < MAX_PREVIEW_RANDOM_ACTION_COUNT; i++) {
            if (!m_bRunning) {
                break;
            }

            if (!CheckEntry()) {
                break;
            }

            CTupleAction action = RandomAction();
            QWidget* pParentWidget = (QWidget*)GetWidget(std::get<0>(action).c_str());
            if (nullptr == pParentWidget) {
                CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                continue;
            }

            AX_NVR_TS_CASE_INFO_T tCaseInfo = {std::get<1>(action), TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
            if (!GetCaseInfo(std::get<2>(action), pModuleCfg->vecCaseInfo, tCaseInfo)) {
                LOG_MM_E(TAG, "Can not find <%s> configurations", std::get<2>(action).c_str());
                CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                continue;
            }

            switch (std::get<3>(action)) {
                case TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON: {
                    QPushButton* pBtn = pParentWidget->findChild<QPushButton*>(QString(std::get<1>(action).c_str()));
                    if (pBtn) {
                        if (pBtn->isEnabled()) {
                            AX_U32 nTimes = std::get<4>(action);
                            while (nTimes-- > 0 && m_bRunning) {
                                if (!CheckEntry()) {
                                    break;
                                }

                                if (!IsFinished()) {
                                    LOG_MM_W(TAG, "Last case not finished, operation <%s> ignored", pBtn->objectName().toStdString().c_str());
                                    CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                                    continue;
                                }

                                ActionBtnClick(pBtn, tCaseInfo.nOprInterval, std::get<1>(action), AX_TRUE);
                            }
                        } else {
                            LOG_MM_W(TAG, "Button <%s> is invalid, ignore this operation.", pBtn->objectName().toStdString().c_str());
                            CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                        }
                    }

                    break;
                }
                case TS_ACTION_TYPE::TS_ACTION_TYPE_TOGGLE_BUTTON: {
                    QPushButton* pBtn = pParentWidget->findChild<QPushButton*>(QString(std::get<1>(action).c_str()));
                    if (pBtn) {
                        if (pBtn->isEnabled()) {
                            AX_U32 nTimes = std::get<4>(action);
                            while (nTimes-- > 0 && m_bRunning) {
                                if (!CheckEntry()) {
                                    break;
                                }

                                if (!IsFinished()) {
                                    LOG_MM_W(TAG, "Last case not finished, operation <%s> ignored", pBtn->objectName().toStdString().c_str());
                                    CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                                    continue;
                                }

                                ActionToggleBtnClick(pBtn, tCaseInfo.nOprInterval, std::get<1>(action), AX_TRUE);
                            }
                        } else {
                            LOG_MM_W(TAG, "Toggle Button <%s> is invalid, ignore this operation.", pBtn->objectName().toStdString().c_str());
                            CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                        }
                    }

                    break;
                }
                case TS_ACTION_TYPE::TS_ACTION_TYPE_SCALE_LABEL: {
                    vector<ScaleLabel*> vecLabels;
                    PreviewMain* pPreviewMain = (PreviewMain*)GetWidget("PreviewMain");
                    QList<NVR_PREVIEW_WIDGET_T> listView = pPreviewMain->m_pSplitWidgetMgr->GetPreviewWidgetList();
                    for (auto &m : listView) {
                        if (m.bDisp) {
                            vecLabels.emplace_back(m.pDispWidget);
                        }
                    }

                    if (vecLabels.size() > 0) {
                        if (!IsFinished()) {
                            LOG_MM_W(TAG, "Last case not finished, operation <MAX-MIN> ignored");
                            break;
                        }

                        AX_U32 nIndex = rand() % vecLabels.size();
                        ActionScaleLabelDbClicked(vecLabels[nIndex], tCaseInfo.nOprInterval, "LabelMax", AX_TRUE);

                        if (!IsFinished()) {
                            LOG_MM_W(TAG, "Case <MAX> not finished, operation <MIN> ignored");
                            break;
                        }

                        ActionScaleLabelDbClicked(vecLabels[nIndex], tCaseInfo.nOprInterval, "LabelMin", AX_TRUE);
                    }
                    break;
                }
                default: {
                    LOG_M_E(TAG, "Unsupported action type: %d", std::get<3>(action));
                    break;
                }
            }

            CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
        }
    }
}

void CTestPreview::testChangeMainSub() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    QWidget* pToolbar = (QWidget*)GetWidget("PreviewTopToolbar");
    QPushButton* pBtn = nullptr;

    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();

    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"main_sub", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("main_sub", pModuleCfg->vecCaseInfo, tCaseInfo);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        static AX_S32 nRound = 0;
        LOG_M_C(TAG, "Start round %d of case PREVIEW-MAIN_SUB", ++nRound);

        pBtn = pToolbar->findChild<QPushButton*>("pushButton_main_sub1");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "main_sub(sub)");

            /* Recover */
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "main_sub(main)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_main_sub1");
        }
    }
}

void CTestPreview::testMinMax() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"min_max", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("min_max", GetModuleConfig()->vecCaseInfo, tCaseInfo);

    vector<ScaleLabel*> vecLabels;
    PreviewMain* pPreviewMain = (PreviewMain*)GetWidget("PreviewMain");
    QList<NVR_PREVIEW_WIDGET_T> listView = pPreviewMain->m_pSplitWidgetMgr->GetPreviewWidgetList();
    for (auto &m : listView) {
        if (m.bDisp) {
            vecLabels.emplace_back(m.pDispWidget);
        }
    }

    if (vecLabels.size() > 0) {
        AX_U32 nIndex = rand() % vecLabels.size();
        ActionScaleLabelDbClicked(vecLabels[nIndex], tCaseInfo.nOprInterval, "LabelMax");
        ActionScaleLabelDbClicked(vecLabels[nIndex], tCaseInfo.nOprInterval, "LabelMin");
    }
}

void CTestPreview::testChangeSplit() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    QPushButton* pBtn = nullptr;
    AX_BOOL bActionRet = AX_FALSE;

    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();

    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"split", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("split", pModuleCfg->vecCaseInfo, tCaseInfo);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        static AX_S32 nRound = 0;
        LOG_M_C(TAG, "Start round %d of case PREVIEW-SPLIT", ++nRound);
        QWidget* pToolbar = (QWidget*)GetWidget("PreviewTopToolbar");
        pBtn = pToolbar->findChild<QPushButton*>("pushButton_1x1");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "split(pushButton_1x1)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_1x1");
            break;
        }

        pBtn = pToolbar->findChild<QPushButton*>("pushButton_2x2");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "split(pushButton_2x2)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_2x2");
            break;
        }

        pBtn = pToolbar->findChild<QPushButton*>("pushButton_1x8");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "split(pushButton_1x8)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_1x8");
            break;
        }

        pBtn = pToolbar->findChild<QPushButton*>("pushButton_4x4");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "split(pushButton_4x4)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_4x4");
            break;
        }

        pBtn = pToolbar->findChild<QPushButton*>("pushButton_8x8");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "split(pushButton_8x8)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_8x8");
            break;
        }

        /* Recover layout */
        pBtn = pToolbar->findChild<QPushButton*>("pushButton_2x2");
        if (pBtn) {
            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, "split(pushButton_2x2)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_2x2");
            break;
        }
    }
}

void CTestPreview::testChangePrevNext() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    QWidget* pToolbar = (QWidget*)GetWidget("PreviewTopToolbar");
    QPushButton* pBtnPrev = pToolbar->findChild<QPushButton*>("pushButton_previous");
    QPushButton* pBtnNext = pToolbar->findChild<QPushButton*>("pushButton_next");
    if (!pBtnPrev || !pBtnNext) {
        LOG_M_E(TAG, "Can not find PREV/NEXT buttons, ignore this case.");
        return;
    }

    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"previous_next", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("previous_next", pModuleCfg->vecCaseInfo, tCaseInfo);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        static AX_S32 nRound = 0;
        LOG_M_C(TAG, "Start round %d of case PREVIEW-PREV_NEXT", ++nRound);

        // AX_U32 nOprCount = MAX_LAYOUT_STREAM_COUNT / 4 - 1;
        AX_U32 nOprCount = 2;
        AX_U32 nIndex = 0;
        while (nIndex++ < nOprCount && m_bRunning) {
            if (!CheckEntry()) {
                break;
            }

            ActionBtnClick(pBtnNext, tCaseInfo.nOprInterval, "previous_next(next)");
        }

        nIndex = 0;
        while (nIndex++ < nOprCount && m_bRunning) {
            if (!CheckEntry()) {
                break;
            }

            ActionBtnClick(pBtnPrev, tCaseInfo.nOprInterval, "previous_next(previous)");
        }
    }
}

void CTestPreview::testPIP() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    QWidget* pToolbar = (QWidget*)GetWidget("PreviewTopToolbar");
    QPushButton* pBtn = nullptr;

    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();

    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"pip", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("pip", pModuleCfg->vecCaseInfo, tCaseInfo);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        static AX_S32 nRound = 0;
        LOG_M_C(TAG, "Start round %d of case PREVIEW-PIP", ++nRound);

        pBtn = pToolbar->findChild<QPushButton*>("pushButton_pip");
        if (pBtn) {
            ActionToggleBtnClick(pBtn, tCaseInfo.nOprInterval, "PIP(open)");
            ActionToggleBtnClick(pBtn, tCaseInfo.nOprInterval, "PIP(close)");
        } else {
            LOG_M_E(TAG, "Can not find button <%s>", "pushButton_pip");
        }
    }
}

void CTestPreview::OnRecvResult(const AX_NVR_ACTION_RES_T& tActionResult) {
    if (AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK == tActionResult.enResult) {
        LOG_MM_C(TAG, "Action<%s> successed." , m_strActionName.c_str());
    } else {
        LOG_MM_E(TAG, "Action<%s> failed." , m_strActionName.c_str());
    }

    m_bCaseResult = AX_TRUE;
}

AX_BOOL CTestPreview::GetCaseInfo(const string& strCaseName, vector<AX_NVR_TS_CASE_INFO_T>& vecCaseInfo, AX_NVR_TS_CASE_INFO_T& tOutInfo) {
    for (auto &m : vecCaseInfo) {
        if (m.strName == strCaseName) {
            tOutInfo = m;
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

CTupleAction CTestPreview::RandomAction() {
    CTupleAction action[MAX_PREVIEW_RANDOM_ACTION_COUNT] = {
        make_tuple("PreviewTopToolbar", "pushButton_1x1", "split", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_2x2", "split", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_1x8", "split", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_4x4", "split", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_8x8", "split", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_previous", "previous_next", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_next", "previous_next", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PreviewTopToolbar", "pushButton_main_sub1", "main_sub", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 2),
        make_tuple("PreviewTopToolbar", "pushButton_pip", "pip", TS_ACTION_TYPE::TS_ACTION_TYPE_TOGGLE_BUTTON, 1),
        make_tuple("", "", "min_max", TS_ACTION_TYPE::TS_ACTION_TYPE_SCALE_LABEL, 1)
    };

    AX_S32 nIndex = rand() % MAX_PREVIEW_RANDOM_ACTION_COUNT;
    return action[nIndex];
}