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
    std::string strParamFilePath;
    AX_U8 u8CaliEnable;
    std::string strCaliServerIP;
    AX_U16 u16CaliServerPort;

    AX_U8 arrPipeId[AX_AVS_PIPE_NUM];
    AX_U8 arrChnId[AX_AVS_PIPE_NUM];
    AX_U8 u8MasterPipeId;

    AX_U8 u8GrpId;
    AX_U8 u8ChnNum;
    AX_U8 u8ChnId;
    AX_F32 arrPipeFrameRate[AX_AVS_PIPE_NUM];
    AX_F32 arrGrpFrameRate[2];
    AX_F32 arrChnFrameRate[2];
    AX_S16 arrPipeResolution[AX_AVS_PIPE_NUM][2];
    AX_S16 arrGrpResolution[2];
    AX_S16 arrChnResolution[2];
    AX_U8  arrPipeLinkFlag[AX_AVS_PIPE_NUM];
    AX_U8  arrGrpFBC[2];
    AX_U8  arrChnFBC[2];
    AX_BOOL bChnInplace;
} AX_APP_AVS_CFG_T, *AX_APP_AVS_CFG_PTR;

typedef enum {
    E_AVS_PARAM_TYPE_NORMAL = 0,
    E_AVS_PARAM_TYPE_MESHTABLE,
    E_AVS_PARAM_TYPE_MAX,
} PANO_AVS_PARAM_TYPE_E;

class CAVSCfgParser : public CAXSingleton<CAVSCfgParser> {
    friend class CAXSingleton<CAVSCfgParser>;

public:
    AX_BOOL GetConfig(AX_APP_AVS_CFG_T& stAVSCfg);

private:
    CAVSCfgParser(AX_VOID) = default;
    ~CAVSCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;
    AX_BOOL ParseFile(const std::string& strPath, AX_APP_AVS_CFG_T& stAVSCfg);
    AX_BOOL ParseJson(picojson::object& objJsonRoot, AX_APP_AVS_CFG_T& stAVSCfg);
};