/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AXRtspServer.h"
#include <sys/prctl.h>
#include "AXStringHelper.hpp"
#include "AppLogApi.h"
#include "CommonUtils.hpp"

#define RTSP_SRV "RTSP_SRV"

AX_BOOL CAXRtspServer::Init() {

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::DeInit() {
    return AX_TRUE;
}

AX_BOOL CAXRtspServer::Start() {
    m_pServerThread = new thread(&CAXRtspServer::RtspServerThreadFunc, this);
    if (m_pServerThread) {
        m_bServerThreadWorking = AX_TRUE;
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CAXRtspServer::Stop() {
    m_bServerThreadWorking = AX_FALSE;
    StopRtspServerThread();

    if (m_pServerThread) {
        m_pServerThread->join();
        delete m_pServerThread;
        m_pServerThread = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::SendNalu(AX_U32 nChannel, AX_VOID* pData, AX_U32 nLen, AX_U64 nPTS, AX_BOOL bIFrame /*= AX_FALSE*/) {
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    if (m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO]) {
        m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO]->SendNalu(nChannel, (AX_U8*)pData, nLen, nPTS, bIFrame);
    }

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::SendAudio(AX_U32 nChannel, AX_VOID* pData, AX_U32 nLen, AX_U64 nPTS) {
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    if (m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO]) {
        m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO]->SendNalu(nChannel, (AX_U8*)pData, nLen, nPTS, AX_TRUE);
    }

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::AddSessionAttr(AX_U32 nChannel, const RTSP_SESS_ATTR_T &stSessAttr) {
    m_vecMediaSessionAttr.emplace_back(stSessAttr);

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::UpdateSessionAttr(AX_U32 nChannel, const RTSP_SESS_ATTR_T &stSessAttr) {
    for (auto &stAttr : m_vecMediaSessionAttr) {
        if (stAttr.nChannel == nChannel) {
            stAttr.stVideoAttr = stSessAttr.stVideoAttr;
            stAttr.stAudioAttr = stSessAttr.stAudioAttr;
            break;
        }
    }

    return AX_TRUE;
}

AX_VOID CAXRtspServer::RtspServerThreadFunc() {
    prctl(PR_SET_NAME, "APP_RTSP_Main");

    OutPacketBuffer::maxSize = 700000;
    TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
    m_pUEnv = BasicUsageEnvironment::createNew(*taskSchedular);
    m_rtspServer = RTSPServer::createNew(*m_pUEnv, 8554, NULL);
    if (nullptr == m_rtspServer) {
        LOG_M_E(RTSP_SRV, "Failed to create rtsp server :: %s", m_pUEnv->getResultMsg());
        return;
    }

    AX_CHAR szIP[64] = {0};
    AX_BOOL bGetIPRet = AX_FALSE;
    if (CCommonUtils::GetIP(&szIP[0])) {
        bGetIPRet = AX_TRUE;
    }

    AX_U32 nChannel = 0;
    AX_U32 nMaxFrmSize = 0;
    AX_U32 nBitRate = 0;
    AX_PAYLOAD_TYPE_E ePt = PT_H264;
    AX_U32 nSessionCount = m_vecMediaSessionAttr.size();
    for (AX_U32 i = 0; i < nSessionCount; i++) {
        nChannel = m_vecMediaSessionAttr[i].nChannel;
        std::string strStream = CAXStringHelper::Format("axstream%d", nChannel);
        ServerMediaSession* sms = ServerMediaSession::createNew(*m_pUEnv, strStream.c_str(), strStream.c_str(), "Live Stream");
        if (m_vecMediaSessionAttr[i].stVideoAttr.bEnable) {
            ePt = m_vecMediaSessionAttr[i].stVideoAttr.ePt;
            nMaxFrmSize = m_vecMediaSessionAttr[i].stVideoAttr.nMaxFrmSize;
            nBitRate = m_vecMediaSessionAttr[i].stVideoAttr.nBitRate;
            m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO] = AXLiveServerMediaSession::createNewVideo(*m_pUEnv, true, ePt, nMaxFrmSize, nBitRate);
            sms->addSubsession(m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO]);
        }
        if (m_vecMediaSessionAttr[i].stAudioAttr.bEnable) {
            ePt = m_vecMediaSessionAttr[i].stAudioAttr.ePt;
            nMaxFrmSize = m_vecMediaSessionAttr[i].stAudioAttr.nMaxFrmSize;
            nBitRate = m_vecMediaSessionAttr[i].stAudioAttr.nBitRate;
            AX_U32 nSampleRate = m_vecMediaSessionAttr[i].stAudioAttr.nSampleRate;
            AX_U8 nChnCnt = m_vecMediaSessionAttr[i].stAudioAttr.nChnCnt;
            AX_S32 nAOT = m_vecMediaSessionAttr[i].stAudioAttr.nAOT;
            m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO] = AXLiveServerMediaSession::createNewAudio(*m_pUEnv, true, ePt, nMaxFrmSize, nBitRate, nSampleRate, nChnCnt, nAOT);
            sms->addSubsession(m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO]);
        }
        m_rtspServer->addServerMediaSession(sms);

        char* url = nullptr;
        if (bGetIPRet) {
            url = new char[128];
            sprintf(url, "rtsp://%s:8554/%s", szIP, strStream.c_str());
        } else {
            url = m_rtspServer->rtspURL(sms);
        }

        printf("Play the stream using url: <<<<< %s >>>>>\n", url);

        delete[] url;
        url = nullptr;
    }

    while (m_bServerThreadWorking) {
        m_chStopEventLoop = 0;
        taskSchedular->doEventLoop(&m_chStopEventLoop);
        if (m_bNeedRestartSessions) {
            DoRestartSessions();
        }
    }

    delete (taskSchedular);

    LOG_M_I(RTSP_SRV, "Quit rtsp server thread func.");

    return;
}

AX_VOID CAXRtspServer::StopRtspServerThread() {
    AX_U32 nChannel = 0;
    AX_U32 nSessionCount = m_vecMediaSessionAttr.size();
    for (AX_U32 i = 0; i < nSessionCount; i++) {
        std::unique_lock<std::mutex> lck(m_mtxSessions);
        nChannel = m_vecMediaSessionAttr[i].nChannel;

        std::string strStream = CAXStringHelper::Format("axstream%d", nChannel);
        m_rtspServer->deleteServerMediaSession(strStream.c_str());

        for (AX_U32 j = 0; j < RTSP_SESS_MEDIA_BUTT; j ++) {
            m_pMediaSession[nChannel][j] = nullptr;
        }
    }

    m_pUEnv = nullptr;

    m_chStopEventLoop = 1;
}

AX_VOID CAXRtspServer::RestartSessions(AX_VOID) {
    static constexpr AX_U32 kRestartSessionsTimeoutMilliseconds = 50;
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    m_bNeedRestartSessions = AX_TRUE;
    m_chStopEventLoop = 1;
    m_cvSessions.wait_for(lck, std::chrono::milliseconds(kRestartSessionsTimeoutMilliseconds));
}

AX_VOID CAXRtspServer::DoRestartSessions(AX_VOID) {
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    m_bNeedRestartSessions = AX_FALSE;
    AX_U32 nSessionCount = m_vecMediaSessionAttr.size();
    for (AX_U32 i = 0; i < nSessionCount; i++) {
        AX_U32 nChannel = m_vecMediaSessionAttr[i].nChannel;
        std::string strStream = CAXStringHelper::Format("axstream%d", nChannel);
        m_rtspServer->deleteServerMediaSession(strStream.c_str());
        m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO] = nullptr;
        m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO] = nullptr;

        ServerMediaSession* sms = ServerMediaSession::createNew(*m_pUEnv, strStream.c_str(), strStream.c_str(), "Live Stream");
        if (m_vecMediaSessionAttr[i].stVideoAttr.bEnable) {
            AX_PAYLOAD_TYPE_E ePt = m_vecMediaSessionAttr[i].stVideoAttr.ePt;
            AX_U32 nMaxFrmSize = m_vecMediaSessionAttr[i].stVideoAttr.nMaxFrmSize;
            AX_U32 nBitRate = m_vecMediaSessionAttr[i].stVideoAttr.nBitRate;
            m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO] = AXLiveServerMediaSession::createNewVideo(*m_pUEnv, true, ePt, nMaxFrmSize, nBitRate);
            sms->addSubsession(m_pMediaSession[nChannel][RTSP_SESS_MEDIA_VIDEO]);
        }

        if (m_vecMediaSessionAttr[i].stAudioAttr.bEnable) {
            AX_PAYLOAD_TYPE_E ePt = m_vecMediaSessionAttr[i].stAudioAttr.ePt;
            AX_U32 nMaxFrmSize = m_vecMediaSessionAttr[i].stAudioAttr.nMaxFrmSize;
            AX_U32 nBitRate = m_vecMediaSessionAttr[i].stAudioAttr.nBitRate;
            AX_U32 nSampleRate = m_vecMediaSessionAttr[i].stAudioAttr.nSampleRate;
            AX_U8 nChnCnt = m_vecMediaSessionAttr[i].stAudioAttr.nChnCnt;
            AX_S32 nAOT = m_vecMediaSessionAttr[i].stAudioAttr.nAOT;
            m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO] = AXLiveServerMediaSession::createNewAudio(*m_pUEnv, true, ePt, nMaxFrmSize, nBitRate, nSampleRate, nChnCnt, nAOT);
            sms->addSubsession(m_pMediaSession[nChannel][RTSP_SESS_MEDIA_AUDIO]);
        }

        m_rtspServer->addServerMediaSession(sms);
    }

    m_cvSessions.notify_one();
}
