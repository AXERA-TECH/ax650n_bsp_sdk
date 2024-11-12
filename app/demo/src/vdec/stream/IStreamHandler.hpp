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
#include <memory>
#include <mutex>
#include <string>
#include "ax_global_type.h"

enum class STREAM_TYPE_E { FILE = 0, RTSP = 1 };

typedef struct STREAMER_INFO_S {
    std::string strPath;
    STREAM_TYPE_E eStreamType;
    AX_PAYLOAD_TYPE_E eVideoType;
    AX_PAYLOAD_TYPE_E eAudioType;
    AX_S32 nCookie;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nFps;
    AX_VOID *session;

    STREAMER_INFO_S(AX_VOID) {
        eStreamType = STREAM_TYPE_E::FILE;
        eVideoType = PT_H264;
        eAudioType = PT_G711A;
        nCookie = 0;
        nWidth = 0;
        nHeight = 0;
        nFps = 0;
        session = nullptr;
    }
} STREAMER_INFO_T;

typedef struct STREAMER_ATTR_S {
    std::string strPath; /* stream path for RTSP URL or file */
    AX_U32 nMaxWidth;    /* maximum width */
    AX_U32 nMaxHeight;   /* maximum height */
    AX_U32 nCacheTime;   /* cache time in millseconds */
    AX_BOOL bLoop;       /* only available for file, loop to playback */
    AX_BOOL bSyncObs;    /* exit sending stream to next observers when one observer returns fail */
    AX_S32 nCookie;      /* stream cookie, set equal to vdGrp of VDEC */

    AX_U32 nForceFps; /* mandatory fps */
    AX_S32 nMaxSendNaluIntervalMilliseconds; /* UT: > 0, send nalu to VDEC interval for [0, nMaxSendNaluIntervalMilliseconds] */

    STREAMER_ATTR_S(AX_VOID) {
        bLoop = AX_TRUE;
        bSyncObs = AX_FALSE;
        nMaxWidth = 0;
        nMaxHeight = 0;
        nCacheTime = 0;
        nCookie = 0;

        nForceFps = 0;
        nMaxSendNaluIntervalMilliseconds = 0;
    }
} STREAMER_ATTR_T;

typedef struct STREAMER_STAT_S {
    AX_BOOL bStarted;
    AX_U64 nCount; /* not used */

    STREAMER_STAT_S(AX_VOID) {
        bStarted = AX_FALSE;
        nCount = 0;
    }
} STREAMER_STAT_T;

/**
 * @brief streamer observer
 *
 */
class IStreamObserver {
public:
    virtual ~IStreamObserver(AX_VOID) = default;
    virtual AX_BOOL OnRecvVideoData(AX_S32 nCookie, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) = 0;
    virtual AX_BOOL OnRecvAudioData(AX_S32 nCookie, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) = 0;
};

/**
 * @brief
 *
 *  API invoke flow:
 *      Init -> GetStreamInfo -> CreateDecoder -> RegObserver -> Start -> Stop -> UnRegObserver
 *
 */
class IStreamHandler {
public:
    virtual ~IStreamHandler(AX_VOID) = default;

    virtual AX_BOOL Init(const STREAMER_ATTR_T&) = 0;
    virtual AX_BOOL DeInit(AX_VOID) = 0;

    /* stream information could be feteched after Init */
    virtual const STREAMER_INFO_T& GetStreamInfo(AX_VOID) = 0;

    /* observer should be registed before Start */
    virtual AX_BOOL RegObserver(IStreamObserver*) = 0;
    virtual AX_BOOL UnRegObserver(IStreamObserver*) = 0;

    virtual AX_BOOL Start(AX_VOID) = 0;
    virtual AX_BOOL Stop(AX_VOID) = 0;

    virtual AX_BOOL QueryStatus(STREAMER_STAT_T&) = 0;
};

using IStreamerHandlerPtr = std::unique_ptr<IStreamHandler>;

class CStreamBaseHander : public IStreamHandler {
public:
    CStreamBaseHander(AX_VOID) = default;
    virtual ~CStreamBaseHander(AX_VOID) = default;

    AX_BOOL QueryStatus(STREAMER_STAT_T& stStat) override {
        std::lock_guard<std::mutex> lck(m_mtxStat);
        stStat = m_stStat;
        return AX_TRUE;
    }

    const STREAMER_INFO_T& GetStreamInfo(AX_VOID) override {
        return m_stInfo;
    }

    AX_BOOL RegObserver(IStreamObserver* pObs) override {
        std::lock_guard<std::mutex> lck(m_mtxStat);
        if (pObs && !m_stStat.bStarted) {
            for (auto&& m : m_lstObs) {
                if (m == pObs) {
                    return AX_TRUE;
                }
            }

            m_lstObs.push_back(pObs);
            return AX_TRUE;
        }

        return AX_FALSE;
    }

    AX_BOOL UnRegObserver(IStreamObserver* pObs) override {
        std::lock_guard<std::mutex> lck(m_mtxStat);
        if (pObs && !m_stStat.bStarted) {
            for (auto&& m : m_lstObs) {
                if (m == pObs) {
                    m_lstObs.remove(m);
                    return AX_TRUE;
                }
            }
        }

        return AX_FALSE;
    }

protected:
    AX_VOID UpdateStatus(AX_BOOL bStart) {
        std::lock_guard<std::mutex> lck(m_mtxStat);
        m_stStat.bStarted = bStart;
        if (bStart) {
            m_stStat.nCount = 0;
        }
    }

protected:
    STREAMER_INFO_T m_stInfo;
    STREAMER_STAT_T m_stStat;
    std::mutex m_mtxStat;
    std::list<IStreamObserver*> m_lstObs;
};
