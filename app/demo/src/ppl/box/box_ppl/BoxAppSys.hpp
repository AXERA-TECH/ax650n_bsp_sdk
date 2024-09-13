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
#include <functional>
#include <string>
#include <vector>
#include "ax_global_type.h"

typedef struct {
    AX_U32 nMaxGrp;
} BOX_APP_SYS_ATTR_T;

class CBoxAppSys {
public:
    CBoxAppSys(AX_VOID) = default;
    virtual ~CBoxAppSys(AX_VOID) = default;

    AX_BOOL Init(const BOX_APP_SYS_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL InitAppLog(const std::string& strAppName);
    AX_BOOL DeInitAppLog(AX_VOID);

protected:
    static std::string GetSdkVersion(AX_VOID);

    virtual AX_BOOL InitSysMods(AX_VOID);
    virtual AX_BOOL DeInitSysMods(AX_VOID);

    virtual AX_S32 APP_SYS_Init(AX_VOID);
    virtual AX_S32 APP_SYS_DeInit(AX_VOID);
    virtual AX_S32 APP_VDEC_Init(AX_VOID);
    virtual AX_S32 APP_VENC_Init(AX_VOID);
    virtual AX_S32 APP_NPU_Init(AX_VOID);
    virtual AX_S32 APP_NPU_DeInit(AX_VOID);

private:
    typedef struct {
        AX_BOOL bInited;
        std::string strName;
        std::function<AX_S32(AX_VOID)> Init;
        std::function<AX_S32(AX_VOID)> DeInit;
    } SYS_MOD_T;

    BOX_APP_SYS_ATTR_T m_strAttr;
    std::vector<SYS_MOD_T> m_arrMods;
};
