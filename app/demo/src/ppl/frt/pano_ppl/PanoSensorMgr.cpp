/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <math.h>
#include "PanoSensorMgr.h"
#include <sys/prctl.h>
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "FramerateCtrlHelper.h"
#include "GlobalDef.h"
#include "OptionHelper.h"
#include "SensorOptionHelper.h"
#include "SensorFactory.hpp"
#include "ax_nt_ctrl_api.h"
#include "ax_nt_stream_api.h"

#define SNS_MGR "SNS_MGR"

using namespace AX_PANO;

///////////////////////////////////////////////////////////////////////////////////////////
AX_BOOL CSensorMgr::Init() {
    AX_U32 nSensorCount = APP_SENSOR_COUNT();
    for (AX_U32 i = 0; i < nSensorCount; i++) {
        SENSOR_CONFIG_T tSensorCfg;
        if (!APP_SENSOR_CONFIG(i, tSensorCfg)) {
            LOG_M_E(SNS_MGR, "Failed to get sensor config %d", i);
            return AX_FALSE;
        }

        CBaseSensor* pSensor = (CBaseSensor*)(CSensorFactory::GetInstance()->CreateSensor(tSensorCfg));
        if (nullptr == pSensor) {
            LOG_M_E(SNS_MGR, "Failed to create sensor instance %d", i);
            return AX_FALSE;
        } else {
            LOG_M(SNS_MGR, "Create sensor instance %d ok.", i);
        }

        pSensor->RegAttrUpdCallback(UpdateAttrCB);

        if (!pSensor->Init()) {
            LOG_M_E(SNS_MGR, "Failed to initial sensor instance %d", i);
            return AX_FALSE;
        } else {
            LOG_M(SNS_MGR, "Initailize sensor %d ok.", i);
        }

        m_vecSensorIns.emplace_back(pSensor);
    }

    SET_APP_WEB_SHOW_SENSOR_MODE(E_WEB_SHOW_SENSOR_MODE_PANO_SINGLE);
    SET_APP_WEB_PANO_SENSOR_ID(0);

    return AX_TRUE;
}

AX_BOOL CSensorMgr::DeInit() {
   for (auto pSensor : m_vecSensorIns) {
        if (!pSensor->Close()) {
            return AX_FALSE;
        }
    }

    for (ISensor* pSensor : m_vecSensorIns) {
        CSensorFactory::GetInstance()->DestorySensor(pSensor);
    }

    return AX_TRUE;
}

AX_BOOL CSensorMgr::Start() {
    for (auto pSensor : m_vecSensorIns) {
        if (!pSensor->Open()) {
            return AX_FALSE;
        }

        if (0 != pSensor->EnableMultiCamSync(AX_TRUE)) {
            return AX_FALSE;
        }
    }

    if (0 != Enable3ASyncRatio(AX_TRUE)) {
        return AX_FALSE;
    }

    for (auto pSensor : m_vecSensorIns) {
        if (!pSensor->StartIspLoopThread()) {
            return AX_FALSE;
        }
    }

    StartNtCtrl();
    StartYuvGetThread();
    StartDispatchRawThread();

    return AX_TRUE;
}

AX_BOOL CSensorMgr::Stop() {
    StopNtCtrl();
    StopYuvGetThread();
    StopDispatchRawThread();
    for (auto pSensor : m_vecSensorIns) {
        if (!pSensor->StopIspLoopThread()) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CSensorMgr::Start(CBaseSensor* pSensor) {
    if (!pSensor->Open()) {
        return AX_FALSE;
    }

    if (!pSensor->StartIspLoopThread()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CSensorMgr::Stop(CBaseSensor* pSensor) {
    if (!pSensor->StopIspLoopThread()) {
        return AX_FALSE;
    }

    if (!pSensor->Close()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CSensorMgr::StartDispatchRawThread() {
    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();
        m_mapDev2ThreadParam[tSnsCfg.nDevID].nSnsID = tSnsCfg.nSnsID;
        m_mapDev2ThreadParam[tSnsCfg.nDevID].nDevID = tSnsCfg.nDevID;
        m_mapDev2ThreadParam[tSnsCfg.nDevID].eHdrMode = tSnsCfg.eSensorMode;
        m_mapDev2ThreadParam[tSnsCfg.nDevID].fDevFramerate = tSnsCfg.fFrameRate;
        for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
            m_mapDev2ThreadParam[tSnsCfg.nDevID].vecTargetPipeFramerate.emplace_back(tSnsCfg.arrPipeAttr[i].nPipeID,
                                                                                     tSnsCfg.arrPipeAttr[i].fPipeFramerate);
        }
    }

    for (auto& m : m_mapDev2ThreadParam) {
        RAW_DISPATCH_THREAD_PARAM_T& tParams = m.second;
        tParams.hThread = std::thread(&CSensorMgr::RawDispatchThreadFunc, this, &tParams);
    }
}

AX_VOID CSensorMgr::StopDispatchRawThread() {
    for (auto& mapDev2Param : m_mapDev2ThreadParam) {
        RAW_DISPATCH_THREAD_PARAM_T& tParams = mapDev2Param.second;
        if (tParams.hThread.joinable()) {
            tParams.bThreadRunning = AX_FALSE;
        }
    }

    for (auto& mapDev2Param : m_mapDev2ThreadParam) {
        RAW_DISPATCH_THREAD_PARAM_T& tParams = mapDev2Param.second;
        if (tParams.hThread.joinable()) {
            tParams.hThread.join();
        }
    }
}

AX_VOID CSensorMgr::StartYuvGetThread() {
    for (auto& mapChn2Param : m_mapYuvThreadParams) {
        for (auto& param : mapChn2Param.second) {
            YUV_THREAD_PARAM_T& tParams = param.second;
            tParams.hThread = std::thread(&CSensorMgr::YuvGetThreadFunc, this, &tParams);
        }
    }
}

AX_VOID CSensorMgr::StopYuvGetThread() {
    for (auto& mapChn2Param : m_mapYuvThreadParams) {
        for (auto& param : mapChn2Param.second) {
            YUV_THREAD_PARAM_T& tParams = param.second;
            if (tParams.hThread.joinable()) {
                tParams.bThreadRunning = AX_FALSE;
            }
        }
    }

    for (auto& mapChn2Param : m_mapYuvThreadParams) {
        for (auto& param : mapChn2Param.second) {
            YUV_THREAD_PARAM_T& tParams = param.second;
            if (tParams.hThread.joinable()) {
                tParams.hThread.join();
            }
        }
    }
}

AX_VOID CSensorMgr::StartNtCtrl() {
    AX_S32 nRet = 0;
    AX_BOOL bInit = AX_FALSE;
    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();
        for (AX_U8 i = 0; i < pSensor->GetPipeCount(); i++) {
            if (tSnsCfg.arrPipeAttr[i].bTuning) {
                nRet = AX_NT_StreamInit(6000);
                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_StreamInit failed, ret=0x%x.", nRet);
                    return;
                }

                nRet = AX_NT_CtrlInit(tSnsCfg.arrPipeAttr[i].nTuningPort);
                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_CtrlInit failed, ret=0x%x.", nRet);
                    return;
                } else {
                    LOG_MM(SNS_MGR, "Enable tunning on port: %d", tSnsCfg.arrPipeAttr[i].nTuningPort);
                }

                bInit = AX_TRUE;
                break;
            }
        }
        if (bInit) break;
    }

    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();

        for (AX_U8 i = 0; i < pSensor->GetPipeCount(); i++) {
            AX_U8 nPipeID = tSnsCfg.arrPipeAttr[i].nPipeID;
            if (tSnsCfg.arrPipeAttr[i].bTuning) {
                for (AX_U8 j = 0; j < AX_VIN_CHN_ID_MAX; j++) {
                    if (tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].tChnCompressInfo.enCompressMode != 0) {
                        LOG_M_W(SNS_MGR, "Pipe %d Channel %d is in compress mode, does not support nt streaming.", nPipeID, j);
                    }
                }

                AX_NT_SetStreamSource(nPipeID);
            }
        }
    }
}

AX_VOID CSensorMgr::StopNtCtrl() {
    AX_S32 nRet = 0;
    AX_BOOL bDeInit = AX_FALSE;
    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();

        for (AX_U8 i = 0; i < pSensor->GetPipeCount(); i++) {
            if (tSnsCfg.arrPipeAttr[i].bTuning) {
                nRet = AX_NT_CtrlDeInit();
                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_CtrlDeInit failed, ret=0x%x.", nRet);
                    return;
                }

                nRet = AX_NT_StreamDeInit();
                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_StreamDeInit failed, ret=0x%x.", nRet);
                    return;
                }

                bDeInit = AX_TRUE;
                break;
            }
        }

        if (bDeInit) break;
    }
}

AX_VOID CSensorMgr::SetYuvThreadParams(AX_U32 nSnsID, AX_U32 nPipeID, AX_U32 nChannel, AX_BOOL bMultiplex) {
    if (nChannel >= AX_VIN_CHN_ID_MAX) {
        return;
    }
    m_mapYuvThreadParams[nPipeID][nChannel].nSnsID = nSnsID;
    m_mapYuvThreadParams[nPipeID][nChannel].nPipeID = nPipeID;
    m_mapYuvThreadParams[nPipeID][nChannel].nIspChn = nChannel;
    m_mapYuvThreadParams[nPipeID][nChannel].bMultiplex = bMultiplex;
    m_mapYuvThreadParams[nPipeID][nChannel].bThreadRunning = AX_FALSE;
}

AX_VOID CSensorMgr::RegObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver) {
    if (nullptr != pObserver) {
        AX_S8 nSensorID = PipeFromSns(nPipeID);
        if (-1 == nSensorID) {
            LOG_MM_E(SNS_MGR, "Pipe %d does not configured in sensor.json", nPipeID);
            return;
        }

        AX_VIN_PIPE_ATTR_T tPipeAttr = m_vecSensorIns[nSensorID]->GetPipeAttr(nPipeID);
        AX_VIN_CHN_ATTR_T tChnAttr = m_vecSensorIns[nSensorID]->GetChnAttr(nPipeID, nChannel);

        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nGroup = nPipeID;
        tTransAttr.nChannel = nChannel;
        tTransAttr.fFramerate = tPipeAttr.tFrameRateCtrl.fDstFrameRate;
        tTransAttr.nWidth = tChnAttr.nWidth;
        tTransAttr.nHeight = tChnAttr.nHeight;
        tTransAttr.bEnableFBC = tChnAttr.tCompressInfo.enCompressMode == AX_COMPRESS_MODE_NONE ? AX_FALSE : AX_TRUE;
        tTransAttr.bLink = AX_FALSE;
        tTransAttr.nSnsSrc = PipeFromSns(nPipeID);

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_VIN, nPipeID, nChannel, &tTransAttr)) {
            m_mapObservers[nPipeID][nChannel].push_back(pObserver);
        }
    }
}

AX_VOID CSensorMgr::UnregObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_mapObservers[nPipeID][nChannel].begin(); it != m_mapObservers[nPipeID][nChannel].end(); it++) {
        if (*it == pObserver) {
            m_mapObservers[nPipeID][nChannel].erase(it);
            break;
        }
    }
}

AX_VOID CSensorMgr::NotifyAll(AX_S32 nPipe, AX_U32 nChannel, AX_VOID* pFrame) {
    if (nullptr == pFrame) {
        return;
    }

    if (m_mapObservers[nPipe][nChannel].size() == 0) {
        ((CAXFrame*)pFrame)->FreeMem();
        return;
    }

    for (vector<IObserver*>::iterator it = m_mapObservers[nPipe][nChannel].begin(); it != m_mapObservers[nPipe][nChannel].end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_VIN, nPipe, nChannel, pFrame);
    }
}

AX_VOID CSensorMgr::VideoFrameRelease(CAXFrame* pAXFrame) {
    if (pAXFrame) {
        AX_U32 nPipe = pAXFrame->nGrp;
        AX_U32 nChn = pAXFrame->nChn;

        m_mtxFrame[nPipe][nChn].lock();
        for (list<CAXFrame*>::iterator it = m_qFrame[nPipe][nChn].begin(); it != m_qFrame[nPipe][nChn].end(); it++) {
            if ((*it)->nGrp == pAXFrame->nGrp && (*it)->nChn == pAXFrame->nChn &&
                (*it)->stFrame.stVFrame.stVFrame.u64SeqNum == pAXFrame->stFrame.stVFrame.stVFrame.u64SeqNum) {
                if (!pAXFrame->bMultiplex || (*it)->DecFrmRef() == 0) {
                    AX_VIN_ReleaseYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, (AX_IMG_INFO_T*)(*it)->pUserDefine);
                    LOG_MM_D(SNS_MGR, "[%d][%d] AX_VIN_ReleaseYuvFrame, seq:%lld, addr:%p", nPipe, nChn,
                             pAXFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pAXFrame->pUserDefine);

                    AX_IMG_INFO_T* pIspImg = (AX_IMG_INFO_T*)(*it)->pUserDefine;

                    SAFE_DELETE_PTR(pIspImg);
                    SAFE_DELETE_PTR((*it));

                    m_qFrame[nPipe][nChn].erase(it);
                }

                break;
            }
        }
        m_mtxFrame[nPipe][nChn].unlock();
    }
}

AX_VOID CSensorMgr::RawDispatchThreadFunc(RAW_DISPATCH_THREAD_PARAM_T* pThreadParam) {
    AX_U8 nDevID = pThreadParam->nDevID;
    AX_F32 fDevFramerate = pThreadParam->fDevFramerate;
    AX_U32 nDevFramerate = fDevFramerate;
    AX_SNS_HDR_MODE_E eHdrMode = pThreadParam->eHdrMode;
    vector<PIPE_FRAMERATE_INFO_T>& vecTargetPipeFramerate = pThreadParam->vecTargetPipeFramerate;

    LOG_MM(SNS_MGR, "[%d] +++", nDevID);

    AX_CHAR szName[32] = {0};
    sprintf(szName, "RAW_DISP_%d", nDevID);
    prctl(PR_SET_NAME, szName);

    if (ceil(fDevFramerate) != (AX_S32)fDevFramerate) {
        nDevFramerate = fDevFramerate * 2;
        for (auto& m : vecTargetPipeFramerate) {
            m.fPipeFramerate = m.fPipeFramerate * 2;
        }
    }

    /* Calculate framerate control for each pipe */
    AX_F32 nDevFramerateRemain = nDevFramerate;
    std::map<AX_U8, std::unique_ptr<CFramerateCtrlHelper>> mapPipe2FrmCtrl;
    for (auto& m : vecTargetPipeFramerate) {
        mapPipe2FrmCtrl[m.nPipeID] = std::unique_ptr<CFramerateCtrlHelper>(new CFramerateCtrlHelper(nDevFramerateRemain, m.fPipeFramerate));
        LOG_M(SNS_MGR, "Framerate control => [dev%d, pipe%d]: %d => %d", nDevID, m.nPipeID, nDevFramerate, (AX_U32)m.fPipeFramerate);

        nDevFramerateRemain -= m.fPipeFramerate;
    }

    AX_S32 nRet = 0;
    pThreadParam->bThreadRunning = AX_TRUE;
    AX_IMG_INFO_T tImg[4] = {0};
    AX_U64 nSeq = 0;
    while (pThreadParam->bThreadRunning) {
        for (AX_S32 i = 0; i < eHdrMode; i++) {
            nRet = AX_VIN_GetDevFrame(nDevID, (AX_SNS_HDR_FRAME_E)i, tImg + i, -1);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(SNS_MGR, "AX_VIN_GetDevFrame failed, ret=0x%x.", nRet);
                continue;
            } else {
                LOG_M_D(SNS_MGR, "[%lld] Get dev frame, hdrframe=%d, seq=%lld.", nSeq, i, tImg[i].tFrameInfo.stVFrame.u64SeqNum);
            }
        }

        for (auto& m : vecTargetPipeFramerate) {
            AX_U8 nPipe = m.nPipeID;
            if (!mapPipe2FrmCtrl[nPipe].get()->FramerateCtrl()) {
                nRet = AX_VIN_SendRawFrame(nPipe, AX_VIN_FRAME_SOURCE_ID_IFE, eHdrMode, (const AX_IMG_INFO_T**)&tImg, 0);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(SNS_MGR, "AX_VIN_SendRawFrame failed, ret=0x%x.", nRet);
                    continue;
                } else {
                    LOG_M_D(SNS_MGR, "[%lld] Send raw to IFE pipe %d, frame num=%d", nSeq, nPipe, eHdrMode);
                }

                break;
            }
        }

        for (AX_S32 i = 0; i < eHdrMode; i++) {
            nRet = AX_VIN_ReleaseDevFrame(nDevID, (AX_SNS_HDR_FRAME_E)i, tImg + i);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(SNS_MGR, "AX_VIN_ReleaseDevFrame failed, ret=0x%x.", nRet);
                continue;
            } else {
                LOG_M_D(SNS_MGR, "[%lld] Release dev frame, hdrframe=%d, seq=%lld.", nSeq, i, tImg[i].tFrameInfo.stVFrame.u64SeqNum);
            }
        }

        nSeq++;
    }
}

AX_VOID CSensorMgr::YuvGetThreadFunc(YUV_THREAD_PARAM_T* pThreadParam) {
    AX_U8 nPipe = pThreadParam->nPipeID;
    AX_U8 nChn = pThreadParam->nIspChn;
    AX_U8 nSnsID = pThreadParam->nSnsID;

    LOG_MM(SNS_MGR, "[%d][%d] +++", nPipe, nChn);

    AX_CHAR szName[32] = {0};
    sprintf(szName, "YUV_Get_%d_%d", nPipe, nChn);
    prctl(PR_SET_NAME, szName);

    AX_S32 nRet = 0;
    pThreadParam->bThreadRunning = AX_TRUE;
    while (pThreadParam->bThreadRunning) {
        if (!m_bGetYuvFlag[nSnsID]) {
            CElapsedTimer::GetInstance()->mSleep(10);
            continue;
        }
        AX_IMG_INFO_T* pVinImg = new (std::nothrow) AX_IMG_INFO_T();
        if (nullptr == pVinImg) {
            LOG_M_E(SNS_MGR, "Allocate buffer for YuvGetThread failed.");
            CElapsedTimer::GetInstance()->mSleep(10);
            continue;
        }

        nRet = AX_VIN_GetYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, pVinImg, 1000);
        if (AX_SUCCESS != nRet) {
            if (pThreadParam->bThreadRunning) {
                LOG_M_E(SNS_MGR, "[%d][%d] AX_VIN_GetYuvFrame failed, ret=0x%x, unreleased buffer=%d", nPipe, nChn, nRet,
                        m_qFrame[nPipe][nChn].size());
            }
            SAFE_DELETE_PTR(pVinImg);
            continue;
        }

        LOG_MM_D(SNS_MGR, "[%d][%d] Seq %llu, Size %d, w:%d, h:%d, PTS:%llu, [FramePhyAddr:0x%llx, FrameVirAddr:0x%llx], addr:%p", nPipe,
                 nChn, pVinImg->tFrameInfo.stVFrame.u64SeqNum, pVinImg->tFrameInfo.stVFrame.u32FrameSize,
                 pVinImg->tFrameInfo.stVFrame.u32Width, pVinImg->tFrameInfo.stVFrame.u32Height, pVinImg->tFrameInfo.stVFrame.u64PTS,
                 pVinImg->tFrameInfo.stVFrame.u64PhyAddr[0], pVinImg->tFrameInfo.stVFrame.u64VirAddr[0], pVinImg);

        ///////////////////////////// DEBUG DATA //////////////////////////////////
        // AX_VIN_ReleaseYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, pVinImg);
        // SAFE_DELETE_PTR(pVinImg);
        // continue;
        ///////////////////////////////////////////////////////////////

        CAXFrame* pAXFrame = new (std::nothrow) CAXFrame();
        pAXFrame->nGrp = nPipe;
        pAXFrame->nChn = nChn;
        pAXFrame->stFrame.stVFrame = pVinImg->tFrameInfo;
        pAXFrame->pFrameRelease = this;
        pAXFrame->pUserDefine = pVinImg;
        /* Here, we can not determine bMultiplex flag according to number of observers, because each observer must filter frames by target
         * pipe & channel */
        pAXFrame->bMultiplex = pThreadParam->bMultiplex;

        m_mtxFrame[nPipe][nChn].lock();
        if (m_qFrame[nPipe][nChn].size() >= 5) {
            LOG_MM_W(SNS_MGR, "[%d][%d] queue size is %d, drop this frame", nPipe, nChn, m_qFrame[nPipe][nChn].size());
            AX_VIN_ReleaseYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, pVinImg);
            SAFE_DELETE_PTR(pVinImg);
            SAFE_DELETE_PTR(pAXFrame);

            m_mtxFrame[nPipe][nChn].unlock();
            continue;
        }

        m_qFrame[nPipe][nChn].push_back(pAXFrame);
        m_mtxFrame[nPipe][nChn].unlock();

        NotifyAll(nPipe, nChn, pAXFrame);
    }

    LOG_MM(SNS_MGR, "[%d][%d] ---", nPipe, nChn);
}

AX_BOOL CSensorMgr::RestartWithScenario(AX_S32 nScenario) {
    if (!Stop()) {
        return AX_FALSE;
    }

    if (!DeInit()) {
        return AX_FALSE;
    }

    SET_APP_CURR_SCENARIO((AX_IPC_SCENARIO_E)nScenario);

    if (!Init()) {
        return AX_FALSE;
    }

    if (!Start()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

CBaseSensor* CSensorMgr::GetSnsInstance(AX_U32 nIndex) {
    if (nIndex >= m_vecSensorIns.size()) {
        return nullptr;
    }

    return m_vecSensorIns[nIndex];
}

AX_S8 CSensorMgr::PipeFromSns(AX_U8 nPipeID) {
    for (AX_U8 i = 0; i < GetSensorCount(); i++) {
        CBaseSensor* pSensor = GetSnsInstance(i);
        AX_U32 nPipeCount = pSensor->GetPipeCount();
        for (AX_U8 j = 0; j < nPipeCount; j++) {
            if (nPipeID == pSensor->GetSnsConfig().arrPipeAttr[j].nPipeID) {
                return i;
            }
        }
    }

    return -1;
}

AX_BOOL CSensorMgr::UpdateAttrCB(ISensor* pInstance) {
    if (nullptr == pInstance) {
        return AX_FALSE;
    }

    /* Sample code to update attributes before sensor.Open */
    // SNS_ABILITY_T tSnsAbilities = pInstance->GetAbilities();

    // AX_VIN_PIPE_ATTR_T tPipeAttr = pInstance->GetPipeAttr();
    // tPipeAttr.tCompressInfo = AX_TRUE;
    // pInstance->SetPipeAttr(tPipeAttr);

    return AX_TRUE;
}

AX_BOOL CSensorMgr::ChangeDaynightMode(AX_U32 nSnsID, AX_DAYNIGHT_MODE_E eDaynightMode) {
    CBaseSensor* pSensor = GetSnsInstance(nSnsID);
    AX_U32 nPipeCount = pSensor->GetPipeCount();
    for (AX_U8 i = 0; i < nPipeCount; i++) {
        for (AX_U8 j = 0; j < AX_VIN_CHN_ID_MAX; j++) {
            AX_VIN_CHN_ATTR_T tChnAttr = pSensor->GetChnAttr(i, j);
            if (0 == tChnAttr.nDepth) {
                continue;
            }

            AX_S32 nRet = AX_VIN_SetChnDayNightMode(i, (AX_VIN_CHN_ID_E)j, eDaynightMode);
            if (0 != nRet) {
                LOG_M_E(SNS_MGR, "[%d][%d][%d] AX_VIN_SetChnDayNightMode failed, ret=0x%x.", nSnsID, i, j, nRet);
                return AX_FALSE;
            } else {
                LOG_M_W(SNS_MGR, "[%d][%d][%d] AX_VIN_SetChnDayNightMode OK.", nSnsID, i, j);
            }
        }
    }

    return AX_TRUE;
}

AX_VOID CSensorMgr::SwitchSnsMode(AX_U32 nSnsID, AX_U32 nSnsMode) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }
    m_bGetYuvFlag[nSnsID] = AX_FALSE;
    Stop(pCurSensor);

    pCurSensor->ChangeHdrMode(nSnsMode);
    AX_U8 nDevID = pCurSensor->GetSnsConfig().nDevID;
    m_mapDev2ThreadParam[nDevID].eHdrMode = (AX_SNS_HDR_MODE_E)nSnsMode;
    pCurSensor->Init();

    Start(pCurSensor);
    m_bGetYuvFlag[nSnsID] = AX_TRUE;

    return;
}

AX_VOID CSensorMgr::ChangeSnsFps(AX_U32 nSnsID, AX_F32 fFrameRate) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }
    AX_SNS_ATTR_T stSnsAttr = pCurSensor->GetSnsAttr();
    stSnsAttr.fFrameRate = fFrameRate;
    pCurSensor->SetSnsAttr(stSnsAttr);
    pCurSensor->UpdateSnsAttr();
}

AX_VOID CSensorMgr::ChangeSnsMirrorFlip(AX_U32 nSnsID, AX_BOOL bMirror, AX_BOOL bFlip) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }

    pCurSensor->ChangeSnsMirrorFlip(bMirror, bFlip);
}

AX_VOID CSensorMgr::SetAeSyncRatio(const AX_ISP_IQ_AE_SYNC_RATIO_T& tAeSyncRatio) {
    m_tAeSyncRatio = tAeSyncRatio;
}

AX_VOID CSensorMgr::SetAwbSyncRatio(const AX_ISP_IQ_AWB_SYNC_RATIO_T& tAwbSyncRatio) {
    m_tAwbSyncRatio = tAwbSyncRatio;
}

AX_S32 CSensorMgr::Enable3ASyncRatio(AX_BOOL bEnable) {
    AX_S32 nRet = 0;
    AX_U32 n3ARatioOffValue = 1 << 20;

    AX_ISP_IQ_AE_SYNC_RATIO_T  tAeSyncRatio{0};
    AX_ISP_IQ_AWB_SYNC_RATIO_T tAwbSyncRatio{0};

    tAeSyncRatio.nAeSyncRatio = bEnable ? m_tAeSyncRatio.nAeSyncRatio : n3ARatioOffValue;
    tAwbSyncRatio.nRGainRatio = bEnable ? m_tAwbSyncRatio.nRGainRatio : n3ARatioOffValue;
    tAwbSyncRatio.nBGainRatio = bEnable ? m_tAwbSyncRatio.nBGainRatio : n3ARatioOffValue;

    LOG_M_I(SNS_MGR, "bEnable: %d, nAeSyncRatio: %d, nRGainRatio: %d, nBGainRatio: %d",
                    bEnable,
                    tAeSyncRatio.nAeSyncRatio,
                    tAwbSyncRatio.nRGainRatio,
                    tAwbSyncRatio.nBGainRatio);

    nRet = AX_ISP_IQ_SetAeSyncParam(&tAeSyncRatio);
    if (0 != nRet) {
        LOG_M_E(SNS_MGR, "AX_ISP_IQ_SetAeSyncParam fail, nRet:%d", nRet);
        return nRet;
    }

    nRet = AX_ISP_IQ_SetAwbSyncParam(&tAwbSyncRatio);
    if (0 != nRet) {
        LOG_M_E(SNS_MGR, "AX_ISP_IQ_SetAwbSyncParam fail, nRet:%d", nRet);
        return nRet;
    }

    return nRet;
}
