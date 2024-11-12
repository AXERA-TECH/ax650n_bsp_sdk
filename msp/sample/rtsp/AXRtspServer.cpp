/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dirent.h>
#include <vector>
#include <string>
#include "ax_base_type.h"
#include "AXRtspServer.h"
#include "AXFramedSource.h"
#include "GroupsockHelper.hh"

using namespace std;

#define BASE_PORT     (18888)
#define RTSPSERVER    "RTSPSERVER"

#define VIDEO_DEFAULT_MAX_FRM_SIZE (700000)
#define AUDIO_DEFAULT_MAX_FRM_SIZE (10240)

static char gStopEventLoop = 0;

static AX_BOOL GetIP(AX_CHAR* pOutIPAddress, AX_U32 nLen)
{
    const std::vector<std::string> vNetType{"eth", "usb"};
    for (size_t i = 0; i < vNetType.size(); i++) {
        for (char c = '0'; c <= '9'; ++c) {
            string strDevice = vNetType[i] + c;
            int fd;
            int ret;
            struct ifreq ifr;
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            strcpy(ifr.ifr_name, strDevice.c_str());
            ret = ioctl(fd, SIOCGIFADDR, &ifr);
            ::close(fd);
            if (ret < 0) {
                continue;
            }

            char* pIP = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
            if (pIP) {
                strncpy((char *)pOutIPAddress, pIP, nLen - 1);
                pOutIPAddress[nLen - 1] = '\0';
                return AX_TRUE;
            }
        }
    }
    return AX_FALSE;
}

void* RtspServerThreadFunc(void *args)
{
    COMM_RTSP_PRT("+++");

    prctl(PR_SET_NAME, "IPC_RTSP_Main");

    AXRtspServer * pThis = (AXRtspServer *)args;
    auto &nMaxNum = pThis->m_nMaxNum;
    auto &pUEnv = pThis->m_pUEnv;
    auto &pRtspServer = pThis->m_pRtspServer;
    auto &pLiveServerMediaSession = pThis->m_pLiveServerMediaSession;
    auto &stRtspAttr = pThis->m_stRtspAttr;

    if (nMaxNum > MAX_RTSP_CHANNEL_NUM) {
        *pUEnv << "Rtsp channel exceeded max number" <<"\n";
        return nullptr;
    }

    OutPacketBuffer::maxSize = 700000;
    TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
    pUEnv = BasicUsageEnvironment::createNew(*taskSchedular);
    pRtspServer = RTSPServer::createNew(*pUEnv, 8554, NULL);
    if(pRtspServer == NULL) {
        *pUEnv << "Failed to create rtsp server ::" << pUEnv->getResultMsg() <<"\n";
        return nullptr;
    }

    AX_CHAR szIP[64] = {0};
    AX_BOOL bGetIPRet = AX_FALSE;
    if (GetIP(&szIP[0], sizeof(szIP))) {
        bGetIPRet = AX_TRUE;
    }

    for (AX_S32 i = 0; i < nMaxNum; i++) {
        AX_U32 nMaxFrmSize = 0;
        AX_PAYLOAD_TYPE_E ePt = PT_H264;
        AX_CHAR strStream[64] = {0};

        auto &stSessionAttr = stRtspAttr[i];
        auto &nChannel = stRtspAttr[i].nChannel;
        auto &pMediaSession = pLiveServerMediaSession[nChannel];

        sprintf(strStream,"axstream%d", nChannel);
        ServerMediaSession* sms = ServerMediaSession::createNew(*pUEnv, strStream, strStream, "Live Stream");

        if (stSessionAttr.stVideoAttr.bEnable) {
            ePt = stSessionAttr.stVideoAttr.ePt;
            nMaxFrmSize = VIDEO_DEFAULT_MAX_FRM_SIZE;
            pMediaSession[RTSP_MEDIA_VIDEO] = AXLiveServerMediaSession::createNewVideo(*pUEnv, true, ePt, nMaxFrmSize);
            sms->addSubsession(pMediaSession[RTSP_MEDIA_VIDEO]);
        }

        if (stSessionAttr.stAudioAttr.bEnable) {
            ePt = stSessionAttr.stAudioAttr.ePt;
            nMaxFrmSize = AUDIO_DEFAULT_MAX_FRM_SIZE;
            AX_U32 nSampleRate = stSessionAttr.stAudioAttr.nSampleRate;
            AX_U8 nChnCnt = stSessionAttr.stAudioAttr.nChnCnt;
            AX_S32 nAOT = stSessionAttr.stAudioAttr.nAOT;
            pMediaSession[RTSP_MEDIA_AUDIO] = AXLiveServerMediaSession::createNewAudio(*pUEnv, true, ePt, nMaxFrmSize, nSampleRate, nChnCnt, nAOT);
            sms->addSubsession(pMediaSession[RTSP_MEDIA_AUDIO]);
        }

        pRtspServer->addServerMediaSession(sms);

        char* url = nullptr;
        if (bGetIPRet) {
            url = new char[140];
            sprintf(url, "rtsp://%s:8554/%s", szIP, strStream);
        } else {
            url = pRtspServer->rtspURL(sms);
        }

        COMM_RTSP_PRT("Play the stream using url: <<<<< %s >>>>>", url);
        delete[] url;
    }

    taskSchedular->doEventLoop(&gStopEventLoop);
    delete(taskSchedular);

    COMM_RTSP_PRT("---");

    return nullptr;
}

AXRtspServer::AXRtspServer(void)
{
    m_uBasePort   = BASE_PORT;
    m_tidServer   = 0;
    m_nMaxNum     = 0;

    m_pUEnv       = NULL;
    m_pRtspServer = NULL;

    for (size_t i = 0; i < MAX_RTSP_CHANNEL_NUM; i++) {
        for (size_t j = 0; j < RTSP_MEDIA_BUTT; j ++) {
            m_pLiveServerMediaSession[i][j] = NULL;
        }
    }
}

AXRtspServer::~AXRtspServer(void)
{
}

AX_BOOL AXRtspServer::Init(const RTSP_ATTR_T *pstAttr, AX_S32 nNum, AX_U16 uBasePort)
{
    if (!pstAttr) {
        COMM_RTSP_PRT("Invalid parameter");
        return AX_FALSE;
    }

    if (uBasePort == 0) {
        uBasePort = BASE_PORT;
    }

    m_uBasePort = uBasePort;
    gStopEventLoop = 0;

    if (nNum <= MAX_RTSP_CHANNEL_NUM) {
        memcpy(m_stRtspAttr, pstAttr, sizeof(RTSP_ATTR_T) * nNum);
    }
    else {
        COMM_RTSP_PRT("exceed max rtsp channel numer");
        return AX_FALSE;
    }
    m_nMaxNum = nNum;

    return AX_TRUE;
}

void AXRtspServer::SendNalu(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts/*=0*/, AX_BOOL bIFrame/*=AX_FALSE*/)
{
    if (m_pLiveServerMediaSession[nChn][RTSP_MEDIA_VIDEO]) {
        m_pLiveServerMediaSession[nChn][RTSP_MEDIA_VIDEO]->SendNalu(nChn, pBuf, nLen, nPts, bIFrame);
    }
}

void AXRtspServer::SendAudio(AX_U32 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts)
{
    if (m_pLiveServerMediaSession[nChn][RTSP_MEDIA_AUDIO]) {
        m_pLiveServerMediaSession[nChn][RTSP_MEDIA_AUDIO]->SendNalu(nChn, pBuf, nLen, nPts, AX_TRUE);
    }
}

AX_BOOL AXRtspServer::Start(void)
{
    if (0 != pthread_create(&m_tidServer, NULL, RtspServerThreadFunc, this)) {
        m_tidServer = 0;
        COMM_RTSP_PRT("pthread_create(RtspServerThreadFunc) fail");
        return AX_FALSE;
    }
    return AX_TRUE;
}

void AXRtspServer::Stop(void)
{
    for (AX_S32 i = 0; i < m_nMaxNum; i++) {
        if (m_pRtspServer) {
            AX_CHAR strStream[64] = {0};
            sprintf(strStream, "axstream%d", m_stRtspAttr[i].nChannel);
            ServerMediaSession* sms = m_pRtspServer->lookupServerMediaSession(strStream);
            if (sms) {
                m_pRtspServer->removeServerMediaSession(sms);
                m_pRtspServer->closeAllClientSessionsForServerMediaSession(sms);
                sms->deleteAllSubsessions();
            }

            for (AX_U32 j = 0; j < RTSP_MEDIA_BUTT; j ++) {
                m_pLiveServerMediaSession[i][j] = nullptr;
            }
        }
    }

    m_pUEnv = NULL;
    gStopEventLoop = 1;
}
