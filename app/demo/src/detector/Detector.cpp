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
#include "AlgoOptionHelper.h"
#include "AppLog.hpp"
#include "DetectResult.hpp"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "OptionHelper.h"
#include "attrParser.hpp"

#ifndef SLT
#include "WebServer.h"
#endif

#define DETECTOR "detector"

namespace {
static AX_VOID SkelResult2DetectResult(DETECT_RESULT_T &hvcfp, const AX_SKEL_RESULT_T *pstResult, CDetector *pThis) {
    hvcfp.nSeqNum = pstResult->nFrameId;
    hvcfp.nW = pstResult->nOriginalWidth;
    hvcfp.nH = pstResult->nOriginalHeight;

    for (AX_U32 i = 0; i < pstResult->nObjectSize; ++i) {
        auto &stObjectItem = pstResult->pstObjectItems[i];

        AX_APP_ALGO_HVCFP_TYPE_E eType = AX_APP_ALGO_GET_HVCFP_TYPE(stObjectItem.pstrObjectCategory);

        AX_U32 nCountLimit = 0;
        AX_U32 *pCount = NULL;
        AX_APP_ALGO_HVCFP_ITEM_PTR pstItem = NULL;
        if (AX_APP_ALGO_HVCFP_BODY == eType) {
            nCountLimit = MAX_DETECT_RESULT_COUNT;
            pCount = &hvcfp.nBodyCount;
            pstItem = (AX_APP_ALGO_HVCFP_ITEM_PTR)hvcfp.stBodys;
        } else if (AX_APP_ALGO_HVCFP_VEHICLE == eType) {
            nCountLimit = MAX_DETECT_RESULT_COUNT;
            pCount = &hvcfp.nVehicleCount;
            pstItem = (AX_APP_ALGO_HVCFP_ITEM_PTR)hvcfp.stVehicles;
        } else if (AX_APP_ALGO_HVCFP_CYCLE == eType) {
            nCountLimit = MAX_DETECT_RESULT_COUNT;
            pCount = &hvcfp.nCycleCount;
            pstItem = (AX_APP_ALGO_HVCFP_ITEM_PTR)hvcfp.stCycles;
        } else if (AX_APP_ALGO_HVCFP_FACE == eType) {
            nCountLimit = MAX_DETECT_RESULT_COUNT;
            pCount = &hvcfp.nFaceCount;
            pstItem = (AX_APP_ALGO_HVCFP_ITEM_PTR)hvcfp.stFaces;
        } else if (AX_APP_ALGO_HVCFP_PLATE == eType) {
            nCountLimit = MAX_DETECT_RESULT_COUNT;
            pCount = &hvcfp.nPlateCount;
            pstItem = (AX_APP_ALGO_HVCFP_ITEM_PTR)hvcfp.stPlates;
        } else {
            continue;
        }

        auto &nCount = *(AX_U32 *)pCount;
        if (nCount >= nCountLimit) {
            continue;
        }

        auto &stItem = pstItem[nCount];

        stItem.eType = eType;
        stItem.u64FrameId = stObjectItem.nFrameId;
        stItem.u64TrackId = stObjectItem.nTrackId;
        stItem.fConfidence = stObjectItem.fConfidence;

        // track status
        if (stObjectItem.eTrackState == AX_SKEL_TRACK_STATUS_NEW) {
            stItem.eTrackStatus = AX_APP_ALGO_TRACK_STATUS_NEW;
        } else if (stObjectItem.eTrackState == AX_SKEL_TRACK_STATUS_UPDATE) {
            stItem.eTrackStatus = AX_APP_ALGO_TRACK_STATUS_UPDATE;
        } else if (stObjectItem.eTrackState == AX_SKEL_TRACK_STATUS_DIE) {
            stItem.eTrackStatus = AX_APP_ALGO_TRACK_STATUS_LOST;
        } else if (stObjectItem.eTrackState == AX_SKEL_TRACK_STATUS_SELECT) {
            stItem.eTrackStatus = AX_APP_ALGO_TRACK_STATUS_SELECT;
        } else {
            stItem.eTrackStatus = AX_APP_ALGO_TRACK_STATUS_BUTT;
        }

        if (stObjectItem.eTrackState == AX_SKEL_TRACK_STATUS_NEW || stObjectItem.eTrackState == AX_SKEL_TRACK_STATUS_UPDATE) {
            if (0 >= stObjectItem.stRect.fW || 0 >= stObjectItem.stRect.fH) {
                continue;
            }
        }

        // box
        stItem.stBox.fX = stObjectItem.stRect.fX / pstResult->nOriginalWidth;
        stItem.stBox.fY = stObjectItem.stRect.fY / pstResult->nOriginalHeight;
        stItem.stBox.fW = stObjectItem.stRect.fW / pstResult->nOriginalWidth;
        stItem.stBox.fH = stObjectItem.stRect.fH / pstResult->nOriginalHeight;
        stItem.stBox.nImgWidth = pstResult->nOriginalWidth;
        stItem.stBox.nImgHeight = pstResult->nOriginalHeight;

        // img
        stItem.stImg.bExist = stObjectItem.bCropFrame;
        stItem.stImg.pData = stObjectItem.stCropFrame.pFrameData;
        stItem.stImg.nDataSize = stObjectItem.stCropFrame.nFrameDataSize;
        stItem.stImg.nWidth = stObjectItem.stCropFrame.nFrameWidth;
        stItem.stImg.nHeight = stObjectItem.stCropFrame.nFrameHeight;

        // panorama img
        stItem.stPanoramaImg.bExist = stObjectItem.bCropFrame;
        stItem.stPanoramaImg.pData = stObjectItem.stPanoraFrame.pFrameData;
        stItem.stPanoramaImg.nDataSize = stObjectItem.stPanoraFrame.nFrameDataSize;
        stItem.stPanoramaImg.nWidth = stObjectItem.stPanoraFrame.nFrameWidth;
        stItem.stPanoramaImg.nHeight = stObjectItem.stPanoraFrame.nFrameHeight;

        // attr parser
        ATTR_INFO_T stAttrInfo;
        CAttrParser::GetInstance()->AttrParser(&stObjectItem, &stAttrInfo);

        if (AX_APP_ALGO_HVCFP_BODY == eType) {
            stItem.stAttr.stBodyAttr.bExist = stAttrInfo.bExist;
        } else if (AX_APP_ALGO_HVCFP_VEHICLE == eType) {
            stItem.stAttr.stVehicleAttr.bExist = stAttrInfo.bExist;
            stItem.stAttr.stVehicleAttr.stPlateAttr.bExist = stAttrInfo.bExist;
            stItem.stAttr.stVehicleAttr.stPlateAttr.bValid = stAttrInfo.stVehicleInfo.stPlateInfo.bValid;
            strncpy(stItem.stAttr.stVehicleAttr.stPlateAttr.strPlateCode, (const AX_CHAR *)stAttrInfo.stVehicleInfo.stPlateInfo.szNum,
                    sizeof(stItem.stAttr.stVehicleAttr.stPlateAttr.strPlateCode));
            stItem.stAttr.stVehicleAttr.stPlateAttr.ePlateColor = stAttrInfo.stVehicleInfo.stPlateInfo.ePlateColor;
        } else if (AX_APP_ALGO_HVCFP_CYCLE == eType) {
            stItem.stAttr.stCycleAttr.bExist = stAttrInfo.bExist;
        } else if (AX_APP_ALGO_HVCFP_FACE == eType) {
            stItem.stAttr.stFaceAttr.bExist = stAttrInfo.bExist;
            stItem.stAttr.stFaceAttr.nAge = stAttrInfo.stFaceInfo.nAge;
            stItem.stAttr.stFaceAttr.nGender = stAttrInfo.stFaceInfo.nGender;
            stItem.stAttr.stFaceAttr.eRespirator = stAttrInfo.stFaceInfo.eRespirator;
        } else if (AX_APP_ALGO_HVCFP_PLATE == eType) {
            stItem.stAttr.stPlateAttr.bExist = stAttrInfo.bExist;
            stItem.stAttr.stPlateAttr.bValid = stAttrInfo.stPlateInfo.bValid;
            strncpy(stItem.stAttr.stPlateAttr.strPlateCode, (const AX_CHAR *)stAttrInfo.stPlateInfo.szNum,
                    sizeof(stItem.stAttr.stPlateAttr.strPlateCode));
            stItem.stAttr.stPlateAttr.ePlateColor = stAttrInfo.stPlateInfo.ePlateColor;
        }

        stItem.stAttr.stFacePoseBlur.fPitch = stObjectItem.stFacePostBlur.fPitch;
        stItem.stAttr.stFacePoseBlur.fYaw = stObjectItem.stFacePostBlur.fYaw;
        stItem.stAttr.stFacePoseBlur.fRoll = stObjectItem.stFacePostBlur.fRoll;
        stItem.stAttr.stFacePoseBlur.fBlur = stObjectItem.stFacePostBlur.fBlur;

        nCount += 1;
    }
}

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
    DETECT_RESULT_T hvcfp;
    hvcfp.nSeqNum = pPrivData->nSeqNum;
    hvcfp.nGrpId = pPrivData->nGrpId;
    hvcfp.nSnsId = pPrivData->nSnsId;

    SkelResult2DetectResult(hvcfp, pstResult, pThis);

    pThis->NotifyAll((AX_U32)hvcfp.nSnsId, 0, &hvcfp);

    /* release hvcfp result */
    AX_SKEL_Release((AX_VOID *)pstResult);

    /* giveback private data */
    pThis->ReleaseSkelPrivateData(pPrivData);

    return;
}
}  // namespace

AX_VOID CDetector::RunDetect(AX_VOID *pArg) {
    LOG_MM_I(DETECTOR, "+++");

    AX_U64 nFrameId = 0;
    AX_U8 nSnsId = 0;
    AX_U8 nNextSnsId = 0;
    const AX_U8 nGrpCount = m_stAttr.nGrpCount;
    CAXFrame *pAxFrame{nullptr};
    AX_SKEL_FRAME_T skelFrame = {0};
    AX_U64 nStartMs = 0;
    AX_U64 nEndMs = 0;
    AX_U32 nFrameCnt = 0;
    AX_F64 fFps = 0.0;
    nStartMs = CElapsedTimer::GetInstance()->GetTickCount();
    while (m_DetectThread.IsRunning()) {
        for (nSnsId = nNextSnsId; nSnsId < nGrpCount; ++nSnsId) {
            if (m_arrFrameQ[nSnsId].Pop(pAxFrame, 0)) {
                break;
            }
        }
        if (nSnsId >= nGrpCount) {
            nNextSnsId = 0;
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        } else {
            nNextSnsId = nSnsId + 1;
            if (nNextSnsId >= nGrpCount) {
                nNextSnsId = 0;
            }
        }

        SKEL_FRAME_PRIVATE_DATA_T *pPrivData = m_skelData.borrow();
        if (!pPrivData) {
            LOG_M_E(DETECTOR, "%s: borrow skel frame private data fail", __func__);
            pAxFrame->FreeMem();
            continue;
        } else {
            pPrivData->nSeqNum = pAxFrame->stFrame.stVFrame.stVFrame.u64SeqNum;
            pPrivData->nGrpId = m_stAttr.nGrp;
            pPrivData->nSnsId = nSnsId;
        }
        skelFrame.nFrameId = ++nFrameId;
        skelFrame.nStreamId = nSnsId;
        skelFrame.stFrame = pAxFrame->stFrame.stVFrame.stVFrame;
        skelFrame.pUserData = (void *)pPrivData;

        AX_S32 ret = AX_SKEL_SendFrame(m_Skel, &skelFrame, -1);
        if (0 != ret) {
            LOG_MM_E(DETECTOR,
                     "AX_SKEL_SendFrame fail,ret =0x%x, nSnsId:%d vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx VirAddr:0x%llx %dx%d "
                     "stride %d blkId "
                     "0x%x, size:%u",
                     ret, nSnsId, pAxFrame->nGrp, pAxFrame->nChn, pAxFrame->stFrame.stVFrame.stVFrame.u64SeqNum,
                     pAxFrame->stFrame.stVFrame.stVFrame.u64PTS, pAxFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0],
                     pAxFrame->stFrame.stVFrame.stVFrame.u64VirAddr[0], pAxFrame->stFrame.stVFrame.stVFrame.u32Width,
                     pAxFrame->stFrame.stVFrame.stVFrame.u32Height, pAxFrame->stFrame.stVFrame.stVFrame.u32PicStride[0],
                     pAxFrame->stFrame.stVFrame.stVFrame.u32BlkId[0], pAxFrame->stFrame.stVFrame.stVFrame.u32FrameSize);
        }
        pAxFrame->FreeMem();
        CElapsedTimer::GetInstance()->mSleep(10);
        nFrameCnt++;
        if (!(nFrameCnt % 100)) {
            nEndMs = CElapsedTimer::GetInstance()->GetTickCount();
            fFps = (1000 / ((AX_F64)(nEndMs - nStartMs) / nFrameCnt));
            LOG_MM_I(DETECTOR, "took time:%ldms,nFrameCnt:%d,  fps:%lf", nEndMs - nStartMs, nFrameCnt, fFps);
            nStartMs = CElapsedTimer::GetInstance()->GetTickCount();
            nFrameCnt = 0;
        }
    }
    LOG_MM_I(DETECTOR, "---");
}

AX_BOOL CDetector::Init(const DETECTOR_ATTR_T &stAttr) {
    LOG_MM_I(DETECTOR, "+++");

    if (0 == stAttr.nGrpCount) {
        LOG_MM_E(DETECTOR, "invalid nGrpCount");
        return AX_FALSE;
    }
    AX_BOOL res = AX_TRUE;
    m_arrFrameQ = new (std::nothrow) CAXLockQ<CAXFrame *>[stAttr.nGrpCount];
    if (!m_arrFrameQ) {
        LOG_MM_E(DETECTOR, "alloc queue fail");
        return AX_FALSE;
    } else {
        for (AX_U32 i = 0; i < stAttr.nGrpCount; ++i) {
            m_arrFrameQ[i].SetCapacity(10);
        }
    }

    m_stAttr.ePPL = stAttr.ePPL;
    m_stAttr.nFrameDepth = stAttr.nFrameDepth;
    m_stAttr.nGrpCount = stAttr.nGrpCount;
    m_stAttr.strModelPath = stAttr.strModelPath;
    m_stAttr.nGrp = stAttr.nGrp;

    /* define how many frames can skel to handle in parallel */
    constexpr AX_U32 PARALLEL_FRAME_COUNT = 2;
    m_skelData.reserve(stAttr.nGrpCount * PARALLEL_FRAME_COUNT);

    LOG_MM_I(DETECTOR, "m_stAttr.width:%d, height:%d, modelPath:%s", m_stAttr.nWidth, m_stAttr.nHeight, stAttr.strModelPath.c_str());
    /* [1]: SKEL init */
    AX_SKEL_INIT_PARAM_T stInit;
    AX_S32 ret = 0;
    memset(&stInit, 0, sizeof(stInit));
    stInit.pStrModelDeploymentPath = m_stAttr.strModelPath.c_str();

    ret = AX_SKEL_Init(&stInit);
    if (AX_SKEL_SUCC != ret) {
        LOG_M_E(DETECTOR, "AX_SKEL_Init() fail, ret= 0x%x\n", ret);
        delete[] m_arrFrameQ;
        m_arrFrameQ = nullptr;
        return AX_FALSE;
    }
    do {
        /* [2]: print SKEL version*/
        const AX_SKEL_VERSION_INFO_T *pstVersion{nullptr};
        ret = AX_SKEL_GetVersion(&pstVersion);
        if (AX_SKEL_SUCC != ret) {
            LOG_M_E(DETECTOR, "AX_SKEL_GetVersion() fail, ret= 0x%x\n", ret);
        } else {
            if (pstVersion && pstVersion->pstrVersion) {
                LOG_MM_D(DETECTOR, "SKEL version:%s", pstVersion->pstrVersion);
            }
            AX_SKEL_Release((AX_VOID *)pstVersion);
        }

        /* [3]: check SKEL model required */
        const AX_SKEL_CAPABILITY_T *pstCapability{nullptr};
        ret = AX_SKEL_GetCapability(&pstCapability);
        if (AX_SKEL_SUCC != ret) {
            LOG_M_E(DETECTOR, "AX_SKEL_GetCapability() fail, ret= 0x%x\n", ret);
            res = AX_FALSE;
            break;
        } else {
            AX_BOOL bHVCFP{AX_FALSE};
            for (AX_U32 i = 0; i < pstCapability->nPPLConfigSize; i++) {
                if (m_stAttr.ePPL == pstCapability->pstPPLConfig[i].ePPL) {
                    bHVCFP = AX_TRUE;
                }
            }
            AX_SKEL_Release((AX_VOID *)pstCapability);
            if (!bHVCFP) {
                LOG_M_E(DETECTOR, "%s: SKEL not found HVCFP model", __func__);
                res = AX_FALSE;
                break;
            }
        }
        /* [4]: create SKEL handle*/
        AX_SKEL_HANDLE_PARAM_T stHandleParam;
        memset(&stHandleParam, 0, sizeof(stHandleParam));
        stHandleParam.ePPL = m_stAttr.ePPL;
        stHandleParam.nFrameDepth = 1;
        stHandleParam.nFrameCacheDepth = 1;
        stHandleParam.nIoDepth = 0;
        stHandleParam.nWidth = m_stAttr.nWidth;
        stHandleParam.nHeight = m_stAttr.nHeight;
        stHandleParam.nNpuType = (AX_U32)AX_SKEL_NPU_DEFAULT;

        ret = AX_SKEL_Create(&stHandleParam, &m_Skel);

        if (AX_SKEL_SUCC != ret || NULL == m_Skel) {
            LOG_M_E(DETECTOR, "AX_SKEL_Create fail, ret= 0x%x\n", ret);
            res = AX_FALSE;
            break;
        }
        /* [5]: register callback */
        ret = AX_SKEL_RegisterResultCallback(m_Skel, SkelResultCallback, this);
        if (AX_SKEL_SUCC != ret) {
            LOG_M_E(DETECTOR, "AX_SKEL_RegisterResultCallback() fail, ret = 0x%x", ret);
            res = AX_FALSE;
            break;
        }
        LOG_M_D(DETECTOR, "AX_SKEL_RegisterResultCallback success.");

        /* [6]: set Skel Push Mode */
        SetConfig(ALGO_HVCFP_PARAM(m_nSnsId));

        m_initState = AX_TRUE;

    } while (0);

    LOG_MM_I(DETECTOR, "---");
    return res;
}

AX_BOOL CDetector::DeInit(AX_VOID) {
    if (m_initState) {
        LOG_MM_I(DETECTOR, "+++");

        if (m_DetectThread.IsRunning()) {
            LOG_MM_E(DETECTOR, "detect thread is running");
            return AX_FALSE;
        }

        if (m_arrFrameQ) {
            delete[] m_arrFrameQ;
            m_arrFrameQ = nullptr;
        }

        AX_S32 ret = AX_SKEL_DeInit();
        if (0 != ret) {
            LOG_MM_E(DETECTOR, "AX_SKEL_DeInit() fail, ret = 0x%x", ret);
            return AX_FALSE;
        }

        LOG_MM_I(DETECTOR, "---");
    }

    return AX_TRUE;
}

AX_BOOL CDetector::Start(AX_VOID) {
    if (m_initState) {
        LOG_MM_I(DETECTOR, "+++");

        if (!m_DetectThread.Start(std::bind(&CDetector::RunDetect, this, std::placeholders::_1), nullptr, "AppDetect")) {
            LOG_MM_E(DETECTOR, "create detect thread fail");
            return AX_FALSE;
        }

        LOG_MM_I(DETECTOR, "---");
    }
    return AX_TRUE;
}

AX_BOOL CDetector::Stop(AX_VOID) {
    LOG_MM_I(DETECTOR, "+++");
    if (m_DetectThread.IsRunning()) {
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

        if (m_Skel) {
            AX_SKEL_Destroy(m_Skel);
        }
    }

    LOG_MM_I(DETECTOR, "---");

    return AX_TRUE;
}

AX_BOOL CDetector::SendFrame(AX_U32 nGrp, CAXFrame *paxFrame) {
    if (!m_initState) {
        paxFrame->FreeMem();
        return AX_TRUE;
    } else {
        if (!m_arrSnsAiEnable[nGrp]) {
            paxFrame->FreeMem();
            return AX_TRUE;
        }
        if (!m_arrFrameQ[nGrp].Push(paxFrame)) {
            LOG_MM_W(DETECTOR, "push grp[%d] frame %lld to q fail.GetCapacity:%d", nGrp, paxFrame->stFrame.stVFrame.stVFrame.u64SeqNum,
                     m_arrFrameQ[nGrp].GetCapacity());
            paxFrame->FreeMem();
            CElapsedTimer::GetInstance()->mSleep(1);
        }
    }
    return AX_TRUE;
}

AX_S32 CDetector::CheckCapacity(AX_S32 nTimeOut) {
    return -1;
}

AX_VOID CDetector::ClearQueue(AX_S32 nGrp) {
    AX_U32 nCount = m_arrFrameQ[nGrp].GetCount();
    if (nCount > 0) {
        CAXFrame *pAxFrame;
        for (AX_U32 i = 0; i < nCount; ++i) {
            if (m_arrFrameQ[nGrp].Pop(pAxFrame, 0)) {
                pAxFrame->FreeMem();
            }
        }
    }
}

AX_VOID CDetector::RegObserver(IObserver *pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nGroup = m_stAttr.nGrp;
        tTransAttr.nChannel = 0;
        tTransAttr.bLink = AX_FALSE;
        tTransAttr.nWidth = m_stAttr.nWidth;
        tTransAttr.nHeight = m_stAttr.nHeight;

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_DETECT, m_stAttr.nGrp, 0, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CDetector::UnregObserver(IObserver *pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (std::vector<IObserver *>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        if (*it == pObserver) {
            m_vecObserver.erase(it);
            break;
        }
    }
}

AX_VOID CDetector::NotifyAll(AX_U32 nSnsId, AX_U32 nChn, AX_VOID *pStream) {
    if (nullptr == pStream) {
        return;
    }

    for (std::vector<IObserver *>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_DETECT, nSnsId, nChn, pStream);
    }
}

AX_VOID CDetector::Enable(AX_U8 nSnsId, AX_BOOL bEnable) {
    m_arrSnsAiEnable[nSnsId] = bEnable;
    if (!bEnable) {
        DETECT_RESULT_T hvcfp;
        hvcfp.nBodyCount = 0;
        hvcfp.nVehicleCount = 0;
        hvcfp.nCycleCount = 0;
        hvcfp.nFaceCount = 0;
        hvcfp.nPlateCount = 0;
        NotifyAll((AX_U32)nSnsId, 0, &hvcfp);
    }
};

AX_S32 CDetector::SetSkelPushMode(AI_PUSH_STATEGY_T &stStrategy) {
    AX_APP_ALGO_PUSH_STRATEGY_T stPushStrategy;

    stPushStrategy.ePushMode = (AX_APP_ALGO_PUSH_MODE_E)stStrategy.ePushMode;
    stPushStrategy.nInterval = (AX_U32)stStrategy.nPushIntervalMs;
    stPushStrategy.nPushCount = (AX_U32)stStrategy.nPushCounts;

    AX_BOOL bRet = SetPushStrategy(stPushStrategy, AX_TRUE);

    return (bRet ? 0 : -1);
}

AX_BOOL CDetector::SetRoi(const AX_APP_ALGO_ROI_CONFIG_T &stRoi, AX_BOOL bUpdateParam) {
    if (stRoi.stPolygon.nPointNum != 4) {
        return AX_FALSE;
    }

    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_ROI_CONFIG_T stRoiConfig;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    AX_F32 x = stRoi.stPolygon.stPoints[0].fX;
    AX_F32 y = stRoi.stPolygon.stPoints[0].fY;
    AX_F32 w = stRoi.stPolygon.stPoints[3].fX - x;
    AX_F32 h = stRoi.stPolygon.stPoints[3].fY - y;

    AX_SKEL_RECT_T stRect = {x, y, w, h};

    stRoiConfig.bEnable = stRoi.bEnable;
    stRoiConfig.stRect = stRect;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"detect_roi";
    stConfigItem.pstrValue = (AX_VOID *)&stRoiConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_ROI_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stRoiConfig = stRoi;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "roi(enable:%d, rect:[%f, %f, %f, %f])", stRoi.bEnable, x, y, w, h);

    return AX_TRUE;
}

AX_BOOL CDetector::SetPushStrategy(const AX_APP_ALGO_PUSH_STRATEGY_T &stPushStrategy, AX_BOOL bUpdateParam) {
    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    AX_SKEL_PUSH_STRATEGY_T stPushStrategyConfig;

    // set push_strategy
    stPushStrategyConfig.ePushMode = (AX_SKEL_PUSH_MODE_E)stPushStrategy.ePushMode;
    stPushStrategyConfig.nIntervalTimes = stPushStrategy.nInterval;
    stPushStrategyConfig.nPushCounts = stPushStrategy.nPushCount;
    stPushStrategyConfig.bPushSameFrame = AX_TRUE;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"push_strategy";
    stConfigItem.pstrValue = (AX_VOID *)&stPushStrategyConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_PUSH_STRATEGY_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stPushStrategy = stPushStrategy;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "push strategy(mode:%d, interval:%d, count:%d)", stPushStrategy.ePushMode, stPushStrategy.nInterval,
            stPushStrategy.nPushCount);

    return AX_TRUE;
}

AX_BOOL CDetector::SetObjectFilter(AX_APP_ALGO_HVCFP_TYPE_E eType, const AX_APP_ALGO_HVCFP_FILTER_CONFIG_T &stObjectFliter,
                                   AX_BOOL bUpdateParam) {
    if (eType >= AX_APP_ALGO_HVCFP_TYPE_BUTT) {
        return AX_FALSE;
    }

    std::string strObject = AX_APP_ALGO_GET_OBJECT_CATEGORY(eType);

    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stConfidenceConfig;
    AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T stObjectSizeFilterConfig;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    memset(&stObjectSizeFilterConfig, 0x00, sizeof(stObjectSizeFilterConfig));
    stObjectSizeFilterConfig.nWidth = stObjectFliter.nWidth;
    stObjectSizeFilterConfig.nHeight = stObjectFliter.nHeight;

    std::string aObject;
    aObject = strObject + "_min_size";
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)aObject.c_str();
    stConfigItem.pstrValue = (AX_VOID *)&stObjectSizeFilterConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    std::string aObjectConfidence;
    aObjectConfidence = strObject + "_confidence";
    stConfidenceConfig.fValue = (AX_F32)stObjectFliter.fConfidence;
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)aObjectConfidence.c_str();
    stConfigItem.pstrValue = (AX_VOID *)&stConfidenceConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stObjectFliterConfig[eType] = stObjectFliter;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "%s filter(%d X %d, confidence: %.2f)", strObject.c_str(), stObjectFliter.nWidth, stObjectFliter.nHeight,
            stObjectFliter.fConfidence);

    return AX_TRUE;
}

AX_BOOL CDetector::SetTrackSize(const AX_APP_ALGO_TRACK_SIZE_T &stTrackSize, AX_BOOL bUpdateParam) {
    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    // set max track human size
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stMaxTrackHumanSize;
    stMaxTrackHumanSize.fValue = (AX_F32)stTrackSize.nTrackHumanSize;
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"body_max_target_count";
    stConfigItem.pstrValue = (AX_VOID *)&stMaxTrackHumanSize;
    stConfigItem.nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    // set max track vehicle size
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stMaxTrackVehicleSize;
    stMaxTrackVehicleSize.fValue = (AX_F32)stTrackSize.nTrackVehicleSize;
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"vehicle_max_target_count";
    stConfigItem.pstrValue = (AX_VOID *)&stMaxTrackVehicleSize;
    stConfigItem.nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    // set max track cycle size
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stMaxTrackCycleSize;
    stMaxTrackCycleSize.fValue = (AX_F32)stTrackSize.nTrackCycleSize;
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"cycle_max_target_count";
    stConfigItem.pstrValue = (AX_VOID *)&stMaxTrackCycleSize;
    stConfigItem.nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stTrackSize = stTrackSize;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "Track size(human: %d, vehicle: %d, cycle: %d)", stTrackSize.nTrackHumanSize, stTrackSize.nTrackVehicleSize,
            stTrackSize.nTrackCycleSize);

    return AX_TRUE;
}

AX_BOOL CDetector::SetPanorama(const AX_APP_ALGO_PANORAMA_T &stPanorama, AX_BOOL bUpdateParam) {
    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_PUSH_PANORAMA_CONFIG_T stPanoramaConfig;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    stPanoramaConfig.bEnable = stPanorama.bEnable;
    stPanoramaConfig.nQuality = (AX_U32)ALGO_HVCFP_PARAM(m_nSnsId).nCropEncoderQpLevel;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"push_panorama";
    stConfigItem.pstrValue = (AX_VOID *)&stPanoramaConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_PUSH_PANORAMA_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stPanoramaConfig = stPanorama;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "Panorama(enable:%d)", stPanorama.bEnable);

    return AX_TRUE;
}

AX_BOOL CDetector::SetCropEncoderQpLevel(const AX_U32 &nCropEncoderQpLevel, AX_BOOL bUpdateParam) {
    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    // set crop encoder qpLevel
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stCropEncoderQpLevelThreshold;
    stCropEncoderQpLevelThreshold.fValue = (AX_F32)nCropEncoderQpLevel;
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"crop_encoder_qpLevel";
    stConfigItem.pstrValue = (AX_VOID *)&stCropEncoderQpLevelThreshold;
    stConfigItem.nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.nCropEncoderQpLevel = (AX_U8)nCropEncoderQpLevel;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "fCropEncoderQpLevel (%d)", nCropEncoderQpLevel);

    return AX_TRUE;
}

AX_BOOL CDetector::SetCropThreshold(AX_APP_ALGO_HVCFP_TYPE_E eType, const AX_APP_ALGO_CROP_THRESHOLD_CONFIG_T &stCropThreshold,
                                    AX_BOOL bUpdateParam) {
#if 0
    if (eType >= AX_APP_ALGO_HVCFP_TYPE_BUTT) {
        return AX_FALSE;
    }

    std::string strObject = AX_APP_ALGO_GET_OBJECT_CATEGORY(eType);

    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T stCropThresholdConfig;

    memset(&stCropThresholdConfig, 0x00, sizeof(stCropThresholdConfig));
    stCropThresholdConfig.fScaleLeft = stCropThreshold.fScaleLeft;
    stCropThresholdConfig.fScaleRight = stCropThreshold.fScaleRight;
    stCropThresholdConfig.fScaleTop = stCropThreshold.fScaleTop;
    stCropThresholdConfig.fScaleBottom = stCropThreshold.fScaleBottom;

    vContent.clear();

    std::string aObject;
    aObject = strObject + "_crop_encoder";

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (char *)aObject.c_str();
    stConfigItem.pstrValue = (AX_VOID *)&stCropThresholdConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stCropThresholdConfig[eType] = stCropThreshold;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    LOG_M_I(DETECTOR, "%s CropThreshold(%.2f, %.2f, %.2f, %.2f)",
            strObject.c_str(),
            stCropThreshold.fScaleLeft,
            stCropThreshold.fScaleRight,
            stCropThreshold.fScaleTop,
            stCropThreshold.fScaleBottom);
#endif

    return AX_TRUE;
}

AX_BOOL CDetector::SetPushFilter(AX_APP_ALGO_HVCFP_TYPE_E eType, const AX_APP_ALGO_PUSH_FILTER_CONFIG_T &stPushFliter,
                                 AX_BOOL bUpdateParam) {
    if (eType >= AX_APP_ALGO_HVCFP_TYPE_BUTT) {
        return AX_FALSE;
    }

    std::string strObject = AX_APP_ALGO_GET_OBJECT_CATEGORY(eType);

    std::vector<AX_SKEL_CONFIG_ITEM_T> vContent;
    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_ATTR_FILTER_CONFIG_T stPushFilterConfig;

    vContent.clear();
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    memset(&stPushFilterConfig, 0x00, sizeof(stPushFilterConfig));
    if (strObject == "face") {
        stPushFilterConfig.stFaceAttrFilterConfig.nWidth = stPushFliter.stFacePushFilterConfig.nWidth;
        stPushFilterConfig.stFaceAttrFilterConfig.nHeight = stPushFliter.stFacePushFilterConfig.nHeight;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fPitch = stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fPitch;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fYaw = stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fYaw;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fRoll = stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fRoll;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fBlur = stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fBlur;
    } else {
        stPushFilterConfig.stCommonAttrFilterConfig.fQuality = stPushFliter.stCommonPushFilterConfig.fQuality;
    }

    std::string aObject;
    aObject = "push_quality_" + strObject;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)aObject.c_str();
    stConfigItem.pstrValue = (AX_VOID *)&stPushFilterConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_ATTR_FILTER_CONFIG_T);
    vContent.emplace_back(stConfigItem);

    stConfig.pstItems = vContent.data();
    stConfig.nSize = vContent.size();

    AX_S32 nRet = AX_SKEL_SetConfig(m_Skel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(DETECTOR, "AX_SKEL_SetConfig error: %d", nRet);

        return AX_FALSE;
    }

    if (bUpdateParam) {
        AX_APP_ALGO_HVCFP_PARAM_T stParam = ALGO_HVCFP_PARAM(m_nSnsId);
        stParam.stPushFliterConfig[eType] = stPushFliter;
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stParam);
    }

    if (strObject == "face") {
        LOG_M_I(DETECTOR, "%s push filter(%d X %d, p: %.2f, y: %.2f, r: %.2f, b: %.2f)", strObject.c_str(),
                stPushFliter.stFacePushFilterConfig.nWidth, stPushFliter.stFacePushFilterConfig.nHeight,
                stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fPitch, stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fYaw,
                stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fRoll, stPushFliter.stFacePushFilterConfig.stFacePoseBlur.fBlur);
    } else {
        LOG_M_I(DETECTOR, "%s push filter(Quality: %.2f)", strObject.c_str(), stPushFliter.stCommonPushFilterConfig.fQuality);
    }

    return AX_TRUE;
}

AX_BOOL CDetector::SetConfig(const AX_APP_ALGO_HVCFP_PARAM_T &stConfig, AX_BOOL bUpdateParam) {
    // update roi
    SetRoi(stConfig.stRoiConfig);

    // update push strategy
    SetPushStrategy(stConfig.stPushStrategy);

    // update object fliter
    for (size_t i = 0; i < AX_APP_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetObjectFilter((AX_APP_ALGO_HVCFP_TYPE_E)i, stConfig.stObjectFliterConfig[i]);
    }

    // update track size
    SetTrackSize(stConfig.stTrackSize);

    // update panorama
    SetPanorama(stConfig.stPanoramaConfig);

    // update crop encoder qpLevel
    SetCropEncoderQpLevel((AX_U32)stConfig.nCropEncoderQpLevel);

    // update crop threshold
    for (size_t i = 0; i < AX_APP_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetCropThreshold((AX_APP_ALGO_HVCFP_TYPE_E)i, stConfig.stCropThresholdConfig[i]);
    }

    // update push fliter
    for (size_t i = 0; i < AX_APP_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetPushFilter((AX_APP_ALGO_HVCFP_TYPE_E)i, stConfig.stPushFliterConfig[i]);
    }

    if (bUpdateParam) {
        SET_ALGO_HVCFP_PARAM(m_nSnsId, stConfig);
    }

    return AX_TRUE;
}
