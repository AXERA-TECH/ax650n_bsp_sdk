/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "BoxRecorder.hpp"
#include <sys/time.h>
#include <exception>
#include "AppLogApi.h"
#include "fs.hpp"
#include "make_unique.hpp"

#define REC "REC"

AX_BOOL CBoxRecorder::Init(const BOX_RECORDER_ATTR_T &stAttr) {
    m_stAttr = stAttr;

    if (!m_stAttr.bMuxer) {
        m_stAttr.nMaxRecordSize *= (1024 * 1024);
    }

    if (m_stAttr.strRecordPath.empty()) {
        m_stAttr.strRecordPath = "./";
    } else {
        if (m_stAttr.strRecordPath[m_stAttr.strRecordPath.length() - 1] != '/') {
            m_stAttr.strRecordPath += "/";
        }

        try {
            fs::create_directories(m_stAttr.strRecordPath);
        } catch (fs::filesystem_error &e) {
            LOG_M_E(REC, "create %s fail, %s", m_stAttr.strRecordPath.c_str(), e.what());
            return AX_FALSE;
        }
    }

    if (!CreateEncoderInstance()) {
        return AX_FALSE;
    }

    if (m_stAttr.bMuxer) {
        if (!CreateMuxerInstance()) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBoxRecorder::DeInit(AX_VOID) {
    if (m_pEncoder) {
        m_pEncoder->UnregObserver(this);
        if (!m_pEncoder->DeInit()) {
            return AX_FALSE;
        }

        m_pEncoder = nullptr;
    }

    if (m_pMuxer) {
        m_pMuxer->DeInit();
        m_pMuxer = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL CBoxRecorder::Start(AX_VOID) {
    if (!m_stAttr.bMuxer) {
        if (!CreateRawFile()) {
            return AX_FALSE;
        }
    } else {
        if (!m_pMuxer->Start()) {
            return AX_FALSE;
        }
    }

    STAGE_START_PARAM_T arg;
    /* as we use link, here disable dispatch thread of stage */
    arg.bStartProcessingThread = m_stAttr.bLinkMode ? AX_FALSE : AX_TRUE;
    if (!m_pEncoder->Start(&arg)) {
        if (!m_stAttr.bMuxer) {
            m_ofs.close();
        } else {
            m_pMuxer->Stop();
        }

        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBoxRecorder::Stop(AX_VOID) {
    /* fixme:
       1. stop encoder first caused SIGSEGV
       2. frame lost if stop muxer first? */
    if (!m_stAttr.bMuxer) {
        long sz = 0;
        if (m_ofs.is_open()) {
            m_ofs.flush();
            sz = m_ofs.tellp();
            m_ofs.close();
        }

        if (0 == sz) {
            LOG_M_E(REC, "record file size is 0"); /* ut */
        }
    } else {
        m_pMuxer->Stop();
    }

    if (!m_pEncoder->Stop()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBoxRecorder::OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID *pData) {
    if (E_OBS_TARGET_TYPE_VENC != eTarget || nChn != (AX_U32)m_stAttr.veChn) {
        return AX_TRUE;
    }

    AX_VENC_STREAM_T *pStream = (AX_VENC_STREAM_T *)pData;
    if (!pStream) {
        LOG_E("BoxRecorder::OnRecvData() recv nil data");
        return AX_FALSE;
    }

    if (!m_stAttr.bMuxer) {
        if (m_ofs.is_open()) {
            m_ofs.write((const char *)(pStream->stPack.pu8Addr), pStream->stPack.u32Len);
            m_nFileSize += pStream->stPack.u32Len;
            if (m_stAttr.nMaxRecordSize > 0 && m_nFileSize > m_stAttr.nMaxRecordSize) {
                CreateRawFile();
            }
        }
    } else {
        AX_BOOL bIFrame = (AX_VENC_INTRA_FRAME == pStream->stPack.enCodingType) ? AX_TRUE : AX_FALSE;
        m_pMuxer->SendRawFrame(m_stAttr.veChn, pStream->stPack.pu8Addr, pStream->stPack.u32Len, pStream->stPack.u64PTS, bIFrame);
    }

    return AX_TRUE;
}

AX_BOOL CBoxRecorder::OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) {
    return AX_TRUE;
}

AX_BOOL CBoxRecorder::CreateRawFile(AX_VOID) {
    if (m_ofs.is_open()) {
        m_ofs.flush();
        m_ofs.close();
    }

    timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);
    time_t now = time(NULL);
    localtime_r(&now, &t);

    AX_CHAR szName[64];
    sprintf(szName, "record_%04d_%02d_%02d_%02d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    if (PT_H264 == m_stAttr.ePayloadType) {
        strcat(szName, ".264");
    } else {
        strcat(szName, ".265");
    }

    std::string strPath = m_stAttr.strRecordPath + szName;
    m_ofs.open(strPath.c_str(), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    if (!m_ofs.is_open()) {
        LOG_E("%s: open %s file fail, %s", __func__, strPath.c_str(), strerror(errno));
        return AX_FALSE;
    }

    m_nFileSize = 0;
    return AX_TRUE;
}

AX_BOOL CBoxRecorder::CreateEncoderInstance(AX_VOID) {
    VIDEO_CONFIG_T conf;
    conf.nChannel = m_stAttr.veChn;
    conf.ePayloadType = m_stAttr.ePayloadType;
    conf.nGOP = ((0 == m_stAttr.nGop) ? m_stAttr.nFps : m_stAttr.nGop);
    conf.fFramerate = m_stAttr.nFps;
    conf.nWidth = m_stAttr.nW;
    conf.nHeight = m_stAttr.nH;
    conf.eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    conf.stEncodeCfg[0].ePayloadType = conf.ePayloadType;
    conf.stEncodeCfg[0].stRCInfo[0] = m_stAttr.stRC;
    conf.eRcType = m_stAttr.stRC.eRcType;

    conf.nBitrate = m_stAttr.nBitRate;
    conf.bFBC = AX_FALSE;
    conf.bLink = m_stAttr.bLinkMode;
    conf.nInFifoDepth = m_stAttr.nFifoDepth[0];
    conf.nOutFifoDepth = m_stAttr.nFifoDepth[1];
    conf.eMemSource = AX_MEMORY_SOURCE_CMM;

    m_pEncoder = std::make_unique<CVideoEncoder>(conf);
    if (!m_pEncoder) {
        LOG_M_E(REC, "create record encoder instance fail");
        return AX_FALSE;
    }

    do {
        if (!m_pEncoder->InitParams()) {
            break;
        }

        m_pEncoder->RegObserver(this);

        if (!m_pEncoder->Init()) {
            break;
        }

        return AX_TRUE;

    } while (0);

    m_pEncoder = nullptr;
    return AX_FALSE;
}

AX_BOOL CBoxRecorder::CreateMuxerInstance(AX_VOID) {
    MPEG4EC_INFO_T stMp4;

    stMp4.nchn = m_stAttr.veChn;
    stMp4.strSavePath = m_stAttr.strRecordPath;
    stMp4.nMaxFileInMBytes = m_stAttr.nMaxRecordSize;

    stMp4.stVideoAttr.bEnable = AX_TRUE;
    stMp4.stVideoAttr.ePt = m_stAttr.ePayloadType;
    stMp4.stVideoAttr.nFrameRate = m_stAttr.nFps;
    stMp4.stVideoAttr.nfrWidth = m_stAttr.nW;
    stMp4.stVideoAttr.nfrHeight = m_stAttr.nH;
    stMp4.stVideoAttr.nBitrate = m_stAttr.nBitRate;
    stMp4.stVideoAttr.nMaxFrmSize = m_stAttr.nW * m_stAttr.nH / 8;

    LOG_M_C(REC, "MP4 chn %d, %dx%d, fps %d, bps %d, 264(%d), max file size %d MB, path %s", stMp4.nchn, stMp4.stVideoAttr.nfrWidth, stMp4.stVideoAttr.nfrHeight,
            stMp4.stVideoAttr.nFrameRate, stMp4.stVideoAttr.nBitrate, stMp4.stVideoAttr.ePt, stMp4.nMaxFileInMBytes, stMp4.strSavePath.c_str());
    m_pMuxer = std::make_unique<CMPEG4Encoder>();
    if (!m_pMuxer) {
        LOG_M_E(REC, "create record muxer instance fail");
        return AX_FALSE;
    }

    do {
        if (!m_pMuxer->InitParam(stMp4)) {
            break;
        }

        if (!m_pMuxer->Init()) {
            break;
        }

        return AX_TRUE;

    } while (0);

    m_pMuxer = nullptr;
    return AX_FALSE;
}

AX_BOOL CBoxRecorder::SendFrame(CAXFrame *pFrame) {
    if (pFrame) {
        return m_pEncoder->EnqueueFrame(pFrame);
    }

    return AX_FALSE;
}
