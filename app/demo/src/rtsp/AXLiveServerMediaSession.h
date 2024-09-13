/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXLIVESEVERMEDIASESSION_H__
#define __AXLIVESEVERMEDIASESSION_H__

#include <pthread.h>
#include <queue>
#include "ax_global_type.h"
#include "AXFramedSource.h"
#include "OnDemandServerMediaSubsession.hh"
#include "liveMedia.hh"

class AXLiveServerMediaSession : public OnDemandServerMediaSubsession {
public:
    static AXLiveServerMediaSession* createNewVideo(UsageEnvironment& env, bool reuseFirstSource, AX_PAYLOAD_TYPE_E ePt = PT_H264, AX_U32 nMaxFrmSize = 700000, AX_U32 nBitRate = 48000);
    static AXLiveServerMediaSession* createNewAudio(UsageEnvironment& env, bool reuseFirstSource, AX_PAYLOAD_TYPE_E ePt = PT_AAC, AX_U32 nMaxFrmSize = 8192, AX_U32 nBitRate = 48000, AX_U32 nSampleRate = 16000, AX_U8 nChnCnt = 1, AX_S32 nAOT = 1);
    void checkForAuxSDPLine1();
    void afterPlayingDummy1();
    void SendNalu(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_FALSE);

protected:
    AXLiveServerMediaSession(UsageEnvironment& env, bool reuseFirstSource, AX_PAYLOAD_TYPE_E ePt, AX_U32 nMaxFrmSize, AX_U32 nBitRate, AX_U32 nSampleRate, AX_U8 nChnCnt, AX_S32 nAOT);
    virtual ~AXLiveServerMediaSession(void);
    void setDoneFlag() {
        fDoneFlag = ~0;
    }

protected:
    virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
    virtual void closeStreamSource(FramedSource* inputSource);
    virtual char const* sdpLines();

private:
    AX_PAYLOAD_TYPE_E m_ePt{PT_H264};
    AX_U32 m_nMaxFrmBuffSize;
    AX_U32 m_nSampleRate;
    AX_U32 m_nBitRate;
    AX_U32 m_nChnCnt;
    AX_U32 m_nAOT;
    char* fAuxSDPLine;
    char fDoneFlag;
    RTPSink* fDummySink;
    AXFramedSource* m_pSource;
    pthread_spinlock_t m_tLock;
};

#endif /*__AXLIVESEVERMEDIASESSION_H__*/
