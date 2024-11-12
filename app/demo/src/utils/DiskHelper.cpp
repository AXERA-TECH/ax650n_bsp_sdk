/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "DiskHelper.hpp"
#include <sys/vfs.h>
#include <algorithm>
#include "fs.hpp"

AX_U64 CDiskHelper::GetFreeSpaceSize(const char *path) {
    if (!path) {
        return 0;
    }

    struct statfs stat;
    if (statfs(path, &stat) < 0) {
        return 0;
    }

    return stat.f_bsize * stat.f_bfree;
}

AX_BOOL CDiskHelper::CreateDir(const char *dir, AX_BOOL bAlwaysCreate) {
    if (!dir) {
        return AX_FALSE;
    }

    try {
        fs::path p(dir);
        if (fs::exists(p)) {
            if (bAlwaysCreate) {
                fs::remove_all(dir);
            }
        }

        fs::create_directories(dir);

    } catch (fs::filesystem_error &e) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_U64 CDiskHelper::GetDirSize(const char *path) {
    if (!path) {
        return 0;
    }

    fs::path p(path);

    if (!fs::exists(p)) {
        return 0;
    }

    AX_U64 nSize = {0};
    if (fs::is_directory(p)) {
        for (const auto &entry : fs::recursive_directory_iterator(p)) {
            if (entry.is_regular_file()) {
                nSize += entry.file_size();
            }
        }
    }

    return nSize;
}

std::deque<DISK_FILE_INFO_T> CDiskHelper::TraverseFiles(const char *dir, const char *extension /*= nullptr*/) {
    std::deque<DISK_FILE_INFO_T> v;

    if (!dir) {
        return v;
    }

    fs::path p(dir);

    if (!fs::exists(p)) {
        return v;
    }

    for (auto &&entry : fs::recursive_directory_iterator(p)) {
        if (entry.is_regular_file()) {
            if (nullptr != extension && strcasecmp(extension, entry.path().extension().c_str())) {
                continue;
            }

            v.emplace_back(std::move(entry.path().string()), entry.file_size(),
                           fs::file_time_type::clock::to_time_t(entry.last_write_time()));
        }
    }

    if (v.size() > 1) {
        /* sort by last by modified time in ascending order */
        std::sort(v.begin(), v.end(), [&](const DISK_FILE_INFO_T &a, DISK_FILE_INFO_T &b) -> bool { return a.t < b.t; });
    }

    return v;
}

std::deque<DISK_FILE_INFO_T> CDiskHelper::TraverseDirs(const char *dir) {
    std::deque<DISK_FILE_INFO_T> v;

    if (!dir) {
        return v;
    }

    fs::path p(dir);

    if (!fs::exists(p)) {
        return v;
    }

    for (auto &&entry : fs::directory_iterator(p)) {
        if (entry.is_directory()) {
            v.emplace_back(std::move(entry.path().string()), entry.file_size(),
                           fs::file_time_type::clock::to_time_t(entry.last_write_time()));
        }
    }

    if (v.size() > 1) {
        /* sort by last by modified time in ascending order */
        std::sort(v.begin(), v.end(), [&](const DISK_FILE_INFO_T &a, DISK_FILE_INFO_T &b) -> bool { return a.t < b.t; });
    }

    return v;
}

AX_BOOL CDiskHelper::RemoveFile(const char* path) {
    std::error_code e;
    return fs::remove(path, e) ? AX_TRUE : AX_FALSE;
}

AX_BOOL CDiskHelper::RemoveDir(const char* dir) {
    std::error_code e;
    auto r = fs::remove_all(dir, e);
    return (static_cast<std::uintmax_t>(-1) == r || 0 == r) ? AX_FALSE : AX_TRUE;
}