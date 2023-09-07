/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#pragma once
#include <vector>
#include "AXLockQ.hpp"
#include "AXSingleton.h"
#include "AXThread.hpp"
#include "IPPLBuilder.h"
#include "ITSBuilder.h"
#include "ITSTestSuiteCfgParser.h"
#include "WebOptionHelper.h"

class IPPLBuilder;

namespace AX_ITS {

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

    AX_BOOL IsEnabled() {
        return m_bEnabled;
    }
    AX_VOID InitCameraAttr(AX_U8 nSnsID, AX_U8 nSnsType, WEB_CAMERA_ATTR_T& tCameraAttr);
    AX_VOID InitVideoAttr(AX_U8 nSnsID, AX_U8 nChnID, WEB_VIDEO_ATTR_T& tVideoAttr);
    AX_VOID InitAiAttr(AX_U8 nSnsID);
    AX_VOID InitIvesAttr(AX_U8 nSnsID);

private:
    CTestSuite() = default;
    ~CTestSuite() = default;
    AX_VOID RunTest(AX_VOID* pArg);
    AX_VOID PrintCaseParam(TESTSUITE_OPERATION_T& opera);
    AX_VOID SortCases(std::vector<TESTSUITE_OPERATION_T>& vecWebRequests);
    AX_VOID UpdateWebAttr(TESTSUITE_OPERATION_T& opera);
    AX_VOID InitOsdAttr();

    AX_BOOL SkipOpera(TESTSUITE_OPERATION_T oper);
    AX_VOID SaveOpera(TESTSUITE_OPERATION_T oper);

private:
    enum TESTSUITE_RC_TYPE { CBR, VBR, FIXQP };

    AX_BOOL m_bEnabled{AX_FALSE};
    AX_BOOL m_bInitState{AX_FALSE};
    CAXThread m_UtThread;
    IPPLBuilder* m_pPPLBuilder{nullptr};
    std::vector<TESTSUITE_OPERATION_T> m_vecTestCase;
    APP_TEST_SUITE_CONFIG_T m_tTestCfg;
    std::map<AX_U8, WEB_CAMERA_ATTR_T> m_mapSns2CameraSetting;
    std::map<AX_U8, std::map<AX_U8, WEB_VIDEO_ATTR_T>> m_mapSns2VideoAttr;
    std::map<AX_U8, AI_ATTR_T> m_mapSns2AiAttr;
    std::map<AX_U8, OSD_SENSOR_CONFIG_T> m_mapSns2OsdConfig;
    WEB_CAMERA_ATTR_T* m_pCurCameraAttr{nullptr};
    OSD_SENSOR_CONFIG_T* m_pCurOsdAttr{nullptr};
    WEB_VIDEO_ATTR_T* m_pCurVideoAttr{nullptr};
    AI_ATTR_T* m_pCurAiAttr{nullptr};
};

}  // namespace AX_ITS