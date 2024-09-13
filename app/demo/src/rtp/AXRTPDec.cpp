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
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/prctl.h>
#include "AXRTPDec.h"

#ifdef TEST_LATENCY
#include "ax_sys_api.h"
#endif


static AX_S32 AX_RTPDEC_H264Header(AX_U8 *outBuf)
{
	outBuf[0] = 0x00;
	outBuf[1] = 0x00;
	outBuf[2] = 0x00;
	outBuf[3] = 0x01;
	return 4;
}

static AX_S32 AX_RTPDEC_H264(AX_RTP_DEC_T *pRtpDec, AX_U8 *inBuf, AX_S32 inLen) {
    AX_S32 padding, mFlag;
    AX_S32 remainLen;

    if(inLen < AX_RTP_HEADER_LEN)
    {
        return -1;
    }

    if((inBuf[0] >> 6) != 2)
    {
        return -1;
    }

    remainLen = inLen;
    padding = 0x0;
    if(inBuf[0] & 0x20)
    {
        padding = inBuf[remainLen - 1];
        remainLen -= padding;
    }

    mFlag = (inBuf[1] >> 7) & 0x1;
    if((inBuf[AX_RTP_HEADER_LEN] & 0x1f) != 0x1C)
	{
		pRtpDec->outBufLen = AX_RTPDEC_H264Header(pRtpDec->outBuf);
		memcpy(pRtpDec->outBuf + pRtpDec->outBufLen, inBuf + AX_RTP_HEADER_LEN, remainLen - AX_RTP_HEADER_LEN);
		memcpy(&pRtpDec->u64SeqNum, inBuf + AX_RTP_HEADER_LEN - 8, 8);
		pRtpDec->outBufLen += remainLen - AX_RTP_HEADER_LEN;
#ifdef TEST_LATENCY
		unsigned long long int curPts = 0x0;
		int ret = -1;
		ret = AX_SYS_GetCurPTS(&curPts);
		if (0 != ret) {
			printf("AX_SYS_GetCurPTS failed!\n");
			return -1;
		}
		pRtpDec->pts = curPts;
#endif
		pRtpDec->cbVideoFrameOut(pRtpDec->clientData, pRtpDec->outBuf, pRtpDec->outBufLen, pRtpDec->nChannel);
#ifdef	TEST_LATENCY
		pRtpDec->sequence += 1;
#endif
		pRtpDec->outBufLen = 0;
	}
	else
	{
		if((mFlag == 0x0) && ((inBuf[AX_RTP_HEADER_LEN] & 0x1f) == 0x1C) && ((inBuf[AX_RTP_HEADER_LEN + 1] >> 6) == 0x02))
		{
			pRtpDec->outBufLen = AX_RTPDEC_H264Header(pRtpDec->outBuf);
			pRtpDec->outBuf[pRtpDec->outBufLen] = inBuf[AX_RTP_HEADER_LEN] & 0xE0;
			pRtpDec->outBuf[pRtpDec->outBufLen] |= inBuf[AX_RTP_HEADER_LEN + 1] & 0x1F;
			pRtpDec->outBufLen += 1;
			memcpy(pRtpDec->outBuf + pRtpDec->outBufLen, inBuf + AX_RTP_HEADER_LEN + 2, remainLen - AX_RTP_HEADER_LEN - 2);
			memcpy(&pRtpDec->u64SeqNum, inBuf + AX_RTP_HEADER_LEN - 8, 8);
			pRtpDec->outBufLen += remainLen - AX_RTP_HEADER_LEN - 2;
		}
		else if((mFlag == 0x0) && ((inBuf[AX_RTP_HEADER_LEN] & 0x1f) == 0x1C) && ((inBuf[AX_RTP_HEADER_LEN + 1] >> 6) == 0x00))
		{
			memcpy(pRtpDec->outBuf + pRtpDec->outBufLen, inBuf + AX_RTP_HEADER_LEN + 2, remainLen - AX_RTP_HEADER_LEN - 2);
			pRtpDec->outBufLen += remainLen - AX_RTP_HEADER_LEN - 2;
		}
		else if((mFlag == 0x1) && ((inBuf[AX_RTP_HEADER_LEN] & 0x1f) == 0x1C) &&  ((inBuf[AX_RTP_HEADER_LEN + 1] >> 6) == 0x01))
		{
			memcpy(pRtpDec->outBuf + pRtpDec->outBufLen, inBuf + AX_RTP_HEADER_LEN + 2, remainLen - AX_RTP_HEADER_LEN - 2);
			pRtpDec->outBufLen += remainLen - AX_RTP_HEADER_LEN - 2;
#ifdef TEST_LATENCY
			unsigned long long int curPts = 0x0;
			int ret = -1;
			ret = AX_SYS_GetCurPTS(&curPts);
			if (0 != ret) {
				printf("AX_SYS_GetCurPTS failed!\n");
				return -1;
			}
			pRtpDec->pts = curPts;
#endif
			pRtpDec->cbVideoFrameOut(pRtpDec->clientData, pRtpDec->outBuf, pRtpDec->outBufLen, pRtpDec->nChannel);
#ifdef TEST_LATENCY
			pRtpDec->sequence += 1;
#endif
			pRtpDec->outBufLen = 0;
		}
	}

    return 0;
}

AX_S32 AX_RTPDEC_Init(AX_RTP_DEC_T *pRtpDec, AX_VOID* data, AX_S32 chn, AX_PAYLOAD_TYPE_E enMediaType, videoFrameOut cbVideoFrameOut, audioFrameOut cbAudioFrameOut)
{
    memset(pRtpDec, 0, sizeof(AX_RTP_DEC_T));
    pRtpDec->clientData = data;
#ifdef TEST_LATENCY
    pRtpDec->sequence = 0x0;
    pRtpDec->pts = 0x0;
#endif
    pRtpDec->u64SeqNum = 0x0;
    pRtpDec->nChannel = chn;
    pRtpDec->enAVMediaType = enMediaType;
    pRtpDec->cbVideoFrameOut = cbVideoFrameOut;
    pRtpDec->cbAudioFrameOut = cbAudioFrameOut;
    pRtpDec->outBuf = (AX_U8 *)malloc(1024*1024*2);
    return 0;
}

AX_S32 AX_RTPDEC_Proc(AX_RTP_DEC_T *pRtpDec, AX_U8 *inBuf, AX_S32 inLen)
{
    AX_S32 payloadType;
    if(inLen <= AX_RTP_HEADER_LEN)
    {
        return -1;
    }

    payloadType = inBuf[1] & 0x7F;
    switch(payloadType)
    {
        case 0:
            break;
        case 8:
            break;
        default:
            if(pRtpDec->enAVMediaType == PT_H264)
            {
                AX_RTPDEC_H264(pRtpDec, (AX_U8 *)inBuf, inLen);
            }
            else
            {
                printf("not support avMedia type!");
            }
            break;
    }

    return 0;
}

AX_S32 AX_RTPDEC_DeInit(AX_RTP_DEC_T *pRtpDec)
{
    if(NULL != pRtpDec->outBuf){
        free(pRtpDec->outBuf);
        pRtpDec->outBuf = NULL;
    }
    memset(pRtpDec, 0, sizeof(AX_RTP_DEC_T));
    return 0;
}



