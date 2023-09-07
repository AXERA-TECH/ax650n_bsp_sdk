/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "BoxSataFile.hpp"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "AppLogApi.h"

#define TAG "SATA"

AX_BOOL CBoxSataFile::Open(const BOX_SATA_FILE_ATTR_T& stAttr) {
    if (0 == stAttr.nMaxSpace || 0 == stAttr.nFileSize || stAttr.nMaxSpace < stAttr.nFileSize) {
        LOG_M_E(TAG, "invalid vdGrp %02d max. space size %d or reserved file size is %d", stAttr.nCookie, stAttr.nMaxSpace,
                stAttr.nFileSize);
        return AX_FALSE;
    }

    m_stAttr = stAttr;
    m_diskFiles = CDiskHelper::TraverseFiles(stAttr.strDirPath.c_str());
    m_IoPerf.Reset();

    return AllocFile();
}

AX_BOOL CBoxSataFile::Close(AX_VOID) {
    // AX_U64 nTick1 = CIOPerf::GetTickCount();
#ifdef __LINUX_IO_API__
    if (m_fd >= 0) {
        //  fsync(m_fd);
        close(m_fd);
        m_fd = -1;
#else
    if (m_fp) {
        fflush(m_fp);
        //  fsync(fileno(m_fp));
        fclose(m_fp);
        m_fp = nullptr;
#endif
        // AX_U64 nTick2 = CIOPerf::GetTickCount();
        // m_IoPerf.Update(0, nTick2 - nTick1);

        LOG_M_C(TAG, "vdGrp %02d IO: avg = %.2f MB/s, min = %.2f MB/s, max = %.2f MB/s, total = %lld bytes", m_stAttr.nCookie,
                m_IoPerf.GetAvgSpeed(), m_IoPerf.GetMinSpeed(), m_IoPerf.GetMaxSpeed(), m_IoPerf.GetTotalBytes());
    }

    return AX_TRUE;
}

AX_U32 CBoxSataFile::Write(const AX_VOID* pData, AX_U32 nBytesToWrite) {
    AX_U8* pBuff = (AX_U8*)pData;
    AX_U32 nLeft = nBytesToWrite;

    // LOG_M_C(TAG, "vdGrp %02d %s 0x%x +++", m_stAttr.nCookie, __func__, nBytesToWrite);
    AX_U64 nTick1, nTick2;
    while (nLeft > 0) {
        AX_U64 nFree = m_stAttr.nFileSize - m_nCurFileSize;
        if (0 == nFree) {
            Close();

            if (!AllocFile()) {
                return 0;
            }

            m_IoPerf.Reset();
            nFree = m_stAttr.nFileSize;
        }

        nTick1 = CIOPerf::GetTickCount();
        AX_U32 nBytes = (nLeft > nFree) ? nFree : nLeft;
#ifdef __LINUX_IO_API__
        if (nBytes != write(m_fd, pBuff, nBytes)) {
#else
        if (nBytes != fwrite(pBuff, 1, nBytes, m_fp)) {
#endif
            LOG_M_E(TAG, "vdGrp %d write %d bytes fail, %s", m_stAttr.nCookie, nBytes, strerror(errno));
            return 0;
        }

        nTick2 = CIOPerf::GetTickCount();
        m_IoPerf.Update(nBytes, nTick2 - nTick1);

        m_nCurFileSize += nBytes;
        nLeft -= nBytes;
        pBuff += nBytes;
    }

    if (m_stAttr.bSyncIO) {
        nTick1 = CIOPerf::GetTickCount();
#ifdef __LINUX_IO_API__
        fsync(m_fd);
#else
        fflush(m_fp);
        fsync(fileno(m_fp));
#endif
        nTick2 = CIOPerf::GetTickCount();
        m_IoPerf.Update(0, nTick2 - nTick1);
    }

    return nBytesToWrite;
}

AX_U32 CBoxSataFile::Read(AX_VOID* pData, AX_U32 nBytesToRead) {
#ifdef __LINUX_IO_API__
    return read(m_fd, pData, nBytesToRead);
#else
    return fread(pData, 1, nBytesToRead, m_fp);
#endif
}

AX_BOOL CBoxSataFile::AllocFile(AX_VOID) {
    if (!CheckSpaceAndRemoveOldFiles()) {
        return AX_FALSE;
    }

    timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);
    time_t now = time(NULL);
    localtime_r(&now, &t);

    AX_CHAR szName[64];
    sprintf(szName, "vdGrp%02d_%04d_%02d_%02d_%02d_%02d_%02d.dat", m_stAttr.nCookie, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
            t.tm_min, t.tm_sec);
    std::string strPath = m_stAttr.strDirPath + szName;

#ifdef __LINUX_IO_API__
    /*
        If the O_APPEND file status flag is set on the open file
        description, then a write(2) always moves the file offset to the
        end of the file, regardless of the use of lseek().
    */
    m_fd = open(strPath.c_str(), O_CREAT | O_RDWR | O_TRUNC /* | O_APPEND */, S_IRUSR | S_IWUSR);
    if (m_fd < 0) {
#else
    m_fp = fopen(strPath.c_str(), "wb");
    if (!m_fp) {
#endif
        LOG_M_E(TAG, "open < %s > fail, %s", strPath.c_str(), strerror(errno));
        return AX_FALSE;
    }

#ifdef __LINUX_IO_API__
    if (fallocate(m_fd, FALLOC_FL_KEEP_SIZE, 0, m_stAttr.nFileSize) < 0) {
#else
    if (fallocate(fileno(m_fp), FALLOC_FL_KEEP_SIZE, 0, m_stAttr.nFileSize) < 0) {
#endif
        LOG_M_E(TAG, "fallocate < %s > fail, %s", strPath.c_str(), strerror(errno));
        return AX_FALSE;
    }

#ifdef __LINUX_IO_API__
    lseek(m_fd, 0, SEEK_SET);
#else
    fseek(m_fp, 0, SEEK_SET);
#endif

    m_diskFiles.emplace_back(strPath.c_str(), m_stAttr.nFileSize, 0);
    m_nCurFileSize = 0;

    return AX_TRUE;
}

AX_BOOL CBoxSataFile::CheckSpaceAndRemoveOldFiles(AX_VOID) {
#if 0
    AX_U64 nFreeSpace = CDiskHelper::GetFreeSpaceSize(m_stAttr.strDirPath.c_str());
    if (nFreeSpace < m_stAttr.nFileSize) {
        static AX_BOOL bWarning = AX_TRUE;
        if (bWarning) {
            bWarning = AX_FALSE;
            LOG_M_E(TAG, ">>>>>>>>>>>>>>> free space left %lld (%lld MBytes) < %lld <<<<<<<<<<<<<<<", nFreeSpace, nFreeSpace >> 20,
                    m_stAttr.nFileSize);
        }

        return AX_FALSE;
    }
#endif

    AX_U64 nUsedSpace = {0};
    for (auto&& m : m_diskFiles) {
        nUsedSpace += m.size;
    }

    AX_S64 nLeftSpace = (AX_S64)(m_stAttr.nMaxSpace - nUsedSpace);
    if (nLeftSpace >= (AX_S64)m_stAttr.nFileSize) {
        return AX_TRUE;
    }

    AX_BOOL bRefresh = {AX_FALSE};
    AX_U32 nFileCount = m_diskFiles.size();
    for (AX_U32 i = 0; i < nFileCount; ++i) {
        DISK_FILE_INFO_T& m = m_diskFiles.front();

        if (CDiskHelper::RemoveFile(m.path.c_str())) {
            nLeftSpace += m.size;
            LOG_M_I(TAG, "left space %lld(%lld MB), delete %s", nLeftSpace, nLeftSpace >> 20, m.path.c_str());
        } else {
            /* delete file fail, mark to refresh list */
            bRefresh = AX_TRUE;
            LOG_M_W(TAG, "fail to delete < %s >, %s", m.path.c_str(), strerror(errno));
        }

        m_diskFiles.pop_front();

        if (nLeftSpace >= (AX_S64)m_stAttr.nFileSize) {
            break;
        }
    }

    if (bRefresh) {
        LOG_M_W(TAG, "traversing %s ...", m_stAttr.strDirPath.c_str());
        m_diskFiles = CDiskHelper::TraverseFiles(m_stAttr.strDirPath.c_str());
    }

    return (nLeftSpace >= (AX_S64)m_stAttr.nFileSize) ? AX_TRUE : AX_FALSE;
}