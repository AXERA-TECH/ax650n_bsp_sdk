/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiCardMstBuilder.hpp"
#include <stdlib.h>
#include "AXPoolManager.hpp"
#include "AppLogApi.h"
#include "DispatchObserver.hpp"
#include "GlobalDef.h"
#include "PrintHelper.hpp"
#include "StreamerFactory.hpp"
#include "VoObserver.hpp"
#include "make_unique.hpp"

#define AICARD "AICARD_MST"
using namespace std;

#define VDEC_CHN0 0
#define VDEC_CHN1 1
#define VDEC_CHN2 2
#define DISPVO_CHN VDEC_CHN1
#define DETECT_CHN VDEC_CHN2

AX_BOOL CAiCardMstBuilder::Init(AX_VOID) {
    CAiCardMstConfig *pConfig = CAiCardMstConfig::GetInstance();
    if (!pConfig->Init()) {
        printf("Load aicard config file failed.\n");
        return AX_FALSE;
    }

    /* [1]: Load configuration */
    STREAM_CONFIG_T streamConfig = pConfig->GetStreamConfig();
    DISPVO_CONFIG_T dispVoConfig = pConfig->GetDispVoConfig();

    m_nDecodeGrpCount = streamConfig.nDecodeGrps;

    /* [2]: Init system */
    AICARD_APP_SYS_ATTR_T tSysAttr{.nMaxGrp = (AX_U32)m_nDecodeGrpCount};
    if (!m_sys.Init(tSysAttr, "AiCardMst")) {
        return AX_FALSE;
    }

    /* [3]: Init streamers */
    if (!InitStreamer(streamConfig)) {
        return AX_FALSE;
    }

    /* [4]: Init display and observer */
    dispVoConfig.nChnDepth = 2;
    if (!InitDisplay(dispVoConfig)) {
        return AX_FALSE;
    }

    /* [5]: Init detector and observer */
    CDetectResult::GetInstance()->Clear();

    /* [6]: Init dispatchers */
    if (!InitDispatcher(dispVoConfig.strBmpPath)) {
        return AX_FALSE;
    }

    /* [7]: Init data sender. */
    /* Transfer module must be initialized before Decoder to make sure that register option for FileStreamer observer is earlier */
    if (!InitTransHelper()) {
        return AX_FALSE;
    }

    /* [8]: Init video decoder */
    streamConfig.nChnW[DISPVO_CHN] = m_disp->GetVideoLayout()[0].u32Width;
    streamConfig.nChnH[DISPVO_CHN] = m_disp->GetVideoLayout()[0].u32Height;
    if (!InitDecoder(streamConfig)) {
        return AX_FALSE;
    }

    /* [9]: Init AI switch simulator */
    if (!InitAiSwitchSimlator()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::InitStreamer(const STREAM_CONFIG_T &streamConfig) {
    const AX_U32 nCount = streamConfig.v.size();
    m_arrStreamer.resize(nCount);
    for (AX_U32 i = 0; i < nCount; ++i) {
        STREAMER_ATTR_T stAttr;
        stAttr.strPath = streamConfig.v[i];
        stAttr.nMaxWidth = streamConfig.nMaxGrpW;
        stAttr.nMaxHeight = streamConfig.nMaxGrpH;
        stAttr.nCookie = (AX_S32)i;
        stAttr.bLoop = AX_TRUE;
        stAttr.bSyncObs = AX_TRUE;

        m_arrStreamer[i] = CStreamerFactory::GetInstance()->CreateHandler(stAttr.strPath);
        if (!m_arrStreamer[i]) {
            return AX_FALSE;
        }

        if (!m_arrStreamer[i]->Init(stAttr)) {
            return AX_FALSE;
        }

        LOG_M_C(AICARD, "stream %d: %s", i, stAttr.strPath.c_str());
    }

    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::InitDisplay(const DISPVO_CONFIG_T &dispVoConfig) {
    m_disp = make_unique<CVo>();
    if (!m_disp) {
        LOG_MM_E(AICARD, "Create display instance fail");
        return AX_FALSE;
    }

    VO_ATTR_T stAttr;
    stAttr.voDev = dispVoConfig.nDevId;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = (AX_VO_INTF_SYNC_E)dispVoConfig.nHDMI;
    stAttr.nBgClr = 0x000000;
    stAttr.nLayerDepth = dispVoConfig.nLayerDepth;
    stAttr.nTolerance = dispVoConfig.nTolerance;
    stAttr.strResDirPath = dispVoConfig.strResDirPath;
    stAttr.bShowLogo = dispVoConfig.bShowLogo;
    stAttr.bShowNoVideo = dispVoConfig.bShowNoVideo;
    stAttr.arrChns.resize(m_nDecodeGrpCount);
    for (auto &&m : stAttr.arrChns) {
        m.nPriority = 0;
        m.nDepth = dispVoConfig.nChnDepth;
        if (m.nDepth < 1) {
            m.nDepth = 1;
        }
    }
    if (!m_disp->Init(stAttr)) {
        return AX_FALSE;
    }

    m_dispObserver = CObserverMaker::CreateObserver<CVoObserver>(m_disp.get(), DISPVO_CHN);
    if (!m_dispObserver) {
        LOG_MM_E(AICARD, "Create display observer instance fail");
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::InitDispatcher(const string &strFontPath) {
    m_arrDispatcher.resize(m_nDecodeGrpCount);
    m_arrDispatchObserver.resize(m_nDecodeGrpCount);
    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        m_arrDispatcher[i] = make_unique<CDispatcher>();
        if (!m_arrDispatcher[i]) {
            LOG_MM_E(AICARD, "Create dispatcher %d instance fail", i);
            return AX_FALSE;
        } else {
            if (m_dispObserver) {
                m_arrDispatcher[i]->RegObserver(m_dispObserver.get());
            }
        }

        DISPATCH_ATTR_T stAttr;
        stAttr.vdGrp = i;
        stAttr.strBmpFontPath = strFontPath;
        stAttr.nDepth = -1;
        if (!m_arrDispatcher[i]->Init(stAttr)) {
            return AX_FALSE;
        }

        m_arrDispatchObserver[i] = CObserverMaker::CreateObserver<CDispatchObserver>(m_arrDispatcher[i].get(), DISPVO_CHN);
        if (!m_arrDispatchObserver[i]) {
            LOG_MM_E(AICARD, "Create dispatch %d observer instance fail", i);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::InitDecoder(const STREAM_CONFIG_T &streamConfig) {
    LOG_MM_C(AICARD, "+++");

    m_vdec = make_unique<CVideoDecoder>();
    if (!m_vdec) {
        LOG_MM_E(AICARD, "Create vidoe decoder instance failed.");
        return AX_FALSE;
    }

    vector<VDEC_GRP_ATTR_T> arrVdGrps(m_nDecodeGrpCount);
    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        const STREAMER_INFO_T &stInfo = m_arrStreamer[i]->GetStreamInfo();

        VDEC_GRP_ATTR_T tGrpAttr;
        tGrpAttr.bEnable = AX_TRUE;
        tGrpAttr.enCodecType = stInfo.eVideoType;
        tGrpAttr.nMaxWidth = ALIGN_UP(streamConfig.nMaxGrpW, 16);  /* H264 MB 16x16 */
        tGrpAttr.nMaxHeight = ALIGN_UP(streamConfig.nMaxGrpH, 16); /* H264 MB 16x16 */

        if (0 == streamConfig.nDefaultFps) {
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

        if (tGrpAttr.nFps > 0) {
            tGrpAttr.bFramerateCtrl = AX_TRUE;
        }

        tGrpAttr.bPrivatePool = (2 == streamConfig.nUserPool) ? AX_TRUE : AX_FALSE;

        /* FILE: playback + frame or stream mode according configuration */
        tGrpAttr.eDecodeMode = AX_VDEC_DISPLAY_MODE_PLAYBACK;
        if (0 == streamConfig.nInputMode) {
            tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
            tGrpAttr.nMaxStreamBufSize = tGrpAttr.nMaxWidth * tGrpAttr.nMaxHeight * 2;
        } else {
            tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_STREAM;
            tGrpAttr.nMaxStreamBufSize = streamConfig.nMaxStreamBufSize;
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
                    tChnAttr.u32OutputFifoDepth = streamConfig.nChnDepth[j];
                    tChnAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                    tChnAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                    tChnAttr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
                    break;
                case DETECT_CHN:
                    /* pp2 scaler max. 1920x1080 */
                    tGrpAttr.bChnEnable[j] = AX_FALSE;
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
        m_vdec->RegObserver(vdGrp, m_arrDispatchObserver[i].get());

        VDEC_GRP_ATTR_T tGrpAttr;
        m_vdec->GetGrpAttr(vdGrp, tGrpAttr);
        m_disp->SetChnFrameRate(m_disp->GetVideoChn(i), tGrpAttr.nFps);

        for (AX_U32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            if (!tGrpAttr.bChnEnable[j]) {
                continue;
            }

            if (tGrpAttr.bPrivatePool) {
                continue;
            }

            AX_VDEC_CHN_ATTR_T &stChn = tGrpAttr.stChnAttr[j];
            AX_U32 nBlkSize = CVideoDecoder::GetBlkSize(stChn.u32PicWidth, stChn.u32PicHeight, stChn.u32FrameStride, tGrpAttr.enCodecType,
                                                        &stChn.stCompressInfo, stChn.enImgFormat);

            if (0 == streamConfig.nUserPool) {
                CAXPoolManager::GetInstance()->AddBlockToFloorPlan(nBlkSize, stChn.u32OutputFifoDepth);
                LOG_M_N(AICARD, "VDEC vdGrp %d vdChn %d blkSize %d blkCount %d", vdGrp, j, nBlkSize, stChn.u32OutputFifoDepth);
            } else {
                AX_POOL_CONFIG_T stPoolConfig;
                memset(&stPoolConfig, 0, sizeof(stPoolConfig));
                stPoolConfig.MetaSize = 4096;
                stPoolConfig.BlkSize = nBlkSize;
                stPoolConfig.BlkCnt = stChn.u32OutputFifoDepth;
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

                LOG_M_C(AICARD, "pool %2d (blkSize %d blkCount %d) is attached to VDEC vdGrp %d vdChn %d", pool, stPoolConfig.BlkSize,
                        stPoolConfig.BlkCnt, vdGrp, j);
            }
        }
    }

    if (0 == streamConfig.nUserPool) {
        if (!CAXPoolManager::GetInstance()->CreateFloorPlan(512)) {
            return AX_FALSE;
        }
    }

    LOG_MM_C(AICARD, "---");

    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::InitTransHelper() {
    LOG_MM_C(AICARD, "+++");
    m_transHelper = make_unique<CTransferHelper>();
    if (!m_transHelper) {
        LOG_MM_E(AICARD, "Create transfer helper instance failed.");
        return AX_FALSE;
    }

    PCIE_CONFIG_T tPcieConfig = CAiCardMstConfig::GetInstance()->GetPCIECofnig();

    TRANSFER_ATTR_T tAttr;
    tAttr.nSlaveCount = tPcieConfig.nSlaveCount;
    tAttr.nMaxVideoCount = m_nDecodeGrpCount;
    tAttr.nBuffCount = tPcieConfig.nBuffCount;
    tAttr.nBuffSize = tPcieConfig.nBuffSize;
    tAttr.nSendTimeout = tPcieConfig.nSendTimeout;
    tAttr.nRecvTimeout = tPcieConfig.nRecvTimeout;
    tAttr.nTraceData   = tPcieConfig.nTraceData;
    tAttr.nRetryCount  = tPcieConfig.nRetryCount;
    if (!m_transHelper->Init(tAttr)) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        m_arrStreamer[i]->RegObserver(m_transHelper.get());
    }

    LOG_MM_C(AICARD, "---");
    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::InitAiSwitchSimlator() {
    CAiSwitchConfig *pConfig = CAiSwitchConfig::GetInstance();
    if (!pConfig->Init() || 0 == pConfig->GetAttrCount()) {
        LOG_M_E(AICARD, "Parse ai switch config file failed.");
        return AX_FALSE;
    }

    if (pConfig->IsEnabled()) {
        m_aiSwitchSimulator = make_unique<CAiSwitchSimulator>();
        if (!m_aiSwitchSimulator) {
            LOG_MM_E(AICARD, "Create ai switch simulator instance failed.");
            return AX_FALSE;
        }

        m_aiSwitchSimulator->BindTransfer(m_transHelper.get());
    }

    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::DeInit(AX_VOID) {
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

    DESTORY_INSTANCE(m_transHelper);

    for (auto &&m : m_arrDispatcher) {
        DESTORY_INSTANCE(m);
    }
    m_arrDispatcher.clear();

    /* If private pool, destory consumer before producer */
    DESTORY_INSTANCE(m_disp);
    DESTORY_INSTANCE(m_vdec);

#undef DESTORY_INSTANCE

    m_sys.DeInit();
    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::Start(AX_VOID) {
    LOG_MM_W(AICARD, "+++");

    if (!Init()) {
        DeInit();
        return AX_FALSE;
    }

    do {
        if (m_disp) {
            if (!m_disp->Start()) {
                return AX_FALSE;
            }
        } else {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> DISP module is disabled <<<<<<<<<<<<<<<<<<<<<");
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
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> VDEC module is disabled <<<<<<<<<<<<<<<<<<<<<");
            return AX_FALSE;
        }

        if (m_transHelper) {
            if (!m_transHelper->Start()) {
                return AX_FALSE;
            }
        } else {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> TRANSFER module is disabled <<<<<<<<<<<<<<<<<<<<<");
            return AX_FALSE;
        }

        for (auto &&m : m_arrStreamer) {
            if (m) {
                thread t([](IStreamHandler *p) { p->Start(); }, m.get());
                t.detach();
            }
        }

        if (m_aiSwitchSimulator) {
            if (!m_aiSwitchSimulator->Start()) {
                return AX_FALSE;
            }
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> AI SWITCH SIMULATOR module is disabled <<<<<<<<<<<<<<<<<<<<<");
        }

        CPrintHelper::GetInstance()->Start();

        return AX_TRUE;

    } while (0);

    Stop();

    LOG_MM_W(AICARD, "---");

    return AX_FALSE;
}

AX_BOOL CAiCardMstBuilder::Stop(AX_VOID) {
    CPrintHelper::GetInstance()->Stop();

    for (auto &&m : m_arrStreamer) {
        m->UnRegObserver(m_vdec.get());
    }

    vector<thread> v;
    v.reserve(m_arrStreamer.size());
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

    if (m_vdec) {
        m_vdec->UnRegAllObservers();
    }

    for (auto &&m : m_arrDispatcher) {
        m->Clear();
    }

    if (m_transHelper) {
        if (!m_transHelper->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> TRANSFER module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> TRANSFER module stop successfully <<<<<<<<<<<<<<<<<<<<<");
        }
    }

    if (m_disp) {
        if (!m_disp->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> DISPLAY module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> DISPLAY module stop successfully <<<<<<<<<<<<<<<<<<<<<");
        }
    }

    if (m_aiSwitchSimulator) {
        if (!m_aiSwitchSimulator->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> AI SWITCH SIMULATOR module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> AI SWITCH SIMULATOR module stop successfully <<<<<<<<<<<<<<<<<<<<<");
        }
    }

    for (auto &&m : m_arrDispatcher) {
        if (!m->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> DISPATCH module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> [%d] DISPATCH module stop successfully <<<<<<<<<<<<<<<<<<<<<", m->GetGrp());
        }
    }

    if (m_vdec) {
        if (!m_vdec->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> VDEC module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> VDEC module stop successfully <<<<<<<<<<<<<<<<<<<<<");
        }
    }

    DeInit();
    return AX_TRUE;
}

AX_BOOL CAiCardMstBuilder::QueryStreamsAllEof(AX_VOID) {
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