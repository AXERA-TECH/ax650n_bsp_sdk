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
#include "ax_global_type.h"
#include "AXSingleton.h"
#include "IniWrapper.hpp"

namespace aicard_slv {

typedef struct {
    AX_PAYLOAD_TYPE_E eVideoType;
    AX_U32 nUserPool;
    AX_U32 nMaxGrpW;
    AX_U32 nMaxGrpH;
    AX_U32 arrChnW[3];
    AX_U32 arrChnH[3];
    AX_U32 nDefaultFps;
    AX_S32 nChnDepth[3];
    AX_U32 nInputMode;
    AX_U32 nMaxStreamBufSize;
    AX_U32 nDecodeGrps;
    std::vector<std::string> v;
    AX_BOOL bEnableReset;
} VDEC_CONFIG_T;

typedef struct {
    AX_U32 nPPL;
    AX_U32 nVNPU;
    AX_BOOL bTrackEnable;
} DETECT_CHN_PARAM_T;

typedef struct {
    AX_BOOL bEnable;
    AX_BOOL bEnableSimulator {AX_FALSE};
    AX_U32 nW;
    AX_U32 nH;
    AX_U32 nSkipRate;
    AX_S32 nDepth;
    AX_S32 nChannelNum;
    DETECT_CHN_PARAM_T tChnParam[3];
    std::string strModelPath;
} DETECT_CONFIG_T;

typedef struct {
    AX_S16 nBuffSize {600};
    AX_U8  nBuffCount {2};
    AX_S16 nSendTimeout {-1};
    AX_S16 nRecvTimeout {-1};
    AX_S16 nTraceData {0};
    AX_S16 nRetryCount {1};
} PCIE_CONFIG_T;

/**
 * @brief
 *
 */
class CAiCardSlvConfig : public CAXSingleton<CAiCardSlvConfig> {
    friend class CAXSingleton<CAiCardSlvConfig>;

public:
    AX_BOOL Init(AX_VOID);

    VDEC_CONFIG_T   GetVdecConfig(AX_VOID);
    DETECT_CONFIG_T GetDetectConfig(AX_VOID);
    PCIE_CONFIG_T   GetPCIECofnig(AX_VOID);

private:
    CAiCardSlvConfig(AX_VOID) = default;
    virtual ~CAiCardSlvConfig(AX_VOID) = default;

    string GetExecPath(AX_VOID);

private:
    CIniWrapper m_iniParser;
};

}  // namespace aicard_slv
