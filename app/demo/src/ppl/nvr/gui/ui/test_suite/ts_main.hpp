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


class CTestMain: public QObject, public CTestSuiteBase
{
    Q_OBJECT

public:
    CTestMain();
    virtual ~CTestMain() = default;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL EmitUIOprSignal(const TS_OPERATION_INFO_T& tOprInfo) override;

public slots:
    void OnRecvResult(const AX_NVR_ACTION_RES_T& tActionResult);

public slots:
    void testVersion();
    void testOpenPatrol();

signals:
    void signal_ts_widget_opr(const TS_OPERATION_INFO_T& tOprInfo);

private:
    string GetActionDesc(MAIN_ACTION_TYPE eType);
    AX_BOOL GetCaseInfo(const string& strCaseName, vector<AX_NVR_TS_CASE_INFO_T>& vecCaseInfo, AX_NVR_TS_CASE_INFO_T& tOutInfo);

};