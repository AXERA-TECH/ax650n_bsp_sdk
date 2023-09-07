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
#include <stdio.h>
#include <string>
#include "DiskHelper.hpp"
#include "IOPerf.hpp"
#include "ax_global_type.h"

typedef struct {
    AX_S32 nCookie;
    AX_U64 nFileSize;
    AX_U64 nMaxSpace;
    AX_BOOL bSyncIO;
    std::string strDirPath;
} BOX_SATA_FILE_ATTR_T;

class CBoxSataFile {
public:
    CBoxSataFile(AX_VOID) = default;

    AX_BOOL Open(const BOX_SATA_FILE_ATTR_T &stAttr);
    AX_BOOL Close(AX_VOID);

    AX_U32 Write(const AX_VOID *pData, AX_U32 nBytesToWrite);
    AX_U32 Read(AX_VOID *pData, AX_U32 nBytesToRead);

protected:
    AX_BOOL AllocFile(AX_VOID);
    AX_BOOL CheckSpaceAndRemoveOldFiles(AX_VOID);

private:
    BOX_SATA_FILE_ATTR_T m_stAttr;
#ifdef __LINUX_IO_API__
    int m_fd = {-1};
#else
    FILE *m_fp = {nullptr};
#endif
    AX_U32 m_nCurFileSize = {0};
    CIOPerf m_IoPerf;
    std::deque<DISK_FILE_INFO_T> m_diskFiles;
};