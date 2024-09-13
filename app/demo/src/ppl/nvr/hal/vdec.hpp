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
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <unordered_map>
#include "AXSingleton.h"
#include "AXThread.hpp"
#include "IObserver.h"
#include "ax_vdec_api.h"
#include "haltype.hpp"
#include "istream.hpp"

#ifdef TEST_LATENCY
#include "KvmVdecDef.h"
#endif

#define MAX_VDEC_GRP_NUM (AX_VDEC_MAX_GRP_NUM)
#define MAX_VDEC_CHN_NUM (AX_VDEC_MAX_CHN_NUM)
#define INVALID_VDEC_GRP (-1)
#define INVALID_VDEC_CHN (-1)
#define NVR_VDEC_RESET_GRP_TRY_COUNT (3)

/*
    @param - AX_VDEC_CHN_ATTR_T.u32OutputFifoDepth:

            = MAX(MAX(u32OutputFifoDepth, tot_buffers), AX_VDEC_MAX_FRAME_BUF_NUM(34))
                if(no_reorder)
                    tot_buffers = MAX(storage->active_sps->num_ref_frames,1) + 1;
                else
                    tot_buffers = max_dpb_size + 1;

                tot_buffers += storage->n_extra_frm_buffers; // n_extra_frm_buffers = AX_VDEC_EXTRA_OUT_BUF_CNT(1)

            max_dpb_size is related to max_ref_frames and vui.max_dec_frame_buffering

            so if u32OutputFifoDepth = 0, then set to tot_buffers
*/
typedef struct VDEC_CHN_ATTR_S {
    AX_BOOL bEnable;
    AX_BOOL bLinked;
    AX_VDEC_CHN_ATTR_T stAttr;
    VDEC_CHN_ATTR_S(AX_VOID) {
        memset(this, 0, sizeof(*this));
    }
} VDEC_CHN_ATTR_T;

typedef struct VDEC_ATTR_S {
    AX_VDEC_GRP vdGrp;
    AX_PAYLOAD_TYPE_E enCodecType;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_VDEC_DISPLAY_MODE_E enDecodeMode;
    AX_VDEC_INPUT_MODE_E enInputMode;
    AX_VDEC_MODE_E enIPBMode;
    AX_U32 nMaxStreamBufSize; /* if 0, equal to nWidth * nHeight * 3 / 2 */
    AX_U32 nFps;
    AX_BOOL bPrivatePool;
    VDEC_CHN_ATTR_T stChnAttr[MAX_VDEC_CHN_NUM];
    /*
        default timeout in ms of AX_VDEC_SendStream:
        recommend:
        - AX_VDEC_DISPLAY_MODE_PLAYBACK: -1 (INFINITE)
        -  AX_VDEC_DISPLAY_MODE_PREVIEW: 100
     */
    AX_S32 nTimeOut;
    VDEC_ATTR_S(AX_VOID) {
        vdGrp = INVALID_VDEC_GRP; /* INVALID_VDEC_GRP: auto allocate grp id */
        enCodecType = PT_H264;
        nWidth = 0;
        nHeight = 0;
        enDecodeMode = AX_VDEC_DISPLAY_MODE_PREVIEW;
        enInputMode = AX_VDEC_INPUT_MODE_FRAME;
        enIPBMode = VIDEO_DEC_MODE_IPB;
        nMaxStreamBufSize = 0;
        nFps = 0;
        bPrivatePool = AX_TRUE;
        nTimeOut = 100;
    }
} VDEC_ATTR_T;

class CVDEC : public IStreamObserver {
public:
    static AX_U32 GetBlkSize(AX_U32 nWidth, AX_U32 nHeight, AX_PAYLOAD_TYPE_E enType,
                             CONST AX_FRAME_COMPRESS_INFO_T* pstCompressInfo = nullptr,
                             AX_IMG_FORMAT_E ePxlFmt = AX_FORMAT_YUV420_SEMIPLANAR);

    static CVDEC* CreateInstance(CONST VDEC_ATTR_T& stAttr);
    CVDEC(AX_VOID) = default;
    AX_BOOL Destory(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL ResetGrp(AX_S32 nTryCount = NVR_VDEC_RESET_GRP_TRY_COUNT);

    /* App is in charge of PTS */
    AX_BOOL SendStream(CONST AX_U8* pStream, AX_U32 nLen, AX_U64 u64PTS, AX_U64 nPrivData = 0, AX_BOOL bSkipDisplay = AX_FALSE,
                       AX_U64 u64UserData = 0,
                       AX_S32 nTimeOut = INFINITE);

    AX_BOOL RegisterObserver(AX_S32 vdChn, IObserver* pObs);
    AX_BOOL UnRegisterObserver(AX_S32 vdChn, IObserver* pObs);

    /* user pool, only available if VDEC_ATTR_T.bPrivatePool = AX_FALSE */
    AX_BOOL AttachPool(AX_S32 vdChn, AX_POOL pool);
    AX_BOOL DetachPool(AX_S32 vdChn);

    CONST VDEC_ATTR_T& GetAttr(AX_VOID) CONST;
    AX_VDEC_GRP GetGrpId(AX_VOID) CONST;

    /* set only available before Start */
    AX_BOOL GetChnAttr(AX_S32 vdChn, AX_VDEC_CHN_ATTR_T& stChnAttr);
    AX_BOOL SetChnAttr(AX_S32 vdChn, CONST AX_VDEC_CHN_ATTR_T& stChnAttr);

    AX_BOOL Init(CONST VDEC_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

public:
    /* IStreamObserver::OnRecvStreamData */
    AX_BOOL OnRecvStreamData(CONST STREAM_FRAME_T& stFrame) override;
    AX_BOOL OnRecvStreamInfo(CONST STREAM_INFO_T& stInfo) override;
    AX_VOID OnNotifyConnStatus(CONST AX_CHAR* pUrl, CONNECT_STATUS_E enStatus) override;

protected:
    CVDEC(CONST CVDEC&) = delete;
    CVDEC& operator=(CONST CVDEC&) = delete;

    AX_BOOL CheckAttr(CONST VDEC_ATTR_T& stAttr);

protected:
    VDEC_ATTR_T m_stAttr;
    AX_VDEC_GRP m_vdGrp = {INVALID_VDEC_GRP};
    std::mutex m_mtxObs;
    std::list<IObserver*> m_lstObs[MAX_VDEC_CHN_NUM];
    std::atomic<AX_BOOL> m_bStarted = {AX_FALSE};
    AX_S32 m_nLastSendCode = {0};
};

inline CONST VDEC_ATTR_T& CVDEC::GetAttr(AX_VOID) CONST {
    return m_stAttr;
}

inline AX_VDEC_GRP CVDEC::GetGrpId(AX_VOID) CONST {
    return m_vdGrp;
}

class CDecodeTask : public CAXSingleton<CDecodeTask> {
    friend class CAXSingleton<CDecodeTask>;
    friend class CVDEC;

    /* hash and equal_to for unordered_multimap */
    struct pair_hash {
        template <typename T1, typename T2>
        std::size_t operator()(CONST std::pair<T1, T2>& p) CONST {
            return std::hash<T1>{}(p.first) ^ std::hash<T2>{}(p.second);
        }
    };

    struct pair_equal {
        template <typename T1, typename T2>
        bool operator()(CONST std::pair<T1, T2>& a, CONST std::pair<T1, T2>& b) CONST {
            return a.first == b.first && a.second == b.second;
        }
    };

public:
    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

protected:
    AX_VOID RegisterObserver(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, IObserver* pObs);
    AX_VOID UnRegisterObserver(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, IObserver* pObs);

    AX_VOID RegisterObservers(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, CONST std::list<IObserver*>& lstObs);
    AX_VOID UnRegisterObservers(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, CONST std::list<IObserver*>& lstObs);

private:
    CDecodeTask(AX_VOID) = default;

    AX_VOID DecodingThread(AX_VOID* pArg);
    AX_VOID WaitTask(AX_VOID);
    AX_VOID Wakeup(AX_VOID);
    AX_VOID RemoveObservers(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn);
    AX_VOID OnRecvFrame(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, CONST AX_VIDEO_FRAME_INFO_T& stVFrame);

private:
    CAXThread m_thread;
    AX_BOOL m_bStarted = {AX_FALSE};
    std::condition_variable m_cv;
    std::mutex m_mtxObs;
    std::unordered_multimap<std::pair<AX_VDEC_GRP, AX_VDEC_CHN>, IObserver*, pair_hash, pair_equal> m_mapObs;
};

inline AX_VOID CDecodeTask::Wakeup(AX_VOID) {
    m_cv.notify_one();
}
