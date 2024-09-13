/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AXLiveServerMediaSession.h"
#include <GroupsockHelper.hh>

AXLiveServerMediaSession* AXLiveServerMediaSession::createNewVideo(UsageEnvironment& env, bool reuseFirstSource, AX_PAYLOAD_TYPE_E ePt, AX_U32 nMaxFrmSize, AX_U32 nBitRate) {
    return new AXLiveServerMediaSession(env, reuseFirstSource, ePt, nMaxFrmSize, nBitRate, 0, 0, 0);
}

AXLiveServerMediaSession* AXLiveServerMediaSession::createNewAudio(UsageEnvironment& env, bool reuseFirstSource, AX_PAYLOAD_TYPE_E ePt, AX_U32 nMaxFrmSize, AX_U32 nBitRate, AX_U32 nSampleRate, AX_U8 nChnCnt, AX_S32 nAOT) {
    return new AXLiveServerMediaSession(env, reuseFirstSource, ePt, nMaxFrmSize, nBitRate, nSampleRate, nChnCnt, nAOT);
}
AXLiveServerMediaSession::AXLiveServerMediaSession(UsageEnvironment& env, bool reuseFirstSource, AX_PAYLOAD_TYPE_E ePt, AX_U32 nMaxFrmSize, AX_U32 nBitRate, AX_U32 nSampleRate, AX_U8 nChnCnt, AX_S32 nAOT)
    : OnDemandServerMediaSubsession(env, reuseFirstSource),
      m_ePt(ePt),
      m_nMaxFrmBuffSize(nMaxFrmSize),
      m_nSampleRate(nSampleRate),
      m_nBitRate(nBitRate),
      m_nChnCnt(nChnCnt),
      m_nAOT(nAOT),
      fAuxSDPLine(NULL),
      fDoneFlag(0),
      fDummySink(NULL),
      m_pSource(NULL) {
    pthread_spin_init(&m_tLock, 0);
}

AXLiveServerMediaSession::~AXLiveServerMediaSession(void) {
    delete[] fAuxSDPLine;
    pthread_spin_destroy(&m_tLock);
}

static void afterPlayingDummy(void* clientData) {
    AXLiveServerMediaSession* session = (AXLiveServerMediaSession*)clientData;
    session->afterPlayingDummy1();
}

void AXLiveServerMediaSession::afterPlayingDummy1() {
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
    AXLiveServerMediaSession* session = (AXLiveServerMediaSession*)clientData;
    session->checkForAuxSDPLine1();
}

void AXLiveServerMediaSession::checkForAuxSDPLine1() {
    char const* dasl;
    if (fAuxSDPLine != NULL) {
        setDoneFlag();
    } else if (fDummySink != NULL && (dasl = fDummySink->auxSDPLine()) != NULL) {
        fAuxSDPLine = strDup(dasl);
        fDummySink = NULL;
        setDoneFlag();
    } else {
        int uSecsDelay = 100000;
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsDelay, (TaskFunc*)checkForAuxSDPLine, this);
    }
}

char const* AXLiveServerMediaSession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
    if (fAuxSDPLine != NULL) {
        return fAuxSDPLine;
    }
    if (fDummySink == NULL) {
        fDummySink = rtpSink;
        fDummySink->startPlaying(*inputSource, afterPlayingDummy, this);
        checkForAuxSDPLine(this);
    }
    envir().taskScheduler().doEventLoop(&fDoneFlag);
    return fAuxSDPLine;
}

FramedSource* AXLiveServerMediaSession::createNewStreamSource(unsigned clientSessionID, unsigned& estBitRate) {
    // Based on encoder configuration i kept it 90000
    estBitRate = m_nBitRate;

    AXFramedSource* source = AXFramedSource::createNew(envir(), m_nMaxFrmBuffSize);
    pthread_spin_lock(&m_tLock);
    m_pSource = source;
    pthread_spin_unlock(&m_tLock);
    // are you trying to keep the reference of the source somewhere? you shouldn't.
    // Live555 will create and delete this class object many times. if you store it somewhere
    // you will get memory access violation. instead you should configure you source to always read from your data source
    if (PT_H264 == m_ePt) {
        return H264VideoStreamFramer::createNew(envir(), source);
    } else if (PT_H265 == m_ePt) {
        return H265VideoStreamFramer::createNew(envir(), source);
    } else if (PT_AAC == m_ePt) {
        return ADTSAudioStreamFramer::createNew(envir(), source, m_nSampleRate);
    } else if (PT_G711A == m_ePt) {
        return PCMGenericStreamFramer::createNew(envir(), source, m_nSampleRate);
    } else if (PT_G711U == m_ePt) {
        return PCMGenericStreamFramer::createNew(envir(), source, m_nSampleRate);
    } else if (PT_G726 == m_ePt) {
        return PCMGenericStreamFramer::createNew(envir(), source, m_nSampleRate);
    }

    return nullptr;
}

void AXLiveServerMediaSession::closeStreamSource(FramedSource* inputSource) {
    pthread_spin_lock(&m_tLock);
    m_pSource = NULL;
    Medium::close(inputSource);
    pthread_spin_unlock(&m_tLock);
}

char const* AXLiveServerMediaSession::sdpLines() {
    return OnDemandServerMediaSubsession::sdpLines();
}

RTPSink* AXLiveServerMediaSession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
                                                    FramedSource* inputSource) {
    increaseSendBufferTo(envir(), rtpGroupsock->socketNum(), 500 * 1024);
    if (PT_H264 == m_ePt) {
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    } else if (PT_H265 == m_ePt) {
        return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    } else if (PT_AAC == m_ePt) {
        return ADTSAudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, m_nSampleRate, m_nChnCnt, m_nAOT);
    } else if (PT_G711A == m_ePt) {
        return PCMGenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, m_nSampleRate, m_nChnCnt, (AX_S32)m_ePt);
    } else if (PT_G711U == m_ePt) {
        return PCMGenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, m_nSampleRate, m_nChnCnt, (AX_S32)m_ePt);
    } else if (PT_G726 == m_ePt) {
        return PCMGenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, m_nSampleRate, m_nChnCnt, (AX_S32)m_ePt);
    }

    return nullptr;
}

void AXLiveServerMediaSession::SendNalu(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts /*=0*/, AX_BOOL bIFrame /*=AX_FALSE*/) {
    pthread_spin_lock(&m_tLock);
    if (m_pSource) {
        m_pSource->AddFrameBuff(nChn, pBuf, nLen, nPts, bIFrame);
    }

    pthread_spin_unlock(&m_tLock);
}
