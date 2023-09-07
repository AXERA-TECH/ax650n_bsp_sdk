/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "CmdLineParser.h"
#include <utility>
#include "AppLogApi.h"
#include "IPPLBuilder.h"

using namespace std;

#define CMD_PARSER "CMD_PARSER"

AX_S32 CCmdLineParser::Parse(int argc, const char* const argv[]) {
    m_mapParams.clear();

    std::unique_ptr<cmdline::parser> pParser;
    pParser = make_unique<cmdline::parser>();

    pParser->add<string>(AX_IPC_CMD_KEY_PPL, 'p', "ppl index", true, "0");
    pParser->add<string>(AX_IPC_CMD_KEY_TYPE, 's', "sensor type", true, "0");
    pParser->add<string>(AX_IPC_CMD_KEY_SCENARIO, 'n', "scenario", false, "0");
    pParser->add<string>(AX_IPC_CMD_KEY_LOG_LEVEL, 'l', "log level", false, "2");
    pParser->add<string>(AX_IPC_CMD_KEY_LOG_TARGET, 't', "log target", false, "4");
    pParser->add(AX_IPC_CMD_KEY_GDB_DEBUG, 'd', "gdb debug");
    pParser->add<string>(AX_IPC_UT, 'u', "ut case", false, "0");

    if (!pParser->parse(argc, argv)) {
        printf("CMDLine parameter parse failed.\n");
        return -1;
    }

    m_mapParams.insert(make_pair(AX_IPC_CMD_KEY_PPL, pParser->get<string>(AX_IPC_CMD_KEY_PPL)));
    m_mapParams.insert(make_pair(AX_IPC_CMD_KEY_TYPE, pParser->get<string>(AX_IPC_CMD_KEY_TYPE)));
    m_mapParams.insert(make_pair(AX_IPC_CMD_KEY_SCENARIO, pParser->get<string>(AX_IPC_CMD_KEY_SCENARIO)));
    m_mapParams.insert(make_pair(AX_IPC_CMD_KEY_LOG_LEVEL, pParser->get<string>(AX_IPC_CMD_KEY_LOG_LEVEL)));
    m_mapParams.insert(make_pair(AX_IPC_CMD_KEY_LOG_TARGET, pParser->get<string>(AX_IPC_CMD_KEY_LOG_TARGET)));
    m_mapParams.insert(make_pair(AX_IPC_UT, pParser->get<string>(AX_IPC_UT)));

    return 0;
}

AX_BOOL CCmdLineParser::GetIntValue(const string& strKey, AX_S32& nOutVal) {
    std::map<string, string>::iterator it = m_mapParams.find(strKey);
    if (it != m_mapParams.end()) {
        nOutVal = atoi(it->second.c_str());
        return AX_TRUE;
    } else {
        LOG_MM_W(CMD_PARSER, "Can not get value for key: %s", strKey.c_str());
    }

    return AX_FALSE;
}

AX_BOOL CCmdLineParser::GetStrValue(const string& strKey, string& strOutVal) {
    std::map<string, string>::iterator it = m_mapParams.find(strKey);
    if (it != m_mapParams.end()) {
        strOutVal = it->second;
        return AX_TRUE;
    }

    return AX_FALSE;
}

string CCmdLineParser::ScenarioEnum2Str(AX_U8 nScenario) {
    std::string strScenario;
    switch ((AX_IPC_SCENARIO_E)nScenario) {
        case E_PPL_SCENRIO_0:
        case E_PPL_SCENRIO_1:
        case E_PPL_SCENRIO_2:
        case E_PPL_SCENRIO_3:
        case E_PPL_SCENRIO_4:
        case E_PPL_SCENRIO_5:
        case E_PPL_SCENRIO_10:
        case E_IPC_SCENARIO_SLT:
            strScenario = "scenario_" + std::to_string(nScenario);
            break;
        default:
            strScenario = AX_IPC_SCENARIO_INVALID;
    }

    return strScenario;
}

AX_BOOL CCmdLineParser::GetPPLIndex(AX_S32& nIndex) {
    return GetIntValue(AX_IPC_CMD_KEY_PPL, nIndex);
}

AX_BOOL CCmdLineParser::GetLoadType(AX_S32& nType) {
    return GetIntValue(AX_IPC_CMD_KEY_TYPE, nType);
}

AX_BOOL CCmdLineParser::GetScenario(AX_S32& nScenario) {
    return GetIntValue(AX_IPC_CMD_KEY_SCENARIO, nScenario);
}

AX_BOOL CCmdLineParser::GetLogLevel(AX_S32& nLevel) {
    return GetIntValue(AX_IPC_CMD_KEY_LOG_LEVEL, nLevel);
}

AX_BOOL CCmdLineParser::GetLogTarget(AX_S32& nLogTarget) {
    return GetIntValue(AX_IPC_CMD_KEY_LOG_TARGET, nLogTarget);
}

AX_BOOL CCmdLineParser::isUTEnabled() {
    AX_S32 nIndex = 0;
    GetIntValue(AX_IPC_UT, nIndex);
    return (nIndex == 0) ? AX_FALSE : AX_TRUE;
}

AX_BOOL CCmdLineParser::isDulSnsMode() {
    AX_S32 nType;
    GetIntValue(AX_IPC_CMD_KEY_TYPE, nType);
    return nType % 2 ? AX_FALSE : AX_TRUE;
}
