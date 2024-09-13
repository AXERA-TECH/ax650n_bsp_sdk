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
#include <string>
#include <vector>
#include "AXSingleton.h"
#include "IniWrapper.hpp"

namespace boxconf {

typedef struct {
    AX_U32 nUserPool;
    AX_U32 nMaxGrpW;
    AX_U32 nMaxGrpH;
    AX_U32 nChnW[3];
    AX_U32 nChnH[3];
    AX_U32 nDefaultFps;
    AX_S32 nChnDepth[3];
    AX_U32 nInputMode;
    AX_U32 nMaxStreamBufSize;
    AX_U32 nDecodeGrps;
    AX_U64 nSataFileSize;
    AX_U64 nMaxSpaceSize;
    std::string strSataPath;
    std::vector<std::string> v;
    AX_U32 nLinkMode;
} STREAM_CONFIG_T;

typedef struct {
    AX_U32 nPPL;
    AX_U32 nVNPU;
    AX_BOOL bTrackEnable;
} DETECT_CHN_PARAM_T;

typedef struct {
    AX_BOOL bEnable;
    AX_U32 nW;
    AX_U32 nH;
    AX_U32 nSkipRate;
    AX_S32 nDepth;
    AX_S32 nChannelNum;
    AX_S32 nVnpuMode;
    DETECT_CHN_PARAM_T tChnParam[32];
    std::string strModelPath;
} DETECT_CONFIG_T;

typedef struct {
    AX_S32 nDevId;
    AX_U32 nHDMI;
    AX_U32 nChnDepth;
    AX_U32 nLayerDepth;
    AX_U32 nTolerance;
    AX_U32 nDispType;
    AX_BOOL bShowLogo;
    AX_BOOL bShowNoVideo;
    std::string strResDirPath;
    std::string strBmpPath;
    AX_BOOL bRecord;
    std::string strRecordPath;
    AX_S32 nMaxRecordSize;
    AX_BOOL bRecordMuxer;
    AX_BOOL bOnlineMode;
} DISPVO_CONFIG_T;

typedef struct {
    AX_U32 nFifoDepth[2];
    AX_U32 nPayloadType;
    AX_U32 nGop;
    AX_U32 nBitRate;
    AX_U32 nRCType;
    AX_U32 nMinQp;
    AX_U32 nMaxQp;
    AX_U32 nMinIQp;
    AX_U32 nMaxIQp;
    AX_U32 nMinIProp;
    AX_U32 nMaxIProp;
    AX_S32 nIntraQpDelta;
} VENC_CONFIG_T;

typedef struct {
    AX_U32 nMode;
    AX_U32 nLv;
} COMPRESS_CONFIG_T;

typedef struct {
    AX_U32 nMaxSendNaluIntervalMilliseconds;
} UT_CONFIG_T;

/**
 * @brief
 *
 */
class CBoxConfig : public CAXSingleton<CBoxConfig> {
    friend class CAXSingleton<CBoxConfig>;

public:
    AX_BOOL Init(AX_VOID);

    STREAM_CONFIG_T GetStreamConfig(AX_VOID);
    DETECT_CONFIG_T GetDetectConfig(AX_VOID);
    DISPVO_CONFIG_T GetDispVoConfig(const std::string &SECT);
    VENC_CONFIG_T GetVencConfig(AX_VOID);
    COMPRESS_CONFIG_T GetCompressConfig(AX_VOID);
    UT_CONFIG_T GetUTConfig(AX_VOID);

private:
    CBoxConfig(AX_VOID) = default;
    virtual ~CBoxConfig(AX_VOID) = default;

    std::string GetExecPath(AX_VOID);

private:
    CIniWrapper m_Ini;
};

}  // namespace boxconf
