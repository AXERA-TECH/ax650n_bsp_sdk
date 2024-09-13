/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "Detector.hpp"
#include <string.h>
#include <chrono>
#include <exception>
#include "AXException.hpp"
#include "AppLogApi.h"
#include "DetectResult.hpp"
#if defined(__RECORD_VB_TIMESTAMP__)
#include "TimestampHelper.hpp"
#endif

using namespace std;
#define DETECTOR "SKEL"

static AX_VOID SkelResultCallback(AX_SKEL_HANDLE pHandle, AX_SKEL_RESULT_T *pstResult, AX_VOID *pUserData) {
    CDetector *pThis = (CDetector *)pUserData;
    if (!pThis) {
        THROW_AX_EXCEPTION("skel handle %p result callback user data is nil", pHandle);
    }

    SKEL_FRAME_PRIVATE_DATA_T *pPrivData = (SKEL_FRAME_PRIVATE_DATA_T *)(pstResult->pUserData);
    if (!pPrivData) {
        THROW_AX_EXCEPTION("skel handle %p frame private data is nil", pHandle);
    }

    DETECT_RESULT_T fhvp;
    // fhvp.nSeqNum = pstResult->nFrameId;
    fhvp.nW = pstResult->nOriginalWidth;
    fhvp.nH = pstResult->nOriginalHeight;
    fhvp.nSeqNum = pPrivData->nSeqNum;
    fhvp.nGrpId = pPrivData->nGrpId;
    fhvp.nCount = pstResult->nObjectSize;

    AX_U32 index = 0;
    for (AX_U32 i = 0; i < fhvp.nCount && index < MAX_DETECT_RESULT_COUNT; ++i) {
        if (pstResult->pstObjectItems[i].eTrackState != AX_SKEL_TRACK_STATUS_NEW &&
            pstResult->pstObjectItems[i].eTrackState != AX_SKEL_TRACK_STATUS_UPDATE) {
            continue;
        }

        if (0 == strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "body")) {
            fhvp.item[index].eType = DETECT_TYPE_BODY;
        } else if (0 == strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "vehicle")) {
            fhvp.item[index].eType = DETECT_TYPE_VEHICLE;
        } else if (0 == strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "cycle")) {
            fhvp.item[index].eType = DETECT_TYPE_CYCLE;
        } else if (0 == strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "face")) {
            fhvp.item[index].eType = DETECT_TYPE_FACE;
        } else if (0 == strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "plate")) {
            fhvp.item[index].eType = DETECT_TYPE_PLATE;
        } else {
            LOG_M_W(DETECTOR, "unknown detect result %s of vdGrp %d frame %lld (skel %lld)",
                    pstResult->pstObjectItems[i].pstrObjectCategory, fhvp.nGrpId, fhvp.nSeqNum, pstResult->nFrameId);
            fhvp.item[index].eType = DETECT_TYPE_UNKNOWN;
        }

        fhvp.item[index].nTrackId = pstResult->pstObjectItems[i].nTrackId;
        fhvp.item[index].tBox = pstResult->pstObjectItems[i].stRect;
        index++;
    }

    fhvp.nCount = index;

    /* save fhvp result */
    CDetectResult::GetInstance()->Set(pPrivData->nGrpId, fhvp);

    /* release fhvp result */
    AX_SKEL_Release((AX_VOID *)pstResult);

    /* giveback private data */
    pThis->ReleaseSkelPrivateData(pPrivData);
}

AX_VOID CDetector::RunDetect(AX_VOID *pArg) {
    LOG_M_C(DETECTOR, "detect thread is running");

    AX_U64 nFrameId = 0;
    AX_U32 nCurrGrp = 0;
    AX_U32 nNextGrp = 0;
    const AX_U32 TOTAL_GRP_COUNT = m_stAttr.nGrpCount;
    CAXFrame axFrame;
    AX_U32 nSkipCount = 0;
    while (m_DetectThread.IsRunning()) {
        for (nCurrGrp = nNextGrp; nCurrGrp < TOTAL_GRP_COUNT; ++nCurrGrp) {
            if (m_arrFrameQ[nCurrGrp].Pop(axFrame, 0)) {
                nSkipCount = 0;
                break;
            }

            if (++nSkipCount == TOTAL_GRP_COUNT) {
                this_thread::sleep_for(chrono::microseconds(1000));
                nSkipCount = 0;
            }
        }

        if (nCurrGrp == TOTAL_GRP_COUNT) {
            nNextGrp = 0;
            continue;
        } else {
            nNextGrp = nCurrGrp + 1;
            if (nNextGrp == TOTAL_GRP_COUNT) {
                nNextGrp = 0;
            }
        }

        SKEL_FRAME_PRIVATE_DATA_T *pPrivData = m_skelData.borrow();
        if (!pPrivData) {
            LOG_M_E(DETECTOR, "%s: borrow skel frame private data fail", __func__);
            axFrame.DecRef();
            continue;
        } else {
            pPrivData->nSeqNum = axFrame.stFrame.stVFrame.stVFrame.u64SeqNum;
            pPrivData->nGrpId = axFrame.nGrp;
            pPrivData->nSkelChn = axFrame.nGrp % m_stAttr.nChannelNum;
        }

        AX_SKEL_FRAME_T skelFrame;
        skelFrame.nFrameId = ++nFrameId; /* skel recommends to unique frame id */
        skelFrame.nStreamId = axFrame.nGrp;
        skelFrame.stFrame = axFrame.stFrame.stVFrame.stVFrame;
        skelFrame.pUserData = (AX_VOID *)pPrivData;
        LOG_M_N(DETECTOR, "runskel vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x", axFrame.nGrp, axFrame.nChn,
                axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
                axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
                axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
                axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);

#if defined(__RECORD_VB_TIMESTAMP__)
        CTimestampHelper::RecordTimestamp(axFrame.stFrame.stVFrame.stVFrame, axFrame.nGrp, axFrame.nChn, TIMESTAMP_SKEL_PRE_SEND);
#endif

#ifdef __BOX_DEBUG__
        AX_S32 ret = 0;
        usleep(3000);
        m_skelData.giveback(pPrivData);
#else
        AX_S32 ret = AX_SKEL_SendFrame(m_hSkel[pPrivData->nSkelChn], &skelFrame, -1);
#endif

#if defined(__RECORD_VB_TIMESTAMP__)
        CTimestampHelper::RecordTimestamp(axFrame.stFrame.stVFrame.stVFrame, axFrame.nGrp, axFrame.nChn, TIMESTAMP_SKEL_POS_SEND);
#endif

        /* release frame after done */
        axFrame.DecRef();

        if (0 != ret) {
            LOG_M_E(DETECTOR, "%s: AX_SKEL_SendFrame(vdGrp %d, seq %lld, frame %lld) fail, ret = 0x%x", __func__, axFrame.nGrp,
                    axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, skelFrame.nFrameId, ret);

            m_skelData.giveback(pPrivData);
        }
    }

    LOG_M_D(DETECTOR, "%s: ---", __func__);
}

AX_BOOL CDetector::Init(const DETECTOR_ATTR_T &stAttr) {
    LOG_M_D(DETECTOR, "%s: +++", __func__);

    if (0 == stAttr.nGrpCount) {
        LOG_M_E(DETECTOR, "%s: 0 grp", __func__);
        return AX_FALSE;
    }

    m_stAttr = stAttr;

    if (m_stAttr.nSkipRate <= 0) {
        m_stAttr.nSkipRate = 1;
    }

    /* define how many frames can skel to handle in parallel */
    constexpr AX_U32 PARALLEL_FRAME_COUNT = 2;
    m_skelData.reserve(stAttr.nGrpCount * PARALLEL_FRAME_COUNT);

    m_arrFrameQ = new (nothrow) CAXLockQ<CAXFrame>[stAttr.nGrpCount];
    if (!m_arrFrameQ) {
        LOG_M_E(DETECTOR, "%s: alloc queue fail", __func__);
        return AX_FALSE;
    } else {
        for (AX_U32 i = 0; i < stAttr.nGrpCount; ++i) {
            m_arrFrameQ[i].SetCapacity(-1);
        }
    }

    /* [2]: SKEL init */
    AX_SKEL_INIT_PARAM_T stInit;
    memset(&stInit, 0, sizeof(stInit));
    stInit.pStrModelDeploymentPath = m_stAttr.strModelPath.c_str();
    AX_S32 ret = AX_SKEL_Init(&stInit);
    if (0 != ret) {
        LOG_M_E(DETECTOR, "%s: AX_SKEL_Init fail, ret = 0x%x", __func__, ret);

        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
        return AX_FALSE;
    }

    do {
        /* [3]: print SKEL version */
        const AX_SKEL_VERSION_INFO_T *pstVersion = NULL;
        ret = AX_SKEL_GetVersion(&pstVersion);
        if (0 != ret) {
            LOG_M_E(DETECTOR, "%s: AX_SKEL_GetVersion() fail, ret = 0x%x", __func__, ret);
        } else {
            if (pstVersion && pstVersion->pstrVersion) {
                LOG_M_I(DETECTOR, "SKEL version: %s", pstVersion->pstrVersion);
            }

            AX_SKEL_Release((AX_VOID *)pstVersion);
        }

        /* [4]: check whether has FHVP model or not */
        const AX_SKEL_CAPABILITY_T *pstCapability = NULL;
        ret = AX_SKEL_GetCapability(&pstCapability);
        if (0 != ret) {
            LOG_M_E(DETECTOR, "%s: AX_SKEL_GetCapability() fail, ret = 0x%x", __func__, ret);
            break;
        } else {
            AX_BOOL bFHVP{AX_FALSE};
            if (pstCapability && 0 == pstCapability->nPPLConfigSize) {
                LOG_M_E(DETECTOR, "%s: SKEL model has 0 PPL", __func__);
            } else {
                for (AX_U32 i = 0; i < pstCapability->nPPLConfigSize; ++i) {
                    if (AX_SKEL_PPL_HVCFP == pstCapability->pstPPLConfig[i].ePPL) {
                        bFHVP = AX_TRUE;
                        break;
                    }
                }
            }

            AX_SKEL_Release((AX_VOID *)pstCapability);
            if (!bFHVP) {
                LOG_M_E(DETECTOR, "%s: SKEL not found FHVP model", __func__);
                break;
            }
        }

        for (AX_U32 nChn = 0; nChn < m_stAttr.nChannelNum; ++nChn) {
            /* [5]: create SEKL handle */
            AX_SKEL_HANDLE_PARAM_T stHandleParam;
            memset(&stHandleParam, 0, sizeof(stHandleParam));
            stHandleParam.ePPL = (AX_SKEL_PPL_E)m_stAttr.tChnAttr[nChn].nPPL;
            stHandleParam.nFrameDepth = m_stAttr.nDepth;
            stHandleParam.nFrameCacheDepth = 0;
            stHandleParam.nIoDepth = 0;
            stHandleParam.nWidth = m_stAttr.nW;
            stHandleParam.nHeight = m_stAttr.nH;
            if (m_stAttr.tChnAttr[nChn].nVNPU == AX_SKEL_NPU_DEFAULT) {
                stHandleParam.nNpuType = AX_SKEL_NPU_DEFAULT;
            } else {
                stHandleParam.nNpuType = (AX_U32)(1 << (m_stAttr.tChnAttr[nChn].nVNPU - 1));
            }

            AX_SKEL_CONFIG_T stConfig = {0};
            AX_SKEL_CONFIG_ITEM_T stItems[16] = {0};
            AX_U8 itemIndex = 0;
            stConfig.nSize = 0;
            stConfig.pstItems = &stItems[0];

            if (!m_stAttr.tChnAttr[nChn].bTrackEnable) {
                // track_disable
                stConfig.pstItems[itemIndex].pstrType = (AX_CHAR *)"track_disable";
                AX_SKEL_COMMON_THRESHOLD_CONFIG_T stTrackDisableThreshold = {0};
                stTrackDisableThreshold.fValue = 1;
                stConfig.pstItems[itemIndex].pstrValue = (AX_VOID *)&stTrackDisableThreshold;
                stConfig.pstItems[itemIndex].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
                itemIndex++;
            }

            // push_disable
            stConfig.pstItems[itemIndex].pstrType = (AX_CHAR *)"push_disable";
            AX_SKEL_COMMON_THRESHOLD_CONFIG_T stPushDisableThreshold = {0};
            stPushDisableThreshold.fValue = 1;
            stConfig.pstItems[itemIndex].pstrValue = (AX_VOID *)&stPushDisableThreshold;
            stConfig.pstItems[itemIndex].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
            itemIndex++;

            stConfig.nSize = itemIndex;
            stHandleParam.stConfig = stConfig;

            LOG_M_C(DETECTOR, "ppl %d, depth %d, cache depth %d, %dx%d", stHandleParam.ePPL, stHandleParam.nFrameDepth,
                    stHandleParam.nFrameCacheDepth, stHandleParam.nWidth, stHandleParam.nHeight);
            ret = AX_SKEL_Create(&stHandleParam, &m_hSkel[nChn]);

            if (0 != ret || NULL == m_hSkel[nChn]) {
                LOG_M_E(DETECTOR, "%s: AX_SKEL_Create() fail, ret = 0x%x", __func__, ret);
                break;
            }

            /* [6]: register result callback */
            ret = AX_SKEL_RegisterResultCallback(m_hSkel[nChn], SkelResultCallback, this);
            if (0 != ret) {
                LOG_M_E(DETECTOR, "%s: AX_SKEL_RegisterResultCallback() fail, ret = 0x%x", __func__, ret);
                break;
            }
        }

        LOG_M_D(DETECTOR, "%s: ---", __func__);
        return AX_TRUE;

    } while (0);

    DeInit();
    LOG_M_D(DETECTOR, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CDetector::DeInit(AX_VOID) {
    LOG_M_D(DETECTOR, "%s: +++", __func__);

    AX_S32 ret;
    if (m_DetectThread.IsRunning()) {
        LOG_M_E(DETECTOR, "%s: detect thread is running", __func__);
        return AX_FALSE;
    }

    AX_U64 nBodyCnt = CDetectResult::GetInstance()->GetTotalCount(DETECT_TYPE_BODY);
    AX_U64 nVehiCnt = CDetectResult::GetInstance()->GetTotalCount(DETECT_TYPE_VEHICLE);
    AX_U64 nPlatCnt = CDetectResult::GetInstance()->GetTotalCount(DETECT_TYPE_PLATE);
    AX_U64 nCyclCnt = CDetectResult::GetInstance()->GetTotalCount(DETECT_TYPE_CYCLE);
    if (0 == nBodyCnt && 0 == nVehiCnt && 0 == nPlatCnt && 0 == nCyclCnt) {
        /* report "0 fhvp result" for UT case */
        LOG_M_C(DETECTOR, "0 fhvp result is detected");
    } else {
        LOG_M_C(DETECTOR, "total detect result => body = %lld, vehicle = %lld, plate = %lld, cycle = %d", nBodyCnt, nVehiCnt, nPlatCnt,
                nCyclCnt);
    }

    for (AX_U32 nChn = 0; nChn < m_stAttr.nChannelNum; ++nChn) {
        if (m_hSkel[nChn]) {
            ret = AX_SKEL_Destroy(m_hSkel[nChn]);
            if (0 != ret) {
                LOG_M_E(DETECTOR, "%s: AX_SKEL_Destroy() fail, ret = 0x%x", __func__, ret);
                return AX_FALSE;
            }
        }
    }

    ret = AX_SKEL_DeInit();
    if (0 != ret) {
        LOG_M_E(DETECTOR, "%s: AX_SKEL_DeInit() fail, ret = 0x%x", __func__, ret);
        return AX_FALSE;
    }

    if (m_arrFrameQ) {
        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
    }

    LOG_M_D(DETECTOR, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CDetector::Start(AX_VOID) {
    LOG_M_D(DETECTOR, "%s: +++", __func__);

    if (!m_DetectThread.Start([this](AX_VOID *pArg) -> AX_VOID { RunDetect(pArg); }, this, "AppDetect", SCHED_FIFO, 99)) {
        LOG_M_E(DETECTOR, "%s: create detect thread fail", __func__);
        return AX_FALSE;
    }
    LOG_M_D(DETECTOR, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CDetector::Clear(AX_VOID) {
    if (m_arrFrameQ) {
        for (AX_U32 i = 0; i < m_stAttr.nGrpCount; ++i) {
            ClearQueue(i);
        }
    }

    return AX_TRUE;
}

AX_BOOL CDetector::Stop(AX_VOID) {
    LOG_M_D(DETECTOR, "%s: +++", __func__);

    m_DetectThread.Stop();

    if (m_arrFrameQ) {
        for (AX_U32 i = 0; i < m_stAttr.nGrpCount; ++i) {
            m_arrFrameQ[i].Wakeup();
        }
    }

    m_DetectThread.Join();

    if (m_arrFrameQ) {
        for (AX_U32 i = 0; i < m_stAttr.nGrpCount; ++i) {
            ClearQueue(i);
        }
    }

    LOG_M_D(DETECTOR, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CDetector::SendFrame(const CAXFrame &axFrame) {
    LOG_M_I(DETECTOR, "recvfrm vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x", axFrame.nGrp, axFrame.nChn,
            axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
            axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
            axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
            axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);

    if (SkipFrame(axFrame)) {
        LOG_M_I(DETECTOR, "dropfrm vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x", axFrame.nGrp, axFrame.nChn,
                axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
                axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
                axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
                axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);
        return AX_TRUE;
    }

    axFrame.IncRef();

#if defined(__RECORD_VB_TIMESTAMP__)
    CTimestampHelper::RecordTimestamp(axFrame.stFrame.stVFrame.stVFrame, axFrame.nGrp, axFrame.nChn, TIMESTAMP_SKEL_PUSH_FIFO);
#endif

    if (!m_arrFrameQ[axFrame.nGrp].Push(axFrame)) {
        LOG_M_E(DETECTOR, "%s: push frame %lld to q fail", __func__, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum);
        axFrame.DecRef();
    }

    return AX_TRUE;
}

AX_BOOL CDetector::SkipFrame(const CAXFrame &axFrame) {
#ifdef __VDEC_PP_FRAME_CTRL__
    return AX_FALSE;
#else

    if (m_stAttr.nSkipRate <= 1) {
        return AX_FALSE;
    }

    return (1 == (axFrame.stFrame.stVFrame.stVFrame.u64SeqNum % m_stAttr.nSkipRate)) ? AX_FALSE : AX_TRUE;
#endif
}

AX_VOID CDetector::ClearQueue(AX_S32 nGrp) {
    AX_U32 nCount = m_arrFrameQ[nGrp].GetCount();
    if (nCount > 0) {
        CAXFrame axFrame;
        for (AX_U32 i = 0; i < nCount; ++i) {
            if (m_arrFrameQ[nGrp].Pop(axFrame, 0)) {
                axFrame.DecRef();
            }
        }
    }
}
