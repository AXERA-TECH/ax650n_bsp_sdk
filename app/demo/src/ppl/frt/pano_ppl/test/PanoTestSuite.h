/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once
#include <vector>
#include "AXLockQ.hpp"
#include "AXSingleton.h"
#include "AXThread.hpp"
#include "PanoTestSuiteCfgParser.h"
#include "IPPLBuilder.h"
#include "WebOptionHelper.h"

class IPPLBuilder;

namespace AX_PANO {

class CTestSuite : public CAXSingleton<CTestSuite> {
    friend class CAXSingleton<CTestSuite>;

public:
    AX_VOID BindPPL(IPPLBuilder* pPPLBuilder) {
        m_pPPLBuilder = pPPLBuilder;
    };

    AX_BOOL Init();
    AX_BOOL UnInit();
    AX_BOOL Start();
    AX_BOOL Stop();

    AX_VOID InitCameraAttr(AX_U8 nSnsID, AX_U8 nSnsType, WEB_CAMERA_ATTR_T& tCameraAttr);
    AX_VOID InitVideoAttr(AX_U8 nSnsID, AX_U8 nChnID, WEB_VIDEO_ATTR_T& tVideoAttr);
    AX_VOID InitAiAttr(AX_U8 nSnsID);
    AX_VOID InitIvesAttr(AX_U8 nSnsID);

private:
    CTestSuite() = default;
    ~CTestSuite() = default;
    AX_VOID RunTest(AX_VOID* pArg);
    AX_VOID InitOsdAttr();

private:
    AX_BOOL m_bInitState{AX_FALSE};
    CAXThread m_UtThread;
    IPPLBuilder* m_pPPLBuilder{nullptr};
    std::vector<WEB_REQ_OPERATION_T> m_vecTestCase;
    APP_TEST_SUITE_CONFIG_T m_tTestCfg;
};

}  // namespace AX_PANO