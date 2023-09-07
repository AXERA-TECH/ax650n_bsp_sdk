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

#include "Capture.hpp"
#include "DspStage.h"
#include "IPPLBuilder.h"
#include "Detector.hpp"
#include "ITSFrameCollector.h"
#include "ITSLinkage.h"
#include "ITSSensorMgr.h"
#include "IVPSGrpStage.h"
#include "JpegEncoder.h"
#include "PoolConfig.h"
#include "VideoEncoder.h"

#ifndef SLT
#include <mutex>
#include "IVESStage.h"
#include "Mpeg4Encoder.h"
#endif

namespace AX_ITS {

class CITSBuilder : public IPPLBuilder {
public:
    CITSBuilder(AX_VOID) = default;
    virtual ~CITSBuilder(AX_VOID) = default;

public:
    virtual AX_BOOL Construct(AX_VOID) override;
    virtual AX_BOOL Destroy(AX_VOID) override;
    virtual AX_BOOL Start(AX_VOID) override;
    virtual AX_BOOL Stop(AX_VOID) override;

#ifndef SLT
    /* Web relative interfaces */
    AX_BOOL ProcessWebOprs(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq, AX_VOID** pResult = nullptr) override;
    AX_BOOL ProcessTestSuiteOpers(WEB_REQ_OPERATION_T& tOperation);
#endif

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

private:
    AX_BOOL InitPoolConfig();
    string ModType2String(PPL_MODULE_TYPE_E eType);
    IPC_MOD_RELATIONSHIP_T GetRelation(LINK_MOD_INFO_T& tModLink) const;
    AX_BOOL GetRelationsBySrcMod(IPC_MOD_INFO_T& tSrcMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                 AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetRelationsByDstMod(IPC_MOD_INFO_T& tDstMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                 AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetPrecedingMod(const IPC_MOD_INFO_T& tDstMod, IPC_MOD_INFO_T& tPrecedingMod);
    AX_VOID PostStartProcess();

#ifndef SLT
    IModule* ModType2Instance(AX_U8 nGrp, AX_U8 nChn, OPR_TARGET_MODULE_E eModuleType);
    AX_BOOL DispatchOpr(WEB_REQ_OPERATION_T& tOperation, AX_VOID** pResult = nullptr);
    AX_VOID SortOperations(vector<WEB_REQ_OPERATION_T>& vecWebRequests);
    AX_VENC_RC_MODE_E FormatRcMode(AX_U8 nEncoderType, AX_VENC_RC_MODE_E nRcType);
    AX_VOID SwithchPayloadType(const WEB_REQ_OPERATION_T& tOperation);
    AX_PAYLOAD_TYPE_E GetChannelPayloadType(AX_U8 nChennel) const;
    AX_VOID RemoveObserverFromList(AX_U8 nGrp, AX_U8 nChn);
#endif

public:
    CSensorMgr m_mgrSensor;
    vector<CFrameCollector*> m_vecCollectorInstance;
    vector<CIVPSGrpStage*> m_vecIvpsInstance;
    vector<CVideoEncoder*> m_vecVencInstance;
    vector<CJpegEncoder*> m_vecJencInstance;
    vector<CDspStage*> m_vecDspInstance;

#ifndef SLT
    vector<CIVESStage*> m_vecIvesInstance;
#endif
    CDetector m_detector;
    CCapture m_capture;
    CPoolConfig* m_pPoolConfig{nullptr};
    CLinkage m_linkage;

    std::vector<std::unique_ptr<IObserver>> m_vecCollectorObs;
    std::vector<std::unique_ptr<IObserver>> m_vecDetectorObs;
    std::vector<std::unique_ptr<IObserver>> m_vecIvpsObs;
    std::vector<std::unique_ptr<IObserver>> m_vecVencObs;
    std::vector<std::unique_ptr<IObserver>> m_vecJencObs;
    std::vector<std::unique_ptr<IObserver>> m_vecIvesObs;
    std::vector<std::unique_ptr<IObserver>> m_vecDspObs;
    std::vector<std::unique_ptr<IObserver>> m_vecCaptureObs;
    std::vector<std::unique_ptr<IObserver>> m_vecOsdObs;

#ifndef SLT
    vector<CMPEG4Encoder*> m_vecMpeg4Instance;
    std::vector<std::unique_ptr<IObserver>> m_vecRtspObs;
    std::vector<std::unique_ptr<IObserver>> m_vecWebObs;
    std::vector<std::unique_ptr<IObserver>> m_vecMpeg4Obs;
    std::mutex m_muxWebOperation;
#endif

private:
    typedef struct {
        AX_BOOL bInited;
        std::string strName;
        std::function<AX_S32(AX_VOID)> Init;
        std::function<AX_S32(AX_VOID)> DeInit;
    } SYS_MOD_T;

    std::vector<SYS_MOD_T> m_arrMods;
};

}  // namespace AX_ITS
