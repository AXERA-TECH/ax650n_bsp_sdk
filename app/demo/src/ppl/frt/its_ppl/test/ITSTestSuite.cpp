/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#include "ITSTestSuite.h"
#include <algorithm>
#include "AXTypeConverter.hpp"
#include "AppLog.hpp"
#include "CmdLineParser.h"
#include "ElapsedTimer.hpp"
#include "OptionHelper.h"
#include "WebOptionHelper.h"
#include "AlgoOptionHelper.h"

#define INTERVAL_MS_MIN 1000
#define TESTSUITE "TestSuite"
using namespace AX_ITS;

AX_VOID CTestSuite::RunTest(AX_VOID* pArg) {
    LOG_MM_C(TESTSUITE, "+++");
    AX_S64 const nLoop = m_tTestCfg.nLoopNum;
    AX_U32 const nDefIntervalMs = m_tTestCfg.nDefIntervalMs < INTERVAL_MS_MIN ? INTERVAL_MS_MIN : m_tTestCfg.nDefIntervalMs;
    AX_S64 cnt = 0;
    AX_ULONG nIndex;
    AX_U32 vecSize = m_vecTestCase.size();
    while (m_UtThread.IsRunning()) {
        std::vector<TESTSUITE_OPERATION_T> vecReq = m_vecTestCase;
        TESTSUITE_OPERATION_T reqOper;
        if (!m_tTestCfg.bRandomEnable) {
            for (nIndex = 0; nIndex < vecSize; nIndex++) {
                reqOper = vecReq[nIndex];
                if (!SkipOpera(reqOper)) {
                    PrintCaseParam(reqOper);
                    if (m_pPPLBuilder->ProcessTestSuiteOpers(reqOper)) {
                        SaveOpera(reqOper);
                    } else {
                        LOG_MM_E(TESTSUITE, "m_pPPLBuilder->ProcessTestSuiteOpers Failed.");
                    }
                    if (reqOper.nIntervalMs > 0) {
                        CElapsedTimer::mSleep(reqOper.nIntervalMs);
                    } else {
                        CElapsedTimer::mSleep(nDefIntervalMs);
                    }
                    if (!m_UtThread.IsRunning()) {
                        break;
                    }
                }
            }
        } else {
            srand(unsigned(time(0)));
            random_shuffle(vecReq.begin(), vecReq.end());
            for (nIndex = 0; nIndex < vecSize; nIndex++) {
                reqOper = vecReq.back();
                if (!SkipOpera(reqOper)) {
                    PrintCaseParam(reqOper);
                    if (m_pPPLBuilder->ProcessTestSuiteOpers(reqOper)) {
                        SaveOpera(reqOper);
                    } else {
                        LOG_MM_E(TESTSUITE, "m_pPPLBuilder->ProcessTestSuiteOpers Failed.");
                    }
                    if (reqOper.nIntervalMs > 0) {
                        CElapsedTimer::mSleep(reqOper.nIntervalMs);

                    } else {
                        CElapsedTimer::mSleep(nDefIntervalMs);
                    }
                    if (!m_UtThread.IsRunning()) {
                        break;
                    }
                }
                vecReq.pop_back();
            }
        }
        cnt++;
        LOG_MM_C(TESTSUITE, "Executed %d times.", cnt);
        if (nLoop >= 0 && cnt >= nLoop) {
            break;
        }
    }

    LOG_MM_C(TESTSUITE, "---");
}

AX_VOID CTestSuite::SaveOpera(TESTSUITE_OPERATION_T oper) {
    LOG_MM_I(TESTSUITE, "+++");
    UpdateWebAttr(oper);
    switch (oper.eReqType) {
        case E_REQ_TYPE_VIDEO:
            if (E_WEB_OPERATION_TYPE_CHANNEL_SWITCH == oper.GetOperationType()) {
                m_pCurVideoAttr->bEnable = oper.tChnSwitch.bOn;
                m_pCurVideoAttr->nEncoderType = oper.tChnSwitch.nEncoderType;
            } else if (E_WEB_OPERATION_TYPE_BITRATE == oper.GetOperationType()) {
                m_pCurVideoAttr->nBitrate = oper.tBitrate.nBitrate;
                m_pCurVideoAttr->nEncoderType = oper.tBitrate.nEncoderType;
            } else if (E_WEB_OPERATION_TYPE_RESOLUTION == oper.GetOperationType()) {
                m_pCurVideoAttr->nWidth = oper.tResolution.nWidth;
                m_pCurVideoAttr->nHeight = oper.tResolution.nHeight;
                m_pCurVideoAttr->nEncoderType = oper.tResolution.nEncoderType;
            } else if (E_WEB_OPERATION_TYPE_RC_INFO == oper.GetOperationType()) {
                m_pCurVideoAttr->nRcType = CAXTypeConverter::RcMode2Int(oper.tRcInfo.eRcType);
                m_pCurVideoAttr->nEncoderType = oper.tRcInfo.nEncoderType;
            } else if (E_WEB_OPERATION_TYPE_VENC_LINK_ENABLE == oper.GetOperationType()) {
                m_pCurVideoAttr->bLink = oper.tVencLinkEnable.bLinkEnable;
            } else if (E_WEB_OPERATION_TYPE_ENC_TYPE == oper.GetOperationType()) {
                m_pCurVideoAttr->nEncoderType = oper.tEncType.nEncoderType;
            }

            break;
        case E_REQ_TYPE_CAMERA:
            if (E_WEB_OPERATION_TYPE_CAPTURE_AUTO == oper.GetOperationType()) {
                m_pCurCameraAttr->bCaptureEnable = oper.tCapEnable.bOn;
            } else if (E_WEB_OPERATION_TYPE_DAYNIGHT == oper.GetOperationType()) {
                m_pCurCameraAttr->nDayNightMode = oper.tDaynight.nDayNightMode;
            } else if (E_WEB_OPERATION_TYPE_CAMERA_FPS == oper.GetOperationType()) {
                m_pCurCameraAttr->fFramerate = oper.tSnsFpsAttr.fSnsFrameRate;
            }
            break;
        case E_REQ_TYPE_AI:
            if (E_WEB_OPERATION_TYPE_AI_ENABLE == oper.GetOperationType()) {
                m_pCurAiAttr->bEnable = oper.tAiEnable.bOn;
            } else if (E_WEB_OPERATION_TYPE_AI_PUSH_MODE == oper.GetOperationType()) {
                m_pCurAiAttr->tPushStategy = oper.tAiPushStategy;
            } else if (E_WEB_OPERATION_TYPE_AI_EVENT == oper.GetOperationType()) {
                m_pCurAiAttr->tEvents = oper.tEvent;
            }
            break;
        case E_REQ_TYPE_OSD:
            if (E_WEB_OPERATION_TYPE_OSD_ENABLE == oper.GetOperationType()) {
                m_pCurOsdAttr->bEnable = oper.tOsdEnable.bOn;
            }
            break;
        default:
            break;
    }
    LOG_MM_I(TESTSUITE, "---");
}

/**
 * Skip operation when the following occurs
 * 1. The value of operation is equal to the current value
 * 2. when eReqType is E_REQ_TYPE_VIDEO, the value of opeation's encode type unequal current encode type.
 *    And the channel for opeation is closed
 */
AX_BOOL CTestSuite::SkipOpera(TESTSUITE_OPERATION_T oper) {
    LOG_MM_I(TESTSUITE, "+++");

    AX_BOOL ret = AX_FALSE;
    AX_U32 nSnsID = oper.nSnsID;
    AX_U32 nChn;
    switch (oper.eReqType) {
        case E_REQ_TYPE_VIDEO:
            nChn = oper.nChannel;
            m_pCurVideoAttr = &m_mapSns2VideoAttr[nSnsID][nChn];
            if (E_WEB_OPERATION_TYPE_CHANNEL_SWITCH == oper.GetOperationType()) {
                if (m_pCurVideoAttr->nEncoderType != oper.tChnSwitch.nEncoderType || m_pCurVideoAttr->bEnable == oper.tChnSwitch.bOn) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_BITRATE == oper.GetOperationType()) {
                AX_VENC_RC_MODE_E eRcMode = CAXTypeConverter::FormatRcMode(m_pCurVideoAttr->nEncoderType, m_pCurVideoAttr->nRcType);
                if (AX_FALSE == m_pCurVideoAttr->bEnable || m_pCurVideoAttr->nEncoderType != oper.tBitrate.nEncoderType ||
                    m_pCurVideoAttr->nBitrate == oper.tBitrate.nBitrate ||
                    (AX_VENC_RC_MODE_H264FIXQP == eRcMode || AX_VENC_RC_MODE_H265FIXQP == eRcMode ||
                     AX_VENC_RC_MODE_MJPEGFIXQP == eRcMode)) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_RESOLUTION == oper.GetOperationType()) {
                if (AX_FALSE == m_pCurVideoAttr->bEnable || m_pCurVideoAttr->nEncoderType != oper.tResolution.nEncoderType ||
                    (m_pCurVideoAttr->nWidth == oper.tResolution.nWidth && m_pCurVideoAttr->nHeight == oper.tResolution.nHeight)) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_RC_INFO == oper.GetOperationType()) {
                RC_INFO_T tRcInfo;
                m_pCurVideoAttr->GetEncRcCfg(tRcInfo);
                if (AX_FALSE == m_pCurVideoAttr->bEnable || m_pCurVideoAttr->nEncoderType != oper.tRcInfo.nEncoderType ||
                    (m_pCurVideoAttr->nRcType == CAXTypeConverter::RcMode2Int(oper.tRcInfo.eRcType) &&
                     tRcInfo.nMaxIProp == oper.tRcInfo.nMaxIProp && tRcInfo.nMinIProp == oper.tRcInfo.nMinIProp)) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_VENC_LINK_ENABLE == oper.GetOperationType()) {
                if (m_pCurVideoAttr->bLink == oper.tVencLinkEnable.bLinkEnable) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_ENC_TYPE == oper.GetOperationType()) {
                if (AX_FALSE == m_pCurVideoAttr->bEnable || m_pCurVideoAttr->nEncoderType == oper.tEncType.nEncoderType) {
                    ret = AX_TRUE;
                }
            }

            break;
        case E_REQ_TYPE_CAMERA:
            m_pCurCameraAttr = &m_mapSns2CameraSetting[nSnsID];
            if (E_WEB_OPERATION_TYPE_CAPTURE_AUTO == oper.GetOperationType()) {
                if (m_pCurCameraAttr->bCaptureEnable == oper.tCapEnable.bOn) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_DAYNIGHT == oper.GetOperationType()) {
                if (m_pCurCameraAttr->nDayNightMode == oper.tDaynight.nDayNightMode) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_CAMERA_FPS == oper.GetOperationType()) {
                if (m_pCurCameraAttr->fFramerate == oper.tSnsFpsAttr.fSnsFrameRate) {
                    ret = AX_TRUE;
                }
            }
            break;
        case E_REQ_TYPE_AI:
            m_pCurAiAttr = &m_mapSns2AiAttr[nSnsID];
            if (E_WEB_OPERATION_TYPE_AI_ENABLE == oper.GetOperationType()) {
                if (m_pCurAiAttr->bEnable == oper.tAiEnable.bOn) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_AI_PUSH_MODE == oper.GetOperationType()) {
                if (m_pCurAiAttr->tPushStategy.ePushMode == oper.tAiPushStategy.ePushMode &&
                    m_pCurAiAttr->tPushStategy.nPushCounts == oper.tAiPushStategy.nPushCounts &&
                    m_pCurAiAttr->tPushStategy.nPushIntervalMs == oper.tAiPushStategy.nPushIntervalMs) {
                    ret = AX_TRUE;
                }
            } else if (E_WEB_OPERATION_TYPE_AI_EVENT == oper.GetOperationType()) {
                if (m_pCurAiAttr->tEvents.tMD.bEnable == oper.tEvent.tMD.bEnable &&
                    m_pCurAiAttr->tEvents.tMD.nThrsHoldY == oper.tEvent.tMD.nThrsHoldY &&
                    m_pCurAiAttr->tEvents.tMD.nConfidence == oper.tEvent.tMD.nConfidence &&
                    m_pCurAiAttr->tEvents.tOD.bEnable == oper.tEvent.tOD.bEnable &&
                    m_pCurAiAttr->tEvents.tOD.nThrsHoldY == oper.tEvent.tOD.nThrsHoldY &&
                    m_pCurAiAttr->tEvents.tOD.nConfidence == oper.tEvent.tOD.nConfidence &&
                    m_pCurAiAttr->tEvents.tSCD.bEnable == oper.tEvent.tSCD.bEnable &&
                    m_pCurAiAttr->tEvents.tSCD.nThrsHoldY == oper.tEvent.tSCD.nThrsHoldY &&
                    m_pCurAiAttr->tEvents.tSCD.nConfidence == oper.tEvent.tSCD.nConfidence) {
                    ret = AX_TRUE;
                }
            }
            break;
        case E_REQ_TYPE_OSD:
            m_pCurOsdAttr = &m_mapSns2OsdConfig[nSnsID];
            if (E_WEB_OPERATION_TYPE_OSD_ENABLE == oper.GetOperationType()) {
                if (m_pCurOsdAttr->bEnable == oper.tOsdEnable.bOn) {
                    ret = AX_TRUE;
                }
            }
            break;
        default:
            break;
    }
    return ret;
}

AX_VOID CTestSuite::SortCases(std::vector<TESTSUITE_OPERATION_T>& vecWebRequests) {
    std::sort(vecWebRequests.begin(), vecWebRequests.end(),
              [](TESTSUITE_OPERATION_T t1, TESTSUITE_OPERATION_T t2) { return t1.nPriority > t2.nPriority; });
}

AX_VOID CTestSuite::UpdateWebAttr(TESTSUITE_OPERATION_T& opera) {
    if (E_WEB_OPERATION_TYPE_RESOLUTION == opera.GetOperationType()) {
        WEB_VIDEO_ATTR_T attr;
        attr.nWidth = opera.tResolution.nWidth;
        attr.nHeight = opera.tResolution.nHeight;
        AX_U8 nUniChn = opera.nChannel;
        LOG_MM_I(TESTSUITE, "nSnsID:%d, chn:%d, nUniChn:%d", opera.nSnsID, opera.nChannel, nUniChn);
        CWebOptionHelper::GetInstance()->SetVideoByUniChn(opera.nSnsID, attr);
    } else if (E_WEB_OPERATION_TYPE_AI_EVENT == opera.GetOperationType()) {
        if (-1 == opera.nSnsID) {
            CWebOptionHelper::GetInstance()->SetAiEvent(0, opera.tEvent);
            CWebOptionHelper::GetInstance()->SetAiEvent(1, opera.tEvent);
        } else {
            CWebOptionHelper::GetInstance()->SetAiEvent(opera.nSnsID, opera.tEvent);
        }
    }
}

AX_VOID CTestSuite::PrintCaseParam(TESTSUITE_OPERATION_T& opera) {
    LOG_MM_I(TESTSUITE, "+++");
    LOG_MM_I(TESTSUITE, "operaType:%d", opera.GetOperationType());
    RC_INFO_T tRcInfo;
    if (E_REQ_TYPE_VIDEO == opera.eReqType) {
        if (nullptr != m_pCurVideoAttr) {
            m_pCurVideoAttr->GetEncRcCfg(tRcInfo);
        } else {
            LOG_MM_E(TESTSUITE, "m_pCurVideoAttr is nullptr.");
            return;
        }
    }

    switch (opera.GetOperationType()) {
        case E_WEB_OPERATION_TYPE_ROTATION:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, capture_enable: (%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID,
                     m_pCurCameraAttr->nRotation, opera.tRotation.nRotation);
            break;
        case E_WEB_OPERATION_TYPE_SNS_MODE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, sensor mode: (%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID,
                     m_pCurCameraAttr->nSnsMode, opera.tSnsMode.nSnsMode);
            break;
        case E_WEB_OPERATION_TYPE_RESOLUTION:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d ,nChn:%d, encoderType:%d, resolution (%d*%d ---> %d*%d)]", opera.strDesc.c_str(),
                     opera.nSnsID, opera.nChannel, opera.tResolution.nEncoderType, m_pCurVideoAttr->nWidth, m_pCurVideoAttr->nHeight,
                     opera.tResolution.nWidth, opera.tResolution.nHeight);
            break;
        case E_WEB_OPERATION_TYPE_ENC_TYPE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d,nChn:%d, encoderType:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID, opera.nChannel,
                     m_pCurVideoAttr->nEncoderType, opera.tResolution.nEncoderType);
            break;
        case E_WEB_OPERATION_TYPE_BITRATE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, nChn:%d, encoderType:%d, nrcType:%d bitRate:(%d ---> %d)]", opera.strDesc.c_str(),
                     opera.nSnsID, opera.nChannel, opera.tBitrate.nEncoderType, m_pCurVideoAttr->nRcType, m_pCurVideoAttr->nBitrate,
                     opera.tBitrate.nBitrate);
            break;
        case E_WEB_OPERATION_TYPE_RC_INFO:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d ,nChn:%d, encoderType:%d, rcType(%d ---> %d)min_iprop/max_ipror(%d/%d ---> %d/%d)]",
                     opera.strDesc.c_str(), opera.nSnsID, opera.nChannel, opera.tRcInfo.nEncoderType, m_pCurVideoAttr->nRcType,
                     opera.tRcInfo.eRcType, tRcInfo.nMinIProp, tRcInfo.nMaxIProp, opera.tRcInfo.nMinIProp, opera.tRcInfo.nMaxIProp);
            break;
        case E_WEB_OPERATION_TYPE_DAYNIGHT:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, dayNightMode:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID,
                     m_pCurCameraAttr->nDayNightMode, opera.tDaynight.nDayNightMode);
            break;
        case E_WEB_OPERATION_TYPE_NR_MODE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, nrMode:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID, m_pCurCameraAttr->nNrMode,
                     opera.tNR.nNRMode);
            break;
        case E_WEB_OPERATION_TYPE_CAPTURE_AUTO:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, capture_enable:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID,
                     m_pCurCameraAttr->bCaptureEnable, opera.tCapEnable.bOn);
            break;
        case E_WEB_OPERATION_TYPE_AI_ENABLE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, ai_enable:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID, m_pCurAiAttr->bEnable,
                     opera.tAiEnable.bOn);
            break;
        case E_WEB_OPERATION_TYPE_AI_PUSH_MODE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, ai_push_mode,ai_push_count, ai_push_intervalMs (%d/%d/%d ---> %d/%d/%d)]",
                     opera.strDesc.c_str(), opera.nSnsID, m_pCurAiAttr->tPushStategy.ePushMode, m_pCurAiAttr->tPushStategy.nPushCounts,
                     m_pCurAiAttr->tPushStategy.nPushIntervalMs, opera.tAiPushStategy.ePushMode, opera.tAiPushStategy.nPushCounts,
                     opera.tAiPushStategy.nPushIntervalMs);
            break;
        case E_WEB_OPERATION_TYPE_AI_EVENT:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, ai_event_md_enable,ai_event_md_thresholdy,ai_event_md_confidence (%d/%d/%d ---> %d/%d/%d)]",
                     opera.strDesc.c_str(), opera.nSnsID, m_pCurAiAttr->tEvents.tMD.bEnable, m_pCurAiAttr->tEvents.tMD.nThrsHoldY,
                     m_pCurAiAttr->tEvents.tMD.nConfidence, opera.tEvent.tMD.bEnable, opera.tEvent.tMD.nThrsHoldY,
                     opera.tEvent.tMD.nConfidence);
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, ai_event_od_enable,ai_event_od_thresholdy,ai_event_od_confidence (%d/%d/%d ---> %d/%d/%d)]",
                     opera.strDesc.c_str(), opera.nSnsID, m_pCurAiAttr->tEvents.tOD.bEnable, m_pCurAiAttr->tEvents.tOD.nThrsHoldY,
                     m_pCurAiAttr->tEvents.tOD.nConfidence, opera.tEvent.tOD.bEnable, opera.tEvent.tOD.nThrsHoldY,
                     opera.tEvent.tOD.nConfidence);
            LOG_MM_C(TESTSUITE,
                     "%s: [SnsID:%d, ai_event_scd_enable,ai_event_scd_thresholdy,ai_event_scd_confidence (%d/%d/%d ---> %d/%d/%d)]",
                     opera.strDesc.c_str(), opera.nSnsID, m_pCurAiAttr->tEvents.tSCD.bEnable, m_pCurAiAttr->tEvents.tSCD.nThrsHoldY,
                     m_pCurAiAttr->tEvents.tSCD.nConfidence, opera.tEvent.tSCD.bEnable, opera.tEvent.tSCD.nThrsHoldY,
                     opera.tEvent.tSCD.nConfidence);
            break;
        case E_WEB_OPERATION_TYPE_OSD_ENABLE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, osd_enable:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID, m_pCurOsdAttr->bEnable,
                     opera.tOsdEnable.bOn);
            break;
        case E_WEB_OPERATION_TYPE_CHANNEL_SWITCH:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, nChn:%d tChnSwitch.bOn:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID, opera.nChannel,
                     m_pCurVideoAttr->bEnable, opera.tChnSwitch.bOn);
            break;
        case E_WEB_OPERATION_TYPE_VENC_LINK_ENABLE:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d, nChn:%d tVencLinkEnable.bLinkEnable:(%d ---> %d)]", opera.strDesc.c_str(), opera.nSnsID,
                     opera.nChannel, m_pCurVideoAttr->bLink, opera.tVencLinkEnable.bLinkEnable);
            break;
        case E_WEB_OPERATION_TYPE_CAMERA_FPS:
            LOG_MM_C(TESTSUITE, "%s: [SnsID:%d,  sensor frameRate:(%f ---> %f)]", opera.strDesc.c_str(), opera.nSnsID,
                     m_pCurCameraAttr->fFramerate, opera.tSnsFpsAttr.fSnsFrameRate);
            break;
        default:
            LOG_MM_C(TESTSUITE, "GetOperationType()%d: not support yet", opera.GetOperationType());
            break;
    }
}

AX_BOOL CTestSuite::Init() {
    AX_S32 nScenario;
    InitOsdAttr();
    CCmdLineParser::GetInstance()->GetScenario(nScenario);
    if (CTestSuiteCfgParser::GetInstance()->GetUTCase(nScenario, m_vecTestCase) &&
        CTestSuiteCfgParser::GetInstance()->GetTestAttr(nScenario, m_tTestCfg)) {
        m_bEnabled = AX_TRUE;
        return AX_TRUE;
    }
    return AX_FALSE;
}

AX_VOID CTestSuite::InitCameraAttr(AX_U8 nSnsID, AX_U8 nSnsType, WEB_CAMERA_ATTR_T& tCameraAttr) {
    m_mapSns2CameraSetting[nSnsID] = tCameraAttr;
}

AX_VOID CTestSuite::InitAiAttr(AX_U8 nSnsID) {
    m_mapSns2AiAttr[nSnsID].bEnable = AX_TRUE;
    m_mapSns2AiAttr[nSnsID].tPushStategy.ePushMode = (E_AI_DETECT_PUSH_MODE_TYPE)ALGO_HVCFP_PARAM(nSnsID).stPushStrategy.ePushMode;
    m_mapSns2AiAttr[nSnsID].tPushStategy.nPushCounts = ALGO_HVCFP_PARAM(nSnsID).stPushStrategy.nPushCount;
    m_mapSns2AiAttr[nSnsID].tPushStategy.nPushIntervalMs = ALGO_HVCFP_PARAM(nSnsID).stPushStrategy.nInterval;
}

AX_VOID CTestSuite::InitIvesAttr(AX_U8 nSnsID) {
    AX_U8 u8ThresHoldY{0};
    AX_U8 u8Confidence{0};
    AX_S32 s32ThresHoldY{0};
    AX_S32 s32Confidence{0};

    m_mapSns2AiAttr[nSnsID].tEvents.tOD.bEnable = CAlgoOptionHelper::GetInstance()->IsEnableOD(nSnsID);
    auto pODInstance =
        dynamic_cast<AX_ITS::CITSBuilder*>(CWebServer::GetInstance()->GetPPLInstance())->m_vecIvesInstance.at(0)->GetODInstance();
    if (nullptr == pODInstance) {
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY = 100;
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence = 80;
    } else {
        pODInstance->GetDefaultThresholdY(nSnsID, u8ThresHoldY, u8Confidence);
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY = u8ThresHoldY;
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence = u8Confidence;
    }

    m_mapSns2AiAttr[nSnsID].tEvents.tMD.bEnable = CAlgoOptionHelper::GetInstance()->IsEnableMD(nSnsID);
    auto pMDInstance =
        dynamic_cast<AX_ITS::CITSBuilder*>(CWebServer::GetInstance()->GetPPLInstance())->m_vecIvesInstance.at(0)->GetMDInstance();
    if (nullptr == pMDInstance) {
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY = 20;
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence = 50;
    } else {
        pMDInstance->GetDefaultThresholdY(nSnsID, u8ThresHoldY, u8Confidence);
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY = u8ThresHoldY;
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence = u8Confidence;
    }

    m_mapSns2AiAttr[nSnsID].tEvents.tSCD.bEnable = CAlgoOptionHelper::GetInstance()->IsEnableSCD(nSnsID);
    auto pSCDInstance =
        dynamic_cast<AX_ITS::CITSBuilder*>(CWebServer::GetInstance()->GetPPLInstance())->m_vecIvesInstance.at(0)->GetSCDInstance();
    if (nullptr == pSCDInstance) {
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY = 60;
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence = 60;
    } else {
        pSCDInstance->GetThreshold(nSnsID, s32ThresHoldY, s32Confidence);
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY = s32ThresHoldY;
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence = s32Confidence;
    }
}
AX_VOID CTestSuite::InitVideoAttr(AX_U8 nSnsID, AX_U8 nChnID, WEB_VIDEO_ATTR_T& tVideoAttr) {
    m_mapSns2VideoAttr[nSnsID][nChnID] = tVideoAttr;
}

AX_VOID CTestSuite::InitOsdAttr() {
    m_mapSns2OsdConfig[0].bEnable = COptionHelper::GetInstance()->IsEnableOSD();
    m_mapSns2OsdConfig[1].bEnable = COptionHelper::GetInstance()->IsEnableOSD();
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
