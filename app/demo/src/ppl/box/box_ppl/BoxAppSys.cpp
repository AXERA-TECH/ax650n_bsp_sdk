/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "BoxAppSys.hpp"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "AXPoolManager.hpp"
#include "AppLogApi.h"
#include "BoxConfig.hpp"
#include "arraysize.h"
#include "ax_engine_api.h"
#include "ax_ivps_api.h"
#include "ax_sys_api.h"
#include "ax_vdec_api.h"
#include "ax_venc_api.h"
#include "ax_vo_api.h"

using namespace std;
using namespace boxconf;

AX_BOOL CBoxAppSys::Init(const BOX_APP_SYS_ATTR_T& stAttr) {
    if (0 == stAttr.nMaxGrp) {
        LOG_E("%s: 0 grp", __func__);
        return AX_FALSE;
    } else {
        m_strAttr = stAttr;
    }

    if (!InitSysMods()) {
        DeInitAppLog();
        return AX_FALSE;
    }

    LOG_C("============== APP(APP Ver: %s, SDK Ver: %s) Started %s %s ==============\n", APP_BUILD_VERSION, GetSdkVersion().c_str(),
          __DATE__, __TIME__);
    return AX_TRUE;
}

AX_BOOL CBoxAppSys::DeInit(AX_VOID) {
    if (!DeInitSysMods()) {
        return AX_FALSE;
    }

    LOG_C("============== APP(APP Ver: %s, SDK Ver: %s) Exited %s %s ==============\n", APP_BUILD_VERSION, GetSdkVersion().c_str(),
          __DATE__, __TIME__);

    return AX_TRUE;
}

AX_BOOL CBoxAppSys::InitAppLog(const string& strAppName) {
    APP_LOG_ATTR_T stAttr;
    memset(&stAttr, 0, sizeof(stAttr));
    stAttr.nTarget = APP_LOG_TARGET_STDOUT;
#if defined(__SLT__)
    stAttr.nLv = APP_LOG_ERROR;
#else
    stAttr.nLv = APP_LOG_WARN;
#endif
    strncpy(stAttr.szAppName, strAppName.c_str(), arraysize(stAttr.szAppName) - 1);

    AX_CHAR* env1 = getenv("APP_LOG_TARGET");
    if (env1) {
        stAttr.nTarget = atoi(env1);
    }

    AX_CHAR* env2 = getenv("APP_LOG_LEVEL");
    if (env2) {
        stAttr.nLv = atoi(env2);
    }

    return (0 == AX_APP_Log_Init(&stAttr)) ? AX_TRUE : AX_FALSE;
}

AX_BOOL CBoxAppSys::DeInitAppLog(AX_VOID) {
    AX_APP_Log_DeInit();
    return AX_TRUE;
}

AX_BOOL CBoxAppSys::InitSysMods(AX_VOID) {
    m_arrMods.clear();
    m_arrMods.reserve(8);
    m_arrMods.push_back({AX_FALSE, "SYS", bind(&CBoxAppSys::APP_SYS_Init, this), bind(&CBoxAppSys::APP_SYS_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "VDEC", bind(&CBoxAppSys::APP_VDEC_Init, this), AX_VDEC_Deinit});
    m_arrMods.push_back({AX_FALSE, "VENC", bind(&CBoxAppSys::APP_VENC_Init, this), AX_VENC_Deinit});
    m_arrMods.push_back({AX_FALSE, "IVPS", AX_IVPS_Init, AX_IVPS_Deinit});

#ifndef __DUMMY_VO__
    m_arrMods.push_back({AX_FALSE, "DISP", AX_VO_Init, AX_VO_Deinit});
#endif
    m_arrMods.push_back({AX_FALSE, "NPU", bind(&CBoxAppSys::APP_NPU_Init, this), bind(&CBoxAppSys::APP_NPU_DeInit, this)});

    for (auto& m : m_arrMods) {
        AX_S32 ret = m.Init();
        if (0 != ret) {
            LOG_E("%s: init module %s fail, ret = 0x%x", __func__, m.strName.c_str(), ret);
            return AX_FALSE;
        } else {
            m.bInited = AX_TRUE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBoxAppSys::DeInitSysMods(AX_VOID) {
    const auto nSize = m_arrMods.size();
    if (0 == nSize) {
        return AX_TRUE;
    }

    for (AX_S32 i = (AX_S32)(nSize - 1); i >= 0; --i) {
        if (m_arrMods[i].bInited) {
            AX_S32 ret = m_arrMods[i].DeInit();
            if (0 != ret) {
                LOG_E("%s: deinit module %s fail, ret = 0x%x", __func__, m_arrMods[i].strName.c_str(), ret);
                return AX_FALSE;
            }

            m_arrMods[i].bInited = AX_FALSE;
        }
    }

    m_arrMods.clear();
    return AX_TRUE;
}

AX_S32 CBoxAppSys::APP_VDEC_Init(AX_VOID) {
    AX_VDEC_MOD_ATTR_T stModAttr;
    memset(&stModAttr, 0, sizeof(stModAttr));
    stModAttr.u32MaxGroupCount = m_strAttr.nMaxGrp;
    stModAttr.enDecModule = AX_ENABLE_ONLY_VDEC;
    AX_S32 ret = AX_VDEC_Init(&stModAttr);
    if (0 != ret) {
        LOG_E("%s: AX_VDEC_Init() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

AX_S32 CBoxAppSys::APP_VENC_Init(AX_VOID) {
    AX_VENC_MOD_ATTR_T stModAttr;
    memset(&stModAttr, 0, sizeof(stModAttr));
    stModAttr.enVencType = AX_VENC_VIDEO_ENCODER;
    stModAttr.stModThdAttr.bExplicitSched = AX_FALSE;
    stModAttr.stModThdAttr.u32TotalThreadNum = 1;
    AX_S32 ret = AX_VENC_Init(&stModAttr);
    if (0 != ret) {
        LOG_E("%s: AX_VENC_Init() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

AX_S32 CBoxAppSys::APP_SYS_Init(AX_VOID) {
    AX_S32 ret = AX_SYS_Init();
    if (0 != ret) {
        LOG_E("%s: AX_SYS_Init() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    AX_APP_Log_SetSysModuleInited(AX_TRUE);

    ret = AX_POOL_Exit();
    if (0 != ret) {
        LOG_E("%s: AX_POOL_Exit() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

AX_S32 CBoxAppSys::APP_SYS_DeInit(AX_VOID) {
    AX_S32 ret = AX_SUCCESS;

    if (!CAXPoolManager::GetInstance()->DestoryAllPools()) {
        return -1;
    }

    AX_APP_Log_SetSysModuleInited(AX_FALSE);

    ret = AX_SYS_Deinit();
    if (0 != ret) {
        LOG_E("%s: AX_SYS_Deinit() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

AX_S32 CBoxAppSys::APP_NPU_Init(AX_VOID) {
    AX_ENGINE_NPU_ATTR_T stAttr;
    memset(&stAttr, 0, sizeof(stAttr));

    CBoxConfig* pConfig = CBoxConfig::GetInstance();
    if (!pConfig->Init()) {
        LOG_E("%s: load box config file fail", __func__);
        return -1;
    }
    DETECT_CONFIG_T detectConfig = pConfig->GetDetectConfig();

    if (detectConfig.nVnpuMode >= AX_ENGINE_VIRTUAL_NPU_BUTT) {
        LOG_E("%s: nVnpuMode error: %d", __func__, detectConfig.nVnpuMode);
        return -1;
    }

    stAttr.eHardMode = (AX_ENGINE_NPU_MODE_T)detectConfig.nVnpuMode;

    AX_S32 ret = AX_ENGINE_Init(&stAttr);
    if (0 != ret) {
        LOG_E("%s: AX_ENGINE_Init() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

AX_S32 CBoxAppSys::APP_NPU_DeInit(AX_VOID) {
    AX_S32 ret = AX_ENGINE_Deinit();
    if (0 != ret) {
        LOG_E("%s: AX_ENGINE_Deinit() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

string CBoxAppSys::GetSdkVersion(AX_VOID) {
    string strSdkVer{"Unknown"};
    if (FILE* fp = fopen("/proc/ax_proc/version", "r")) {
        constexpr AX_U32 SDK_VERSION_PREFIX_LEN = strlen("Ax_Version") + 1;
        AX_CHAR szSdkVer[64] = {0};
        fread(&szSdkVer[0], 64, 1, fp);
        fclose(fp);
        szSdkVer[strlen(szSdkVer) - 1] = 0;
        strSdkVer = szSdkVer + SDK_VERSION_PREFIX_LEN;
    }

    return strSdkVer;
}
