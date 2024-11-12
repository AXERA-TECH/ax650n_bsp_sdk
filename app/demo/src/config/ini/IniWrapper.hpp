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
#include "ax_base_type.h"
#include "inifile.h"
#include <string>
#include <map>

class CIniWrapper final {
public:
    CIniWrapper(AX_VOID) = default;
    ~CIniWrapper(AX_VOID) = default;

    AX_BOOL Load(const std::string& strIniPath);
    AX_S32 GetIntValue(const std::string& strAppName, const std::string& strKeyName, AX_S32 nDefault);
    AX_BOOL SetIntValue(const std::string& strAppName, const std::string& strKeyName, AX_S32 nValue);
    AX_F64 GetDoubleValue(const std::string& strAppName, const std::string& strKeyName, AX_F64 dDefault);
    std::string GetStringValue(const std::string& strAppName, const std::string& strKeyName, const std::string& strDefault);

    /* get all key=value of specified section */
    AX_VOID GetAllKeys(const std::string& strAppName, std::map<std::string, std::string> &mapKeys);

    AX_VOID GetIntValue(const std::string& strAppName, const std::string& strKeyName, vector<AX_S32> &values);
    AX_VOID GetDoubleValue(const std::string& strAppName, const std::string& strKeyName, vector<AX_F64> &values);

private:
    AX_VOID Str2Array(const std::string &strValues, vector<AX_F64> &szOut);

private:
    inifile::IniFile m_ini;
};
