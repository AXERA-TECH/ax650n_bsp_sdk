/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ITSBuilder.h"
#include "AXTypeConverter.hpp"
#include "AppLogApi.h"
#include "CmdLineParser.h"
#include "CommonUtils.hpp"
#include "DetectObserver.hpp"
#include "DspCfgParser.h"
#include "DspObserver.hpp"
#include "EncoderOptionHelper.h"
#include "GlobalDef.h"
#include "ITSFrameCollectorObserver.h"
#include "ITSOSDHelper.h"
#include "ITSSpecConfig.h"
#include "IvpsObserver.h"
#include "IvpsOptionHelper.h"
#include "JencObserver.h"
#include "OptionHelper.h"
#include "OsdObserver.h"
#include "PrintHelper.h"
#include "SensorOptionHelper.h"
#include "VencObserver.h"
#include "ax_adec_api.h"
#include "ax_aenc_api.h"
#include "ax_ai_api.h"
#include "ax_ao_api.h"
#include "ax_engine_api.h"
#include "ax_engine_type.h"
#include "ax_venc_api.h"
#ifndef SLT
#include "./test/ITSTestSuite.h"
#include "./test/ITSTestSuiteCfgParser.h"
#include "AXRtspObserver.h"
#include "AXRtspServer.h"
#include "AlgoOptionHelper.h"
#include "AudioCfgParser.h"
#include "AudioWrapper.hpp"
#include "CaptureObserver.hpp"
#include "IvesObserver.hpp"
#include "MpegObserver.h"
#include "WebObserver.h"
#include "WebOptionHelper.h"
#include "WebServer.h"
#endif

#define PPL "PPL"

#ifndef SLT
namespace {
AX_VOID AudioPlayCallback(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PLAY_FILE_RESULT_PTR pstResult) {
    if (pstResult) {
        LOG_MM(PPL, "[%d] %s play complete, status: %d", eChan, pstResult->pstrFileName, pstResult->eStatus);
    }
}
}  // namespace
#endif

using namespace AX_ITS;

AX_BOOL CITSBuilder::Construct(AX_VOID) {
    /* Step-1: Global initialization */
    if (!InitSysMods()) {
        return AX_FALSE;
    }

    LOG_MM(PPL, "+++");

    if (m_linkage.Setup()) {
        LOG_MM_W(PPL, "Linkage setup.");
    } else {
        LOG_MM_W(PPL, "No linkage setup.");
    }
    AX_BOOL _bSnapshot[MAX_SENSOR_COUNT * MAX_PIPE_PER_DEV]{AX_FALSE};

    /* ###################### Step-1: Initialize AUDIO ###################### */
#ifndef SLT
    // audio
    {
        AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();
        AX_APP_Audio_Init(&stAudioAttr);
    }
#endif

    /* ###################### Step-2: Initialize sensor ###################### */
    /* Step-2-1: Load sensor configurations from json */
    if (m_mgrSensor.Init()) {
#ifndef SLT
        AX_U8 nLastSnsID = 0xFF;
#endif
        AX_U32 nSnsCnt = m_mgrSensor.GetSensorCount();
        for (AX_U32 i = 0; i < nSnsCnt; i++) {
            CBaseSensor* pSensor = m_mgrSensor.GetSnsInstance(i);
            SENSOR_CONFIG_T tSnsConfig = pSensor->GetSnsConfig();

            AX_U32 nPipeCnt = tSnsConfig.nPipeCount;
            for (AX_U32 j = 0; j < nPipeCnt; j++) {
                /* Step-2-2: Initialize SensorMgr's GetYuvFrame thread params */
                AX_U8 nPipeID = tSnsConfig.arrPipeAttr[j].nPipeID;
                _bSnapshot[nPipeID] = tSnsConfig.arrPipeAttr[j].bSnapshot;
                for (AX_S32 nChn = 0; nChn < AX_VIN_CHN_ID_MAX; nChn++) {
                    IPC_MOD_INFO_T tSrcMod = {E_PPL_MOD_TYPE_VIN, nPipeID, nChn};
                    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
                    if (!GetRelationsBySrcMod(tSrcMod, vecRelations)) {
                        continue;
                    }

                    AX_U8 nUnlinkChnCount = 0;
                    for (auto& relation : vecRelations) {
                        if (!relation.bLink) {
                            nUnlinkChnCount++;
                            m_mgrSensor.SetYuvThreadParams(i, nPipeID, nChn, _bSnapshot[nPipeID], AX_FALSE);
                        }
                    }

                    if (nUnlinkChnCount > 1) {
                        m_mgrSensor.SetYuvThreadParams(i, nPipeID, nChn, _bSnapshot[nPipeID], AX_TRUE);
                    }
                }

#ifndef SLT
                /* Step-2-3: Initialize web's camera setting */
                AX_U8 nSnsID = tSnsConfig.nSnsID;
                if (nLastSnsID != nSnsID) {
                    /* Use the first pipe attributes for web options */
                    AX_S32 nScenario;
                    CCmdLineParser::GetInstance()->GetScenario(nScenario);
                    WEB_CAMERA_ATTR_T tCamera;
                    tCamera.nSnsMode = tSnsConfig.eSensorMode;
                    tCamera.nRotation = COptionHelper::GetInstance()->GetRotation();
                    tCamera.fFramerate = tSnsConfig.fFrameRate;
                    tCamera.nDayNightMode = 0;
                    tCamera.nNrMode = 1;
                    {
                        /* Capabilities */
                        tCamera.bSnsModeEnable = AX_FALSE;
                        tCamera.bPNModeEnable = (nScenario == E_PPL_SCENRIO_4 ? AX_TRUE : AX_FALSE);
                        tCamera.bTriggerEnable = (nScenario == E_PPL_SCENRIO_3 ? AX_TRUE : AX_FALSE);
                        tCamera.bLdcEnable = AX_FALSE;

                        if (E_PPL_SCENRIO_0 == nScenario || E_PPL_SCENRIO_1 == nScenario || E_PPL_SCENRIO_2 == nScenario ||
                            E_PPL_SCENRIO_3 == nScenario) {
                            tCamera.bFlashEnable = AX_TRUE;
                            tCamera.bCaptureEnable = AX_TRUE;

                        } else {
                            tCamera.bFlashEnable = AX_FALSE;
                            tCamera.bCaptureEnable = AX_FALSE;
                        }
                    }
                    CWebOptionHelper::GetInstance()->InitCameraAttr(nSnsID, tSnsConfig.eSensorType, tCamera);
                    CTestSuite::GetInstance()->InitCameraAttr(nSnsID, tSnsConfig.eSensorType, tCamera);
                }

                nLastSnsID = nSnsID;
#endif

                LOG_M_W(PPL, "Load sensor config for dev %d pipe %d successfully.", tSnsConfig.nDevID, nPipeID);
            }
        }
    } else {
        LOG_MM_E(PPL, "Initailize SensorMgr failed.");
        return AX_FALSE;
    }

    /* ###################### Step-3: Initialize vin -> collector  (multiple in and out to multiple modules) ###################### */
    for (AX_U8 i = 0; i < MAX_COLLECT_GROUP_NUM; i++) {
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_COLLECT, i, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (!GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            LOG_MM_W(PPL, "Can not find relation for dest module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
            break;
        }

        /* Step-3-1: Initialization */
        CFrameCollector* pCollectorInstance = new CFrameCollector(i);
        if (!pCollectorInstance->Init()) {
            SAFE_DELETE_PTR(pCollectorInstance);
            return AX_FALSE;
        }
        /* Step-3-2: Register target pipe & channel */
        AX_BOOL bSnsRegister = AX_FALSE;
        for (auto& relation : vecRelations) {
            if (!relation.bLink) {
                if (E_PPL_MOD_TYPE_VIN == relation.tSrcModChn.eModType) {
                    bSnsRegister = AX_TRUE;
                    pCollectorInstance->RegTargetChannel(relation.tSrcModChn.nGroup, relation.tSrcModChn.nChannel);
                } else {
                    LOG_MM_W(PPL, "Collector only receives VIN out frames now, please check the ppl.json configuration.");
                    // SAFE_DELETE_PTR(pCollectorInstance);
                    // return AX_FALSE;
                }
            } else {
                LOG_MM_E(PPL, "Collector not support link mode, please check the ppl.json configuration.");
                SAFE_DELETE_PTR(pCollectorInstance);
            }
        }

        /* Step-3-3: Register observers */
        if (bSnsRegister) {
            m_vecCollectorObs.emplace_back(CObserverMaker::CreateObserver<CFrameCollectorObserver>(pCollectorInstance));
            std::vector<std::pair<AX_U8, AX_U8>> vecTargets = pCollectorInstance->GetTargetChannels();
            for (auto& m : vecTargets) {
                m_mgrSensor.RegObserver(m.first, m.second, m_vecCollectorObs[m_vecCollectorObs.size() - 1].get());
            }
        } else {
            m_vecCollectorObs.emplace_back(CObserverMaker::CreateObserver<CFrameCollectorObserver>(pCollectorInstance));
        }

        m_vecCollectorInstance.emplace_back(pCollectorInstance);

        LOG_M_I(PPL, "Init collector group %d successfully.", i);
    }

    /* ###################### Step-4: Initialize IVPS ###################### */
    for (AX_U8 i = 0; i < MAX_IVPS_GROUP_NUM; i++) {
        /* Step-4-1: Load ivps configurations from json */
        IVPS_GROUP_CFG_T tIvpsGrpCfg;
        AX_U8 nScenario = APP_CURR_SCENARIO();
        if (!APP_IVPS_CONFIG(nScenario, i, tIvpsGrpCfg)) {
            /* Not configured for this group */
            break;
        }

        /* Step 4-2: Copy configurations and do initialization */
        CIVPSGrpStage* pIvpsInstance = new CIVPSGrpStage(tIvpsGrpCfg);
        if (!pIvpsInstance->Init()) {
            break;
        }

        if (COptionHelper::GetInstance()->IsEnableOSD()) {
            IOSDHelper* pHelper = new COSDHelper();
            pIvpsInstance->AttachOsdHelper(pHelper);
        }

        /* Step-4-3: Register IVPS observers and fill attributes from source modules */
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_IVPS, i, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (!GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            LOG_MM_E(PPL, "Can not find relation for dest module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
            return AX_FALSE;
        }

        /* NOTICE: Suppose each ivps group has only one frame source */
        IPC_MOD_RELATIONSHIP_T& tRelation = vecRelations[0];
        if (!tRelation.bLink) {
            m_vecIvpsObs.emplace_back(CObserverMaker::CreateObserver<CIvpsObserver>(pIvpsInstance));
            if (E_PPL_MOD_TYPE_COLLECT == tRelation.tSrcModChn.eModType) {
                m_vecCollectorInstance[tRelation.tSrcModChn.nGroup]->RegObserver(m_vecIvpsObs[m_vecIvpsObs.size() - 1].get());

            } else if (E_PPL_MOD_TYPE_VIN == tRelation.tSrcModChn.eModType) {
                m_mgrSensor.RegObserver(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel,
                                        m_vecIvpsObs[m_vecIvpsObs.size() - 1].get());
            } else if (E_PPL_MOD_TYPE_DETECT == tRelation.tSrcModChn.eModType) {
                m_detector.RegObserver(m_vecIvpsObs[m_vecIvpsObs.size() - 1].get());
            }
        } else {
            IVPS_GROUP_CFG_T* pConfig = pIvpsInstance->GetGrpCfg();

            if (E_PPL_MOD_TYPE_COLLECT == tRelation.tSrcModChn.eModType) {
                LOG_M_E(PPL, "Can not link COLLECT to IVPS, please check ppl.json.");
            } else if (E_PPL_MOD_TYPE_VIN == tRelation.tSrcModChn.eModType) {
                AX_U8 nOutChannels = tIvpsGrpCfg.nGrpChnNum;
                for (AX_U8 j = 0; j < nOutChannels; j++) {
                    AX_S8 nSnsID = m_mgrSensor.PipeFromSns(tRelation.tSrcModChn.nGroup);
                    if (-1 == nSnsID) {
                        LOG_M_E(PPL, "[IVPS] Pipe %d does not configured in sensor.json", tRelation.tSrcModChn.nGroup);
                        return AX_FALSE;
                    }

                    pConfig->nSnsSrc = nSnsID;

                    if (-1 == pConfig->arrGrpResolution[0]) {
                        pConfig->arrGrpResolution[0] = m_mgrSensor.GetSnsInstance(nSnsID)
                                                           ->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel)
                                                           .nWidth;
                    }

                    if (-1 == pConfig->arrGrpResolution[1]) {
                        pConfig->arrGrpResolution[1] = m_mgrSensor.GetSnsInstance(nSnsID)
                                                           ->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel)
                                                           .nHeight;
                    }

                    if (-1 == pConfig->arrChnResolution[j][0]) {
                        pConfig->arrChnResolution[j][0] = m_mgrSensor.GetSnsInstance(nSnsID)
                                                              ->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel)
                                                              .nWidth;
                    }

                    if (-1 == pConfig->arrChnResolution[j][1]) {
                        pConfig->arrChnResolution[j][1] = m_mgrSensor.GetSnsInstance(nSnsID)
                                                              ->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel)
                                                              .nHeight;
                    }

                    /* vin frameRate control *1000 ,so the true frameRate is /1000*/
                    if (-1 == pConfig->arrChnFramerate[j][0]) {
                        pConfig->arrChnFramerate[j][0] =
                            m_mgrSensor.GetSnsInstance(nSnsID)->GetPipeAttr(tRelation.tSrcModChn.nGroup).tFrameRateCtrl.fDstFrameRate;
                        if (_bSnapshot[tRelation.tSrcModChn.nGroup]) {
                            pConfig->arrChnFramerate[j][0] *= 2;
                        }
                    }

                    if (-1 == pConfig->arrChnFramerate[j][1]) {
                        pConfig->arrChnFramerate[j][1] = pConfig->arrChnFramerate[j][0];
                    }
                }
            }

            pConfig->nGrpLinkFlag = AX_TRUE;
        }

        /* Step-4-4: Initialize IVPS with latest attributes */
        if (!pIvpsInstance->InitParams()) {
            return AX_FALSE;
        }

        /* Step-4-5: Save instance */
        m_vecIvpsInstance.emplace_back(pIvpsInstance);

        LOG_M_I(PPL, "Init ivps group %d successfully.", i);
    }

    /* ###################### Step-5: Initialize ivps -> collector (single in and out to multiple modules 1-1-N)* ###################### */

    for (AX_U8 i = 0; i < MAX_COLLECT_GROUP_NUM; i++) {
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_COLLECT, i, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (!GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            LOG_MM_W(PPL, "Can not find relation for dest module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
            break;
        }
        for (auto& relation : vecRelations) {
            if (!relation.bLink) {
                AX_U32 nGroup = relation.tSrcModChn.nGroup;
                if (E_PPL_MOD_TYPE_IVPS == relation.tSrcModChn.eModType) {
                    m_vecIvpsInstance[nGroup]->RegObserver(relation.tSrcModChn.nChannel,
                                                           m_vecCollectorObs[relation.tDstModChn.nGroup].get());
                    CFrameCollector* pCollectorInstance = m_vecCollectorInstance[relation.tDstModChn.nGroup];
                    pCollectorInstance->RegTargetChannel(relation.tSrcModChn.nGroup, relation.tSrcModChn.nChannel);

                } else {
                    LOG_MM_W(PPL, "Collector only receives IVSP out frames now, please check the ppl.json configuration.");
                }
            }
        }
    }

    /* ###################### Step-6: Initialize detector ###################### */
    {
        AX_U8 DetectFlag = 0;
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_DETECT, 0, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            m_vecDetectorObs.emplace_back(CObserverMaker::CreateObserver<CDetectObserver>(&m_detector));
            for (auto& relation : vecRelations) {
                if (!relation.bLink) {
                    if (E_PPL_MOD_TYPE_COLLECT == relation.tSrcModChn.eModType) {
                        DetectFlag = 1;
                        m_vecCollectorInstance[relation.tSrcModChn.nGroup]->RegObserver(
                            m_vecDetectorObs[m_vecDetectorObs.size() - 1].get());
                    } else if (E_PPL_MOD_TYPE_IVPS == relation.tSrcModChn.eModType) {
                        DetectFlag = 1;
                        m_vecIvpsInstance[relation.tSrcModChn.nGroup]->RegObserver(relation.tSrcModChn.nChannel,
                                                                                   m_vecDetectorObs[m_vecDetectorObs.size() - 1].get());
                    }
                } else {
                    LOG_MM_E(PPL, "Detector does not support link mode, please check the ppl.json configuration.");
                }
            }
        } else {
            LOG_MM_W(PPL, "Can not find relation for dest module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
        }

        if (DetectFlag && CAlgoOptionHelper::GetInstance()->GetDetectAlgoType(0) > 0) {
            DETECTOR_ATTR_T tDetectAttr;
            tDetectAttr.strModelPath = ALGO_HVCFP_PARAM(0).strDetectModelsPath;
            tDetectAttr.ePPL = (AX_SKEL_PPL_E)CAlgoOptionHelper::GetInstance()->GetDetectAlgoType(0);
            tDetectAttr.ePPL = AX_SKEL_PPL_HVCFP;
            tDetectAttr.nGrpCount = MAX_DETECTOR_GROUP_NUM;
            tDetectAttr.nGrp = tDstMod.nGroup;

            if (AX_FALSE == m_detector.Init(tDetectAttr)) {
                LOG_M_E(PPL, "Init detector failed.");
                return AX_FALSE;
            }

#ifndef SLT
            if (ALGO_HVCFP_PARAM(0).bPushToWeb) {
                /* Register detector web observers */
                m_vecWebObs.emplace_back(CObserverMaker::CreateObserver<CWebObserver>(CWebServer::GetInstance()));
                m_detector.RegObserver(m_vecWebObs[m_vecWebObs.size() - 1].get());
            }
#endif

            m_vecOsdObs.emplace_back(CObserverMaker::CreateObserver<COsdObserver>(&m_vecIvpsInstance));
            m_detector.RegObserver(m_vecOsdObs[m_vecOsdObs.size() - 1].get());
        }
    }

    AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();

    /* ###################### Step-7: Initialize VENC ###################### */
    for (AX_U8 i = 0; i < MAX_VENC_CHANNEL_NUM; i++) {
        /* Step-7-1: Load configurations from json */
        VIDEO_CONFIG_T tConfig;
        AX_U8 nScenario = APP_CURR_SCENARIO();
        if (!APP_VENC_CONFIG(nScenario, i, tConfig)) {
            /* Not configured for this channel */
            break;
        }

        CVideoEncoder* pVencInstance = new CVideoEncoder(tConfig);
        if (!pVencInstance->Init()) {
            break;
        }

        /* Step-7-2: IVPS register VENC observers and init VENC attributes according to IVPS attributes */
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_VENC, 0, pVencInstance->GetChannel()};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (!GetRelationsByDstMod(tDstMod, vecRelations)) {
            LOG_MM_E(PPL, "Can not find relation for dest module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
            return AX_FALSE;
        }

        /* TODO: No matter link or not, attributes can always transfer from src module by GET method, need unify? */
        VIDEO_CONFIG_T* pConfig = pVencInstance->GetChnCfg();

        IPC_MOD_RELATIONSHIP_T& tRelation = vecRelations[0];
        m_vecVencObs.emplace_back(CObserverMaker::CreateObserver<CVencObserver>(pVencInstance));

        /*any module regist venc observer must transfer attributes, like resolution info, to venc instance, so this
         * operation must called before venc's InitParams method */
        if (E_PPL_MOD_TYPE_IVPS == tRelation.tSrcModChn.eModType) {
            m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->RegObserver(tRelation.tSrcModChn.nChannel,
                                                                        m_vecVencObs[m_vecVencObs.size() - 1].get());
        } else if (E_PPL_MOD_TYPE_VIN == tRelation.tSrcModChn.eModType) {
            m_mgrSensor.RegObserver(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel,
                                    m_vecVencObs[m_vecVencObs.size() - 1].get());
        } else if (E_PPL_MOD_TYPE_COLLECT == tRelation.tSrcModChn.eModType) {
            m_vecCollectorInstance[tRelation.tSrcModChn.nGroup]->RegObserver(m_vecVencObs[m_vecVencObs.size() - 1].get());
        }

        if (tRelation.bLink) {
            /* Under link mode, any module must transfer attributes, like resolution info, to venc instance. */
            if (E_PPL_MOD_TYPE_IVPS == tRelation.tSrcModChn.eModType) {
                pConfig->nWidth =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnResolution[tRelation.tSrcModChn.nChannel][0];
                pConfig->nHeight =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnResolution[tRelation.tSrcModChn.nChannel][1];
                pConfig->fFramerate =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnFramerate[tRelation.tSrcModChn.nChannel][1];
                pConfig->bFBC =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnFBC[tRelation.tSrcModChn.nChannel][0] == 0 ? AX_FALSE
                                                                                                                                  : AX_TRUE;
            } else if (E_PPL_MOD_TYPE_VIN == tRelation.tSrcModChn.eModType) {
                AX_S8 nSnsID = m_mgrSensor.PipeFromSns(tRelation.tSrcModChn.nGroup);
                if (-1 == nSnsID) {
                    LOG_M_E(PPL, "[VENC] Pipe %d does not configured in sensor.json", tRelation.tSrcModChn.nGroup);
                    return AX_FALSE;
                }

                pConfig->nWidth =
                    m_mgrSensor.GetSnsInstance(nSnsID)->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel).nWidth;
                pConfig->nHeight =
                    m_mgrSensor.GetSnsInstance(nSnsID)->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel).nHeight;
                pConfig->fFramerate =
                    m_mgrSensor.GetSnsInstance(nSnsID)->GetPipeAttr(tRelation.tSrcModChn.nGroup).tFrameRateCtrl.fDstFrameRate;
                pConfig->bFBC = m_mgrSensor.GetSnsInstance(nSnsID)
                                            ->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel)
                                            .tCompressInfo.enCompressMode == 0
                                    ? AX_FALSE
                                    : AX_TRUE;
            } else if (E_PPL_MOD_TYPE_COLLECT == tRelation.tSrcModChn.eModType) {
                pConfig->nWidth = m_vecCollectorInstance[tRelation.tSrcModChn.nGroup]->GetCollectorCfg()->nWidth;
                pConfig->nHeight = m_vecCollectorInstance[tRelation.tSrcModChn.nGroup]->GetCollectorCfg()->nHeight;
                pConfig->fFramerate = m_vecCollectorInstance[tRelation.tSrcModChn.nGroup]->GetCollectorCfg()->fFramerate;
                pConfig->bFBC = m_vecCollectorInstance[tRelation.tSrcModChn.nGroup]->GetCollectorCfg()->bEnableFBC;
            }
            pConfig->bLink = AX_TRUE;
        }

        /* Step-7-3: Initialize VENC with latest attributes */
        if (!pVencInstance->InitParams()) {
            LOG_M_E(PPL, "Init venc %d failed.", tConfig.nChannel);
            return AX_FALSE;
        }

#ifndef SLT
        /* Step-7-4: Initialize web's video setting */
        AX_U8 nEncoderType = CAXTypeConverter::EncoderType2Int(tConfig.ePayloadType);
        AX_U8 nPrevChn = CWebServer::GetInstance()->RegistPreviewChnMappingInOrder(tConfig.nPipeSrc, tConfig.nChannel, nEncoderType);
        WEB_VIDEO_ATTR_T tVideoAttr;

        tVideoAttr.nUniChn = tConfig.nChannel;
        tVideoAttr.bEnable = AX_TRUE;
        tVideoAttr.nEncoderType = nEncoderType; /* 0: H264; 1: MJpeg; 2: H265 */
        tVideoAttr.nBitrate = pConfig->nBitrate;
        tVideoAttr.nWidth = pConfig->nWidth;
        tVideoAttr.nHeight = pConfig->nHeight;
        tVideoAttr.nRcType = CAXTypeConverter::RcMode2Int(tConfig.eRcType);
        memcpy(tVideoAttr.stEncodeCfg, tConfig.stEncodeCfg, sizeof(APP_ENC_RC_CONFIG) * APP_ENCODER_TYPE_MAX);

        tVideoAttr.bLink = pConfig->bLink;
        CWebOptionHelper::GetInstance()->InitVideoAttr(tConfig.nPipeSrc, nPrevChn, tVideoAttr);
        CTestSuite::GetInstance()->InitVideoAttr(tConfig.nPipeSrc, tConfig.nChannel, tVideoAttr);
        /* Step-7-5: Register observers */
        m_vecRtspObs.emplace_back(CObserverMaker::CreateObserver<CAXRtspObserver>(CAXRtspServer::GetInstance()));
        AX_APP_Audio_RegPacketObserver(APP_AUDIO_RTSP_CHANNEL(), m_vecRtspObs[m_vecRtspObs.size() - 1].get());
        pVencInstance->RegObserver(m_vecRtspObs[m_vecRtspObs.size() - 1].get());

        m_vecWebObs.emplace_back(CObserverMaker::CreateObserver<CWebObserver>(CWebServer::GetInstance()));
        pVencInstance->RegObserver(m_vecWebObs[m_vecWebObs.size() - 1].get());

        if (i == 0) {
            m_vecWebObs.emplace_back(CObserverMaker::CreateObserver<CWebObserver>(CWebServer::GetInstance()));
            AX_APP_Audio_RegPacketObserver(APP_AUDIO_WEB_STREAM_CHANNEL(), m_vecWebObs[m_vecWebObs.size() - 1].get());
        }

        if ((0 == tConfig.nChannel) && (COptionHelper::GetInstance()->ISEnableMp4Record())) {
            CMPEG4Encoder* pMp4Instance = new CMPEG4Encoder();
            VIDEO_CONFIG_T* pConfig = pVencInstance->GetChnCfg();
            MPEG4EC_INFO_T tMpeg4Info;
            tMpeg4Info.nchn = pConfig->nChannel;
            tMpeg4Info.nMaxFileInMBytes = 64;

            AX_F32 fRatio = COptionHelper::GetInstance()->GetVencOutBuffRatio();
            AX_U32 nBuffSize = pConfig->nWidth * pConfig->nHeight * 3 / 2 * fRatio;
            tMpeg4Info.stVideoAttr.bEnable = AX_TRUE;
            tMpeg4Info.stVideoAttr.ePt = pConfig->ePayloadType;
            tMpeg4Info.stVideoAttr.nfrWidth = pConfig->nWidth;
            tMpeg4Info.stVideoAttr.nfrHeight = pConfig->nHeight;
            tMpeg4Info.stVideoAttr.nFrameRate = pConfig->fFramerate;
            tMpeg4Info.stVideoAttr.nBitrate = pConfig->nBitrate;
            tMpeg4Info.stVideoAttr.nMaxFrmSize = nBuffSize;

            AX_APP_AUDIO_CHAN_E nAudioMp4Chn = APP_AUDIO_MP4_CHANNEL();
            AX_APP_AUDIO_ENCODER_ATTR_T stAttr;
            if (0 == AX_APP_Audio_GetEncoderAttr(nAudioMp4Chn, &stAttr)) {
                tMpeg4Info.stAudioAttr.bEnable = AX_TRUE;
                tMpeg4Info.stAudioAttr.ePt = (AX_PAYLOAD_TYPE_E)stAttr.eType;
                tMpeg4Info.stAudioAttr.nBitrate = stAttr.nBitRate;
                tMpeg4Info.stAudioAttr.nSampleRate = (AX_U32)stAttr.eSampleRate;
                tMpeg4Info.stAudioAttr.nChnCnt = (AX_APP_AUDIO_SOUND_MODE_MONO == stAttr.eSoundMode) ? 1 : 2;
                tMpeg4Info.stAudioAttr.nAOT = (AX_S32)stAttr.nAOT;
                tMpeg4Info.stAudioAttr.nMaxFrmSize = COptionHelper::GetInstance()->GetAencOutFrmSize();
            } else {
                tMpeg4Info.stAudioAttr.bEnable = AX_FALSE;
            }

            tMpeg4Info.strSavePath = COptionHelper::GetInstance()->GetMp4SavedPath();
            if (AX_FALSE == pMp4Instance->InitParam(tMpeg4Info)) {
                LOG_MM_E(PPL, "MP4 InitParam failed");
                return AX_FALSE;
            }

            //  m_vecMpeg4Obs.emplace_back(CMPEG4Observer::NewInstance(pMp4Instance));
            m_vecMpeg4Obs.emplace_back(CObserverMaker::CreateObserver<CMPEG4Observer>(pMp4Instance));
            AX_APP_Audio_RegPacketObserver(nAudioMp4Chn, m_vecMpeg4Obs[m_vecMpeg4Obs.size() - 1].get());
            pVencInstance->RegObserver(m_vecMpeg4Obs[m_vecMpeg4Obs.size() - 1].get());
            m_vecMpeg4Instance.emplace_back(pMp4Instance);
        }
#else
        if (0 == tConfig.nChannel) {
            CPrintHelper::GetInstance()->SetSLTTargetFPS(E_PH_MOD_VENC, 0, pConfig->fFramerate);
        }
#endif

        /* Step-7-6: Save instance */
        m_vecVencInstance.emplace_back(pVencInstance);

        LOG_M_I(PPL, "Init venc channel %d successfully.", i);
    }

    WEB_AUDIO_ATTR_T tAudioAttr;
    tAudioAttr.fCapture_volume = (AX_F32)stAudioAttr.stCapAttr.stDevAttr.fVolume;
    tAudioAttr.fPlay_volume = (AX_F32)stAudioAttr.stPlayAttr.stDevAttr.fVolume;
    CWebServer::GetInstance()->EnableAudioPlay(APP_AUDIO_PLAY_AVAILABLE());
    CWebServer::GetInstance()->EnableAudioCapture(APP_AUDIO_CAP_AVAILABLE());
    CWebOptionHelper::GetInstance()->SetAudio(tAudioAttr);

    /* ###################### Step-8: Initialize JENC ###################### */
    for (AX_U8 i = 0; i < MAX_JENC_CHANNEL_NUM; i++) {
        /* Step-8-1: Load configurations from json */
        JPEG_CONFIG_T tConfig;
        AX_U8 nScenario = APP_CURR_SCENARIO();
        if (!APP_JENC_CONFIG(nScenario, i, tConfig)) {
            /* Not configured for this channel */
            break;
        }

        CJpegEncoder* pJencInstance = new CJpegEncoder(tConfig);
        if (!pJencInstance->Init()) {
            break;
        }

        /* Step-8-2: IVPS register JENC observers and init JENC attributes according to IVPS attributes */
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_VENC, 0, pJencInstance->GetChannel()};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (!GetRelationsByDstMod(tDstMod, vecRelations)) {
            LOG_MM_E(PPL, "Can not find relation for dest module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
            return AX_FALSE;
        }

        IPC_MOD_RELATIONSHIP_T& tRelation = vecRelations[0];
        if (!tRelation.bLink) {
            m_vecJencObs.emplace_back(CObserverMaker::CreateObserver<CJencObserver>(pJencInstance));

            if (E_PPL_MOD_TYPE_IVPS == tRelation.tSrcModChn.eModType) {
                m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->RegObserver(tRelation.tSrcModChn.nChannel,
                                                                            m_vecJencObs[m_vecJencObs.size() - 1].get());
            } else if (E_PPL_MOD_TYPE_VIN == tRelation.tSrcModChn.eModType) {
                m_mgrSensor.RegObserver(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel,
                                        m_vecJencObs[m_vecJencObs.size() - 1].get());
            }
        } else {
            /* Under link mode, any module must transfer attributes, like resolution info, to jenc instance. */
            JPEG_CONFIG_T* pConfig = pJencInstance->GetChnCfg();
            if (E_PPL_MOD_TYPE_IVPS == tRelation.tSrcModChn.eModType) {
                pConfig->nWidth =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnResolution[tRelation.tSrcModChn.nChannel][0];
                pConfig->nHeight =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnResolution[tRelation.tSrcModChn.nChannel][1];
                pConfig->bFBC =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnFBC[tRelation.tSrcModChn.nChannel][0] == 0 ? AX_FALSE
                                                                                                                                  : AX_TRUE;
                pConfig->nSrcFrameRate =
                    m_vecIvpsInstance[tRelation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnFramerate[tRelation.tSrcModChn.nChannel][1];
            } else if (E_PPL_MOD_TYPE_VIN == tRelation.tSrcModChn.eModType) {
                AX_S8 nSnsID = m_mgrSensor.PipeFromSns(tRelation.tSrcModChn.nGroup);
                if (-1 == nSnsID) {
                    LOG_M_E(PPL, "[JENC] Pipe %d does not configured in sensor.json", tRelation.tSrcModChn.nGroup);
                    return AX_FALSE;
                }

                pConfig->nWidth =
                    m_mgrSensor.GetSnsInstance(nSnsID)->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel).nWidth;
                pConfig->nHeight =
                    m_mgrSensor.GetSnsInstance(nSnsID)->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel).nHeight;
                pConfig->bFBC = m_mgrSensor.GetSnsInstance(nSnsID)
                                            ->GetChnAttr(tRelation.tSrcModChn.nGroup, tRelation.tSrcModChn.nChannel)
                                            .tCompressInfo.enCompressMode == 0
                                    ? AX_FALSE
                                    : AX_TRUE;
            }
            pConfig->bLink = AX_TRUE;
        }

        /* Step-8-3: Initialize JENC with latest attributes */
        if (!pJencInstance->InitParams()) {
            return AX_FALSE;
        }

#ifndef SLT
        /* Register unique capture channel, only the first setting will be effective */
        CWebServer::GetInstance()->RegistUniCaptureChn(pJencInstance->GetChannel(), (JPEG_TYPE_E)pJencInstance->GetChnCfg()->nJpegType);
        /* Step-8-4: Register observers */
        m_vecWebObs.emplace_back(CObserverMaker::CreateObserver<CWebObserver>(CWebServer::GetInstance()));
        pJencInstance->RegObserver(m_vecWebObs[m_vecWebObs.size() - 1].get());
#endif

        /* Step-8-5: Save instance */
        m_vecJencInstance.emplace_back(pJencInstance);

        LOG_M_I(PPL, "Init jenc channel %d successfully.", i);
    }

    /* Step-10: Initialize common pool configurations */
    if (!InitPoolConfig()) {
        return AX_FALSE;
    }

#ifndef SLT
    /* Step-11: Initialize RTSP */
    if (!CAXRtspServer::GetInstance()->Init()) {
        return AX_FALSE;
    }
    LOG_M_I(PPL, "Init Rtsp successfully.");

    /* Step-12: Initialize webserver */
    if (CWebServer::GetInstance()->Init()) {
        CWebServer::GetInstance()->BindPPL(this);
    } else {
        return AX_FALSE;
    }
    LOG_M_I(PPL, "Init Webserver successfully.");

    /*Stop-13: Initialize MPEG4*/
    for (auto pInstance : m_vecMpeg4Instance) {
        if (!pInstance->Init()) {
            return AX_FALSE;
        }
    }
    LOG_M_I(PPL, "Init MPEG4 successfully.");

    /*Step-14: Initialize UT*/
    if (CCmdLineParser::GetInstance()->isUTEnabled()) {
        if (CTestSuite::GetInstance()->Init()) {
            CTestSuite::GetInstance()->BindPPL(this);
        }
    }

    /*Step-15: Initialize DSP*/
    for (int i = 0; i < DSP_ID_MAX; i++) {
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_DSP, i, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            for (auto& relation : vecRelations) {
                if (E_PPL_MOD_TYPE_IVPS == relation.tSrcModChn.eModType) {
                    DSP_ATTR_S tDspCfg;
                    AX_U8 nScenario = APP_CURR_SCENARIO();
                    if (!CDspCfgParser::GetInstance()->GetDspConfig(nScenario, i, tDspCfg)) {
                        /* Not configured for this group */
                        break;
                    }
                    CDspStage* pDsp = new CDspStage(tDspCfg);
                    m_vecDspInstance.emplace_back(pDsp);
                    m_vecDspObs.emplace_back(CObserverMaker::CreateObserver<CDspObserver>(pDsp));
                    m_vecIvpsInstance[relation.tSrcModChn.nGroup]->RegObserver(relation.tSrcModChn.nChannel,
                                                                               m_vecDspObs[m_vecDspObs.size() - 1].get());
                    m_vecIvpsInstance[relation.tSrcModChn.nGroup]->SetChnInplace(relation.tSrcModChn.nChannel, AX_FALSE);
                    pDsp->Init();
                    LOG_MM_I(PPL, "Init DSP successfully.");

                } else if (E_PPL_MOD_TYPE_COLLECT == relation.tSrcModChn.eModType) {
                    DSP_ATTR_S tDspCfg;
                    AX_U8 nScenario = APP_CURR_SCENARIO();
                    if (!CDspCfgParser::GetInstance()->GetDspConfig(nScenario, i, tDspCfg)) {
                        /* Not configured for this group */
                        break;
                    }
                    CDspStage* pDsp = new CDspStage(tDspCfg);
                    m_vecDspInstance.emplace_back(pDsp);
                    m_vecDspObs.emplace_back(CObserverMaker::CreateObserver<CDspObserver>(pDsp));
                    m_vecCollectorInstance[relation.tSrcModChn.nGroup]->RegObserver(m_vecDspObs[m_vecDspObs.size() - 1].get());
                    pDsp->Init();
                    LOG_MM_I(PPL, "Init DSP successfully.");
                }
            }
        }
    }
    LOG_MM_I(PPL, "Init DSP successfully.");

    /*Step-16: Initialize IVES*/
    for (int i = 0; i < MAX_IVES_GROUP_NUM; i++) {
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_IVES, i, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            for (auto& relation : vecRelations) {
                if (E_PPL_MOD_TYPE_IVPS == relation.tSrcModChn.eModType) {
                    CIVESStage* pIves = new CIVESStage();
                    m_vecIvesInstance.emplace_back(pIves);
                    m_vecIvesObs.emplace_back(CObserverMaker::CreateObserver<CIvesObserver>(pIves));
                    m_vecIvpsInstance[relation.tSrcModChn.nGroup]->RegObserver(relation.tSrcModChn.nChannel,
                                                                               m_vecIvesObs[m_vecIvesObs.size() - 1].get());
                    /*fixme: IVES requires a 640*360 resulution ,but INPLACE mode does not support resize */
                    m_vecIvpsInstance[relation.tSrcModChn.nGroup]->SetChnInplace(relation.tSrcModChn.nChannel, AX_FALSE);

                    CWebOptionHelper::GetInstance()->InitIvesAttr(tDstMod.nGroup, pIves);
                    CTestSuite::GetInstance()->InitIvesAttr(tDstMod.nGroup);

                    pIves->Init(tDstMod.nGroup);

                    // web event
                    m_vecWebObs.emplace_back(CObserverMaker::CreateObserver<CWebObserver>(CWebServer::GetInstance()));
                    pIves->RegObserver(m_vecWebObs[m_vecWebObs.size() - 1].get());
                } else if (E_PPL_MOD_TYPE_DSP == relation.tSrcModChn.eModType) {
                    CIVESStage* pIves = new CIVESStage();
                    m_vecIvesInstance.emplace_back(pIves);
                    m_vecIvesObs.emplace_back(CObserverMaker::CreateObserver<CIvesObserver>(pIves));
                    m_vecDspInstance[relation.tSrcModChn.nGroup]->RegObserver(relation.tSrcModChn.nChannel,
                                                                              m_vecIvesObs[m_vecIvesObs.size() - 1].get());
                    CWebOptionHelper::GetInstance()->InitIvesAttr(tDstMod.nGroup, pIves);
                    CTestSuite::GetInstance()->InitIvesAttr(tDstMod.nGroup);

                    pIves->Init(tDstMod.nGroup);

                    // web event
                    m_vecWebObs.emplace_back(CObserverMaker::CreateObserver<CWebObserver>(CWebServer::GetInstance()));
                    pIves->RegObserver(m_vecWebObs[m_vecWebObs.size() - 1].get());
                }
            }
        }
    }
    LOG_MM_I(PPL, "Init IVES successfully.");
    {
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_DETECT, 0, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            for (auto& relation : vecRelations) {
                if (!relation.bLink) {
                    if (E_PPL_MOD_TYPE_COLLECT == relation.tSrcModChn.eModType) {
                        CWebOptionHelper::GetInstance()->InitAiAttr(m_vecCollectorInstance[relation.tSrcModChn.nGroup]->GetGroup());
                        CTestSuite::GetInstance()->InitAiAttr(m_vecCollectorInstance[relation.tSrcModChn.nGroup]->GetGroup());
                    } else if (E_PPL_MOD_TYPE_IVPS == relation.tSrcModChn.eModType) {
                        CWebOptionHelper::GetInstance()->InitAiAttr(m_vecIvpsInstance[relation.tSrcModChn.nGroup]->GetGrpCfg()->nSnsSrc);
                        CTestSuite::GetInstance()->InitAiAttr(m_vecIvpsInstance[relation.tSrcModChn.nGroup]->GetGrpCfg()->nSnsSrc);
                    }
                } else {
                    LOG_MM_E(PPL, "Detector does not support link mode, please check the ppl.json configuration.");
                }
            }
        }
    }
    LOG_M_I(PPL, "Init DETECT successfully.");

    /* ######################### Step-16: Initialize capture ######################### */
    do {
        IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_CAPTURE, 0, 0};
        vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
        if (!GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
            LOG_MM_E(PPL, "Can not find relation for capture module: (Module:%s, Grp:%d, Chn:%d)", ModType2String(tDstMod.eModType).c_str(),
                     tDstMod.nGroup, tDstMod.nChannel);
            break;
        }

        for (const auto& relation : vecRelations) {
            if (relation.bLink || E_PPL_MOD_TYPE_COLLECT != relation.tSrcModChn.eModType) {
                continue;
            }
            m_vecCaptureObs.emplace_back(CObserverMaker::CreateObserver<CCaptureObserver>(&m_capture));
            m_vecCollectorInstance[relation.tSrcModChn.nGroup]->RegObserver(m_vecCaptureObs[m_vecCaptureObs.size() - 1].get());
        }
    } while (0);

#endif

    LOG_MM(PPL, "---");

    return AX_TRUE;
}

AX_BOOL CITSBuilder::Destroy(AX_VOID) {
    LOG_MM(PPL, "+++");

#ifndef SLT
    if (!CWebServer::GetInstance()->DeInit()) {
        return AX_FALSE;
    }

    if (!CAXRtspServer::GetInstance()->DeInit()) {
        return AX_FALSE;
    }

    for (auto& pInstance : m_vecMpeg4Instance) {
        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }
#endif

    for (auto& pInstance : m_vecVencInstance) {
        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }

    for (auto& pInstance : m_vecJencInstance) {
        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }

    for (auto& pInstance : m_vecIvpsInstance) {
        if (COptionHelper::GetInstance()->IsEnableOSD()) {
            pInstance->DetachOsdHelper();
        }

        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }

    if (!m_detector.DeInit()) {
        return AX_FALSE;
    }
#ifndef SLT
    for (auto& pInstance : m_vecIvesInstance) {
        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }
#endif
    for (auto& pInstance : m_vecCollectorInstance) {
        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }
    for (auto& pInstance : m_vecDspInstance) {
        if (!pInstance->DeInit()) {
            return AX_FALSE;
        }
        SAFE_DELETE_PTR(pInstance);
    }
    if (!m_mgrSensor.DeInit()) {
        return AX_FALSE;
    }

#ifndef SLT
    AX_APP_Audio_Deinit();
#endif

    if (!m_linkage.Release()) {
        return AX_FALSE;
    }

    SAFE_DELETE_PTR(m_pPoolConfig);

    if (!DeInitSysMods()) {
        return AX_FALSE;
    }

    LOG_MM(PPL, "---");
    return AX_TRUE;
}

AX_BOOL CITSBuilder::Start(AX_VOID) {
    LOG_MM(PPL, "+++");

    if (!m_pPoolConfig->Start()) {
        LOG_M(PPL, "pool start failed.");
        return AX_FALSE;
    }

#ifndef SLT
    if (!CWebServer::GetInstance()->Start()) {
        return AX_FALSE;
    }

    if (!CAXRtspServer::GetInstance()->Start()) {
        LOG_M_E(PPL, "Start rtsp server failed.");
        return AX_FALSE;
    }

    for (auto& pInstance : m_vecMpeg4Instance) {
        if (!pInstance->Start()) {
            return AX_FALSE;
        }
    }
#endif

    for (auto& pInstance : m_vecVencInstance) {
        STAGE_START_PARAM_T tStartParam;
        tStartParam.bStartProcessingThread = AX_TRUE;
        if (!pInstance->Start(&tStartParam)) {
            return AX_FALSE;
        }
    }

    for (auto& pInstance : m_vecJencInstance) {
        AX_BOOL bLinkMode = pInstance->GetChnCfg()->bLink;

        STAGE_START_PARAM_T tStartParam;
        tStartParam.bStartProcessingThread = bLinkMode ? AX_FALSE : AX_TRUE;
        if (!pInstance->Start(&tStartParam)) {
            return AX_FALSE;
        }
    }

    for (auto& pInstance : m_vecIvpsInstance) {
        AX_BOOL bLinkMode = pInstance->GetGrpCfg()->nGrpLinkFlag == 0 ? AX_FALSE : AX_TRUE;

        STAGE_START_PARAM_T tStartParam;
        tStartParam.bStartProcessingThread = bLinkMode ? AX_FALSE : AX_TRUE;
        if (!pInstance->Start(&tStartParam)) {
            return AX_FALSE;
        }
    }

    if (!m_detector.Start()) {
        return AX_FALSE;
    }

    for (auto& pInstance : m_vecDspInstance) {
        STAGE_START_PARAM_T tStartParam;
        tStartParam.bStartProcessingThread = AX_TRUE;
        if (!pInstance->Start(&tStartParam)) {
            return AX_FALSE;
        }
    }

#ifndef SLT
    for (auto& pInstance : m_vecIvesInstance) {
        if (!pInstance->Start()) {
            return AX_FALSE;
        }
    }

#endif
    for (auto& pInstance : m_vecCollectorInstance) {
        if (!pInstance->Start()) {
            return AX_FALSE;
        }
    }

    if (!m_mgrSensor.Start()) {
        return AX_FALSE;
    }

#ifndef SLT
    AX_APP_Audio_Start();

    if (APP_AUDIO_PLAY_AVAILABLE()) {
        AX_APP_AUDIO_WELCOME_CFG_T stWelcomeConfig = APP_AUDIO_WELCOME_CONFIG();
        if (stWelcomeConfig.bEnable) {
            AX_APP_Audio_PlayFile(APP_AUDIO_LOCAL_PLAY_CHANNEL(), stWelcomeConfig.eType, stWelcomeConfig.strFileName.c_str(), 1,
                                  AudioPlayCallback, this);
        }
    }
#endif

    CPrintHelper::GetInstance()->Start();

#ifndef SLT
    if (CTestSuite::GetInstance()->IsEnabled() && !CTestSuite::GetInstance()->Start()) {
        return AX_FALSE;
    }
#endif
    PostStartProcess();

    LOG_MM(PPL, "---");
    return AX_TRUE;
}

AX_BOOL CITSBuilder::Stop(AX_VOID) {
    LOG_MM(PPL, "+++");

#ifndef SLT
    if (CTestSuite::GetInstance()->IsEnabled() && !CTestSuite::GetInstance()->Stop()) {
        return AX_FALSE;
    }
#endif

    CPrintHelper::GetInstance()->Stop();

#ifndef SLT
    for (auto& pInstance : m_vecMpeg4Instance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }

    AX_APP_Audio_Stop();
#endif

    if (!m_mgrSensor.Stop()) {
        return AX_FALSE;
    }

    for (auto& pInstance : m_vecCollectorInstance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }

    if (!m_detector.Stop()) {
        return AX_FALSE;
    }

#ifndef SLT
    for (auto& pInstance : m_vecIvesInstance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }
#endif

    for (auto& pInstance : m_vecDspInstance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }

    for (auto& pInstance : m_vecIvpsInstance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }

    for (auto& pInstance : m_vecVencInstance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }

    for (auto& pInstance : m_vecJencInstance) {
        if (!pInstance->Stop()) {
            return AX_FALSE;
        }
    }

#ifndef SLT
    if (!CWebServer::GetInstance()->Stop()) {
        return AX_FALSE;
    }

    if (!CAXRtspServer::GetInstance()->Stop()) {
        return AX_FALSE;
    }

#endif

    if (!m_pPoolConfig->Stop()) {
        return AX_FALSE;
    }

    LOG_MM(PPL, "---");
    return AX_TRUE;
}

IPC_MOD_RELATIONSHIP_T CITSBuilder::GetRelation(LINK_MOD_INFO_T& tModLink) const {
    return m_linkage.GetRelation(tModLink);
}

AX_BOOL CITSBuilder::GetRelationsBySrcMod(IPC_MOD_INFO_T& tSrcMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                          AX_BOOL bIgnoreChn /*= AX_FALSE*/) const {
    return m_linkage.GetRelationsBySrcMod(tSrcMod, vecOutRelations, bIgnoreChn);
}

AX_BOOL CITSBuilder::GetRelationsByDstMod(IPC_MOD_INFO_T& tDstMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                          AX_BOOL bIgnoreChn /*= AX_FALSE*/) const {
    return m_linkage.GetRelationsByDstMod(tDstMod, vecOutRelations, bIgnoreChn);
}

AX_BOOL CITSBuilder::GetPrecedingMod(const IPC_MOD_INFO_T& tDstMod, IPC_MOD_INFO_T& tPrecedingMod) {
    return m_linkage.GetPrecedingMod(tDstMod, tPrecedingMod);
}

#ifndef SLT
AX_BOOL CITSBuilder::ProcessWebOprs(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq, AX_VOID** pResult /*= nullptr*/) {
    std::unique_lock<std::mutex> guard(m_muxWebOperation, std::defer_lock);
    if (!guard.try_lock()) {
        LOG_MM_E(PPL, "Web Operation is processing.");
        return AX_FALSE;
    }

    vector<WEB_REQ_OPERATION_T> vecWebOpr;
    vecWebOpr.clear();
    if (!CWebOptionHelper::GetInstance()->ParseWebRequest(eReqType, pJsonReq, vecWebOpr)) {
        LOG_MM_E(PPL, "Parse web request failed ITS.");
        return AX_FALSE;
    }

    SortOperations(vecWebOpr);

    AX_U32 nOprSize = vecWebOpr.size();
    for (AX_U32 i = 0; i < nOprSize; i++) {
        /* Here only get the last operation results now */
        if (!DispatchOpr(vecWebOpr[i], pResult)) {
            LOG_MM_E(PPL, "Dispatch web operation failed.");
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CITSBuilder::DispatchOpr(WEB_REQ_OPERATION_T& tOperation, AX_VOID** pResult /*= nullptr*/) {
    AX_BOOL ret = AX_TRUE;
    switch (tOperation.GetOperationType()) {
        case E_WEB_OPERATION_TYPE_SNS_MODE: {
            m_mgrSensor.SwitchSnsMode(tOperation.nSnsID, tOperation.tSnsMode.nSnsMode);
            break;
        }
        case E_WEB_OPERATION_TYPE_CAMERA_FPS: {
            m_mgrSensor.ChangeSnsFps(tOperation.nSnsID, tOperation.tSnsFpsAttr.fSnsFrameRate);
            break;
        }
        case E_WEB_OPERATION_TYPE_ROTATION: {
            // CWebServer::GetInstance()->StopPreview();
            for (auto pInstance : m_vecIvpsInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    for (AX_U8 nChn = 0; nChn < pInstance->GetGrpCfg()->nGrpChnNum; nChn++) {
                        pInstance->EnableChannel(nChn, AX_FALSE);
                    }
                }
            }

            for (auto pInstance : m_vecVencInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->StopRecv();
                }
            }

            for (auto pInstance : m_vecJencInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->StopRecv();
                }
            }

            for (auto pInstance : m_vecVencInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->UpdateRotation(tOperation.tRotation.nRotation);
                }
            }

            for (auto pInstance : m_vecIvpsInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->UpdateRotation((AX_IVPS_ROTATION_E)tOperation.tRotation.nRotation);
                }
            }

            for (auto pInstance : m_vecJencInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->UpdateRotation(tOperation.tRotation.nRotation);
                }
            }

            for (auto pInstance : m_vecVencInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->StartRecv();
                }
            }

            for (auto pInstance : m_vecJencInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    pInstance->StartRecv();
                }
            }

            for (auto pInstance : m_vecIvpsInstance) {
                if (pInstance->GetSensorSrc() == tOperation.nSnsID) {
                    for (AX_U8 nChn = 0; nChn < pInstance->GetGrpCfg()->nGrpChnNum; nChn++) {
                        pInstance->EnableChannel(nChn, AX_TRUE);
                    }
                }
            }

            // CWebServer::GetInstance()->RestartPreview();

            break;
        }
        case E_WEB_OPERATION_TYPE_TRIGGER: {
            LOG_MM_I(PPL, " E_WEB_OPERATION_TYPE_TRIGGER");
            m_mgrSensor.GetSnsInstance(tOperation.nSnsID)->TriggerFlash();
            break;
        }
        case E_WEB_OPERATION_TYPE_ENC_TYPE: {
            const AX_U8 nEncoderType = tOperation.tEncType.nEncoderType;
            AX_PAYLOAD_TYPE_E ePayloadType = CAXTypeConverter::Int2EncoderType(nEncoderType);
            CPrintHelper::GetInstance()->Reset(E_PH_MOD_VENC, tOperation.nChannel);

            for (auto& pVideoInstance : m_vecVencInstance) {
                if (pVideoInstance->GetChannel() == tOperation.nChannel) {
                    VIDEO_CONFIG_T tConfig;
                    tConfig.nWidth = pVideoInstance->GetResolution().nWidth;
                    tConfig.nHeight = pVideoInstance->GetResolution().nHeight;
                    pVideoInstance->Stop();
                    CWebServer::GetInstance()->UpdateMediaTypeInPreviewChnMap(tOperation.nSnsID, tOperation.nChannel, nEncoderType);
                    pVideoInstance->UpdatePayloadType(ePayloadType);

                    pVideoInstance->StopRecv();
                    for (auto& obs : m_vecRtspObs) {
                        CAXRtspObserver* pRtstpObserver = static_cast<CAXRtspObserver*>(obs.get());
                        if (pRtstpObserver->IsSame(tOperation.nGroup, tOperation.nChannel)) {
                            pRtstpObserver->UpdateVideoPayloadType(AX_TRUE, ePayloadType);
                            break;
                        }
                    }

                    pVideoInstance->UpdateChnResolution(tConfig);
                    pVideoInstance->StartRecv();
                    break;
                }
            }

            CAXRtspServer::GetInstance()->RestartSessions();

            ret = AX_TRUE;
            break;
        }
        case E_WEB_OPERATION_TYPE_RESOLUTION: {
            /* Change IVPS & VENC channel's resolution */
            IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_VENC, 0, tOperation.nChannel};
            IPC_MOD_INFO_T tPrecedingMod;
            tPrecedingMod.eModType = E_PPL_MOD_TYPE_IVPS;
            if (GetPrecedingMod(tDstMod, tPrecedingMod)) {
                CVideoEncoder* pVencInstance = nullptr;
                for (auto& pInstance : m_vecVencInstance) {
                    if (pInstance->GetChannel() == tOperation.nChannel) {
                        pVencInstance = pInstance;
                        break;
                    }
                }

                /* IVPS disable channel */
                m_vecIvpsInstance[tPrecedingMod.nGroup]->EnableChannel(tPrecedingMod.nChannel, AX_FALSE);
                /* VENC stop receive */
                pVencInstance->StopRecv();
                /*Reset chn fifo*/
                pVencInstance->ResetChn();
                pVencInstance->SetPauseFlag(AX_TRUE);
                /* IVPS update resolution */
                m_vecIvpsInstance[tPrecedingMod.nGroup]->UpdateChnResolution(tPrecedingMod.nChannel, tOperation.tResolution.nWidth,
                                                                             tOperation.tResolution.nHeight);
                /* OSD update position and size */
                m_vecIvpsInstance[tPrecedingMod.nGroup]->RefreshOSDByResChange();
                /* VENC update resolution */

                VIDEO_CONFIG_T tConfig;
                tConfig.nWidth = tOperation.tResolution.nWidth;
                tConfig.nHeight = tOperation.tResolution.nHeight;
                pVencInstance->UpdateChnResolution(tConfig);

                /* VENC start receive */
                pVencInstance->StartRecv();
                pVencInstance->SetPauseFlag(AX_FALSE);
                /* IVPS enable channel */
                m_vecIvpsInstance[tPrecedingMod.nGroup]->EnableChannel(tPrecedingMod.nChannel, AX_TRUE);

            } else {
                LOG_MM_E(PPL, "Can not find preceding ivps module for venc unique channel %d.", tOperation.nChannel);
                ret = AX_FALSE;
            }

            break;
        }
        case E_WEB_OPERATION_TYPE_BITRATE: {
            WEB_VIDEO_ATTR_T tAttr;
            AX_BOOL bFound = CWebOptionHelper::GetInstance()->GetVideoByUniChn(tOperation.nSnsID, tOperation.nChannel, tAttr);
            if (!bFound) {
                LOG_MM_E(PPL, "Can not find vodeo information for venc unique channel %d.", tOperation.nChannel);
                ret = AX_FALSE;
                break;
            }
            /* Change VENC channel's bitrate */
            CVideoEncoder* pVencInstance = nullptr;
            for (auto& pInstance : m_vecVencInstance) {
                if (pInstance->GetChannel() == tAttr.nUniChn) {
                    pVencInstance = pInstance;
                }
            }

            if (nullptr != pVencInstance) {
                pVencInstance->UpdateChnBitrate(tOperation.tBitrate.nBitrate);
                break;
            } else {
                ret = AX_FALSE;
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_RC_INFO: {
            WEB_VIDEO_ATTR_T tAttr;
            AX_BOOL bFound = CWebOptionHelper::GetInstance()->GetVideoByUniChn(tOperation.nSnsID, tOperation.nChannel, tAttr);
            if (!bFound) {
                LOG_MM_E(PPL, "Can not find vodeo information for venc unique channel %d.", tOperation.nChannel);
                ret = AX_FALSE;
                break;
            }
            /* Change VENC channel's rc info */
            CVideoEncoder* pVencInstance = nullptr;
            for (auto& pInstance : m_vecVencInstance) {
                if (pInstance->GetChannel() == tOperation.nChannel) {
                    pVencInstance = pInstance;
                }
            }

            if (nullptr != pVencInstance) {
                RC_INFO_T tRcInfo;

                tRcInfo.eRcType = tOperation.tRcInfo.eRcType;
                tRcInfo.nMinQp = tOperation.tRcInfo.nMinQp;
                tRcInfo.nMaxQp = tOperation.tRcInfo.nMaxQp;
                tRcInfo.nMinIQp = tOperation.tRcInfo.nMinIQp;
                tRcInfo.nMaxIQp = tOperation.tRcInfo.nMaxIQp;
                tRcInfo.nMinIProp = tOperation.tRcInfo.nMinIProp;
                tRcInfo.nMaxIProp = tOperation.tRcInfo.nMaxIProp;
                tRcInfo.nBitrate = tOperation.tRcInfo.nBitrate;
                LOG_MM_D(PPL, "rcInfo [eRcType:%d, nMinQp:%d,nMaxQp:%d,nMinIQp:%d,nMaxIQp:%d ,nMinIProp:%d,nMaxIProp:%d]", tRcInfo.eRcType,
                         tRcInfo.nMinQp, tRcInfo.nMaxQp, tRcInfo.nMinIQp, tRcInfo.nMaxIQp, tRcInfo.nMinIProp, tRcInfo.nMaxIProp);
                ret = pVencInstance->UpdateRcInfo(tRcInfo);
                tAttr.SetEncRcCfg(tRcInfo);
                CWebOptionHelper::GetInstance()->SetVideoByUniChn(tOperation.nSnsID, tAttr);
            } else {
                ret = AX_FALSE;
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_DAYNIGHT: {
            CBaseSensor* pSensor = m_mgrSensor.GetSnsInstance(tOperation.nSnsID);
            if (nullptr != pSensor) {
                ret = pSensor->ChangeDaynightMode((AX_DAYNIGHT_MODE_E)tOperation.tDaynight.nDayNightMode);
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_IMAGE_ATTR: {
            CBaseSensor* pSensor = m_mgrSensor.GetSnsInstance(tOperation.nSnsID);
            if (nullptr != pSensor) {
                if (0 == tOperation.tImageAttr.nAutoMode) {
                    /* manual mode*/
                    APP_ISP_IMAGE_ATTR_T tImgAttr;
                    tImgAttr.nAutoMode = tOperation.tImageAttr.nAutoMode;
                    tImgAttr.nSharpness = tOperation.tImageAttr.nSharpness;
                    tImgAttr.nBrightness = tOperation.tImageAttr.nBrightness;
                    tImgAttr.nContrast = tOperation.tImageAttr.nContrast;
                    tImgAttr.nSaturation = tOperation.tImageAttr.nSaturation;

                    ret = pSensor->SetIspIQAttr(tImgAttr);
                } else {
                    /* auto mode */
                    WEB_CAMERA_ATTR_T tCamera = CWebOptionHelper::GetInstance()->GetCamera(tOperation.nSnsID);

                    APP_ISP_IMAGE_ATTR_T tImgAttr;
                    tImgAttr.nAutoMode = tOperation.tImageAttr.nAutoMode;
                    ret = pSensor->RestoreIspIQAttr(tImgAttr);

                    /* update web imageAttr value */
                    tCamera.tImageAttr.nAutoMode = tOperation.tImageAttr.nAutoMode;
                    tCamera.tImageAttr.nSharpness = tImgAttr.nSharpness;
                    tCamera.tImageAttr.nBrightness = tImgAttr.nBrightness;
                    tCamera.tImageAttr.nContrast = tImgAttr.nContrast;
                    tCamera.tImageAttr.nSaturation = tImgAttr.nSaturation;
                    CWebOptionHelper::GetInstance()->SetCamera(tOperation.nSnsID, tCamera);
                }
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_NR_MODE: {
            // m_mgrSensor.ChangeNRMode(tOperation.tNR.nNRMode);
            LOG_MM_W(PPL, "E_WEB_OPERATION_TYPE_NR_MODE unsupport.");
            ret = AX_FALSE;
            break;
        }
        case E_WEB_OPERATION_TYPE_CAPTURE_AUTO: {
            CWebServer::GetInstance()->EnableCaptrue(tOperation.nSnsID, tOperation.tCapEnable.bOn);
            break;
        }
        case E_WEB_OPERATION_TYPE_AI_ENABLE: {
            m_detector.Enable(tOperation.nSnsID, tOperation.tAiEnable.bOn);

            for (auto& pInstance : m_vecIvpsInstance) {
                if (pInstance->GetGrpCfg()->nSnsSrc == tOperation.nSnsID) {
                    if (pInstance->GetOsdHelper()) {
                        pInstance->GetOsdHelper()->EnableAiRegion(tOperation.tAiEnable.bOn);
                    }
                }
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_AI_PUSH_MODE: {
            m_detector.SetSkelPushMode(tOperation.tAiPushStategy);
            break;
        }
        case E_WEB_OPERATION_TYPE_AI_EVENT: {
            if (-1 == tOperation.nSnsID) {
                for (auto& pInstance : m_vecIvesInstance) {
                    if (!((tOperation.tEvent.tMD.bEnable == AX_FALSE) && (pInstance->GetMDCapacity() == tOperation.tEvent.tMD.bEnable))) {
                        pInstance->SetMDCapacity((AX_BOOL)tOperation.tEvent.tMD.bEnable);
                        if (AX_TRUE == pInstance->GetMDCapacity()) {
                            for (AX_S32 i = 0; i < pInstance->GetMDInstance()->GetAreaCount(); i++) {
                                pInstance->GetMDInstance()->SetThresholdY(tOperation.nSnsID, i, tOperation.tEvent.tMD.nThrsHoldY,
                                                                          tOperation.tEvent.tMD.nConfidence);
                            }
                        }
                    }

                    if (!((tOperation.tEvent.tOD.bEnable == AX_FALSE) && (pInstance->GetODCapacity() == tOperation.tEvent.tOD.bEnable))) {
                        pInstance->SetODCapacity((AX_BOOL)tOperation.tEvent.tOD.bEnable);
                        if (AX_TRUE == pInstance->GetODCapacity()) {
                            for (AX_S32 i = 0; i < pInstance->GetODInstance()->GetAreaCount(); i++) {
                                pInstance->GetODInstance()->SetThresholdY(tOperation.nSnsID, i, tOperation.tEvent.tOD.nThrsHoldY,
                                                                          tOperation.tEvent.tOD.nConfidence);
                            }
                        }
                    }

                    if (!((tOperation.tEvent.tOD.bEnable == AX_FALSE) && (pInstance->GetSCDCapacity() == tOperation.tEvent.tSCD.bEnable))) {
                        pInstance->SetSCDCapacity((AX_BOOL)tOperation.tEvent.tSCD.bEnable);
                        if (AX_TRUE == pInstance->GetSCDCapacity()) {
                            pInstance->GetSCDInstance()->SetThreshold(tOperation.nSnsID, tOperation.tEvent.tSCD.nThrsHoldY,
                                                                      tOperation.tEvent.tSCD.nConfidence);
                        }
                    }
                }
            } else {
                CIVESStage* pInstance{nullptr};
                try {
                    pInstance = m_vecIvesInstance.at(tOperation.nSnsID);
                } catch (std::out_of_range& e) {
                    LOG_MM_E(PPL, "nSnsID out of range %d. error: %s", tOperation.nSnsID, e.what());
                    ret = AX_FALSE;
                    break;
                }

                if (!((tOperation.tEvent.tMD.bEnable == AX_FALSE) && (pInstance->GetMDCapacity() == tOperation.tEvent.tMD.bEnable))) {
                    pInstance->SetMDCapacity((AX_BOOL)tOperation.tEvent.tMD.bEnable);
                    if (AX_TRUE == pInstance->GetMDCapacity()) {
                        for (AX_S32 i = 0; i < pInstance->GetMDInstance()->GetAreaCount(); i++) {
                            pInstance->GetMDInstance()->SetThresholdY(tOperation.nSnsID, i, tOperation.tEvent.tMD.nThrsHoldY,
                                                                      tOperation.tEvent.tMD.nConfidence);
                        }
                    }
                }

                if (!((tOperation.tEvent.tOD.bEnable == AX_FALSE) && (pInstance->GetODCapacity() == tOperation.tEvent.tOD.bEnable))) {
                    pInstance->SetODCapacity((AX_BOOL)tOperation.tEvent.tOD.bEnable);
                    if (AX_TRUE == pInstance->GetODCapacity()) {
                        for (AX_S32 i = 0; i < pInstance->GetODInstance()->GetAreaCount(); i++) {
                            pInstance->GetODInstance()->SetThresholdY(tOperation.nSnsID, i, tOperation.tEvent.tOD.nThrsHoldY,
                                                                      tOperation.tEvent.tOD.nConfidence);
                        }
                    }
                }

                if (!((tOperation.tEvent.tOD.bEnable == AX_FALSE) && (pInstance->GetSCDCapacity() == tOperation.tEvent.tSCD.bEnable))) {
                    pInstance->SetSCDCapacity((AX_BOOL)tOperation.tEvent.tSCD.bEnable);
                    if (AX_TRUE == pInstance->GetSCDCapacity()) {
                        pInstance->GetSCDInstance()->SetThreshold(tOperation.nSnsID, tOperation.tEvent.tSCD.nThrsHoldY,
                                                                  tOperation.tEvent.tSCD.nConfidence);
                    }
                }
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_OSD_ATTR: {
            for (auto& pInstance : m_vecIvpsInstance) {
                if (pInstance->GetGrpCfg()->nSnsSrc == tOperation.nSnsID && pInstance->GetGrpCfg()->nGrp == tOperation.nGroup) {
                    if (pInstance->GetOsdHelper()) {
                        pInstance->GetOsdHelper()->Refresh();
                    }
                }
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_CHANNEL_SWITCH: {
            /* Open or Close IVPS channel */
            IPC_MOD_INFO_T tDstMod;
            if (1 == tOperation.tChnSwitch.nEncoderType) {
                tDstMod = {E_PPL_MOD_TYPE_MJENC, 0, tOperation.nChannel};
            } else {
                tDstMod = {E_PPL_MOD_TYPE_VENC, 0, tOperation.nChannel};
            }

            IPC_MOD_INFO_T tPrecedingMod;
            tPrecedingMod.eModType = E_PPL_MOD_TYPE_IVPS;
            if (GetPrecedingMod(tDstMod, tPrecedingMod)) {
                /* IVPS disable channel */
                m_vecIvpsInstance[tPrecedingMod.nGroup]->EnableChannel(tPrecedingMod.nChannel, tOperation.tChnSwitch.bOn);
            } else {
                LOG_MM_E(PPL, "Can not find preceding ivps module for venc/mjpeg unique channel %d.", tOperation.nChannel);
                ret = AX_FALSE;
            }
            break;
        }
        case E_WEB_OPERATION_TYPE_GET_RESOLUTION: {
            WEB_VIDEO_ATTR_T tAttr;
            AX_BOOL bFound = CWebOptionHelper::GetInstance()->GetVideoByUniChn(tOperation.nSnsID, tOperation.nChannel, tAttr);
            if (!bFound) {
                LOG_MM_E(PPL, "Can not find vodeo information for venc unique channel %d.", tOperation.nChannel);
                ret = AX_FALSE;
                break;
            }

            // IVPS_GRP nGrp = g_stageIVPS.GetGrpByVencChn(nChan);
            // IVPS_CHN nChn = g_stageIVPS.GetGrpInnerChnByVencChn(nChan);

            // WEB_CAMERA_ATTR_T tCameraAttr = GetCamera();
            // if (AX_IVPS_ROTATION_90 == tCameraAttr.nRotation || AX_IVPS_ROTATION_270 == tCameraAttr.nRotation) {
            //     AX_U8 nUniqueChn = g_stageIVPS.GetUniqueChn(nGrp, nChn);
            //     ::swap(tAttr.width, tAttr.height);
            //     if (gOptions.IsIvpsChannelFBCEnabled(g_tEPOptions[nUniqueChn].nChannel)) {
            //         tAttr.width = ALIGN_UP(tAttr.width, AX_ENCODER_FBC_WIDTH_ALIGN_VAL);
            //     }
            // }

            /* TODO: Resolution relies on rotation and fbc configurations */
            tOperation.tGetResolution.nWidth = tAttr.nWidth;
            tOperation.tGetResolution.nHeight = tAttr.nHeight;

            sprintf((AX_CHAR*)pResult, "%dx%d", tAttr.nWidth, tAttr.nHeight);

            break;
        }
        case E_WEB_OPERATION_TYPE_GET_TITLE: {
            string strConfigDir = CCommonUtils::GetPPLSpecification();
            sprintf((AX_CHAR*)pResult, "%s", strConfigDir.c_str());

            break;
        }
        case E_WEB_OPERATION_TYPE_VENC_LINK_ENABLE: {
            CVideoEncoder* pVencInstance = nullptr;
            for (auto& pInstance : m_vecVencInstance) {
                if (pInstance->GetChannel() == tOperation.nChannel) {
                    pVencInstance = pInstance;
                }
            }
            if (pVencInstance) {
                pVencInstance->SetSendFlag(AX_FALSE);
                pVencInstance->StopRecv();
                ret = m_linkage.SetLinkMode(E_PPL_MOD_TYPE_VENC, tOperation.nChannel, tOperation.tVencLinkEnable.bLinkEnable);
                if (AX_TRUE != (ret = pVencInstance->UpdateLinkMode(tOperation.tVencLinkEnable.bLinkEnable))) {
                    LOG_MM_E(PPL, "VENC SetLinkMode faild. ret:%x", ret);
                }
                IPC_MOD_INFO_T tDstMod = {E_PPL_MOD_TYPE_VENC, 0, tOperation.nChannel};
                vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
                if (GetRelationsByDstMod(tDstMod, vecRelations, AX_TRUE)) {
                    for (auto relation : vecRelations) {
                        if (E_PPL_MOD_TYPE_VENC == relation.tDstModChn.eModType && relation.tDstModChn.nChannel == tOperation.nChannel) {
                            m_vecIvpsInstance[relation.tSrcModChn.nGroup]->GetGrpCfg()->arrChnLinkFlag[relation.tSrcModChn.nChannel] =
                                tOperation.tVencLinkEnable.bLinkEnable;
                        }
                    }
                }
                pVencInstance->StartRecv();
                if (!tOperation.tVencLinkEnable.bLinkEnable) {
                    pVencInstance->SetSendFlag(AX_TRUE);
                }
            }

            break;
        }
        case E_WEB_OPERATION_TYPE_CAPTURE:
            m_capture.CapturePicture(tOperation.nGroup, tOperation.nChannel, 80, pResult);
            ret = AX_TRUE;
            break;
        case E_WEB_OPERATION_TYPE_AUDIO_ATTR: {
            AX_APP_Audio_SetCapVolume((AX_F32)tOperation.tAudioAttr.nCapture_volume);
            AX_APP_Audio_SetPlayVolume((AX_F32)tOperation.tAudioAttr.nPlay_volume);

            ret = AX_TRUE;
            break;
        }

        default:
            LOG_MM_W(PPL, "invalid opertion.");
            ret = AX_FALSE;
            break;
    }

    if (ret && E_REQ_TYPE_VIDEO == tOperation.eReqType) {
        /*notify web restart preview*/
        WEB_EVENTS_DATA_T event;
        event.eType = E_WEB_EVENTS_TYPE_ReStartPreview;
        event.nReserved = tOperation.nSnsID;
        CWebServer::GetInstance()->SendEventsData(&event);
    }

    return ret;
}

AX_VOID CITSBuilder::PostStartProcess() {
    LOG_MM_D(PPL, "+++");
    AX_U32 nSnsCnt = m_mgrSensor.GetSensorCount();
    for (AX_U32 i = 0; i < nSnsCnt; i++) {
        CBaseSensor* pSensor = m_mgrSensor.GetSnsInstance(i);

        WEB_CAMERA_ATTR_T tCamera = CWebOptionHelper::GetInstance()->GetCamera(i);
        APP_ISP_IMAGE_ATTR_T tImgAttr;
        pSensor->GetIspIQAttr(tImgAttr);
        tCamera.tImageAttr.nAutoMode = tImgAttr.nAutoMode;
        tCamera.tImageAttr.nSharpness = tImgAttr.nSharpness;
        tCamera.tImageAttr.nBrightness = tImgAttr.nBrightness;
        tCamera.tImageAttr.nContrast = tImgAttr.nContrast;
        tCamera.tImageAttr.nSaturation = tImgAttr.nSaturation;
        CWebOptionHelper::GetInstance()->SetCamera(i, tCamera);
    }
}

IModule* CITSBuilder::ModType2Instance(AX_U8 nGrp, AX_U8 nChn, OPR_TARGET_MODULE_E eModuleType) {
    switch (eModuleType) {
        case OPR_TARGET_MODULE_SNS_MGR: {
            return &m_mgrSensor;
        }
        case OPR_TARGET_MODULE_IVPS: {
            return m_vecIvpsInstance[nGrp];
        }
        case OPR_TARGET_MODULE_VENC: {
            return m_vecVencInstance[nChn];
        }
        case OPR_TARGET_MODULE_JENC: {
            return m_vecJencInstance[nChn];
        }
        default:
            return nullptr;
    }

    return nullptr;
}

AX_VOID CITSBuilder::SortOperations(vector<WEB_REQ_OPERATION_T>& vecWebRequests) {
    std::sort(vecWebRequests.begin(), vecWebRequests.end(),
              [](WEB_REQ_OPERATION_T t1, WEB_REQ_OPERATION_T t2) { return t1.nPriority > t2.nPriority; });
}

AX_BOOL CITSBuilder::ProcessTestSuiteOpers(WEB_REQ_OPERATION_T& tOperation) {
    return DispatchOpr(tOperation);
}

#endif

string CITSBuilder::ModType2String(PPL_MODULE_TYPE_E eType) {
    switch (eType) {
        case E_PPL_MOD_TYPE_VIN: {
            return "VIN";
        }
        case E_PPL_MOD_TYPE_IVPS: {
            return "IVPS";
        }
        case E_PPL_MOD_TYPE_VENC: {
            return "VENC";
        }
        // case E_PPL_MOD_TYPE_JENC: {
        //     return "JENC";
        // }
        case E_PPL_MOD_TYPE_SNS_MANAGER: {
            return "SNS_MGR";
        }
        case E_PPL_MOD_TYPE_DETECT: {
            return "DETECT";
        }
        case E_PPL_MOD_TYPE_COLLECT: {
            return "COLLECT";
        }
        case E_PPL_MOD_TYPE_CAPTURE: {
            return "CAPTURE";
        }
        default:
            return "Unknown";
    }
}

AX_BOOL CITSBuilder::InitPoolConfig() {
    LOG_MM(PPL, "+++");
    POOL_ATTR_T tAttr;
    tAttr.nMaxWidth = m_mgrSensor.GetSnsInstance(0)->GetSnsAttr().nWidth;
    tAttr.nMaxHeight = m_mgrSensor.GetSnsInstance(0)->GetSnsAttr().nHeight;
    tAttr.bRotatetion = AX_FALSE;

    m_pPoolConfig = new CPoolConfig(tAttr);
    if (nullptr == m_pPoolConfig) {
        return AX_FALSE;
    }

    LOG_MM(PPL, "---");
    return AX_TRUE;
}

AX_BOOL CITSBuilder::InitSysMods(AX_VOID) {
    m_arrMods.clear();
    m_arrMods.push_back({AX_FALSE, "LOG", bind(&CITSBuilder::APP_LOG_Init, this), bind(&CITSBuilder::APP_LOG_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "SYS", bind(&CITSBuilder::APP_SYS_Init, this), bind(&CITSBuilder::APP_SYS_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "NPU", bind(&CITSBuilder::APP_NPU_Init, this), bind(&CITSBuilder::APP_NPU_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "VIN", AX_VIN_Init, AX_VIN_Deinit});
    m_arrMods.push_back({AX_FALSE, "MIPI_RX", AX_MIPI_RX_Init, AX_MIPI_RX_DeInit});
    m_arrMods.push_back({AX_FALSE, "IVPS", AX_IVPS_Init, AX_IVPS_Deinit});
    m_arrMods.push_back({AX_FALSE, "VENC", bind(&CITSBuilder::APP_VENC_Init, this), bind(&CITSBuilder::APP_VENC_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "ACAP", bind(&CITSBuilder::APP_ACAP_Init, this), bind(&CITSBuilder::APP_ACAP_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "APLAY", bind(&CITSBuilder::APP_APLAY_Init, this), bind(&CITSBuilder::APP_APLAY_DeInit, this)});

    for (auto& m : m_arrMods) {
        AX_S32 ret = m.Init();
        if (0 != ret) {
            LOG_MM_E(PPL, "%s: init module %s fail, ret = 0x%x", __func__, m.strName.c_str(), ret);
            return AX_FALSE;
        } else {
            LOG_MM_W(PPL, "%s: init module %s OK", __func__, m.strName.c_str());
            m.bInited = AX_TRUE;
        }
    }
    CITSSPEC::Init();
    CIVESStage::InitModule();

    return AX_TRUE;
}

AX_BOOL CITSBuilder::DeInitSysMods(AX_VOID) {
    CIVESStage::DeinitModule();

    const auto nSize = m_arrMods.size();
    if (0 == nSize) {
        return AX_TRUE;
    }

    for (AX_S32 i = (AX_S32)(nSize - 1); i >= 0; --i) {
        if (m_arrMods[i].bInited) {
            AX_S32 ret = m_arrMods[i].DeInit();
            if (0 != ret) {
                LOG_MM_E(PPL, "%s: deinit module %s fail, ret = 0x%x", __func__, m_arrMods[i].strName.c_str(), ret);
                return AX_FALSE;
            } else {
                LOG_MM_W(PPL, "%s: deinit module %s OK", __func__, m_arrMods[i].strName.c_str());
            }

            m_arrMods[i].bInited = AX_FALSE;
        }
    }

    m_arrMods.clear();
    return AX_TRUE;
}

AX_S32 CITSBuilder::APP_SYS_Init() {
    AX_S32 nRet = AX_SUCCESS;

    nRet = AX_SYS_Init();
    if (0 != nRet) {
        return nRet;
    }

    AX_APP_Log_SetSysModuleInited(AX_TRUE);

    nRet = AX_POOL_Exit();
    if (0 != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_SYS_DeInit() {
    AX_S32 nRet = AX_SUCCESS;

    nRet = AX_POOL_Exit();
    if (0 != nRet) {
        return nRet;
    }

    AX_APP_Log_SetSysModuleInited(AX_FALSE);

    nRet = AX_SYS_Deinit();
    if (0 != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_VENC_Init() {
    AX_S32 nRet = AX_SUCCESS;

    AX_VENC_MOD_ATTR_T tModAttr;
    memset(&tModAttr, 0, sizeof(AX_VENC_MOD_ATTR_T));
    tModAttr.enVencType = AX_VENC_MULTI_ENCODER;
    tModAttr.stModThdAttr.bExplicitSched = AX_FALSE;
    tModAttr.stModThdAttr.u32TotalThreadNum = COptionHelper::GetInstance()->GetSetVencThreadNum();

    nRet = AX_VENC_Init(&tModAttr);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_VENC_DeInit() {
    AX_S32 nRet = AX_SUCCESS;

    nRet = AX_VENC_Deinit();
    if (AX_SUCCESS != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_ACAP_Init() {
    AX_S32 nRet = AX_SUCCESS;

    if (APP_AUDIO_CAP_AVAILABLE()) {
        nRet = AX_AI_Init();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }

        nRet = AX_AENC_Init();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_ACAP_DeInit() {
    AX_S32 nRet = AX_SUCCESS;

    if (APP_AUDIO_CAP_AVAILABLE()) {
        nRet = AX_AI_DeInit();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }

        nRet = AX_AENC_DeInit();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_APLAY_Init() {
    AX_S32 nRet = AX_SUCCESS;

    if (APP_AUDIO_PLAY_AVAILABLE()) {
        nRet = AX_AO_Init();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }

        nRet = AX_ADEC_Init();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_APLAY_DeInit() {
    AX_S32 nRet = AX_SUCCESS;

    if (APP_AUDIO_PLAY_AVAILABLE()) {
        nRet = AX_AO_DeInit();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }

        nRet = AX_ADEC_DeInit();

        if (AX_SUCCESS != nRet) {
            return nRet;
        }
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_NPU_Init() {
    AX_S32 nRet = AX_SUCCESS;

    AX_ENGINE_NPU_ATTR_T tNpuAttr;
    memset(&tNpuAttr, 0, sizeof(AX_ENGINE_NPU_ATTR_T));
    tNpuAttr.eHardMode = (AX_ENGINE_NPU_MODE_T)COptionHelper::GetInstance()->GetNpuMode();
    nRet = AX_ENGINE_Init(&tNpuAttr);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_NPU_DeInit() {
    AX_S32 nRet = AX_SUCCESS;

    nRet = AX_ENGINE_Deinit();
    if (AX_SUCCESS != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_LOG_Init(AX_VOID) {
    AX_S32 nRet = AX_SUCCESS;

    APP_LOG_ATTR_T tLogAttr;
    memset(&tLogAttr, 0, sizeof(APP_LOG_ATTR_T));

    AX_S32 nLogTarget = 0;
    if (CCmdLineParser::GetInstance()->GetLogTarget(nLogTarget)) {
        tLogAttr.nTarget = nLogTarget;
    } else {
        tLogAttr.nTarget = APP_LOG_TARGET_STDOUT;
    }

    AX_S32 nLogLevel = 0;
    if (CCmdLineParser::GetInstance()->GetLogLevel(nLogLevel)) {
        tLogAttr.nLv = nLogLevel;
    } else {
        tLogAttr.nLv = APP_LOG_ERROR;
    }

    sprintf(tLogAttr.szAppName, "%s", "FRTDemo-ITS");

    nRet = AX_APP_Log_Init(&tLogAttr);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }

    return AX_SUCCESS;
}

AX_S32 CITSBuilder::APP_LOG_DeInit() {
    AX_APP_Log_DeInit();

    return AX_SUCCESS;
}
