/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "datastreamIndFile.hpp"
#include "AppLog.hpp"
#include "ElapsedTimer.hpp"
#include "fs.hpp"
#include <fcntl.h>
#include <unistd.h>

#define DSIF "DS_IND_FILE"
#define MAX_IFRAME_OFFSET_BUFF_SIZE (1024 * 1024)

using namespace std;

AX_BOOL CDataStreamIndFile::Init(const AX_CHAR* pFilePath, AXIF_OPEN_FLAG_E eOpenFlag, AX_S32 nDate /*= 0*/, AX_S32 nTime /*= 0*/) {
    LOG_MM_I(DSIF, "+++");

    if (nullptr == pFilePath) {
        return AX_FALSE;
    }

    strcpy(m_szFilePath, pFilePath);

    if (!Load(eOpenFlag)) {
        return AX_FALSE;
    }

    if (AXIF_OPEN_FOR_READ == eOpenFlag) {
        /* Start read frame with specified time */
        m_nStartDate = nDate;
        m_nStartTime = nTime;

        m_pIFrmOffsetBuf = (AX_U8*)malloc(MAX_IFRAME_OFFSET_BUFF_SIZE);
    }

    LOG_MM_I(DSIF, "---");
    return AX_TRUE;
}

AX_BOOL CDataStreamIndFile::DeInit() {
    LOG_MM_I(DSIF, "+++");

    CloseFile();

    if (m_pIFrmOffsetBuf) {
        free(m_pIFrmOffsetBuf);
        m_pIFrmOffsetBuf = nullptr;
    }

    LOG_MM_I(DSIF, "---");
    return AX_TRUE;
}

AX_BOOL CDataStreamIndFile::Load(AXIF_OPEN_FLAG_E eOpenFlag) {
    LOG_MM_I(DSIF, "+++");

    fs::path p(m_szFilePath);
    if (fs::exists(p)) {
        LOG_MM_I(DSIF, "Load existed index file: %s", m_szFilePath);
        if (!OpenFile(m_szFilePath, AX_TRUE, eOpenFlag)) {
            return AX_FALSE;
        }

        AX_S32 nRead = read(m_hFD, &m_tFileHeader, sizeof(AXIF_FILE_HEADER_T));
        if (nRead != sizeof(AXIF_FILE_HEADER_T)) {
            LOG_MM_E(DSIF, "Read file header failed, ret=%d, err=%s.", nRead, strerror(errno));
            return AX_FALSE;
        } else {
            LOG_MM_I(DSIF, "Index file header received, %d data file exist(s).", m_tFileHeader.uFileCount);
        }
    } else {
        if (AXIF_OPEN_FOR_READ == eOpenFlag) {
            LOG_MM_E(DSIF, "Can not read file %s, file not exist.", m_szFilePath);
            return AX_FALSE;
        }

        m_tFileHeader.uMagic = AX_DSIF_FILE_MAGIC;
        m_tFileHeader.uVersion = AX_DSIF_VERSION;
        m_tFileHeader.uFileCount = 0;
        m_tFileHeader.uTotalFrameCount = 0;
        m_tFileHeader.uTotalIFrameCount = 0;
        m_tFileHeader.uTotalSize = 0;
        m_tFileHeader.uTotalTime = 0;
    }

    LOG_MM_I(DSIF, "---");
    return AX_TRUE;
}

AX_BOOL CDataStreamIndFile::OpenFile(const AX_CHAR* pFilePath, AX_BOOL bFileExists, AXIF_OPEN_FLAG_E eOpenFlag) {
    LOG_MM_I(DSIF, "+++");

    if (nullptr == pFilePath) {
        LOG_M_E(DSIF, "Index file path is NULL.");
        return AX_FALSE;
    }

    if (AX_DS_INVALID_HANDLE != m_hFD) {
        LOG_M_E(DSIF, "File handle already exist.");
        return AX_FALSE;
    }

    if (bFileExists) {
        if (AXIF_OPEN_FOR_READ == eOpenFlag) {
            m_hFD = open(pFilePath, O_RDONLY, S_IRUSR | S_IWUSR);
        } else if (AXIF_OPEN_FOR_WRITE == eOpenFlag) {
            m_hFD = open(pFilePath, O_RDWR, S_IRUSR | S_IWUSR);
        } else if (AXIF_OPEN_FOR_BOTH == eOpenFlag) {
            m_hFD = open(pFilePath, O_RDWR, S_IRUSR | S_IWUSR);
        }
    } else {
        m_hFD = open(pFilePath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    }

    if (AX_DS_INVALID_HANDLE == m_hFD) {
        LOG_M_E(DSIF, "Open data stream index file %s failed, ret=%s.", pFilePath, strerror(errno));
        return AX_FALSE;
    }

    // if (fallocate(m_hFD, FALLOC_FL_KEEP_SIZE, 0, 10 * 1024 * 1024) < 0) {
    //     LOG_M_E(DSIF, "fallocate %s failed, err: %s", m_szFilePath, strerror(errno));
    //     return AX_FALSE;
    // }

    LOG_MM_I(DSIF, "---");
    return AX_TRUE;
}

AX_VOID CDataStreamIndFile::CloseFile() {
    LOG_MM_I(DSIF, "+++");

    if (AX_DS_INVALID_HANDLE == m_hFD) {
        return;
    }

    close(m_hFD);
    m_hFD = AX_DS_INVALID_HANDLE;

    LOG_MM_I(DSIF, "---");
}

AX_BOOL CDataStreamIndFile::WriteFileHeader(AXIF_FILE_HEADER_T& tIFFileHeader) {
    if (AX_DS_INVALID_HANDLE == m_hFD) {
        return AX_FALSE;
    }

    AX_S32 nWritten = write(m_hFD, &tIFFileHeader, sizeof(AXIF_FILE_HEADER_T));
    if (nWritten != sizeof(AXIF_FILE_HEADER_T)) {
        LOG_M_E(DSIF, "Write index file header to %s failed, %d<>%d, err_cd=%s.", m_szFilePath, nWritten, sizeof(AXIF_FILE_HEADER_T), strerror(errno));
        return AX_FALSE;
    } else {
        fsync(m_hFD);
    }

    LOG_MM_C(DSIF, "Write index file header info:");
    LOG_MM_C(DSIF, "Total Size: %d, Data File Count: %d, Total Time: %lld, Total Frame: %d, Total I Frame: %d"
                    , tIFFileHeader.uTotalSize
                    , tIFFileHeader.uFileCount
                    , tIFFileHeader.uTotalTime
                    , tIFFileHeader.uTotalFrameCount
                    , tIFFileHeader.uTotalIFrameCount);

    return AX_TRUE;
}

AX_BOOL CDataStreamIndFile::WriteFileInfo(AXIF_FILE_INFO_T& tIFFileInfo) {
    if (AX_DS_INVALID_HANDLE == m_hFD) {
        return AX_FALSE;
    }

    AX_S32 nFileInfoHeaderSize = sizeof(AXIF_FILE_INFO_T) - sizeof(AX_U32*); /* Remove ptr size of pIFrameOffsetStart */
    AX_S32 nWritten = write(m_hFD, &tIFFileInfo, nFileInfoHeaderSize);

    if (nWritten != nFileInfoHeaderSize) {
        LOG_M_E(DSIF, "Write index file info to %s failed.", m_szFilePath);
        return AX_FALSE;
    } else {
        LOGBUF(APP_LOG_DEBUG, &tIFFileInfo, nFileInfoHeaderSize, APP_LOG_SYNC_SEND);
    }

    AX_S32 nIFrameOffsetBufLen = tIFFileInfo.uIFrameCount * sizeof(AX_U32);
    nWritten = write(m_hFD, tIFFileInfo.pIFrameOffsetStart, nIFrameOffsetBufLen);
    if (nWritten != nIFrameOffsetBufLen) {
        LOG_M_E(DSIF, "Write index file's I frame offset info to %s failed, written %d <> buff %d.", m_szFilePath, nWritten, nIFrameOffsetBufLen);
        return AX_FALSE;
    } else {
        LOGBUF(APP_LOG_DEBUG, tIFFileInfo.pIFrameOffsetStart, nIFrameOffsetBufLen, APP_LOG_SYNC_SEND);
    }

    fsync(m_hFD);

    LOG_M_I(DSIF, "Write index file info:");
    LOG_M_I(DSIF, "File: %s, Total Size: %d, Frame Count: %d, IFrame Count: %d, Video Total Time: %lld"
                    , tIFFileInfo.szFilePath
                    , tIFFileInfo.uFileSize
                    , tIFFileInfo.uFrameCount
                    , tIFFileInfo.uIFrameCount
                    , tIFFileInfo.tEndTime.TickFrom(tIFFileInfo.tStartTime));

    return AX_TRUE;
}

AX_BOOL CDataStreamIndFile::CreateAndSave(AXIF_FILE_INFO_T& tIFFileInfo) {
    m_tFileHeader.uFileCount++;
    m_tFileHeader.uTotalSize += tIFFileInfo.uSize;
    m_tFileHeader.uTotalFrameCount += tIFFileInfo.uFrameCount;
    m_tFileHeader.uTotalIFrameCount += tIFFileInfo.uIFrameCount;
    m_tFileHeader.uTotalTime += tIFFileInfo.tEndTime.TickFrom(tIFFileInfo.tStartTime) * 1000;

    if (AX_DS_INVALID_HANDLE == m_hFD) {
        /* Create new index file */
        if (!OpenFile(m_szFilePath, AX_FALSE, AXIF_OPEN_FOR_WRITE)) {
            LOG_M_E(DSIF, "Create index file %s failed.", m_szFilePath);
            return AX_FALSE;
        } else {
            LOG_M_I(DSIF, "Create index file %s successfully.", m_szFilePath);
        }

        m_tFileHeader.uTotalSize += sizeof(AXIF_FILE_HEADER_T);
        if (!WriteFileHeader(m_tFileHeader)) {
            return AX_FALSE;
        } else {
            LOGBUF(APP_LOG_DEBUG, &m_tFileHeader, sizeof(AXIF_FILE_HEADER_T), APP_LOG_SYNC_SEND);
        }

        if (!WriteFileInfo(tIFFileInfo)) {
            return AX_FALSE;
        }
    } else {
        /* Update header to old index file */
        AX_S32 nPos = lseek(m_hFD, 0, SEEK_SET);
        if (nPos != 0) {
            LOG_MM_E(DSIF, "lseek to start position failed, pos=%d, err=%s.", nPos, strerror(errno));
            return AX_FALSE;
        }

        LOG_MM_D(DSIF, "Seek to pos %d successfully.", nPos);
        if (!WriteFileHeader(m_tFileHeader)) {
            return AX_FALSE;
        } else {
            LOGBUF(APP_LOG_DEBUG, &m_tFileHeader, sizeof(AXIF_FILE_HEADER_T), APP_LOG_SYNC_SEND);
            LOG_M_I(DSIF, "Load existing index file header info:");
            LOG_M_I(DSIF, "Total Size: %d, Data File Count: %d, Total Time: %lld, Total Frame: %d, Total I Frame: %d"
                    , m_tFileHeader.uTotalSize
                    , m_tFileHeader.uFileCount
                    , m_tFileHeader.uTotalTime
                    , m_tFileHeader.uTotalFrameCount
                    , m_tFileHeader.uTotalIFrameCount);
        }

        /* Add file info to old index file */
        nPos = lseek(m_hFD, 0, SEEK_END);
        if (-1 == nPos) {
            LOG_MM_E(DSIF, "lseek to end position failed, pos=%d, err=%s.", nPos, strerror(errno));
            return AX_FALSE;
        }

        if (!WriteFileInfo(tIFFileInfo)) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

string CDataStreamIndFile::FormatFileName(AX_U8 nStreamID) {
    AX_CHAR szFileName[16] = {0};
    sprintf(szFileName, "%s%s.ind", 0 == nStreamID ? "main" : "sub", (0 == nStreamID ? "" : to_string(nStreamID).c_str()));

    return szFileName;
}

AXIF_FILE_INFO_EX_T CDataStreamIndFile::FindInfo(AX_S32 nInfoIndex, AX_BOOL bFillOffset /*= AX_FALSE*/) {
    if (!IsOpened()) {
        LOG_MM_E(DSIF, "Index file not opened.");
        return CDSIFIterator::END_VALUE;
    }

    if (-1 == nInfoIndex || nInfoIndex == (AX_S32)m_tFileHeader.uFileCount) {
        return CDSIFIterator::END_VALUE;
    } else {
        AXIF_FILE_INFO_EX_T tFileInfo;
        AX_U32 nFilePos = sizeof(AXIF_FILE_HEADER_T);
        AX_S32 nPureFileInfoSize = sizeof(AXIF_FILE_INFO_T) - sizeof(AX_U32*);
        for (AX_S32 i = 0; i <= nInfoIndex; ++i) {
            AX_S32 nPos = lseek(m_hFD, nFilePos, SEEK_SET);
            if (-1 == nPos) {
                LOG_MM_E(DSIF, "lseek to file info pos %d failed, ret=%d, err=%s.", nFilePos, nPos, strerror(errno));
                return CDSIFIterator::END_VALUE;
            }

            AX_S32 nRead = read(m_hFD, &tFileInfo.tInfo, nPureFileInfoSize);
            if (nRead != nPureFileInfoSize) {
                LOG_MM_E(DSIF, "Read file info failed.");
                return CDSIFIterator::END_VALUE;
            }

            nFilePos += tFileInfo.tInfo.uSize;
        }

        if (bFillOffset) {
            AX_S32 nOffsetSize = tFileInfo.tInfo.uIFrameCount * sizeof(AX_U32);
            if (nOffsetSize > MAX_IFRAME_OFFSET_BUFF_SIZE) {
                LOG_MM_E(DSIF, "I frame offset size exceeding max size 1M");
                return CDSIFIterator::END_VALUE;
            }

            AX_S32 nRead = read(m_hFD, m_pIFrmOffsetBuf, nOffsetSize);
            tFileInfo.pIFrmOffsetBuf = m_pIFrmOffsetBuf;

            if (nRead != nOffsetSize) {
                LOG_MM_E(DSIF, "Read I frame offset buff failed.");
                return CDSIFIterator::END_VALUE;
            }
        }

        return tFileInfo;
    }

    return CDSIFIterator::END_VALUE;
}

AX_BOOL CDataStreamIndFile::FindFrameLocationByIndex(AX_S32 nFrmIndex, AXIF_FRAME_LOCATION_T& tLocation) {
    AX_S32 nFrmStartInd = 0;
    AX_S32 nFrmEndInd = -1;
    AX_S32 nFileIndex = 0;
    CDSIFIterator itStart = info_begin();
    CDSIFIterator itEnd = info_end();
    AXIF_FILE_INFO_EX_T info;
    for (; itStart != itEnd; ++itStart) {
        info = *itStart;
        nFrmStartInd = nFrmEndInd + 1;
        nFrmEndInd = nFrmStartInd + info.tInfo.uFrameCount - 1;
        if (nFrmIndex >= nFrmStartInd && nFrmIndex <= nFrmEndInd) {
            tLocation.strDataFile = info.tInfo.szFilePath;
            tLocation.nFileIndex = nFileIndex;
            tLocation.nGlobalFrameIndex = nFrmIndex;
            tLocation.nFrameIndexWithinFile = nFrmIndex - nFrmStartInd;
            tLocation.nFileStartFrmIndex = nFrmStartInd;
            tLocation.nFileEndFrmIndex = nFrmEndInd;

            return AX_TRUE;
        } else {
            nFileIndex++;
        }
    }

    return AX_FALSE;
}

AX_BOOL CDataStreamIndFile::FindFrameLocationByTime(time_t nTargetSeconds, AXIF_FRAME_LOCATION_T& tLocation, AX_BOOL bIFrameOnly /*= AX_FALSE*/, AX_BOOL bReverse /*= AX_FALSE*/) {
    AX_BOOL bFindResult = AX_FALSE;
    AX_S32 nFileStartSec = 0;
    AX_S32 nFileEndSec = 0;
    AX_U32 nTotalFrmIndex = 0;
    AX_S32 nFileIndex = bReverse ? m_tFileHeader.uFileCount - 1 : 0;
    CDSIFIterator itStart = bReverse ? info_rbegin() : info_begin();
    CDSIFIterator itEnd = bReverse ? info_rend() : info_end();
    AXIF_FILE_INFO_EX_T info;

    for (; itStart != itEnd; ++itStart) {
        info = *itStart;
        nFileStartSec = info.tInfo.tStartTime.uSec;
        nFileEndSec = info.tInfo.tEndTime.uSec;

        std::unique_ptr<CDataStreamFile> dsf = make_unique<CDataStreamFile>();
        if (!dsf || !dsf->Open(info.tInfo.szFilePath, AX_DSF_OPEN_FOR_READ)) {
            return AX_FALSE;
        }

        do {
            if (nTargetSeconds >= nFileStartSec && nTargetSeconds <= nFileEndSec) {
                AXDS_FRAME_HEADER_T* pHeader = nullptr;
                AX_S32 nFrmIndexWithinFile = -1;

                if (bReverse) {
                    for (AX_S32 i = info.tInfo.uIFrameCount - 1; i >= 0; --i) {
                        pHeader = dsf->FindFrameByOffset(*((AX_U32 *)info.pIFrmOffsetBuf + i));
                        if (pHeader->tTimeStamp.uSec <= nTargetSeconds) {
                            nFrmIndexWithinFile = i;
                            break;
                        }
                    }
                } else {
                    nFrmIndexWithinFile = dsf->FindFrmIndexByTime(nTargetSeconds, bIFrameOnly);
                }

                if (-1 != nFrmIndexWithinFile) {
                    tLocation.strDataFile = info.tInfo.szFilePath;
                    tLocation.nFileIndex = nFileIndex;
                    tLocation.nGlobalFrameIndex = nTotalFrmIndex + (!bReverse ? nFrmIndexWithinFile : info.tInfo.uIFrameCount - 1 - nFrmIndexWithinFile);
                    tLocation.nFrameIndexWithinFile = nFrmIndexWithinFile;
                    tLocation.nFileStartFrmIndex = nTotalFrmIndex;
                    tLocation.nFileEndFrmIndex = nTotalFrmIndex + (!bReverse ? info.tInfo.uFrameCount - 1 : info.tInfo.uIFrameCount - 1);

                    bFindResult = AX_TRUE;
                    break;
                } else {
                    nTotalFrmIndex += (!bReverse ? info.tInfo.uFrameCount : info.tInfo.uIFrameCount);
                    nFileIndex = bReverse ? nFileIndex - 1 : nFileIndex + 1;

                    break;
                }
            } else if ((!bReverse && nTargetSeconds < nFileStartSec) || (bReverse && nTargetSeconds > nFileStartSec)) {
                AXDS_FRAME_HEADER_T* pHeader = nullptr;
                AX_S32 nFrmIndexWithinFile = -1;
                if (bReverse) {
                    for (AX_S32 i = info.tInfo.uIFrameCount - 1; i >= 0; --i) {
                        pHeader = dsf->FindFrameByOffset(*((AX_U32 *)info.pIFrmOffsetBuf + i));
                        if (pHeader->tTimeStamp.uSec <= nTargetSeconds) {
                            nFrmIndexWithinFile = i;
                            break;
                        }
                    }
                } else {
                    nFrmIndexWithinFile = dsf->FindFrmIndexByTime(nTargetSeconds, bIFrameOnly);
                }

                if (-1 != nFrmIndexWithinFile) {
                    tLocation.strDataFile = info.tInfo.szFilePath;
                    tLocation.nFileIndex = nFileIndex;
                    tLocation.nGlobalFrameIndex = nTotalFrmIndex + (!bReverse ? nFrmIndexWithinFile : info.tInfo.uIFrameCount - 1 - nFrmIndexWithinFile);
                    tLocation.nFrameIndexWithinFile = nFrmIndexWithinFile;
                    tLocation.nFileStartFrmIndex = nTotalFrmIndex;
                    tLocation.nFileEndFrmIndex = nTotalFrmIndex + (!bReverse ? info.tInfo.uFrameCount - 1 : info.tInfo.uIFrameCount - 1);

                    bFindResult = AX_TRUE;
                } else {
                    nTotalFrmIndex += (!bReverse ? info.tInfo.uFrameCount : info.tInfo.uIFrameCount);
                    nFileIndex = bReverse ? nFileIndex - 1 : nFileIndex + 1;
                }

                break;
            } else {
                nTotalFrmIndex += (!bReverse ? info.tInfo.uFrameCount : info.tInfo.uIFrameCount);
                nFileIndex = bReverse ? nFileIndex - 1 : nFileIndex + 1;
                break;
            }
        } while (1);

        dsf->Close();

        if (bFindResult) {
            break;
        }
    }

    return bFindResult;
}

CDSIFIterator CDataStreamIndFile::info_begin() {
    return CDSIFIterator(this, CDSIFIterator::BEGIN);
}

CDSIFIterator CDataStreamIndFile::info_end() {
    return CDSIFIterator(this, CDSIFIterator::END);
}

CDSIFIterator CDataStreamIndFile::info_rbegin() {
    return CDSIFIterator(this, CDSIFIterator::RBEGIN);
}

CDSIFIterator CDataStreamIndFile::info_rend() {
    return CDSIFIterator(this, CDSIFIterator::REND);
}

CDSIterator CDataStreamIndFile::begin() {
    if (m_nStartTime > 0) {
        time_t timeVal = CElapsedTimer::GetTimeTVal(m_nStartDate, m_nStartTime);
        AXIF_FRAME_LOCATION_T tLocation;
        if (FindFrameLocationByTime(timeVal, tLocation, AX_TRUE)) {
            return CDSIterator(this, CDSIterator::BEGIN).Relocate(tLocation);
        } else {
            LOG_MM_W(DSIF, "Can not locate at time(%d %d).", m_nStartDate, m_nStartTime);
            return CDSIterator(this, CDSIterator::END);
        }
    }

    return CDSIterator(this, CDSIterator::BEGIN);
}

CDSIterator CDataStreamIndFile::end() {
    return CDSIterator(this, CDSIterator::END);
}

CDSIterator CDataStreamIndFile::rbegin() {
    if (m_nStartTime > 0) {
        time_t timeVal = CElapsedTimer::GetTimeTVal(m_nStartDate, m_nStartTime);
        AXIF_FRAME_LOCATION_T tLocation;
        if (FindFrameLocationByTime(timeVal, tLocation, AX_TRUE, AX_TRUE)) {
            return CDSIterator(this, CDSIterator::RBEGIN).Relocate(tLocation);
        } else {
            LOG_MM_W(DSIF, "Can not locate at time(%d %d).", m_nStartDate, m_nStartTime);
            return CDSIterator(this, CDSIterator::REND);
        }
    }

    return CDSIterator(this, CDSIterator::RBEGIN);
}

CDSIterator CDataStreamIndFile::rend() {
    return CDSIterator(this, CDSIterator::REND);
}