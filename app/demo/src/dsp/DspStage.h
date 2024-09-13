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

#include <list>
#include <map>
#include <mutex>
#include <vector>
#include "AXFrame.hpp"
#include "AXRingBuffer.h"
#include "AXStage.hpp"
#include "GlobalDef.h"
#include "IObserver.h"
#include "OSDHandlerWrapper.h"
#include "ax_dsp_cv_api.h"

#define DSP_INVALID_VAL 0
#define DSP_ID_MAX 2

typedef struct _DSP_ATTR_S {
    AX_U8 nDeepCnt{1};
    AX_U32 nDspId{DSP_INVALID_VAL};
    AX_S8 nGrp;
    AX_U32 nSrcWidth{DSP_INVALID_VAL};
    AX_U32 nSrcHeight{DSP_INVALID_VAL};
    AX_U32 nDstWidth{DSP_INVALID_VAL};
    AX_U32 nDstHeight{DSP_INVALID_VAL};
    AX_F32 fSrcFramerate{DSP_INVALID_VAL};
    std::string strSramPath;
    std::string strItcmPath;
} DSP_ATTR_S;

typedef struct _DSP_DST_S {
    AX_U64 u64VirAddr{DSP_INVALID_VAL};
    AX_U64 u64PhyAddr{DSP_INVALID_VAL};
    AX_BOOL bUsable{AX_TRUE};
} DSP_DST_S;
class CDspStage : public CAXStage, public IFrameRelease {
public:
    CDspStage(DSP_ATTR_S& tDspAttr);
    virtual ~CDspStage(AX_VOID) = default;

    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start(STAGE_START_PARAM_PTR pStartParams) override;
    virtual AX_BOOL Stop() override;
    virtual AX_VOID VideoFrameRelease(CAXFrame* pFrame) override;

    DSP_ATTR_S* GetDspAttr() {
        return &m_tDspAttr;
    }
    AX_VOID RegObserver(AX_S32 nChannel, IObserver* pObserver);

    AX_BOOL SendFrame(AX_U32 nGrp, CAXFrame* axFrame);
    AX_VOID NotifyAll(AX_U32 nGrp, AX_U32 nType, AX_VOID* pStream);
    AX_VOID ProcessFrame(AX_VOID* pArg);

private:
    DSP_ATTR_S m_tDspAttr;
    std::vector<IObserver*> m_vecObserver;
    std::vector<DSP_DST_S> m_listDst;
    CAXLockQ<CAXFrame*>* m_arrFrameQ{nullptr};
    AX_BOOL m_initState{AX_FALSE};
    CAXThread m_DspThread;
    AX_DSP_CV_RESIZE_PARAM_T m_DspParam{0};
};