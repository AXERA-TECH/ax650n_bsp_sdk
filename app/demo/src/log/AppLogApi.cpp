/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AppLogApi.h"
#include "AppLogWrapper.hpp"

static CAppLogWrapper logger;

AX_S32 AX_APP_Log_Init(const APP_LOG_ATTR_T *pstAttr) {
    return logger.Init(pstAttr);
}

AX_VOID AX_APP_Log_DeInit(AX_VOID) {
    logger.DeInit();
}

AX_S32 AX_APP_GetLogLevel(AX_VOID) {
    return logger.GetLogLevel();
}

AX_VOID AX_APP_SetLogLevel(AX_S32 nLv) {
    logger.SetLogLevel(nLv);
}

AX_VOID AX_APP_Log_SetSysModuleInited(AX_BOOL bInited) {
    logger.SetSysModuleInited(bInited);
}

AX_VOID AX_APP_LogFmtStr(AX_S32 nLv, const AX_CHAR *pFmt, ...) {
    va_list args;
    va_start(args, pFmt);
    logger.LogArgStr(nLv, pFmt, args);
    va_end(args);
}

AX_VOID AX_APP_LogBufData(AX_S32 nLv, const AX_VOID *pBuf, AX_U32 nBufSize, AX_U32 nFlag) {
    logger.LogBufData(nLv, pBuf, nBufSize, nFlag);
}
