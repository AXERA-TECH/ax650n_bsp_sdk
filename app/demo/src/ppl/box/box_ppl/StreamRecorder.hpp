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
#include <stdlib.h>
#include <condition_variable>
#include <mutex>
#include <string>
#include "AXThread.hpp"
#include "BoxSataFile.hpp"
#include "IStreamHandler.hpp"

#define GET_SAVE_DIR(root, vdGrp) ((root) + "/vdGrp" + std::to_string(vdGrp) + "/")

typedef struct {
    AX_U32 nCookie;
    AX_U64 nFileSize;
    AX_U64 nMaxSpace;
    std::string strDirPath;
} STREAM_RECORD_ATTR_T;

typedef struct BUFFER_S {
    AX_U8 *buf;
    AX_S32 len;
    AX_S32 cap;

    BUFFER_S(AX_U32 _cap) : cap(_cap) {
        buf = (AX_U8 *)malloc(sizeof(AX_U8) * cap);
        len = 0;
    }

    ~BUFFER_S(AX_VOID) {
        free(buf);
    }
} BUFFER_T, *PBUFFER_T;

class CStreamRecorder : public IStreamObserver {
public:
    CStreamRecorder(AX_VOID) = default;

    AX_BOOL Init(const STREAM_RECORD_ATTR_T &stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL OnRecvVideoData(AX_S32 nCookie, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS) override;
    AX_BOOL OnRecvAudioData(AX_S32 nCookie, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS) override;

protected:
    AX_VOID ProcThread(AX_VOID *);

private:
    STREAM_RECORD_ATTR_T m_stAttr;
    CAXThread m_ProcThread;
    CBoxSataFile m_file;
    PBUFFER_T m_cacheBuf = {nullptr};
    PBUFFER_T m_writeBuf = {nullptr};
    std::condition_variable m_cvWrite;
    std::condition_variable m_cvCache;
    std::mutex m_mtxBuf;
};