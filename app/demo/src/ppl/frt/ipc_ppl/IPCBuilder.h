/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <mutex>
#include "Capture.hpp"
#include "Detector.hpp"
#include "IPCFrameCollector.h"
#include "IPCLinkage.h"
#include "IPCSensorMgr.h"
#include "IPPLBuilder.h"
#include "IVESStage.h"
#include "IVPSGrpStage.h"
#include "JpegEncoder.h"
#include "Mpeg4Encoder.h"
#include "PoolConfig.h"
#include "VideoEncoder.h"
#include "Avs.h"

namespace AX_IPC {

class CIPCBuilder : public IPPLBuilder {
public:
    CIPCBuilder(AX_VOID) = default;
    virtual ~CIPCBuilder(AX_VOID) = default;

public:
    virtual AX_BOOL Construct(AX_VOID) override;
    virtual AX_BOOL Destroy(AX_VOID) override;
    virtual AX_BOOL Start(AX_VOID) override;
    virtual AX_BOOL Stop(AX_VOID) override;

    /* Web relative interfaces */
    virtual AX_BOOL ProcessWebOprs(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq, AX_VOID** pResult = nullptr) override;
    virtual AX_BOOL ProcessTestSuiteOpers(WEB_REQ_OPERATION_T& tOperation) override;

    static AX_VOID CalibrateDone(AX_S32 result, AX_AVSCALI_AVS_PARAMS_T* pAVSParams, AX_AVSCALI_3A_SYNC_RATIO_T* pSyncRatio, AX_VOID* pPrivData);
protected:
    virtual AX_BOOL InitSysMods(AX_VOID);
    virtual AX_BOOL DeInitSysMods(AX_VOID);

    virtual AX_S32 APP_LOG_Init(AX_VOID);
    virtual AX_S32 APP_LOG_DeInit(AX_VOID);
    virtual AX_S32 APP_SYS_Init(AX_VOID);
    virtual AX_S32 APP_SYS_DeInit(AX_VOID);
    virtual AX_S32 APP_VENC_Init(AX_VOID);
    virtual AX_S32 APP_VENC_DeInit(AX_VOID);
    virtual AX_S32 APP_NPU_Init(AX_VOID);
    virtual AX_S32 APP_NPU_DeInit(AX_VOID);
    virtual AX_S32 APP_ACAP_Init(AX_VOID);
    virtual AX_S32 APP_ACAP_DeInit(AX_VOID);
    virtual AX_S32 APP_APLAY_Init(AX_VOID);
    virtual AX_S32 APP_APLAY_DeInit(AX_VOID);
    virtual AX_S32 APP_VIN_Stitch_Attr_Init(AX_VOID);
    virtual AX_S32 APP_VIN_Stitch_Attr_DeInit(AX_VOID);

private:
    AX_BOOL InitAudio();
    AX_BOOL InitSensor();
    AX_BOOL InitIvps();
    AX_BOOL InitVenc();
    AX_BOOL InitJenc();
    AX_BOOL InitCollector();
    AX_BOOL InitDetector();
    AX_BOOL InitIves();
    AX_BOOL InitCapture();
    AX_BOOL InitVo();
    AX_BOOL InitPoolConfig();
    AX_BOOL GetRelationsBySrcMod(IPC_MOD_INFO_T& tSrcMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                 AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetRelationsByDstMod(IPC_MOD_INFO_T& tDstMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                 AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetPrecedingMod(const IPC_MOD_INFO_T& tDstMod, IPC_MOD_INFO_T& tPrecedingMod);
    std::string ModType2String(PPL_MODULE_TYPE_E eType);
    AX_BOOL DispatchOpr(WEB_REQ_OPERATION_T& tOperation, AX_VOID** pResult /*= nullptr*/);
    AX_VOID SortOperations(vector<WEB_REQ_OPERATION_T>& vecWebRequests);
    AX_VOID PostStartProcess(AX_VOID);

    AX_BOOL InitAvs();
    AX_VOID DeInitAvs();
    AX_VOID StartAvs();
    AX_VOID StopAvs();

    AX_BOOL GetEncoder(AX_VOID **ppEncoder, AX_BOOL *pIsJenc, AX_S32 encoderChn);
    AX_VOID UpdateEncoderResolution(AX_VOID *pEncoder, AX_BOOL bJenc, AX_S32 nSrcGrp, AX_S32 srcChn);

    template<typename T1, typename T2>
    AX_VOID UpdateEncoderResolution(T1* pEncoder, T2* pConfig, AX_S32 nSrcGrp, AX_S32 srcChn);

public:
    CSensorMgr m_mgrSensor;
    vector<CIVPSGrpStage*> m_vecIvpsInstance;
    vector<CVideoEncoder*> m_vecVencInstance;
    vector<CFrameCollector*> m_vecCollectorInstance;
    vector<CIVESStage*> m_vecIvesInstance;
    vector<CJpegEncoder*> m_vecJencInstance;
    vector<CMPEG4Encoder*> m_vecMpeg4Instance;
    CDetector m_detector;
    CCapture m_capture;
    CAvs m_avs;

    std::vector<std::unique_ptr<IObserver>> m_vecRtspObs;
    std::vector<std::unique_ptr<IObserver>> m_vecWebObs;
    std::vector<std::unique_ptr<IObserver>> m_vecCollectorObs;
    std::vector<std::unique_ptr<IObserver>> m_vecDetectorObs;
    std::vector<std::unique_ptr<IObserver>> m_vecIvesObs;
    std::vector<std::unique_ptr<IObserver>> m_vecIvpsObs;
    std::vector<std::unique_ptr<IObserver>> m_vecMpeg4Obs;
    std::vector<std::unique_ptr<IObserver>> m_vecCaptureObs;
    std::vector<std::unique_ptr<IObserver>> m_vecOsdObs;

    CLinkage m_linkage;
    CPoolConfig* m_pPoolConfig{nullptr};
    std::mutex m_muxWebOperation;

private:
    typedef struct {
        AX_BOOL bInited;
        std::string strName;
        std::function<AX_S32(AX_VOID)> Init;
        std::function<AX_S32(AX_VOID)> DeInit;
    } SYS_MOD_T;
    std::vector<SYS_MOD_T> m_arrMods;
    AX_S32 m_nScenario{0};
};
}  // namespace AX_IPC