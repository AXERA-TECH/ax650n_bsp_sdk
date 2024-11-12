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
#include "AXNVRFrameworkDefine.h"
#include "ax_base_type.h"

#include "detector.hpp"
#include "ivps.hpp"
#include "venc.hpp"
#include "region.hpp"
#include "rtspstream.hpp"
#include "streamContainer.hpp"
#include "streamTransfer.hpp"
#include "datastream_record.hpp"
#include "datastream_play.hpp"

#ifdef USE_FFMPEGSTREAM
    #include "ffmpegstream.hpp"
#endif

#include "vdec.hpp"
#include "vo.hpp"


enum class AX_NVR_CHN_STATE {
    IDLE        = 0,  //
    TRANSFER    = 1,  // data_transfer - preview
    STARTED     = 2,  // display - started
    PAUSED      = 3,  // display - playback paused
    STOPPED     = 4,  // display - playback stopped
};

enum class AX_NVR_CHN_SRC_TYPE {
    NONE        = 0,
    RTSP        = 1,
    FFMPEG      = 2,
    RECORD      = 3,
};


typedef struct _AX_NVR_CHN_ATTR_T {
    // preview  : device id
    // playback : vo channel id
    AX_NVR_DEV_ID nDevID = -1;
    AX_NVR_CHN_IDX_TYPE enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
    AX_NVR_CHN_VIEW_TYPE enView = AX_NVR_CHN_VIEW_TYPE::PREVIEW;
    CAXNVRDisplayCtrl *pDisplay = nullptr;

    // stream src
    AX_BOOL bPing = AX_TRUE;
    AX_NVR_CHN_SRC_TYPE enStreamSrcType = AX_NVR_CHN_SRC_TYPE::NONE;

    // record
    AX_BOOL bRecord = AX_TRUE;
    CDataStreamRecord *pRecord = nullptr;
    CDataStreamPlay *pPlayback = nullptr;
    AX_U32 nStartDate = 0;      // YYYYMMDD
    AX_U32 nStartTime = 0;      // HHMMSS
    AX_BOOL bReverse = AX_FALSE;

    // detect
    AX_NVR_DETECT_SRC_TYPE enDetectSrcType = AX_NVR_DETECT_SRC_TYPE::NONE;
    CDetector *pDetector = nullptr;
    CDetectObserver *pDetectObs = nullptr;

    // display vo,
    VO_LAYER nVoLayer = (AX_U32)-1;
    VO_CHN nVoChannel = (AX_U32)-1;

    // link channel
    AX_S32 nLinkVdecIvpsChn = 0;   // vdec chn 0
    AX_S32 nRegiVdecDeteChn = 1;   // vdec chn 1
    AX_S32 nLinkIvpsVoooChn = 0;   // ivps chn 0
    AX_S32 nRegiIvpsDeteChn = 1;   // ivps chn 1

    // VDEC
    AX_U8 nPpDepth[MAX_VDEC_CHN_NUM] = {3, 4, 3};

    // IVPS
    AX_U32 nBackupInDepth = 3;
    AX_U32 nFifoDepthForDetect = 2;
    AX_U32 nFifoDepthForVo = 0;
    AX_U32 nFrmBufNum = 3;

    AX_VOID reset(AX_VOID) {
        nDevID = -1;
        enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
        enView = AX_NVR_CHN_VIEW_TYPE::PREVIEW;
        pDisplay = nullptr;

        bPing = AX_FALSE;
        enStreamSrcType = AX_NVR_CHN_SRC_TYPE::NONE;

        bRecord = AX_TRUE;
        pRecord = nullptr;
        pPlayback = nullptr;
        nStartDate = 0;
        nStartTime = 0;
        bReverse = AX_FALSE;

        enDetectSrcType = AX_NVR_DETECT_SRC_TYPE::NONE;
        pDetector = nullptr;
        pDetectObs = nullptr;

        nVoLayer = (AX_U32)-1;
        nVoChannel = (AX_U32)-1;

        nLinkVdecIvpsChn = 0;
        nRegiVdecDeteChn = 1;
        nLinkIvpsVoooChn = 0;
        nRegiIvpsDeteChn = 1;

        nPpDepth[0] = 3;
        nPpDepth[1] = 4;
        nPpDepth[2] = 3;

        nBackupInDepth = 3;
        nFifoDepthForDetect = 2;
        nFifoDepthForVo = 0;
        nFrmBufNum = 3;
    };

    AX_VOID reset_vo(AX_VOID) {
        nVoLayer = 99;
        nVoChannel = 99;
    }

} AX_NVR_CHN_ATTR_T;

typedef struct _AX_NVR_CHN_RES_T {
    AX_U32 w = 0;
    AX_U32 h = 0;
} AX_NVR_CHN_RES_T;

class CAXNVRChannel {
public:
    static CAXNVRChannel* CreateInstance(const AX_NVR_CHN_ATTR_T& stAttr);

    CAXNVRChannel(AX_VOID) = default;
    virtual ~CAXNVRChannel(AX_VOID) = default;

    AX_BOOL Init(const AX_NVR_CHN_ATTR_T &attr);
    AX_VOID DeInit(AX_VOID);

    AX_BOOL TrySwitchMainSub1(AX_U32 nDate=0, AX_U32 nTime=0);
    AX_BOOL SwitchMainSub1(AX_U32 nDate=0, AX_U32 nTime=0);

    // const AX_NVR_CHN_ATTR_T& GetAttr(AX_VOID) const;
    // AX_VOID SetAttr(const AX_NVR_CHN_ATTR_T &attr);

    AX_BOOL StartRtsp(const std::string &strURL, AX_BOOL bRecord, AX_BOOL bForce = AX_TRUE, AX_S32 nCookie = -1);
    AX_BOOL StopRtsp(AX_BOOL bForce = AX_FALSE);

    AX_BOOL StartFile(AX_U32 nDate, AX_U32 nTime, AX_S32 nReverse = -1);
    AX_BOOL StopFile(AX_VOID);

    AX_BOOL StartDisp(VO_LAYER nVoLayer, VO_CHN nVoChannel, const AX_VO_RECT_T &stRect);
    AX_BOOL StopDisp(AX_VOID);

    AX_BOOL InitReversePlaybackModules(AX_U32 nWidth, AX_U32 nHeight, AX_F32 fFps, AX_U32 nGop, AX_PAYLOAD_TYPE_E enPayload);
    AX_BOOL DeInitReversePlaybackModules();
    AX_BOOL StartReversePlaybackLinkages();
    AX_BOOL StopReversePlaybackLinkages();

    //
    AX_BOOL EnableDetect(AX_BOOL bEnable) {return AX_TRUE;};
    AX_BOOL EnableRecord(AX_BOOL bEnable) {return AX_TRUE;};

    //
    AX_BOOL PauseResume(AX_BOOL bForceResume = AX_FALSE);
    AX_BOOL Step(AX_BOOL bReverse);
    AX_BOOL Hide(AX_VOID) {return AX_TRUE;};
    AX_BOOL Show(AX_VOID) {return AX_TRUE;};

    //
    AX_BOOL UpdateRPatrolRect(const AX_VO_RECT_T &stRect);
    AX_BOOL UpdateFps(AX_F32 fFactor);
    AX_BOOL UpdateRect(const AX_VO_RECT_T &stRect);
    AX_BOOL Crop(const AX_VO_RECT_T &stRect, const AX_IVPS_RECT_T &stCropRect, AX_BOOL bCrop);

    //
    AX_NVR_CHN_RES_T GetResolution(AX_VOID);
    VO_CHN GetCurrentVoChn(AX_VOID);
    std::pair<AX_U32, AX_U32> GetCurrentDateTime(AX_VOID);

    CStream0 &GetStream(AX_VOID);

    const AX_NVR_CHN_STATE& GetCurrentState(AX_VOID);

protected:
    std::mutex mutex_;

    AX_NVR_CHN_ATTR_T m_stAttr;
    AX_NVR_CHN_STATE m_enState = AX_NVR_CHN_STATE::IDLE;

    AX_MOD_INFO_T m_stModeInfoVdec;
    AX_MOD_INFO_T m_stModeInfoVdec2;
    AX_MOD_INFO_T m_stModeInfoIvps;
    AX_MOD_INFO_T m_stModeInfoIvps2;
    AX_MOD_INFO_T m_stModeInfoVenc;
    AX_MOD_INFO_T m_stModeInfoVo;

    CRtspStream m_objRtspStream;
    CFFMpegStream m_objFFMpegStream;
    CVDEC m_objVdec;
    CVDEC m_objVdec2;
    CIVPS m_objIvps;
    CIVPS m_objIvps2;
    CVENC m_objVenc;
    CVideoStreamTransfer m_objStreamTrans;
    CRegion m_objRegion;
    CRegionObserver m_objRgnObs;
    CDataStreamObserver m_objRecordObs;

private:
    AX_BOOL pause_resume(AX_BOOL bForceResume = AX_FALSE);
    AX_VOID set_state(AX_NVR_CHN_STATE enStatus);
    AX_BOOL set_fps(VO_LAYER nVoLayer, VO_CHN nVoChannel, AX_F32 fFps, AX_F32 fFactor = 1.0f);
    AX_BOOL clean(VO_LAYER nVoLayer, VO_CHN nVoChannel);
    AX_BOOL init_rtsp(const std::string &strURL, AX_S32 nCookie);
    AX_BOOL init_ffmpeg(AX_VOID);
    AX_BOOL init_vdec(AX_PAYLOAD_TYPE_E enPayload, AX_U32 nWidth, AX_U32 nHeight, const AX_VO_SIZE_T &voSize, AX_VDEC_GRP vdGrp = INVALID_VDEC_GRP);
    AX_BOOL init_vdec2(AX_PAYLOAD_TYPE_E enPayload, AX_U32 nWidth, AX_U32 nHeight, const AX_VO_SIZE_T &voSize, AX_VDEC_GRP vdGrp = INVALID_VDEC_GRP);
    AX_BOOL init_venc(AX_PAYLOAD_TYPE_E enPayload, AX_U32 nWidth, AX_U32 nHeight, AX_U32 nFPS, VENC_GRP veGrp = INVALID_VENC_GRP);
    AX_BOOL init_ivps(AX_U32 nSrcW, AX_U32 nSrcH, const AX_VO_RECT_T &voChnWin, AX_IVPS_GRP ivGrp = INVALID_IVPS_GRP);
    AX_BOOL init_ivps2(AX_U32 nSrcW, AX_U32 nSrcH, AX_IVPS_GRP ivGrp = INVALID_IVPS_GRP);
    AX_BOOL init_transfer(AX_U32 nFps, AX_U32 nGop, VENC_GRP veGrp = INVALID_VENC_GRP);
    AX_BOOL init_region(AX_S32 nGrp, AX_U32 nSrcW, AX_U32 nSrcH, const AX_VO_RECT_T &voChnWin);
};
