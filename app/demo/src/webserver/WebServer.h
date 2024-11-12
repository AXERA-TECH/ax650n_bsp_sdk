
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

#include <map>
#include <mutex>
#include <thread>
#include "AXRingBuffer.h"
#include "AXSingleton.h"
#include "AppLogApi.h"
#include "EncoderOptionHelper.h"
#include "IModule.h"
#include "JpegEncoder.h"

#define MAX_WS_CONN_NUM (9)
#define AX_WEB_VENC_RING_BUFF_COUNT (5)
#define AX_WEB_JENC_RING_BUFF_COUNT (10)
#define AX_WEB_MJENC_RING_BUFF_COUNT (10)
#define AX_WEB_EVENTS_RING_BUFF_COUNT (5)
#define AX_WEB_AENC_RING_BUFF_COUNT (10)

#define WS_EVENTS_CHANNEL (MAX_WS_CONN_NUM - 1)
#define MAX_EVENTS_CHN_SIZE (256)

typedef enum {
    JPEG_TYPE_BODY = 0,
    JPEG_TYPE_VEHICLE,
    JPEG_TYPE_CYCLE,
    JPEG_TYPE_FACE,
    JPEG_TYPE_PLATE,
    JPEG_TYPE_CAPTURE,
    JPE_TYPE_FLASH,
    JPEG_TYPE_BUTT
} JPEG_TYPE_E;

typedef enum {
    E_REQ_TYPE_CAMERA = 0,
    E_REQ_TYPE_AUDIO,
    E_REQ_TYPE_VIDEO,
    E_REQ_TYPE_AI,
    E_REQ_TYPE_GET_SYSTEM_INFO,
    E_REQ_TYPE_GET_ASSIST_INFO,
    E_REQ_TYPE_OSD,
    E_REQ_TYPE_CAPTURE,
    E_REQ_TYPE_TRIGGER,
    E_REQ_TYPE_IMAGE,
    E_REQ_TYPE_SWITCH_3A_SYNCRATIO,
    E_REQ_TYPE_MAX,
} WEB_REQUEST_TYPE_E;

typedef enum {
    E_WS_CHANNEL_TYPE_VENC = 0,
    E_WS_CHANNEL_TYPE_JENC,
    E_WS_CHANNEL_TYPE_EVENTS,
    E_WS_CHANNEL_TYPE_CAPTURE,
    E_WS_CHANNEL_TYPE_MAX
} WS_CHANNEL_TYPE_E;

typedef enum {
    OPR_TARGET_MODULE_SNS_MGR = 0,
    OPR_TARGET_MODULE_IVPS,
    OPR_TARGET_MODULE_VENC,
    OPR_TARGET_MODULE_JENC,
    OPR_TARGET_MODULE_MAX
} OPR_TARGET_MODULE_E;

typedef enum {
    E_WEB_OPERATION_TYPE_ROTATION = 0,
    E_WEB_OPERATION_TYPE_SNS_MODE,
    E_WEB_OPERATION_TYPE_RESOLUTION,
    E_WEB_OPERATION_TYPE_ENC_TYPE,
    E_WEB_OPERATION_TYPE_BITRATE,
    E_WEB_OPERATION_TYPE_RC_INFO,
    E_WEB_OPERATION_TYPE_DAYNIGHT,
    E_WEB_OPERATION_TYPE_NR_MODE,
    E_WEB_OPERATION_TYPE_CAPTURE_AUTO,
    E_WEB_OPERATION_TYPE_AI_ENABLE,
    E_WEB_OPERATION_TYPE_AI_PUSH_MODE,
    E_WEB_OPERATION_TYPE_AI_EVENT,
    E_WEB_OPERATION_TYPE_OSD_ENABLE,
    E_WEB_OPERATION_TYPE_OSD_ATTR,
    E_WEB_OPERATION_TYPE_CHANNEL_SWITCH,
    E_WEB_OPERATION_TYPE_GET_RESOLUTION,
    E_WEB_OPERATION_TYPE_GET_TITLE,
    E_WEB_OPERATION_TYPE_VENC_LINK_ENABLE,
    E_WEB_OPERATION_TYPE_IMAGE_ATTR,
    E_WEB_OPERATION_TYPE_CAPTURE,
    E_WEB_OPERATION_TYPE_CAMERA_FPS,
    E_WEB_OPERATION_TYPE_AUDIO_ATTR,
    E_WEB_OPERATION_TYPE_TRIGGER,
    E_WEB_OPERATION_TYPE_SNS_MIRROR_FLIP,
    E_WEB_OPERATION_TYPE_LDC_ATTR,
    E_WEB_OPERATION_TYPE_SWITCH_3A_SYNCRATIO,
    E_WEB_OPERATION_TYPE_MAX
} WEB_OPERATION_TYPE_E;

typedef struct {
    AX_U8 nSnsSrc;
    AX_U8 nChannel;
    AX_U32 nWidth;
    AX_U32 nHeight;
} JPEG_HEAD_INFO_T;

typedef struct _JPEG_CAPTURE_INFO_T {
    JPEG_HEAD_INFO_T tHeaderInfo;
    AX_VOID* pData;

    _JPEG_CAPTURE_INFO_T() {
        memset(this, 0, sizeof(_JPEG_CAPTURE_INFO_T));
    }
} JPEG_CAPTURE_INFO_T;

typedef struct {
    AX_U8 nGender; /* 0-female, 1-male */
    AX_U8 nAge;
    AX_CHAR szMask[32];
    AX_CHAR szInfo[32];
} JPEG_FACE_INFO_T;

typedef struct {
    AX_CHAR szNum[16];
    AX_CHAR szColor[32];
} JPEG_PLATE_INFO_T;

typedef struct _JPEG_DATA_INFO_T {
    JPEG_TYPE_E eType; /* JPEG_TYPE_E */
    union {
        JPEG_CAPTURE_INFO_T tCaptureInfo;
        JPEG_FACE_INFO_T tFaceInfo;
        JPEG_PLATE_INFO_T tPlateInfo;
    };

    _JPEG_DATA_INFO_T() {
        eType = JPEG_TYPE_BUTT;
    }
} JPEG_DATA_INFO_T;

typedef struct _JpegHead {
    AX_U32 nMagic;
    AX_U32 nTotalLen;
    AX_U32 nTag;
    AX_U32 nJsonLen;
    AX_CHAR szJsonData[256];

    _JpegHead() {
        nMagic = 0x54495841;  // "AXIT" by default
        nTotalLen = 0;
        nTag = 0x4E4F534A;    // "JSON" by default
        nJsonLen = 0;
        memset(szJsonData, 0x0, sizeof(szJsonData));
    }

} JpegHead;

typedef struct _WEB_OPR_ROTATION {
    AX_U8 nRotation;

    _WEB_OPR_ROTATION() {
        nRotation = 0;
    }
} WEB_OPR_ROTATION_T;

typedef struct _WEB_OPR_SNS_MODE {
    AX_U8 nSnsMode;

    _WEB_OPR_SNS_MODE() {
        nSnsMode = 0;
    }
} WEB_OPR_SNS_MODE_T;

typedef struct _WEB_OPR_RESOLUTION {
    AX_U8 nEncoderType; /* 0: H264; 1: MJPEG; 2: H265 */
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_BOOL bFirst;
    AX_BOOL bLast;

    _WEB_OPR_RESOLUTION() {
        nEncoderType = 0;
        nWidth = 0;
        nHeight = 0;
    }
} WEB_OPR_RESOLUTION_T;

typedef struct _WEB_OPR_AUDIO {
    AX_U8 nCapture_volume{0};
    AX_U8 nPlay_volume{0};
} WEB_OPR_AUDIO_ATTR_T;

typedef struct _WEB_OPR_ENCODER_TYPE {
    AX_U32 nEncoderType;

    _WEB_OPR_ENCODER_TYPE() {
        nEncoderType = 0;
    }
} WEB_OPR_ENCODER_TYPE_T;

typedef struct _WEB_OPR_BITRATE {
    AX_U8 nEncoderType; /* 0: H264; 1: MJPEG; 2: H265 */
    AX_U32 nBitrate;

    _WEB_OPR_BITRATE() {
        nEncoderType = 0;
        nBitrate = 0;
    }
} WEB_OPR_BITRATE_T;

typedef struct _WEB_OPR_DAYNIGHT {
    AX_U32 nDayNightMode;

    _WEB_OPR_DAYNIGHT() {
        nDayNightMode = 0;
    }
} WEB_OPR_DAYNIGHT_T;

typedef struct _WEB_OPR_CAP_SWITCH {
    AX_BOOL bOn;

    _WEB_OPR_CAP_SWITCH() {
        bOn = AX_TRUE;
    }
} WEB_OPR_CAP_SWITCH_T;

typedef struct _WEB_OPR_AI_SWITCH {
    AX_BOOL bOn;

    _WEB_OPR_AI_SWITCH() {
        bOn = AX_TRUE;
    }
} WEB_OPR_AI_SWITCH_T;

typedef enum {
    E_AI_DETECT_PUSH_MODE_TYPE_FAST = 1,
    E_AI_DETECT_PUSH_MODE_TYPE_INTERVAL,
    E_AI_DETECT_PUSH_MODE_TYPE_BEST
} E_AI_DETECT_PUSH_MODE_TYPE;

typedef struct _WEB_OPR_AI_PUSH_STATEGY {
    E_AI_DETECT_PUSH_MODE_TYPE ePushMode;
    AX_U32 nPushCounts{1};
    AX_U32 nPushIntervalMs{1};
    _WEB_OPR_AI_PUSH_STATEGY() {
        ePushMode = E_AI_DETECT_PUSH_MODE_TYPE_BEST;
    }
} WEB_OPR_AI_PUSH_STATEGY, AI_PUSH_STATEGY_T;

typedef struct _WEB_OPR_EVENTS_DETECT_ATTR_T {
    AX_BOOL bEnable{AX_FALSE};
    AX_U16 nThrsHoldY{0};
    AX_U16 nConfidence{0};

    _WEB_OPR_EVENTS_DETECT_ATTR_T() = default;
} WEB_OPR_EVENTS_DETECT_ATTR_T, *WEB_OPR_EVENTS_DETECT_ATTR_PTR;

typedef struct _WEB_OPR_AI_EVENTS_T {
    WEB_OPR_EVENTS_DETECT_ATTR_T tMD;
    WEB_OPR_EVENTS_DETECT_ATTR_T tOD;
    WEB_OPR_EVENTS_DETECT_ATTR_T tSCD;

    _WEB_OPR_AI_EVENTS_T() {
        tMD.bEnable = AX_FALSE;
        tMD.nConfidence = 1;
        tMD.nThrsHoldY = 1;
        tOD.bEnable = AX_FALSE;
        tOD.nThrsHoldY = 1;
        tOD.nConfidence = 1;
        tSCD.bEnable = AX_FALSE;
        tSCD.nThrsHoldY = 1;
        tSCD.nConfidence = 1;
    }
} WEB_OPR_AI_EVENTS_T, AI_EVENTS_OPTION_T, *WEB_OPR_AI_EVENTS_PTR;

typedef struct _WEB_OPR_OSD_SWITCH {
    AX_BOOL bOn;

    _WEB_OPR_OSD_SWITCH() {
        bOn = AX_TRUE;
    }
} WEB_OPR_OSD_SWITCH_T;

typedef struct _WEB_OPR_NR {
    AX_U32 nNRMode;

    _WEB_OPR_NR() {
        nNRMode = 0;
    }
} WEB_OPR_NR_T;

typedef struct _WEB_OPR_CHN_SWITCH {
    AX_BOOL bOn;
    AX_U8 nEncoderType; /* 0: H264; 1: MJPEG; 2: H265 */

    _WEB_OPR_CHN_SWITCH() {
        bOn = AX_TRUE;
        nEncoderType = 0;
    }
} WEB_OPR_CHN_SWITCH_T;

typedef struct _WEB_OPR_GET_RESOLUTION {
    AX_U32 nWidth;
    AX_U32 nHeight;

    _WEB_OPR_GET_RESOLUTION() {
        nWidth = 0;
        nHeight = 0;
    }
} WEB_OPR_GET_RESOLUTION_T;

// typedef struct _WEB_OPR_RC_INFO {
//     AX_U8 nRcType; /* 0: CBR; 1: VBR; 2: FIXQP */
//     AX_U8 nMaxIProp;
//     AX_U8 nMinIProp;
//     AX_U8 nEncoderType; /* 0: H264; 1: MJPEG; 2: H265 */
// }
typedef struct _WEB_OPR_RC_INFO : public RC_INFO_T {
    AX_U8 nEncoderType{0}; /* 0: H264; 1: MJPEG; 2: H265 */
} WEB_OPR_RC_INFO_T;

typedef struct _WEB_OPR_VENC_LINK_T {
    AX_BOOL bLinkEnable{AX_FALSE};
} WEB_OPR_VENC_LINK_T;

typedef struct _WEB_OPR_IMAGE_ATTR_T {
    AX_U8 nAutoMode{0}; /*1:Auto; 0:Manual*/
    AX_U8 nSharpness{0};
    AX_U8 nBrightness{0};
    AX_U8 nContrast{0};
    AX_U8 nSaturation{0};
} WEB_OPR_IMAGE_ATTR_T;

typedef struct _WEB_OPR_LDC_ATTR_T {
    AX_BOOL bLdcEnable{AX_FALSE};
    AX_BOOL bLdcAspect{AX_FALSE};
    AX_S16 nXRatio{0};
    AX_S16 nYRatio{0};
    AX_S16 nXYRatio{0};
    AX_S16 nDistorRatio{0};
} WEB_OPR_LDC_ATTR_T;

typedef struct _WEB_OPR_SNS_FPS_ATTR_T {
    AX_F32 fSnsFrameRate{0};
} WEB_OPR_SNS_FPS_ATTR_T;

typedef struct _WEB_OPR_SNS_MIRROR_FLIP_ATTR_T {
    AX_BOOL bMirror{AX_FALSE};
    AX_BOOL bFlip{AX_FALSE};
} WEB_OPR_SNS_MIRROR_FLIP_ATTR_T;

typedef struct _WEB_REQ_OPERATION_T {
    AX_U8 nGroup;   /* Target group (-1 means all groups affected) */
    AX_U8 nSnsID;
    AX_U8 nChannel; /* Target channel (-1 means all channels affected) */
    WEB_REQUEST_TYPE_E eReqType;
    AX_S32 nIntervalMs;
    AX_U16 nPriority; /* Bigger value means higher priority */
    AX_BOOL b3ASyncRationOn;
    union {
        WEB_OPR_ROTATION_T tRotation;
        WEB_OPR_SNS_MODE_T tSnsMode;
        WEB_OPR_RESOLUTION_T tResolution;
        WEB_OPR_ENCODER_TYPE_T tEncType;
        WEB_OPR_BITRATE_T tBitrate;
        WEB_OPR_RC_INFO_T tRcInfo;
        WEB_OPR_DAYNIGHT_T tDaynight;
        WEB_OPR_NR_T tNR;
        WEB_OPR_CAP_SWITCH_T tCapEnable;
        WEB_OPR_AI_SWITCH_T tAiEnable;
        WEB_OPR_AI_PUSH_STATEGY tAiPushStategy;
        WEB_OPR_AI_EVENTS_T tEvent;
        WEB_OPR_OSD_SWITCH_T tOsdEnable;
        WEB_OPR_CHN_SWITCH_T tChnSwitch;
        WEB_OPR_GET_RESOLUTION_T tGetResolution;
        WEB_OPR_VENC_LINK_T tVencLinkEnable;
        WEB_OPR_IMAGE_ATTR_T tImageAttr;
        WEB_OPR_LDC_ATTR_T tLdcAttr;
        WEB_OPR_SNS_FPS_ATTR_T tSnsFpsAttr;
        WEB_OPR_AUDIO_ATTR_T tAudioAttr;
        WEB_OPR_SNS_MIRROR_FLIP_ATTR_T tSnsMirrorFlip;
    };
    _WEB_REQ_OPERATION_T() {
        nGroup = -1;
        nChannel = -1;
        eReqType = E_REQ_TYPE_MAX;
        eOperationType = E_WEB_OPERATION_TYPE_MAX;
        nIntervalMs = -1;
        b3ASyncRationOn = AX_TRUE;
    }

    AX_VOID SetOperaType(WEB_OPERATION_TYPE_E type) {
        eOperationType = type;
        switch (eOperationType) {
            case E_WEB_OPERATION_TYPE_ROTATION: {
                nPriority = 1;
                break;
            }
            case E_WEB_OPERATION_TYPE_SNS_MODE: {
                nPriority = 9;
                break;
            }
            case E_WEB_OPERATION_TYPE_ENC_TYPE: {
                nPriority = 5;
                break;
            }
            case E_WEB_OPERATION_TYPE_BITRATE: {
                nPriority = 3;
                break;
            }
            case E_WEB_OPERATION_TYPE_RC_INFO: {
                nPriority = 2;
                break;
            }
            case E_WEB_OPERATION_TYPE_CAMERA_FPS: {
                nPriority = 1;
                break;
            }
            default:
                nPriority = 0;
                break;
        }
    }
    WEB_OPERATION_TYPE_E GetOperationType() {
        return eOperationType;
    }

private:
    WEB_OPERATION_TYPE_E eOperationType;

} WEB_REQ_OPERATION_T;

typedef enum _WEB_EVENTS_TYPE_E {
    E_WEB_EVENTS_TYPE_ReStartPreview = 0,
    E_WEB_EVENTS_TYPE_MD,
    E_WEB_EVENTS_TYPE_OD,
    E_WEB_EVENTS_TYPE_SCD,
    E_WEB_EVENTS_TYPE_LogOut,
    E_WEB_EVENTS_TYPE_MAX
} WEB_EVENTS_TYPE_E;

typedef struct _AI_EVENTS_MD_INFO_T {
    AX_CHAR szDisplay[64];
    AX_U32 nAreaID;
    AX_U64 nReserved;
    _AI_EVENTS_MD_INFO_T() {
        memset(szDisplay, 0, sizeof(szDisplay));
        nAreaID = 0;
        nReserved = 0;
    }
} AI_EVENTS_MD_INFO_T;

typedef struct _AI_EVENTS_OD_INFO_T {
    AX_CHAR szDisplay[64];
    AX_U32 nAreaID;
    AX_U64 nReserved;
    _AI_EVENTS_OD_INFO_T() {
        memset(szDisplay, 0, sizeof(szDisplay));
        nAreaID = 0;
        nReserved = 0;
    }
} AI_EVENTS_OD_INFO_T;

typedef struct _AI_EVENTS_SCD_INFO_T {
    AX_CHAR szDisplay[64];
    AX_U32 nAreaID;
    AX_U64 nReserved;
    _AI_EVENTS_SCD_INFO_T() {
        memset(szDisplay, 0, sizeof(szDisplay));
        nAreaID = 0;
        nReserved = 0;
    }
} AI_EVENTS_SCD_INFO_T;

typedef struct _WEB_EVENTS_DATA {
    WEB_EVENTS_TYPE_E eType;
    AX_U64 nTime;
    AX_U32 nReserved;
    union {
        AI_EVENTS_MD_INFO_T tMD;
        AI_EVENTS_OD_INFO_T tOD;
        AI_EVENTS_SCD_INFO_T tSCD;
    };
    _WEB_EVENTS_DATA() {
        eType = E_WEB_EVENTS_TYPE_MAX;
        nTime = 0;
        nReserved = 0;
    }
} WEB_EVENTS_DATA_T, *WEB_EVENTS_DATA_PTR;

class IPPLBuilder;
class CWebServer : public CAXSingleton<CWebServer>, public IModule {
    friend class CAXSingleton<CWebServer>;

public:
    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL Stop() override;

public:
    AX_VOID BindPPL(IPPLBuilder* pPPLBuilder) {
        m_pPPLBuilder = pPPLBuilder;
    };
    AX_VOID SendPreviewData(AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_FALSE);
    AX_VOID SendPushImgData(AX_U8 nSnsID, AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_TRUE,
                            JPEG_DATA_INFO_T* pJpegInfo = nullptr);
    AX_VOID SendCaptureData(AX_U8 nSnsID, AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_TRUE,
                            JPEG_DATA_INFO_T* pJpegInfo = nullptr);
    AX_VOID SendSnapshotData(AX_VOID* data, AX_U32 size);
    AX_BOOL SendEventsData(WEB_EVENTS_DATA_T* data);
    AX_VOID SendAudioData(AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts);

    AX_BOOL RequestRingbuf(AX_U32 nUniChn, AX_U32 nElementBuffSize, AX_U32 nBuffCount, std::string strName);
    AX_U8 RegistPreviewChnMappingInOrder(AX_U8 nSnsID, AX_U8 nUniChn, AX_U8 nType);
    AX_VOID UpdateMediaTypeInPreviewChnMap(AX_U8 nSnsID, AX_U8 nUniChn, AX_U8 nType);
    AX_VOID RegistUniCaptureChn(AX_S8 nCaptureChn, JPEG_TYPE_E eType = JPEG_TYPE_CAPTURE);

    AX_VOID UpdateConnStatus(AX_VOID);

    AX_VOID EnableCaptrue(AX_U8 nSnsID, AX_BOOL bEnable = AX_TRUE) {
        m_arrCaptureEnable[nSnsID] = bEnable;
    }

    AX_VOID EnableAudioPlay(AX_BOOL bEnable = AX_TRUE) {
        m_bAudioPlayAvailable = bEnable;
    }

    AX_VOID EnableAudioCapture(AX_BOOL bEnable = AX_TRUE) {
        m_bAudioCaptureAvailable = bEnable;
    }

    IPPLBuilder* GetPPLInstance() {
        return m_pPPLBuilder;
    }

    AX_S8 GetCaptureChannel() {
        return m_nCaptureChannel;
    }

    AX_S8 GetSnapshotChannel() {
        return m_nSnapshotChannel;
    }

    AX_S8 GetAencChannel() {
        return m_nAencChannel;
    }

    AX_S8 GetTalkChannel() {
        return m_nTalkChannel;
    }

private:
    CWebServer(AX_VOID);
    ~CWebServer(AX_VOID);

    AX_VOID RestartPreview();
    AX_VOID LogOut();
    AX_VOID SendWSData(AX_VOID);
    AX_VOID RecvWSData(AX_VOID);

    static AX_VOID* WebServerThreadFunc(AX_VOID* pThis);
    static AX_VOID* SendDataThreadFunc(AX_VOID* pThis);
    static AX_VOID* RecvDataThreadFunc(AX_VOID* pThis);

    std::string FormatIVESEventsJson(WEB_EVENTS_DATA_T* pEvent);
    std::string FormatPreviewEventsJson(WEB_EVENTS_DATA_T* pEvent);

private:
    typedef struct _WS_CHANNEL_DATA_T {
        CAXRingBuffer* pRingBuffer;
        AX_U8 nChannel;
        AX_U8 nInnerIndex;
        _WS_CHANNEL_DATA_T() {
            memset(this, 0, sizeof(_WS_CHANNEL_DATA_T));
        }
    } WS_CHANNEL_DATA_T;

    AX_BOOL m_bServerStarted{AX_FALSE};
    AX_BOOL m_bAudioCaptureAvailable{AX_FALSE};
    WS_CHANNEL_DATA_T m_arrChannelData[MAX_WS_CONN_NUM];
    AX_BOOL m_arrConnStatus[MAX_WS_CONN_NUM]{AX_FALSE};
    AX_BOOL m_arrCaptureEnable[2]{AX_TRUE, AX_TRUE};

    IPPLBuilder* m_pPPLBuilder{nullptr};
    std::thread* m_pAppwebThread{nullptr};
    std::thread* m_pSendDataThread{nullptr};
    std::thread* m_pRecvDataThread{nullptr};
    std::mutex m_mtxConnStatus;

    AX_S8 m_nCaptureChannel{-1};
    AX_S8 m_nSnapshotChannel{(MAX_WS_CONN_NUM + 1)};
    AX_S8 m_nAencChannel{(MAX_WS_CONN_NUM - 2)};
    AX_S8 m_nTalkChannel{(MAX_WS_CONN_NUM + 2)};
    AX_BOOL m_bAudioPlayAvailable{AX_FALSE};
    std::map<AX_U8, JPEG_TYPE_E> m_mapJencChnType;
};