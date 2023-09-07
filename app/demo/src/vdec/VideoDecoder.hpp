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
#include <map>
#include <vector>
#include "AXFrame.hpp"
#include "AXThread.hpp"
#include "IObserver.h"
#include "IStreamHandler.hpp"
#if defined(__RECORD_VB_TIMESTAMP__)
#include "TimestampHelper.hpp"
#endif
#include "ax_vdec_api.h"

#define MAX_VDEC_CHN_NUM (AX_VDEC_MAX_CHN_NUM)

typedef struct VDEC_GRP_ATTR_S {
    AX_BOOL bEnable;
    AX_PAYLOAD_TYPE_E enCodecType;
    AX_U32 nMaxWidth;
    AX_U32 nMaxHeight;
    AX_U32 nFps;
    AX_VDEC_DISPLAY_MODE_E eDecodeMode;
    AX_VDEC_INPUT_MODE_E enInputMode;
    AX_U32 nMaxStreamBufSize;
    AX_BOOL bPrivatePool;
    AX_BOOL bFramerateCtrl;
    AX_BOOL bChnEnable[MAX_VDEC_CHN_NUM];
    AX_VDEC_CHN_ATTR_T stChnAttr[MAX_VDEC_CHN_NUM];

    VDEC_GRP_ATTR_S(AX_VOID) {
        bEnable = AX_FALSE;
        enCodecType = PT_H264;
        nMaxWidth = 0;
        nMaxHeight = 0;
        nFps = 0;
        enInputMode = AX_VDEC_INPUT_MODE_FRAME;
        eDecodeMode = AX_VDEC_DISPLAY_MODE_PREVIEW;
        nMaxStreamBufSize = 0;
        bPrivatePool = AX_FALSE;
        bFramerateCtrl = AX_FALSE;
        memset(&bChnEnable, 0, sizeof(bChnEnable));
        memset(&stChnAttr, 0, sizeof(stChnAttr));
    }
} VDEC_GRP_ATTR_T;

/**
 * @brief
 *
 */
class CStreamCacheBuf;
class CVideoDecoder : public IStreamObserver {
public:
    CVideoDecoder(AX_VOID) noexcept = default;

    /* calculate blk size of VB */
    static AX_U32 GetBlkSize(AX_U32 nW, AX_U32 nH, AX_U32 nAlign, AX_PAYLOAD_TYPE_E enType,
                             AX_FRAME_COMPRESS_INFO_T *pstCompressInfo = nullptr, AX_IMG_FORMAT_E ePxlFmt = AX_FORMAT_YUV420_SEMIPLANAR);

    AX_BOOL Init(const std::vector<VDEC_GRP_ATTR_T> &v);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL AttachPool(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, AX_POOL pool);
    AX_BOOL DetachPool(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn);

    AX_BOOL GetGrpAttr(AX_VDEC_GRP vdGrp, VDEC_GRP_ATTR_T &stGrpAttr) const;
    AX_BOOL GetAllGrpAttr(std::vector<VDEC_GRP_ATTR_T> &vecGrpAttr);
    AX_BOOL SetChnAttr(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, const AX_VDEC_CHN_ATTR_T &stChnAttr);

    /* static, only can register or unregister before Start */
    AX_BOOL RegObserver(AX_VDEC_GRP vdGrp, IObserver *pObs);
    AX_BOOL UnRegObserver(AX_VDEC_GRP vdGrp, IObserver *pObs);
    AX_BOOL UnRegAllObservers(AX_VOID);

    /* stream data callback */
    AX_BOOL OnRecvVideoData(AX_S32 nCookie, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS) override;
    AX_BOOL OnRecvAudioData(AX_S32 nCookie, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS) override;

protected:
    typedef struct VDEC_GRP_INFO_S {
        AX_BOOL bActive;  /* AX_TRUE: VDEC GRP is created */
        AX_BOOL bStarted; /* AX_TRUE: start to recv */
        VDEC_GRP_ATTR_T stAttr;
        AX_BOOL bLinked;  /* AX_TRUE: if one PP is linked */

#ifdef __DUMP_VDEC_NALU__
        AX_U32 nNaluCount;
#endif
        VDEC_GRP_INFO_S(AX_VOID) {
            bActive = AX_FALSE;
            bStarted = AX_FALSE;
            bLinked = AX_FALSE;
#ifdef __DUMP_VDEC_NALU__
            nNaluCount = 0;
#endif
        }
    } VDEC_GRP_INFO_T;

    AX_BOOL CreateDecoder(AX_VDEC_GRP vdGrp, const VDEC_GRP_INFO_T &stGrpInfo);

    AX_VOID RecvThread(AX_VOID *pArg);
    AX_BOOL Notify(const CAXFrame &axFrame);
    AX_BOOL Send(AX_VDEC_GRP vdGrp, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS);

protected:
    std::mutex m_mtxObs;
    std::map<AX_VDEC_GRP, std::list<IObserver *>> m_mapObs;
    std::vector<VDEC_GRP_INFO_T> m_arrGrpInfo;
    CAXThread m_DecodeThread; /* receive decoded frame thread */

#if defined(__RECORD_VB_TIMESTAMP__)
    std::vector<CTimestampHelper> m_arrTimestampHelper;
#endif

    std::vector<std::unique_ptr<CStreamCacheBuf>> m_arrCacheBuf;
    std::mutex m_mtxStop;
};

class CStreamCacheBuf {
public:
    explicit CStreamCacheBuf(AX_U32 nCapacity) noexcept : m_nCapacity(nCapacity) {
        if (0 == m_nCapacity) {
            m_nCapacity = 0x200000;
        }
        m_buf = std::make_unique<AX_U8[]>(m_nCapacity);
    }

    AX_BOOL Insert(const AX_U8 *pData, AX_U32 nSize) {
        if ((nSize + m_nCurSize) <= m_nCapacity) {
            memcpy(&m_buf[m_nCurSize], pData, nSize);
            m_nCurSize += nSize;
            return AX_TRUE;
        } else {
            return AX_FALSE;
        }
    }

    const AX_U8 *GetCacheBuf(AX_U32 &nSize) {
        nSize = m_nCurSize;
        m_nCurSize = 0;
        return &m_buf[0];
    }

    AX_U32 GetCapacity(AX_VOID) const {
        return m_nCapacity;
    }

private:
    std::unique_ptr<AX_U8[]> m_buf;
    AX_U32 m_nCapacity{0};
    AX_U32 m_nCurSize{0};
};