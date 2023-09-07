/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "PanoTestSuite.h"
#include <algorithm>
#include "AppLog.hpp"
#include "CmdLineParser.h"
#include "ElapsedTimer.hpp"

#define INTERVAL_MS_MIN 1000
#define TESTSUITE "IPCTestSuite"

using namespace AX_PANO;

AX_VOID CTestSuite::RunTest(AX_VOID* pArg) {
    LOG_MM_I(TESTSUITE, "+++");
    AX_S64 const nLoop = m_tTestCfg.nLoopNum;
    AX_U32 const nDefIntervalMs = m_tTestCfg.nDefIntervalMs < INTERVAL_MS_MIN ? INTERVAL_MS_MIN : m_tTestCfg.nDefIntervalMs;
    AX_S64 cnt = 0;
    AX_ULONG nIndex;
    AX_U32 vecSize = m_vecTestCase.size();
    while (m_UtThread.IsRunning()) {
        std::vector<WEB_REQ_OPERATION_T> vecReq = m_vecTestCase;
        if (!m_tTestCfg.bRandomEnable) {
            for (nIndex = 0; nIndex < vecSize; nIndex++) {
                m_pPPLBuilder->ProcessTestSuiteOpers(vecReq[nIndex]);
                LOG_MM_C(TESTSUITE, "NormalModel ,[snsID:%d] %d done.", vecReq[nIndex].nGroup, vecReq[nIndex].GetOperationType());
                if (m_vecTestCase[nIndex].nIntervalMs > 0) {
                    CElapsedTimer::mSleep(m_vecTestCase[nIndex].nIntervalMs);
                } else {
                    CElapsedTimer::mSleep(nDefIntervalMs);
                }
            }
        } else {
            srand(unsigned(time(0)));
            random_shuffle(vecReq.begin(), vecReq.end());
            WEB_REQ_OPERATION_T reqOper;
            for (nIndex = 0; nIndex < vecSize; nIndex++) {
                m_pPPLBuilder->ProcessTestSuiteOpers(vecReq.back());
                vecReq.pop_back();
                LOG_MM_C(TESTSUITE, "RandomModel , %d done.", vecReq[nIndex].GetOperationType());
                if (m_vecTestCase[nIndex].nIntervalMs > 0) {
                    CElapsedTimer::mSleep(m_vecTestCase[nIndex].nIntervalMs);

                } else {
                    CElapsedTimer::mSleep(nDefIntervalMs);
                }
            }
        }
        if (nLoop >= 0 && ++cnt >= nLoop) {
            break;
        }
    }

    LOG_MM_I(TESTSUITE, "---");
}

AX_BOOL CTestSuite::Init() {
    InitOsdAttr();

    AX_S32 nScenario;
    CCmdLineParser::GetInstance()->GetScenario(nScenario);
    if (CTestSuiteCfgParser::GetInstance()->GetUTCase(nScenario, m_vecTestCase) &&
        CTestSuiteCfgParser::GetInstance()->GetTestAttr(nScenario, m_tTestCfg)) {
        return AX_TRUE;
    }
    return AX_FALSE;
}

AX_BOOL CTestSuite::Start() {
    if (!m_UtThread.Start(std::bind(&CTestSuite::RunTest, this, std::placeholders::_1), nullptr, "AppTestSuite")) {
        LOG_MM_E(TESTSUITE, "create UTThread failed");
        return AX_FALSE;
    }
    return AX_TRUE;
}

AX_BOOL CTestSuite::UnInit() {
    return AX_TRUE;
}
AX_BOOL CTestSuite::Stop() {
    if (m_UtThread.IsRunning()) {
        m_UtThread.Stop();
        m_UtThread.Join();
    }
    return AX_TRUE;
}

AX_VOID CTestSuite::InitCameraAttr(AX_U8 nSnsID, AX_U8 nSnsType, WEB_CAMERA_ATTR_T& tCameraAttr) {
    // AX620E TODO:
}

AX_VOID CTestSuite::InitAiAttr(AX_U8 nSnsID) {
    // AX620E TODO:
}

AX_VOID CTestSuite::InitIvesAttr(AX_U8 nSnsID) {
    // AX620E TODO:
}
AX_VOID CTestSuite::InitVideoAttr(AX_U8 nSnsID, AX_U8 nChnID, WEB_VIDEO_ATTR_T& tVideoAttr) {
    // AX620E TODO:
}

AX_VOID CTestSuite::InitOsdAttr() {
    // AX620E TODO:
}
