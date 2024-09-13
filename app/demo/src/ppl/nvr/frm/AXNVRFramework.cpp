/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "AXNVRFramework.h"
#include "TestSuiteConfigParser.h"
#include "ax_engine_api.h"
#include "ax_engine_type.h"

#define TAG "FRM"

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

// #define SKEL_MODEL_PATH ("/opt/etc/skelModels/1024x576/part")

AX_BOOL CAXNVRFramework::Init(AX_U32 nDetectType, AX_U32 loglevel) {

    AX_BOOL bRet = AX_FALSE;
    do {
        AX_S32 nRet = 0;
        // - log level
        AX_U32 nLogLevel = loglevel;
        // - log
        APP_LOG_ATTR_T stLog;
        memset(&stLog, 0, sizeof(stLog));
        stLog.nTarget = APP_LOG_TARGET_STDOUT;
        stLog.nLv = nLogLevel;

        AX_CHAR* env = getenv("APP_LOG_TARGET");
        if (env) {
            stLog.nTarget = atoi(env);
        }

        strcpy(stLog.szAppName, "nvr");
        nRet = AX_APP_Log_Init(&stLog);
        if (0 != nRet) {
            printf("AX_APP_Log_Init failed, ret = 0x%x\n", nRet);
        } else {
            printf("[SUCCESS]AX_APP_Log_Init\n");

            if (stLog.nTarget & APP_LOG_TARGET_SYSLOG) {
                AX_APP_Log_SetSysModuleInited(AX_TRUE);
            }
        }

        // read config
        m_stRPratrolCfg = CNVRConfigParser::GetInstance()->GetRoundPatrolConfig();
        m_stDetectCfg = CNVRConfigParser::GetInstance()->GetDetectConfig();
        m_stPrimaryCfg = CNVRConfigParser::GetInstance()->GetPrimaryDispConfig();
        m_stSecondaryCfg = CNVRConfigParser::GetInstance()->GetSecondaryDispConfig();
        m_stRecordCfg = CNVRConfigParser::GetInstance()->GetRecordConfig();
        m_stDeviceCfg = CNVRConfigParser::GetInstance()->GetDeviceConfig();

        if (0 == nDetectType) {
            /* disable detect by user input arg(-t 0). */
            m_stDetectCfg.bEnable = AX_FALSE;
        }

        // - sys
        nRet = AX_SYS_Init();
        if (0 != nRet) {
            LOG_M_E(TAG, "AX_SYS_Init failed, ret = 0x%x", nRet);
            break;
        }

        // - vo
        nRet = AX_VO_Init();
        if (0 != nRet) {
            LOG_M_E(TAG, "AX_VO_Init failed, ret = 0x%x", nRet);
            break;
        }

        m_pPrimaryDispCtrl = this->initPrimaryDispCtrl();
        if (!m_pPrimaryDispCtrl) {
            LOG_M_E(TAG, "init primary screen failed");
            break;
        }

        m_pSecondaryDispCtrl = new (std::nothrow) CAXNVRDisplayCtrl;
        if (!m_pSecondaryDispCtrl) {
            LOG_M_E(TAG, "create secondary screen failed.");
            break;
        }

        if (!this->initVdec()) {
            LOG_M_E(TAG, "init and start VDEC failed.");
            break;
        }

        if (!this->initIvps()) {
            break;
        }

        if (!this->initVenc()) {
            break;
        }

        // detect and detect observer
        if (!this->initDetect(nDetectType)) {
            break;
        }

        // if (nDetectType) {
        //     CRegionTask::GetInstance()->Start();
        // }

        // record
        m_pDataStreamRecord = this->initDataStreamRecord();
        if (!m_pDataStreamRecord) {
            LOG_M_E(TAG, "init record failed.");
            break;
        }

        // preview
        m_pPreviewCtrl = this->initPreviewCtrl(m_pPrimaryDispCtrl,
                                            m_pSecondaryDispCtrl,
                                            m_pDataStreamRecord,
                                            nDetectType,
                                            m_pDetect,
                                            m_pDetectObs);
        if (!m_pPreviewCtrl) {
            LOG_M_E(TAG, "init preview ctrl failed.");
            break;
        }


        m_pDataStreamPlayback = this->initDataStreamPlayback();
        if (!m_pDataStreamPlayback) {
            LOG_M_E(TAG, "init playback failed.");
            break;
        }

        // playback
        m_pPlaybakCtrl = this->initPlaybakCtrl(m_pPrimaryDispCtrl, m_pDataStreamPlayback);
        if (!m_pPlaybakCtrl) {
            LOG_M_E(TAG, "init playback ctrl failed.");
            break;
        }

        AX_U32 nRemoteDeviceCnt = 0;
        std::vector<AX_NVR_DEV_INFO_T> mapRemoteDevice = CRemoteDeviceParser::GetInstance()->GetRemoteDeviceMap(&nRemoteDeviceCnt, m_stDeviceCfg.strPath);
        AX_U32 index = 0;
        for (auto &info : mapRemoteDevice) {
            if (index >= nRemoteDeviceCnt) break;
            if (!m_pPreviewCtrl->InsertDevice(info)) {
                LOG_M_W(TAG, "Start channel %d failed.", info.nChannelId);
            }
            index ++;
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_VOID CAXNVRFramework::DeInit() {
    // if (nDetectType) {
    //     CRegionTask::GetInstance()->Stop();
    // }

    if (m_pDetect) {
        m_pDetect->Stop();
    }

    if (m_pPlaybakCtrl) {
        m_pPlaybakCtrl->DeInit();
        DEL_PTR(m_pPlaybakCtrl)
    }

    if (m_pPreviewCtrl) {
        m_pPreviewCtrl->DeInit();
        DEL_PTR(m_pPreviewCtrl)
    }

    if (m_pDetect) {
        m_pDetect->DeInit();
        DEL_PTR(m_pDetectObs)
    }

    AX_ENGINE_Deinit();

    if (m_pDataStreamRecord) {
        m_pDataStreamRecord->StopAll();
        m_pDataStreamRecord->DeInit();
        DEL_PTR(m_pDataStreamRecord)
    }

    if (m_pDataStreamPlayback) {
        m_pDataStreamPlayback->DeInit();
        DEL_PTR(m_pDataStreamPlayback)
    }

    if (m_pPrimaryDispCtrl) {
        m_pPrimaryDispCtrl->DeInit();
        DEL_PTR(m_pPrimaryDispCtrl)
    }

    if (m_pSecondaryDispCtrl) {
        this->DeInitSecondaryDispCtrl();
        // if (m_bSecondaryInit) {
        //     m_pSecondaryDispCtrl->DeInit();
        // }
        DEL_PTR(m_pSecondaryDispCtrl)
    }

    CDecodeTask::GetInstance()->Stop();

    AX_VO_Deinit();
    AX_IVPS_Deinit();
    AX_VDEC_Deinit();
    // AX_POOL_Exit();
    AX_SYS_Deinit();

    AX_CHAR* env = getenv("APP_LOG_TARGET");
    if (env) {
        if (atoi(env) & APP_LOG_TARGET_SYSLOG) {
            AX_APP_Log_SetSysModuleInited(AX_FALSE);
        }
    }
    AX_APP_Log_DeInit();
}

AX_BOOL CAXNVRFramework::InitSecondaryDispCtrl(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++", __func__, __LINE__);

    do {
        if (nullptr == m_pSecondaryDispCtrl) {
            LOG_M_E(TAG, "Invalid secondary screen handle");
            break;
        }

        if (m_stSecondaryCfg.nDevId == -1) {
            LOG_M_E(TAG, "Secondary screen not configured, please check the configuration file");
            break;
        }

        if (m_bSecondaryInit) {
            LOG_M_W(TAG, "[ERROR]Secondary screen has been initialized");
            bRet = AX_TRUE;
            break;
        }

        AX_NVR_VO_INFO_T stSecondaryAttr;
        stSecondaryAttr.enDispDevType = AX_NVR_DISPDEV_TYPE::SECOND;
        stSecondaryAttr.stVoAttr.dev = m_stSecondaryCfg.nDevId;
        stSecondaryAttr.stVoAttr.uiLayer = 1;

        if (!m_stDetectCfg.bEnable) {
            /* if no need to draw rects, change to AX_VO_LAYER_OUT_TO_LINK to not push into layer fifo to speed up VB */
            stSecondaryAttr.stVoAttr.bLinkVo2Disp = AX_TRUE;
        } else {
            stSecondaryAttr.stVoAttr.bLinkVo2Disp = m_stSecondaryCfg.bLink;
        }
        stSecondaryAttr.stVoAttr.s32FBIndex[0] = m_stSecondaryCfg.nFBQt;
        stSecondaryAttr.stVoAttr.s32FBIndex[1] = m_stSecondaryCfg.nFBRect;
        stSecondaryAttr.stVoAttr.enIntfType = AX_VO_INTF_HDMI;
        stSecondaryAttr.stVoAttr.enIntfSync = (AX_VO_INTF_SYNC_E)m_stSecondaryCfg.nHDMI;
        stSecondaryAttr.stVoAttr.nLayerDepth = m_stSecondaryCfg.nLayerDepth;
        stSecondaryAttr.stVoAttr.nBgClr = 0x303030;
        stSecondaryAttr.stVoAttr.nDetectW = m_stDetectCfg.nW;
        stSecondaryAttr.stVoAttr.nDetectH = m_stDetectCfg.nH;
        stSecondaryAttr.stVoAttr.enVoMode = (m_stSecondaryCfg.nOnlineMode ? AX_VO_MODE_ONLINE : AX_VO_MODE_OFFLINE);
        if (!m_pSecondaryDispCtrl->Init(stSecondaryAttr)) {
            LOG_M_E(TAG, "Secondary screen init failed");
            break;
        }

        m_bSecondaryInit = AX_TRUE;
        bRet = AX_TRUE;

    } while (0);

    LOG_M_D(TAG, "[%s][%d] ---", __func__, __LINE__);
    return bRet;
}

AX_VOID CAXNVRFramework::DeInitSecondaryDispCtrl(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++", __func__, __LINE__);
    if (m_pSecondaryDispCtrl && m_stSecondaryCfg.nDevId != -1) {
        if (!m_bSecondaryInit) {
            LOG_M_W(TAG, "Secondary screen has not initialized");
        } else {
            m_pSecondaryDispCtrl->DeInit();
            m_bSecondaryInit = AX_FALSE;
        }
    }
    LOG_M_D(TAG, "[%s][%d] ---", __func__, __LINE__);
}

AX_S32 CAXNVRFramework::MapToDisplay(AX_NVR_VIEW_TYPE enViewType, AX_NVR_DEV_ID nDevID) {
    AX_S32 nChn = -1;
    if (enViewType == AX_NVR_VIEW_TYPE::PREVIEW) {
        const CVO *pVo = m_pPrimaryDispCtrl->GetVo();
        if (pVo) {
            VO_ATTR_T attr = pVo->GetAttr();
            int nCount = attr.stChnInfo.nCount;
            if (nCount != 0) {
                nChn = nDevID%nCount;
            }
        }
    }

    return nChn;
}

AX_VOID CAXNVRFramework::GetResolution(AX_NVR_DEV_ID nDevID, AX_NVR_VIEW_TYPE enViewType, AX_U32 &nWidth, AX_U32 &nHeight) {
    if (enViewType == AX_NVR_VIEW_TYPE::PREVIEW) {
        m_pPreviewCtrl->GetResolution(nDevID, nWidth, nHeight);
    } else if (enViewType == AX_NVR_VIEW_TYPE::PLAYBACK) {
        m_pPlaybakCtrl->GetResolution(nDevID, nWidth, nHeight);
    }
}

AX_BOOL CAXNVRFramework::StartRoundPatrol(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;

    // m_pSecondaryDispCtrl->SetStartIndex(m_stRPratrolCfg.u32LayoutSplitCntIndex);
    m_pSecondaryDispCtrl->SetStartIndex(0);
    if (!m_pSecondaryDispCtrl->UpdateViewRound(AX_NVR_VIEW_CHANGE_TYPE::SHOW)) {
        return AX_FALSE;
    }

    vector<AX_NVR_DEV_ID> vecDevID;
    for (int i = m_nStartDevId; i < (AX_S32)m_pSecondaryDispCtrl->GetCurrentLayoutCnt(); ++i) {
        vecDevID.emplace_back(i);
    }

    bRet = m_pPreviewCtrl->StartRoundPatrol(vecDevID);

    return bRet;
}

AX_VOID CAXNVRFramework::StopRoundPatrol(AX_VOID) {
    vector<AX_NVR_DEV_ID> vecDevID;
    for (int i = m_nStartDevId; i < (AX_S32)m_pSecondaryDispCtrl->GetCurrentLayoutCnt(); ++i) {
        vecDevID.emplace_back(i);
    }

    m_pPreviewCtrl->StopRoundPatrol(vecDevID);
    m_pSecondaryDispCtrl->UpdateViewRound(AX_NVR_VIEW_CHANGE_TYPE::HIDE);
}

AX_BOOL CAXNVRFramework::UpdateRoundPatrol(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;

    vector<AX_NVR_DEV_ID> vecLastDevID;
    vector<AX_NVR_DEV_ID> vecNextDevID;
    vector<AX_NVR_DEV_ID> vecUpdDevID;
    vector<AX_NVR_DEV_ID> vecStartDevID;
    vector<AX_NVR_DEV_ID> vecStopDevID;
    if (m_stRPratrolCfg.enType == AX_NVR_RPATROL_TYPE::SPLIT) {
        for (int i = m_nStartDevId; i < (AX_S32)m_pSecondaryDispCtrl->GetCurrentLayoutCnt(); ++i) {
            vecLastDevID.emplace_back(i);
        }

        if (!m_pSecondaryDispCtrl->UpdateViewRound(AX_NVR_VIEW_CHANGE_TYPE::UPDATE)) {
            return AX_FALSE;
        }

        for (int i = m_nStartDevId; i < (AX_S32)m_pSecondaryDispCtrl->GetCurrentLayoutCnt(); ++i) {
            vecNextDevID.emplace_back(i);
        }

        for (auto& nLastDev : vecLastDevID) {
            AX_BOOL bExist = AX_FALSE;
            for (auto& nNextDev : vecNextDevID) {
                if (nLastDev == nNextDev) {
                    bExist = AX_TRUE;
                    break;
                }
            }

            if (bExist) {
                vecUpdDevID.emplace_back(nLastDev);
            } else {
                vecStopDevID.emplace_back(nLastDev);
            }
        }

        for (auto& nNextDev : vecNextDevID) {
            if (nNextDev >= MAX_DEVICE_ROUNDPATROL_COUNT) {
                continue;
            }

            AX_BOOL bExist = AX_FALSE;
            for (auto& nLastDev : vecLastDevID) {
                if (nNextDev == nLastDev) {
                    bExist = AX_TRUE;
                    break;
                }
            }

            if (!bExist) {
                vecStartDevID.emplace_back(nNextDev);
            }
        }

        /* Stop no more exist channels */
        bRet = m_pPreviewCtrl->StopRoundPatrol(vecStopDevID);
        if (!bRet) {
            LOG_MM_E(TAG, "Stop round patrol failed.");
        }

        /* Update exist channel resolution according to vo display */
        bRet = m_pPreviewCtrl->UpdateRoundPatrolPreview(vecUpdDevID);
        if (!bRet) {
            LOG_MM_E(TAG, "Update round patrol failed.");
        }

        /* Start new channels, and notice that VDEC channels would be start keeping output resolution as input */
        bRet = m_pPreviewCtrl->StartRoundPatrol(vecStartDevID);
        if (!bRet) {
            LOG_MM_E(TAG, "Start round patrol failed.");
        }

        /* Update new started channels whose VDEC channel output resolution should be calculated according to vo display */
        bRet = m_pPreviewCtrl->UpdateRoundPatrolPreview(vecStartDevID);
        if (!bRet) {
            LOG_MM_E(TAG, "Update round patrol failed.");
        }
    }

    return bRet;
}

AX_U32 CAXNVRFramework::GetCurrentSplitCnt(AX_VOID) {
    return m_pSecondaryDispCtrl->GetCurrentLayoutCnt();
}

vector<AX_U32> CAXNVRFramework::GetDeviceFileList(AX_VOID) {
    vector<AX_U32> vecDevFileList(MAX_DEVICE_COUNT);
    int device = 1;
    for (auto &dev_file: vecDevFileList) {
        dev_file = device;
        device++;
    }
    return vecDevFileList;
}

CAXNVRDisplayCtrl *CAXNVRFramework::initPrimaryDispCtrl(AX_VOID) {
    AX_NVR_VO_INFO_T stPrimaryAttr;
    stPrimaryAttr.enDispDevType = AX_NVR_DISPDEV_TYPE::PRIMARY;
    stPrimaryAttr.stVoAttr.dev = m_stPrimaryCfg.nDevId;
    stPrimaryAttr.stVoAttr.uiLayer = 0;

    if (!m_stDetectCfg.bEnable) {
        /* if no need to draw rects, change to AX_VO_LAYER_OUT_TO_LINK to not push into layer fifo to speed up VB */
        stPrimaryAttr.stVoAttr.bLinkVo2Disp = AX_TRUE;
    } else {
        stPrimaryAttr.stVoAttr.bLinkVo2Disp = m_stPrimaryCfg.bLink;
    }

    stPrimaryAttr.stVoAttr.s32FBIndex[0] = m_stPrimaryCfg.nFBQt;
    stPrimaryAttr.stVoAttr.s32FBIndex[1] = m_stPrimaryCfg.nFBRect;
    stPrimaryAttr.stVoAttr.enIntfType = AX_VO_INTF_HDMI;
    stPrimaryAttr.stVoAttr.enIntfSync = (AX_VO_INTF_SYNC_E)m_stPrimaryCfg.nHDMI;
    stPrimaryAttr.stVoAttr.nLayerDepth = m_stPrimaryCfg.nLayerDepth;
    stPrimaryAttr.stVoAttr.nBgClr = 0x202020; // ((44 << 2) << 20) |((44 << 2) << 10) |(44 << 2);
    stPrimaryAttr.stVoAttr.nDetectW = m_stDetectCfg.nW;
    stPrimaryAttr.stVoAttr.nDetectH = m_stDetectCfg.nH;
    stPrimaryAttr.stVoAttr.enVoMode = (m_stPrimaryCfg.nOnlineMode ? AX_VO_MODE_ONLINE : AX_VO_MODE_OFFLINE);
    CAXNVRDisplayCtrl *pPrimaryDispCtrl = nullptr;
    pPrimaryDispCtrl = new (std::nothrow) CAXNVRDisplayCtrl;
    if (pPrimaryDispCtrl) {
        if (!pPrimaryDispCtrl->Init(stPrimaryAttr)) {
            DEL_PTR(pPrimaryDispCtrl)
            return pPrimaryDispCtrl;
        }
    }
    return pPrimaryDispCtrl;
}

AX_BOOL CAXNVRFramework::initVdec(AX_VOID) {
    // - vdec
    AX_VDEC_MOD_ATTR_T stModAttr;
    memset(&stModAttr, 0, sizeof(stModAttr));
    stModAttr.u32MaxGroupCount = MAX_DEVICE_VDEC_COUNT;
    stModAttr.enDecModule = AX_ENABLE_ONLY_VDEC;
    AX_S32 nRet = AX_VDEC_Init(&stModAttr);
    if (0 != nRet) {
        LOG_M_E(TAG, "AX_VDEC_Init failed, ret = 0x%x", nRet);
        return AX_FALSE;
    }
    // -- decode thread(select)
    return CDecodeTask::GetInstance()->Start();
}

AX_BOOL CAXNVRFramework::initIvps(AX_VOID) {
    // - ivps
    AX_S32 nRet = AX_IVPS_Init();
    if (0 != nRet) {
        LOG_M_E(TAG, "AX_IVPS_Init fail, ret = 0x%x", nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAXNVRFramework::initVenc(AX_VOID) {
    // - venc
    AX_VENC_MOD_ATTR_T stModAttr;
    memset(&stModAttr, 0, sizeof(stModAttr));
    stModAttr.enVencType = AX_VENC_VIDEO_ENCODER;
    stModAttr.stModThdAttr.bExplicitSched = AX_FALSE;
    stModAttr.stModThdAttr.u32TotalThreadNum = 1;
    AX_S32 nRet = AX_VENC_Init(&stModAttr);
    if (0 != nRet) {
        LOG_M_E(TAG, "AX_VENC_Init fail, ret = 0x%x", nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

CDataStreamRecord *CAXNVRFramework::initDataStreamRecord(AX_VOID) {
    // record
    CDataStreamRecord *pRecord = new(std::nothrow) CDataStreamRecord;
    if (pRecord) {
        AXDS_RECORD_INIT_ATTR_T stAttr;
        sprintf(stAttr.szParentDir[0], "%s", m_stRecordCfg.strPath.c_str());
        stAttr.uMaxDevCnt = MAX_DEVICE_COUNT;
        stAttr.uStreamCnt = MAX_DEVICE_STREAM_COUNT;
        stAttr.uMaxDevSpace = m_stRecordCfg.uMaxDevSpace;
        stAttr.uMaxFilePeriod = m_stRecordCfg.uMaxFilePeriod;
        stAttr.bGenIFOnClose = AX_TRUE;
        if (!pRecord->Init(stAttr)) {
            DEL_PTR(pRecord)
            return pRecord;
        }
    }
    return pRecord;
}

CDataStreamPlay *CAXNVRFramework::initDataStreamPlayback(AX_VOID) {
    // record
    CDataStreamPlay *pPlaybck = new(std::nothrow) CDataStreamPlay;
    if (pPlaybck) {
        AXDS_PLAY_INIT_ATTR_T stAttr;
        stAttr.strParentDir = m_stRecordCfg.strPath;
        stAttr.uMaxDevCnt = MAX_DEVICE_COUNT;
        stAttr.uStreamCnt = MAX_DEVICE_STREAM_COUNT;
        stAttr.bOnlyIFrameOnReverse = m_stRecordCfg.bOnlyIFrameOnReverse;

        /* In case test suite enabled, playback streams exist under directory configured in test_suite.json */
        AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
        if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
            AX_NVR_TS_CONFIG_T tTestSuiteConfig = CTestSuiteConfigParser::GetInstance()->GetConfig();
            for (auto& m: tTestSuiteConfig.vecModuleInfo) {
                if (m.strName == "playback") {
                    stAttr.strParentDir = m.strDataPath;
                }
            }
        }

        if (!pPlaybck->Init(stAttr)) {
            DEL_PTR(pPlaybck)
            return pPlaybck;
        }
    }
    return pPlaybck;
}

CAXNVRPreviewCtrl *CAXNVRFramework::initPreviewCtrl(CAXNVRDisplayCtrl *pPrimaryDispCtrl,
                                                    CAXNVRDisplayCtrl *pSecondDispCtrl,
                                                    CDataStreamRecord *pRecord,
                                                    int nDetectType,
                                                    CDetector *pDetect,
                                                    CDetectObserver *pDetectObs) {
    // Preview
    CAXNVRPreviewCtrl *pPreviewCtrl = new (std::nothrow) CAXNVRPreviewCtrl;
    if (pPreviewCtrl) {
        AX_NVR_DEVICE_MGR_ATTR_T stAttr;
        stAttr.pPrimary = pPrimaryDispCtrl;
        stAttr.pSecond = pSecondDispCtrl;
        stAttr.pRecord = pRecord;
        stAttr.enDetectSrc = (AX_NVR_DETECT_SRC_TYPE)nDetectType;
        if (AX_NVR_DETECT_SRC_TYPE::NONE != stAttr.enDetectSrc) {
            stAttr.pDetect = pDetect;
            stAttr.pDetectObs = pDetectObs;
        }
        pPreviewCtrl->Init(stAttr);
    }
    return pPreviewCtrl;
}

CAXNVRPlaybakCtrl *CAXNVRFramework::initPlaybakCtrl(CAXNVRDisplayCtrl *pPrimaryDispCtrl, CDataStreamPlay *pPlayback) {
    // Playback
    CAXNVRPlaybakCtrl *pPlaybakCtrl = new (std::nothrow) CAXNVRPlaybakCtrl;
    if (pPlaybakCtrl) {
        AX_NVR_FILE_MGR_ATTR_T stRecordAttr;
        stRecordAttr.pPrimary = m_pPrimaryDispCtrl;
        stRecordAttr.strPath = m_stRecordCfg.strPath;
        stRecordAttr.pPlayback = pPlayback;

        /* In case test suite enabled, playback streams under directory of configured in test_suite.json */
        AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
        if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
            AX_NVR_TS_CONFIG_T tTestSuiteConfig = CTestSuiteConfigParser::GetInstance()->GetConfig();
            for (auto& m: tTestSuiteConfig.vecModuleInfo) {
                if (m.strName == "playback") {
                    stRecordAttr.strPath = m.strDataPath;
                }
            }
        }

        pPlaybakCtrl->Init(stRecordAttr);
    }
    return pPlaybakCtrl;
}

AX_BOOL CAXNVRFramework::initDetect(int nDetectType) {

    AX_BOOL bRet = AX_FALSE;
    do {
        if (nDetectType == 0) {
            bRet = AX_TRUE;
            break;
        }

        // -- npu engine init
        AX_ENGINE_NPU_ATTR_T tNpuAttr;
        memset(&tNpuAttr, 0, sizeof(AX_ENGINE_NPU_ATTR_T));
        tNpuAttr.eHardMode = AX_ENGINE_VIRTUAL_NPU_STD;
        AX_S32 nRet = AX_ENGINE_Init(&tNpuAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(TAG, "AX_ENGINE_Init fail, ret = 0x%x", nRet);
            break;
        }

        // -- detect init
        DETECTOR_ATTR_T tDAttr;
        tDAttr.nSkipRate = m_stDetectCfg.nSkipRate;
        tDAttr.strModelPath = m_stDetectCfg.strModelPath;
        tDAttr.ePPL = AX_SKEL_PPL_HVCFP;
        tDAttr.nGrpCount = MAX_DETECTOR_GROUP_NUM;
        tDAttr.nChannelNum = m_stDetectCfg.nChannelNum;
        tDAttr.nWidth = m_stDetectCfg.nW;
        tDAttr.nHeight = m_stDetectCfg.nH;
        tDAttr.nFrameDepth = m_stDetectCfg.nDepth;
        m_pDetect = new(std::nothrow) CDetector;
        if (!m_pDetect) {
            LOG_M_E(TAG, "create Detect failed");
            break;
        }

        if (!m_pDetect->Init(tDAttr)) {
            LOG_M_E(TAG, "init Detect failed");
            break;
        }

        m_pDetectObs = new(std::nothrow) CDetectObserver(m_pDetect);
        if (!m_pDetectObs) {
            LOG_M_E(TAG, "create DetectObserver failed");
            break;
        }

        if (!m_pDetect->Start()) {
            LOG_M_E(TAG, "start Detect failed");
            break;
        }

        bRet = AX_TRUE;

    } while(0);

    return bRet;
}