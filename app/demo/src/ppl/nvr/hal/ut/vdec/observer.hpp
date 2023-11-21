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

        if (0 == m_u64LastPTS) {
            m_u64LastPTS = axFrame.stFrame.stVFrame.stVFrame.u64PTS;
        } else {
            if (axFrame.stFrame.stVFrame.stVFrame.u64PTS <= m_u64LastPTS) {
                LOG_M_E("OBS", "pts %lld of frame %lld from vdGrp %2d vdChn %2d is < last %lld", axFrame.stFrame.stVFrame.stVFrame.u64PTS,
                        axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.nGrp, axFrame.nChn, m_u64LastPTS);
            } else {
                m_u64LastPTS = axFrame.stFrame.stVFrame.stVFrame.u64PTS;
            }
        }

        if (axFrame.stFrame.stVFrame.stVFrame.u64SeqNum <= m_u64SeqNum) {
            LOG_M_E("OBS", "seq %lld of frame %lld from vdGrp %2d vdChn %2d is < last %lld", axFrame.stFrame.stVFrame.stVFrame.u64SeqNum,
                    axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.nGrp, axFrame.nChn, m_u64SeqNum);
        } else {
            m_u64SeqNum = axFrame.stFrame.stVFrame.stVFrame.u64SeqNum;
        }

        /*
         if (0 == axFrame.stFrame.stVFrame.stVFrame.u64SeqNum % 30) {
            LOG_M_C("OBS", "Observer %p received frame %lld: %dx%d stride %d pts %lld from vdGrp %2d vdChn %2d", this,
                    axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u32Width, axFrame.stFrame.stVFrame.stVFrame.u32Height,
                    axFrame.stFrame.stVFrame.stVFrame.u32PicStride[0], axFrame.stFrame.stVFrame.stVFrame.u64PTS, axFrame.nGrp, axFrame.nChn);
        } */

        axFrame.DecRef();
        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E, AX_U32, AX_U32, OBS_TRANS_ATTR_PTR) override {
        return AX_TRUE;
    }

private:
    AX_U64 m_u64LastPTS = {0};
    AX_U64 m_u64SeqNum = {0};
};