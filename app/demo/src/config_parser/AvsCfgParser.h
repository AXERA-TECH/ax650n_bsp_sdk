/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "picojson.h"
#include "AXSingleton.h"
#include "ax_avs_api.h"

typedef struct axAPP_AVS_CFG_T {
    AX_U8 u8PipeNum;
    AX_U8 u8Mode;
    AX_U8 u8BlendMode;
    AX_U8 u8ParamType;
    AX_U8 u8ProjectionType;
    AX_BOOL bSyncPipe;
    AX_BOOL bDynamicSeam;
    AX_FRAME_COMPRESS_INFO_T stAvsCompress;
    std::string strParamFilePath;
    AX_U8 u8CaliEnable;
    std::string strCaliServerIP;
    AX_U16 u16CaliServerPort;

    AX_U8 arrPipeId[AX_AVS_PIPE_NUM];
    AX_U8 arrChnId[AX_AVS_PIPE_NUM];
    AX_U8 u8MasterPipeId;
} AX_APP_AVS_CFG_T, *AX_APP_AVS_CFG_PTR;

typedef enum {
    E_AVS_PARAM_TYPE_NORMAL = 0,
    E_AVS_PARAM_TYPE_MESHTABLE,
    E_AVS_PARAM_TYPE_MAX,
} AX_APP_AVS_PARAM_TYPE_E;

class CAvsCfgParser : public CAXSingleton<CAvsCfgParser> {
    friend class CAXSingleton<CAvsCfgParser>;

public:
    AX_BOOL GetConfig(AX_APP_AVS_CFG_T& stAvsCfg);

private:
    CAvsCfgParser(AX_VOID) = default;
    ~CAvsCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;
    AX_BOOL ParseFile(const std::string& strPath, AX_APP_AVS_CFG_T& stAvsCfg);
    AX_BOOL ParseJson(picojson::object& objJsonRoot, AX_APP_AVS_CFG_T& stAvsCfg);
};