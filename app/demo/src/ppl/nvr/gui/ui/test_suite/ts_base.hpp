/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <QtWidgets/QWidget>
#include "TestSuiteConfigParser.h"
#include "NVRConfigParser.h"
#include "ax_base_type.h"
#include "AppLogApi.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPushButton>
#include <QTabWidget>
#include <QCalendarWidget>
#include "utils/ScaleLabel.h"

#define TS_DEFAULT_REPEAT_COUNT (1)
#define TS_DEFAULT_OPERATION_INTERFAL (5000)
#define TS_RECOVER_BTN_1 "RecoverBtn_1"
#define TS_RECOVER_BTN_2 "RecoverBtn_2"

enum class TS_ACTION_TYPE {
    TS_ACTION_TYPE_BUTTON = 0,
    TS_ACTION_TYPE_TOGGLE_BUTTON,
    TS_ACTION_TYPE_TAB,
    TS_ACTION_TYPE_SCALE_LABEL,
    TS_ACTION_TYPE_CALENDAR,
};

enum class TS_OPERATION_TYPE {
    TS_OPR_BTN_CLICK = 0,
    TS_OPR_TOGGLE_BTN_CLICK,
    TS_OPR_TAB_SELECT,
    TS_OPR_CALENDAR_SELECT,
};


typedef struct TS_OPERATION_INFO {
    TS_OPERATION_TYPE type;
    QWidget* target {nullptr};

    union _TS_OPERATION_ARGS {
        bool bBtnChecked;
        AX_U32 nTabSelIndex;
        QDate calendarSelDate;

        _TS_OPERATION_ARGS() {};
    } args;
} TS_OPERATION_INFO_T;

typedef std::tuple<string/*parent widget name*/, string/*action widget name*/, string/*action name*/, TS_ACTION_TYPE, AX_U32/*nTimes*/> CTupleAction;

class CTestSuiteBase
{
public:
    virtual AX_BOOL Start() = 0;
    virtual AX_VOID Stop() { m_bRunning = AX_FALSE; };
    virtual AX_VOID ActionBtnClick(QPushButton* pBtn, AX_S32 nWaitTick = 1000, const string& strActionName = "", AX_BOOL bFeedbackRequired = AX_FALSE);
    virtual AX_VOID ActionToggleBtnClick(QPushButton* pBtn, AX_S32 nWaitTick = 1000, const string& strActionName = "", AX_BOOL bFeedbackRequired = AX_FALSE);
    virtual AX_VOID ActionTabClick(QTabWidget* pTab, AX_U32 nIndex, AX_S32 nWaitTick = 0, const string& strActionName = "", AX_BOOL bFeedbackRequired = AX_FALSE);
    virtual AX_VOID ActionScaleLabelDbClicked(ScaleLabel* pLabel, AX_S32 nWaitTick = 0, const string& strActionName = "", AX_BOOL bFeedbackRequired = AX_FALSE);
    virtual AX_VOID ActionCalendarDateSelected(QCalendarWidget* pCalendar, QDate& date, AX_S32 nWaitTick = 0, const string& strActionName = "", AX_BOOL bFeedbackRequired = AX_FALSE);
    virtual AX_BOOL EmitUIOprSignal(const TS_OPERATION_INFO_T& tOprInfo) = 0;

    AX_VOID RegisterWidget(const AX_CHAR* strName, QWidget* pWidget);
    QWidget* GetWidget(const AX_CHAR* strName);
    virtual AX_BOOL Enter();
    virtual AX_BOOL CheckEntry();
    virtual AX_VOID SetModuleConfig(const AX_NVR_TS_MODULE_INFO_T& tConfig);
    virtual AX_NVR_TS_MODULE_INFO_PTR GetModuleConfig();

    AX_VOID ResetAction(const string& strActionName, AX_BOOL bFeedbackRequired = AX_FALSE) {
        if (bFeedbackRequired) {
            m_bCaseResult = AX_FALSE;
        }

        m_strActionName = strActionName;
    }

    AX_BOOL IsFinished() {
        return m_bCaseResult;
    }

    AX_BOOL TryRecover() {
        QPushButton* pBtn = (QPushButton*)GetWidget("RecoverBtn_1");
        if (pBtn) {
            pBtn->clicked();
        }

        pBtn = (QPushButton*)GetWidget("RecoverBtn_2");
        if (pBtn) {
            pBtn->clicked();
        }

        return AX_TRUE;
    }

protected:
    AX_BOOL m_bRunning {AX_FALSE};
    AX_BOOL m_bCaseResult {AX_TRUE};
    string m_strModuleName {""};
    string m_strActionName {""};

private:
    std::map<std::string, QWidget*> m_mapWidgets;
    AX_NVR_TS_MODULE_INFO_T m_tModuleConfig;
};