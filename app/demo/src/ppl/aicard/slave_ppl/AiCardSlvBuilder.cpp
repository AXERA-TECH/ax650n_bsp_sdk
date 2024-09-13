/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiCardSlvBuilder.hpp"
#include <stdlib.h>
#include "AXPoolManager.hpp"
#include "AppLogApi.h"
#include "GlobalDef.h"
#include "make_unique.hpp"
#include "ElapsedTimer.hpp"
#include "PrintHelper.hpp"
#include "VdecCtrlObserver.hpp"
#include "CommonDef.h"

#define AICARD "AICARD_SLV"
using namespace std;


AX_BOOL CAiCardSlvBuilder::Init(AX_VOID) {
    LOG_MM_C(AICARD, "+++");

    CAiCardSlvConfig *pConfig = CAiCardSlvConfig::GetInstance();
    if (!pConfig->Init()) {
        LOG_MM_E(AICARD, "Load aicard config file failed.");
        return AX_FALSE;
    }

    /* [1]: Load configuration */
    VDEC_CONFIG_T tVdecConfig = pConfig->GetVdecConfig();
    DETECT_CONFIG_T tDetectConfig = pConfig->GetDetectConfig();
    // PCIE_CONFIG_T tPcieConfig = CAiCardSlvConfig::GetInstance()->GetPCIECofnig();
    m_nDecodeGrpCount = tVdecConfig.nDecodeGrps;
    m_bSimulateDetRets = tDetectConfig.bEnableSimulator;

    /* [2]: Init system */
    AICARD_SLV_SYS_ATTR_T tSysAttr{.nMaxGrp = (AX_U32)m_nDecodeGrpCount};
    if (!m_sys.Init(tSysAttr, "AiCardSlv")) {
        return AX_FALSE;
    }

    if (!m_bSimulateDetRets) {
        /* [3]: Init detector */
        if (!InitDetector(tDetectConfig)) {
            return AX_FALSE;
        }
    }

    /* [4]: Init video decoder */
    tVdecConfig.arrChnW[DETECT_CHN] = tDetectConfig.nW;
    tVdecConfig.arrChnH[DETECT_CHN] = tDetectConfig.nH;
    if (!InitDecoder(tVdecConfig)) {
        return AX_FALSE;
    }

    /* [4]: Init master/slave transfer module */
    if (!InitTransHelper()) {
        return AX_FALSE;
    }

    LOG_MM_C(AICARD, "---");
    return AX_TRUE;
}

AX_BOOL CAiCardSlvBuilder::InitDetector(const DETECT_CONFIG_T &tDetectConfig) {
    LOG_MM_C(AICARD, "+++");

    m_detector = make_unique<CDetector>();
    if (!m_detector) {
        LOG_M_E(AICARD, "%s: create detector instance fail", __func__);
        return AX_FALSE;
    }

    DETECTOR_ATTR_T tDetectAttr;
    tDetectAttr.nGrpCount = m_nDecodeGrpCount;
    tDetectAttr.nSkipRate = tDetectConfig.nSkipRate;
    tDetectAttr.nW = tDetectConfig.nW;
    tDetectAttr.nH = tDetectConfig.nH;
    tDetectAttr.nDepth = tDetectConfig.nDepth * m_nDecodeGrpCount;
    strcpy(tDetectAttr.szModelPath, tDetectConfig.strModelPath.c_str());
    tDetectAttr.nChannelNum = AX_MIN(tDetectConfig.nChannelNum, DETECTOR_MAX_CHN_NUM);
    for (AX_U32 i = 0; i < tDetectAttr.nChannelNum; ++i) {
        tDetectAttr.tChnAttr[i].nPPL = tDetectConfig.tChnParam[i].nPPL;
        tDetectAttr.tChnAttr[i].nVNPU = tDetectConfig.tChnParam[i].nVNPU;
        tDetectAttr.tChnAttr[i].bTrackEnable = tDetectConfig.tChnParam[i].bTrackEnable;
    }

    if (!m_detector->Init(tDetectAttr, tDetectConfig.bEnableSimulator)) {
        return AX_FALSE;
    }

    m_detectorObserver = CObserverMaker::CreateObserver<CDetectObserver>(m_detector.get(), DETECT_CHN);
    if (!m_detectorObserver) {
        LOG_M_E(AICARD, "%s: create detect observer fail", __func__);
        return AX_FALSE;
    }

    LOG_MM_C(AICARD, "---");

    return AX_TRUE;
}

AX_BOOL CAiCardSlvBuilder::InitTransHelper() {
    LOG_MM_C(AICARD, "+++");

    m_transHelper = make_unique<CTransferHelper>();
    if (!m_transHelper) {
        LOG_MM_E(AICARD, "Create transfer helper instance failed.");
        return AX_FALSE;
    }

    PCIE_CONFIG_T tPcieConfig = CAiCardSlvConfig::GetInstance()->GetPCIECofnig();

    TRANSFER_ATTR_T tAttr;
    tAttr.nMaxVideoCount = m_nDecodeGrpCount;
    tAttr.nBuffCount = tPcieConfig.nBuffCount;
    tAttr.nBuffSize = tPcieConfig.nBuffSize;
    tAttr.nSendTimeout = tPcieConfig.nSendTimeout;
    tAttr.nRecvTimeout = tPcieConfig.nRecvTimeout;
    tAttr.bEnableSimulateDetRets = m_bSimulateDetRets;
    tAttr.nTraceData  = tPcieConfig.nTraceData;
    tAttr.nRetryCount = tPcieConfig.nRetryCount;
    if (!m_transHelper->Init(tAttr)) {
        return AX_FALSE;
    } else {
        m_transHelper->RegStreamObserver(m_vdec.get());
        m_transHelper->RegCommandObserver(m_vdecCtrlObserver.get());
        if (!m_bSimulateDetRets) {
            m_transHelper->RegCommandObserver(m_detectorObserver.get());
        }
    }

    LOG_MM_C(AICARD, "---");

    return AX_TRUE;
}

AX_BOOL CAiCardSlvBuilder::InitDecoder(const VDEC_CONFIG_T &tVdecConfig) {
    LOG_MM_C(AICARD, "+++");

    m_vdec = make_unique<CVideoDecoder>();
    if (!m_vdec) {
        LOG_MM_E(AICARD, "Create vidoe decoder instance failed.");
        return AX_FALSE;
    }

    vector<VDEC_GRP_ATTR_T> arrVdGrps(m_nDecodeGrpCount);
    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        VDEC_GRP_ATTR_T tGrpAttr;
        tGrpAttr.bEnable = AX_TRUE;
        tGrpAttr.enCodecType = tVdecConfig.eVideoType;
        tGrpAttr.nMaxWidth = ALIGN_UP(tVdecConfig.nMaxGrpW, 16);  /* H264 MB 16x16 */
        tGrpAttr.nMaxHeight = ALIGN_UP(tVdecConfig.nMaxGrpH, 16); /* H264 MB 16x16 */

        /* use configured fps for file streamer */
        tGrpAttr.nFps = tVdecConfig.nDefaultFps;
        if (1 == i) {
            tGrpAttr.nFps = 30;
        }

        if (tGrpAttr.nFps > 0) {
            tGrpAttr.bFramerateCtrl = AX_TRUE;
        }

        tGrpAttr.bPrivatePool = (2 == tVdecConfig.nUserPool) ? AX_TRUE : AX_FALSE;

        /* FILE: playback + frame or stream mode according configuration */
        tGrpAttr.eDecodeMode = AX_VDEC_DISPLAY_MODE_PLAYBACK;
        if (0 == tVdecConfig.nInputMode) {
            tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
            tGrpAttr.nMaxStreamBufSize = tGrpAttr.nMaxWidth * tGrpAttr.nMaxHeight * 2;
        } else {
            tGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_STREAM;
            tGrpAttr.nMaxStreamBufSize = tVdecConfig.nMaxStreamBufSize;
        }

        for (AX_U32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            AX_VDEC_CHN_ATTR_T &tChnAttr = tGrpAttr.stChnAttr[j];
            switch (j) {
                case VDEC_CHN0:
                    /* pp0 disable, because scaler is not support */
                    tGrpAttr.bChnEnable[j] = AX_FALSE;
                    break;
                case DETECT_CHN:
                    /* pp1 scaler max. 4096x2160 */
                    tGrpAttr.bChnEnable[j] = AX_TRUE;
                    tChnAttr.u32PicWidth = tVdecConfig.arrChnW[j];
                    tChnAttr.u32PicHeight = tVdecConfig.arrChnH[j];
                    tChnAttr.u32FrameStride = ALIGN_UP(tChnAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                    tChnAttr.u32OutputFifoDepth = tVdecConfig.nChnDepth[j];
                    tChnAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                    tChnAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                    tChnAttr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
                    break;
                default:
                    tGrpAttr.bChnEnable[j] = AX_FALSE;
                    break;
            }
        }

        arrVdGrps[i] = move(tGrpAttr);
    }

    if (!m_vdec->Init(arrVdGrps)) {
        return AX_FALSE;
    } else {
        m_vdecCtrlObserver = CObserverMaker::CreateObserver<CVdecCtrlObserver>(m_vdec.get(), tVdecConfig.bEnableReset);
        if (!m_vdecCtrlObserver) {
            LOG_M_E(AICARD, "%s: create vdec ctrl command observer fail", __func__);
            return AX_FALSE;
        }
    }

    for (AX_U32 i = 0; i < m_nDecodeGrpCount; ++i) {
        AX_VDEC_GRP vdGrp = (AX_VDEC_GRP)i;
        m_vdec->RegObserver(vdGrp, m_detectorObserver.get());

        VDEC_GRP_ATTR_T tGrpAttr;
        m_vdec->GetGrpAttr(vdGrp, tGrpAttr);

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

            if (0 == tVdecConfig.nUserPool) {
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

                ((CVdecCtrlObserver *)(m_vdecCtrlObserver.get()))->SetPool(vdGrp, (AX_VDEC_CHN) j, pool);

                LOG_M_C(AICARD, "pool %2d (blkSize %d blkCount %d) is attached to VDEC vdGrp %d vdChn %d", pool, stPoolConfig.BlkSize,
                        stPoolConfig.BlkCnt, vdGrp, j);
            }
        }
    }

    if (0 == tVdecConfig.nUserPool) {
        if (!CAXPoolManager::GetInstance()->CreateFloorPlan(512)) {
            return AX_FALSE;
        }
    }

    LOG_MM_C(AICARD, "---");

    return AX_TRUE;
}

AX_BOOL CAiCardSlvBuilder::DeInit(AX_VOID) {
    /* destory instances */
#define DESTORY_INSTANCE(p) \
    do {                    \
        if (p) {            \
            p->DeInit();    \
            p = nullptr;    \
        }                   \
    } while (0)

    DESTORY_INSTANCE(m_transHelper);

    /* If private pool, destory consumer before producer */
    DESTORY_INSTANCE(m_detector);
    DESTORY_INSTANCE(m_vdec);

#undef DESTORY_INSTANCE

    m_sys.DeInit();
    return AX_TRUE;
}

AX_BOOL CAiCardSlvBuilder::Start(AX_VOID) {
    if (!Init()) {
        DeInit();
        return AX_FALSE;
    }

    do {
        if (m_detector) {
            if (!m_detector->Start()) {
                return AX_FALSE;
            }
        } else {
            if (m_bSimulateDetRets) {
                LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> DETECTOR module is disabled and results would be simulated <<<<<<<<<<<<<<<<<<<<<");
            } else {
                LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> DETECTOR module is disabled <<<<<<<<<<<<<<<<<<<<<");
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

        CPrintHelper::GetInstance()->Start();

        return AX_TRUE;

    } while (0);

    Stop();

    return AX_FALSE;
}

AX_BOOL CAiCardSlvBuilder::Stop(AX_VOID) {
    CPrintHelper::GetInstance()->Stop();

    if (m_transHelper) {
        if (!m_transHelper->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> TRANSFER module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> TRANSFER module stop successfully <<<<<<<<<<<<<<<<<<<<<");
        }
    }

    if (m_detector) {
        m_detector->Clear();
        if (!m_detector->Stop()) {
            LOG_MM_E(AICARD, ">>>>>>>>>>>>>>>> DETECTOR module stop failed <<<<<<<<<<<<<<<<<<<<<");
        } else {
            LOG_MM_C(AICARD, ">>>>>>>>>>>>>>>> DETECTOR module stop successfully <<<<<<<<<<<<<<<<<<<<<");
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
