
/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "VideoEncoderEx.hpp"
#include "AppLogApi.h"
#include <string.h>

#define TAG "VENC"

CVideoEncoderEx::CVideoEncoderEx(VENC_ATTR_T tAttr) {
    m_tVideoConfig = tAttr;
}

AX_BOOL CVideoEncoderEx::Init() {
    AX_S32 veChn = GetChannel();

#ifndef __MMAP_VENC_BY_APP__
    m_tVencChnAttr.stVencAttr.flag = 0x2; /* bit1: cached  bit0: multi-core */
#endif

    AX_S32 ret = AX_VENC_CreateChn(veChn, &m_tVencChnAttr);
    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "[%d] AX_VENC_CreateChn fail, ret = 0x%x", veChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoEncoderEx::DeInit() {
    AX_S32 veChn = GetChannel();

    AX_S32 ret = AX_VENC_DestroyChn(veChn);
    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "[%d] AX_VENC_DestroyChn() fail, ret = 0x%x", veChn, ret);
    }

    return AX_TRUE;
}

AX_BOOL CVideoEncoderEx::Start() {
    StartRecv();

    AX_CHAR szName[16];
    sprintf(szName, "AppVencGet%d", GetChannel());
    if (!m_thread.Start([this](AX_VOID *pArg) -> AX_VOID { DispatchThread(pArg); }, nullptr, szName)) {
        LOG_MM_E(TAG, "start %s thread fail", szName);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoEncoderEx::Stop() {
    AX_S32 ret;
    AX_S32 veChn = GetChannel();

    m_thread.Stop();

    StopRecv();

    /* Wakeup AX_VENC_GetStream(-1) */
    ret = AX_VENC_ResetChn(veChn);
    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "[%d] AX_VENC_ResetChn() fail, ret = 0x%x", ret);
    }

    m_thread.Join();

    return AX_TRUE;
}

AX_VOID CVideoEncoderEx::StartRecv() {
    LOG_MM_I(TAG, "[%d] VENC start receive", GetChannel());

    AX_VENC_RECV_PIC_PARAM_T tRecvParam;
    tRecvParam.s32RecvPicNum = -1;

    AX_S32 ret = AX_VENC_StartRecvFrame(GetChannel(), &tRecvParam);
    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "[%d] AX_VENC_StartRecvFrame failed, ret=0x%x", GetChannel(), ret);
    }

    return;
}

AX_VOID CVideoEncoderEx::StopRecv() {
    LOG_MM_I(TAG, "[%d] VENC stop receive", GetChannel());

    AX_S32 ret = AX_VENC_StopRecvFrame((VENC_CHN)GetChannel());
    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "[%d] AX_VENC_StopRecvFrame failed, ret=0x%x", GetChannel(), ret);
    }
    LOG_MM_I(TAG, "[%d] ---", GetChannel());
}

AX_BOOL CVideoEncoderEx::InitRcParams() {
    LOG_MM_I(TAG, "[%d] +++", GetChannel());

    APP_ENC_RC_CONFIG tRcConfig;
    if (AX_FALSE == m_tVideoConfig.GetRcCfg(m_tVideoConfig.ePayloadType, tRcConfig)) {
        LOG_MM_E(TAG, "Get RC config failed.");
        return AX_FALSE;
    }

    RC_INFO_T rcInfo;
    AX_VENC_RC_MODE_E eRcType;
    switch (m_tVideoConfig.ePayloadType) {
        case PT_H264: {
            // set h264 cbr
            eRcType = AX_VENC_RC_MODE_H264CBR;
            if (AX_TRUE == tRcConfig.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH264Cbr.u32Gop = m_tVideoConfig.nGOP;
                m_tVencRcParams.tH264Cbr.u32BitRate = m_tVideoConfig.nBitrate;
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
            if (AX_TRUE == tRcConfig.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH264Vbr.u32Gop = m_tVideoConfig.nGOP;
                m_tVencRcParams.tH264Vbr.u32MaxBitRate = m_tVideoConfig.nBitrate;
                m_tVencRcParams.tH264Vbr.u32MinQp = ADAPTER_RANGE(rcInfo.nMinQp, 0, 51);
                m_tVencRcParams.tH264Vbr.u32MaxQp = ADAPTER_RANGE(rcInfo.nMaxQp, 0, 51);
                m_tVencRcParams.tH264Vbr.u32MinIQp = ADAPTER_RANGE(rcInfo.nMinIQp, 0, 51);
                m_tVencRcParams.tH264Vbr.u32MaxIQp = ADAPTER_RANGE(rcInfo.nMaxIQp, 0, 51);
                m_tVencRcParams.tH264Vbr.s32IntraQpDelta = ADAPTER_RANGE(rcInfo.nIntraQpDelta, -51, 51);
            }

            // set h264 fixQp
            eRcType = AX_VENC_RC_MODE_H264FIXQP;
            if (AX_TRUE == tRcConfig.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH264FixQp.u32Gop = m_tVideoConfig.nGOP;
                m_tVencRcParams.tH264FixQp.u32IQp = 25;
                m_tVencRcParams.tH264FixQp.u32PQp = 30;
                m_tVencRcParams.tH264FixQp.u32BQp = 32;
            }

            break;
        }
        case PT_H265: {
            // set h265 cbr
            eRcType = AX_VENC_RC_MODE_H265CBR;
            if (AX_TRUE == tRcConfig.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH265Cbr.u32Gop = m_tVideoConfig.nGOP;
                m_tVencRcParams.tH265Cbr.u32BitRate = m_tVideoConfig.nBitrate;
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
            if (AX_TRUE == tRcConfig.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH265Vbr.u32Gop = m_tVideoConfig.nGOP;
                m_tVencRcParams.tH265Vbr.u32MaxBitRate = m_tVideoConfig.nBitrate;
                m_tVencRcParams.tH265Vbr.u32MinQp = ADAPTER_RANGE(rcInfo.nMinQp, 0, 51);
                m_tVencRcParams.tH265Vbr.u32MaxQp = ADAPTER_RANGE(rcInfo.nMaxQp, 0, 51);
                m_tVencRcParams.tH265Vbr.u32MinIQp = ADAPTER_RANGE(rcInfo.nMinIQp, 0, 51);
                m_tVencRcParams.tH265Vbr.u32MaxIQp = ADAPTER_RANGE(rcInfo.nMaxIQp, 0, 51);
                m_tVencRcParams.tH265Vbr.s32IntraQpDelta = ADAPTER_RANGE(rcInfo.nIntraQpDelta, -51, 51);
            }

            // set h265 fixQp
            eRcType = AX_VENC_RC_MODE_H265FIXQP;
            if (AX_TRUE == tRcConfig.GetRcInfo(eRcType, rcInfo)) {
                m_tVencRcParams.tH265FixQp.u32Gop = m_tVideoConfig.nGOP;
                m_tVencRcParams.tH265FixQp.u32IQp = 25;
                m_tVencRcParams.tH265FixQp.u32PQp = 30;
                m_tVencRcParams.tH265FixQp.u32BQp = 32;
            }

            break;
        }
        default:
            LOG_M_E(TAG, "Unrecognized payload type: %d.", m_tVideoConfig.ePayloadType);
            break;
    }
    LOG_MM_I(TAG, "[%d] ---", GetChannel());

    return AX_TRUE;
}

AX_BOOL CVideoEncoderEx::InitParams() {
    memset(&m_tVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    m_tVencChnAttr.stVencAttr.enMemSource = m_tVideoConfig.eMemSource;

    m_tVencChnAttr.stVencAttr.u32MaxPicWidth = m_tVideoConfig.nMaxWidth;
    if (APP_ENCODE_PARSE_INVALID == m_tVideoConfig.nMaxWidth) {
        m_tVencChnAttr.stVencAttr.u32MaxPicWidth = ALIGN_UP(AX_MAX(m_tVideoConfig.nWidth, m_tVideoConfig.nHeight), AX_ENCODER_FBC_STRIDE_ALIGN_VAL);
    }
    m_tVencChnAttr.stVencAttr.u32MaxPicHeight = m_tVideoConfig.nMaxHeight;
    if (APP_ENCODE_PARSE_INVALID == m_tVideoConfig.nMaxHeight) {
        m_tVencChnAttr.stVencAttr.u32MaxPicHeight = m_tVencChnAttr.stVencAttr.u32MaxPicWidth; /* Make max height same as width for the case of rotation 90  */
    }
    LOG_M_I(TAG, "[%d] u32MaxPicWidth: %d, u32MaxPicHeight: %d",
                   GetChannel(), m_tVencChnAttr.stVencAttr.u32MaxPicWidth,
                   m_tVencChnAttr.stVencAttr.u32MaxPicHeight);

    m_tVencChnAttr.stVencAttr.u32PicWidthSrc = m_tVideoConfig.nWidth;
    m_tVencChnAttr.stVencAttr.u32PicHeightSrc = m_tVideoConfig.nHeight;

    m_tVencChnAttr.stVencAttr.u32BufSize = m_tVideoConfig.nBufSize; /*stream buffer size*/

    m_tVencChnAttr.stVencAttr.u8InFifoDepth = m_tVideoConfig.nInFifoDepth;
    m_tVencChnAttr.stVencAttr.u8OutFifoDepth = m_tVideoConfig.nOutFifoDepth;
    m_tVencChnAttr.stVencAttr.enLinkMode = m_tVideoConfig.bLink ? AX_VENC_LINK_MODE : AX_VENC_UNLINK_MODE;
    m_tVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = m_tVideoConfig.fFramerate;
    m_tVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = m_tVideoConfig.fFramerate;
    LOG_MM_D(TAG, "VENC attr: chn:%d, encoder:%d, w:%d, h:%d, link:%d, memSrc:%d, in_depth:%d, out_depth:%d,frameRate:%f",
             m_tVideoConfig.nChannel, m_tVideoConfig.ePayloadType, m_tVencChnAttr.stVencAttr.u32PicWidthSrc,
             m_tVencChnAttr.stVencAttr.u32PicHeightSrc, m_tVencChnAttr.stVencAttr.enLinkMode == AX_VENC_LINK_MODE ? AX_TRUE : AX_FALSE,
             m_tVencChnAttr.stVencAttr.enMemSource, m_tVencChnAttr.stVencAttr.u8InFifoDepth, m_tVencChnAttr.stVencAttr.u8OutFifoDepth,
             m_tVideoConfig.fFramerate);

    /* GOP Setting */
    m_tVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;

    if (!InitRcParams()) {
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
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_MJPEGVBR) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;
                memcpy(&m_tVencChnAttr.stRcAttr.stMjpegVbr, &m_tVencRcParams.tMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
            } else if (m_tVideoConfig.eRcType == AX_VENC_RC_MODE_MJPEGFIXQP) {
                m_tVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
                memcpy(&m_tVencChnAttr.stRcAttr.stMjpegFixQp, &m_tVencRcParams.tMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
            }
            break;
        }
        default:
            LOG_M_E(TAG, "Payload type unrecognized.");
            break;
    }
    LOG_MM(TAG, "Video attr: chn:%d, w:%d, h:%d, bitrate:%d, link:%d, memSrc:%d, in_depth:%d, out_depth:%d,frameRate:%f",
           m_tVideoConfig.nChannel, m_tVencChnAttr.stVencAttr.u32PicWidthSrc, m_tVencChnAttr.stVencAttr.u32PicHeightSrc,
           m_tVideoConfig.nBitrate, m_tVencChnAttr.stVencAttr.enLinkMode == AX_VENC_LINK_MODE ? AX_TRUE : AX_FALSE,
           m_tVencChnAttr.stVencAttr.enMemSource, m_tVencChnAttr.stVencAttr.u8InFifoDepth, m_tVencChnAttr.stVencAttr.u8OutFifoDepth,
           m_tVideoConfig.fFramerate);
    return AX_TRUE;
}

AX_VOID CVideoEncoderEx::DispatchThread(AX_VOID *) {
    const AX_S32 veChn = GetChannel();
    LOG_MM_I(TAG, "[%d] +++", veChn);

    AX_VENC_STREAM_T stStream;
    memset(&stStream, 0, sizeof(AX_VENC_STREAM_T));

    AX_S32 ret = 0;
    while (m_thread.IsRunning()) {
        ret = AX_VENC_GetStream(veChn, &stStream, -1);
        if (AX_SUCCESS != ret) {
            if (!m_thread.IsRunning()) {
                break;
            }

            if (AX_ERR_VENC_FLOW_END == ret) {
                break;
            }

            if (AX_ERR_VENC_QUEUE_EMPTY == ret) {
                // std::this_thread::sleep_for(std::chrono::milliseconds(10));
                LOG_MM_W(TAG, "no stream in venc %d queue", veChn);
            } else {
                LOG_M_E(TAG, "[%d] AX_VENC_GetStream fail, ret = 0x%x", veChn, ret);
            }

            continue;
        }

        if (stStream.stPack.pu8Addr && stStream.stPack.u32Len > 0) {
            // AX_BOOL bIFrame = (AX_VENC_INTRA_FRAME == stStream.stPack.enCodingType) ? AX_TRUE : AX_FALSE;
            // LOG_MM_D(TAG, "[%d] VENC output: type:%d, size:%d, pts:%lld, userdata:%lld", m_tVideoConfig.nChannel, bIFrame, stStream.stPack.u32Len, stStream.stPack.u64PTS, stStream.stPack.u64UserData);

            AX_BOOL bGopStart = (0 == stStream.stPack.u64UserData) ? AX_TRUE : AX_FALSE;
            NotifyAll(veChn, &stStream.stPack, bGopStart);
        } else {
            LOG_M_E(TAG, "[%d] AX_VENC_GetStream output data error, addr %p, size %d", veChn, stStream.stPack.pu8Addr,
                    stStream.stPack.u32Len);
        }

        ret = AX_VENC_ReleaseStream(veChn, &stStream);
        if (AX_SUCCESS != ret) {
            LOG_M_E(TAG, "[%d] AX_VENC_ReleaseStream(seq %lld) fail, ret = 0x%x", veChn, stStream.stPack.u64SeqNum, ret);
            continue;
        }
    }

    LOG_MM_I(TAG, "[%d] ---", veChn);
}

AX_BOOL CVideoEncoderEx::NotifyAll(AX_U32 nChannel, AX_VOID* pStream, AX_BOOL bGopStart /*= AX_TRUE*/) {
    if (nullptr == pStream) {
        return AX_TRUE;
    }

    AX_BOOL bRet = AX_TRUE;
    for (std::vector<IVencPackObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        if (!(*it)->OnRecvStreamPack(nChannel, *(AX_VENC_PACK_T*)pStream, bGopStart)) {
            bRet = AX_FALSE;
        }
    }

    return bRet;
}

AX_BOOL CVideoEncoderEx::IsKeyFrame(const AX_VENC_PACK_T &stPack) {
    if (PT_H264 == stPack.enType) {
        for (AX_U32 i = 0; i < stPack.u32NaluNum; ++i) {
            if (AX_H264E_NALU_SPS == stPack.stNaluInfo[i].unNaluType.enH264EType ||
                AX_H264E_NALU_PPS == stPack.stNaluInfo[i].unNaluType.enH264EType ||
                AX_H264E_NALU_IDRSLICE == stPack.stNaluInfo[i].unNaluType.enH264EType) {
                return AX_TRUE;
            }
        }
    } else {
        for (AX_U32 i = 0; i < stPack.u32NaluNum; ++i) {
            if (AX_H265E_NALU_SPS == stPack.stNaluInfo[i].unNaluType.enH265EType ||
                AX_H265E_NALU_PPS == stPack.stNaluInfo[i].unNaluType.enH265EType ||
                AX_H265E_NALU_VPS == stPack.stNaluInfo[i].unNaluType.enH265EType ||
                AX_H265E_NALU_IDRSLICE == stPack.stNaluInfo[i].unNaluType.enH265EType) {
                return AX_TRUE;
            }
        }
    }

    return AX_FALSE;
}

AX_VOID CVideoEncoderEx::RegObserver(IVencPackObserver* pObserver) {
    if (nullptr != pObserver) {
        m_vecObserver.emplace_back(pObserver);
    }
}

AX_VOID CVideoEncoderEx::UnregObserver(IVencPackObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (std::vector<IVencPackObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        if (*it == pObserver) {
            m_vecObserver.erase(it);
            break;
        }
    }
}

AX_BOOL CVideoEncoderEx::ActiveGDR(AX_U32 nRefreshNum) {
    AX_S32 ret;

    AX_S32 veChn = GetChannel();

    AX_VENC_INTRA_REFRESH_T param;
    memset(&param, 0, sizeof(param));
    param.bRefresh = AX_TRUE;
    param.enIntraRefreshMode = AX_VENC_INTRA_REFRESH_ROW;
    param.u32RefreshNum = nRefreshNum;

    ret = AX_VENC_SetIntraRefresh(veChn, &param);
    if (0 != ret) {
        LOG_MM_E(TAG, "AX_VENC_SetIntraRefresh(veChn %d) fail, ret = 0x%x", veChn, ret);
        return AX_FALSE;
    }

    LOG_MM_W(TAG, "Active veChn %d GDR: %d", veChn, nRefreshNum);
    return AX_TRUE;
}
