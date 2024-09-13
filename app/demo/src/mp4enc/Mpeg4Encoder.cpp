/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "Mpeg4Encoder.h"
#include "AppLogApi.h"

#define MPEG4 "MPEG4"

#define MP4_DEFAULT_RECORD_FILE_NUM (10)
#define MP4_DEFAULT_VIDEO_DEPTH (5)
#define MP4_DEFAULT_AUDIO_DEPTH (5)
#define MP4_DEFAULT_LOOP_SET AX_TRUE

#define MP4_FORMAT_NAME "mp4"

namespace {
// mp4 generate file name callback
AX_VOID mpeg4_gen_file_name_callback(MP4_HANDLE handle, unsigned long long int index, char *file_name, int max_len, void *user_data) {
    CMPEG4Encoder *inst = (CMPEG4Encoder *)user_data;

    if (inst) {
        inst->GenFileName(index, file_name, max_len);
    }
}

// mp4 status callback
AX_VOID mpeg4_status_report_callback(MP4_HANDLE handle, const char *file_name, mp4_status_e status, void *user_data) {
    CMPEG4Encoder *inst = (CMPEG4Encoder *)user_data;

    if (inst) {
        inst->StatusReport(file_name, status);
    }
}
}

CMPEG4Encoder::CMPEG4Encoder() {
}

CMPEG4Encoder::~CMPEG4Encoder() {
}

AX_BOOL CMPEG4Encoder::Init() {
    return AX_TRUE;
}

AX_BOOL CMPEG4Encoder::Start() {
    return AX_TRUE;
}

AX_BOOL CMPEG4Encoder::Stop() {
    if (m_Mp4Handle) {
        mp4_destroy(m_Mp4Handle);
        m_Mp4Handle = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL CMPEG4Encoder::DeInit() {
    Stop();

    return AX_TRUE;
}

AX_BOOL CMPEG4Encoder::InitParam(const MPEG4EC_INFO_T& stMpeg4Info) {
    mp4_info_t mp4_info;
    memset(&mp4_info, 0x00, sizeof(mp4_info));

    m_Chn = stMpeg4Info.nchn;

    mp4_info.loop = (bool)MP4_DEFAULT_LOOP_SET;
    mp4_info.max_file_size = (int)stMpeg4Info.nMaxFileInMBytes;
    mp4_info.max_file_num = MP4_DEFAULT_RECORD_FILE_NUM;
    mp4_info.dest_path = (char *)stMpeg4Info.strSavePath.c_str();
    mp4_info.user_data = this;

    mp4_info.sr_callback = mpeg4_status_report_callback;
    mp4_info.gfn_callback = mpeg4_gen_file_name_callback;

    if (stMpeg4Info.stVideoAttr.bEnable) {
        mp4_info.video.enable = true;
        mp4_info.video.object = (stMpeg4Info.stVideoAttr.ePt) == PT_H264 ? MP4_OBJECT_AVC : MP4_OBJECT_HEVC;
        mp4_info.video.framerate = (int)stMpeg4Info.stVideoAttr.nFrameRate;
        mp4_info.video.bitrate = (int)stMpeg4Info.stVideoAttr.nBitrate;
        mp4_info.video.width = (int)stMpeg4Info.stVideoAttr.nfrWidth;
        mp4_info.video.height = (int)stMpeg4Info.stVideoAttr.nfrHeight;
        mp4_info.video.depth = MP4_DEFAULT_VIDEO_DEPTH;
        mp4_info.video.max_frm_size = stMpeg4Info.stVideoAttr.nMaxFrmSize;
    }

    if (stMpeg4Info.stAudioAttr.bEnable) {
        mp4_info.audio.enable = true;
        if (stMpeg4Info.stAudioAttr.ePt == PT_G711A) {
            mp4_info.audio.object = MP4_OBJECT_ALAW;
        }
        else if (stMpeg4Info.stAudioAttr.ePt == PT_G711U) {
            mp4_info.audio.object = MP4_OBJECT_ULAW;
        }
        else if (stMpeg4Info.stAudioAttr.ePt == PT_AAC) {
            mp4_info.audio.object = MP4_OBJECT_AAC;
        }
        else {
            return AX_FALSE;
        }

        mp4_info.audio.samplerate = (int)stMpeg4Info.stAudioAttr.nSampleRate;
        mp4_info.audio.bitrate = (int)stMpeg4Info.stAudioAttr.nBitrate;
        mp4_info.audio.chncnt = (int)stMpeg4Info.stAudioAttr.nChnCnt;
        mp4_info.audio.aot = (int)stMpeg4Info.stAudioAttr.nAOT;
        mp4_info.audio.depth = MP4_DEFAULT_AUDIO_DEPTH;
        mp4_info.audio.max_frm_size = stMpeg4Info.stAudioAttr.nMaxFrmSize;
    }

    m_Mp4Handle = mp4_create(&mp4_info);

    if (!m_Mp4Handle) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CMPEG4Encoder::SendRawFrame(AX_U8 nChn, AX_VOID* data, AX_U32 size, AX_U64 nPts /*=0*/, AX_BOOL bIFrame /*=AX_FALSE*/) {
    if (0 == mp4_send(m_Mp4Handle, MP4_DATA_VIDEO, data, size, nPts, (bool)bIFrame)) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CMPEG4Encoder::SendAudioFrame(AX_U8 nChn, AX_VOID* data, AX_U32 size, AX_U64 nPts /*=0*/) {
    if (0 == mp4_send(m_Mp4Handle, MP4_DATA_AUDIO, data, size, nPts, true)) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_VOID CMPEG4Encoder::GenFileName(AX_U64 u64Index, AX_CHAR* szFileName, AX_S32 nMaxLen) {
    if (szFileName) {
        time_t rawtime;
        struct tm* ptminfo;
        time(&rawtime);
        ptminfo = localtime(&rawtime);
        snprintf((char*)szFileName, nMaxLen, "chn%d-%02d-%02d-%02d-%02d-%02d-%02d_%lld.%s", m_Chn, ptminfo->tm_year + 1900,
                 ptminfo->tm_mon + 1, ptminfo->tm_mday, ptminfo->tm_hour, ptminfo->tm_min, ptminfo->tm_sec, u64Index, MP4_FORMAT_NAME);
    }
}

AX_VOID CMPEG4Encoder::StatusReport(const AX_CHAR *szFileName, mp4_status_e eStatus) {
    const AX_CHAR* status_str[MP4_STATUS_BUTT] = {"", "start", "complete", "deleted", "failure", "disk full"};

    if (eStatus == MP4_STATUS_FAILURE
        || eStatus == MP4_STATUS_DISK_FULL) {
        LOG_MM_E(MPEG4, "%s status: %s", szFileName ? szFileName: "", status_str[eStatus]);
    }
    else {
        LOG_MM_N(MPEG4, "%s status: %s", szFileName ? szFileName: "", status_str[eStatus]);
    }
}
