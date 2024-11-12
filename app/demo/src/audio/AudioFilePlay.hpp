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

#include <vector>
#include <queue>
#include "condition_variable.hpp"
#include "AXStage.hpp"
#include "IObserver.h"
#include "ax_codec_comm.h"
#include "AudioPlayDev.hpp"
#include "AudioDecoder.hpp"

typedef enum audioFILE_PLAY_STATUS_E {
    AUDIO_FILE_PLAY_STATUS_COMPLETE,
    AUDIO_FILE_PLAY_STATUS_STOP,
    AUDIO_FILE_PLAY_STATUS_ERROR,
    AUDIO_FILE_PLAY_STATUS_BUTT
} AUDIO_FILE_PLAY_STATUS_E;

typedef struct audioPLAY_FILE_RESULT_T {
    AX_PAYLOAD_TYPE_E eType;
    std::string strFileName;
    AUDIO_FILE_PLAY_STATUS_E eStatus;
    AX_VOID *pUserData;
} AUDIO_FILE_PLAY_RESULT_T, *AUDIO_FILE_PLAY_RESULT_PTR;

typedef AX_VOID (*AUDIO_FILE_PLAY_CALLBACK)(AX_U32 nChannel, const AUDIO_FILE_PLAY_RESULT_PTR pstResult);

typedef struct audioPLAY_FILE_ATTR_T {
    AX_S32 nLoop;
    AX_U64 u64SeqNum;
    AX_PAYLOAD_TYPE_E eType;
    std::string strFileName;
    AX_VOID *pUserData;
    AUDIO_FILE_PLAY_CALLBACK callback;
    AUDIO_FILE_PLAY_STATUS_E eStatus;

    audioPLAY_FILE_ATTR_T () {
        nLoop = 1;
        u64SeqNum = 1;
        eType = PT_AAC;
        strFileName = "";
        pUserData = nullptr;
        callback = nullptr;
        eStatus = AUDIO_FILE_PLAY_STATUS_BUTT;
    }
} AUDIO_FILE_PLAY_ATTR_T, *AUDIO_FILE_PLAY_ATTR_PTR;

class CAudioFilePlay {
public:
    CAudioFilePlay(CAudioPlayDev *pAudioPlayDev, CAudioDecoder *pAudioDecoder, AX_U32 nChannel);
    virtual ~CAudioFilePlay();

    virtual AX_BOOL Start();
    virtual AX_BOOL Stop();
    virtual AX_BOOL Init();
    virtual AX_BOOL DeInit();
    virtual AX_BOOL Play(AX_PAYLOAD_TYPE_E eType,
                        const std::string &strFileName,
                        AX_S32 nLoop,
                        AUDIO_FILE_PLAY_CALLBACK callback,
                        AX_VOID *pUserData);
    virtual AX_BOOL StopPlay();

protected:
    AX_VOID ResultCallbackThread();
    AX_VOID ProcessFileThread();

private:
    AX_VOID ProcessPlayFile(AUDIO_FILE_PLAY_ATTR_PTR pstPlayFileAttr);

private:
    AX_U32 m_nChannel{0};
    CAudioPlayDev *m_pAudioPlayDev{nullptr};
    CAudioDecoder *m_pAudioDecoder{nullptr};

    AX_BOOL m_bStart{AX_FALSE};

    // play file
    AX_BOOL m_bPlayingFile{AX_FALSE};
    AX_BOOL m_bStopPlayFile{AX_FALSE};
    AX_BOOL m_bPlayFileThreadRunning{AX_FALSE};
    AUDIO_FILE_PLAY_STATUS_E m_ePlayFileStatus{AUDIO_FILE_PLAY_STATUS_BUTT};
    std::thread m_hPlayFileThread;
    std::queue<AUDIO_FILE_PLAY_ATTR_T *> m_qPlayFile;
    std::mutex m_mtxPlayFile;
    std::condition_variable m_cvPlayFile;

    // wait play
    std::mutex m_mtxWaitPlayFile;
    std::condition_variable m_cvWaitPlayFile;

    // wait stop
    std::mutex m_mtxWaitStopPlayFile;
    std::condition_variable m_cvWaitStopPlayFile;

    // result callback
    AX_BOOL m_bResultThreadRunning{AX_FALSE};
    std::queue<AUDIO_FILE_PLAY_ATTR_T *> m_qResult;
    std::mutex m_mtxResult;
    std::condition_variable m_cvResult;
    std::thread m_hResultThread;
};
