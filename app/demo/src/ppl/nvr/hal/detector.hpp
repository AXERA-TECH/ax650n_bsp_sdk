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
#include <vector>
#include "AXFrame.hpp"
#include "AXLockQ.hpp"
#include "AXResource.hpp"
#include "AXThread.hpp"
#include "IObserver.h"
#include "ax_skel_api.h"
#include "detectResult.h"
#define MAX_DETECTOR_GROUP_NUM (97) /* 64 + 32 + 1 */
#define DETECTOR_MAX_CHN_NUM 3

typedef struct DETECTOR_CHN_ATTR_S {
    AX_U32 nPPL;
    AX_U32 nVNPU;
    AX_BOOL bTrackEnable;

    DETECTOR_CHN_ATTR_S(AX_VOID) {
        nPPL = AX_SKEL_PPL_HVCFP;
        nVNPU = AX_SKEL_NPU_DEFAULT;
        bTrackEnable = AX_FALSE;
    }
} DETECTOR_CHN_ATTR_T;

typedef struct DETECTOR_ATTR_S {
    AX_U32 nGrpCount{MAX_DETECTOR_GROUP_NUM};
    AX_S32 nSkipRate{0};
    std::string strModelPath;
    AX_SKEL_PPL_E ePPL{AX_SKEL_PPL_MAX};
    AX_U32 nFrameDepth{1};
    AX_U32 nWidth{0};
    AX_U32 nHeight{0};
    AX_U32 nChannelNum{1};
    DETECTOR_CHN_ATTR_T tChnAttr[DETECTOR_MAX_CHN_NUM];
} DETECTOR_ATTR_T;

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

    AX_BOOL SendFrame(AX_U32 nGrp, CAXFrame& axFrame);

    AX_VOID ReleaseSkelPrivateData(SKEL_FRAME_PRIVATE_DATA_T* pData) {
        m_skelData.giveback(pData);
    }

    AX_VOID RegisterObserver(AX_U32 nGrp, IObserver* pObserver);
    AX_VOID UnRegisterObserver(AX_U32 nGrp, IObserver* pObs);
    AX_VOID NotifyAll(AX_U32 nGrp, AX_U32 nChn, DETECT_RESULT_T& pData);

    const DETECTOR_ATTR_T& GetAttr(AX_VOID) const {
        return m_stAttr;
    }

private:
    AX_BOOL SkipFrame(const CAXFrame& axFrame);

protected:
    AX_VOID ProcessFrame(AX_VOID* pArg);
    AX_VOID ClearQueue(AX_U32 nGrp);

protected:
    CAXLockQ<CAXFrame>* m_arrFrameQ{nullptr};
    DETECTOR_ATTR_T m_stAttr;
    CAXThread m_DetectThread;
    AX_SKEL_HANDLE m_hSkel[DETECTOR_MAX_CHN_NUM]{NULL};

private:
    std::mutex m_mtxObs;
    std::mutex m_mtxApi;

    AX_BOOL m_bInited{AX_FALSE};
    AX_BOOL m_bStarted{AX_FALSE};
    CAXResource<SKEL_FRAME_PRIVATE_DATA_T> m_skelData;
    std::map<AX_U32, IObserver*> m_mapObs;
};

class CDetectObserver : public IObserver {
public:
    CDetectObserver(CDetector* pSink) : m_pSink(pSink){};

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        CAXFrame* pFrame = (CAXFrame*)pData;
        if (!m_pSink) {
            return AX_FALSE;
        }
        if (E_OBS_TARGET_TYPE_VDEC == eTarget) {
            LOG_M_D("detector_obs",
                    "Observer %p grp:%d,chn:%d, received from VDEC: frame %lld, %dx%d stride %d pts %lld from vdGrp %2d vdChn "
                    "%2d,blkId:0x%x,phy:0x%llx",
                    nGrp, pFrame->nChn, this, pFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pFrame->stFrame.stVFrame.stVFrame.u32Width,
                    pFrame->stFrame.stVFrame.stVFrame.u32Height, pFrame->stFrame.stVFrame.stVFrame.u32PicStride[0],
                    pFrame->stFrame.stVFrame.stVFrame.u64PTS, nGrp, pFrame->nChn, pFrame->stFrame.stVFrame.stVFrame.u32BlkId[0],
                    pFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0]);
        } else if (E_OBS_TARGET_TYPE_IVPS == eTarget) {
            LOG_M_D("detector_obs", "Observer %p received from IVPS: frame %lld, %dx%d stride %d pts %lld from vdGrp %2d vdChn %2d", this,
                    pFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pFrame->stFrame.stVFrame.stVFrame.u32Width,
                    pFrame->stFrame.stVFrame.stVFrame.u32Height, pFrame->stFrame.stVFrame.stVFrame.u32PicStride[0],
                    pFrame->stFrame.stVFrame.stVFrame.u64PTS, nGrp, nChn, pFrame->stFrame.stVFrame.stVFrame.u32BlkId[0],
                    pFrame->stFrame.stVFrame.stVFrame.u64PhyAddr[0]);
        }
        return m_pSink->SendFrame(nGrp, *pFrame);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        return AX_TRUE;
    }

private:
    CDetector* m_pSink{nullptr};
};