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

#include  "PanoAVSCfgParser.h"
#include "ax_avscali_api.h"

#define MAX_PANO_SNS_NUM 2

typedef AX_VOID(*CBCaliDone)(AX_S32, AX_AVSCALI_AVS_PARAMS_T*, AX_AVSCALI_3A_SYNC_RATIO_T*);

typedef struct _PANO_AVS_ATTR_T {
    AX_U8 u8PipeNum;
    AX_AVS_MODE_E enMode;
    AX_AVS_BLEND_MODE_E enBlendMode;
    PANO_AVS_PARAM_TYPE_E enParamType;
    AX_AVS_PROJECTION_MODE_E enProjectionType;
    AX_BOOL bSyncPipe;
    AX_BOOL bDynamicSeam;
    std::string strParamFilePath;
    AX_U8 u8CaliEnable;
    std::string strCaliServerIP;
    AX_U16 u16CaliServerPort;
    CBCaliDone pCaliDone;

    AX_U8 arrPipeId[AX_AVS_PIPE_NUM];
    AX_U8 arrChnId[AX_AVS_PIPE_NUM];
    AX_U8 u8MasterPipeId;
    AX_S16 arrPipeResolution[AX_AVS_PIPE_NUM][2];
    _PANO_AVS_ATTR_T() {
        u8PipeNum = 2;
        enMode = AVS_MODE_BLEND;
        enBlendMode = AVS_BLEND_ALPHA;
        enParamType = E_AVS_PARAM_TYPE_NORMAL;
        enProjectionType = AVS_PROJECTION_RECTLINEAR;
        bSyncPipe = AX_TRUE;
        bDynamicSeam = AX_TRUE;
        strParamFilePath = "/opt/param/";
        u8CaliEnable = 0;
        strCaliServerIP = "";
        u16CaliServerPort = 9999;
        u8MasterPipeId = 0;
    }
}PANO_AVS_ATTR_T, *PANO_AVS_ATTR_PTR;

typedef struct _PANO_AVS_MEM_ADDR_T {
    AX_U64   u64PhyAddr;
    AX_VOID* pVirAddr;
}PANO_AVS_MEM_ADDR_T;

typedef struct _PANO_AVS_RESOLUTION_T {
    AX_U32 u32Width;
    AX_U32 u32Height;
}PANO_AVS_RESOLUTION_T;

class CPanoAVS{
public:
    CPanoAVS(AX_VOID) = default;

    AX_BOOL Init(const PANO_AVS_ATTR_T& stPanoAVSAttr);
    AX_BOOL DeInit(AX_BOOL bDeInitCali = AX_FALSE);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_BOOL bStopCali = AX_FALSE);

    AX_S32 LoadParam(AX_VOID);
    AX_BOOL UpdateParam(AX_VOID);

    PANO_AVS_RESOLUTION_T GetCaliReso(AX_VOID) {
        return m_stCaliReso;
    }

    AX_VOID SetCaliDataPath(std::string path) {
        m_strCaliDataPath = path;
    }

private:
    AX_S32 CalibrateSensors(AX_U32 width, AX_U32 height);

    PANO_AVS_ATTR_T m_stAttr;
    AX_AVS_GRP_ATTR_T m_stGrpAttr;
    AX_AVS_CHN_ATTR_T m_stAVSChnAttr;
    PANO_AVS_RESOLUTION_T m_stCaliReso;

    PANO_AVS_MEM_ADDR_T m_stMeshAddr[MAX_PANO_SNS_NUM] { 0 };
    PANO_AVS_MEM_ADDR_T m_stMaskAddr[MAX_PANO_SNS_NUM - 1] { 0 };

    AX_BOOL m_bIsCalibrated { AX_FALSE };
    AX_AVS_GRP m_nGrp { 0 };
    AX_AVS_CHN m_nChn { 0 };

    std::string m_strCaliDataPath {""};
};