/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ts_playback.hpp"
#include "AXNVRFrameworkDefine.h"
#include "ElapsedTimer.hpp"
#include "playback/PlaybackMain.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QCalendarWidget>

#define TAG "TS_PLAYBACK"

CTestPlayback::CTestPlayback() {
    m_strModuleName = TAG;
}

AX_BOOL CTestPlayback::EmitUIOprSignal(const TS_OPERATION_INFO_T& tOprInfo) {
    if (tOprInfo.target) {
        emit signal_ts_widget_opr(tOprInfo);
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CTestPlayback::Start() {
    if (!initTestCase()) {
        return AX_FALSE;
    }

    m_bRunning = AX_TRUE;
    AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
    if (AX_NVR_TS_RUN_MODE::STABILITY == tTsCfg.eMode) {
        testRandom();
    } else if (AX_NVR_TS_RUN_MODE::UT == tTsCfg.eMode) {
        if (init()) {
            testSingleStreamPlayProcess();
        }

        if (init()) {
            testMultiStreamPlayProcess();
        }
    } else {
        LOG_MM_E(TAG, "Unknown test suite run mode: %d.", tTsCfg.eMode);
        return AX_FALSE;
    }

    m_bRunning = AX_FALSE;

    cleanupTestCase();

    return AX_TRUE;
}

AX_BOOL CTestPlayback::initTestCase() {
    if (Enter()) {
        LOG_M_C(TAG, "<===== Start test for module PLAYBACK =====>");
        SaveCtrlButtons();

        return AX_TRUE;
    }

    return AX_FALSE;
}

void CTestPlayback::cleanupTestCase() {
    LOG_M_C(TAG, "<===== End test for module PLAYBACK =====>");
}

AX_BOOL CTestPlayback::init() {
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

void CTestPlayback::testRandom() {
    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    AX_S32 nRepeatCount = pModuleCfg->nRepeatCount;

    QWidget* pParentWidget = (QWidget*)GetWidget("PlayCtrl");
    QPushButton* pStartBtn = pParentWidget->findChild<QPushButton*>(QString("pushButtonPlay"));

    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"control", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("control", pModuleCfg->vecCaseInfo, tCaseInfo);

    SelectDate(pModuleCfg->strDate);

    QTabWidget* pTab = ((QWidget*)GetWidget("PlayLeftToolbar"))->findChild<QTabWidget*>(QString("tabWidget"));
    AX_U32 nTabCount = pTab->count();
    /* steps of each round of random test:
        step1: select tab_0, select first channel, push button PLAY, random action on controls(play, pause, stop, etc.)
        step2: select tab_1, select all channels, push button PLAY, random action on controls(play, stop, etc.)
        step3: select tab_2, select all channels, push button PLAY, random action on controls(play, stop, etc.)
        step4: select tab_3, select all channels, push button PLAY, random action on controls(play, stop, etc.)
    */
    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        AX_U32 nTabIndex = 0;
        while (nTabIndex < nTabCount && m_bRunning) {
            if (!CheckEntry()) {
                break;
            }

            if (!IsFinished()) {
                LOG_MM_W(TAG, "Last case not finished, tab switch ignored");
                CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                continue;
            }

            AX_BOOL bSelectAll = nTabIndex == 0 ? AX_FALSE : AX_TRUE;
            TabChangeAndSelect(nTabIndex, bSelectAll, tCaseInfo.nOprInterval);
            ActionBtnClick(pStartBtn, tCaseInfo.nOprInterval, "pushButtonPlay");
            /* 1x1 layout support all button operations while other layouts only support some of them,
               So, let 1x1 layout case get more chances to run. */
            RandomControl(nTabIndex, 0 == nTabIndex ? 5 : 2);

            nTabIndex++;
        }
    }
}

void CTestPlayback::RandomControl(AX_U32 nTabIndex, AX_U32 nRoundCount) {
    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();

    auto __GetRandomDate__ = [this](QCalendarWidget* pCalendar, AX_BOOL bValid) {
        QDate currDate = QDate::currentDate();
        QDate arrDate[2] = {QDate(currDate.year(), currDate.month(), currDate.day() == 1 ? 2 : currDate.day() - 1), QDate::currentDate()};
        return arrDate[bValid ? 1 : 0];
    };

    static AX_BOOL s_bFastSpeed = AX_FALSE;
    static AX_BOOL s_bSlowSpeed = AX_FALSE;
    while (nRoundCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        for (AX_U32 i = 0; i < MAX_PLAYBACK_RANDOM_ACTION_COUNT; i++) {
            if (!m_bRunning) {
                break;
            }

            if (!CheckEntry()) {
                break;
            }

            CTupleAction action = RandomAction(nTabIndex == 0 ? MAX_PLAYBACK_RANDOM_ACTION_COUNT : 4);
            if (std::get<1>(action) == "pushButtonFast") {
                if (s_bFastSpeed) {
                    /* TODO: Only support 2-times-speed now */
                    LOG_MM_W(TAG, "Ignore fast speed operation twice.");
                    continue;
                } else if (!s_bSlowSpeed) {
                    s_bFastSpeed = AX_TRUE;
                }

                s_bSlowSpeed = AX_FALSE;
            } else if (std::get<1>(action) == "pushButtonSlow") {
                if (s_bSlowSpeed) {
                    /* TODO: Only support 1/2-times-speed now */
                    LOG_MM_W(TAG, "Ignore slow speed operation twice.");
                    continue;
                } else if (!s_bFastSpeed) {
                    s_bSlowSpeed = AX_TRUE;
                }

                s_bFastSpeed = AX_FALSE;
            }

            QWidget* pParentWidget = (QWidget*)GetWidget(std::get<0>(action).c_str());
            AX_NVR_TS_CASE_INFO_T tCaseInfo = {std::get<1>(action), TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
            GetCaseInfo(std::get<2>(action), pModuleCfg->vecCaseInfo, tCaseInfo);

            switch (std::get<3>(action)) {
                case TS_ACTION_TYPE::TS_ACTION_TYPE_TAB: {
                    /* Would not come here now */
                    if (nullptr == pParentWidget) {
                        continue;
                    }

                    if (!IsFinished()) {
                        LOG_MM_W(TAG, "Last case not finished, operation <%s> ignored", std::get<1>(action).c_str());
                        CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                        continue;
                    }

                    QTabWidget* pTab = pParentWidget->findChild<QTabWidget*>(QString(std::get<1>(action).c_str()));
                    if (pTab) {
                        AX_U32 nRandomIndex = rand() % pTab->count();
                        TabChangeAndSelect(nRandomIndex, AX_TRUE, tCaseInfo.nOprInterval);
                    }

                    break;
                }
                case TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON: {
                    if (nullptr == pParentWidget) {
                        continue;
                    }

                    if (!IsFinished()) {
                        LOG_MM_W(TAG, "Last case not finished, operation <%s> ignored", std::get<1>(action).c_str());
                        CElapsedTimer::GetInstance()->mSleep(pModuleCfg->nCaseInterval);
                        continue;
                    }

                    QPushButton* pBtn = pParentWidget->findChild<QPushButton*>(QString(std::get<1>(action).c_str()));
                    if (pBtn) {
                        AX_U32 nTimes = std::get<4>(action);
                        while (nTimes-- > 0 && m_bRunning) {
                            ActionBtnClick(pBtn, tCaseInfo.nOprInterval, std::get<1>(action), AX_TRUE);
                        }
                    }

                    break;
                }
                case TS_ACTION_TYPE::TS_ACTION_TYPE_CALENDAR: {
                    /* Would not come here now */
                    if (nullptr == pParentWidget) {
                        continue;
                    }

                    QCalendarWidget* pCalendar = pParentWidget->findChild<QCalendarWidget*>(QString(std::get<1>(action).c_str()));
                    if (pCalendar) {
                        AX_BOOL bSelectValidDate = rand() % 2 ? AX_TRUE : AX_FALSE;
                        QDate date = __GetRandomDate__(pCalendar, bSelectValidDate);
                        LOG_MM_C(TAG, "Select date: %s", date.toString().toStdString().c_str());
                        ActionCalendarDateSelected(pCalendar, date, tCaseInfo.nOprInterval, std::get<1>(action));
                    }
                    break;
                }
                case TS_ACTION_TYPE::TS_ACTION_TYPE_SCALE_LABEL: {
                    ActionMinMaxLabel(tCaseInfo.nOprInterval);
                    break;
                }
                default : {
                    LOG_M_E(TAG, "Unsupported action type: %d", std::get<3>(action));
                    break;
                }
            }
        }
    }
}

void CTestPlayback::testSingleStreamPlayProcess() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    QTabWidget* pTab = nullptr;
    AX_BOOL bActionRet = AX_FALSE;
    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"single stream", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("single stream", pModuleCfg->vecCaseInfo, tCaseInfo);

    SelectDate(pModuleCfg->strDate);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        static AX_S32 nRound = 0;
        LOG_M_C(TAG, "Start round %d of case UT-PLAYBACK-SINGLE", ++nRound);

        QWidget* pToolbar = (QWidget*)GetWidget("PlayLeftToolbar");
        pTab = pToolbar->findChild<QTabWidget*>("tabWidget");
        if (pTab) {
            TabChangeAndSelect(0, AX_FALSE, tCaseInfo.nOprInterval);
            ActionBtnClick(FindActionBtn("Play"), tCaseInfo.nOprInterval, "Play");
            ActionBtnClick(FindActionBtn("MainSub"), tCaseInfo.nOprInterval, "MainSub");
            ActionBtnClick(FindActionBtn("MainSub"), tCaseInfo.nOprInterval, "MainSub");
            ActionBtnClick(FindActionBtn("Fast"), 5000, "Fast");
            ActionBtnClick(FindActionBtn("Slow"), 5000, "Slow");
            ActionBtnClick(FindActionBtn("Slow"), 5000, "Slow");
            ActionBtnClick(FindActionBtn("Fast"), 5000, "Fast");
            ActionBtnClick(FindActionBtn("Pause"), tCaseInfo.nOprInterval, "Pause");
            ActionBtnClick(FindActionBtn("Next"), tCaseInfo.nOprInterval, "Next");
            ActionBtnClick(FindActionBtn("Next"), tCaseInfo.nOprInterval, "Next");
            ActionBtnClick(FindActionBtn("Next"), tCaseInfo.nOprInterval, "Next");
            ActionBtnClick(FindActionBtn("Previous"), tCaseInfo.nOprInterval, "Previous");
            ActionBtnClick(FindActionBtn("Previous"), tCaseInfo.nOprInterval, "Previous");
            ActionBtnClick(FindActionBtn("Previous"), tCaseInfo.nOprInterval, "Previous");
            ActionBtnClick(FindActionBtn("Stop"), tCaseInfo.nOprInterval, "Stop");
            LocateTime(2359);
            ActionBtnClick(FindActionBtn("Reverse"), 5000, "Reverse");
            ActionBtnClick(FindActionBtn("Slow"), 5000, "Slow");
            ActionBtnClick(FindActionBtn("Fast"), 5000, "Fast");
            ActionBtnClick(FindActionBtn("Fast"), 5000, "Fast");
            ActionBtnClick(FindActionBtn("Slow"), 5000, "Slow");
            ActionBtnClick(FindActionBtn("Stop"), tCaseInfo.nOprInterval, "Stop");
            LocateTime(0);
        } else {
            LOG_M_E(TAG, "Can not find widget <%s>", "tabWidget");
            break;
        }

        bActionRet = AX_TRUE;
    }
}

void CTestPlayback::testMultiStreamPlayProcess() {
    if (!IsFinished()) {
        LOG_M_E(TAG, "Last case not finished, try recover and continue.");
        TryRecover();
    }

    QTabWidget* pTab = nullptr;
    AX_BOOL bActionRet = AX_FALSE;
    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"multiple stream", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("multiple stream", pModuleCfg->vecCaseInfo, tCaseInfo);

    SelectDate(pModuleCfg->strDate);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        static AX_S32 nRound = 0;
        LOG_M_C(TAG, "Start round %d of case UT-PLAYBACK-MULTIPLE", ++nRound);

        QWidget* pToolbar = (QWidget*)GetWidget("PlayLeftToolbar");
        pTab = pToolbar->findChild<QTabWidget*>("tabWidget");
        if (pTab) {
            TabChangeAndSelect(2, AX_TRUE, tCaseInfo.nOprInterval);
            ActionBtnClick(FindActionBtn("Play"), tCaseInfo.nOprInterval, "Play");
            ActionBtnClick(FindActionBtn("MainSub"), tCaseInfo.nOprInterval, "MainSub");
            ActionBtnClick(FindActionBtn("MainSub"), tCaseInfo.nOprInterval, "MainSub");
            ActionMinMaxLabel(tCaseInfo.nOprInterval);
            ActionBtnClick(FindActionBtn("Stop"), tCaseInfo.nOprInterval, "Stop");
        } else {
            LOG_M_E(TAG, "Can not find widget <%s>", "tabWidget");
            break;
        }

        bActionRet = AX_TRUE;
    }
}

void CTestPlayback::LocateTime(AX_U32 nTimeHHMM) {
    QWidget* pControlWidget = (QWidget*)GetWidget("PlayCtrl");
    QWidget* pTimeline = pControlWidget->findChild<QWidget*>(QString("PlaybackTimeLine"));
    if (pTimeline) {
        emit timeLocate(nTimeHHMM);
    }
}

void CTestPlayback::SelectDate(const string& strDate) {
    QWidget* pParentWidget = (QWidget*)GetWidget("PlayLeftToolbar");
    if (nullptr == pParentWidget) {
        return;
    }

    QCalendarWidget* pCalendar = pParentWidget->findChild<QCalendarWidget*>(QString("calendarWidget"));
    if (pCalendar) {
        QDate date = QDate::fromString(QString(strDate.c_str()), "yyyy-MM-dd");
        ActionCalendarDateSelected(pCalendar, date, 1000, "SelectDate");
    }
}

void CTestPlayback::TabChangeAndSelect(AX_U32 nTabIndex, AX_BOOL bSelectAll, AX_S32 nWaitTick /*= 1000*/) {
    QWidget* pParentWidget = (QWidget*)GetWidget("PlayLeftToolbar");
    if (nullptr == pParentWidget) {
        return;
    }

    QTabWidget* pTab = pParentWidget->findChild<QTabWidget*>(QString("tabWidget"));
    if (pTab) {
        AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();

        ActionTabClick(pTab, nTabIndex, nWaitTick, "tabWidget");

        QTableWidget* pTable = nullptr;
        switch (nTabIndex) {
            case 0: {
                pTable = pTab->findChild<QTableWidget*>(QString("tableWidget_1x1"));
                break;
            }
            case 1: {
                pTable = pTab->findChild<QTableWidget*>(QString("tableWidget_2x2"));
                break;
            }
            case 2: {
                pTable = pTab->findChild<QTableWidget*>(QString("tableWidget_1_7"));
                break;
            }
            case 3: {
                pTable = pTab->findChild<QTableWidget*>(QString("tableWidget_4x4"));
                break;
            }
            default: break;
        }

        if (bSelectAll) {
            int nChannelCnt = pTable->rowCount();
            for (int i = 0; i < nChannelCnt; i++) {
                emit tableCellClicked(i, 0);
            }
        } else {
            emit tableCellClicked(0, 0);
        }

        CElapsedTimer::GetInstance()->mSleep(nWaitTick);
    }
}

void CTestPlayback::ActionMinMaxLabel(AX_S32 nWaitTick /*= 1000*/) {
    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();

    vector<ScaleLabel*> vecLabels;
    PlaybackMain* pPlaybackMain = (PlaybackMain*)GetWidget("PlayMain");
    QList<NVR_PREVIEW_WIDGET_T> listView = pPlaybackMain->m_pSplitWidgetMgr->GetPreviewWidgetList();
    for (auto &m : listView) {
        if (m.bDisp) {
            vecLabels.emplace_back(m.pDispWidget);
        }
    }

    if (vecLabels.size() > 0) {
        /* UT-PLAYBACK only prepare 2 sample videos now */
        AX_U32 nIndex = rand() % 2;
        if (nIndex >= vecLabels.size()) {
            nIndex = 0;
        }

        ActionScaleLabelDbClicked(vecLabels[nIndex], nWaitTick, "LabelMax");
        ActionScaleLabelDbClicked(vecLabels[nIndex], nWaitTick, "LabelMin");
    }
}

void CTestPlayback::OnRecvResult(const AX_NVR_ACTION_RES_T& tActionResult) {
    if (AX_NVR_PREVIEW_RES_TYPE::PREVIEW_OK == tActionResult.enResult) {
        LOG_MM_C(TAG, "Action<%s> successed." , m_strActionName.c_str());
    } else {
        LOG_MM_E(TAG, "Action<%s> failed." , m_strActionName.c_str());
    }

    m_bCaseResult = AX_TRUE;
}

AX_BOOL CTestPlayback::GetCaseInfo(const string& strCaseName, vector<AX_NVR_TS_CASE_INFO_T>& vecCaseInfo, AX_NVR_TS_CASE_INFO_T& tOutInfo) {
    for (auto &m : vecCaseInfo) {
        if (m.strName == strCaseName) {
            tOutInfo = m;
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_VOID CTestPlayback::SaveCtrlButtons() {
    QWidget* pControlWidget = (QWidget*)GetWidget("PlayCtrl");
    m_mapActionButtons["Play"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonPlay"));
    m_mapActionButtons["Pause"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonPause"));
    m_mapActionButtons["Stop"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonStop"));
    m_mapActionButtons["Reverse"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonReverse"));
    m_mapActionButtons["Previous"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonPreviousFrame"));
    m_mapActionButtons["Next"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonNextFrame"));
    m_mapActionButtons["Slow"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonSlow"));
    m_mapActionButtons["Fast"] = pControlWidget->findChild<QPushButton*>(QString("pushButtonFast"));

    QWidget* pTopWidget = (QWidget*)GetWidget("PlayTopToolbar");
    m_mapActionButtons["MainSub"] = pControlWidget->findChild<QPushButton*>(QString("pushButton_main_sub1"));
}

QPushButton* CTestPlayback::FindActionBtn(const string& strBtnName) {
    std::map<std::string, QPushButton*>::iterator itFinder = m_mapActionButtons.find(strBtnName);
    if (itFinder != m_mapActionButtons.end()) {
        return itFinder->second;
    }

    return nullptr;
}

CTupleAction CTestPlayback::RandomAction(AX_U32 nRangeMax /*= MAX_PLAYBACK_RANDOM_ACTION_COUNT*/) {
    if (nRangeMax > MAX_PLAYBACK_RANDOM_ACTION_COUNT) {
        nRangeMax = MAX_PLAYBACK_RANDOM_ACTION_COUNT;
    }

    CTupleAction action[MAX_PLAYBACK_RANDOM_ACTION_COUNT] = {
        // <- Valid for all tabs ->
        make_tuple("PlayCtrl", "pushButtonPlay", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PlayCtrl", "pushButtonStop", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PlayTopToolbar", "pushButton_main_sub1", "main_sub", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 2),
        make_tuple("", "", "min_max", TS_ACTION_TYPE::TS_ACTION_TYPE_SCALE_LABEL, 1),

        // <- Valid only for tab 1x1 ->
        make_tuple("PlayCtrl", "pushButtonPause", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PlayCtrl", "pushButtonReverse", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PlayCtrl", "pushButtonPreviousFrame", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 3),
        make_tuple("PlayCtrl", "pushButtonNextFrame", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 3),
        make_tuple("PlayCtrl", "pushButtonSlow", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
        make_tuple("PlayCtrl", "pushButtonFast", "control", TS_ACTION_TYPE::TS_ACTION_TYPE_BUTTON, 1),
    };

    AX_S32 nIndex = rand() % nRangeMax;
    return action[nIndex];
}
