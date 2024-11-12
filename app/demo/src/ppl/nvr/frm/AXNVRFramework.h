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
#include "AXSingleton.h"
#include "detector.hpp"
#include "datastream_record.hpp"
#include "datastream_play.hpp"

#include "AXNVRFrameworkDefine.h"
#include "RemoteDeviceParser.h"
#include "NVRConfigParser.h"
#include "AXNVRDisplayCtrl.h"
#include "AXNVRPreviewCtrl.h"
#include "AXNVRPlaybackCtrl.h"


class CAXNVRFramework : public CAXSingleton<CAXNVRFramework> {
    friend class CAXSingleton<CAXNVRFramework>;

private:
    CAXNVRFramework(AX_VOID) noexcept = default;
    virtual ~CAXNVRFramework(AX_VOID) = default;

public:
    AX_BOOL Init(AX_U32 nDetectType, AX_U32 loglevel);
    AX_VOID DeInit();

    // Vo
    CAXNVRDisplayCtrl *PrimaryDispCtrl(AX_VOID);
    AX_BOOL InitSecondaryDispCtrl(AX_VOID);
    AX_VOID DeInitSecondaryDispCtrl(AX_VOID);

    // AX_BOOL UpdateView(AX_NVR_VIEW_TYPE enViewType, AX_NVR_VIEW_CHANGE_TYPE enChangeType, const vector<AX_NVR_RECT_T> &vecVoRect = vector<AX_NVR_RECT_T>());

    AX_S32 MapToDisplay(AX_NVR_VIEW_TYPE enViewType, AX_NVR_DEV_ID nDevID);

    // preview and playback
    CAXNVRPreviewCtrl *PreviewCtrl(AX_VOID);
    CAXNVRPlaybakCtrl *PlaybakCtrl(AX_VOID);

    // AX_BOOL ZoomAndMove(AX_NVR_DEV_ID nDevID, AX_NVR_VIEW_TYPE enViewType, const AX_NVR_RECT_T &stCropRect, AX_BOOL bReset);
    AX_VOID GetResolution(AX_NVR_DEV_ID nDevID, AX_NVR_VIEW_TYPE enViewType, AX_U32 &nWidth, AX_U32 &nHeight);
    vector<AX_U32> GetDeviceFileList(AX_VOID);

    // Round Patrol
    AX_BOOL StartRoundPatrol(AX_VOID);
    AX_VOID StopRoundPatrol(AX_VOID);
    AX_BOOL UpdateRoundPatrol(AX_VOID);
    AX_U32 GetCurrentSplitCnt(AX_VOID);

    inline AX_VOID SetRoundPatrolInfo(const AX_NVR_RPATROL_CONFIG_T &stInfo) { m_stRPratrolCfg = stInfo; };
    inline const AX_NVR_RPATROL_CONFIG_T& GetRoundPatrolInfo() const { return m_stRPratrolCfg; };
    inline AX_VOID SetRoundPatrolDevice(const vector<AX_NVR_DEV_ID> &vecDevID) {};
    inline const vector<AX_NVR_DEV_ID> GetRoundPatrolDevice() const { return vector<AX_NVR_DEV_ID>(); };

private:
    std::mutex mutex_;
    int m_nStartDevId = 0;

    CAXNVRDisplayCtrl *m_pPrimaryDispCtrl = nullptr;
    CAXNVRDisplayCtrl *m_pSecondaryDispCtrl = nullptr;
    CAXNVRPreviewCtrl *m_pPreviewCtrl = nullptr;
    CAXNVRPlaybakCtrl *m_pPlaybakCtrl = nullptr;

    CDetectObserver *m_pDetectObs = nullptr;
    CDetector *m_pDetect = nullptr;
    CDataStreamRecord *m_pDataStreamRecord = nullptr;
    CDataStreamPlay *m_pDataStreamPlayback = nullptr;

    AX_NVR_RPATROL_CONFIG_T m_stRPratrolCfg;
    AX_NVR_DETECT_CONFIG_T m_stDetectCfg;
    AX_NVR_DISPVO_CONFIG_T m_stPrimaryCfg;
    AX_NVR_DISPVO_CONFIG_T m_stSecondaryCfg;
    AX_NVR_RECORD_CONFIG_T m_stRecordCfg;
    AX_NVR_DEVICE_CONFIG_T m_stDeviceCfg;

    AX_BOOL m_bSecondaryInit = AX_FALSE;

private:
    CAXNVRDisplayCtrl *initPrimaryDispCtrl(AX_VOID);
    AX_BOOL initVdec(AX_VOID);
    AX_BOOL initIvps(AX_VOID);
    AX_BOOL initVenc(AX_VOID);

    AX_BOOL initDetect(int nDetectType);
    CDataStreamRecord *initDataStreamRecord(AX_VOID);
    CDataStreamPlay *initDataStreamPlayback(AX_VOID);
    CAXNVRPreviewCtrl *initPreviewCtrl(CAXNVRDisplayCtrl *pPrimaryDispCtrl,
                                    CAXNVRDisplayCtrl *pSecondDispCtrl,
                                    CDataStreamRecord *pRecord,
                                    int nDetectType,
                                    CDetector *pDetect,
                                    CDetectObserver *pDetectObs);

    CAXNVRPlaybakCtrl *initPlaybakCtrl(CAXNVRDisplayCtrl *pPrimaryDispCtrl,
                                    CDataStreamPlay *pPlayback);

};

inline CAXNVRPreviewCtrl *CAXNVRFramework::PreviewCtrl(AX_VOID) {
    return m_pPreviewCtrl;
}

inline CAXNVRPlaybakCtrl *CAXNVRFramework::PlaybakCtrl(AX_VOID) {
    return m_pPlaybakCtrl;
}

inline CAXNVRDisplayCtrl *CAXNVRFramework::PrimaryDispCtrl(AX_VOID) {
    return m_pPrimaryDispCtrl;
}