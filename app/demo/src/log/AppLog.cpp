/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AppLog.hpp"
#include <string.h>
#include <unistd.h>
static constexpr AX_U32 MAX_LOG_FILE_SIZE = 1 * 1024 * 1024;

AX_BOOL CAppLog::Open(const APP_LOG_ATTR_T &stAttr) {
    AX_CHAR szPath[260];
    for (AX_U32 i = 0; i < MAX_APP_LOG_FILE_COUNT; ++i) {
        if (0 == i) {
            sprintf(szPath, "/var/log/%s.log", stAttr.szAppName);
        } else {
            sprintf(szPath, "/var/log/%s_%d.log", stAttr.szAppName, i);
        }

        m_arrFiles[i] = szPath;
    }

    m_fp = fopen(m_arrFiles[0].c_str(), "w");
    if (!m_fp) {
        printf("open app log file %s fail\n", m_arrFiles[0].c_str());
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CAppLog::Close(AX_VOID) {
    if (m_fp) {
        fclose(m_fp);
        m_fp = nullptr;
    }
}

AX_VOID CAppLog::Log(AX_S32 nLv, const AX_CHAR *pStr) {
    std::lock_guard<std::mutex> lck(m_mtx);

    if (!m_fp) {
        return;
    }

    long nFileLen = ftell(m_fp);
    if (-1 == nFileLen) {
        return;
    }

    AX_U32 nLen = strlen(pStr);
    if (nFileLen + nLen > MAX_LOG_FILE_SIZE) {
        if (!SwitchFile()) {
            return;
        }
    }

    fwrite(pStr, 1, nLen, m_fp);
}

AX_BOOL CAppLog::SwitchFile(AX_VOID) {
    Close();

    for (AX_U32 i = MAX_APP_LOG_FILE_COUNT - 1; i > 0; i--) {
        if (MAX_APP_LOG_FILE_COUNT - 1 == i && 0 == access(m_arrFiles[i].c_str(), F_OK)) {
            if (0 != remove(m_arrFiles[i].c_str())) {
                printf("ERROR: Failed to remove app log file: %s!\n", m_arrFiles[i].c_str());
                return AX_FALSE;
            }
        }

        if (0 == access(m_arrFiles[i - 1].c_str(), F_OK)) {
            if (0 != rename(m_arrFiles[i - 1].c_str(), m_arrFiles[i].c_str())) {
                printf("ERROR: Failed to rename app log file: %s!\n", m_arrFiles[i - 1].c_str());
                return AX_FALSE;
            }
        }
    }

    m_fp = fopen(m_arrFiles[0].c_str(), "w");
    if (!m_fp) {
        printf("ERROR: Failed to open app log file: %s!\n", m_arrFiles[0].c_str());
        return AX_FALSE;
    }

    return AX_TRUE;
}