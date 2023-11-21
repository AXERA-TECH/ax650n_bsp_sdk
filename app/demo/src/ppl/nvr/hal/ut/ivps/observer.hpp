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
#include "AXFrame.hpp"
#include "AppLogApi.h"
#include "IObserver.h"

class MyObserver : public IObserver {
public:
    MyObserver(AX_VOID) = default;

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID *pData) override {
        CAXFrame &axFrame = *((CAXFrame *)pData);

        /* observer is in charge of VB reference count management */
        axFrame.IncRef();

        if (0 == axFrame.stFrame.stVFrame.stVFrame.u64SeqNum % 30) {
            if (E_OBS_TARGET_TYPE_VDEC == eTarget) {
                LOG_M_C("OBS", "Observer %p received from VDEC: frame %lld, %dx%d stride %d pts %lld from vdGrp %2d vdChn %2d", this,
                        axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u32Width, axFrame.stFrame.stVFrame.stVFrame.u32Height,
                        axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0], axFrame.stFrame.stVFrame.stVFrame.u64PTS, axFrame.nGrp, axFrame.nChn);
            } else if (E_OBS_TARGET_TYPE_IVPS == eTarget) {
                LOG_M_C("OBS", "Observer %p received from IVPS: frame %lld, %dx%d stride %d pts %lld from vdGrp %2d vdChn %2d", this,
                        axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u32Width, axFrame.stFrame.stVFrame.stVFrame.u32Height,
                        axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0], axFrame.stFrame.stVFrame.stVFrame.u64PTS, axFrame.nGrp, axFrame.nChn);
            }
        }

        axFrame.DecRef();
        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E, AX_U32, AX_U32, OBS_TRANS_ATTR_PTR) override {
        return AX_TRUE;
    }
};