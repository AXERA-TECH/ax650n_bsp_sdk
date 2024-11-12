/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiCardSlvAppSys.hpp"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "AXPoolManager.hpp"
#include "AppLogApi.h"
#include "arraysize.h"
#include "ax_engine_api.h"
#include "ax_ivps_api.h"
#include "ax_sys_api.h"
#include "ax_vdec_api.h"
#include "AiCardSlvConfig.hpp"

#define SLV_APP "AICARD_SLV_APP"

using namespace std;
using namespace aicard_slv;

AX_BOOL CAiCardSlvAppSys::Init(const AICARD_SLV_SYS_ATTR_T& stAttr, const std::string& strAppName) {
    if (0 == stAttr.nMaxGrp) {
        LOG_MM_E(SLV_APP, "Group count is zero.");
        return AX_FALSE;
    } else {
        m_strAttr = stAttr;
    }

    if (!InitAppLog(strAppName)) {
        return AX_FALSE;
    }

    if (!InitSysMods()) {
        DeInitAppLog();
        return AX_FALSE;
    }

    LOG_MM_C(SLV_APP, "============== APP(APP Ver: %s, SDK Ver: %s) Started %s %s ==============\n", APP_BUILD_VERSION, GetSdkVersion().c_str(),
          __DATE__, __TIME__);
    return AX_TRUE;
}

AX_BOOL CAiCardSlvAppSys::DeInit(AX_VOID) {
    if (!DeInitSysMods()) {
        return AX_FALSE;
    }

    LOG_MM_C(SLV_APP, "============== APP(APP Ver: %s, SDK Ver: %s) Exited %s %s ==============\n", APP_BUILD_VERSION, GetSdkVersion().c_str(),
          __DATE__, __TIME__);

    DeInitAppLog();

    return AX_TRUE;
}

AX_BOOL CAiCardSlvAppSys::InitAppLog(const string& strAppName) {
    APP_LOG_ATTR_T stAttr;
    memset(&stAttr, 0, sizeof(stAttr));
    stAttr.nTarget = APP_LOG_TARGET_STDOUT;
    stAttr.nLv = APP_LOG_WARN;
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

AX_BOOL CAiCardSlvAppSys::DeInitAppLog(AX_VOID) {
    AX_APP_Log_DeInit();
    return AX_TRUE;
}

AX_BOOL CAiCardSlvAppSys::InitSysMods(AX_VOID) {
    m_arrMods.clear();
    m_arrMods.reserve(3);
    m_arrMods.push_back({AX_FALSE, "SYS", bind(&CAiCardSlvAppSys::APP_SYS_Init, this), bind(&CAiCardSlvAppSys::APP_SYS_DeInit, this)});
    m_arrMods.push_back({AX_FALSE, "VDEC", bind(&CAiCardSlvAppSys::APP_VDEC_Init, this), AX_VDEC_Deinit});
    m_arrMods.push_back({AX_FALSE, "NPU", bind(&CAiCardSlvAppSys::APP_NPU_Init, this), bind(&CAiCardSlvAppSys::APP_NPU_DeInit, this)});

    for (auto& m : m_arrMods) {
        AX_S32 ret = m.Init();
        if (0 != ret) {
            LOG_MM_E(SLV_APP, "Init module %s fail, ret = 0x%x", m.strName.c_str(), ret);
            return AX_FALSE;
        } else {
            m.bInited = AX_TRUE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAiCardSlvAppSys::DeInitSysMods(AX_VOID) {
    const auto nSize = m_arrMods.size();
    if (0 == nSize) {
        return AX_TRUE;
    }

    for (AX_S32 i = (AX_S32)(nSize - 1); i >= 0; --i) {
        if (m_arrMods[i].bInited) {
            AX_S32 ret = m_arrMods[i].DeInit();
            if (0 != ret) {
                LOG_MM_E(SLV_APP, "Deinit module %s fail, ret = 0x%x", m_arrMods[i].strName.c_str(), ret);
                return AX_FALSE;
            }

            m_arrMods[i].bInited = AX_FALSE;
        }
    }

    m_arrMods.clear();
    return AX_TRUE;
}

AX_S32 CAiCardSlvAppSys::APP_VDEC_Init(AX_VOID) {
    AX_VDEC_MOD_ATTR_T stModAttr;
    memset(&stModAttr, 0, sizeof(stModAttr));
    stModAttr.u32MaxGroupCount = m_strAttr.nMaxGrp;
    stModAttr.enDecModule = AX_ENABLE_ONLY_VDEC;
    AX_S32 ret = AX_VDEC_Init(&stModAttr);
    if (0 != ret) {
        LOG_MM_E(SLV_APP, "AX_VDEC_Init() fail, ret = 0x%x", ret);
        return ret;
    }

    return 0;
}

AX_S32 CAiCardSlvAppSys::APP_SYS_Init(AX_VOID) {
    AX_S32 ret = AX_SYS_Init();
    if (0 != ret) {
        LOG_MM_E(SLV_APP, "AX_SYS_Init() fail, ret = 0x%x", ret);
        return ret;
    }

    AX_APP_Log_SetSysModuleInited(AX_TRUE);

    ret = AX_POOL_Exit();
    if (0 != ret) {
        LOG_MM_E(SLV_APP, "AX_POOL_Exit() fail, ret = 0x%x", ret);
        return ret;
    }

    return 0;
}

AX_S32 CAiCardSlvAppSys::APP_SYS_DeInit(AX_VOID) {
    AX_S32 ret = AX_SUCCESS;

    if (!CAXPoolManager::GetInstance()->DestoryAllPools()) {
        return -1;
    }

    AX_APP_Log_SetSysModuleInited(AX_FALSE);

    ret = AX_SYS_Deinit();
    if (0 != ret) {
        LOG_MM_E(SLV_APP, "AX_SYS_Deinit() fail, ret = 0x%x", ret);
        return ret;
    }

    return 0;
}

AX_S32 CAiCardSlvAppSys::APP_NPU_Init(AX_VOID) {
    AX_ENGINE_NPU_ATTR_T stAttr;
    memset(&stAttr, 0, sizeof(stAttr));

    CAiCardSlvConfig *pConfig = CAiCardSlvConfig::GetInstance();
    if (!pConfig->Init()) {
        LOG_MM_E(SLV_APP, "Load aicard slave config file failed.");
        return -1;
    }
    DETECT_CONFIG_T detectConfig = pConfig->GetDetectConfig();

    if (detectConfig.nChannelNum > 1) {
        stAttr.eHardMode = AX_ENGINE_VIRTUAL_NPU_STD;
    }
    else {
        stAttr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
    }

    AX_S32 ret = AX_ENGINE_Init(&stAttr);
    if (0 != ret) {
        LOG_E("%s: AX_ENGINE_Init() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

AX_S32 CAiCardSlvAppSys::APP_NPU_DeInit(AX_VOID) {
    AX_S32 ret = AX_ENGINE_Deinit();
    if (0 != ret) {
        LOG_E("%s: AX_ENGINE_Deinit() fail, ret = 0x%x", __func__, ret);
        return ret;
    }

    return 0;
}

string CAiCardSlvAppSys::GetSdkVersion(AX_VOID) {
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
