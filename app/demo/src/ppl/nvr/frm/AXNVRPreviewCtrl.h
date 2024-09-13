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
#include "AXNVRDisplayCtrl.h"
#include "AXNVRChannel.h"
#include "AXNVRFrameworkDefine.h"
#include "ax_base_type.h"
#include "detector.hpp"
#include "datastream_record.hpp"
#include "AXThread.hpp"

#include <vector>
#include <map>
#include <mutex>
using namespace std;


typedef struct _AX_NVR_DEVICE_MGR_ATTR_T {
    AX_U32 nMaxCount = MAX_DEVICE_COUNT;
    CAXNVRDisplayCtrl *pPrimary = nullptr;
    CAXNVRDisplayCtrl *pSecond = nullptr;
    CDataStreamRecord *pRecord = nullptr;
    CDetector *pDetect = nullptr;
    CDetectObserver *pDetectObs = nullptr;
    AX_NVR_DETECT_SRC_TYPE enDetectSrc = AX_NVR_DETECT_SRC_TYPE::NONE;
} AX_NVR_DEVICE_MGR_ATTR_T;

typedef struct _AX_NVR_DEVICE_T {
    CAXNVRChannel *pRPatrolChn;
    CAXThread *pThreadRPatrol;
    CAXThread *pThreadRPatrolUpdate;
    CAXNVRChannel *pPreviewChnMain;
    CAXNVRChannel *pPreviewChnSub1;
    CAXThread *pThreadPreview;
    AX_NVR_DEV_INFO_T stDevInfo;
    AX_BOOL bRes = AX_TRUE;
} AX_NVR_DEVICE_T;


/**
 * @brief for preivew and round-patrol
 *
 */
class CAXNVRPreviewCtrl {
public:
    CAXNVRPreviewCtrl(AX_VOID) = default;
    virtual ~CAXNVRPreviewCtrl(AX_VOID) = default;

    // init-deinit
    AX_VOID Init(const AX_NVR_DEVICE_MGR_ATTR_T &stAttr);
    AX_VOID DeInit();

    // device manager
    AX_BOOL InsertDevice(const AX_NVR_DEV_INFO_T &stDevice);
    AX_BOOL UpdateDevice(const AX_NVR_DEV_INFO_T &stDevice);
    AX_BOOL DeleteDevice(const vector<AX_NVR_DEV_ID> &vecDevID);
    AX_NVR_DEV_INFO_T SelectDevice(AX_NVR_DEV_ID nDeviceID);
    std::vector<AX_NVR_DEV_ID> GetFreeDevices();
    std::vector<AX_NVR_DEV_INFO_T> GetDevices();

    // preview action
    AX_BOOL UpdatePreview(const ax_nvr_channel_vector &vecChn);
    AX_BOOL StartPreview(const ax_nvr_channel_vector &vecChn);
    AX_BOOL SwitchPreviewMainSub(const ax_nvr_channel_vector &vecChn);
    // - stop all
    AX_VOID StopPreview(AX_VOID);
    // - single dev action
    AX_BOOL ZoomAndMove(AX_NVR_DEV_ID nDeviceID, const AX_NVR_RECT_T &stCropRect, AX_BOOL bCrop);

    // round-patrol action
    AX_BOOL StartRoundPatrol(const vector<AX_NVR_DEV_ID> &vecDevID);
    AX_BOOL StopRoundPatrol(const vector<AX_NVR_DEV_ID> &vecDevID);
    AX_BOOL UpdateRoundPatrolPreview(const vector<AX_NVR_DEV_ID> &vecDevID);

    // pip action
    AX_BOOL StartPip(AX_NVR_DEV_ID nDeviceID, AX_NVR_CHN_IDX_TYPE enIdx);
    AX_BOOL StopPip(AX_VOID);

    // get attr-resolution
    AX_BOOL GetResolution(AX_NVR_DEV_ID nDeviceID, AX_U32 &nWidth, AX_U32 &nHeight);

private:
    std::mutex mutex_;

    AX_BOOL m_bInit = AX_FALSE;
    AX_NVR_DEVICE_MGR_ATTR_T m_stAttr;
    CAXNVRChannel m_chnPip;

    std::map<AX_NVR_DEV_ID, AX_NVR_DEVICE_T> m_mapDevice;

protected:
    CAXNVRChannel *createRPatrolChannel(AX_NVR_DEV_ID nDevID);
    CAXNVRChannel *createPreviewChannel(AX_NVR_DEV_ID nDevID, AX_NVR_CHN_IDX_TYPE enIdx);
    AX_BOOL startRtsp(CAXNVRChannel *pChannel, const AX_NVR_DEV_CHN_INFO_T &stChannelInfo, AX_BOOL bForce, AX_S32 nCookie);

    AX_VOID StopPreviewThread(AX_VOID* pArg);
    AX_VOID StartPreviewThread(AX_VOID* pArg);

    AX_VOID StopRPatrolThread(AX_VOID* pArg);
    AX_VOID StartRPatrolThread(AX_VOID* pArg);
    AX_VOID UpdateRPatrolThread(AX_VOID* pArg);

    AX_S32 GetRTSPCookie(AX_NVR_CHN_VIEW_TYPE enView, AX_NVR_CHN_IDX_TYPE enChn, AX_NVR_DEV_ID nChn, AX_BOOL bPip = AX_FALSE);
};