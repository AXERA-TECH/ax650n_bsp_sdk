/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "detector.hpp"
#include "AppLog.hpp"
#include "ElapsedTimer.hpp"
#include "ax_skel_type.h"

#define TAG "detector"

static AX_VOID SkelResultCallback(AX_SKEL_HANDLE pHandle, AX_SKEL_RESULT_T *pstResult, AX_VOID *pthis) {
    CDetector *pThis = (CDetector *)pthis;
    if (!pThis) {
        THROW_AX_EXCEPTION("skel handle %p result callback user data is nil", pHandle);
        AX_SKEL_Release((AX_VOID *)pstResult);
        return;
    }
    SKEL_FRAME_PRIVATE_DATA_T *pPrivData = (SKEL_FRAME_PRIVATE_DATA_T *)(pstResult->pUserData);
    if (!pPrivData) {
        THROW_AX_EXCEPTION("skel handle %p frame private data is nil", pHandle);
        AX_SKEL_Release((AX_VOID *)pstResult);
        return;
    }
    AX_U32 nObjectSize = 0;
    DETECT_RESULT_T fhvp;
    fhvp.nSeqNum = pstResult->nFrameId;
    fhvp.nW = pstResult->nOriginalWidth;
    fhvp.nH = pstResult->nOriginalHeight;
    fhvp.nSeqNum = pPrivData->nSeqNum;
    fhvp.nGrpId = pPrivData->nGrpId;

    nObjectSize = (pstResult->nObjectSize > MAX_DETECT_RESULT_COUNT) ? MAX_DETECT_RESULT_COUNT : pstResult->nObjectSize;
    AX_U32 index = 0;
    for (AX_U32 i = 0; i < nObjectSize; ++i) {
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
            LOG_M_W(TAG, "unknown detect result %s of vdGrp %d frame %lld (skel %lld)", pstResult->pstObjectItems[i].pstrObjectCategory,
                    fhvp.nGrpId, fhvp.nSeqNum, pstResult->nFrameId);
            fhvp.item[index].eType = DETECT_TYPE_UNKNOWN;
        }

        LOG_MM_D(TAG, "SkelResultCallback Grp:%d Rect: [x:%f, y:%f, w:%f, h:%f]", fhvp.nGrpId, pstResult->pstObjectItems[i].stRect.fX,
                 pstResult->pstObjectItems[i].stRect.fY, pstResult->pstObjectItems[i].stRect.fW, pstResult->pstObjectItems[i].stRect.fH);
        if ((pstResult->pstObjectItems[i].eTrackState == AX_SKEL_TRACK_STATUS_NEW ||
             pstResult->pstObjectItems[i].eTrackState == AX_SKEL_TRACK_STATUS_UPDATE) &&
            (pstResult->pstObjectItems[i].stRect.fW > 0) && (pstResult->pstObjectItems[i].stRect.fH > 0)) {
            fhvp.item[index].tBox = pstResult->pstObjectItems[i].stRect;
            index++;
        }
    }
    fhvp.nCount = index;
    /* notify result */
    pThis->NotifyAll(pPrivData->nGrpId, pPrivData->nChnId, fhvp);

    /* release fhvp result */
    AX_SKEL_Release((AX_VOID *)pstResult);

    /* giveback private data */
    pThis->ReleaseSkelPrivateData(pPrivData);
    return;
}

AX_VOID CDetector::ProcessFrame(AX_VOID * /*pArg*/) {
    LOG_MM_I(TAG, "+++");

    AX_U64 nFrameId = 0;
    AX_U8 nCurrGrp = 0;
    AX_U8 nNextGrp = 0;
    const AX_U8 nGrpCount = m_stAttr.nGrpCount;
    CAXFrame axFrame;
    AX_SKEL_FRAME_T skelFrame;

#if 0
    AX_U64 nStartMs = 0;
    AX_U64 nEndMs = 0;
    AX_U32 nFrameCnt = 0;
    AX_F64 fFps = 0.0;
    nStartMs = CElapsedTimer::GetInstance()->GetTickCount();
#endif
    while (m_DetectThread.IsRunning()) {
        for (nCurrGrp = nNextGrp; nCurrGrp < nGrpCount; ++nCurrGrp) {
            if (m_arrFrameQ[nCurrGrp].Pop(axFrame, 0)) {
                break;
            }
        }
        if (nCurrGrp >= nGrpCount) {
            nNextGrp = 0;
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        } else {
            nNextGrp = nCurrGrp + 1;
            if (nNextGrp >= nGrpCount) {
                nNextGrp = 0;
            }
        }

        SKEL_FRAME_PRIVATE_DATA_T *pPrivData = m_skelData.borrow();
        if (!pPrivData) {
            LOG_M_E(TAG, "%s: borrow skel frame private data fail", __func__);
            axFrame.DecRef();
            continue;
        } else {
            pPrivData->nSeqNum = axFrame.stFrame.stVFrame.stVFrame.u64SeqNum;
            pPrivData->nGrpId = nCurrGrp;
            pPrivData->nSkelChn = axFrame.nGrp % m_stAttr.nChannelNum;
        }
        skelFrame.nFrameId = ++nFrameId;
        skelFrame.nStreamId = nCurrGrp;
        skelFrame.stFrame = axFrame.stFrame.stVFrame.stVFrame;
        skelFrame.pUserData = (void *)pPrivData;
        AX_S32 ret = AX_SKEL_SendFrame(m_hSkel[pPrivData->nSkelChn], &skelFrame, -1);
        if (0 != ret) {
            LOG_MM_E(TAG,
                     "AX_SKEL_SendFrame skel[%d] fail,ret =0x%x, nCurrGrp:%d vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx "
                     "VirAddr:0x%llx %dx%d "
                     "stride %d blkId "
                     "0x%x, size:%u",
                     pPrivData->nSkelChn, ret, nCurrGrp, axFrame.nGrp, axFrame.nChn, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum,
                     axFrame.stFrame.stVFrame.stVFrame.u64PTS, axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0],
                     axFrame.stFrame.stVFrame.stVFrame.u64VirAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
                     axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
                     axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0], axFrame.stFrame.stVFrame.stVFrame.u32FrameSize);
        }

        axFrame.DecRef();

#if 0
        nFrameCnt++;
        nEndMs = CElapsedTimer::GetInstance()->GetTickCount();
        if ((nEndMs - nStartMs) >= 20 * 1000) {
            fFps = (1000 / ((AX_F64)(nEndMs - nStartMs) / nFrameCnt));
            LOG_MM_C(TAG, "took time:%ldms,nFrameCnt:%d,  fps:%lf, skelChannelNum:%d", nEndMs - nStartMs, nFrameCnt,
                     fFps / m_stAttr.nChannelNum, m_stAttr.nChannelNum);
            nStartMs = CElapsedTimer::GetInstance()->GetTickCount();
            nFrameCnt = 0;
        }
#endif
    }

    LOG_MM_I(TAG, "---");
}

AX_BOOL CDetector::Init(const DETECTOR_ATTR_T &stAttr) {
    LOG_MM_I(TAG, "+++");
    std::lock_guard<std::mutex> lock(m_mtxApi);

    if (0 == stAttr.nGrpCount || stAttr.nGrpCount > MAX_DETECTOR_GROUP_NUM) {
        LOG_MM_E(TAG, "invalid detect grp count %d", stAttr.nGrpCount);
        return AX_FALSE;
    }

    AX_BOOL res = AX_TRUE;
    m_arrFrameQ = new (std::nothrow) CAXLockQ<CAXFrame>[stAttr.nGrpCount];
    if (!m_arrFrameQ) {
        LOG_MM_E(TAG, "alloc queue fail");
        return AX_FALSE;
    } else {
        for (AX_U32 i = 0; i < stAttr.nGrpCount; ++i) {
            m_arrFrameQ[i].SetCapacity(10);
        }
    }

    m_stAttr = stAttr;

    if (m_stAttr.nSkipRate <= 0) {
        m_stAttr.nSkipRate = 2;
    }

    /* define how many frames can skel to handle in parallel */
    constexpr AX_U32 PARALLEL_FRAME_COUNT = 2;
    m_skelData.reserve(stAttr.nGrpCount * PARALLEL_FRAME_COUNT);

    /* [1]: SKEL init */
    AX_SKEL_INIT_PARAM_T stInit;
    memset(&stInit, 0, sizeof(stInit));
    stInit.pStrModelDeploymentPath = m_stAttr.strModelPath.c_str();

    AX_S32 ret = AX_SKEL_Init(&stInit);
    if (AX_SKEL_SUCC != ret) {
        LOG_M_E(TAG, "AX_SKEL_Init() fail, ret= 0x%x\n", ret);

        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;

        return AX_FALSE;
    }

    do {
        /* [2]: print SKEL version*/
        const AX_SKEL_VERSION_INFO_T *pstVersion{nullptr};
        ret = AX_SKEL_GetVersion(&pstVersion);
        if (AX_SKEL_SUCC != ret) {
            LOG_M_E(TAG, "AX_SKEL_GetVersion() fail, ret= 0x%x\n", ret);
        } else {
            if (pstVersion && pstVersion->pstrVersion) {
                LOG_MM_D(TAG, "SKEL version:%s", pstVersion->pstrVersion);
            }

            AX_SKEL_Release((AX_VOID *)pstVersion);
        }

        /* [3]: check SKEL model required */
        const AX_SKEL_CAPABILITY_T *pstCapability{nullptr};
        ret = AX_SKEL_GetCapability(&pstCapability);
        if (AX_SKEL_SUCC != ret) {
            LOG_M_E(TAG, "AX_SKEL_GetCapability() fail, ret= 0x%x\n", ret);
            res = AX_FALSE;
            break;
        } else {
            AX_BOOL bFHVP{AX_FALSE};
            for (AX_U32 i = 0; i < pstCapability->nPPLConfigSize; i++) {
                if (m_stAttr.ePPL == pstCapability->pstPPLConfig[i].ePPL) {
                    bFHVP = AX_TRUE;
                }
            }

            AX_SKEL_Release((AX_VOID *)pstCapability);
            if (!bFHVP) {
                LOG_M_E(TAG, "%s: SKEL not found FHVP model", __func__);
                res = AX_FALSE;
                break;
            }
        }

        AX_U32 nNum = (m_stAttr.nChannelNum > DETECTOR_MAX_CHN_NUM) ? DETECTOR_MAX_CHN_NUM : m_stAttr.nChannelNum;
        for (AX_U32 i = 0; i < nNum; i++) {
            /* [4]: create SKEL handle*/
            AX_SKEL_HANDLE_PARAM_T stHandleParam;
            memset(&stHandleParam, 0, sizeof(stHandleParam));
            stHandleParam.ePPL = m_stAttr.ePPL;
            stHandleParam.nFrameDepth = m_stAttr.nFrameDepth * m_stAttr.nGrpCount / nNum;
            stHandleParam.nFrameCacheDepth = 0;
            stHandleParam.nIoDepth = 0;
            stHandleParam.nWidth = m_stAttr.nWidth;
            stHandleParam.nHeight = m_stAttr.nHeight;
            stHandleParam.nNpuType = (AX_U32)(1 << i);  // AX_SKEL_NPU_DEFAULT

            AX_SKEL_CONFIG_T stConfig;
            AX_SKEL_CONFIG_ITEM_T stItems[16] = {0};
            AX_U8 itemIndex = 0;
            stConfig.nSize = 0;
            stConfig.pstItems = &stItems[0];

            // track_disable
            stConfig.pstItems[itemIndex].pstrType = (AX_CHAR *)"track_disable";
            AX_SKEL_COMMON_THRESHOLD_CONFIG_T stTrackDisableThreshold = {0};
            stTrackDisableThreshold.fValue = 1;
            stConfig.pstItems[itemIndex].pstrValue = (AX_VOID *)&stTrackDisableThreshold;
            stConfig.pstItems[itemIndex].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
            itemIndex++;
            // push_disable
            stConfig.pstItems[itemIndex].pstrType = (AX_CHAR *)"push_disable";
            AX_SKEL_COMMON_THRESHOLD_CONFIG_T stPushDisableThreshold = {0};
            stPushDisableThreshold.fValue = 1;
            stConfig.pstItems[itemIndex].pstrValue = (AX_VOID *)&stPushDisableThreshold;
            stConfig.pstItems[itemIndex].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
            itemIndex++;

            stConfig.nSize = itemIndex;
            stHandleParam.stConfig = stConfig;

            ret = AX_SKEL_Create(&stHandleParam, &m_hSkel[i]);
            if (AX_SKEL_SUCC != ret || NULL == m_hSkel[i]) {
                LOG_M_E(TAG, "AX_SKEL_Create fail, ret= 0x%x\n", ret);
                res = AX_FALSE;
                break;
            }

            /* [5]: register callback */
            ret = AX_SKEL_RegisterResultCallback(m_hSkel[i], SkelResultCallback, this);
            if (AX_SKEL_SUCC != ret) {
                LOG_M_E(TAG, "AX_SKEL_RegisterResultCallback() fail, ret = 0x%x", ret);
                res = AX_FALSE;
                break;
            }

            LOG_MM_I(TAG, "AX_SKEL_RegisterResultCallback success.");
        }

        m_bInited = AX_TRUE;

    } while (0);

    if (!res) {
        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
    }

    LOG_MM_I(TAG, "---");
    return res;
}

AX_BOOL CDetector::DeInit(AX_VOID) {
    LOG_MM_I(TAG, "+++");
    std::lock_guard<std::mutex> lock(m_mtxApi);

    if (!m_bInited) {
        return AX_TRUE;
    }

    if (m_bStarted) {
        LOG_MM_E(TAG, "detect thread is running");
        return AX_FALSE;
    }

    if (m_arrFrameQ) {
        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
    }

    AX_S32 ret;
    for (AX_U32 nChn = 0; nChn < m_stAttr.nChannelNum; ++nChn) {
        if (m_hSkel[nChn]) {
            ret = AX_SKEL_Destroy(m_hSkel[nChn]);
            if (0 != ret) {
                LOG_M_E(TAG, "%s: AX_SKEL_Destroy() fail, ret = 0x%x", __func__, ret);
                return AX_FALSE;
            }

            m_hSkel[nChn] = NULL;
        }
    }

    ret = AX_SKEL_DeInit();
    if (0 != ret) {
        LOG_MM_E(TAG, "AX_SKEL_DeInit() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    m_bInited = AX_FALSE;
    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CDetector::Start(AX_VOID) {
    LOG_MM_I(TAG, "+++");
    std::lock_guard<std::mutex> lock(m_mtxApi);

    if (!m_bInited) {
        LOG_MM_E(TAG, "please init at first!");
        return AX_FALSE;
    }

    if (m_bStarted) {
        LOG_MM_W(TAG, "detect is already started");
        return AX_TRUE;
    }

    if (!m_DetectThread.Start(std::bind(&CDetector::ProcessFrame, this, std::placeholders::_1), nullptr, "AppDetect", SCHED_FIFO, 99)) {
        LOG_MM_E(TAG, "create detect thread fail");
        return AX_FALSE;
    }

    m_bStarted = AX_TRUE;
    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CDetector::Stop(AX_VOID) {
    LOG_MM_I(TAG, "+++");
    std::lock_guard<std::mutex> lock(m_mtxApi);

    if (!m_bStarted) {
        return AX_TRUE;
    }

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

    m_bStarted = AX_FALSE;
    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CDetector::SendFrame(AX_U32 nGrp, CAXFrame &axFrame) {
    std::lock_guard<std::mutex> lock(m_mtxApi);

    if (!m_bStarted) {
        // LOG_MM_E(TAG, "detect is not started yet");
        return AX_FALSE;
    }

    if (nGrp >= m_stAttr.nGrpCount) {
        LOG_MM_E(TAG, "Group id: %d out of range(%d - %d), frame ignored.", nGrp, 0, m_stAttr.nGrpCount - 1);
        return AX_TRUE;
    }

#if 0 /* replace by VDEC or IVPS FRC */
        if (SkipFrame(axFrame)) {
            LOG_M_D(TAG, "dropfrm vdGrp %d vdChn %d frameNum %lld pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x", axFrame.nGrp,
                    axFrame.nChn, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
                    axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32Width,
                    axFrame.stFrame.stVFrame.stVFrame.u32Height, axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0],
                    axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);
            return AX_TRUE;
        }
#endif

    axFrame.IncRef();

    if (!m_arrFrameQ[nGrp].Push(axFrame)) {
        LOG_MM_W(TAG, "push grp[%d] frame %lld to q fail.GetCapacity:%d", nGrp, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum,
                 m_arrFrameQ[nGrp].GetCapacity());

        axFrame.DecRef();
        // CElapsedTimer::GetInstance()->mSleep(1);
    }

    return AX_TRUE;
}

AX_BOOL CDetector::SkipFrame(const CAXFrame &axFrame) {
    return (1 == (axFrame.stFrame.stVFrame.stVFrame.u64SeqNum % m_stAttr.nSkipRate)) ? AX_FALSE : AX_TRUE;
}

AX_VOID CDetector::ClearQueue(AX_U32 nGrp) {
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

AX_VOID CDetector::RegisterObserver(AX_U32 nGrp, IObserver *pObserver) {
    std::lock_guard<std::mutex> lock(m_mtxObs);
    m_mapObs.insert(std::make_pair(nGrp, pObserver));
}

AX_VOID CDetector::UnRegisterObserver(AX_U32 nGrp, IObserver *pObs) {
    std::lock_guard<std::mutex> lock(m_mtxObs);
    auto iter = m_mapObs.find(nGrp);
    if (iter != m_mapObs.end()) {
        m_mapObs.erase(iter);
    }
}

AX_VOID CDetector::NotifyAll(AX_U32 nGrp, AX_U32 nChn, DETECT_RESULT_T &pData) {
    std::lock_guard<std::mutex> lock(m_mtxObs);
    auto iter = m_mapObs.find(nGrp);
    if (iter != m_mapObs.end()) {
        (AX_VOID) iter->second->OnRecvData(E_OBS_TARGET_TYPE_REGION, nGrp, nChn, &pData);
    }
}
