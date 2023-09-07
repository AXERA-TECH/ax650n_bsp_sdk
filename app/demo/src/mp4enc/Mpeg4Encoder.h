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
#include <string>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include "ax_global_type.h"
#include "mp4_api.h"

typedef enum axMPEG4_SESS_MEDIA_TYPE_E {
    MPEG4_SESS_MEDIA_VIDEO,
    MPEG4_SESS_MEDIA_AUDIO,
    MPEG4_SESS_MEDIA_BUTT
} MPEG4_SESS_MEDIA_TYPE_E;

typedef struct MPEG4EC_INFO_S {
    AX_U32 nchn;
    AX_U32 nMaxFileInMBytes;
    std::string strSavePath;

    struct mpeg4_video_sess_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_U32 nFrameRate;
        AX_S32 nfrWidth;
        AX_S32 nfrHeight;
        AX_S32 nBitrate;
        AX_U32 nMaxFrmSize;
    } stVideoAttr;
    struct mpeg4_audio_sess_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_S32 nBitrate;
        AX_U32 nMaxFrmSize;
        AX_U32 nSampleRate;
        AX_U8 nChnCnt;
        AX_S32 nAOT; // audio object type
    } stAudioAttr;

    MPEG4EC_INFO_S() {
        nchn = 0;
        nMaxFileInMBytes = 0;
        memset(&stVideoAttr, 0x00, sizeof(stVideoAttr));
        memset(&stAudioAttr, 0x00, sizeof(stAudioAttr));
    }
} MPEG4EC_INFO_T;

class CMPEG4Encoder {
public:
    CMPEG4Encoder();
    virtual ~CMPEG4Encoder();

    virtual AX_BOOL Init();
    virtual AX_BOOL DeInit();
    virtual AX_BOOL Start();
    virtual AX_BOOL Stop();

    AX_BOOL InitParam(const MPEG4EC_INFO_T& stMpeg4Info);
    AX_BOOL SendRawFrame(AX_U8 nChn, AX_VOID* data, AX_U32 size, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_FALSE);
    AX_BOOL SendAudioFrame(AX_U8 nChn, AX_VOID* data, AX_U32 size, AX_U64 nPts = 0);

    AX_VOID GenFileName(AX_U64 u64Index, AX_CHAR* szFileName, AX_S32 nMaxLen);
    AX_VOID StatusReport(const AX_CHAR *szFileName, mp4_status_e eStatus);

public:
    AX_U8 m_Chn;
    MP4_HANDLE m_Mp4Handle{nullptr};
};
