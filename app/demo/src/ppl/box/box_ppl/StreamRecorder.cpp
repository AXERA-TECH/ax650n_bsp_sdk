/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "StreamRecorder.hpp"
#include <string.h>
#include <algorithm>
#include <exception>
#include "AppLogApi.h"
#include "fs.hpp"

#define TAG "SATA"

#if 0
static AX_U64 GetTickCount(AX_VOID) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

static AX_VOID normalizeMinMaxAvg(AX_U32 data[], AX_U32 size, AX_U32& min, AX_U32& max, AX_U32& avg) {
    min = data[0];
    max = data[0];
    AX_U32 sum = data[0];

    for (AX_U32 i = 1; i < size; ++i) {
        sum += data[i];

        if (data[i] > max) {
            max = data[i];
        }

        if (data[i] < min) {
            min = data[i];
        }
    }

    avg = sum / size;
}
#endif

AX_VOID CStreamRecorder::ProcThread(AX_VOID*) {
    // constexpr AX_U32 MAX_TICK_NUM = 100;
    // AX_U32 ticks[MAX_TICK_NUM];
    // AX_U32 i = 0;
    while (1) {
        std::unique_lock<std::mutex> lck(m_mtxBuf);
        while (0 == m_writeBuf->len && m_ProcThread.IsRunning()) {
            m_cvWrite.wait(lck);
        }

        if (!m_ProcThread.IsRunning()) {
            break;
        }

        // AX_U64 nTick1 = GetTickCount();
        m_file.Write(m_writeBuf->buf, m_writeBuf->len);
        m_writeBuf->len = 0;

        // ready to swap
        m_cvCache.notify_one();

        // AX_U64 nTick2 = GetTickCount();
        // ticks[i] = (AX_U32)(nTick2 - nTick1);
        // if (++i == MAX_TICK_NUM) {
        //     AX_U32 min, max, avg;
        //     normalizeMinMaxAvg(ticks, MAX_TICK_NUM, min, max, avg);
        //     LOG_M_C(TAG, "vdGrp %02d, MIN = %d, AVG = %d, MAX = %d", m_stAttr.nCookie, min, avg, max);
        //     i = 0;
        // }
    }
}

AX_BOOL CStreamRecorder::OnRecvVideoData(AX_S32 nCookie, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    AX_S32 nLeft = nLen;
    const AX_U8* pBuff = pData;
    while (nLeft > 0) {
        AX_S32 nFree = m_cacheBuf->cap - m_cacheBuf->len;
        if (0 == nFree) {
            // buf is full, wait last write done and swap
            std::unique_lock<std::mutex> lck(m_mtxBuf);
            if (!m_cvCache.wait_for(lck, std::chrono::milliseconds(5), [&]() -> bool { return (0 == m_writeBuf->len); })) {
                LOG_M_W(TAG, "vdGrp%02d last write %d bytes timeout, abandon %d bytes", m_stAttr.nCookie, m_writeBuf->len, nLeft);
                break;
            }

            std::swap(m_cacheBuf, m_writeBuf);
            nFree = m_cacheBuf->cap;

            // start to write new buffer
            m_cvWrite.notify_one();
        }

        AX_S32 nSize = (nLeft > nFree) ? nFree : nLeft;
        if (nSize > 0) {
            memcpy(&m_cacheBuf->buf[m_cacheBuf->len], pBuff, nSize);
            m_cacheBuf->len += nSize;
            nLeft -= nSize;
            pBuff += nSize;
        } else {
            LOG_M_E(TAG, "vdGrp%02d data: len = %d, free = %d, size = %d, cachebuf: cap = %d, len = %d", m_stAttr.nCookie, nLen, nFree,
                    nSize, m_cacheBuf->cap, m_cacheBuf->len);
        }
    }

    return AX_TRUE;
}

AX_BOOL CStreamRecorder::OnRecvAudioData(AX_S32 nCookie, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    return AX_TRUE;
}

AX_BOOL CStreamRecorder::Init(const STREAM_RECORD_ATTR_T& stAttr) {
    fs::path dir(stAttr.strDirPath);
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        LOG_M_E(TAG, "< %s > not exist or not directory, please mount at first!", stAttr.strDirPath.c_str());
        return AX_FALSE;
    }

    m_cacheBuf = new (std::nothrow) BUFFER_T(512 * 1024);
    if (!m_cacheBuf) {
        LOG_M_E(TAG, "alloc cache buffer fail");
        return AX_FALSE;
    }

    m_writeBuf = new (std::nothrow) BUFFER_T(512 * 1024);
    if (!m_writeBuf) {
        LOG_M_E(TAG, "alloc write buffer fail");
        delete m_cacheBuf;
        m_cacheBuf = nullptr;

        return AX_FALSE;
    }

    m_stAttr = stAttr;
    return AX_TRUE;
}

AX_BOOL CStreamRecorder::DeInit(AX_VOID) {
    if (m_cacheBuf) {
        delete m_cacheBuf;
        m_cacheBuf = nullptr;
    }

    if (m_writeBuf) {
        delete m_writeBuf;
        m_writeBuf = nullptr;
    }

    return AX_TRUE;
}

AX_BOOL CStreamRecorder::Start(AX_VOID) {
    BOX_SATA_FILE_ATTR_T stAttr;
    stAttr.nCookie = m_stAttr.nCookie;
    stAttr.nFileSize = m_stAttr.nFileSize;
    stAttr.nMaxSpace = m_stAttr.nMaxSpace;
    stAttr.bSyncIO = AX_FALSE;
    stAttr.strDirPath = GET_SAVE_DIR(m_stAttr.strDirPath, m_stAttr.nCookie);
    if (!m_file.Open(stAttr)) {
        return AX_FALSE;
    }

    AX_CHAR szName[16];
    sprintf(szName, "AppSATA%d", m_stAttr.nCookie);
    if (!m_ProcThread.Start([this](AX_VOID* pArg) -> AX_VOID { ProcThread(pArg); }, this, szName)) {
        m_file.Close();
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CStreamRecorder::Stop(AX_VOID) {
    m_ProcThread.Stop();
    {
        std::lock_guard<std::mutex> lck(m_mtxBuf);
        m_cvWrite.notify_one();
    }
    m_ProcThread.Join();

    m_file.Close();
    return AX_TRUE;
}
