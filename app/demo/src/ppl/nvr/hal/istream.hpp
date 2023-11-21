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
#include <string.h>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include "haltype.hpp"
#include "nalu.hpp"

typedef enum { RTP_OVER_UDP = 0, RTP_OVER_TCP = 1 } RTP_TRANSPORT_MODE_E;
typedef enum { DISCONNECT = 0, RECONNECT = 1, CONNECTED = 2 } CONNECT_STATUS_E;

typedef struct STREAM_ATTR_S {
    std::string strURL;
    AX_U32 nMaxBufSize;
    RTP_TRANSPORT_MODE_E enTransportMode;
    AX_U32 nDebugLevel;
    AX_U32 nKeepAliveInterval;  /* heart beat interval time, unit is seconds */
    AX_U32 nReconnectThreshold; /* if the count of heart beat absent from server is >= nReconnThreshold, trigger to reconnect */
    AX_S32 nFps;                /* ONLY for ffmpeg stream, < 0: no fps ctrl, 0: auto detect fps from stream, > 0: specified fps */
    STREAM_ATTR_S(AX_VOID) {
        nMaxBufSize = 1920 * 1080;
        enTransportMode = RTP_OVER_UDP;
        nDebugLevel = 0;
        nKeepAliveInterval = 10;
        nReconnectThreshold = 3;
        nFps = 0;
    }
} STREAM_ATTR_T;

typedef struct {
    AX_PAYLOAD_TYPE_E enPayload;
    AX_U32 nProfile;
    AX_U32 nLevel; /* level_idc */
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nNumRefs; /* num_ref_frames */
    AX_U32 nFps;
} STREAM_VIDEO_TRACK_INFO_T;

typedef struct {
    AX_PAYLOAD_TYPE_E enPayload;
    AX_U32 nBps;
    AX_U32 nSampleRate;
    AX_U32 nSampleBit;
} STREAM_AUDIO_TRACK_INFO_T;

typedef struct {
    STREAM_VIDEO_TRACK_INFO_T stInfo;
    STREAM_NALU_TYPE_E enNalu;
    AX_U8* pData;
    AX_U32 nLen;
    AX_BOOL bSkipDisplay; /* not support yet */
    AX_U64 nPTS;
    AX_U64 nSequence;
    AX_U64 nFramePts;
} STREAM_VIDEO_FRAME_T;

typedef struct {
    AX_PAYLOAD_TYPE_E enPayload;
    union {
        STREAM_VIDEO_TRACK_INFO_T stVideo;
        STREAM_AUDIO_TRACK_INFO_T stAudio;
    } info;
} STREAM_TRACK_INFO_T;

typedef struct {
    std::string strURL;
    STREAM_VIDEO_TRACK_INFO_T stVideo;
    std::unordered_map<AX_VOID*, STREAM_TRACK_INFO_T> tracks;
} STREAM_INFO_T;

typedef struct {
    STREAM_AUDIO_TRACK_INFO_T stInfo;
    AX_U8* pData;
    AX_U32 nLen;
    AX_U64 nPTS;
} STREAM_AUDIO_FRAME_T;

typedef struct {
    AX_PAYLOAD_TYPE_E enPayload;
    union {
        STREAM_VIDEO_FRAME_T stVideo;
        STREAM_AUDIO_FRAME_T stAudio;
    } frame;
    AX_U64 nPrivData;
} STREAM_FRAME_T;

typedef struct STREAM_STAT_S {
    AX_S32 nState;    /* 0: idle 1: running */
    AX_U64 nCount[2]; /* 0: video, 1: audio */
    STREAM_STAT_S(AX_VOID) {
        Reset();
    }
    AX_VOID Reset(AX_VOID) noexcept {
        memset(this, 0, sizeof(*this));
    }
} STREAM_STAT_T;

class IStreamObserver {
public:
    virtual ~IStreamObserver(AX_VOID) = default;
    virtual AX_BOOL OnRecvStreamData(CONST STREAM_FRAME_T& stFrame) = 0;
    virtual AX_BOOL OnRecvStreamInfo(CONST STREAM_INFO_T& stInfo) = 0;
    virtual AX_VOID OnNotifyConnStatus(CONST AX_CHAR* pUrl, CONNECT_STATUS_E enStatus) = 0;
};

class IStream {
public:
    virtual ~IStream(AX_VOID) = default;

    virtual AX_BOOL RegisterObserver(IStreamObserver* pObs) = 0;
    virtual AX_BOOL UnRegisterObserver(IStreamObserver* pObs) = 0;

    virtual AX_BOOL Init(CONST STREAM_ATTR_T& stAttr) = 0;
    virtual AX_BOOL DeInit(AX_VOID) = 0;

    virtual AX_BOOL Start(AX_VOID) = 0;
    virtual AX_BOOL Stop(AX_VOID) = 0;

    virtual CONST STREAM_STAT_T& QueryStatus(AX_VOID) CONST = 0;
    virtual CONST STREAM_INFO_T& GetStreamInfo(AX_VOID) CONST = 0;
};

class CStream0 : public IStream {
public:
    CStream0(AX_VOID) = default;

    AX_BOOL RegisterObserver(IStreamObserver* pObs) override;
    AX_BOOL UnRegisterObserver(IStreamObserver* pObs) override;

    CONST STREAM_INFO_T& GetStreamInfo(AX_VOID) CONST override;
    CONST STREAM_STAT_T& QueryStatus(AX_VOID) CONST override;

protected:
    STREAM_ATTR_T m_stAttr;
    STREAM_INFO_T m_stInfo;
    STREAM_STAT_T m_stStat;
    std::list<IStreamObserver*> m_lstObs;
    std::mutex m_mtxObs;
};
