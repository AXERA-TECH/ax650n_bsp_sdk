/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "VideoEncoder.h"
#include "AXTypeConverter.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "ax_venc_api.h"

#ifdef SLT
#include "PrintHelper.h"
#endif

#define VENC "VENC"

using namespace std;

CVideoEncoder::CVideoEncoder(VIDEO_CONFIG_T& tConfig) : CAXStage((string)VENC + (char)('0' + tConfig.nChannel)), m_tVideoConfig(tConfig) {
}

CVideoEncoder::~CVideoEncoder() {
}

AX_BOOL CVideoEncoder::Init() {
    SetCapacity(AX_APP_LOCKQ_CAPACITY);
    return AX_TRUE;
}

AX_BOOL CVideoEncoder::DeInit() {
    if (m_bGetThreadRunning) {
        Stop();
    }

    return AX_TRUE;
}

AX_BOOL CVideoEncoder::Start(STAGE_START_PARAM_PTR pStartParams) {
    LOG_MM_I(VENC, "+++");
    AX_S32 ret = AX_VENC_CreateChn(GetChannel(), &m_tVencChnAttr);
    if (AX_SUCCESS != ret) {
        LOG_M_E(VENC, "[%d] AX_VENC_CreateChn failed, ret=0x%x", GetChannel(), ret);
        return AX_FALSE;
    }

    StartRecv();

    StartWorkThread();

    SetSendFlag(AX_TRUE);

    LOG_MM_I(VENC, "---");

    return CAXStage::Start(pStartParams);
}

AX_BOOL CVideoEncoder::Stop() {
    LOG_MM_I(VENC, "[%d] +++", GetChannel());
    CAXStage::Stop();

    m_bGetThreadRunning = AX_FALSE;
    SetSendFlag(AX_TRUE);

    StopWorkThread();

    StopRecv();

    AX_VENC_DestroyChn(GetChannel());

    LOG_MM_I(VENC, "[%d] ---", GetChannel());

    return AX_TRUE;
}

AX_VOID CVideoEncoder::StartRecv() {
    LOG_MM_I(VENC, "[%d] VENC start receive", GetChannel());

    AX_VENC_RECV_PIC_PARAM_T tRecvParam;
    tRecvParam.s32RecvPicNum = -1;

    AX_S32 ret = AX_VENC_StartRecvFrame(GetChannel(), &tRecvParam);
    if (AX_SUCCESS != ret) {
        LOG_M_E(VENC, "[%d] AX_VENC_StartRecvFrame failed, ret=0x%x", GetChannel(), ret);
    }

    return;
}

AX_VOID CVideoEncoder::StopRecv() {
    LOG_MM_I(VENC, "[%d] VENC stop receive", GetChannel());

    AX_S32 ret = AX_VENC_StopRecvFrame((VENC_CHN)GetChannel());
    if (AX_SUCCESS != ret) {
        LOG_M_E(VENC, "[%d] AX_VENC_StopRecvFrame failed, ret=0x%x", GetChannel(), ret);
    }
    LOG_MM_I(VENC, "[%d] ---", GetChannel());
}

AX_BOOL CVideoEncoder::ResetChn() {
    LOG_MM_I(VENC, "[%d] VENC reset chn", GetChannel());
    AX_S32 ret = AX_VENC_ResetChn((VENC_CHN)GetChannel());
    if (AX_SUCCESS != ret) {
        LOG_M_E(VENC, "[%d] AX_VENC_ResetChn failed, ret=0x%x", GetChannel(), ret);
        return AX_FALSE;
    }
    LOG_MM_I(VENC, "[%d] ---", GetChannel());
    return AX_TRUE;
}

AX_VOID CVideoEncoder::StartWorkThread() {
    m_bGetThreadRunning = AX_TRUE;
    m_hGetThread = std::thread(&CVideoEncoder::FrameGetThreadFunc, this, this);
}

AX_VOID CVideoEncoder::StopWorkThread() {
    LOG_MM_I(VENC, "[%d] +++", GetChannel());

    m_bGetThreadRunning = AX_FALSE;

    if (m_hGetThread.joinable()) {
        m_hGetThread.join();
    }
    LOG_MM_I(VENC, "[%d] ---", GetChannel());
}

AX_VOID CVideoEncoder::RegObserver(IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nChannel = m_tVideoConfig.nChannel;
        tTransAttr.nWidth = m_tVideoConfig.nWidth;
        tTransAttr.nHeight = m_tVideoConfig.nHeight;
        tTransAttr.nPayloadType = m_tVideoConfig.ePayloadType;
        tTransAttr.nBitRate = m_tVideoConfig.nBitrate;

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_VENC, m_tVideoConfig.nPipeSrc, m_tVideoConfig.nChannel, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CVideoEncoder::UnregObserver(IObserver* pObserver) {
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

AX_VOID CVideoEncoder::NotifyAll(AX_U32 nChannel, AX_VOID* pStream) {
    if (nullptr == pStream) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_VENC, m_tVideoConfig.nPipeSrc, nChannel, pStream);
    }
}

AX_BOOL CVideoEncoder::InitRcParams(VIDEO_CONFIG_T& tConfig) {
    LOG_MM_I(VENC, "[%d] +++", GetChannel());
    RC_INFO_T rcInfo;
    AX_VENC_RC_MODE_E eRcType;
    switch (tConfig.ePayloadType) {
        case PT_H264: {
            // set h264 cbr
            eRcType = AX_VENC_RC_MODE_H264CBR;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH264Cbr.u32Gop = tConfig.nGOP;
                m_tVencRcParams.tH264Cbr.u32BitRate = tConfig.nBitrate;
                m_tVencRcParams.tH264Cbr.u32MinQp = ADAPTER_RANGE(rcInfo.nMinQp, 0, 51);
                m_tVencRcParams.tH264Cbr.u32MaxQp = ADAPTER_RANGE(rcInfo.nMaxQp, 0, 51);
                m_tVencRcParams.tH264Cbr.u32MinIQp = ADAPTER_RANGE(rcInfo.nMinIQp, 0, 51);
                m_tVencRcParams.tH264Cbr.u32MaxIQp = ADAPTER_RANGE(rcInfo.nMaxIQp, 0, 51);
                m_tVencRcParams.tH264Cbr.u32MinIprop = ADAPTER_RANGE(rcInfo.nMinIProp, 0, 100);
                m_tVencRcParams.tH264Cbr.u32MaxIprop = ADAPTER_RANGE(rcInfo.nMaxIProp, 0, 100);
                m_tVencRcParams.tH264Cbr.s32IntraQpDelta = ADAPTER_RANGE(rcInfo.nIntraQpDelta, -51, 51);
            }

            // set h264 vbr
            eRcType = AX_VENC_RC_MODE_H264VBR;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH264Vbr.u32Gop = tConfig.nGOP;
                m_tVencRcParams.tH264Vbr.u32MaxBitRate = tConfig.nBitrate;
                m_tVencRcParams.tH264Vbr.u32MinQp = ADAPTER_RANGE(rcInfo.nMinQp, 0, 51);
                m_tVencRcParams.tH264Vbr.u32MaxQp = ADAPTER_RANGE(rcInfo.nMaxQp, 0, 51);
                m_tVencRcParams.tH264Vbr.u32MinIQp = ADAPTER_RANGE(rcInfo.nMinIQp, 0, 51);
                m_tVencRcParams.tH264Vbr.u32MaxIQp = ADAPTER_RANGE(rcInfo.nMaxIQp, 0, 51);
                m_tVencRcParams.tH264Vbr.s32IntraQpDelta = ADAPTER_RANGE(rcInfo.nIntraQpDelta, -51, 51);
            }

            // set h264 fixQp
            eRcType = AX_VENC_RC_MODE_H264FIXQP;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH264FixQp.u32Gop = tConfig.nGOP;
                m_tVencRcParams.tH264FixQp.u32IQp = 25;
                m_tVencRcParams.tH264FixQp.u32PQp = 30;
                m_tVencRcParams.tH264FixQp.u32BQp = 32;
            }

            break;
        }
        case PT_H265: {
            // set h265 cbr
            eRcType = AX_VENC_RC_MODE_H265CBR;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH265Cbr.u32Gop = tConfig.nGOP;
                m_tVencRcParams.tH265Cbr.u32BitRate = tConfig.nBitrate;
                m_tVencRcParams.tH265Cbr.u32MinQp = ADAPTER_RANGE(rcInfo.nMinQp, 0, 51);
                m_tVencRcParams.tH265Cbr.u32MaxQp = ADAPTER_RANGE(rcInfo.nMaxQp, 0, 51);
                m_tVencRcParams.tH265Cbr.u32MinIQp = ADAPTER_RANGE(rcInfo.nMinIQp, 0, 51);
                m_tVencRcParams.tH265Cbr.u32MaxIQp = ADAPTER_RANGE(rcInfo.nMaxIQp, 0, 51);
                m_tVencRcParams.tH265Cbr.u32MinIprop = ADAPTER_RANGE(rcInfo.nMinIProp, 0, 100);
                m_tVencRcParams.tH265Cbr.u32MaxIprop = ADAPTER_RANGE(rcInfo.nMaxIProp, 0, 100);
                m_tVencRcParams.tH265Cbr.s32IntraQpDelta = ADAPTER_RANGE(rcInfo.nIntraQpDelta, -51, 51);
            }

            // set h265 vbr
            eRcType = AX_VENC_RC_MODE_H265VBR;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH265Vbr.u32Gop = tConfig.nGOP;
                m_tVencRcParams.tH265Vbr.u32MaxBitRate = tConfig.nBitrate;
                m_tVencRcParams.tH265Vbr.u32MinQp = ADAPTER_RANGE(rcInfo.nMinQp, 0, 51);
                m_tVencRcParams.tH265Vbr.u32MaxQp = ADAPTER_RANGE(rcInfo.nMaxQp, 0, 51);
                m_tVencRcParams.tH265Vbr.u32MinIQp = ADAPTER_RANGE(rcInfo.nMinIQp, 0, 51);
                m_tVencRcParams.tH265Vbr.u32MaxIQp = ADAPTER_RANGE(rcInfo.nMaxIQp, 0, 51);
                m_tVencRcParams.tH265Vbr.s32IntraQpDelta = ADAPTER_RANGE(rcInfo.nIntraQpDelta, -51, 51);
            }

            // set h265 fixQp
            eRcType = AX_VENC_RC_MODE_H265FIXQP;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH265FixQp.u32Gop = tConfig.nGOP;
                m_tVencRcParams.tH265FixQp.u32IQp = 25;
                m_tVencRcParams.tH265FixQp.u32PQp = 30;
                m_tVencRcParams.tH265FixQp.u32BQp = 32;
            }

            break;
        }
        case PT_MJPEG: {
            eRcType = AX_VENC_RC_MODE_MJPEGCBR;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tMjpegCbr.u32BitRate = tConfig.nBitrate;
                m_tVencRcParams.tMjpegCbr.u32StatTime = 1;
                m_tVencRcParams.tMjpegCbr.u32MinQp = rcInfo.nMinQp;
                m_tVencRcParams.tMjpegCbr.u32MaxQp = rcInfo.nMaxQp;
            }

            eRcType = AX_VENC_RC_MODE_MJPEGVBR;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tMjpegVbr.u32MaxBitRate = tConfig.nBitrate;
                m_tVencRcParams.tMjpegVbr.u32StatTime = 1;
                m_tVencRcParams.tMjpegVbr.u32MinQp = rcInfo.nMinQp;
                m_tVencRcParams.tMjpegVbr.u32MaxQp = rcInfo.nMaxQp;
            }

            eRcType = AX_VENC_RC_MODE_MJPEGFIXQP;
            if (AX_TRUE == m_CurEncCfg.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tMjpegFixQp.s32FixedQp = rcInfo.nFixQp;
            }

            break;
        }
        default:
            LOG_M_E(VENC, "Unrecognized payload type: %d.", tConfig.ePayloadType);
            break;
    }
    LOG_MM_I(VENC, "[%d] ---", GetChannel());

    return AX_TRUE;
}

AX_BOOL CVideoEncoder::InitParams() {
    memset(&m_tVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    m_tVencChnAttr.stVencAttr.enMemSource = m_tVideoConfig.eMemSource;

    m_tVencChnAttr.stVencAttr.u32MaxPicWidth = ALIGN_UP(m_tVideoConfig.nWidth, AX_ENCODER_FBC_STRIDE_ALIGN_VAL);
    m_tVencChnAttr.stVencAttr.u32MaxPicHeight = m_tVencChnAttr.stVencAttr.u32MaxPicWidth; /* Make max height same as width for the case of rotation 90  */
    if (!m_tCurResolution.nWidth && !m_tCurResolution.nHeight) {
        m_tCurResolution.nWidth = m_tVideoConfig.nWidth;
        m_tCurResolution.nHeight = m_tVideoConfig.nHeight;
    }

    m_tVencChnAttr.stVencAttr.u32PicWidthSrc = m_tVideoConfig.nWidth;
    m_tVencChnAttr.stVencAttr.u32PicHeightSrc = m_tVideoConfig.nHeight;

    m_tVencChnAttr.stVencAttr.u32BufSize = m_tVideoConfig.nBufSize; /*stream buffer size*/

    m_tVencChnAttr.stVencAttr.u8InFifoDepth = m_tVideoConfig.nInFifoDepth;
    m_tVencChnAttr.stVencAttr.u8OutFifoDepth = m_tVideoConfig.nOutFifoDepth;
    m_tVencChnAttr.stVencAttr.enLinkMode = m_tVideoConfig.bLink ? AX_VENC_LINK_MODE : AX_VENC_UNLINK_MODE;
    m_tVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = m_tVideoConfig.fFramerate;
    m_tVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = m_tVideoConfig.fFramerate;
    LOG_MM_D(VENC, "VENC attr: chn:%d, encoder:%d, w:%d, h:%d, link:%d, memSrc:%d, in_depth:%d, out_depth:%d,frameRate:%f",
             m_tVideoConfig.nChannel, m_tVideoConfig.ePayloadType, m_tVencChnAttr.stVencAttr.u32PicWidthSrc,
             m_tVencChnAttr.stVencAttr.u32PicHeightSrc, m_tVencChnAttr.stVencAttr.enLinkMode == AX_VENC_LINK_MODE ? AX_TRUE : AX_FALSE,
             m_tVencChnAttr.stVencAttr.enMemSource, m_tVencChnAttr.stVencAttr.u8InFifoDepth, m_tVencChnAttr.stVencAttr.u8OutFifoDepth,
             m_tVideoConfig.fFramerate);

    /* GOP Setting */
    m_tVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;
    if (AX_FALSE == m_tVideoConfig.GetEncCfg(m_tVideoConfig.ePayloadType, m_CurEncCfg)) {
        return AX_FALSE;
    }
    if (!InitRcParams(m_tVideoConfig)) {
        return AX_FALSE;
    }

    m_tVencChnAttr.stVencAttr.enType = m_tVideoConfig.ePayloadType;
    switch (m_tVencChnAttr.stVencAttr.enType) {
        case PT_H264: {
            m_tVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_MAIN_PROFILE;
            m_tVencChnAttr.stVencAttr.enLevel = AX_VENC_H264_LEVEL_5_2;
            m_tVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

            if (m_tVideoConfig.eImgFormat == AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010) {
                m_tVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_HIGH_10_PROFILE;
                m_tVencChnAttr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_10;
            }

            if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_H264CBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
                m_tVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                memcpy(&m_tVencChnAttr.stRcAttr.stH264Cbr, &m_tVencRcParams.tH264Cbr, sizeof(AX_VENC_H264_CBR_T));
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_H264VBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264VBR;
                m_tVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                memcpy(&m_tVencChnAttr.stRcAttr.stH264Vbr, &m_tVencRcParams.tH264Vbr, sizeof(AX_VENC_H264_VBR_T));
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_H264FIXQP) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264FIXQP;
                memcpy(&m_tVencChnAttr.stRcAttr.stH264FixQp, &m_tVencRcParams.tH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
            }
            break;
        }
        case PT_H265: {
            m_tVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_PROFILE;  // main profile
            m_tVencChnAttr.stVencAttr.enLevel = AX_VENC_HEVC_LEVEL_5_1;
            m_tVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

            if (m_tVideoConfig.eImgFormat == AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010) {
                m_tVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_10_PROFILE;
                m_tVencChnAttr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_10;
            }

            if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_H265CBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
                m_tVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                memcpy(&m_tVencChnAttr.stRcAttr.stH265Cbr, &m_tVencRcParams.tH265Cbr, sizeof(AX_VENC_H265_CBR_T));
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_H265VBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265VBR;
                m_tVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                memcpy(&m_tVencChnAttr.stRcAttr.stH265Vbr, &m_tVencRcParams.tH265Vbr, sizeof(AX_VENC_H265_VBR_T));
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_H265FIXQP) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265FIXQP;
                memcpy(&m_tVencChnAttr.stRcAttr.stH265FixQp, &m_tVencRcParams.tH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
            }
            break;
        }
        case PT_MJPEG: {
            if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_MJPEGCBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;
                memcpy(&m_tVencChnAttr.stRcAttr.stMjpegCbr, &m_tVencRcParams.tMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
                break;
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_MJPEGVBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;
                memcpy(&m_tVencChnAttr.stRcAttr.stMjpegVbr, &m_tVencRcParams.tMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
                break;
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_MJPEGFIXQP) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
                memcpy(&m_tVencChnAttr.stRcAttr.stMjpegFixQp, &m_tVencRcParams.tMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
                break;
            }
        }
        default:
            LOG_M_E(VENC, "Payload type unrecognized.");
            break;
    }
    LOG_MM(VENC, "Video attr: chn:%d, w:%d, h:%d, bitrate:%d, link:%d, memSrc:%d, in_depth:%d, out_depth:%d,frameRate:%f",
           m_tVideoConfig.nChannel, m_tVencChnAttr.stVencAttr.u32PicWidthSrc, m_tVencChnAttr.stVencAttr.u32PicHeightSrc,
           m_tVideoConfig.nBitrate, m_tVencChnAttr.stVencAttr.enLinkMode == AX_VENC_LINK_MODE ? AX_TRUE : AX_FALSE,
           m_tVencChnAttr.stVencAttr.enMemSource, m_tVencChnAttr.stVencAttr.u8InFifoDepth, m_tVencChnAttr.stVencAttr.u8OutFifoDepth,
           m_tVideoConfig.fFramerate);
    return AX_TRUE;
}

AX_BOOL CVideoEncoder::ProcessFrame(CAXFrame* pFrame) {
    AX_S32 nRet = 0;
    if (!m_bSend) {
        return AX_FALSE;
    }
    nRet = AX_VENC_SendFrame(GetChannel(), &pFrame->stFrame.stVFrame, -1);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(VENC, "[%d] AX_VENC_SendFrame failed, ret=0x%x", GetChannel(), nRet);
        return AX_FALSE;
    } else {
        LOG_M_D(VENC, "[%d] AX_VENC_SendFrame, seq=%lld", GetChannel(), pFrame->stFrame.stVFrame.stVFrame.u64SeqNum);
    }

    return AX_TRUE;
}

AX_VOID CVideoEncoder::FrameGetThreadFunc(CVideoEncoder* pCaller) {
    CVideoEncoder* pThis = (CVideoEncoder*)pCaller;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "APP_VENC_Get_%d", pThis->GetChannel());
    prctl(PR_SET_NAME, szName);

    AX_VENC_STREAM_T stStream;
    memset(&stStream, 0, sizeof(AX_VENC_STREAM_T));

    AX_S32 nChannel = pThis->GetChannel();
    AX_S32 ret = 0;
    while (pThis->m_bGetThreadRunning) {
        m_mtx.lock();
        if(m_bPauseGet) {
            CElapsedTimer::GetInstance()->mSleep(10);
            m_mtx.unlock();
            continue;
        }
        m_mtx.unlock();

        ret = AX_VENC_GetStream(nChannel, &stStream, 2000);

        if (AX_SUCCESS != ret) {
            if (AX_ERR_VENC_FLOW_END == ret) {
                pThis->m_bGetThreadRunning = AX_FALSE;
                break;
            }

            if (AX_ERR_VENC_QUEUE_EMPTY == ret) {
                CElapsedTimer::GetInstance()->mSleep(1);
                continue;
            }

            LOG_M_E(VENC, "[%d] AX_VENC_GetStream failed with %#x!", nChannel, ret);
            continue;
        }

        if (pThis->m_bGetThreadRunning && stStream.stPack.pu8Addr && stStream.stPack.u32Len > 0) {
            LOG_MM_D(VENC, "[%d] Get venc out stream, size=%d.", nChannel, stStream.stPack.u32Len);

#ifdef SLT
            if (0 == nChannel) {
                CPrintHelper::GetInstance()->Add(E_PH_MOD_VENC, 0);
            }
#else
            NotifyAll(nChannel, &stStream);
#endif

        } else {
            LOG_M_E(VENC, "[%d] AX_VENC_GetStream output data error, addr=%p, size=%d", nChannel, stStream.stPack.pu8Addr,
                    stStream.stPack.u32Len);
        }

        ret = AX_VENC_ReleaseStream(nChannel, &stStream);
        if (AX_SUCCESS != ret) {
            LOG_M_E(VENC, "AX_VENC_ReleaseStream failed!");
            continue;
        }
    }
}

AX_BOOL CVideoEncoder::UpdateChnResolution(const VIDEO_CONFIG_T& tNewConfig) {
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
        bFBC = m_tVideoConfig.bFBC;
    }
    if (nWidth != m_tVideoConfig.nWidth || nHeight != m_tVideoConfig.nHeight) {
        nStride = bFBC ? AX_ENCODER_FBC_WIDTH_ALIGN_VAL : 2;
    }

    AX_VENC_CHN_ATTR_T tAttr;
    AX_VENC_GetChnAttr(GetChannel(), &tAttr);
    tAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(nWidth, nStride);
    tAttr.stVencAttr.u32PicHeightSrc = nHeight;

    AX_VENC_SetChnAttr(GetChannel(), &tAttr);

    LOG_MM_C(VENC, "[%d] Reset res: (w: %d, h: %d) max(w:%d, h:%d)", GetChannel(), tAttr.stVencAttr.u32PicWidthSrc,
             tAttr.stVencAttr.u32PicHeightSrc, tAttr.stVencAttr.u32MaxPicWidth, tAttr.stVencAttr.u32MaxPicHeight);

    return AX_TRUE;
}

AX_BOOL CVideoEncoder::RefreshChnResolution() {
    AX_U32 nWidth = 0;
    AX_U32 nHeight = 0;
    GetResolutionByRotate(m_nRotation, nWidth, nHeight);
    AX_BOOL bFBC;
    /*Disable fbc when Rotation 90/270 */
    /*In fbc mode, width shoudle be Align of 128 */
    if (1 == m_nRotation || 3 == m_nRotation) {
        bFBC = AX_FALSE;
    } else {
        bFBC = m_tVideoConfig.bFBC;
    }
    AX_U32 nWidthAlign = bFBC ? AX_ENCODER_FBC_WIDTH_ALIGN_VAL : 2;

    m_tVencChnAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(nWidth, nWidthAlign);
    m_tVencChnAttr.stVencAttr.u32PicHeightSrc = nHeight;
    return AX_TRUE;
}

AX_BOOL CVideoEncoder::UpdateChnBitrate(AX_U32 nBitrate) {
    AX_VENC_RC_PARAM_T tRcParam = {0};
    AX_S32 nRet = AX_SUCCESS;
    nRet = AX_VENC_GetRcParam(GetChannel(), &tRcParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(VENC, "[%d] AX_VENC_GetRcParam failed, ret=0x%x", GetChannel(), nRet);
        return AX_FALSE;
    }
    StopRecv();
    do {
        if (AX_VENC_RC_MODE_H264CBR == tRcParam.enRcMode) {
            tRcParam.stH264Cbr.u32BitRate = nBitrate;
        } else if (AX_VENC_RC_MODE_H264VBR == tRcParam.enRcMode) {
            tRcParam.stH264Vbr.u32MaxBitRate = nBitrate;
        } else if (AX_VENC_RC_MODE_H265CBR == tRcParam.enRcMode) {
            tRcParam.stH265Cbr.u32BitRate = nBitrate;
        } else if (AX_VENC_RC_MODE_H265VBR == tRcParam.enRcMode) {
            tRcParam.stH265Vbr.u32MaxBitRate = nBitrate;
        } else if (AX_VENC_RC_MODE_MJPEGCBR == tRcParam.enRcMode) {
            tRcParam.stMjpegCbr.u32BitRate = nBitrate;
        } else if (AX_VENC_RC_MODE_MJPEGVBR == tRcParam.enRcMode) {
            tRcParam.stMjpegVbr.u32MaxBitRate = nBitrate;
        } else {
            LOG_M_E(VENC, "[%d] Bitrate changing not supported for rc mode: %d", GetChannel(), tRcParam.enRcMode);
            break;
        }

        nRet = AX_VENC_SetRcParam(GetChannel(), &tRcParam);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(VENC, "[%d] AX_VENC_SetRcParam failed, ret=0x%x", GetChannel(), nRet);
            break;
        }
    } while (0);

    StartRecv();

    LOG_M(VENC, "[%d] Update channel bitrate to %d", GetChannel(), nBitrate);

    return nRet == AX_SUCCESS ? AX_TRUE : AX_FALSE;
}

AX_BOOL CVideoEncoder::UpdateRcInfo(RC_INFO_T& tRcInfo) {
    AX_VENC_RC_PARAM_T tRcParam = {0};
    AX_S32 nRet = AX_SUCCESS;
    nRet = AX_VENC_GetRcParam(GetChannel(), &tRcParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(VENC, "[%d] AX_VENC_GetRcParam failed, ret=0x%x", GetChannel(), nRet);
        return AX_FALSE;
    }
    StopRecv();

    do {
        tRcParam.enRcMode = tRcInfo.eRcType;
        LOG_MM_I(VENC, "nMinQp:%d,nMaxQp:%d,nMinIQp:%d,nMaxIQp:%d ,nMinIProp:%d,nMaxIProp:%d", tRcInfo.nMinQp, tRcInfo.nMaxQp,
                 tRcInfo.nMinIQp, tRcInfo.nMaxIQp, tRcInfo.nMinIProp, tRcInfo.nMaxIProp);
        if (AX_VENC_RC_MODE_H264CBR == tRcInfo.eRcType) {
            memcpy(&tRcParam.stH264Cbr, &m_tVencRcParams.tH264Cbr, sizeof(AX_VENC_H264_CBR_T));
            tRcParam.stH264Cbr.u32MinQp = tRcInfo.nMinQp;
            tRcParam.stH264Cbr.u32MaxQp = tRcInfo.nMaxQp;
            tRcParam.stH264Cbr.u32MinIQp = tRcInfo.nMinIQp;
            tRcParam.stH264Cbr.u32MaxIQp = tRcInfo.nMaxIQp;
            tRcParam.stH264Cbr.u32MinIprop = tRcInfo.nMinIProp;
            tRcParam.stH264Cbr.u32MaxIprop = tRcInfo.nMaxIProp;
            tRcParam.stH264Cbr.u32BitRate = tRcInfo.nBitrate;

        } else if (AX_VENC_RC_MODE_H265CBR == tRcParam.enRcMode) {
            memcpy(&tRcParam.stH265Cbr, &m_tVencRcParams.tH265Cbr, sizeof(AX_VENC_H265_CBR_T));
            tRcParam.stH265Cbr.u32MinQp = tRcInfo.nMinQp;
            tRcParam.stH265Cbr.u32MaxQp = tRcInfo.nMaxQp;
            tRcParam.stH265Cbr.u32MinIQp = tRcInfo.nMinIQp;
            tRcParam.stH265Cbr.u32MaxIQp = tRcInfo.nMaxIQp;
            tRcParam.stH265Cbr.u32MinIprop = tRcInfo.nMinIProp;
            tRcParam.stH265Cbr.u32MaxIprop = tRcInfo.nMaxIProp;
            tRcParam.stH265Cbr.u32BitRate = tRcInfo.nBitrate;

        } else if (AX_VENC_RC_MODE_H264VBR == tRcParam.enRcMode) {
            memcpy(&tRcParam.stH264Vbr, &m_tVencRcParams.tH264Vbr, sizeof(AX_VENC_H264_VBR_T));
            tRcParam.stH264Vbr.u32MinQp = tRcInfo.nMinQp;
            tRcParam.stH264Vbr.u32MaxQp = tRcInfo.nMaxQp;
            tRcParam.stH264Vbr.u32MinIQp = tRcInfo.nMinIQp;
            tRcParam.stH264Vbr.u32MaxIQp = tRcInfo.nMaxIQp;
            tRcParam.stH264Vbr.u32MaxBitRate = tRcInfo.nBitrate;

        } else if (AX_VENC_RC_MODE_H265VBR == tRcParam.enRcMode) {
            memcpy(&tRcParam.stH265Vbr, &m_tVencRcParams.tH265Vbr, sizeof(AX_VENC_H265_VBR_T));
            tRcParam.stH265Vbr.u32MinQp = tRcInfo.nMinQp;
            tRcParam.stH265Vbr.u32MaxQp = tRcInfo.nMaxQp;
            tRcParam.stH265Vbr.u32MinIQp = tRcInfo.nMinIQp;
            tRcParam.stH265Vbr.u32MaxIQp = tRcInfo.nMaxIQp;
            tRcParam.stH265Vbr.u32MaxBitRate = tRcInfo.nBitrate;

        } else if (AX_VENC_RC_MODE_H264FIXQP == tRcParam.enRcMode) {
            memcpy(&tRcParam.stH264FixQp, &m_tVencRcParams.tH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
        } else if (AX_VENC_RC_MODE_H265FIXQP == tRcParam.enRcMode) {
            memcpy(&tRcParam.stH265FixQp, &m_tVencRcParams.tH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
        } else if (AX_VENC_RC_MODE_MJPEGCBR == tRcInfo.eRcType) {
            memcpy(&tRcParam.stMjpegCbr, &m_tVencRcParams.tMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
            tRcParam.stMjpegCbr.u32MaxQp = tRcInfo.nMaxQp;
            tRcParam.stMjpegCbr.u32MinQp = tRcInfo.nMinQp;
            tRcParam.stMjpegCbr.u32BitRate = tRcInfo.nBitrate;
        } else if (AX_VENC_RC_MODE_MJPEGVBR == tRcParam.enRcMode) {
            memcpy(&tRcParam.stMjpegVbr, &m_tVencRcParams.tMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
            tRcParam.stMjpegVbr.u32MaxQp = tRcInfo.nMaxQp;
            tRcParam.stMjpegVbr.u32MinQp = tRcInfo.nMinQp;
            tRcParam.stMjpegVbr.u32MaxBitRate = tRcInfo.nBitrate;
        } else if (AX_VENC_RC_MODE_MJPEGFIXQP == tRcParam.enRcMode) {
            memcpy(&tRcParam.stMjpegFixQp, &m_tVencRcParams.tMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
            tRcParam.stMjpegFixQp.s32FixedQp = tRcInfo.nFixQp;
        }
        nRet = AX_VENC_SetRcParam(GetChannel(), &tRcParam);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(VENC, "[%d] AX_VENC_SetRcParam failed, ret=0x%x", GetChannel(), nRet);
            break;
        }

        m_tVideoConfig.eRcType = tRcInfo.eRcType;
    } while (0);

    StartRecv();

    return nRet == AX_SUCCESS ? AX_TRUE : AX_FALSE;
}

AX_BOOL CVideoEncoder::UpdateRotation(AX_U8 nRotation) {
    AX_U32 nNewWidth = 0;
    AX_U32 nNewHeight = 0;

    SetPauseFlag(AX_TRUE);

    if (!GetResolutionByRotate(nRotation, nNewWidth, nNewHeight)) {
        LOG_MM_E(VENC, "[%d] Can not get new resolution for rotate operation.", GetChannel());
        return AX_FALSE;
    }
    AX_BOOL bFBC = m_tVideoConfig.bFBC;
    if (1 == nRotation || 3 == nRotation) {
        /* Rotation 0/180 */
        bFBC = AX_FALSE;
    }
    AX_U32 nWidthAlign = bFBC ? AX_ENCODER_FBC_WIDTH_ALIGN_VAL : 2;

    AX_VENC_CHN_ATTR_T tAttr;
    AX_VENC_GetChnAttr(GetChannel(), &tAttr);
    tAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(nNewWidth, nWidthAlign);
    tAttr.stVencAttr.u32PicHeightSrc = nNewHeight;

    AX_VENC_SetChnAttr(GetChannel(), &tAttr);

    SetPauseFlag(AX_FALSE);

    LOG_MM_C(VENC, "[%d] Reset res: (w: %d, h: %d) (MAX w: %d, h:%d) bFBC:%d", GetChannel(), tAttr.stVencAttr.u32PicWidthSrc,
             tAttr.stVencAttr.u32PicHeightSrc, tAttr.stVencAttr.u32MaxPicWidth, tAttr.stVencAttr.u32MaxPicHeight, bFBC);
    m_nRotation = nRotation;
    return AX_TRUE;
}

AX_BOOL CVideoEncoder::GetResolutionByRotate(AX_U8 nRotation, AX_U32& nWidth, AX_U32& nHeight) {
    nWidth = m_tCurResolution.nWidth;
    nHeight = m_tCurResolution.nHeight;

    if (1 == nRotation || 3 == nRotation) {
        std::swap(nWidth, nHeight);
    }

    return AX_TRUE;
}

AX_BOOL CVideoEncoder::UpdateLinkMode(AX_BOOL bLink) {
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_CHN_ATTR_T tAttr;
    s32Ret = AX_VENC_GetChnAttr(GetChannel(), &tAttr);
    tAttr.stVencAttr.enLinkMode = bLink ? AX_VENC_LINK_MODE : AX_VENC_UNLINK_MODE;
    s32Ret = AX_VENC_SetChnAttr(GetChannel(), &tAttr);
    return (s32Ret == AX_SUCCESS) ? AX_TRUE : AX_FALSE;
}

AX_BOOL CVideoEncoder::UpdatePayloadType(AX_PAYLOAD_TYPE_E ePayloadType) {
    LOG_MM_C(VENC, "+++");
    m_tVideoConfig.eRcType = CAXTypeConverter::FormatRcMode(CAXTypeConverter::EncoderType2Int(ePayloadType),
                                                            CAXTypeConverter::RcMode2Int(m_tVideoConfig.eRcType));
    m_tVideoConfig.ePayloadType = ePayloadType;

    InitParams();
    RefreshChnResolution();
    STAGE_START_PARAM_T tStartParam;
    tStartParam.bStartProcessingThread = m_tVideoConfig.bLink ? AX_FALSE : AX_TRUE;
    return Start(&tStartParam);
}

AX_VOID CVideoEncoder::SetPauseFlag(AX_BOOL bPause) {
    std::lock_guard<std::mutex> lck(m_mtx);
    m_bPauseGet = bPause;
}
