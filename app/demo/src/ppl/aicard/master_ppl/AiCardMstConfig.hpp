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

namespace aicard_mst {

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
    std::vector<std::string> v;
} STREAM_CONFIG_T;

typedef struct {
    AX_U32 nDevId;
    AX_U32 nHDMI;
    AX_U32 nChnDepth;
    AX_U32 nLayerDepth;
    AX_U32 nTolerance;
    AX_BOOL bShowLogo;
    AX_BOOL bShowNoVideo;
    std::string strResDirPath;
    std::string strBmpPath;
} DISPVO_CONFIG_T;

typedef struct {
    AX_S16 nSlaveCount {1};
    AX_S16 nBuffSize {600};
    AX_U8  nBuffCount {2};
    AX_S16 nSendTimeout {-1};
    AX_S16 nRecvTimeout {-1};
    AX_S16 nTraceData  {0};
    AX_S16 nRetryCount {1};
} PCIE_CONFIG_T;

/**
 * @brief
 *
 */
class CAiCardMstConfig : public CAXSingleton<CAiCardMstConfig> {
    friend class CAXSingleton<CAiCardMstConfig>;

public:
    AX_BOOL Init(AX_VOID);

    STREAM_CONFIG_T GetStreamConfig(AX_VOID);
    DISPVO_CONFIG_T GetDispVoConfig(AX_VOID);
    PCIE_CONFIG_T   GetPCIECofnig(AX_VOID);

private:
    CAiCardMstConfig(AX_VOID) = default;
    virtual ~CAiCardMstConfig(AX_VOID) = default;

    string GetExecPath(AX_VOID);

private:
    CIniWrapper m_Ini;
};

}  // namespace aicard_mst
