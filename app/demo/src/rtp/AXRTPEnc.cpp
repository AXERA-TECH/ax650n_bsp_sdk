/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/reboot.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/prctl.h>

#include "AXRTPEnc.h"

static AX_S32 MediaQueueCreate(AX_MEDIA_QUEUE_T *pstMediaQueue);
static AX_S32 MediaQueuePush(AX_MEDIA_QUEUE_T *pstMediaQueue, AX_MEDIA_DATA_T stMediaData);
static AX_S32 MediaQueueIsEmpty(AX_MEDIA_QUEUE_T *pstMediaQueue);
static AX_S32 MediaQueuePop(AX_MEDIA_QUEUE_T *pstMediaQueue, AX_MEDIA_DATA_T *pstMediaData);
static AX_S32 MediaQueueDestory(AX_MEDIA_QUEUE_T *pstMediaQueue);

static AX_S32 AX_RTPENC_Send(AX_RTP_ENC_T *pRtpEnc, AX_U8 *inBuf, size_t inLen);
static AX_S32 AX_RTPENC_H264(AX_RTP_ENC_T *pRtpEnc, AX_U8 *inBuf, AX_S32 inLen, AX_U32 pcr, AX_U64 u64SeqNum);

void *RtpEnc_MediaData_Proc(void *args)
{
    AX_RTP_ENC_T *pRtpEnc = (AX_RTP_ENC_T *)args;

    AX_MEDIA_DATA_T stMediaData;
    prctl(PR_SET_NAME, "rtp lib");
    while(pRtpEnc->nThreadStart)
    {
        memset(&stMediaData, 0, sizeof(AX_MEDIA_DATA_T));
        pthread_mutex_lock(&pRtpEnc->mutexMediaQueue);
        if(pRtpEnc->nCondShareVariable == 0x0)
        {
            pthread_cond_wait(&pRtpEnc->condMediaQueue, &pRtpEnc->mutexMediaQueue);
        }
        MediaQueuePop(&pRtpEnc->stMediaQueue, &stMediaData);
        pRtpEnc->nCondShareVariable--;
        pthread_mutex_unlock(&pRtpEnc->mutexMediaQueue);
        if(stMediaData.mediaDataLen == 0)
        {
            continue;
        }


        if(stMediaData.type == PT_H264)
        {
            AX_RTPENC_H264(pRtpEnc, stMediaData.mediaData, stMediaData.mediaDataLen, stMediaData.timestamp, stMediaData.u64SeqNum);
        }

        if(stMediaData.mediaData != NULL)
        {
            free(stMediaData.mediaData);
        }
    }
    return NULL;
}

AX_S32 AX_RTPENC_Init(AX_RTP_ENC_T *pRtpEnc, AX_CHAR *ipAddr, AX_U16 ipPort, AX_S32 chn) {
    AX_S32 sockfd;
    AX_S32 sendBufSize = 1024*1024;
    AX_S32 s32Ret;

    memset(pRtpEnc, 0, sizeof(AX_RTP_ENC_T));

    if(strlen(ipAddr) == 0x0)
    {
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM,0);
    if(sockfd < 0)
    {
        printf("create socket failed\n");
        return -1;
    }

    if((htonl(inet_addr(ipAddr)) > 0xE0000000) && (htonl(inet_addr(ipAddr)) < 0xEFFFFFFF))
    {
        printf("this is a group address\n");
    }

    s32Ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(int));
    if(s32Ret != 0)
    {
        printf("setsockopt SO_SNDBUF failed\n");
        close(sockfd);
        return -1;
    }

    bzero(&pRtpEnc->deskaddr, sizeof(pRtpEnc->deskaddr));
	pRtpEnc->deskaddr.sin_family = AF_INET;
	pRtpEnc->deskaddr.sin_port = htons(ipPort);
    pRtpEnc->deskaddr.sin_addr.s_addr=inet_addr(ipAddr);

    pRtpEnc->nSockfd = sockfd;
    pRtpEnc->nChannel = chn;

    MediaQueueCreate(&pRtpEnc->stMediaQueue);
    pthread_mutex_init(&pRtpEnc->mutexMediaQueue, NULL);
    pthread_cond_init(&pRtpEnc->condMediaQueue, NULL);
    pRtpEnc->nCondShareVariable = 0;
    pRtpEnc->nThreadStart = 1;

    pthread_create(&pRtpEnc->mediaDataPid, NULL, RtpEnc_MediaData_Proc, (void *)pRtpEnc);

    return 0;
}

AX_S32 AX_RTPENC_DeInit(AX_RTP_ENC_T *pRtpEnc) {
    if(pRtpEnc->nSockfd > 0)
    {
        close(pRtpEnc->nSockfd);
    }
    return 0;
}

AX_S32 AX_RTPENC_Proc(AX_RTP_ENC_T *pRtpEnc, AX_U8 *inBuf, AX_S32 inLen, AX_U64 pcr, AX_S32 type, AX_S32 chn, AX_U64 u64SeqNum) {
    (void)chn;

    AX_MEDIA_DATA_T stMediaData;
    stMediaData.mediaData = (AX_U8 *)malloc(inLen);
    memcpy(stMediaData.mediaData, inBuf, inLen);
    stMediaData.mediaDataLen = inLen;
    stMediaData.timestamp = (pcr*9)/100;
    stMediaData.type = type;
    stMediaData.u64SeqNum = u64SeqNum;

    pthread_mutex_lock(&pRtpEnc->mutexMediaQueue);
    MediaQueuePush(&pRtpEnc->stMediaQueue,stMediaData);
    pRtpEnc->nCondShareVariable++;
    pthread_cond_signal(&pRtpEnc->condMediaQueue);
    pthread_mutex_unlock(&pRtpEnc->mutexMediaQueue);

    return 0;
}


AX_S32 AX_RTPENC_Send(AX_RTP_ENC_T *pRtpEnc, AX_U8 *inBuf, size_t inLen)
{
    AX_S32 nRet;
    if(pRtpEnc->nSockfd > 0)
    {
        nRet = sendto(pRtpEnc->nSockfd, inBuf, inLen, 0, (struct sockaddr *)&pRtpEnc->deskaddr, sizeof(pRtpEnc->deskaddr));
        if (nRet != inLen) {
            printf("sendto--->error, nRet = %d, inLen = %d\n", nRet, inLen);
        }
    }
    return 0;
}

AX_S32 AX_RTPENC_H264(AX_RTP_ENC_T *pRtpEnc, AX_U8 *inBuf, AX_S32 inLen, AX_U32 pcr, AX_U64 u64SeqNum)
{
	AX_U8 sendBuf[2048];
	AX_S32 sendBufLen;
	RTP_HEADER *rtpHdr;
	AX_S32 m,n,k;
	AX_S32 sendDataLen;
	AX_S32 h264_current =0;

	memset(sendBuf, 0, 2048);

	rtpHdr = (RTP_HEADER*)&sendBuf[0];
	rtpHdr->v_p_x_cc = 0x80;
	rtpHdr->payload = PT_H264;
	rtpHdr->ssrc = 0xCDBA0264;
	h264_current = pcr;
	rtpHdr->timestamp = htonl(h264_current);
    rtpHdr->u64SeqNum = u64SeqNum;

	sendDataLen = inLen - 4;
	if(sendDataLen <= 1400)
	{
		rtpHdr->payload |= (0x1 << 7);
        pRtpEnc->nSeqNum++;
        rtpHdr->seq_num = htons(pRtpEnc->nSeqNum);

        memcpy(sendBuf + AX_RTP_HEADER_LEN, inBuf + 4, sendDataLen);
		sendBufLen = AX_RTP_HEADER_LEN + sendDataLen;

		AX_RTPENC_Send(pRtpEnc, sendBuf, sendBufLen);
    }
	else
	{

		m = (sendDataLen - 1) / 1400;
		n = (sendDataLen - 1) % 1400;

		for(k = 0; k < m; k++)
		{
            pRtpEnc->nSeqNum++;
            rtpHdr->seq_num = htons(pRtpEnc->nSeqNum);

            if((k == (m -1)) && (n == 0))
			{
				rtpHdr->payload |= (0x1 << 7);
			}
			else
			{
				rtpHdr->payload &= ~(0x1 << 7);
			}
			sendBuf[AX_RTP_HEADER_LEN] = inBuf[4] & 0xE0;
			sendBuf[AX_RTP_HEADER_LEN] |= 28;

			if(k == 0)
			{
                if (n == 0 && m == 1) {
                    sendBuf[AX_RTP_HEADER_LEN + 1] = 0x40 | (inBuf[4] & 0x1f);
                } else {
                    sendBuf[AX_RTP_HEADER_LEN + 1] = 0x80 | (inBuf[4] & 0x1f);
                }
            }
			else if(k == (m - 1) && (n == 0))
			{
				sendBuf[AX_RTP_HEADER_LEN + 1] = 0x40 | (inBuf[4] & 0x1f);
			}
			else
			{
				sendBuf[AX_RTP_HEADER_LEN + 1] =  inBuf[4] & 0x1f;
			}

			sendBufLen = AX_RTP_HEADER_LEN + 2 + 1400;
			memcpy(sendBuf + AX_RTP_HEADER_LEN + 2, inBuf + 5 + k*1400, 1400);
			AX_RTPENC_Send(pRtpEnc, sendBuf, sendBufLen);
		}

		if(n > 0)
		{
            pRtpEnc->nSeqNum++;
            rtpHdr->seq_num = htons(pRtpEnc->nSeqNum);

            rtpHdr->payload |= (0x1 << 7);
			sendBuf[AX_RTP_HEADER_LEN] = inBuf[4] & 0xE0;
			sendBuf[AX_RTP_HEADER_LEN] |= 28;
			sendBuf[AX_RTP_HEADER_LEN + 1] = 0x40 | (inBuf[4] & 0x1f);

			memcpy(sendBuf + AX_RTP_HEADER_LEN + 2, inBuf + 5 + m*1400, n);
			sendBufLen = AX_RTP_HEADER_LEN + 2 + n;
			AX_RTPENC_Send(pRtpEnc, sendBuf, sendBufLen);
		}
	}

	return 0;
}

AX_S32 MediaQueueCreate(AX_MEDIA_QUEUE_T *pstMediaQueue)
{
    pstMediaQueue->front = (AX_MEDIA_QUEUE_NODE_T *)malloc(sizeof(AX_MEDIA_QUEUE_NODE_T));
    if(pstMediaQueue->front == NULL)
    {
        printf("media queue malloc failed\n");
        return -1;
    }
    pstMediaQueue->rear = pstMediaQueue->front;
    pstMediaQueue->front->next = NULL;
    return 0;
}

AX_S32 MediaQueuePush(AX_MEDIA_QUEUE_T *pstMediaQueue, AX_MEDIA_DATA_T stMediaData)
{

    AX_MEDIA_QUEUE_NODE_T *pstMediaQueueNode;

    pstMediaQueueNode = (AX_MEDIA_QUEUE_NODE_T *)malloc(sizeof(AX_MEDIA_QUEUE_NODE_T));
    if (pstMediaQueueNode == NULL)
    {
        printf("Create media queue node failed\n");
        return -1;
    }

    pstMediaQueueNode->stMediaData = stMediaData;
    pstMediaQueueNode->next = NULL;

    pstMediaQueue->rear->next = pstMediaQueueNode;
    pstMediaQueue->rear = pstMediaQueueNode;

    return 0;
}

AX_S32 MediaQueueIsEmpty(AX_MEDIA_QUEUE_T *pstMediaQueue)
{
    AX_S32 s32Ret;
    if(pstMediaQueue->front == pstMediaQueue->rear)
    {
        s32Ret = 1;
    }
    else
    {
        s32Ret = 0;
    }
    return s32Ret;
}

AX_S32 MediaQueuePop(AX_MEDIA_QUEUE_T *pstMediaQueue, AX_MEDIA_DATA_T *pstMediaData)
{
    AX_MEDIA_DATA_T stMediaData;
    AX_MEDIA_QUEUE_NODE_T *pstMediaQueueNode;

    memset(&stMediaData, 0, sizeof(stMediaData));

    if (MediaQueueIsEmpty(pstMediaQueue) == 1)
    {
        printf("MediaQueue_IsEmpty\n");
        return -1;
    }

    pstMediaQueueNode = pstMediaQueue->front->next;
    if(pstMediaQueueNode == NULL)
    {
        return -1;
    }
    if (pstMediaQueue->front->next == pstMediaQueue->rear)
    {
       pstMediaQueue->rear = pstMediaQueue->front;
       pstMediaQueue->front->next = NULL;
    }
    else
    {
        pstMediaQueue->front->next = pstMediaQueue->front->next->next;
    }

    *pstMediaData = pstMediaQueueNode->stMediaData;
    free(pstMediaQueueNode);
    return 0;
}

AX_S32 MediaQueueDestory(AX_MEDIA_QUEUE_T *pstMediaQueue)
{
    AX_S32 s32Ret;
    AX_MEDIA_DATA_T stMediaData;
    memset(&stMediaData, 0, sizeof(AX_MEDIA_DATA_T));
    while(1)
    {
        s32Ret = MediaQueuePop(pstMediaQueue, &stMediaData);
        if( s32Ret != 0x0)
        {
            break;
        }
        else
        {
            if(stMediaData.mediaData != NULL)
            {
                free(stMediaData.mediaData);
                stMediaData.mediaData = NULL;
            }
        }
    }

    return 0;
}
