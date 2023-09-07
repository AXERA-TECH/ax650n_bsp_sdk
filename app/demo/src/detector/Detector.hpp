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
#include <vector>
#include "AXFrame.hpp"
#include "AXLockQ.hpp"
#include "AXResource.hpp"
#include "AXThread.hpp"
#include "IObserver.h"
#include "WebServer.h"
#include "ax_skel_api.h"
#include "AXAlgo.hpp"


#define MAX_DETECTOR_GROUP_NUM (2)

typedef struct DETECTOR_ATTR_S {
    AX_U8 nGrpCount{0};
    AX_S32 nGrp{0};
    std::string strModelPath;
    AX_SKEL_PPL_E ePPL{AX_SKEL_PPL_MAX};
    AX_U32 nFrameDepth;
    AX_U32 nWidth{0};
    AX_U32 nHeight{0};
    AX_S8 nSnsId[MAX_DETECTOR_GROUP_NUM]{-1};
} DETECTOR_ATTR_T;

typedef struct {
    AX_U64 nSeqNum{0};
    AX_S32 nGrpId{0};
    AX_S32 nChnId{0};
    AX_S8 nSnsId{0};
} SKEL_FRAME_PRIVATE_DATA_T;

/**
 * @brief
 *
 */
class CDetector {
public:
    CDetector(AX_VOID) = default;
    virtual ~CDetector(AX_VOID) = default;

    AX_BOOL Init(const DETECTOR_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL SendFrame(AX_U32 nGrp, CAXFrame* axFrame);
    AX_S32 CheckCapacity(AX_S32 nTimeOut);

    AX_VOID RegObserver(IObserver* pObserver);
    AX_VOID UnregObserver(IObserver* pObserver);
    DETECTOR_ATTR_T* GetDetectorCfg() {
        return &m_stAttr;
    };

    AX_VOID Enable(AX_U8 nSnsId, AX_BOOL bEnable = AX_TRUE);

    AX_VOID ReleaseSkelPrivateData(SKEL_FRAME_PRIVATE_DATA_T* pData) {
        m_skelData.giveback(pData);
    }

    AX_VOID NotifyAll(AX_U32 nSnsId, AX_U32 nChn, AX_VOID* pStream);

    AX_S32 SetSkelPushMode(AI_PUSH_STATEGY_T& stStrategy);

    AX_BOOL SetRoi(const AX_APP_ALGO_ROI_CONFIG_T& stRoi, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetPushStrategy(const AX_APP_ALGO_PUSH_STRATEGY_T& stPushStrategy, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetObjectFilter(AX_APP_ALGO_HVCFP_TYPE_E eType, const AX_APP_ALGO_HVCFP_FILTER_CONFIG_T& stObjectFliter, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetTrackSize(const AX_APP_ALGO_TRACK_SIZE_T& stTrackSize, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetPanorama(const AX_APP_ALGO_PANORAMA_T& stPanorama, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetCropEncoderQpLevel(const AX_U32& nCropEncoderQpLevel, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetCropThreshold(AX_APP_ALGO_HVCFP_TYPE_E eType, const AX_APP_ALGO_CROP_THRESHOLD_CONFIG_T& stCropThreshold, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetPushFilter(AX_APP_ALGO_HVCFP_TYPE_E eType, const AX_APP_ALGO_PUSH_FILTER_CONFIG_T& stPushFliter, AX_BOOL bUpdateParam = AX_FALSE);
    AX_BOOL SetConfig(const AX_APP_ALGO_HVCFP_PARAM_T& stConfig, AX_BOOL bUpdateParam = AX_FALSE);

protected:
    AX_VOID RunDetect(AX_VOID* pArg);
    AX_VOID ClearQueue(AX_S32 nGrp);

protected:
    CAXLockQ<CAXFrame*>* m_arrFrameQ{nullptr};
    DETECTOR_ATTR_T m_stAttr;
    CAXThread m_DetectThread;
    AX_SKEL_HANDLE m_Skel{NULL};

private:
    std::vector<IObserver*> m_vecObserver;
    AX_BOOL m_initState{AX_FALSE};
    AX_BOOL m_arrSnsAiEnable[AX_APP_ALGO_SNS_MAX]{AX_TRUE, AX_TRUE};
    AX_U32 m_nSnsId{0};
    CAXResource<SKEL_FRAME_PRIVATE_DATA_T> m_skelData;
};
