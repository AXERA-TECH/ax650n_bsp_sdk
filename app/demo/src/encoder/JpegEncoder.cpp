/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "JpegEncoder.h"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "PrintHelper.h"
#include "ax_venc_api.h"

#define JENC "JENC"

using namespace std;

CJpegEncoder::CJpegEncoder(JPEG_CONFIG_T& tConfig) : CAXStage((string)JENC + (char)('0' + tConfig.nChannel)), m_tJpegConfig(tConfig) {
}

CJpegEncoder::~CJpegEncoder() {
}

AX_BOOL CJpegEncoder::Start(STAGE_START_PARAM_PTR pStartParams) {
    LOG_MM_I(JENC, "[%d] +++", GetChannel());
    AX_S32 nRet = AX_VENC_CreateChn(m_tJpegConfig.nChannel, &m_tJencChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(JENC, "[%d] AX_VENC_CreateChn failed, nRet=0x%x!", m_tJpegConfig.nChannel, nRet);
        return AX_FALSE;
    }

    // ResetQFactor(m_tJpegConfig.nQpLevel);

    StartRecv();

    StartWorkThread();

    return CAXStage::Start(pStartParams);
}

AX_BOOL CJpegEncoder::Stop() {
    LOG_MM_I(JENC, "[%d] +++", GetChannel());

    CAXStage::Stop();

    StopRecv();

    AX_VENC_DestroyChn(GetChannel());

    StopWorkThread();

    LOG_MM_I(JENC, "[%d] +++", GetChannel());
    return AX_TRUE;
}

AX_VOID CJpegEncoder::StartRecv() {
    LOG_M_I(JENC, "[%d] JENC start receive", m_tJpegConfig.nChannel);

    AX_VENC_RECV_PIC_PARAM_T tRecvParam;
    tRecvParam.s32RecvPicNum = -1;
    AX_S32 s32Ret = AX_VENC_StartRecvFrame(m_tJpegConfig.nChannel, &tRecvParam);
    if (AX_SUCCESS != s32Ret) {
        LOG_MM_E(JENC, "[%d] AX_VENC_StartRecvFrame failed. ret=0x%02x", s32Ret);
    }

    return;
}

AX_VOID CJpegEncoder::StopRecv() {
    LOG_M_I(JENC, "[%d] JENC stop receive ", m_tJpegConfig.nChannel);

    AX_S32 ret = AX_VENC_StopRecvFrame((VENC_CHN)m_tJpegConfig.nChannel);
    if (AX_SUCCESS != ret) {
        LOG_M_E(JENC, "[%d] AX_VENC_StopRecvFrame failed, ret=0x%x", m_tJpegConfig.nChannel, ret);
    }
}

AX_VOID CJpegEncoder::ResetChn() {
    LOG_M_C(JENC, "[%d] JENC reset chn", m_tJpegConfig.nChannel);
    AX_S32 ret = AX_VENC_ResetChn((VENC_CHN)m_tJpegConfig.nChannel);
    if (AX_SUCCESS != ret) {
        LOG_M_E(JENC, "[%d] AX_VENC_ResetChn failed, ret=0x%x", m_tJpegConfig.nChannel, ret);
    }
    return;
}

AX_VOID CJpegEncoder::StartWorkThread() {
    m_bGetThreadRunning = AX_TRUE;
    m_hGetThread = std::thread(&CJpegEncoder::FrameGetThreadFunc, this, this);
}

AX_VOID CJpegEncoder::StopWorkThread() {
    LOG_MM_I(JENC, "[%d] +++", m_tJpegConfig.nChannel);

    m_bGetThreadRunning = AX_FALSE;
    if (m_hGetThread.joinable()) {
        m_hGetThread.join();
    }

    LOG_MM_I(JENC, "[%d] ---", m_tJpegConfig.nChannel);
}

AX_BOOL CJpegEncoder::Init() {
    SetCapacity(AX_APP_LOCKQ_CAPACITY);
    return AX_TRUE;
}

AX_BOOL CJpegEncoder::DeInit() {
    if (m_bGetThreadRunning) {
        Stop();
    }
    if (m_pFramectrl) {
        delete m_pFramectrl;
    }

    return AX_TRUE;
}

AX_VOID CJpegEncoder::RegObserver(IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nWidth = m_tJpegConfig.nWidth;
        tTransAttr.nHeight = m_tJpegConfig.nHeight;

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_JENC, m_tJpegConfig.nPipeSrc, m_tJpegConfig.nChannel, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CJpegEncoder::UnregObserver(IObserver* pObserver) {
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

AX_VOID CJpegEncoder::NotifyAll(AX_U32 nChannel, AX_VOID* pStream) {
    if (nullptr == pStream) {
        return;
    }
    CPrintHelper::GetInstance()->Add(E_PH_MOD_JENC, nChannel);

    // To prevent lag in the web page, control the frame rate sent to the web page.
    if (AX_FALSE == m_pFramectrl->FramerateCtrl()) {
        for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
            (*it)->OnRecvData(E_OBS_TARGET_TYPE_JENC, m_tJpegConfig.nPipeSrc, nChannel, pStream);
        }
    }
}

AX_BOOL CJpegEncoder::InitParams() {
    memset(&m_tJencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    m_tJencChnAttr.stVencAttr.enMemSource = m_tJpegConfig.eMemSource;

    m_tJencChnAttr.stVencAttr.u32MaxPicWidth = ALIGN_UP(m_tJpegConfig.nWidth, AX_ENCODER_FBC_STRIDE_ALIGN_VAL);
    m_tJencChnAttr.stVencAttr.u32MaxPicHeight = ALIGN_UP(m_tJpegConfig.nHeight, AX_ENCODER_FBC_STRIDE_ALIGN_VAL);

    m_tJencChnAttr.stVencAttr.u32PicWidthSrc = m_tJpegConfig.nWidth;
    m_tJencChnAttr.stVencAttr.u32PicHeightSrc = m_tJpegConfig.nHeight;
    m_tJencChnAttr.stVencAttr.u32BufSize = m_tJpegConfig.nBufSize; /*stream buffer size*/

    m_tJencChnAttr.stVencAttr.u8InFifoDepth = m_tJpegConfig.nInFifoDepth;
    m_tJencChnAttr.stVencAttr.u8OutFifoDepth = m_tJpegConfig.nOutFifoDepth;

    m_tJencChnAttr.stVencAttr.enLinkMode = m_tJpegConfig.bLink ? AX_VENC_LINK_MODE : AX_VENC_UNLINK_MODE;

    m_tJencChnAttr.stVencAttr.enType = PT_JPEG;
    m_tJencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = (AX_F32)m_tJpegConfig.nSrcFrameRate;
    m_tJencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = (AX_F32)m_tJpegConfig.nSrcFrameRate;
    m_pFramectrl = new CFramerateCtrlHelper(m_tJpegConfig.nSrcFrameRate, m_tJpegConfig.nDstFrameRate);
    LOG_M(JENC, "JENC attr: chn:%d, w:%d, h:%d, link:%d, memSrc:%d,frameRate[%d,%d]", m_tJpegConfig.nChannel,
          m_tJencChnAttr.stVencAttr.u32PicWidthSrc, m_tJencChnAttr.stVencAttr.u32PicHeightSrc,
          m_tJencChnAttr.stVencAttr.enLinkMode == AX_VENC_LINK_MODE ? AX_TRUE : AX_FALSE, m_tJencChnAttr.stVencAttr.enMemSource,
          m_tJpegConfig.nSrcFrameRate, m_tJpegConfig.nDstFrameRate);

    return AX_TRUE;
}

AX_BOOL CJpegEncoder::ResetQFactor(AX_U32 nQFactor) {
    AX_S32 nRet = AX_SUCCESS;

    AX_VENC_JPEG_PARAM_T stJpegParam;
    memset(&stJpegParam, 0, sizeof(AX_VENC_JPEG_PARAM_T));

    nRet = AX_VENC_GetJpegParam(m_tJpegConfig.nChannel, &stJpegParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(JENC, "[%d] AX_VENC_GetJpegParam failed!", m_tJpegConfig.nChannel, nRet);
        return AX_FALSE;
    }

    stJpegParam.u32Qfactor = nQFactor;

    nRet = AX_VENC_SetJpegParam(m_tJpegConfig.nChannel, &stJpegParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(JENC, "[%d] AX_VENC_SetJpegParam failed!", m_tJpegConfig.nChannel, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CJpegEncoder::ProcessFrame(CAXFrame* pFrame) {
    AX_S32 nRet = 0;

    nRet = AX_VENC_SendFrame(m_tJpegConfig.nChannel, &pFrame->stFrame.stVFrame, -1);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(JENC, "[%d] AX_VENC_SendFrame failed, nRet=0x%x", m_tJpegConfig.nChannel, nRet);
        return AX_FALSE;
    } else {
        LOG_M_D(JENC, "[%d] AX_VENC_SendFrame successfully.", m_tJpegConfig.nChannel);
    }

    return AX_TRUE;
}

AX_VOID CJpegEncoder::FrameGetThreadFunc(AX_VOID* pCaller) {
    AX_S32 nRet = AX_SUCCESS;

    CJpegEncoder* pThis = (CJpegEncoder*)pCaller;
    AX_S32 nChannel = pThis->m_tJpegConfig.nChannel;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "APP_JENC_Get_%d", nChannel);
    prctl(PR_SET_NAME, szName);

    AX_VENC_STREAM_T stStream;
    memset(&stStream, 0, sizeof(AX_VENC_STREAM_T));

    while (pThis->m_bGetThreadRunning) {
        m_mtx.lock();
        if(m_bPauseGet) {
            CElapsedTimer::GetInstance()->mSleep(10);
            m_mtx.unlock();
            continue;
        }
        m_mtx.unlock();

        nRet = AX_VENC_GetStream(nChannel, &stStream, 2000);
        if (AX_SUCCESS != nRet) {
            if (AX_ERR_VENC_FLOW_END == nRet) {
                pThis->m_bGetThreadRunning = AX_FALSE;
                break;
            }

            if (AX_ERR_VENC_QUEUE_EMPTY == nRet) {
                CElapsedTimer::GetInstance()->mSleep(1);
                continue;
            }

            LOG_M_E(JENC, "AX_VENC_GetStream failed with %#x!", nRet);
            continue;
        }

        if (stStream.stPack.pu8Addr && stStream.stPack.u32Len > 0) {
            LOG_MM_D(JENC, "[%d] Get jenc out stream, size=%d.", nChannel, stStream.stPack.u32Len);
            NotifyAll(nChannel, &stStream);
        }

        nRet = AX_VENC_ReleaseStream(nChannel, &stStream);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(JENC, "AX_VENC_ReleaseStream failed!");
            continue;
        }
    }
}
AX_BOOL CJpegEncoder::UpdateRotation(AX_U8 nRotation) {
    LOG_MM_C(JENC, "+++");
    AX_U32 nNewWidth = 0;
    AX_U32 nNewHeight = 0;

    SetPauseFlag(AX_TRUE);

    if (!GetResolutionByRotate(nRotation, nNewWidth, nNewHeight)) {
        LOG_MM_E(JENC, "[%d] Can not get new resolution for rotate operation.", GetChannel());
        return AX_FALSE;
    }
    AX_BOOL bFBC = m_tJpegConfig.bFBC;
    if (1 == nRotation || 3 == nRotation) {
        /* Rotation 0/180 */
        bFBC = AX_FALSE;
    }
    AX_U32 nStrideAlign = bFBC ? AX_ENCODER_FBC_STRIDE_ALIGN_VAL : AX_ENCODER_NONE_FBC_STRIDE_ALIGN_VAL;
    AX_U32 nWidthAlign = bFBC ? AX_ENCODER_FBC_WIDTH_ALIGN_VAL : 2;

    AX_VENC_CHN_ATTR_T tAttr;
    AX_VENC_GetChnAttr(GetChannel(), &tAttr);
    tAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(nNewWidth, nWidthAlign);
    tAttr.stVencAttr.u32PicHeightSrc = nNewHeight;

    tAttr.stVencAttr.u32MaxPicWidth = ALIGN_UP(nNewWidth, nStrideAlign);
    tAttr.stVencAttr.u32MaxPicHeight = ALIGN_UP(nNewHeight, nStrideAlign);
    AX_VENC_SetChnAttr(GetChannel(), &tAttr);

    SetPauseFlag(AX_FALSE);

    LOG_MM_C(JENC, "[%d] Reset res: (w: %d, h: %d) (MAX w: %d, h:%d) bFBC:%d", GetChannel(), tAttr.stVencAttr.u32PicWidthSrc,
             tAttr.stVencAttr.u32PicHeightSrc, tAttr.stVencAttr.u32MaxPicWidth, tAttr.stVencAttr.u32MaxPicHeight, bFBC);
    m_nRotation = nRotation;
    return AX_TRUE;
}

AX_BOOL CJpegEncoder::GetResolutionByRotate(AX_U8 nRotation, AX_U32& nWidth, AX_U32& nHeight) {
    nWidth = m_tJpegConfig.nWidth;
    nHeight = m_tJpegConfig.nHeight;

    if (1 == nRotation || 3 == nRotation) {
        std::swap(nWidth, nHeight);
    }

    return AX_TRUE;
}

AX_BOOL CJpegEncoder::UpdateChnResolution(JPEG_CONFIG_T& tNewConfig) {
    AX_U32 nStride = 2;
    AX_U32 nWidth = tNewConfig.nWidth;
    AX_U32 nHeight = tNewConfig.nHeight;
    m_tCurResolution.nWidth = nWidth;
    m_tCurResolution.nHeight = nHeight;
    AX_BOOL bFBC;
    /*Disable fbc when Rotation 90/270 */
    /*In fbc mode, width shoudle be Align of 128 */
    if (1 == m_nRotation || 3 == m_nRotation) {
        std::swap(nWidth, nHeight);
        bFBC = AX_FALSE;
    } else {
        bFBC = m_tJpegConfig.bFBC;
    }
    if (nWidth != m_tJpegConfig.nWidth || nHeight != m_tJpegConfig.nHeight) {
        nStride = bFBC ? AX_ENCODER_FBC_WIDTH_ALIGN_VAL : 2;
    }

    AX_VENC_CHN_ATTR_T tAttr;
    AX_VENC_GetChnAttr(GetChannel(), &tAttr);
    tAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(nWidth, nStride);
    tAttr.stVencAttr.u32PicHeightSrc = nHeight;

    AX_VENC_SetChnAttr(GetChannel(), &tAttr);

    LOG_MM_C(JENC, "[%d] Reset res: (w: %d, h: %d) max(w:%d, h:%d)", GetChannel(), tAttr.stVencAttr.u32PicWidthSrc,
             tAttr.stVencAttr.u32PicHeightSrc, tAttr.stVencAttr.u32MaxPicWidth, tAttr.stVencAttr.u32MaxPicHeight);

    return AX_TRUE;
}

AX_VOID CJpegEncoder::SetPauseFlag(AX_BOOL bPause) {
    std::lock_guard<std::mutex> lck(m_mtx);
    m_bPauseGet = bPause;
}
