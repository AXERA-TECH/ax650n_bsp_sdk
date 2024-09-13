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
#include "AppLog.hpp"
#include "SysLog.hpp"

class CAppLogWrapper {
public:
    CAppLogWrapper(AX_VOID) = default;
    virtual ~CAppLogWrapper(AX_VOID) = default;

    virtual AX_S32 Init(const APP_LOG_ATTR_T *pstAttr);
    virtual AX_VOID DeInit(AX_VOID);

    AX_S32 GetLogLevel(AX_VOID) const {
        return m_stAttr.nLv;
    };

    AX_VOID SetLogLevel(AX_S32 nLv) {
        if (nLv >= APP_LOG_CRITICAL && nLv < APP_LOG_BUTT) {
            m_stAttr.nLv = nLv;
        }
    };

    AX_VOID SetSysModuleInited(AX_BOOL bInited) {
        if (m_pSysLog) {
            m_pSysLog->SetSysModuleInited(bInited);
        }
    };

    virtual AX_VOID LogArgStr(AX_S32 nLv, const AX_CHAR *pFmt, va_list va);
    virtual AX_VOID LogFmtStr(AX_S32 nLv, const AX_CHAR *pFmt, ...);
    virtual AX_VOID LogBufData(AX_S32 nLv, const AX_VOID *pBuf, AX_U32 nBufSize, AX_U32 nFlag);

protected:
    AX_VOID Logging(AX_S32 nLv, const AX_CHAR *pStr) {
        if (m_pSysLog) {
            m_pSysLog->Log(nLv, pStr);
        }

        if (m_pStdLog) {
            m_pStdLog->Log(nLv, pStr);
        }

        if (m_pAppLog) {
            m_pAppLog->Log(nLv, pStr);
        }
    };

    AX_U64 GetTickCount(AX_VOID);

protected:
    APP_LOG_ATTR_T m_stAttr;
    CSysLog *m_pSysLog{nullptr};
    CStdLog *m_pStdLog{nullptr};
    CAppLog *m_pAppLog{nullptr};
};
