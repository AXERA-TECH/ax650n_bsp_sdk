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
#include "AppLogApi.h"

class IAppLog {
public:
    virtual ~IAppLog(AX_VOID) = default;

    virtual AX_BOOL Open(const APP_LOG_ATTR_T &stAttr) = 0;
    virtual AX_VOID Log(AX_S32 nLv, const AX_CHAR *pStr) = 0;
    virtual AX_VOID Close(AX_VOID) = 0;
};