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

#include "ts_base.hpp"
#include "global/UiGlobalDef.h"
#include "AXEvent.hpp"

#define MAX_PLAYBACK_RANDOM_ACTION_COUNT (10)


class CTestPlayback: public QObject, public CTestSuiteBase
{
    Q_OBJECT

public:
    CTestPlayback();
    virtual ~CTestPlayback() = default;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL EmitUIOprSignal(const TS_OPERATION_INFO_T& tOprInfo) override;

public slots:
    void OnRecvResult(const AX_NVR_ACTION_RES_T& tActionResult);

public slots:
    AX_BOOL initTestCase();
    void cleanupTestCase();
    AX_BOOL init();
    void testSingleStreamPlayProcess();
    void testMultiStreamPlayProcess();

    void testRandom();

signals:
    void tableCellClicked(int nRow, int nCol);
    void timeLocate(int nHHMM);
    void signal_ts_widget_opr(const TS_OPERATION_INFO_T& tOprInfo);

private:
    string GetActionDesc(PLAYBACK_ACTION_TYPE eType);
    AX_BOOL GetCaseInfo(const string& strCaseName, vector<AX_NVR_TS_CASE_INFO_T>& vecCaseInfo, AX_NVR_TS_CASE_INFO_T& tOutInfo);
    CTupleAction RandomAction(AX_U32 nRangeMax = MAX_PLAYBACK_RANDOM_ACTION_COUNT);
    void SelectDate(const string& strDate);
    void TabChangeAndSelect(AX_U32 nTabIndex, AX_BOOL bSelectAll, AX_S32 nWaitTick = 1000);
    void ActionMinMaxLabel(AX_S32 nWaitTick = 1000);
    void RandomControl(AX_U32 nTabIndex, AX_U32 nRoundCount);
    void SaveCtrlButtons();
    QPushButton* FindActionBtn(const string& strBtnName);
    void LocateTime(AX_U32 nTimeHHMM);

private:
    CAXEvent m_hActionvent;
    std::map<std::string, QPushButton*> m_mapActionButtons;
};