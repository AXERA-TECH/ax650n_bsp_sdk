/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ts_base.hpp"
#include "ElapsedTimer.hpp"

#define TAG "TS_BASE"


AX_VOID CTestSuiteBase::RegisterWidget(const AX_CHAR* strName, QWidget* pWidget) {
    m_mapWidgets[strName] = pWidget;
};

QWidget* CTestSuiteBase::GetWidget(const AX_CHAR* strName) {
    std::map<std::string, QWidget*>::iterator itFinder = m_mapWidgets.find(strName);
    if (itFinder != m_mapWidgets.end()) {
        return itFinder->second;
    }

    return nullptr;
};

AX_BOOL CTestSuiteBase::Enter() {
    QPushButton* pWidget = (QPushButton*)GetWidget("BtnEntry");
    if (pWidget) {
        LOG_M_C(TAG, "Enter through button <%s>.", pWidget->objectName().toStdString().c_str());

        ResetAction(pWidget->objectName().toStdString().c_str());

        TS_OPERATION_INFO_T tOprInfo;
        tOprInfo.target = pWidget;
        tOprInfo.type = TS_OPERATION_TYPE::TS_OPR_BTN_CLICK;
        tOprInfo.args.bBtnChecked = true;
        EmitUIOprSignal(tOprInfo);

        CElapsedTimer::GetInstance()->mSleep(1000);
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CTestSuiteBase::CheckEntry() {
    AX_BOOL bRet = AX_TRUE;
    QPushButton* pWidget = (QPushButton*)GetWidget("BtnEntry");
    if (pWidget) {
        bRet = pWidget->isChecked() ? AX_TRUE : AX_FALSE;
        if (!bRet) {
            LOG_MM_E(m_strModuleName.c_str(), "BtnEntry(%s) is not checked, ignore current case.");
        }
        return bRet;
    } else {
        LOG_MM_E(m_strModuleName.c_str(), "Can not find BtnEntry(%s), ignore current case.");
    }

    return AX_FALSE;
}

AX_VOID CTestSuiteBase::SetModuleConfig(const AX_NVR_TS_MODULE_INFO_T& tConfig) {
    m_tModuleConfig = tConfig;
}

AX_NVR_TS_MODULE_INFO_PTR CTestSuiteBase::GetModuleConfig() {
    return &m_tModuleConfig;
}

AX_VOID CTestSuiteBase::ActionToggleBtnClick(QPushButton* pBtn, AX_S32 nWaitTick /*= 1000*/, const string& strActionName /*= ""*/, AX_BOOL bFeedbackRequired /*= AX_FALSE*/) {
    if (nullptr == pBtn) {
        return;
    }

    LOG_M_C(m_strModuleName.c_str(), "Action<%s> triggered", strActionName.empty() ? pBtn->objectName().toStdString().c_str() : strActionName.c_str());

    ResetAction(strActionName.empty() ? pBtn->objectName().toStdString() : strActionName, bFeedbackRequired);

    TS_OPERATION_INFO_T tOprInfo;
    tOprInfo.target = pBtn;
    tOprInfo.type = TS_OPERATION_TYPE::TS_OPR_TOGGLE_BTN_CLICK;
    EmitUIOprSignal(tOprInfo);

    CElapsedTimer::GetInstance()->mSleep(nWaitTick);
}

AX_VOID CTestSuiteBase::ActionBtnClick(QPushButton* pBtn, AX_S32 nWaitTick /*= 1000*/, const string& strActionName /*= ""*/, AX_BOOL bFeedbackRequired /*= AX_FALSE*/) {
    if (nullptr == pBtn) {
        return;
    }

    LOG_M_C(m_strModuleName.c_str(), "Action<%s> triggered", strActionName.empty() ? pBtn->objectName().toStdString().c_str() : strActionName.c_str());

    ResetAction(strActionName.empty() ? pBtn->objectName().toStdString() : strActionName, bFeedbackRequired);

    TS_OPERATION_INFO_T tOprInfo;
    tOprInfo.target = pBtn;
    tOprInfo.type = TS_OPERATION_TYPE::TS_OPR_BTN_CLICK;
    tOprInfo.args.bBtnChecked = true;
    EmitUIOprSignal(tOprInfo);

    CElapsedTimer::GetInstance()->mSleep(nWaitTick);
}

AX_VOID CTestSuiteBase::ActionTabClick(QTabWidget* pTab, AX_U32 nIndex, AX_S32 nWaitTick /*= 0*/, const string& strActionName /*= ""*/, AX_BOOL bFeedbackRequired /*= AX_FALSE*/) {
    if (nullptr == pTab) {
        return;
    }

    AX_U32 nTabCount = pTab->count();
    if (nIndex >= nTabCount) {
        LOG_M_E(m_strModuleName.c_str(), "Tab index %d out of range (0 ~ %d)", nIndex, nTabCount);
        return;
    }

    LOG_M_C(m_strModuleName.c_str(), "Action<%s> triggered", strActionName.empty() ? pTab->tabText(nIndex).toStdString().c_str() : strActionName.c_str());

    ResetAction(strActionName.empty() ? pTab->tabText(nIndex).toStdString().c_str() : strActionName, bFeedbackRequired);

    TS_OPERATION_INFO_T tOprInfo;
    tOprInfo.target = pTab;
    tOprInfo.type = TS_OPERATION_TYPE::TS_OPR_TAB_SELECT;
    tOprInfo.args.nTabSelIndex = nIndex;
    EmitUIOprSignal(tOprInfo);

    CElapsedTimer::GetInstance()->mSleep(nWaitTick);
}

AX_VOID CTestSuiteBase::ActionScaleLabelDbClicked(ScaleLabel* pLabel, AX_S32 nWaitTick /*= 0*/, const string& strActionName /*= ""*/, AX_BOOL bFeedbackRequired /*= AX_FALSE*/) {
    if (nullptr == pLabel) {
        return;
    }

    LOG_M_C(m_strModuleName.c_str(), "Action<%s> triggered", strActionName.empty() ? pLabel->text().toStdString().c_str() : strActionName.c_str());

    ResetAction(strActionName.empty() ? pLabel->text().toStdString().c_str() : strActionName, bFeedbackRequired);

    /* pDbClickEvent would be cleaned by QT itself */
    QMouseEvent* pDbClickEvent = new QMouseEvent(QEvent::MouseButtonDblClick, QPoint(0, 0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::postEvent(pLabel, pDbClickEvent);
    CElapsedTimer::GetInstance()->mSleep(nWaitTick);
}

AX_VOID CTestSuiteBase::ActionCalendarDateSelected(QCalendarWidget* pCalendar, QDate& date, AX_S32 nWaitTick /*= 0*/, const string& strActionName /*= ""*/, AX_BOOL bFeedbackRequired /*= AX_FALSE*/) {
    if (nullptr == pCalendar) {
        return;
    }

    AX_CHAR szAction[32] = {0};
    sprintf(szAction, "change date(%s)", date.toString().toStdString().c_str());

    LOG_M_C(m_strModuleName.c_str(), "Action<%s> triggered", strActionName.empty() ? szAction : strActionName.c_str());
    ResetAction(strActionName.empty() ? szAction : strActionName, bFeedbackRequired);

    TS_OPERATION_INFO_T tOprInfo;
    tOprInfo.target = pCalendar;
    tOprInfo.type = TS_OPERATION_TYPE::TS_OPR_CALENDAR_SELECT;
    tOprInfo.args.calendarSelDate = date;
    EmitUIOprSignal(tOprInfo);

    pCalendar->selectionChanged();

    CElapsedTimer::GetInstance()->mSleep(nWaitTick);
}