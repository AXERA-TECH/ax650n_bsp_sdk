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

#include <map>
#include "AXSingleton.h"
#include "ax_base_type.h"
#include "cmdline.hpp"

#define AX_IPC_CMD_KEY_PPL "ppl"
#define AX_IPC_CMD_KEY_TYPE "type"
#define AX_IPC_CMD_KEY_SCENARIO "scenario"
#define AX_IPC_CMD_KEY_LOG_LEVEL "loglv"
#define AX_IPC_CMD_KEY_LOG_TARGET "logTarget"
#define AX_IPC_CMD_KEY_GDB_DEBUG "gdb"

#define AX_IPC_SCENARIO_SLT "scenario_slt"
#define AX_IPC_UT "unit_test"
#define AX_IPC_SCENARIO_INVALID "scenario_invalid"

typedef enum {
    E_LOAD_TYPE_DUAL_OS08A20 = 0,
    E_LOAD_TYPE_SINGLE_OS08A20 = 1,
    E_LOAD_TYPE_DUAL_OS08B10 = 2,
    E_LOAD_TYPE_SINGLE_OS08B10 = 3,
    E_LOAD_TYPE_SINGLE_SC910GS = 4,
    E_LOAD_TYPE_PANO_DUAL_OS04A10 = 5,
    E_LOAD_TYPE_SINGLE_OS08A20_PANO_DUAL_OS04A10 = 6,
    E_LOAD_TYPE_MAX
} AX_IPC_LOAD_TYPE_E;

class CCmdLineParser : public CAXSingleton<CCmdLineParser> {
    friend class CAXSingleton<CCmdLineParser>;

public:
    AX_S32 Parse(int argc, const char* const argv[]);

    AX_BOOL GetPPLIndex(AX_S32& nIndex);
    AX_BOOL GetLoadType(AX_S32& nType);
    AX_BOOL GetScenario(AX_S32& nScenario);
    AX_BOOL GetLogLevel(AX_S32& nLevel);
    AX_BOOL GetLogTarget(AX_S32& nLogTarget);

    static std::string ScenarioEnum2Str(AX_U8 nScenario);
    AX_BOOL isDulSnsMode();
    AX_BOOL isUTEnabled();

private:
    CCmdLineParser(AX_VOID) = default;
    ~CCmdLineParser(AX_VOID) = default;

    AX_BOOL GetIntValue(const std::string& strKey, AX_S32& nOutVal);
    AX_BOOL GetStrValue(const std::string& strKey, std::string& strOutVal);

private:
    std::map<std::string, std::string> m_mapParams;
};