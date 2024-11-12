/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "IVPSGrpStage.h"
#include <sys/prctl.h>
#include <chrono>
#include <map>
#include "AXFrame.hpp"
#include "AXThread.hpp"
#include "AppLogApi.h"
#include "CmdLineParser.h"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "IvpsOptionHelper.h"
#include "OptionHelper.h"
#include "SensorOptionHelper.h"
#define IVPS "IVPS"

#define MAX_IPC_IVPS_FRAME_TIMEOUT (1000)
#define MAX_REGION_GROUP (3)
#define TDP_CROP_RESIZE_APPLY_GRP (3)
#define TDP_CROP_RESIZE_APPLY_CHN (0)
#define IVPS_IN_FIFO_DEPTH (2)
#define IVPS_OUT_FIFO_DEPTH (2)

CIVPSGrpStage::CIVPSGrpStage(IVPS_GROUP_CFG_T& tGrpConfig) : CAXStage(IVPS), m_tIvpsGrpCfg(tGrpConfig), m_nIvpsGrp(tGrpConfig.nGrp) {
}

AX_BOOL CIVPSGrpStage::Init() {
    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::DeInit() {
    if (m_bStarted) {
        Stop();
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::Start(STAGE_START_PARAM_PTR pStartParams) {
    IVPS_GRP_T& tGrp = m_tIvpsGrp;

    LOG_MM_I(IVPS, "[%d] +++", m_nIvpsGrp);

    if (0 == m_tIvpsGrp.tPipelineAttr.nOutChnNum) {
        return AX_FALSE;
    }

    AX_U16 nIvpsGrp = tGrp.nGroup;
    AX_S32 ret = AX_IVPS_CreateGrp(nIvpsGrp, &tGrp.tGroupAttr);
    if (AX_SUCCESS != ret) {
        LOG_M_E(IVPS, "AX_IVPS_CreateGrp(Grp: %d) failed, ret=0x%x", nIvpsGrp, ret);
        return AX_FALSE;
    }

    ret = AX_IVPS_SetPipelineAttr(nIvpsGrp, &tGrp.tPipelineAttr);
    if (AX_SUCCESS != ret) {
        LOG_MM_E(IVPS, "AX_IVPS_SetPipelineAttr(Grp: %d) failed, ret=0x%x", nIvpsGrp, ret);
        return AX_FALSE;
    }

    for (AX_U8 nChn = 0; nChn < tGrp.tPipelineAttr.nOutChnNum; ++nChn) {
        ret = AX_IVPS_EnableChn(nIvpsGrp, nChn);
        if (AX_SUCCESS != ret) {
            LOG_M_E(IVPS, "AX_IVPS_EnableChn(Grp: %d, Chn: %d) failed, ret=0x%x", nIvpsGrp, nChn, ret);
            return AX_FALSE;
        }
        m_mapChnState[nChn] = AX_TRUE;
        LOG_M(IVPS, "Enable channel (Grp: %d, Chn: %d)", nIvpsGrp, nChn);
    }

    ret = AX_IVPS_StartGrp(nIvpsGrp);
    if (AX_SUCCESS != ret) {
        LOG_M_E(IVPS, "AX_IVPS_StartGrp(Grp: %d) failed, ret=0x%x", nIvpsGrp, ret);
        return AX_FALSE;
    }

    StartWorkThread();

    if (!StartOSD()) {
        return AX_FALSE;
    }

    m_bStarted = AX_TRUE;

    LOG_MM_I(IVPS, "[%d] ---", m_nIvpsGrp);

    return CAXStage::Start(pStartParams);
}

AX_BOOL CIVPSGrpStage::Stop() {
    LOG_MM_I(IVPS, "[%d] +++", m_nIvpsGrp);

    AX_S32 ret = AX_SUCCESS;
    if (0 == m_tIvpsGrp.tPipelineAttr.nOutChnNum) {
        return AX_FALSE;
    }

    if (!StopOSD()) {
        return AX_FALSE;
    }

    StopWorkThread();

    ret = AX_IVPS_StopGrp(m_nIvpsGrp);
    if (AX_SUCCESS != ret) {
        LOG_M_E(IVPS, "AX_IVPS_StopGrp(Grp: %d) failed, ret=0x%x", m_nIvpsGrp, ret);
    }

    for (AX_U8 chn = 0; chn < m_tIvpsGrp.tPipelineAttr.nOutChnNum; ++chn) {
        ret = AX_IVPS_DisableChn(m_nIvpsGrp, chn);
        if (AX_SUCCESS != ret) {
            LOG_M_E(IVPS, "AX_IVPS_DisableChn(Grp: %d, Channel: %d) failed, ret=0x%x", m_nIvpsGrp, chn, ret);
        }
        m_mapChnState[chn] = AX_FALSE;
    }

    ret = AX_IVPS_DestoryGrp(m_nIvpsGrp);
    if (AX_SUCCESS != ret) {
        LOG_M_E(IVPS, "AX_IVPS_DestoryGrp(Grp: %d) failed, ret=0x%x", m_nIvpsGrp, ret);
    }

    m_bStarted = AX_FALSE;

    LOG_MM_I(IVPS, "[%d] ---", m_nIvpsGrp);

    return CAXStage::Stop();
}

AX_VOID CIVPSGrpStage::FrameGetThreadFunc(IVPS_GET_THREAD_PARAM_PTR pThreadParam) {
    AX_S32 nRet = AX_SUCCESS;

    AX_U8 nIvpsGrp = pThreadParam->nIvpsGrp;
    AX_U8 nIvpsChn = pThreadParam->nIvpsChn;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "APP_IVPS_Get_%d", nIvpsGrp);
    prctl(PR_SET_NAME, szName);

    AX_BOOL bLink = m_tIvpsGrpCfg.arrChnLinkFlag[nIvpsChn] == 0 ? AX_FALSE : AX_TRUE;

    LOG_MM_I(IVPS, "[%d][%d] +++ bLink:%d ", nIvpsGrp, nIvpsChn, bLink);

    pThreadParam->bExit = AX_FALSE;
    while (!pThreadParam->bExit) {
        if (!pThreadParam->nChnEnable) {
            CElapsedTimer::GetInstance()->mSleep(100);
            continue;
        }

        if (bLink) {
            CElapsedTimer::GetInstance()->mSleep(200);
            bLink = m_tIvpsGrpCfg.arrChnLinkFlag[nIvpsChn] == 0 ? AX_FALSE : AX_TRUE;
            continue;
        }

        CAXFrame* pFrame = new (std::nothrow) CAXFrame();
        if (!pFrame) {
            LOG_M_E(IVPS, "alloc MediaFrame instance fail");
            continue;
        }

        nRet = AX_IVPS_GetChnFrame(nIvpsGrp, nIvpsChn, &pFrame->stFrame.stVFrame.stVFrame, 95);
        if (AX_SUCCESS != nRet) {
            if (AX_ERR_IVPS_BUF_EMPTY == nRet) {
                delete pFrame;
                CElapsedTimer::GetInstance()->mSleep(1);
                continue;
            }
            delete pFrame;
            LOG_M(IVPS, "[%d][%d] Get ivps frame failed. ret=0x%x", nIvpsGrp, nIvpsChn, nRet);
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }
        LOG_MM_D(IVPS, "[%d][%d] Seq: %lld, w:%d, h:%d, size:%u, release:%p,PhyAddr:%lld", nIvpsGrp, nIvpsChn,
                 pFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pFrame->stFrame.stVFrame.stVFrame.u32Width,
                 pFrame->stFrame.stVFrame.stVFrame.u32Height, pFrame->stFrame.stVFrame.stVFrame.u32FrameSize, pThreadParam->pReleaseStage,
                 pFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0]);

        pFrame->nGrp = nIvpsGrp;
        pFrame->nChn = nIvpsChn;
        pFrame->pFrameRelease = pThreadParam->pReleaseStage;
        NotifyAll(nIvpsGrp, nIvpsChn, pFrame);
    }

    LOG_MM(IVPS, "[%d][%d] ---", nIvpsGrp, nIvpsChn);
}

AX_VOID CIVPSGrpStage::VideoFrameRelease(CAXFrame* pFrame) {
    if (pFrame) {
        if (!pFrame->bMultiplex || pFrame->DecFrmRef() == 0) {
            LOG_M_D(IVPS, "[%d][%d] AX_IVPS_ReleaseChnFrame, seq=%lld, pFrqame:%p", pFrame->nGrp, pFrame->nChn,
                    pFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pFrame);
            AX_IVPS_ReleaseChnFrame(pFrame->nGrp, pFrame->nChn, &pFrame->stFrame.stVFrame.stVFrame);
            delete pFrame;
        }
    }
}

AX_BOOL CIVPSGrpStage::RecvFrame(CAXFrame* pFrame) {
    if (!EnqueueFrame(pFrame)) {
        pFrame->FreeMem();
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::ProcessFrame(CAXFrame* pFrame) {
    if (!pFrame) {
        return AX_FALSE;
    }
    AX_S32 ret = AX_IVPS_SendFrame(m_nIvpsGrp, &pFrame->stFrame.stVFrame.stVFrame, MAX_IPC_IVPS_FRAME_TIMEOUT);
    if (AX_SUCCESS != ret) {
        LOG_M_E(IVPS, "AX_IVPS_SendFrame(Grp %d, size:%u, Seq %d) failed, ret=0x%x", m_nIvpsGrp,
                pFrame->stFrame.stVFrame.stVFrame.u32FrameSize, pFrame->stFrame.stVFrame.stVFrame.u64SeqNum, ret);
        return AX_FALSE;
    } else {
        LOG_MM_D(IVPS, "[%d] AX_IVPS_SendFrame successfully, PhyAddr:0x%llx ,size:%u, seq=%d, bMux=%d.", m_nIvpsGrp,
                 pFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0], pFrame->stFrame.stVFrame.stVFrame.u32FrameSize,
                 pFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pFrame->bMultiplex);
    }

    return AX_TRUE;
}

AX_VOID CIVPSGrpStage::StartWorkThread() {
    LOG_MM_I(IVPS, "[%d] +++", m_tIvpsGrpCfg.nGrp);

    /* Start frame get thread */
    for (AX_U8 nChn = 0; nChn < m_tIvpsGrp.tPipelineAttr.nOutChnNum; ++nChn) {
        /* get thread param */
        m_tGetThreadParam[nChn].nChnEnable = AX_TRUE;
        m_tGetThreadParam[nChn].nIvpsGrp = m_nIvpsGrp;
        m_tGetThreadParam[nChn].nIvpsChn = nChn;
        m_tGetThreadParam[nChn].pReleaseStage = this;
        m_tGetThreadParam[nChn].bExit = AX_FALSE;
        m_hGetThread[nChn] = std::thread(&CIVPSGrpStage::FrameGetThreadFunc, this, &m_tGetThreadParam[nChn]);
    }

    LOG_MM_I(IVPS, "[%d] ---", m_tIvpsGrpCfg.nGrp);
}

AX_VOID CIVPSGrpStage::StopWorkThread() {
    LOG_MM_I(IVPS, "[%d] +++", m_tIvpsGrpCfg.nGrp);

    for (AX_U8 nChn = 0; nChn < m_tIvpsGrp.tPipelineAttr.nOutChnNum; ++nChn) {
        m_tGetThreadParam[nChn].bExit = AX_TRUE;
    }

    for (AX_U8 nChn = 0; nChn < m_tIvpsGrp.tPipelineAttr.nOutChnNum; ++nChn) {
        if (m_hGetThread[nChn].joinable()) {
            m_hGetThread[nChn].join();
        }
    }

    LOG_MM_I(IVPS, "[%d] ---", m_tIvpsGrpCfg.nGrp);
}

AX_BOOL CIVPSGrpStage::InitParams() {
    m_tIvpsGrp = IVPS_GRP_T{};
    m_tIvpsGrp.nGroup = m_nIvpsGrp;
    /* Config group attr */
    m_tIvpsGrp.tGroupAttr.nInFifoDepth = IVPS_IN_FIFO_DEPTH;
    m_tIvpsGrp.tGroupAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;

    /* Config pipeline attr */
    m_tIvpsGrp.tPipelineAttr.nOutChnNum = m_tIvpsGrpCfg.nGrpChnNum;
    m_tIvpsGrp.tPipelineAttr.nInDebugFifoDepth = 0;
    for (AX_U8 i = 0; i < m_tIvpsGrpCfg.nGrpChnNum; i++) {
        m_tIvpsGrp.tPipelineAttr.nOutFifoDepth[i] = IVPS_OUT_FIFO_DEPTH;
    }

    /* Config group filter 0 */
    m_tIvpsGrp.tPipelineAttr.tFilter[0][0].eEngine = m_tIvpsGrpCfg.eGrpEngineType0;
    if (m_tIvpsGrpCfg.eGrpEngineType0 != AX_IVPS_ENGINE_BUTT) {
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].bEngage = AX_TRUE;
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].nDstPicWidth = m_tIvpsGrpCfg.arrGrpResolution[0];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].nDstPicHeight = m_tIvpsGrpCfg.arrGrpResolution[1];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].nDstPicStride = ALIGN_UP(m_tIvpsGrpCfg.arrGrpResolution[0], 2);
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tFRC.fSrcFrameRate = m_tIvpsGrpCfg.arrGrpFramerate[0];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tFRC.fDstFrameRate = m_tIvpsGrpCfg.arrGrpFramerate[1];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)m_tIvpsGrpCfg.arrGrpFBC[0];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tCompressInfo.u32CompressLevel = m_tIvpsGrpCfg.arrGrpFBC[1];

        if (AX_IVPS_ENGINE_GDC == m_tIvpsGrp.tPipelineAttr.tFilter[0][0].eEngine) {
            /* fixme: The value should be from local file*/
            m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.eDewarpType = AX_IVPS_DEWARP_BYPASS;
            m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.eRotation = (AX_IVPS_ROTATION_E)m_tIvpsGrpCfg.nRotation;
            // m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.bFlip = (AX_BOOL)m_tIvpsGrpCfg.nFlip;
            // m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.bMirror = (AX_BOOL)m_tIvpsGrpCfg.nMirror;

            if (m_tIvpsGrpCfg.nLdcEnable) {
                m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.eDewarpType = AX_IVPS_DEWARP_LDC;
                m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.bAspect = m_tIvpsGrpCfg.bLdcAspect;
                m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nXRatio = m_tIvpsGrpCfg.nLdcXRatio;
                m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nYRatio = m_tIvpsGrpCfg.nLdcYRatio;
                m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nXYRatio = m_tIvpsGrpCfg.nLdcXYRatio;
                m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nDistortionRatio = m_tIvpsGrpCfg.nLdcDistortionRatio;
            }

        } else if (AX_IVPS_ENGINE_TDP == m_tIvpsGrp.tPipelineAttr.tFilter[0][0].eEngine) {
            m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tTdpCfg.eRotation = (AX_IVPS_ROTATION_E)m_tIvpsGrpCfg.nRotation;
            // m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tTdpCfg.bFlip = (AX_BOOL)m_tIvpsGrpCfg.nFlip;
            // m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tTdpCfg.bMirror = (AX_BOOL)m_tIvpsGrpCfg.nMirror;
        }
        m_arrChnResolution[0][0] = m_tIvpsGrpCfg.arrGrpResolution[0];
        m_arrChnResolution[0][1] = m_tIvpsGrpCfg.arrGrpResolution[1];
        m_bFBC[0][0] = (AX_BOOL)m_tIvpsGrpCfg.arrGrpFBC[0];
        m_bFBC[0][1] = (AX_BOOL)m_tIvpsGrpCfg.arrGrpFBC[1];
        LOG_M(IVPS, "[%d] Grp filter 0x00: engine:%d, w:%d, h:%d, s:%d, frameRate[%f,%f]", m_tIvpsGrp.nGroup, m_tIvpsGrpCfg.eGrpEngineType0,
              m_tIvpsGrp.tPipelineAttr.tFilter[0][0].nDstPicWidth, m_tIvpsGrp.tPipelineAttr.tFilter[0][0].nDstPicHeight,
              m_tIvpsGrp.tPipelineAttr.tFilter[0][0].nDstPicStride, m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tFRC.fSrcFrameRate,
              m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tFRC.fDstFrameRate);
    }

    /* Config group filter 1 */
    m_tIvpsGrp.tPipelineAttr.tFilter[0][1].eEngine = m_tIvpsGrpCfg.eGrpEngineType1;
    if (m_tIvpsGrpCfg.eGrpEngineType1 != AX_IVPS_ENGINE_BUTT) {
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].bEngage = AX_TRUE;
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].nDstPicWidth =
            m_tIvpsGrpCfg.arrGrpResolution[0]; /* Group filter 1 has not ability of resizing, just use group filter0's out resolution */
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].nDstPicHeight = m_tIvpsGrpCfg.arrGrpResolution[1];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].nDstPicStride = ALIGN_UP(m_tIvpsGrpCfg.arrGrpResolution[0], 2);
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)m_tIvpsGrpCfg.arrGrpFBC[0];
        m_tIvpsGrp.tPipelineAttr.tFilter[0][1].tCompressInfo.u32CompressLevel = m_tIvpsGrpCfg.arrGrpFBC[1];

        m_arrChnResolution[0][0] = m_tIvpsGrpCfg.arrGrpResolution[0];
        m_arrChnResolution[0][1] = m_tIvpsGrpCfg.arrGrpResolution[1];
        m_bFBC[0][0] = (AX_BOOL)m_tIvpsGrpCfg.arrGrpFBC[0];
        m_bFBC[0][1] = (AX_BOOL)m_tIvpsGrpCfg.arrGrpFBC[1];
        LOG_M(IVPS, "[%d] Grp filter 0x01: engine:%d, w:%d, h:%d, s:%d, frameRate[%f,%f]", m_tIvpsGrp.nGroup, m_tIvpsGrpCfg.eGrpEngineType1,
              m_tIvpsGrp.tPipelineAttr.tFilter[0][1].nDstPicWidth, m_tIvpsGrp.tPipelineAttr.tFilter[0][1].nDstPicHeight,
              m_tIvpsGrp.tPipelineAttr.tFilter[0][1].nDstPicStride, m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tFRC.fSrcFrameRate,
              m_tIvpsGrp.tPipelineAttr.tFilter[0][0].tFRC.fDstFrameRate);
    }

    /* Config channel filter 0 */
    for (AX_U8 i = 0; i < m_tIvpsGrpCfg.nGrpChnNum; i++) {
        if (m_tIvpsGrpCfg.arrChnEngineType0[i] != AX_IVPS_ENGINE_BUTT) {
            AX_U8 nChnFilter = i + 1;
            AX_U32 nStride = m_tIvpsGrpCfg.arrChnFBC[i][0] != AX_COMPRESS_MODE_NONE ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : 2;

            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].bEngage = AX_TRUE;
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].eEngine = m_tIvpsGrpCfg.arrChnEngineType0[i];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;

            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicStride = ALIGN_UP(m_tIvpsGrpCfg.arrChnResolution[i][0], nStride);
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tFRC.fSrcFrameRate = (m_tIvpsGrpCfg.arrChnFramerate[i][0]);
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tFRC.fDstFrameRate = (m_tIvpsGrpCfg.arrChnFramerate[i][1]);
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.enCompressMode =
                (AX_COMPRESS_MODE_E)m_tIvpsGrpCfg.arrChnFBC[i][0];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.u32CompressLevel = m_tIvpsGrpCfg.arrChnFBC[i][1];

            if (m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].eEngine == AX_IVPS_ENGINE_TDP) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].bInplace = m_tIvpsGrpCfg.bAarrChnInplace[i];
            } else if (m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].eEngine == AX_IVPS_ENGINE_VPP) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].bInplace = m_tIvpsGrpCfg.bAarrChnInplace[i];
            } else if (m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].eEngine == AX_IVPS_ENGINE_VGP) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].bInplace = m_tIvpsGrpCfg.bAarrChnInplace[i];
            }
            if (AX_IVPS_ROTATION_90 == m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tTdpCfg.eRotation ||
                AX_IVPS_ROTATION_270 == m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tTdpCfg.eRotation) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicWidth =
                    ALIGN_UP(m_tIvpsGrpCfg.arrChnResolution[i][1], AX_IVPS_FBC_WIDTH_ALIGN_VAL);
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicHeight = m_tIvpsGrpCfg.arrChnResolution[i][0];
            } else {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicWidth = m_tIvpsGrpCfg.arrChnResolution[i][0];
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicHeight = m_tIvpsGrpCfg.arrChnResolution[i][1];
            }
            m_arrChnResolution[i][0] = m_tIvpsGrpCfg.arrChnResolution[i][0];
            m_arrChnResolution[i][1] = m_tIvpsGrpCfg.arrChnResolution[i][1];
            m_bFBC[i][0] = (AX_BOOL)m_tIvpsGrpCfg.arrChnFBC[i][0];
            m_bFBC[i][1] = (AX_BOOL)m_tIvpsGrpCfg.arrChnFBC[i][1];
            LOG_M(IVPS, "[%d][%d] Chn filter 0x%02x: engine:%d, w:%d, h:%d, s:%d, inFPS:%d, outFPS:%d, Compress(%d, %d)", m_tIvpsGrp.nGroup,
                  i, nChnFilter << 4, m_tIvpsGrpCfg.arrChnEngineType0[i], m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicWidth,
                  m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicHeight,
                  m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].nDstPicStride,
                  m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tFRC.fSrcFrameRate,
                  m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tFRC.fDstFrameRate,
                  m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.enCompressMode,
                  m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.u32CompressLevel);
        }
    }

    /* Config channel filter 1 */
    for (AX_U8 i = 0; i < m_tIvpsGrpCfg.nGrpChnNum; i++) {
        if (m_tIvpsGrpCfg.arrChnEngineType1[i] != AX_IVPS_ENGINE_BUTT) {
            AX_U8 nChnFilter = i + 1;
            AX_U32 nStride = m_tIvpsGrpCfg.arrChnFBC[i][0] != AX_COMPRESS_MODE_NONE ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : 2;

            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].bEngage = AX_TRUE;
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].eEngine = m_tIvpsGrpCfg.arrChnEngineType1[i];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].nDstPicWidth = m_tIvpsGrpCfg.arrChnResolution[i][0];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].nDstPicHeight = m_tIvpsGrpCfg.arrChnResolution[i][1];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].nDstPicStride = ALIGN_UP(m_tIvpsGrpCfg.arrChnResolution[i][0], nStride);
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].tFRC.fSrcFrameRate = m_tIvpsGrpCfg.arrChnFramerate[i][0];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].tFRC.fDstFrameRate = m_tIvpsGrpCfg.arrChnFramerate[i][1];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].tCompressInfo.enCompressMode =
                (AX_COMPRESS_MODE_E)m_tIvpsGrpCfg.arrChnFBC[i][0];
            m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].tCompressInfo.u32CompressLevel = m_tIvpsGrpCfg.arrChnFBC[i][1];

            m_arrChnResolution[i][0] = m_tIvpsGrpCfg.arrChnResolution[i][0];
            m_arrChnResolution[i][1] = m_tIvpsGrpCfg.arrChnResolution[i][1];
            m_bFBC[i][0] = (AX_BOOL)m_tIvpsGrpCfg.arrChnFBC[i][0];
            m_bFBC[i][1] = (AX_BOOL)m_tIvpsGrpCfg.arrChnFBC[i][1];
            if (m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].eEngine == AX_IVPS_ENGINE_TDP) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].bInplace = m_tIvpsGrpCfg.bAarrChnInplace[i];
            } else if (m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].eEngine == AX_IVPS_ENGINE_VPP) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].bInplace = m_tIvpsGrpCfg.bAarrChnInplace[i];
            } else if (m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].eEngine == AX_IVPS_ENGINE_VGP) {
                m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].bInplace = m_tIvpsGrpCfg.bAarrChnInplace[i];
            }

            LOG_MM(IVPS, "[%d][%d] Chn filter 0x%02x: engine:%d, w:%d, h:%d, s:%d,frameRate[%f,%f]", m_tIvpsGrp.nGroup, i,
                   (nChnFilter << 4) + 1, m_tIvpsGrpCfg.arrChnEngineType1[i], m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].nDstPicWidth,
                   m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].nDstPicHeight,
                   m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].nDstPicStride, m_tIvpsGrpCfg.arrChnFramerate[i][0],
                   m_tIvpsGrpCfg.arrChnFramerate[i][1]);
        }
    }

    return AX_TRUE;
}

AX_VOID CIVPSGrpStage::SetChnInplace(AX_S32 nChannel, AX_BOOL bEnable) {
    AX_S32 nChnFilter = nChannel + 1;
    m_tIvpsGrp.tPipelineAttr.tFilter[nChnFilter][1].bInplace = bEnable;
}

AX_VOID CIVPSGrpStage::RegObserver(AX_S32 nChannel, IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.fSrcFramerate = m_tIvpsGrpCfg.arrChnFramerate[nChannel][0]; /*0: src framerate*/
        tTransAttr.fFramerate = m_tIvpsGrpCfg.arrChnFramerate[nChannel][1];    /* 1: Out framerate */
        tTransAttr.nWidth = m_tIvpsGrpCfg.arrChnResolution[nChannel][0];
        tTransAttr.nHeight = m_tIvpsGrpCfg.arrChnResolution[nChannel][1];
        tTransAttr.bEnableFBC = m_tIvpsGrpCfg.arrChnFBC[nChannel][0] == 0 ? AX_FALSE : AX_TRUE;
        tTransAttr.bLink = m_tIvpsGrpCfg.arrChnLinkFlag[nChannel] == 0 ? AX_FALSE : AX_TRUE;
        tTransAttr.nSnsSrc = m_tIvpsGrpCfg.nSnsSrc;
        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_IVPS, m_nIvpsGrp, nChannel, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CIVPSGrpStage::UnregObserver(AX_S32 nChannel, IObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        if (*it == pObserver) {
            m_vecObserver.erase(it);
            break;
        }
    }
}

AX_VOID CIVPSGrpStage::NotifyAll(AX_U32 nGrp, AX_U32 nChn, AX_VOID* pFrame) {
    if (m_vecObserver.size() == 0) {
        ((CAXFrame*)pFrame)->FreeMem();
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_IVPS, m_nIvpsGrp, nChn, pFrame);
    }
}

AX_BOOL CIVPSGrpStage::EnableChannel(AX_U8 nChn, AX_BOOL bEnable /*= AX_TRUE*/) {
    AX_S32 nRet = AX_SUCCESS;
    if (bEnable) {
        nRet = AX_IVPS_EnableChn(m_tIvpsGrpCfg.nGrp, nChn);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(IVPS, "AX_IVPS_EnableChn(Grp:%d, Chn:%d) failed, ret=0x%x", m_tIvpsGrpCfg.nGrp, nChn, nRet);
            return AX_FALSE;
        } else {
            LOG_M(IVPS, "AX_IVPS_EnableChn(Grp:%d, Chn:%d) successfully", m_tIvpsGrpCfg.nGrp, nChn);
        }
    } else {
        nRet = AX_IVPS_DisableChn(m_tIvpsGrpCfg.nGrp, nChn);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(IVPS, "AX_IVPS_DisableChn(Grp:%d, Chn:%d) failed, ret=0x%x", m_tIvpsGrpCfg.nGrp, nChn, nRet);
            return AX_FALSE;
        } else {
            LOG_M(IVPS, "AX_IVPS_DisableChn(Grp:%d, Chn:%d) successfully", m_tIvpsGrpCfg.nGrp, nChn);
        }
    }
    m_tGetThreadParam[nChn].nChnEnable = bEnable;
    m_mapChnState[nChn] = bEnable;
    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::UpdateRotationResolution(AX_IVPS_ROTATION_E eRotation, AX_U8 nChn, AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nStrideAlign = 0;
    AX_U32 nWidthAlign = 2;
    AX_IVPS_PIPELINE_ATTR_T& tPipelineAttr = m_tIvpsGrp.tPipelineAttr;
    AX_BOOL bFBC;

    /*Disable fbc when Rotation 90/270 */
    if (AX_IVPS_ROTATION_0 == eRotation || AX_IVPS_ROTATION_180 == eRotation) {
        bFBC = (AX_BOOL)m_bFBC[nChn][0];
    } else {
        bFBC = AX_FALSE;
    }

    if (0 == nChn) {
        /* Update Grp filter0 resolution */
        if (tPipelineAttr.tFilter[0][0].bEngage) {
            nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_FBC_WIDTH_ALIGN_VAL;
            nWidthAlign = bFBC ? AX_IVPS_FBC_WIDTH_ALIGN_VAL : 2;

            tPipelineAttr.tFilter[0][0].nDstPicWidth = ALIGN_UP(nWidth, nWidthAlign);
            tPipelineAttr.tFilter[0][0].nDstPicHeight = nHeight;

            tPipelineAttr.tFilter[0][0].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);
            if (AX_FALSE == bFBC) {
                tPipelineAttr.tFilter[0][0].tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
                tPipelineAttr.tFilter[0][0].tCompressInfo.u32CompressLevel = 0;
            } else {
                tPipelineAttr.tFilter[0][0].tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)m_bFBC[nChn][0];
                tPipelineAttr.tFilter[0][0].tCompressInfo.u32CompressLevel = m_bFBC[nChn][1];
            }
            LOG_MM_C(IVPS, "GrpFilter0 Reset resolution for ivps (Grp: %d): w: %d, h: %d, s: %d compress[%d,%d].", m_tIvpsGrpCfg.nGrp,
                     tPipelineAttr.tFilter[0][0].nDstPicWidth, tPipelineAttr.tFilter[0][0].nDstPicHeight,
                     tPipelineAttr.tFilter[0][0].nDstPicStride, tPipelineAttr.tFilter[0][0].tCompressInfo.enCompressMode,
                     tPipelineAttr.tFilter[0][0].tCompressInfo.u32CompressLevel);
        }
        /* Update Grp filter1 resolution */
        if (tPipelineAttr.tFilter[0][1].bEngage) {
            nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_FBC_WIDTH_ALIGN_VAL;
            nWidthAlign = bFBC ? AX_IVPS_FBC_WIDTH_ALIGN_VAL : 2;
            tPipelineAttr.tFilter[0][1].nDstPicWidth = ALIGN_UP(nWidth, nWidthAlign);
            tPipelineAttr.tFilter[0][1].nDstPicHeight = nHeight;

            tPipelineAttr.tFilter[0][1].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);
            if (AX_FALSE == bFBC) {
                tPipelineAttr.tFilter[0][1].tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
                tPipelineAttr.tFilter[0][1].tCompressInfo.u32CompressLevel = 0;
            } else {
                tPipelineAttr.tFilter[0][1].tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)m_bFBC[nChn][0];
                tPipelineAttr.tFilter[0][1].tCompressInfo.u32CompressLevel = m_bFBC[nChn][1];
            }
            LOG_MM_C(IVPS, "GrpFilter1 Reset resolution for ivps (Grp: %d): w: %d, h: %d, s: %d compress[%d,%d].", m_tIvpsGrpCfg.nGrp,
                     tPipelineAttr.tFilter[0][1].nDstPicWidth, tPipelineAttr.tFilter[0][1].nDstPicHeight,
                     tPipelineAttr.tFilter[0][1].nDstPicStride, tPipelineAttr.tFilter[0][0].nDstPicStride,
                     tPipelineAttr.tFilter[0][1].tCompressInfo.enCompressMode, tPipelineAttr.tFilter[0][1].tCompressInfo.u32CompressLevel);
        }
    }
    /* Update chn filter resolution */
    if (tPipelineAttr.tFilter[nChn + 1][0].bEngage) {
        nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_FBC_WIDTH_ALIGN_VAL;
        nWidthAlign = bFBC ? AX_IVPS_FBC_WIDTH_ALIGN_VAL : 2;

        /* Update resolution of channel filter 0 */
        tPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = ALIGN_UP(nWidth, nWidthAlign);
        tPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = nHeight;
        tPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);
        if (AX_FALSE == bFBC) {
            tPipelineAttr.tFilter[nChn + 1][0].tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
            tPipelineAttr.tFilter[nChn + 1][0].tCompressInfo.u32CompressLevel = 0;
        } else {
            tPipelineAttr.tFilter[nChn + 1][0].tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)m_bFBC[nChn][0];
            tPipelineAttr.tFilter[nChn + 1][0].tCompressInfo.u32CompressLevel = m_bFBC[nChn][1];
        }

        LOG_MM(IVPS, "ChnFilter0 reset resolution for ivps (Grp: %d, Chn: %d): w: %d, h: %d, s: %d strdeAlign:%d, widthAlign:%d.",
               m_tIvpsGrpCfg.nGrp, nChn, tPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth, tPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight,
               tPipelineAttr.tFilter[nChn + 1][0].nDstPicStride, nStrideAlign, nWidthAlign);
    }

    /* Update resolution of channel filter 1 */
    if (tPipelineAttr.tFilter[nChn + 1][1].bEngage) {
        nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_FBC_WIDTH_ALIGN_VAL;
        nWidthAlign = bFBC ? AX_IVPS_FBC_WIDTH_ALIGN_VAL : 2;

        tPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = ALIGN_UP(nWidth, nWidthAlign);
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = nHeight;
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);
        if (AX_FALSE == bFBC) {
            tPipelineAttr.tFilter[nChn + 1][1].tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
            tPipelineAttr.tFilter[nChn + 1][1].tCompressInfo.u32CompressLevel = 0;
        } else {
            tPipelineAttr.tFilter[nChn + 1][1].tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)m_bFBC[nChn][0];
            tPipelineAttr.tFilter[nChn + 1][1].tCompressInfo.u32CompressLevel = m_bFBC[nChn][1];
        }

        LOG_MM(IVPS, "ChnFilter1 reset resolution for ivps (Grp: %d, Chn: %d): w: %d, h: %d, s: %d strdeAlign:%d, widthAlign:%d.",
               m_tIvpsGrpCfg.nGrp, nChn, tPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth, tPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight,
               tPipelineAttr.tFilter[nChn + 1][1].nDstPicStride, nStrideAlign, nWidthAlign);
    }

    AX_S32 nRet = AX_IVPS_SetPipelineAttr(m_tIvpsGrpCfg.nGrp, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(IVPS, "AX_IVPS_SetPipelineAttr(Grp %d) failed, ret=0x%x", m_tIvpsGrpCfg.nGrp, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::UpdateChnResolution(AX_U8 nChn, AX_S32 nWidth, AX_S32 nHeight) {
    AX_IVPS_PIPELINE_ATTR_T& tPipelineAttr = m_tIvpsGrp.tPipelineAttr;
    AX_BOOL bFBC;
    /*Disable fbc when Rotation 90/270 */
    if (AX_IVPS_ROTATION_0 == m_eRotation || AX_IVPS_ROTATION_180 == m_eRotation) {
        bFBC = (AX_BOOL)m_bFBC[nChn][0];
    } else {
        bFBC = AX_FALSE;
    }
    AX_U32 nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_FBC_WIDTH_ALIGN_VAL;
    AX_U32 nWidthAlign = bFBC ? AX_IVPS_FBC_WIDTH_ALIGN_VAL : 2;

    m_arrChnResolution[nChn][0] = nWidth;
    m_arrChnResolution[nChn][1] = nHeight;
    if (nWidth == m_tIvpsGrpCfg.arrChnResolution[nChn][0] && nHeight == m_tIvpsGrpCfg.arrChnResolution[nChn][1]) {
        /* Case of recovering to the original configured resolution: disable the manually enabled channel filter 0 */
        if (AX_TRUE == tPipelineAttr.tFilter[nChn + 1][0].bEngage) {
            tPipelineAttr.tFilter[nChn + 1][0].bEngage = AX_FALSE;
        }

        /* Update resolution of channel filter 1 */
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = nWidth;
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = nHeight;
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);
    } else {
        if (AX_IVPS_ROTATION_90 == m_eRotation || AX_IVPS_ROTATION_270 == m_eRotation) {
            std::swap(nWidth, nHeight);
        }
        if (AX_FALSE == tPipelineAttr.tFilter[nChn + 1][0].bEngage) {
            /* Enable channel filter 0 */
            tPipelineAttr.tFilter[nChn + 1][0].bEngage = AX_TRUE;
            tPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_VPP;
            tPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
            tPipelineAttr.tFilter[nChn + 1][0].bInplace = AX_FALSE;
            tPipelineAttr.tFilter[nChn + 1][0].tFRC.fSrcFrameRate = tPipelineAttr.tFilter[nChn + 1][1].tFRC.fSrcFrameRate;
            tPipelineAttr.tFilter[nChn + 1][0].tFRC.fDstFrameRate = tPipelineAttr.tFilter[nChn + 1][1].tFRC.fSrcFrameRate;
            tPipelineAttr.tFilter[nChn + 1][0].tCompressInfo = tPipelineAttr.tFilter[nChn + 1][1].tCompressInfo;
        }

        /* Update resolution of channel filter 0 */
        tPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = ALIGN_UP(nWidth, nWidthAlign);
        tPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = nHeight;
        tPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);

        /* Update resolution of channel filter 1 */
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = ALIGN_UP(nWidth, nWidthAlign);
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = nHeight;
        tPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = ALIGN_UP(nWidth, nStrideAlign);
    }

    LOG_MM(IVPS, "ChnFilter reset resolution for ivps (Grp: %d, Chn: %d): w: %d, h: %d, s: %d nWidthAlign:%d nStrideAlign:%d.",
           m_tIvpsGrpCfg.nGrp, nChn, tPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth, tPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight,
           tPipelineAttr.tFilter[nChn + 1][1].nDstPicStride, nWidthAlign, nStrideAlign);

    AX_S32 nRet = AX_IVPS_SetPipelineAttr(m_tIvpsGrpCfg.nGrp, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(IVPS, "AX_IVPS_SetPipelineAttr(Grp %d) failed, ret=0x%x", m_tIvpsGrpCfg.nGrp, nRet);
        return AX_FALSE;
    }
    m_tIvpsGrpCfg.arrChnResolution[nChn][0] = tPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth;
    m_tIvpsGrpCfg.arrChnResolution[nChn][1] = tPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight;

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::UpdateRotation(AX_IVPS_ROTATION_E eRotation) {
    AX_IVPS_PIPELINE_ATTR_T& tPipelineAttr = m_tIvpsGrp.tPipelineAttr;
    AX_U8 nOutChnNum = tPipelineAttr.nOutChnNum;
    for (AX_U8 nChn = 0; nChn < nOutChnNum; nChn++) {
        AX_U32 nNewWidth = 0;
        AX_U32 nNewHeight = 0;
        if (!GetResolutionByRotate(nChn, eRotation, nNewWidth, nNewHeight)) {
            LOG_MM_E(IVPS, "[%d][%d] Can not get new resolution for rotate operation.", m_tIvpsGrpCfg.nGrp, nChn);
            continue;
        }
        if (m_tIvpsGrpCfg.bRotationEngine) {
            /* Config channel filter 0 */
            if (!UpdateGrpRotation(0, eRotation, nNewWidth, nNewHeight)) {
                LOG_MM_E(IVPS, "[%d][%d] Rotate operation failed.", m_tIvpsGrpCfg.nGrp, nChn);
                return AX_FALSE;
            }
        } else {
            if (AX_FALSE == m_tIvpsGrpCfg.nLdcEnable) {
                UpdateRotationResolution(eRotation, nChn, nNewWidth, nNewHeight);
            }
        }
        m_eRotation = eRotation;
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::UpdateGrpRotation(AX_U8 nGrpFilterIndex, AX_IVPS_ROTATION_E eRotation, AX_U32 nWidth, AX_U32 nHeight) {
    AX_S32 nRet = AX_SUCCESS;

    AX_IVPS_PIPELINE_ATTR_T& tPipelineAttr = m_tIvpsGrp.tPipelineAttr;
    AX_U32 nGrpFilter = (0 << 4) + (AX_U32)nGrpFilterIndex;
    if (AX_FALSE == tPipelineAttr.tFilter[0][nGrpFilterIndex].bEngage) {
        LOG_MM_E(IVPS, "[%d] Group filter %d is not enabled.", m_tIvpsGrpCfg.nGrp, nGrpFilterIndex);
        return AX_FALSE;
    }

    tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicWidth = nWidth;
    tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicHeight = nHeight;

    AX_U32 nAlign;
    if (m_tIvpsGrpCfg.arrGrpFBC[0] != AX_COMPRESS_MODE_NONE) {
        nAlign = AX_IVPS_FBC_STRIDE_ALIGN_VAL;
    } else {
        nAlign = AX_IVPS_FBC_WIDTH_ALIGN_VAL;
    }

    tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicStride = ALIGN_UP(tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicWidth, nAlign);

    LOG_MM(IVPS, "[%d][0x%02X] IVPS change group attr for rotation: rotation:%d, w:%d, h:%d, s:%d", m_tIvpsGrpCfg.nGrp, nGrpFilter,
           eRotation, tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicWidth, tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicHeight,
           tPipelineAttr.tFilter[0][nGrpFilterIndex].nDstPicStride);

    if (AX_IVPS_ENGINE_TDP == tPipelineAttr.tFilter[0][nGrpFilterIndex].eEngine) {
        tPipelineAttr.tFilter[0][nGrpFilterIndex].tTdpCfg.eRotation = eRotation;
    } else if (AX_IVPS_ENGINE_GDC == tPipelineAttr.tFilter[0][nGrpFilterIndex].eEngine) {
        tPipelineAttr.tFilter[0][nGrpFilterIndex].tGdcCfg.eRotation = eRotation;
    }

    nRet = AX_IVPS_SetPipelineAttr(m_tIvpsGrpCfg.nGrp, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
        LOG_MM_E(IVPS, "AX_IVPS_SetPipelineAttr(Grp %d) failed, ret=0x%x", m_tIvpsGrpCfg.nGrp, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::UpdateGrpLDC(AX_BOOL bLdcEnable, AX_BOOL bAspect, AX_S16 nXRatio, AX_S16 nYRatio, AX_S16 nXYRatio,
                                    AX_S16 nDistorRatio) {
    AX_IVPS_PIPELINE_ATTR_T& tPipelineAttr = m_tIvpsGrp.tPipelineAttr;
    if (AX_IVPS_ENGINE_GDC == tPipelineAttr.tFilter[0][0].eEngine) {
        if (bLdcEnable) {
            tPipelineAttr.tFilter[0][0].tGdcCfg.eDewarpType = AX_IVPS_DEWARP_LDC;
            tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.bAspect = bAspect;
            tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nXRatio = nXRatio;
            tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nYRatio = nYRatio;
            tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nXYRatio = nXYRatio;
            tPipelineAttr.tFilter[0][0].tGdcCfg.tLdcAttr.nDistortionRatio = nDistorRatio;
        } else {
            tPipelineAttr.tFilter[0][0].tGdcCfg.eDewarpType = AX_IVPS_DEWARP_BYPASS;
        }

        AX_S32 nRet = AX_IVPS_SetPipelineAttr(m_tIvpsGrpCfg.nGrp, &tPipelineAttr);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(IVPS, "AX_IVPS_SetPipelineAttr(Grp %d) failed, ret=0x%x", m_tIvpsGrpCfg.nGrp, nRet);
            return AX_FALSE;
        }

        return AX_TRUE;
    } else {
        return AX_FALSE;
    }
}

AX_VOID CIVPSGrpStage::RefreshOSDByResChange() {
    if (m_pOsdHelper) {
        m_pOsdHelper->Refresh();
    }
}

AX_BOOL CIVPSGrpStage::GetResolutionByRotate(AX_U8 nChn, AX_IVPS_ROTATION_E eRotation, AX_U32& nWidth, AX_U32& nHeight) {
    nWidth = m_arrChnResolution[nChn][0];
    nHeight = m_arrChnResolution[nChn][1];

    if (AX_IVPS_ROTATION_90 == eRotation || AX_IVPS_ROTATION_270 == eRotation) {
        std::swap(nWidth, nHeight);
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::StartOSD() {
    LOG_MM_W(IVPS, "[%d]StartOSD ...", m_tIvpsGrpCfg.nGrp);
    if (COptionHelper::GetInstance()->IsEnableOSD() && m_pOsdHelper && !m_pOsdHelper->StartOSD(this)) {
        LOG_M_E(IVPS, "Start OSD failed.");
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CIVPSGrpStage::StopOSD() {
    LOG_MM_W(IVPS, "[%d]StopOSD ...", m_tIvpsGrpCfg.nGrp);

    if (COptionHelper::GetInstance()->IsEnableOSD() && m_pOsdHelper && !m_pOsdHelper->StopOSD()) {
        LOG_M_E(IVPS, "Stop OSD failed.");
        return AX_FALSE;
    }

    return AX_TRUE;
}
