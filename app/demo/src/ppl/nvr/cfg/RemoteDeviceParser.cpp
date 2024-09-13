// /**************************************************************************************************
//  *
//  * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
//  *
//  * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
//  * may not be copied or distributed in any isomorphic form without the prior
//  * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
//  *
//  **************************************************************************************************/
#include "AppLogApi.h"
#include "RemoteDeviceParser.h"
#include <fstream>

using namespace std;

#define TAG "PARSER"

template <typename T>
static AX_VOID SET_VALUE(picojson::value &argObj, const string &name, T v)
{
    argObj.get<picojson::object>()[name] = picojson::value(v);
}

template <typename T>
static AX_BOOL GET_VALUE(const picojson::value &argObj, const string &name, T &v)
{
    const picojson::value& obj = argObj.get(name);
    if (obj.is<T>()) {
        v = obj.get<T>();
        return AX_TRUE;
    }
    LOG_M_E(TAG, "get remote device faield <%s> failed.", name);
    return AX_FALSE;
}

AX_BOOL CRemoteDeviceParser::InitOnce() {

    // m_strPath = GetExecPath() + "config/remote_device.json";
    return AX_TRUE;
}

std::vector<AX_NVR_DEV_INFO_T> CRemoteDeviceParser::GetRemoteDeviceMap(AX_U32 *nRemoteDeviceCnt, const std::string &strPath) {

    std::vector<AX_NVR_DEV_INFO_T> vecRemoteDevice;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    do {
        m_strPath = std::move(strPath);
        std::ifstream file(m_strPath);
        if (!file.is_open()) {
            LOG_M_E(TAG, "Failed to open json config file: %s", m_strPath.c_str());
            break;
        }

        picojson::value json;
        file >> json;
        string err = picojson::get_last_error();
        if (!err.empty()) {
            LOG_M_E(TAG, "Failed to load json config file: %s", m_strPath.c_str());
            break;
        }

        if (!json.is<picojson::object>()) {
            LOG_M_E(TAG, "Loaded config file is not a well-formatted JSON.");
            break;
        }
        // parse remote device
        else {
            double nCnt = 0;
            if (!GET_VALUE(json, "count", nCnt)) {
                break;
            }

            *nRemoteDeviceCnt = (AX_U32)nCnt;

            const picojson::value& device_list_value = json.get("remote_device");
            if (device_list_value.is<picojson::array>()) {
                const picojson::array& arr_device = device_list_value.get<picojson::array>();
                if (*nRemoteDeviceCnt > arr_device.size()) {
                    LOG_M_W(TAG, "device size is invalid.");
                    *nRemoteDeviceCnt = arr_device.size();
                }

                double dValue = 0.0;
                string strValue = "";
                for (auto device_value : arr_device) {

                    AX_NVR_DEV_INFO_T devInfo;
                    if (!GET_VALUE(device_value, "channel", dValue)) break;
                    devInfo.nChannelId = dValue;

                    if (!GET_VALUE(device_value, "alias", strValue)) break;
                    strcpy((char*)devInfo.szAlias, strValue.c_str());

                    if (!GET_VALUE(device_value, "type", dValue)) break;
                    devInfo.enType = (AX_NVR_CHN_TYPE)dValue;

                    // preview
                    auto valuePreview = device_value.get<picojson::object>()["preview"];
                    if (!GET_VALUE(valuePreview, "stream", dValue)) break;
                    devInfo.enPreviewIndex = (AX_NVR_CHN_IDX_TYPE)dValue;

                    if (!GET_VALUE(valuePreview, "display", dValue)) break;
                    devInfo.bPreviewDisplay = (AX_BOOL)dValue;

                    // preview
                    auto valuePatrol = device_value.get<picojson::object>()["patrol"];
                    if (!GET_VALUE(valuePatrol, "stream", dValue)) break;
                    devInfo.enPatrolIndex = (AX_NVR_CHN_IDX_TYPE)dValue;

                    if (!GET_VALUE(valuePatrol, "display", dValue)) break;
                    devInfo.bPatrolDisplay = (AX_BOOL)dValue;

                    // main
                    auto valueMain = device_value.get<picojson::object>()["main"];
                    if (!GET_VALUE(valueMain, "rtsp", strValue)) break;
                    strcpy((char*)devInfo.stChnMain.szRtspUrl, strValue.c_str());

                    if (!GET_VALUE(valueMain, "record", dValue)) break;
                    devInfo.stChnMain.bRecord = (AX_BOOL)dValue;

                    // sub1
                    auto valueSub1 = device_value.get<picojson::object>()["sub1"];
                    if (!GET_VALUE(valueSub1, "rtsp", strValue)) break;
                    strcpy((char*)devInfo.stChnSub1.szRtspUrl, strValue.c_str());

                    if (!GET_VALUE(valueSub1, "record", dValue)) break;
                    devInfo.stChnSub1.bRecord = (AX_BOOL)dValue;

                    vecRemoteDevice.emplace_back(devInfo);
                }
            }
        }
    } while (0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return vecRemoteDevice;
}

AX_BOOL CRemoteDeviceParser::SetRemoteDeviceMap(std::vector<AX_NVR_DEV_INFO_T>& vecRemoteDevice) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    do {
        picojson::object obj;
        obj["count"] = picojson::value((double)vecRemoteDevice.size());

        picojson::array arr;
        for (auto &info : vecRemoteDevice) {
            picojson::object objDev;
            objDev["channel"] = picojson::value((double)info.nChannelId);
            objDev["alias"] = picojson::value(string((char*)info.szAlias));
            objDev["type"] = picojson::value((double)info.enType);

            objDev["preview"].get<picojson::object>()["stream"] = picojson::value((double)info.enPreviewIndex);
            objDev["preview"].get<picojson::object>()["display"] = picojson::value((double)info.bPreviewDisplay);
            objDev["patrol"].get<picojson::object>()["stream"] = picojson::value((double)info.enPreviewIndex);
            objDev["patrol"].get<picojson::object>()["display"] = picojson::value((double)info.bPatrolDisplay);

            objDev["main"].get<picojson::object>()["rtsp"] = picojson::value(string((char*)info.stChnMain.szRtspUrl));
            objDev["main"].get<picojson::object>()["record"] = picojson::value((double)info.stChnMain.bRecord);

            objDev["sub1"].get<picojson::object>()["rtsp"] = picojson::value(string((char*)info.stChnSub1.szRtspUrl));
            objDev["sub1"].get<picojson::object>()["record"] = picojson::value((double)info.stChnSub1.bRecord);

            arr.push_back(picojson::value(objDev));
        }
        obj["remote_device"] = picojson::value(arr);

        std::ofstream file(m_strPath);
        if (!file.is_open()) {
            LOG_M_E(TAG, "Failed to open json config file: %s", m_strPath.c_str());
            break;
        }
        file << picojson::value(obj).serialize(true);
        string err = picojson::get_last_error();
        if (!err.empty()) {
            LOG_M_E(TAG, "Failed to save json config file: %s", m_strPath.c_str());
            break;
        }

        bRet = AX_TRUE;

    } while (0);

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return bRet;
}

string CRemoteDeviceParser::GetExecPath(AX_VOID) {
    string strPath;
    AX_CHAR szPath[260] = {0};
    ssize_t sz = readlink("/proc/self/exe", szPath, sizeof(szPath));
    if (sz <= 0) {
        strPath = "./";
    } else {
        strPath = szPath;
        strPath = strPath.substr(0, strPath.rfind('/') + 1);
    }

    return strPath;
}
