/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "BoxBuilder.hpp"
#include <stdlib.h>
#include <algorithm>
#include "AXPoolManager.hpp"
#include "AppLogApi.h"
#include "DetectObserver.hpp"
#include "DiskHelper.hpp"
#include "DispatchObserver.hpp"
#include "GlobalDef.h"
#include "StreamerFactory.hpp"
#include "VoObserver.hpp"
#include "make_unique.hpp"

#define BOX "BOX"
using namespace std;

#define VDEC_CHN0 0
#define VDEC_CHN1 1
#define VDEC_CHN2 2
#define DISPVO_CHN VDEC_CHN1
#define DETECT_CHN VDEC_CHN2

AX_BOOL CBoxBuilder::Init(AX_VOID) {
    m_sys.InitAppLog("BoxDemo");

    CBoxConfig *pConfig = CBoxConfig::GetInstance();
    if (!pConfig->Init()) {
        LOG_M_E(BOX, "%s: load box config file fail", __func__);
        return AX_FALSE;
    }

    /* [1]: Load configuration */
    STREAM_CONFIG_T streamConfig = pConfig->GetStreamConfig();
    DETECT_CONFIG_T detectConfig = pConfig->GetDetectConfig();
    DISPVO_CONFIG_T dispVoConfig = pConfig->GetDispVoConfig("DISPC");
    DISPVO_CONFIG_T dispVoConfig_1 = pConfig->GetDispVoConfig("DISPC1");

    if (dispVoConfig.bRecord && dispVoConfig.bOnlineMode) {
        dispVoConfig.bOnlineMode = AX_FALSE;
        LOG_M_C(BOX, ">>>>>>>>>>>>> recorder must set vo to offline mode <<<<<<<<<<<<<<");
    }

    m_nDecodeGrpCount = streamConfig.nDecodeGrps;
    if (m_nDecodeGrpCount >= MAX_VO_CHN_NUM) {
        m_nDecodeGrpCount = (dispVoConfig.bShowLogo) ? (MAX_VO_CHN_NUM - 1) : MAX_VO_CHN_NUM;
        LOG_M_E(BOX, "%s: configured %d decoded videos, but %d videos are actived", __func__, streamConfig.nDecodeGrps, m_nDecodeGrpCount);
        streamConfig.nDecodeGrps = m_nDecodeGrpCount;
    }

    if (dispVoConfig.nDevId < 0) {
        LOG_M_E(BOX, "HDMI main device id invalid, please check [DISPC] of box.conf");
        return AX_FALSE;
    }

    if (!streamConfig.strSataPath.empty()) {
        if (!CheckDiskSpace(streamConfig)) {
            return AX_FALSE;
        }
    }

    /* [2]: Init system */
    BOX_APP_SYS_ATTR_T tSysAttr{.nMaxGrp = (AX_U32)m_nDecodeGrpCount};
    if (!m_sys.Init(tSysAttr)) {
        return AX_FALSE;
    }

    /* [3]: Init streamers */
    if (!InitStreamer(streamConfig)) {
        return AX_FALSE;
    }

    /* [4]: Init display and observer */
    AX_U32 nVoChn = m_nDecodeGrpCount;
    if (dispVoConfig_1.nDevId != -1 && dispVoConfig_1.nDispType == 1) {
        nVoChn = m_nDecodeGrpCount / 2;
    }

    dispVoConfig.nChnDepth = detectConfig.nSkipRate;
    if (!InitDisplay(AX_DISPDEV_TYPE::PRIMARY, dispVoConfig, nVoChn)) {
        return AX_FALSE;
    } else {
        /* display recorder */
        if (dispVoConfig.bRecord) {
            if (!InitDispRecorder(dispVoConfig.strRecordPath, dispVoConfig.nMaxRecordSize, dispVoConfig.bRecordMuxer)) {
                return AX_FALSE;
            }
        }
    }

    dispVoConfig_1.nChnDepth = detectConfig.nSkipRate;
    if (!InitDisplay(AX_DISPDEV_TYPE::SECONDARY, dispVoConfig_1, nVoChn)) {
        return AX_FALSE;
    }

#if 0
    if (dispVoConfig.bOnlineMode || (dispVoConfig_1.nDevId > -1 && dispVoConfig_1.bOnlineMode)) {
        /* fixme: VO online worst cast: keep VB by 2 dispc interrupts */
        if (streamConfig.nChnDepth[DISPVO_CHN] < 6) {
            streamConfig.nChnDepth[DISPVO_CHN] = 6;
        }
    }
#endif

    /* verify */
    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        /* VDEC has no scaler */
        const STREAMER_INFO_T &stream = m_arrStreamer[i]->GetStreamInfo();
        AX_U32 nMinW = std::max(detectConfig.nW, m_disp->GetVideoLayout()[0].u32Width);
        AX_U32 nMinH = std::max(detectConfig.nH, m_disp->GetVideoLayout()[0].u32Height);
        if (stream.nWidth < nMinW || stream.nHeight < nMinH) {
            LOG_M_E(BOX, "width %d x height %d of stream < %s > is not supported, please change video which at least > %d x %d",
                    stream.nWidth, stream.nHeight, stream.strPath.c_str(), nMinW, nMinH);
            return AX_FALSE;
        }
    }

    /* [5]: Init detector and observer */
    if (detectConfig.bEnable) {
        CDetectResult::GetInstance()->Clear();
        if (!InitDetector(detectConfig)) {
            return AX_FALSE;
        }
    }

    /* [6]: Init dispatchers */
    AX_U32 nDispType = 2;
    if (dispVoConfig_1.nDevId != -1) {
        nDispType = dispVoConfig_1.nDispType;
    }

    if (!streamConfig.nLinkMode) {
        if (!InitDispatcher(dispVoConfig.strBmpPath, nDispType)) {
            return AX_FALSE;
        }
    } else {
        m_arrDispatcher.clear();
        m_arrDispatchObserver.clear();
    }

    /* [7]: Init video decoder */
    streamConfig.nChnW[DISPVO_CHN] = m_disp->GetVideoLayout()[0].u32Width;
    streamConfig.nChnH[DISPVO_CHN] = m_disp->GetVideoLayout()[0].u32Height;
    streamConfig.nChnW[DETECT_CHN] = detectConfig.nW;
    streamConfig.nChnH[DETECT_CHN] = detectConfig.nH;
    if (!InitDecoder(streamConfig)) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        VDEC_GRP_ATTR_T tGrpAttr;
        m_vdec->GetGrpAttr(i, tGrpAttr);

        AX_U32 nFps = (AX_VDEC_DISPLAY_MODE_PREVIEW == tGrpAttr.eDecodeMode) ? 0 : tGrpAttr.nFps;

        if (m_dispSecondary) {
            if (1 /* DIFF */ == nDispType) {
                if ((i % 2) == 0) {
                    m_disp->SetChnFrameRate(m_disp->GetVideoChn(i / 2), nFps);
                } else {
                    m_dispSecondary->SetChnFrameRate(m_dispSecondary->GetVideoChn(i / 2), nFps);
                }
            } else {
                m_disp->SetChnFrameRate(m_disp->GetVideoChn(i), nFps);
                m_dispSecondary->SetChnFrameRate(m_dispSecondary->GetVideoChn(i), nFps);
            }
        } else {
            m_disp->SetChnFrameRate(m_disp->GetVideoChn(i), nFps);
        }
    }

    /* [8]: vo link vdec */
    if (streamConfig.nLinkMode) {
        if (dispVoConfig_1.nDevId != -1) {
            if (dispVoConfig_1.nDispType == 1) {
                AX_U32 nVoChn = m_nDecodeGrpCount / 2;
                // primary
                for (AX_U32 i = 0; i < nVoChn; ++i) {
                    AX_S32 voGrp = (AX_S32)m_disp->GetVideoLayer();
                    AX_S32 voChn = (AX_S32)m_disp->GetVideoChn(i);
                    m_linker.Link({AX_ID_VDEC, (AX_S32)(i * 2), DISPVO_CHN}, {AX_ID_VO, voGrp, voChn});
                }
                // secondary
                for (AX_U32 i = 0; i < nVoChn; ++i) {
                    AX_S32 voGrp = (AX_S32)m_dispSecondary->GetVideoLayer();
                    AX_S32 voChn = (AX_S32)m_dispSecondary->GetVideoChn(i);
                    m_linker.Link({AX_ID_VDEC, (AX_S32)(i * 2 + 1), DISPVO_CHN}, {AX_ID_VO, voGrp, voChn});
                }
            } else if (dispVoConfig_1.nDispType == 0) {
                for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
                    // primary
                    {
                        AX_S32 voGrp = (AX_S32)m_disp->GetVideoLayer();
                        AX_S32 voChn = (AX_S32)m_disp->GetVideoChn(i);
                        LOG_C("link: vdec vdGrp %d vdChn %d ==> vo voGrp %d voChn %d", i, DISPVO_CHN, voGrp, voChn);
                        m_linker.Link({AX_ID_VDEC, (AX_S32)i, DISPVO_CHN}, {AX_ID_VO, voGrp, voChn});
                    }
                    // secondary
                    {
                        AX_S32 voGrp = (AX_S32)m_dispSecondary->GetVideoLayer();
                        AX_S32 voChn = (AX_S32)m_dispSecondary->GetVideoChn(i);
                        LOG_C("link: vdec vdGrp %d vdChn %d ==> vo voGrp %d voChn %d", i, DISPVO_CHN, voGrp, voChn);
                        m_linker.Link({AX_ID_VDEC, (AX_S32)i, DISPVO_CHN}, {AX_ID_VO, voGrp, voChn});
                    }
                }
            } else {
                LOG_M_E(BOX, "Invalid dual-screen display mode. mode=%d", __func__, dispVoConfig_1.nDispType);
                return AX_FALSE;
            }
        } else {
            for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
                AX_S32 voGrp = (AX_S32)m_disp->GetVideoLayer();
                AX_S32 voChn = (AX_S32)m_disp->GetVideoChn(i);
                LOG_C("link: vdec vdGrp %d vdChn %d ==> vo voGrp %d voChn %d", i, DISPVO_CHN, voGrp, voChn);
                m_linker.Link({AX_ID_VDEC, (AX_S32)i, DISPVO_CHN}, {AX_ID_VO, voGrp, voChn});
            }
        }
    }

#if defined(__RECORD_VB_TIMESTAMP__)
    AllocTimestampBufs();
#endif

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::InitStreamer(const STREAM_CONFIG_T &streamConfig) {
    const AX_U32 nCount = streamConfig.v.size();
    m_arrStreamer.resize(nCount);
    for (AX_U32 i = 0; i < nCount; ++i) {
        STREAMER_ATTR_T stAttr;
        stAttr.strPath = streamConfig.v[i];
        stAttr.nMaxWidth = streamConfig.nMaxGrpW;
        stAttr.nMaxHeight = streamConfig.nMaxGrpH;
        stAttr.nCookie = (AX_S32)i;
        stAttr.bLoop = AX_TRUE;

        stAttr.nForceFps = streamConfig.nDefaultFps;
        stAttr.nMaxSendNaluIntervalMilliseconds = CBoxConfig::GetInstance()->GetUTConfig().nMaxSendNaluIntervalMilliseconds;

        m_arrStreamer[i] = CStreamerFactory::GetInstance()->CreateHandler(stAttr.strPath);
        if (!m_arrStreamer[i]) {
            return AX_FALSE;
        }

        if (!m_arrStreamer[i]->Init(stAttr)) {
            return AX_FALSE;
        }

        LOG_M_C(BOX, "stream %d: %s", i, stAttr.strPath.c_str());
    }

    if (!streamConfig.strSataPath.empty()) {
        m_sataWritter.resize(nCount);
        for (AX_U32 i = 0; i < nCount; ++i) {
            STREAM_RECORD_ATTR_T stAttr = {i, streamConfig.nSataFileSize, streamConfig.nMaxSpaceSize, streamConfig.strSataPath};
            m_sataWritter[i] = make_unique<CStreamRecorder>();
            if (!m_sataWritter[i]) {
                return AX_FALSE;
            }

            if (!m_sataWritter[i]->Init(stAttr)) {
                return AX_FALSE;
            }

            m_arrStreamer[i]->RegObserver(m_sataWritter[i].get());
        }
    } else {
        m_sataWritter.clear();
    }

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::InitDisplay(AX_DISPDEV_TYPE enDispDev, const DISPVO_CONFIG_T &dispVoConfig, AX_U32 nChnCount) {
    if (dispVoConfig.nDevId == -1) {
        return AX_TRUE;
    }

    if (AX_DISPDEV_TYPE::PRIMARY == enDispDev) {
        m_disp = make_unique<CVo>();
        if (!m_disp) {
            LOG_M_E(BOX, "%s: create display instance fail", __func__);
            return AX_FALSE;
        }
    } else if (AX_DISPDEV_TYPE::SECONDARY == enDispDev) {
        m_dispSecondary = make_unique<CVo>();
        if (!m_dispSecondary) {
            LOG_M_E(BOX, "%s: create secondary display instance fail", __func__);
            return AX_FALSE;
        }
    }

    VO_ATTR_T stAttr;
    stAttr.voDev = dispVoConfig.nDevId;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = (AX_VO_INTF_SYNC_E)dispVoConfig.nHDMI;
    stAttr.nBgClr = 0x000000;
    if (AX_DISPDEV_TYPE::SECONDARY == enDispDev) stAttr.nBgClr = 0x0000ff;
    stAttr.nLayerDepth = dispVoConfig.nLayerDepth;
    stAttr.nTolerance = dispVoConfig.nTolerance;
    stAttr.strResDirPath = dispVoConfig.strResDirPath;
    stAttr.bShowLogo = dispVoConfig.bShowLogo;
    stAttr.bShowNoVideo = dispVoConfig.bShowNoVideo;
    stAttr.enMode = (dispVoConfig.bOnlineMode ? AX_VO_MODE_ONLINE : AX_VO_MODE_OFFLINE);
    stAttr.arrChns.resize(nChnCount);
    for (auto &&m : stAttr.arrChns) {
        m.nPriority = 0;
        m.nDepth = dispVoConfig.nChnDepth;
        if (m.nDepth < 2) {
            m.nDepth = 2;
        }
    }

    if (AX_DISPDEV_TYPE::PRIMARY == enDispDev) {
        if (!m_disp->Init(stAttr)) {
            return AX_FALSE;
        }
        m_dispObserver = CObserverMaker::CreateObserver<CVoObserver>(m_disp.get(), DISPVO_CHN);
        if (!m_dispObserver) {
            LOG_M_E(BOX, "%s: create display observer instance fail", __func__);
            return AX_FALSE;
        }
    } else if (AX_DISPDEV_TYPE::SECONDARY == enDispDev) {
        if (!m_dispSecondary->Init(stAttr)) {
            return AX_FALSE;
        }
        m_dispObserverSecondary = CObserverMaker::CreateObserver<CVoObserver>(m_dispSecondary.get(), DISPVO_CHN);
        if (!m_dispObserverSecondary) {
            LOG_M_E(BOX, "%s: create display observer instance fail", __func__);
            return AX_FALSE;
        }
    }
    return AX_TRUE;
}

AX_BOOL CBoxBuilder::InitDispRecorder(const string &strRecordPath, AX_S32 nMaxRecordSize, AX_BOOL bMP4) {
    m_dispRecorder = make_unique<CBoxRecorder>();
    if (!m_dispRecorder) {
        LOG_M_E(BOX, "%s: create record instance fail", __func__);
        return AX_FALSE;
    }

    VENC_CONFIG_T vencConfig = CBoxConfig::GetInstance()->GetVencConfig();
    const VO_ATTR_T &tVoAttr = m_disp->GetAttr();
    BOX_RECORDER_ATTR_T conf;
    conf.veChn = 0;
    conf.nFifoDepth[0] = vencConfig.nFifoDepth[0];
    conf.nFifoDepth[1] = vencConfig.nFifoDepth[1];
    conf.nW = tVoAttr.nW;
    conf.nH = tVoAttr.nH;
    conf.nFps = tVoAttr.nHz;
    conf.ePayloadType = (AX_PAYLOAD_TYPE_E)(vencConfig.nPayloadType);
    conf.stRC.eRcType = (AX_VENC_RC_MODE_E)(vencConfig.nRCType);
    conf.stRC.nMinQp = vencConfig.nMinQp;
    conf.stRC.nMaxQp = vencConfig.nMaxQp;
    conf.stRC.nMinIQp = vencConfig.nMinIQp;
    conf.stRC.nMaxIQp = vencConfig.nMaxIQp;
    conf.stRC.nMaxIProp = vencConfig.nMaxIProp;
    conf.stRC.nMinIProp = vencConfig.nMinIProp;
    conf.stRC.nIntraQpDelta = vencConfig.nIntraQpDelta;
    conf.nGop = vencConfig.nGop;
    conf.nBitRate = vencConfig.nBitRate;
    conf.strRecordPath = strRecordPath;
    conf.nMaxRecordSize = ((nMaxRecordSize <= 0) ? 0 : nMaxRecordSize);
    conf.bMuxer = bMP4;
    conf.bLinkMode = AX_TRUE;
    if (!m_dispRecorder->Init(conf)) {
        return AX_FALSE;
    }

    /* link from vo draw layer to venc */
    m_linker.Link({AX_ID_VO, (AX_S32)m_disp->GetVideoLayer(), 0}, {AX_ID_VENC, (AX_S32)conf.veChn, 0});
    LOG_M_N(BOX, "enable recording, save path %s", strRecordPath.c_str());

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::InitDispatcher(const string &strFontPath, AX_U32 nDispType) {
    m_arrDispatcher.resize(m_nDecodeGrpCount);
    m_arrDispatchObserver.resize(m_nDecodeGrpCount);
    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        m_arrDispatcher[i] = make_unique<CDispatcher>();
        if (!m_arrDispatcher[i]) {
            LOG_M_E(BOX, "%s: create dispatcher %d instance fail", __func__, i);
            return AX_FALSE;
        } else {
            if (m_dispObserverSecondary) {
                m_arrDispatcher[i]->RegObserver(m_dispObserverSecondary.get());
            }
            if (m_dispObserver) {
                m_arrDispatcher[i]->RegObserver(m_dispObserver.get());
            }
        }

        DISPATCH_ATTR_T stAttr;
        stAttr.vdGrp = i;
        stAttr.strBmpFontPath = strFontPath;
        stAttr.nDepth = -1;
        stAttr.enDispType = AX_DISP_TYPE(nDispType);
        if (!m_arrDispatcher[i]->Init(stAttr)) {
            return AX_FALSE;
        }

        m_arrDispatchObserver[i] = CObserverMaker::CreateObserver<CDispatchObserver>(m_arrDispatcher[i].get(), DISPVO_CHN);
        if (!m_arrDispatchObserver[i]) {
            LOG_M_E(BOX, "%s: create dispatch %d observer instance fail", __func__, i);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::InitDetector(const DETECT_CONFIG_T &detectConfig) {
    m_detect = make_unique<CDetector>();
    if (!m_detect) {
        LOG_M_E(BOX, "%s: create detector instance fail", __func__);
        return AX_FALSE;
    }

    DETECTOR_ATTR_T tDetectAttr;
    tDetectAttr.nGrpCount = m_nDecodeGrpCount;
    tDetectAttr.nSkipRate = detectConfig.nSkipRate;
    tDetectAttr.nW = detectConfig.nW;
    tDetectAttr.nH = detectConfig.nH;
    tDetectAttr.nDepth = detectConfig.nDepth * m_nDecodeGrpCount;
    tDetectAttr.strModelPath = detectConfig.strModelPath;
    tDetectAttr.nChannelNum = AX_MIN(detectConfig.nChannelNum, DETECTOR_MAX_CHN_NUM);
    for (AX_U32 i = 0; i < tDetectAttr.nChannelNum; ++i) {
        tDetectAttr.tChnAttr[i].nPPL = detectConfig.tChnParam[i].nPPL;
        tDetectAttr.tChnAttr[i].nVNPU = detectConfig.tChnParam[i].nVNPU;
        tDetectAttr.tChnAttr[i].bTrackEnable = detectConfig.tChnParam[i].bTrackEnable;
    }
    if (!m_detect->Init(tDetectAttr)) {
        return AX_FALSE;
    }

    m_detectObserver = CObserverMaker::CreateObserver<CDetectObserver>(m_detect.get(), DETECT_CHN);
    if (!m_detectObserver) {
        LOG_M_E(BOX, "%s: create detect observer fail", __func__);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::InitDecoder(const STREAM_CONFIG_T &streamConfig) {
    m_vdec = make_unique<CVideoDecoder>();
    if (!m_vdec) {
        LOG_M_E(BOX, "%s: create vidoe decoder instance instance fail", __func__);
        return AX_FALSE;
    }

    const COMPRESS_CONFIG_T fbc = CBoxConfig::GetInstance()->GetCompressConfig();
    const DETECT_CONFIG_T &detCfg = CBoxConfig::GetInstance()->GetDetectConfig();

    vector<VDEC_GRP_ATTR_T> arrVdGrps(m_nDecodeGrpCount);
    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        const STREAMER_INFO_T &stInfo = m_arrStreamer[i]->GetStreamInfo();

        VDEC_GRP_ATTR_T tGrpAttr;
        tGrpAttr.bEnable = AX_TRUE;
        tGrpAttr.enCodecType = stInfo.eVideoType;
        tGrpAttr.nMaxWidth = ALIGN_UP(streamConfig.nMaxGrpW, 16);  /* H264 MB 16x16 */
        tGrpAttr.nMaxHeight = ALIGN_UP(streamConfig.nMaxGrpH, 16); /* H264 MB 16x16 */

        if (0 == streamConfig.nDefaultFps || STREAM_TYPE_E::RTSP == stInfo.eStreamType) {
            /* if default fps is 0 or RTSP, fps is parsed by streamer */
            tGrpAttr.nFps = stInfo.nFps;
        } else {
            /* use configured fps for file streamer */
            tGrpAttr.nFps = streamConfig.nDefaultFps;
        }

        if (STREAM_TYPE_E::FILE == stInfo.eStreamType) {
            /* TODO: debug specified fps for VO module */
            char name[32];
            sprintf(name, "VDEC_GRP%d_DECODED_FPS", i);
            const char *env = getenv(name);
            if (env) {
                tGrpAttr.nFps = atoi(env);
            }
        }

        tGrpAttr.bPrivatePool = (2 == streamConfig.nUserPool) ? AX_TRUE : AX_FALSE;

        if (STREAM_TYPE_E::RTSP == stInfo.eStreamType) {
            /* RTSP: always preview + frame mode */
            tGrpAttr.eDecodeMode = AX_VDEC_DISPLAY_MODE_PREVIEW;
            tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
            tGrpAttr.nMaxStreamBufSize = tGrpAttr.nMaxWidth * tGrpAttr.nMaxHeight * 3 / 2;
        } else {
            /* FILE: playback + frame or stream mode according configuration */
            tGrpAttr.eDecodeMode = AX_VDEC_DISPLAY_MODE_PLAYBACK;
            if (0 == streamConfig.nInputMode) {
                tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
                tGrpAttr.nMaxStreamBufSize = tGrpAttr.nMaxWidth * tGrpAttr.nMaxHeight * 3 / 2;
            } else {
                tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_STREAM;
                tGrpAttr.nMaxStreamBufSize = streamConfig.nMaxStreamBufSize;
            }
        }

        for (AX_U32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            AX_VDEC_CHN_ATTR_T &tChnAttr = tGrpAttr.stChnAttr[j];
            switch (j) {
                case VDEC_CHN0:
                    /* pp0 disable, because scaler is not support */
                    tGrpAttr.bChnEnable[j] = AX_FALSE;
                    break;
                case DISPVO_CHN:
                    /* pp1 scaler max. 4096x2160 */
                    tGrpAttr.bChnEnable[j] = AX_TRUE;
                    tChnAttr.u32PicWidth = streamConfig.nChnW[j];
                    tChnAttr.u32PicHeight = streamConfig.nChnH[j];
                    tChnAttr.u32FrameStride = ALIGN_UP(tChnAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                    if (0 == streamConfig.nLinkMode) {
                        tChnAttr.u32OutputFifoDepth = streamConfig.nChnDepth[j];
                    } else {
                        tChnAttr.u32OutputFifoDepth = 0;
                    }
                    tChnAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                    tChnAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                    tChnAttr.stCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)(fbc.nMode);
                    if (AX_COMPRESS_MODE_LOSSY == fbc.nMode) {
                        tChnAttr.stCompressInfo.u32CompressLevel = fbc.nLv;
                    }
                    break;
                case DETECT_CHN:
                    /* pp2 scaler max. 1920x1080 */
                    tGrpAttr.bChnEnable[j] = (m_detect ? AX_TRUE : AX_FALSE);
                    if (tGrpAttr.bChnEnable[j]) {
                        tChnAttr.u32PicWidth = streamConfig.nChnW[j];
                        tChnAttr.u32PicHeight = streamConfig.nChnH[j];
                        tChnAttr.u32FrameStride = ALIGN_UP(tChnAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                        tChnAttr.u32OutputFifoDepth = streamConfig.nChnDepth[j];
                        tChnAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                        tChnAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                        tChnAttr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
#ifdef __VDEC_PP_FRAME_CTRL__
                        if (detCfg.nSkipRate > 1) {
                            tChnAttr.stOutputFrmRate.bFrmRateCtrl = AX_TRUE;
                            tChnAttr.stOutputFrmRate.f32DstFrmRate = stInfo.nFps * 1.0 / detCfg.nSkipRate;
                        }
#endif
                    }
                    break;
                default:
                    break;
            }
        }

        arrVdGrps[i] = move(tGrpAttr);
    }

    if (!m_vdec->Init(arrVdGrps)) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        /* register vdec to streamer */
        m_arrStreamer[i]->RegObserver(m_vdec.get());

        AX_VDEC_GRP vdGrp = (AX_VDEC_GRP)i;

        if (!streamConfig.nLinkMode) {
            m_vdec->RegObserver(vdGrp, m_arrDispatchObserver[i].get());
        }

        if (m_detectObserver) {
            m_vdec->RegObserver(vdGrp, m_detectObserver.get());
        }

        VDEC_GRP_ATTR_T tGrpAttr;
        m_vdec->GetGrpAttr(vdGrp, tGrpAttr);
        for (AX_U32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            if (!tGrpAttr.bChnEnable[j]) {
                continue;
            }

            AX_VDEC_CHN_ATTR_T &stChn = tGrpAttr.stChnAttr[j];
            AX_U32 nBlkSize = CVideoDecoder::GetBlkSize(stChn.u32PicWidth, stChn.u32PicHeight, VDEC_STRIDE_ALIGN, tGrpAttr.enCodecType,
                                                        &stChn.stCompressInfo, stChn.enImgFormat);

            if (tGrpAttr.bPrivatePool) {
                /* fixme: we should calculate blksize by SPS such as sps.frame_cropping_flags */
                stChn.u32FrameBufSize = nBlkSize;
                stChn.u32FrameBufCnt = streamConfig.nChnDepth[j];
                m_vdec->SetChnAttr(vdGrp, j, stChn);
                continue;
            }

            if (0 == streamConfig.nUserPool) {
                CAXPoolManager::GetInstance()->AddBlockToFloorPlan(nBlkSize, streamConfig.nChnDepth[j]);
                LOG_M_N(BOX, "VDEC vdGrp %d vdChn %d blkSize %d blkCount %d", vdGrp, j, nBlkSize, streamConfig.nChnDepth[j]);
            } else {
                AX_POOL_CONFIG_T stPoolConfig;
                memset(&stPoolConfig, 0, sizeof(stPoolConfig));
                stPoolConfig.MetaSize = 4096;
                stPoolConfig.BlkSize = nBlkSize;
                stPoolConfig.BlkCnt = streamConfig.nChnDepth[j];
                stPoolConfig.IsMergeMode = AX_FALSE;
                stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
                sprintf((AX_CHAR *)stPoolConfig.PoolName, "vdec_%d_pp%d_pool", i, j);
                AX_POOL pool = CAXPoolManager::GetInstance()->CreatePool(stPoolConfig);
                if (AX_INVALID_POOLID == pool) {
                    return AX_FALSE;
                }

                if (!m_vdec->AttachPool(vdGrp, (AX_VDEC_CHN)j, pool)) {
                    return AX_FALSE;
                }

                LOG_M_C(BOX, "pool %2d (blkSize %d blkCount %d) is attached to VDEC vdGrp %d vdChn %d", pool, stPoolConfig.BlkSize,
                        stPoolConfig.BlkCnt, vdGrp, j);
            }
        }
    }

    if (0 == streamConfig.nUserPool) {
        if (!CAXPoolManager::GetInstance()->CreateFloorPlan(512)) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::DeInit(AX_VOID) {
    /* destory instances */
#define DESTORY_INSTANCE(p) \
    do {                    \
        if (p) {            \
            p->DeInit();    \
            p = nullptr;    \
        }                   \
    } while (0)

    for (auto &&m : m_arrStreamer) {
        DESTORY_INSTANCE(m);
    }

    for (auto &&m : m_sataWritter) {
        DESTORY_INSTANCE(m);
    }

    for (auto &&m : m_arrDispatcher) {
        DESTORY_INSTANCE(m);
    }
    m_arrDispatcher.clear();

    /* If private pool, destory consumer before producer */
    DESTORY_INSTANCE(m_disp);
    DESTORY_INSTANCE(m_dispSecondary);
    DESTORY_INSTANCE(m_detect);
    DESTORY_INSTANCE(m_vdec);

#undef DESTORY_INSTANCE

#if defined(__RECORD_VB_TIMESTAMP__)
    FreeTimestampBufs();
#endif

    m_sys.DeInit();
    m_sys.DeInitAppLog();
    return AX_TRUE;
}

AX_BOOL CBoxBuilder::Start(AX_VOID) {
    if (!Init()) {
        DeInit();
        return AX_FALSE;
    }

    do {
        if (m_dispRecorder) {
            if (!m_dispRecorder->Start()) {
                return AX_FALSE;
            }
        }

        if (m_disp) {
            if (!m_disp->Start()) {
                return AX_FALSE;
            }
        } else {
            LOG_M_E(BOX, "%s: >>>>>>>>>>>>>>>> DISP module is disabled <<<<<<<<<<<<<<<<<<<<<", __func__);
        }

        if (m_dispSecondary) {
            if (!m_dispSecondary->Start()) {
                return AX_FALSE;
            }
        }

        if (m_detect) {
            if (!m_detect->Start()) {
                return AX_FALSE;
            }
        }

        for (auto &m : m_arrDispatcher) {
            if (!m->Start()) {
                return AX_FALSE;
            }
        }

        if (m_vdec) {
            if (!m_vdec->Start()) {
                return AX_FALSE;
            }
        } else {
            LOG_M_E(BOX, "%s: >>>>>>>>>>>>>>>> VDEC module is disabled <<<<<<<<<<<<<<<<<<<<<", __func__);
            return AX_FALSE;
        }

        for (auto &&m : m_sataWritter) {
            if (m) {
                if (!m->Start()) {
                    return AX_FALSE;
                }
            }
        }

        for (auto &&m : m_arrStreamer) {
            if (m) {
                thread t([](IStreamHandler *p) { p->Start(); }, m.get());
                t.detach();
            }
        }

        return AX_TRUE;

    } while (0);

    StopAllStreams();
    WaitDone();
    return AX_FALSE;
}

AX_BOOL CBoxBuilder::WaitDone(AX_VOID) {
    STREAM_CONFIG_T streamConfig = CBoxConfig::GetInstance()->GetStreamConfig();
    m_linker.UnlinkAll();

    for (auto &&m : m_sataWritter) {
        if (m) {
            m->Stop();
        }
    }

    if (m_detect) {
        m_detect->Stop();
    }

    if (m_dispRecorder) {
        m_dispRecorder->Stop();
    }

    if (m_disp) {
        m_disp->Stop();
    }

    if (m_dispSecondary) {
        m_dispSecondary->Stop();
    }

    for (auto &&m : m_arrDispatcher) {
        m->Stop();
    }

    if (m_vdec) {
        m_vdec->Stop();
    }

    DeInit();
    return AX_TRUE;
}

AX_BOOL CBoxBuilder::QueryStreamsAllEof(AX_VOID) {
    AX_U32 nEofCnt = 0;

    STREAMER_STAT_T stStat;
    for (auto &&m : m_arrStreamer) {
        if (m) {
            m->QueryStatus(stStat);
            if (!stStat.bStarted) {
                ++nEofCnt;
            }
        } else {
            ++nEofCnt;
        }
    }

    return (nEofCnt >= m_arrStreamer.size()) ? AX_TRUE : AX_FALSE;
}

AX_BOOL CBoxBuilder::StopAllStreams(AX_VOID) {
    AX_U32 nCount = m_arrStreamer.size();
    for (AX_U32 i = 0; i < nCount; ++i) {
        if (m_sataWritter.size() > i && m_sataWritter[i]) {
            m_arrStreamer[i]->UnRegObserver(m_sataWritter[i].get());
        }

        m_arrStreamer[i]->UnRegObserver(m_vdec.get());
    }

    m_vdec->UnRegAllObservers();

    for (auto &&m : m_arrDispatcher) {
        if (m_dispObserver) {
            m->UnRegObserver(m_dispObserver.get());
        }

        if (m_dispObserverSecondary) {
            m->UnRegObserver(m_dispObserverSecondary.get());
        }

        m->Clear();
    }

    if (m_detect) {
        m_detect->Clear();
    }

    vector<thread> v;
    v.reserve(nCount);
    for (auto &&m : m_arrStreamer) {
        if (m) {
            STREAMER_STAT_T stStat;
            if (m->QueryStatus(stStat) && stStat.bStarted) {
                v.emplace_back([](IStreamHandler *p) { p->Stop(); }, m.get());
            }
        }
    }

    for (auto &&m : v) {
        if (m.joinable()) {
            m.join();
        }
    }

    return AX_TRUE;
}

AX_BOOL CBoxBuilder::CheckDiskSpace(const STREAM_CONFIG_T &streamConfig) {
    const AX_U32 TOTAL_STREAM_COUNT = streamConfig.v.size();

    AX_U64 nUsedSpace = {0};
    for (AX_U32 i = 0; i < TOTAL_STREAM_COUNT; ++i) {
        std::string dirPath = GET_SAVE_DIR(streamConfig.strSataPath, i);
        CDiskHelper::CreateDir(dirPath.c_str(), AX_FALSE);
        nUsedSpace += CDiskHelper::GetDirSize(dirPath.c_str());
    }

    AX_U64 nNeedSpace = streamConfig.nMaxSpaceSize * TOTAL_STREAM_COUNT;
    AX_U64 nFreeSpace = CDiskHelper::GetFreeSpaceSize(streamConfig.strSataPath.c_str());

    LOG_M_C(BOX, "free space %lld(%lld MB), used space %lld(%lld MB), need space %lld(%lld MB)", nFreeSpace, nFreeSpace >> 20, nUsedSpace,
            nUsedSpace >> 20, nNeedSpace, nNeedSpace >> 20);

    if ((nFreeSpace + nUsedSpace) < nNeedSpace) {
        LOG_M_E(BOX, "no enough space to save, free %lld(%lld MB) + used space %lld(%lld MB), < %lld(%lld MB)", nFreeSpace,
                nFreeSpace >> 20, nUsedSpace, nUsedSpace >> 20, nNeedSpace, nNeedSpace >> 20);
        return AX_FALSE;
    }

    return AX_TRUE;
}

#if defined(__RECORD_VB_TIMESTAMP__)
AX_VOID CBoxBuilder::AllocTimestampBufs(AX_VOID) {
    m_arrTimestampMods.clear();

    AX_U32 nBufNum = 0;
    const char *env = getenv("TIMESTAMP_APP_BUF_NUM");
    if (env) {
        nBufNum = atoi(env);
    }

    if (0 == nBufNum) {
        return;
    }

    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        VDEC_GRP_ATTR_T stAttr;
        m_vdec->GetGrpAttr(i, stAttr);

        for (AX_U32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            if (!stAttr.bChnEnable[j]) {
                continue;
            }

            AX_MOD_INFO_T m = {AX_ID_USER, (AX_S32)i, (AX_S32)j};
            (AX_VOID) CTimestampHelper::AllocTimestampBuf(m, nBufNum);
            m_arrTimestampMods.push_back(m);
        }
    }
}

AX_VOID CBoxBuilder::FreeTimestampBufs(AX_VOID) {
    for (auto &&m : m_arrTimestampMods) {
        CTimestampHelper::FreeTimestampBuf(m);
    }

    m_arrTimestampMods.clear();
}
#endif