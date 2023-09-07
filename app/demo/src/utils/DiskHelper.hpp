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
#include <time.h>
#include <deque>
#include <string>
#include "ax_base_type.h"

typedef struct DISK_FILE_INFO_T {
    std::string path;
    AX_U64 size;
    time_t t; /* last modify time */

    DISK_FILE_INFO_T(const std::string& p, AX_U64 s, const time_t& _t) : path(p), size(s), t(_t) {
    }
} DISK_FILE_INFO_T;

///
class CDiskHelper {
public:
    CDiskHelper(AX_VOID) = default;

    /* get free space */
    static AX_U64 GetFreeSpaceSize(const char* path);

    /* mkdir -p */
    static AX_BOOL CreateDir(const char* dir, AX_BOOL bAlwaysCreate);

    /* get the size of directory */
    static AX_U64 GetDirSize(const char* dir);

    /* travser directory by time in ascending order */
    static std::deque<DISK_FILE_INFO_T> TraverseFiles(const char* dir, const char *extension = nullptr);
    static std::deque<DISK_FILE_INFO_T> TraverseDirs(const char* dir);

    /* rm or rm -r */
    static AX_BOOL RemoveFile(const char* path);
    static AX_BOOL RemoveDir(const char* dir);
};