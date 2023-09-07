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
#include <mutex>
#include <string>
#include "IAppLog.hpp"

#define MAX_APP_LOG_FILE_COUNT (5)

class CAppLog : public IAppLog {
public:
    CAppLog(AX_VOID) = default;
    virtual ~CAppLog(AX_VOID) = default;

    AX_BOOL Open(const APP_LOG_ATTR_T &stAttr) override;
    AX_VOID Log(AX_S32 nLv, const AX_CHAR *pStr) override;
    AX_VOID Close(AX_VOID) override;

protected:
    AX_BOOL SwitchFile(AX_VOID);

private:
    FILE *m_fp{nullptr};
    std::string m_arrFiles[MAX_APP_LOG_FILE_COUNT];
    std::mutex m_mtx;
};