/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "datastreamfile.hpp"
#include "ElapsedTimer.hpp"
#include "AppLog.hpp"
#include "fs.hpp"
#include <fcntl.h>
#include <unistd.h>

#define DSF "DATA_STM_FILE"

using namespace std;

CDataStreamFile::CDataStreamFile(AXDSF_INIT_ATTR_T& tInitAttr)
: m_tInitAttr(tInitAttr) {
    Init();
}

CDataStreamFile::CDataStreamFile() {

}

CDataStreamFile::~CDataStreamFile() {
    DeInit();
}

AX_VOID CDataStreamFile::WriteThread(AX_VOID* pArg) {
    while (m_threadWrite.IsRunning()) {
        if (0 == m_tBufWrite.nSize) {
            m_hWriteStartEvent.WaitEvent();
            m_hWriteStartEvent.ResetEvent();
        }

        if (m_tBufWrite.nSize > 0) {
            do {
                std::lock_guard<std::mutex> lck(m_mtxWriting);
                write(m_hFD, m_tBufWrite.pBuf, m_tBufWrite.nSize);
                m_tBufWrite.nSize = 0;
            } while (0);

            m_hWriteCompleteEvent.SetEvent();
        }
    }
}

AX_BOOL CDataStreamFile::Init() {
    LOG_MM_I(DSF, "+++");

    if (!AllocFileTailBuf()) {
        return AX_FALSE;
    }

    LOG_MM_I(DSF, "---");
    return AX_TRUE;
}

AX_BOOL CDataStreamFile::DeInit() {
    if (!FreeFileTailBuf()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_S32 CDataStreamFile::Open(const AX_CHAR* pszFile, AX_DSF_OPEN_FLAG_E eOpenFlag /*= AX_DSF_OPEN_FOR_READ*/) {
    LOG_MM_I(DSF, "+++");

    m_nTotalCount = 0;
    m_tBufWrite.nSize = 0;
    m_tBufCache.nSize = 0;
    m_tFrameOffsetInfo.nCount = 0;
    m_tIFrameOffsetInfo.nCount = 0;
    m_nSPSOffset = -1;

    if (AX_DSF_OPEN_FOR_READ == eOpenFlag) {
        m_hFD = open(pszFile, O_RDONLY, S_IRUSR | S_IWUSR);
    } else if (AX_DSF_OPEN_FOR_WRITE == eOpenFlag) {
        m_hFD = open(pszFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    } else {
        return AX_DS_INVALID_HANDLE;
    }

    if (AX_DS_INVALID_HANDLE == m_hFD) {
        LOG_M_E(DSF, "Open data file %s failed, err=%s", pszFile, strerror(errno));
        return AX_DS_INVALID_HANDLE;
    } else {
        strcpy(m_szFilePath, pszFile);

        // if (fallocate(m_hFD, FALLOC_FL_KEEP_SIZE, 0, 100 * 1024 * 1024) < 0) {
        //     LOG_M_E(DSF, "fallocate %s failed, err: %s", m_szFilePath, strerror(errno));
        //     return AX_DS_INVALID_HANDLE;
        // }
    }

    if (AX_DSF_OPEN_FOR_WRITE == eOpenFlag) {
        if (!StartWriteThread()) {
            return AX_DS_INVALID_HANDLE;
        }

        if (!WriteFileHeader()) {
            return AX_DS_INVALID_HANDLE;
        }
    } else if (AX_DSF_OPEN_FOR_READ == eOpenFlag) {
        AX_S32 nRead = read(m_hFD, &m_tFileHeader, sizeof(AXDS_FILE_HEADER_T));
        if (nRead != sizeof(AXDS_FILE_HEADER_T)) {
            LOG_MM_E(DSF, "Read file header failed, ret=%d, err=%s.", nRead, strerror(errno));
            return AX_DS_INVALID_HANDLE;
        } else {
            LOG_MM_D(DSF, "tail offset start: 0x%08X", m_tFileHeader.uTailOffset);
        }

        /* Read and save frame offset info */
        m_tFrameOffsetInfo.nCount = m_tFileHeader.uFrameCount;
        m_tFrameOffsetInfo.pOffsetStart = (AX_U32*)malloc(m_tFrameOffsetInfo.nCount * sizeof(AX_U32));

        if (!m_tFrameOffsetInfo.pOffsetStart) {
            LOG_MM_E(DSF, "Malloc buff for saving frame offset info failed.");
            return AX_FALSE;
        } else {
            LOG_MM_D(DSF, "File tail ptr: %p, size: %d", m_tFrameOffsetInfo.pOffsetStart, (AX_U32)(m_tFrameOffsetInfo.nCount * sizeof(AX_U32)));
        }

        AX_U32 nFrmOffset = m_tFileHeader.uTailOffset + sizeof(AXDS_FILE_TAIL_T) - sizeof(AX_U32*);
        AX_S32 nPos = lseek(m_hFD, nFrmOffset, SEEK_SET);
        if (-1 == nPos) {
            LOG_MM_E(DSF, "Seek to frame offset start pos 0x%08X failed, err=%s", nFrmOffset, strerror(errno));
            return AX_DS_INVALID_HANDLE;
        } else {
            LOG_MM_D(DSF, "Seek to frame offset start pos 0x%08X successfully.", nFrmOffset);
        }

        AX_U32 nFrmOffsetSize = m_tFrameOffsetInfo.nCount * sizeof(AX_U32);
        nRead = read(m_hFD, m_tFrameOffsetInfo.pOffsetStart, nFrmOffsetSize);
        if (nRead != (AX_S32)nFrmOffsetSize) {
            LOG_MM_E(DSF, "Read frame offset info(target size: %d) failed, ret=%d, err=%s.", nFrmOffsetSize, nRead, strerror(errno));
            return AX_DS_INVALID_HANDLE;
        } else {
            LOGBUF(APP_LOG_DEBUG, m_tFrameOffsetInfo.pOffsetStart, nFrmOffsetSize, APP_LOG_SYNC_RECV);
        }
    }

    LOG_MM_I(DSF, "---");

    return m_hFD;
}

AX_BOOL CDataStreamFile::Close() {
    LOG_MM_I(DSF, "+++");
    if (IsOpened()) {
        m_bClosing = AX_TRUE;

        if (m_threadWrite.IsRunning()) {
            Flush();
            StopWriteThread();

            /* Write file tail with fd directly */
            WriteFileTail();
            /* Header refresh must be the last operation because the fp will not be recovered */
            UpdateFileHeader();
        } else {
            FreeFileTailBuf();
        }

        close(m_hFD);
        m_hFD = AX_DS_INVALID_HANDLE;

        m_bClosing = AX_FALSE;
    }
    LOG_MM_I(DSF, "---");
    return AX_TRUE;
}

AX_BOOL CDataStreamFile::WriteFrame(const AX_VOID* pData, AX_U32 nSize, AXDS_FRAME_HEADER_T& tFrameHeader) {
    if (!IsOpened() || m_bClosing || nullptr == pData || 0 == nSize) {
        LOG_MM_E(DSF, "error");
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxWrite);

    if (AX_FALSE == m_bGopRetrieved) {
        if (NALU_TYPE_IDR == tFrameHeader.GetNaluType()) {
            if (m_nGOPCalulated > 0) {
                m_bGopRetrieved = AX_TRUE;
            } else {
                m_nGOPCalulated++;
            }
        } else if (NALU_TYPE_OTH == tFrameHeader.GetNaluType()) {
            if (m_nGOPCalulated > 0) {
                m_nGOPCalulated++;
            }
        }
    }

    /* Ensure that the frame header and data would be within the same cache buffer */
    if (m_tBufCache.nSize + sizeof(AXDS_FRAME_HEADER_T) + m_tSPSBufCache.nSize + nSize > MAX_WRITE_CACHE_SIZE) {
        SwapBuf();

        /* Enable starting of writing file */
        m_hWriteStartEvent.SetEvent();
    }

    /* In case of SPS/PPS/VPS, save data without header */
    if (tFrameHeader.IsParamSet()) {
        memcpy(m_tSPSBufCache.pBuf + m_tSPSBufCache.nSize, pData, nSize);
        m_tSPSBufCache.nSize += nSize;
    } else {
        if (!FillFrameHeader(nSize, tFrameHeader)) {
            return AX_FALSE;
        }
    }

    AXDS_FRM_TYPE_E eFrmType = (AXDS_FRM_TYPE_E)tFrameHeader.uFrameType;
    if (m_tFrameOffsetInfo.nCount < m_nEstimatedMaxFrameCount) {
        if (AXDS_FRM_TYPE_VIDEO_IDR == eFrmType) {
            memcpy(m_tFrameOffsetInfo.pOffsetStart + m_tFrameOffsetInfo.nCount, &m_nTotalCount, sizeof(AX_U32));

            if (-1 == m_nSPSOffset) {

                memcpy(m_tIFrameOffsetInfo.pOffsetStart + m_tIFrameOffsetInfo.nCount, &m_nTotalCount, sizeof(AX_U32));
            } else {
                /* Relocate to the recorded offset of SPS/PPS/VPS frame */
                memcpy(m_tIFrameOffsetInfo.pOffsetStart + m_tIFrameOffsetInfo.nCount, &m_nSPSOffset, sizeof(AX_U32));
            }

            m_tIFrameOffsetInfo.nCount += 1;
        } else if (AXDS_FRM_TYPE_VIDEO_SPS == eFrmType || AXDS_FRM_TYPE_VIDEO_PPS == eFrmType || AXDS_FRM_TYPE_VIDEO_VPS == eFrmType) {
            if (-1 == m_nSPSOffset) {
                /* Record offset of first SPS/PPS/VPS frame */
                m_nSPSOffset = m_nTotalCount;
            }
        } else {
            memcpy(m_tFrameOffsetInfo.pOffsetStart + m_tFrameOffsetInfo.nCount, &m_nTotalCount, sizeof(AX_U32));

            m_nSPSOffset = -1; /* if P/B frame exists between SPS/PPS/VPS and I, ignore SPS/PPS/VPS */
        }
    } else {
        LOG_M_E(DSF, "Exceeding estimate max frame count %d.", m_nEstimatedMaxFrameCount);
    }

    /* SPS/PPS/VPS only saves data together with IDR and share one header */
    if (!tFrameHeader.IsParamSet()) {
        /* Save header */
        AX_U32 nHeaderSize = tFrameHeader.uHeaderSize;
        memcpy(m_tBufCache.pBuf + m_tBufCache.nSize, &tFrameHeader, nHeaderSize);
        m_tBufCache.nSize += nHeaderSize;
        m_nTotalCount += nHeaderSize;

        /* Save SPS/PPS/VPS frames */
        if (m_tSPSBufCache.nSize > 0) {
            memcpy(m_tBufCache.pBuf + m_tBufCache.nSize, m_tSPSBufCache.pBuf, m_tSPSBufCache.nSize);
            m_tBufCache.nSize += m_tSPSBufCache.nSize;
            m_nTotalCount += m_tSPSBufCache.nSize;

            m_tSPSBufCache.nSize = 0;
        }

        /* Save IDR/P frames */
        memcpy(m_tBufCache.pBuf + m_tBufCache.nSize, pData, nSize);
        m_tBufCache.nSize += nSize;
        m_nTotalCount += nSize;

        m_tFrameOffsetInfo.nCount += 1;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::WriteFileHeader() {
    if (!IsOpened()) {
        return AX_FALSE;
    }

    AX_U32 nHeaderSize = sizeof(AXDS_FILE_HEADER_T);
    m_tFileHeader.tStartTime.Fill();
    m_tFileHeader.uFileSize = 0; /* Header size will be calculted in UpdateFileHeader with m_nTotalCount */
    m_tFileHeader.uEncodeType = m_tInitAttr.uEncodeType;
    m_tFileHeader.uFrameRate = m_tInitAttr.uFrameRate;
    m_tFileHeader.uGop = m_tInitAttr.uGop > 0 ? m_tInitAttr.uGop : m_nGOPCalulated; /* Unable to retrieve gop from streamer, use gop self calculted */
    m_tFileHeader.uWidth = m_tInitAttr.uWidth;
    m_tFileHeader.uHeight = m_tInitAttr.uHeight;
    memcpy(m_tBufCache.pBuf, &m_tFileHeader, nHeaderSize);
    m_tBufCache.nSize += nHeaderSize;
    m_nTotalCount += nHeaderSize;

    Flush();

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::WriteFileTail() {
    if (!IsOpened()) {
        return AX_FALSE;
    }

    AX_S32 nTailHeader = sizeof(AXDS_FILE_TAIL_T) - sizeof(AX_U32*); /* Do not write ptr size of pBufFrameOffset */
    m_tFileTail.uFrameCount = m_tFrameOffsetInfo.nCount;
    m_tFileTail.pBufFrameOffset = m_tFrameOffsetInfo.pOffsetStart;
    AX_S32 nWritten = write(m_hFD, &m_tFileTail, nTailHeader);
    if (nWritten != nTailHeader) {
        return AX_FALSE;
    }

    AX_S32 nOffsetBytes = m_tFrameOffsetInfo.nCount * sizeof(AX_U32);
    nWritten = write(m_hFD, (AX_VOID*)m_tFrameOffsetInfo.pOffsetStart, nOffsetBytes);
    if (nWritten != nOffsetBytes) {
        LOG_MM_E(DSF, "Write frame offset failed, written=%d, target=%d", nWritten, nOffsetBytes);
        return AX_FALSE;
    }

    m_tFileHeader.uFileSize += nTailHeader + nOffsetBytes;

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::UpdateFileHeader() {
    if (!IsOpened()) {
        return AX_FALSE;
    }

    m_tFileHeader.uFileSize += m_nTotalCount;
    m_tFileHeader.uTailOffset = m_nTotalCount;
    m_tFileHeader.uFrameCount = m_tFrameOffsetInfo.nCount;
    m_tFileHeader.uGop = (0 == m_nGOPCalulated ? 25 : m_nGOPCalulated);
    m_tFileHeader.tEndTime.Fill();

    LOG_MM_D(DSF, "File size: %lld, Frame count: %d", m_tFileHeader.uFileSize, m_tFileHeader.uFrameCount);

    AX_S32 nPos = lseek(m_hFD, 0, SEEK_SET);
    if (-1 == nPos) {
        LOG_MM_E(DSF, "Seek to update header for fd %d failed, err=%s", m_hFD, strerror(errno));
        return AX_FALSE;
    }

    AX_S32 nWritten = write(m_hFD, &m_tFileHeader, sizeof(AXDS_FILE_HEADER_T));
    if (nWritten != sizeof(AXDS_FILE_HEADER_T)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::StartWriteThread() {
    if (!m_threadWrite.Start([this](AX_VOID* pArg) -> AX_VOID { WriteThread(pArg); }, nullptr, "DSF_WRITE")) {
        LOG_MM_E(DSF, "Create data stream write thread fail.");
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::StopWriteThread() {
    if (m_threadWrite.IsRunning()) {
        m_threadWrite.Stop();
        m_hWriteStartEvent.SetEvent();
        m_threadWrite.Join();
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::IsOpened() {
    return AX_DS_INVALID_HANDLE == m_hFD ? AX_FALSE : AX_TRUE;
}

AX_BOOL CDataStreamFile::FillFrameHeader(AX_U32 nSize, AXDS_FRAME_HEADER_T& tHeader) {
    tHeader.uMagic = AX_DSF_FRAME_MAGIC;
    tHeader.uHeaderSize = sizeof(AXDS_FRAME_HEADER_T);
    tHeader.uFrameSize = tHeader.uHeaderSize + m_tSPSBufCache.nSize + nSize;

    if (0 == tHeader.tTimeStamp.uSec && 0 == tHeader.tTimeStamp.uUsec) {
        tHeader.tTimeStamp.Fill();
    }

    return AX_TRUE;
}

const AXDS_FILE_HEADER_T& CDataStreamFile::GetFileHeader() {
    return m_tFileHeader;
}

const AX_CHAR* CDataStreamFile::GetFilePath() {
    return m_szFilePath;
}

const AXDS_FRAME_OFFSET_T& CDataStreamFile::GetIFrameOffsetInfo() {
    std::lock_guard<std::mutex> lck(m_mtxWrite);
    return m_tIFrameOffsetInfo;
};

AX_VOID CDataStreamFile::Flush() {
    if (!IsOpened()) {
        return;
    }

    if (m_tBufCache.nSize > 0) {
        SwapBuf();

        /* Enable starting of writing file */
        m_hWriteStartEvent.SetEvent();
        /* Wait for write file finished */
        m_hWriteCompleteEvent.WaitEvent();
    }
}

AX_VOID CDataStreamFile::SwapBuf() {
    std::lock_guard<std::mutex> lck(m_mtxWriting);

    /* Wait for write file finished */
    if (m_tBufWrite.nSize > 0) {
        m_hWriteCompleteEvent.WaitEvent();
    }

    AX_U8* pTmp = m_tBufCache.pBuf;
    m_tBufCache.pBuf = m_tBufWrite.pBuf;
    m_tBufWrite.pBuf = pTmp;

    m_tBufWrite.nSize = m_tBufCache.nSize;
    m_tBufCache.nSize = 0;

    /* Forbidden the next swap request */
    m_hWriteCompleteEvent.ResetEvent();
}

AX_BOOL CDataStreamFile::AllocFileTailBuf() {
    m_nEstimatedMaxFrameCount = m_tInitAttr.uPeriod * 60 * m_tInitAttr.uFrameRate * 2;
    m_tFrameOffsetInfo.pOffsetStart = (AX_U32*)malloc(m_nEstimatedMaxFrameCount * sizeof(AX_U32));
    if (!m_tFrameOffsetInfo.pOffsetStart) {
        return AX_FALSE;
    } else {
        LOG_MM_D(DSF, "File tail ptr: %p, size: %d", m_tFrameOffsetInfo.pOffsetStart, (AX_U32)(m_nEstimatedMaxFrameCount * sizeof(AX_U32)));
    }

    m_tIFrameOffsetInfo.pOffsetStart = (AX_U32*)malloc(m_nEstimatedMaxFrameCount * sizeof(AX_U32));
    if (!m_tIFrameOffsetInfo.pOffsetStart) {
        return AX_FALSE;
    } else {
        LOG_MM_D(DSF, "Index file ptr: %p, size: %d", m_tIFrameOffsetInfo.pOffsetStart, (AX_U32)(m_nEstimatedMaxFrameCount * sizeof(AX_U32)));
    }

    return AX_TRUE;
}

AX_BOOL CDataStreamFile::FreeFileTailBuf() {
    if (m_tFrameOffsetInfo.pOffsetStart) {
        free(m_tFrameOffsetInfo.pOffsetStart);
        m_tFrameOffsetInfo.pOffsetStart = nullptr;
    }

    if (m_tIFrameOffsetInfo.pOffsetStart) {
        free(m_tIFrameOffsetInfo.pOffsetStart);
        m_tIFrameOffsetInfo.pOffsetStart = nullptr;
    }

    return AX_TRUE;
}

AXDS_FRAME_HEADER_T* CDataStreamFile::FindFrame(AX_S32 nFrmIndex) {
    if (!IsOpened()) {
        return nullptr;
    }

    if (-1 == nFrmIndex || nFrmIndex >= (AX_S32)m_tFileHeader.uFrameCount) {
        return CDSFIterator::END_VALUE;
    }

    AX_S32 nFrmOffsetPos = m_tFileHeader.uTailOffset + sizeof(AXDS_FILE_TAIL_T) - sizeof(AX_U32*) + nFrmIndex * sizeof(AX_U32);
    AX_S32 nPos = lseek(m_hFD, nFrmOffsetPos, SEEK_SET);
    if (-1 == nPos) {
        LOG_MM_E(DSF, "lseek to frame offset saving position failed, ret=%d, err=%s.", nPos, strerror(errno));
        return nullptr;
    } else {
        LOG_MM_D(DSF, "lseek to pos 0x%08X and get frame %d's offset", nPos, nFrmIndex);
    }

    AX_U32 nFrmOffset = 0;
    AX_S32 nRead = read(m_hFD, &nFrmOffset, sizeof(AX_S32));
    if (nRead != sizeof(AX_S32)) {
        LOG_MM_E(DSF, "Read frame offset failed, ret=%d, err=%s.", nRead, strerror(errno));
        return nullptr;
    } else {
        LOG_MM_D(DSF, "Frame %d's offset is 0x%08X", nFrmIndex, nFrmOffset);
    }

    if (nFrmOffset >= m_tFileHeader.uDataOffset && nFrmOffset < m_tFileHeader.uTailOffset) {
        nPos = lseek(m_hFD, nFrmOffset, SEEK_SET);
        if (-1 == nPos) {
            LOG_MM_E(DSF, "lseek to frame offset failed, ret=%d, err=%s.", nPos, strerror(errno));
            return nullptr;
        }

        read(m_hFD, m_arrFrameData, sizeof(AXDS_FRAME_HEADER_T));
        AXDS_FRAME_HEADER_T* pHeader = (AXDS_FRAME_HEADER_T*)&m_arrFrameData[0];
        read(m_hFD, &m_arrFrameData[pHeader->uHeaderSize], pHeader->uFrameSize - pHeader->uHeaderSize);

        return (AXDS_FRAME_HEADER_T*)m_arrFrameData;
    } else {
        LOG_MM_E(DSF, "Frame offset 0x%08X is out of range(0x%08X, 0x%08X)."
                , nFrmOffset, sizeof(AXDS_FILE_HEADER_T)
                , m_tFileHeader.uTailOffset);
    }

    return nullptr;
}

AXDS_FRAME_HEADER_T* CDataStreamFile::FindFrameByOffset(AX_U32 nFrmOffset) {
    if (!IsOpened()) {
        return nullptr;
    }

    if (nFrmOffset >= m_tFileHeader.uTailOffset || nFrmOffset < m_tFileHeader.uDataOffset) {
        LOG_MM_E(DSF, "Frame offset 0x%08X out of range[0x%08X - 0x%08X).", nFrmOffset, m_tFileHeader.uDataOffset, m_tFileHeader.uTailOffset);
        return nullptr;
    }

    AX_S32 nPos = lseek(m_hFD, nFrmOffset, SEEK_SET);
    if (-1 == nPos) {
        LOG_MM_E(DSF, "lseek to frame offset failed, ret=%d, err=%s.", nPos, strerror(errno));
        return nullptr;
    }

    read(m_hFD, m_arrFrameData, sizeof(AXDS_FRAME_HEADER_T));
    AXDS_FRAME_HEADER_T* pHeader = (AXDS_FRAME_HEADER_T*)&m_arrFrameData[0];
    AX_S32 nDataSize = pHeader->uFrameSize - pHeader->uHeaderSize;
    if (nDataSize + sizeof(AXDS_FRAME_HEADER_T) > MAX_FRAME_BUFF_SIZE) {
        LOG_MM_E(DSF, "Frame size(%d) is larger than buffer size(%d).", nDataSize + sizeof(AXDS_FRAME_HEADER_T), MAX_FRAME_BUFF_SIZE);
        return nullptr;
    }

    read(m_hFD, &m_arrFrameData[pHeader->uHeaderSize], pHeader->uFrameSize - pHeader->uHeaderSize);

    return (AXDS_FRAME_HEADER_T*)m_arrFrameData;
}

AX_S32 CDataStreamFile::FindFrmIndexByTime(AX_S32 nSeconds, AX_BOOL bOnlyIFrame /*= AX_FALSE*/) {
    if (!IsOpened()) {
        return -1;
    }

    AX_S32 nFrmCount = m_tFileHeader.uFrameCount;
    for (AX_S32 i = 0; i < nFrmCount; ++i) {
        AXDS_FRAME_HEADER_T* pHeader = FindFrame(i);
        if ((time_t)pHeader->tTimeStamp.uSec >= nSeconds) {
            if (!bOnlyIFrame || NALU_TYPE_IDR == pHeader->GetNaluType()) {
                return i;
            }
        }
    }

    return -1;
}

AX_S32 CDataStreamFile::FindFrmIndexByOffset(AX_U32 nFrmOffset) {
    if (!IsOpened()) {
        return -1;
    }

    if (nFrmOffset >= m_tFileHeader.uTailOffset || nFrmOffset < m_tFileHeader.uDataOffset) {
        LOG_MM_E(DSF, "Frame offset 0x%08X out of range[0x%08X - 0x%08X).", nFrmOffset, m_tFileHeader.uDataOffset, m_tFileHeader.uTailOffset);
        return -1;
    }

    AX_S32 nPos = lseek(m_hFD, m_tFileHeader.uTailOffset + sizeof(AXDS_FILE_TAIL_T) - sizeof(AX_U32*), SEEK_SET);
    if (-1 == nPos) {
        LOG_MM_E(DSF, "lseek to frame tail offset failed, ret=%d, err=%s.", nPos, strerror(errno));
        return -1;
    }

    AX_S32 nFrmIndex = -1;
    AX_U32 nFrmOffsetOfTail = 0;
    for (AX_U32 i = 0; i < m_tFileHeader.uFrameCount; ++i) {
        read(m_hFD, &nFrmOffsetOfTail, 4);
        if (nFrmOffset == nFrmOffsetOfTail) {
            nFrmIndex = i;
            break;
        }
    }

    return nFrmIndex;
}

CDSFIterator CDataStreamFile::frm_begin() {
    return CDSFIterator(this, CDSFIterator::BEGIN);
}

CDSFIterator CDataStreamFile::frm_end() {
    return CDSFIterator(this, CDSFIterator::END);
}

CDSFIterator CDataStreamFile::frm_rbegin() {
    return CDSFIterator(this, CDSFIterator::RBEGIN);
}

CDSFIterator CDataStreamFile::frm_rend() {
    return CDSFIterator(this, CDSFIterator::REND);
}