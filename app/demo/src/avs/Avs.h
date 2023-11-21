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

#include  "AvsCfgParser.h"
#include "ax_avscali_api.h"

using namespace std;

#define MAX_AVS_PIPE_NUM            (4)
#define DEFAULT_AVS_CHN_OUT_WIDTH   (3840)
#define DEFAULT_AVS_CHN_OUT_HEIGHT  (1112)

typedef AX_VOID(*CBCaliDone)(AX_S32, AX_AVSCALI_AVS_PARAMS_T*, AX_AVSCALI_3A_SYNC_RATIO_T*, AX_VOID* pPrivData);

typedef struct _AX_APP_AVS_ATTR_T {
    AX_U8 u8PipeNum;
    AX_AVS_MODE_E enMode;
    AX_AVS_BLEND_MODE_E enBlendMode;
    AX_APP_AVS_PARAM_TYPE_E enParamType;
    AX_AVS_PROJECTION_MODE_E enProjectionType;
    AX_BOOL bSyncPipe;
    AX_BOOL bDynamicSeam;
    AX_FRAME_COMPRESS_INFO_T stAvsCompress;
    AX_U8 u8CaliEnable;
    string strCaliServerIP;
    AX_U16 u16CaliServerPort;

    AX_AVSCALI_INIT_PARAM_T tCaliInitParam;

    _AX_APP_AVS_ATTR_T() {
        memset((AX_VOID *)&tCaliInitParam, 0, sizeof(AX_AVSCALI_INIT_PARAM_T));

        u8PipeNum = 2;
        enMode = AVS_MODE_BLEND;
        enBlendMode = AVS_BLEND_ALPHA;
        enParamType = E_AVS_PARAM_TYPE_NORMAL;
        enProjectionType = AVS_PROJECTION_RECTLINEAR;
        bSyncPipe = AX_TRUE;
        bDynamicSeam = AX_TRUE;
        stAvsCompress.enCompressMode = AX_COMPRESS_MODE_LOSSY;
        stAvsCompress.u32CompressLevel = 4;
        u8CaliEnable = 0;
        strCaliServerIP = "";
        u16CaliServerPort = 9999;
        tCaliInitParam.tSnsInfo.nSnsNum = u8PipeNum;
        tCaliInitParam.tSnsInfo.nMasterPipeId = 0;
        tCaliInitParam.tSnsInfo.nChn = 0;
        tCaliInitParam.tSnsInfo.nImgWidth = 2688;
        tCaliInitParam.tSnsInfo.nImgHeight = 1520;
        tCaliInitParam.tCallbacks.CaliDoneCb = nullptr;
        tCaliInitParam.pPrivData = nullptr;
    }
} AX_APP_AVS_ATTR_T, *AX_APP_AVS_ATTR_PTR;

typedef struct _AVS_MEM_ADDR_T {
    AX_U64   u64PhyAddr;
    AX_VOID* pVirAddr;
} AX_APP_AVS_MEM_ADDR_T;

typedef struct _AVS_RESOLUTION_T {
    AX_U32 u32Width;
    AX_U32 u32Height;
} AX_APP_AVS_RESOLUTION_T;

class CAvs{
public:
    CAvs(AX_VOID) = default;

    AX_BOOL Init(const AX_APP_AVS_ATTR_T& stAVSAttr);
    AX_BOOL DeInit(AX_BOOL bDeInitCali = AX_FALSE);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_BOOL bStopCali = AX_FALSE);

    AX_S32  LoadParam(AX_VOID);
    AX_BOOL UpdateParam(AX_VOID);

    AX_S32  Get3ASyncRatio(AX_AVSCALI_3A_SYNC_RATIO_T& t3ASyncRatio);

    AX_BOOL StartAVSCalibrate();

    AX_APP_AVS_RESOLUTION_T GetAvsResolution(AX_VOID) {
        return m_stAvsResolution;
    }

    AX_VOID SetCaliDataPath(string path) {
        m_strCaliDataPath = path;
    }

private:

    AX_APP_AVS_ATTR_T m_stAvsAttr;
    AX_AVS_GRP_ATTR_T m_stAvsGrpAttr;
    AX_AVS_CHN_ATTR_T m_stAvsChnAttr;
    AX_APP_AVS_RESOLUTION_T m_stAvsResolution {DEFAULT_AVS_CHN_OUT_WIDTH,
                                               DEFAULT_AVS_CHN_OUT_HEIGHT};

    AX_APP_AVS_MEM_ADDR_T m_stAvsMeshAddr[MAX_AVS_PIPE_NUM]     {0};
    AX_APP_AVS_MEM_ADDR_T m_stAvsMaskAddr[MAX_AVS_PIPE_NUM - 1] {0};

    AX_BOOL m_bIsCalibrated {AX_FALSE};
    AX_AVS_GRP m_nGrp {0};
    AX_AVS_CHN m_nChn {0};

    string m_strCaliDataPath {""};
    string m_SDKVersion{""};
};