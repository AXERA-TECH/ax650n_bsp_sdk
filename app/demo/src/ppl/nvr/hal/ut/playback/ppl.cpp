/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "ppl.hpp"
#include <exception>

#define TAG "PPL"

AX_BOOL PPL::Create(const PPL_ATTR_T &stAttr) {
    m_stAttr = stAttr;
    m_vo = stAttr.vo;

    do {
        if (!CreateStream()) {
            break;
        }

        if (!CreateVDEC()) {
            break;
        }

        if (!CreateIVPS()) {
            break;
        }

        /* update video chn fps */
        m_vo->SetChnFps(m_stAttr.voChn, (AX_F32)(m_stream->GetStreamInfo().stVideo.nFps));

        /* bind stream and vdec */
        m_stream->RegisterObserver(m_vdec);

        if (!m_ivps->Start()) {
            break;
        }

        if (!m_vdec->Start()) {
            break;
        }

        SetupLink();

        if (!m_stream->Start()) {
            break;
        }

        return AX_TRUE;

    } while (0);

    Destory();
    return AX_FALSE;
}

AX_BOOL PPL::Destory(AX_VOID) {
    DestroyLink();

    if (m_stream) {
        if (m_vdec) {
            m_stream->UnRegisterObserver(m_vdec);
        }

        if (!m_stream->Stop()) {
            return AX_FALSE;
        }

        if (!DestoryStream()) {
            return AX_FALSE;
        }
    }

    if (m_vdec) {
        if (!m_vdec->Stop()) {
            return AX_FALSE;
        }

        if (!DestoryVDEC()) {
            return AX_FALSE;
        }
    }

    if (m_ivps) {
        if (!m_ivps->Stop()) {
            return AX_FALSE;
        }

        if (!DestoryIVPS()) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL PPL::DestoryStream(AX_VOID) {
    if (m_stream) {
        if (!m_stream->DeInit()) {
            return AX_FALSE;
        }

        delete m_stream;
        m_stream = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL PPL::CreateStream(AX_VOID) {
    if (INPUT_FILE == m_stAttr.enInput) {
        m_stream = new (std::nothrow) CFFMpegStream();
    } else {
        if (0 != ping4(m_stAttr.strUrl.c_str(), 4)) {
            LOG_M_E(TAG, "network to %s is down", m_stAttr.strUrl.c_str());
            return AX_FALSE;
        }

        m_stream = new (std::nothrow) CRtspStream();
    }

    if (m_stream) {
        STREAM_ATTR_T stAttr;
        stAttr.strURL = m_stAttr.strUrl;
        stAttr.nFps = m_stAttr.nFps;
        if (m_stream->Init(stAttr)) {
            return AX_TRUE;
        } else {
            delete m_stream;
            m_stream = nullptr;
        }
    }

    return AX_FALSE;
}

AX_BOOL PPL::DestoryVDEC(AX_VOID) {
    if (m_vdec) {
        m_vdec->Destory();
        m_vdec = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL PPL::CreateVDEC(AX_VOID) {
    CONST STREAM_INFO_T &streamInfo = m_stream->GetStreamInfo();
    VDEC_ATTR_T stAttr;
    stAttr.enCodecType = streamInfo.stVideo.enPayload;
    stAttr.nWidth = ALIGN_UP(streamInfo.stVideo.nWidth, 16);
    stAttr.nHeight = ALIGN_UP(streamInfo.stVideo.nHeight, 16);
    stAttr.nMaxStreamBufSize = stAttr.nWidth * stAttr.nHeight * 2;
    stAttr.enDecodeMode = (1 == m_stAttr.playBackMode) ? AX_VDEC_DISPLAY_MODE_PLAYBACK : AX_VDEC_DISPLAY_MODE_PREVIEW;
    stAttr.bPrivatePool = AX_TRUE;
    stAttr.nTimeOut = (AX_VDEC_DISPLAY_MODE_PLAYBACK == stAttr.enDecodeMode) ? -1 : m_stAttr.nTimeOut;

    for (AX_S32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
        switch (j) {
            case 0:
                stAttr.stChnAttr[j].bEnable = AX_TRUE;
                stAttr.stChnAttr[j].bLinked = AX_TRUE;
                stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = m_stAttr.ppDepth;
                stAttr.stChnAttr[j].stAttr.u32PicWidth = streamInfo.stVideo.nWidth;
                stAttr.stChnAttr[j].stAttr.u32PicHeight = streamInfo.stVideo.nHeight;
                stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_ORIGINAL;
                stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                break;
            case 1:
            case 2:
                stAttr.stChnAttr[j].bEnable = AX_FALSE;
                break;
            default:
                break;
        }

        if (stAttr.stChnAttr[j].bEnable) {
            AX_VDEC_CHN_ATTR_T &stChnAttr = stAttr.stChnAttr[j].stAttr;
            stChnAttr.u32FrameBufSize = CVDEC::GetBlkSize(stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stAttr.enCodecType,
                                                          &stChnAttr.stCompressInfo, stChnAttr.enImgFormat);
            stChnAttr.u32FrameBufCnt = m_stAttr.ppVBCnt;
        }
    }

    m_vdec = CVDEC::CreateInstance(stAttr);
    if (m_vdec) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL PPL::DestoryIVPS(AX_VOID) {
    if (m_ivps) {
        m_ivps->Destory();
        m_ivps = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL PPL::CreateIVPS(AX_VOID) {
    IVPS_CHN_ATTR_T stChn;
    stChn.enEngine = m_stAttr.enEngine;
    stChn.nWidth = m_stAttr.nW;
    stChn.nHeight = m_stAttr.nH;
    stChn.nStride = ALIGN_UP(stChn.nWidth, 16);
    stChn.stPoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
    stChn.bLinked = AX_TRUE;
    stChn.nFifoDepth = 0;
    stChn.stPoolAttr.nFrmBufNum = m_stAttr.ivpsBufCnt;

    IVPS_ATTR_T stAttr;
    stAttr.nChnNum = 1;
    stAttr.nInDepth = m_stAttr.ivpsInDepth;
    stAttr.nBackupInDepth = m_stAttr.ivpsBackupDepth;
    stAttr.stChnAttr[0] = std::move(stChn);

    m_ivps = CIVPS::CreateInstance(stAttr);
    if (m_ivps) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL PPL::SetupLink(AX_VOID) {
    for (AX_S32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
        const VDEC_CHN_ATTR_T &stChn = m_vdec->GetAttr().stChnAttr[j];
        if (stChn.bEnable) {
            if (stChn.bLinked) {
                AX_MOD_INFO_T src = {AX_ID_VDEC, (AX_S32)(m_vdec->GetGrpId()), j};
                AX_MOD_INFO_T dst = {AX_ID_IVPS, (AX_S32)(m_ivps->GetGrpId()), 0};
                m_linker.Link(src, dst);

                src = dst;
                dst = {AX_ID_VO, (AX_S32)(m_vo->GetAttr().voLayer), (AX_S32)m_stAttr.voChn};
                m_linker.Link(src, dst);
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL PPL::DestroyLink(AX_VOID) {
    m_linker.UnlinkAll();
    return AX_TRUE;
}
