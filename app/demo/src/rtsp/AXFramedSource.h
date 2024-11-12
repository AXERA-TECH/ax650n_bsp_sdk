/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXFRAMEDSOURCE_H__
#define __AXFRAMEDSOURCE_H__

#include "AXRingBuffer.h"
#include "FramedSource.hh"

class AXFramedSource : public FramedSource {
public:
    static AXFramedSource* createNew(UsageEnvironment& env, AX_U32 nMaxFrmSize);

public:
    static EventTriggerId eventTriggerId;
    // Note that this is defined here to be a static class variable, because this code is intended to illustrate how to
    // encapsulate a *single* device - not a set of devices.
    // You can, however, redefine this to be a non-static member variable.
    void AddFrameBuff(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_FALSE);
    virtual unsigned maxFrameSize() const;

protected:
    AXFramedSource(UsageEnvironment& env, AX_U32 nMaxFrmSize);
    // called only by createNew(), or by subclass constructors
    virtual ~AXFramedSource();

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
    // virtual void doStopGettingFrames(); // optional

private:
    static void deliverFrame(void* clientData);
    void _deliverFrame();

private:
    static unsigned referenceCount;  // used to count how many instances of this class currently exist
    CAXRingBuffer* m_pRingBuf;
    AX_U32 m_nMaxFrmSize;

    u_int32_t m_nTriggerID;

    FILE* m_pFile {nullptr};
};

#endif /*__AXFRAMEDSOURCE_H__*/