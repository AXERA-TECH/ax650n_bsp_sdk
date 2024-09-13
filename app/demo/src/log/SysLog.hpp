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
#include "IAppLog.hpp"
#ifndef __X86__
#include <ax_sys_log.h>
#endif
#include <stdio.h>
#include <string.h>

class CStdLog : public IAppLog {
public:
    CStdLog(AX_VOID) = default;
    virtual ~CStdLog(AX_VOID) = default;

    AX_BOOL Open(const APP_LOG_ATTR_T &stAttr) override {
        return AX_TRUE;
    };

    AX_VOID Log(AX_S32 nLv, const AX_CHAR *pStr) override {
        fprintf(stdout, "%s", pStr);
    };

    AX_VOID Close(AX_VOID) override{};
};

class CSysLog : public IAppLog {
public:
    CSysLog(AX_VOID) = default;
    virtual ~CSysLog(AX_VOID) = default;

    AX_BOOL Open(const APP_LOG_ATTR_T &stAttr) override {
        strcpy(m_szAppName, stAttr.szAppName);
        return AX_TRUE;
    };

    AX_VOID Log(AX_S32 nLv, const AX_CHAR *pStr) override {
#ifndef __X86__
        if (m_bSysModuleInited) {
            AX_SYS_LogPrint_Ex(nLv, m_szAppName, AX_ID_USER, "%s", pStr);
        } else {
            fprintf(stdout, "%s", pStr);
        }
#endif
    };

    AX_VOID Close(AX_VOID) override{};

    AX_VOID SetSysModuleInited(AX_BOOL bInited) {
        m_bSysModuleInited = bInited;
    };

private:
    AX_CHAR m_szAppName[MAX_APP_NAME_LEN]{"APP"};
    AX_BOOL m_bSysModuleInited{AX_FALSE};
};