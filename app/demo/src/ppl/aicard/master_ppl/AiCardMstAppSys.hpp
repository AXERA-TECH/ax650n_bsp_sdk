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
} AICARD_APP_SYS_ATTR_T;

class CAiCardMstAppSys {
public:
    CAiCardMstAppSys(AX_VOID) = default;
    virtual ~CAiCardMstAppSys(AX_VOID) = default;

    AX_BOOL Init(const AICARD_APP_SYS_ATTR_T& stAttr, const std::string& strAppName = "AiCardMst");
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Link(const AX_MOD_INFO_T& tSrc, const AX_MOD_INFO_T& tDst);
    AX_BOOL UnLink(const AX_MOD_INFO_T& tSrc, const AX_MOD_INFO_T& tDst);

protected:
    static std::string GetSdkVersion(AX_VOID);

    AX_BOOL InitAppLog(const std::string& strAppName);
    AX_BOOL DeInitAppLog(AX_VOID);

    virtual AX_BOOL InitSysMods(AX_VOID);
    virtual AX_BOOL DeInitSysMods(AX_VOID);

    virtual AX_S32 APP_SYS_Init(AX_VOID);
    virtual AX_S32 APP_SYS_DeInit(AX_VOID);
    virtual AX_S32 APP_VDEC_Init(AX_VOID);

private:
    typedef struct {
        AX_BOOL bInited;
        std::string strName;
        std::function<AX_S32(AX_VOID)> Init;
        std::function<AX_S32(AX_VOID)> DeInit;
    } SYS_MOD_T;

    AICARD_APP_SYS_ATTR_T m_strAttr;
    std::vector<SYS_MOD_T> m_arrMods;
};
