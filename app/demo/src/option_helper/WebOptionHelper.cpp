/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "WebOptionHelper.h"
#include "AlgoOptionHelper.h"
#include "AudioOptionHelper.h"
#include "CmdLineParser.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "OptionHelper.h"
#include "OsdConfig.h"
#include "PrintHelper.h"
#include "SensorOptionHelper.h"
#include "appweb.h"
#include "http.h"
#include "mpr.h"
#include "picojson.h"

#define WEB_OPTION "WEB_OPTION"
#define JSON2INT(val) picojson::value(double(val))
#define JSON2BOOL(val) picojson::value(bool(val))

using namespace std;

CWebOptionHelper::CWebOptionHelper(AX_VOID) {
}

CWebOptionHelper::~CWebOptionHelper(AX_VOID) {
}

AX_BOOL CWebOptionHelper::InitOnce() {
    m_mapCapabilities["sys"] = 1;
    m_mapCapabilities["cam"] = 1;
    m_mapCapabilities["img"] = 1;
    m_mapCapabilities["ai"] = 1;
    m_mapCapabilities["audio"] = APP_AUDIO_AVAILABLE() ? 1 : 0;
    m_mapCapabilities["video"] = 1;
    m_mapCapabilities["overlay"] = COptionHelper::GetInstance()->IsEnableOSD() ? 1 : 0;
    m_mapCapabilities["storage"] = 0;
    m_mapCapabilities["playback"] = 1;

    /* os08a20*/
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("3840x2160");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("3072x2048");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("3072x1728");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("2624x1944");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("2688x1520");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("2048x1536");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("2304x1296");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("1280x960");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][0].push_back("1280x720");

    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][1].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][1].push_back("1280x720");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][1].push_back("704x576");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][1].push_back("640x480");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][1].push_back("384x288");

    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][2].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][2].push_back("1280x720");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][2].push_back("704x576");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][2].push_back("640x480");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08A20][2].push_back("384x288");

    m_mapSns2FramerateOpt[E_SNS_TYPE_OS08A20] = "[25, 30]";

    /* os08b10*/
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("3840x2160");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("3072x2048");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("3072x1728");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("2624x1944");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("2688x1520");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("2048x1536");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("2304x1296");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("1280x960");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][0].push_back("1280x720");

    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][1].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][1].push_back("1280x720");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][1].push_back("704x576");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][1].push_back("640x480");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][1].push_back("384x288");

    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][2].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][2].push_back("1280x720");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][2].push_back("704x576");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][2].push_back("640x480");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS08B10][2].push_back("384x288");
    m_mapSns2FramerateOpt[E_SNS_TYPE_OS08B10] = "[25]";

    /* SC910gs*/
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("3840x2160");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("3072x2048");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("3072x1728");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("2624x1944");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("2688x1520");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("2048x1536");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("2304x1296");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("1280x960");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][0].push_back("1280x720");

    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][1].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][1].push_back("1280x720");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][1].push_back("704x576");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][1].push_back("640x480");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][1].push_back("384x288");

    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][2].push_back("1920x1080");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][2].push_back("1280x720");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][2].push_back("704x576");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][2].push_back("640x480");
    m_mapSnsType2ResOptions[E_SNS_TYPE_SC910GS][2].push_back("384x288");
    m_mapSns2FramerateOpt[E_SNS_TYPE_SC910GS] = "[25]";

    /* PANO DUAL OS04A10*/
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS04A10_DUAL_PANO][0].push_back("3712x832");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS04A10_DUAL_PANO][0].push_back("2432x768");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS04A10_DUAL_PANO][0].push_back("1280x364");

    m_mapSnsType2ResOptions[E_SNS_TYPE_OS04A10_DUAL_PANO][1].push_back("2432x768");
    m_mapSnsType2ResOptions[E_SNS_TYPE_OS04A10_DUAL_PANO][1].push_back("1280x364");

    m_mapSnsType2ResOptions[E_SNS_TYPE_OS04A10_DUAL_PANO][2].push_back("1280x364");

    m_mapSns2FramerateOpt[E_SNS_TYPE_OS04A10_DUAL_PANO] = "[30]";

    return AX_TRUE;
}

AX_VOID CWebOptionHelper::InitCameraAttr(AX_U8 nSnsID, AX_U8 nSnsType, WEB_CAMERA_ATTR_T& tCameraAttr) {
    m_mapSns2CameraSetting[nSnsID] = tCameraAttr;
    m_mapSnsID2Type[nSnsID] = nSnsType;
}

AX_VOID CWebOptionHelper::InitVideoAttr(AX_U8 nSnsID, AX_U8 nChnID, WEB_VIDEO_ATTR_T& tVideoAttr) {
    m_mapSns2VideoAttr[nSnsID][nChnID] = tVideoAttr;
    memcpy(m_mapSns2VideoAttr[nSnsID][nChnID].stEncodeCfg, tVideoAttr.stEncodeCfg, sizeof(APP_ENC_RC_CONFIG) * APP_ENCODER_TYPE_MAX);
}

AX_VOID CWebOptionHelper::InitIvesAttr(AX_U8 nSnsID, CIVESStage* pIves) {
    AX_U8 u8ThresHoldY{0};
    AX_U8 u8Confidence{0};
    AX_S32 s32ThresHoldY{0};
    AX_S32 s32Confidence{0};

    m_mapSns2AiAttr[nSnsID].tEvents.tOD.bEnable = CAlgoOptionHelper::GetInstance()->IsEnableOD(nSnsID);
    auto pODInstance = pIves->GetODInstance();
    if (nullptr == pODInstance) {
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY = 100;
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence = 80;
    } else {
        pODInstance->GetDefaultThresholdY(nSnsID, u8ThresHoldY, u8Confidence);
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY = u8ThresHoldY;
        m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence = u8Confidence;
    }

    m_mapSns2AiAttr[nSnsID].tEvents.tMD.bEnable = CAlgoOptionHelper::GetInstance()->IsEnableMD(nSnsID);
    auto pMDInstance = pIves->GetMDInstance();
    if (nullptr == pMDInstance) {
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY = 20;
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence = 50;
    } else {
        pMDInstance->GetDefaultThresholdY(nSnsID, u8ThresHoldY, u8Confidence);
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY = u8ThresHoldY;
        m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence = u8Confidence;
    }

    m_mapSns2AiAttr[nSnsID].tEvents.tSCD.bEnable = CAlgoOptionHelper::GetInstance()->IsEnableSCD(nSnsID);
    auto pSCDInstance = pIves->GetSCDInstance();
    if (nullptr == pSCDInstance) {
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY = 60;
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence = 60;
    } else {
        pSCDInstance->GetThreshold(nSnsID, s32ThresHoldY, s32Confidence);
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY = s32ThresHoldY;
        m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence = s32Confidence;
    }
}

AX_BOOL CWebOptionHelper::InitAiAttr(AX_U8 nSnsID) {
    m_mapSns2AiAttr[nSnsID].bEnable = AX_TRUE;
    m_mapSns2AiAttr[nSnsID].tPushStategy.ePushMode = (E_AI_DETECT_PUSH_MODE_TYPE)ALGO_HVCFP_PARAM(nSnsID).stPushStrategy.ePushMode;
    m_mapSns2AiAttr[nSnsID].tPushStategy.nPushCounts = ALGO_HVCFP_PARAM(nSnsID).stPushStrategy.nPushCount;
    m_mapSns2AiAttr[nSnsID].tPushStategy.nPushIntervalMs = ALGO_HVCFP_PARAM(nSnsID).stPushStrategy.nInterval;

    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::GetCapSettingStr(AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }

    AX_U8 nDualSnsMode = (APP_WEB_SHOW_SENSOR_COUNT() == 1) ? 0 : 1;
    AX_U8 nImgDualSnsMode = ((APP_WEB_SHOW_SENSOR_MODE() == E_WEB_SHOW_SENSOR_MODE_PANO_DUAL) || 0 == nDualSnsMode) ? 0 : 1;
    m_mapCapabilities["img"] =(APP_WEB_SHOW_SENSOR_MODE() == E_WEB_SHOW_SENSOR_MODE_PANO_SINGLE) ? 0 : 1;

    AX_S32 nCount = snprintf(pOutBuf, nSize,
                             "{support_dual_sns: %d, img_page_support_dual_sns: %d, support_page_sys: %d, support_page_cam: %d, support_page_img: %d, support_page_ai: "
                             "%d, support_page_audio: %d, "
                             "support_page_video: %d, "
                             "support_page_overlay: %d, support_page_storage: %d, support_page_playback: %d}",
                             nDualSnsMode, nImgDualSnsMode, m_mapCapabilities["sys"], m_mapCapabilities["cam"], m_mapCapabilities["img"],
                             m_mapCapabilities["ai"], m_mapCapabilities["audio"], m_mapCapabilities["video"], m_mapCapabilities["overlay"],
                             m_mapCapabilities["storage"], m_mapCapabilities["playback"]);

    return nCount > 0 ? AX_TRUE : AX_FALSE;
}

WEB_CAMERA_ATTR_T& CWebOptionHelper::GetCamera(AX_U8 nSnsID) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    return m_mapSns2CameraSetting[nSnsID];
}

AX_VOID CWebOptionHelper::SetCamera(AX_U8 nSnsID, WEB_CAMERA_ATTR_T& tCamera) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    m_mapSns2CameraSetting[nSnsID] = tCamera;
}

AX_BOOL CWebOptionHelper::GetCameraStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }
    WEB_CAMERA_ATTR_T attr = m_mapSns2CameraSetting[nSnsID];
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    picojson::object json;
    json["sns_work_mode"] = JSON2INT(attr.nSnsMode);
    json["rotation"] = JSON2INT(attr.nRotation);
    json["framerate"] = JSON2INT(attr.fFramerate);
    json["daynight"] = JSON2INT(attr.nDayNightMode);
    json["nr_mode"] = JSON2INT(attr.nNrMode);
    json["mirror"] = JSON2BOOL(attr.bMirror);
    json["flip"] = JSON2BOOL(attr.bFlip);
    json["capture"] = JSON2BOOL(attr.bCapture);
    json["capture_enable"] = JSON2BOOL(attr.bCaptureEnable);
    json["switch_work_mode_enable"] = JSON2INT(attr.bSnsModeEnable);
    json["switch_PN_mode_enable"] = JSON2INT(attr.bPNModeEnable);
    json["switch_mirror_flip_enable"] = JSON2BOOL(attr.bMirrorFlipEnable);
    json["switch_rotation_enable"] = JSON2BOOL(attr.bRotationEnable);
    std::string strJson = picojson::value(json).serialize().data();
    sprintf(pOutBuf, strJson.data(), strJson.size());
    return strJson.size() ? AX_TRUE : AX_FALSE;
}

AX_BOOL CWebOptionHelper::GetImageStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }
    WEB_CAMERA_ATTR_T attr = m_mapSns2CameraSetting[nSnsID];
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    picojson::object json;
    json["isp_auto_mode"] = JSON2INT(attr.tImageAttr.nAutoMode);
    json["sharpness"] = JSON2INT(attr.tImageAttr.nSharpness);
    json["brightness"] = JSON2INT(attr.tImageAttr.nBrightness);
    json["contrast"] = JSON2INT(attr.tImageAttr.nContrast);
    json["saturation"] = JSON2INT(attr.tImageAttr.nSaturation);
    std::string strJson = picojson::value(json).serialize().data();
    sprintf(pOutBuf, strJson.data(), strJson.size());
    return strJson.size() ? AX_TRUE : AX_FALSE;
}

AX_BOOL CWebOptionHelper::GetLdcStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }
    WEB_CAMERA_ATTR_T attr = m_mapSns2CameraSetting[nSnsID];
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    picojson::object json;
    json["ldc_support"] = JSON2BOOL(attr.bLdcEnable);
    json["ldc_enable"] = JSON2BOOL(attr.tLdcAttr.bLdcEnable);
    json["aspect"] = JSON2BOOL(attr.tLdcAttr.bLdcAspect);
    json["x_ratio"] = JSON2INT(attr.tLdcAttr.nXRatio);
    json["y_ratio"] = JSON2INT(attr.tLdcAttr.nYRatio);
    json["xy_ratio"] = JSON2INT(attr.tLdcAttr.nXYRatio);
    json["distor_ratio"] = JSON2INT(attr.tLdcAttr.nDistorRatio);
    std::string strJson = picojson::value(json).serialize().data();
    sprintf(pOutBuf, strJson.data(), strJson.size());
    return strJson.size() ? AX_TRUE : AX_FALSE;
}

std::vector<WEB_VIDEO_ATTR_T> CWebOptionHelper::GetVideoUniChnVec(AX_U8 nSnsID) {
    std::vector<WEB_VIDEO_ATTR_T> vecUni;
    for (auto item : m_mapSns2VideoAttr[nSnsID]) {
        vecUni.emplace_back(item.second);
    }
    return vecUni;
}

AX_U8 CWebOptionHelper::GetVideoCount(AX_U8 nSnsID) {
    std::map<AX_U8, WEB_VIDEO_ATTR_T>& mapSnsVideoAttr = m_mapSns2VideoAttr[nSnsID];
    return mapSnsVideoAttr.size();
}

WEB_VIDEO_ATTR_T& CWebOptionHelper::GetVideo(AX_U8 nSnsID, AX_U32 nChnID) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    return m_mapSns2VideoAttr[nSnsID][nChnID];
}

AX_VOID CWebOptionHelper::SetVideo(AX_U8 nSnsID, AX_U32 nChnID, WEB_VIDEO_ATTR_T& tVideo) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    m_mapSns2VideoAttr[nSnsID][nChnID] = tVideo;
}

AX_BOOL CWebOptionHelper::GetVideoByUniChn(AX_U8 nSnsID, AX_U32 nUniChn, WEB_VIDEO_ATTR_T& tVideoAttr) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    for (const auto& item : m_mapSns2VideoAttr[nSnsID]) {
        if (item.second.nUniChn == nUniChn) {
            tVideoAttr = item.second;
            return AX_TRUE;
        }
    }

    LOG_M_W(WEB_OPTION, "[%d,%d] GetVideoByUniChn failed, no found!", nSnsID, nUniChn);
    return AX_FALSE;
}

AX_VOID CWebOptionHelper::SetVideoByUniChn(AX_U8 nSnsID, const WEB_VIDEO_ATTR_T& tVideoAttr) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    for (auto& item : m_mapSns2VideoAttr[nSnsID]) {
        if (item.second.nUniChn == tVideoAttr.nUniChn) {
            item.second = tVideoAttr;
            return;
        }
    }

    LOG_M_W(WEB_OPTION, "[%d,%d] SetVideoByUniChn failed, no found!", nSnsID, tVideoAttr.nUniChn);
}

AX_BOOL CWebOptionHelper::GetVideoStr(AX_U8 nSnsID, AX_U32 nPrevChnID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }

    // index of sensor group.
    AX_U8 nChannelID = nPrevChnID;

    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    WEB_VIDEO_ATTR_T* pAttr = &m_mapSns2VideoAttr[nSnsID][nChannelID];

    picojson::object json;
    json["enable_stream"] = picojson::value(bool(pAttr->bEnable));
    json["enable_res_chg"] = picojson::value(bool(1));
    json["encoder_type"] = picojson::value(double(pAttr->nEncoderType));
    json["bit_rate"] = picojson::value(double(pAttr->nBitrate));
    json["resolution"] = picojson::value(std::string(GenResStr(pAttr->nWidth, pAttr->nHeight)));
    const auto resolutionOpts = GetChnResolutionList(nSnsID, nChannelID);
    picojson::array aryResolutionOpts;
    for (const auto& item : resolutionOpts) {
        aryResolutionOpts.push_back(picojson::value(item));
    }
    json["resolution_opt"] = picojson::value(aryResolutionOpts);
    json["rc_type"] = picojson::value(double(pAttr->nRcType));
    RC_INFO_T rcInfoCur;
    pAttr->GetEncRcCfg(rcInfoCur);
    json["min_qp"] = picojson::value(double(rcInfoCur.nMinQp));
    json["max_qp"] = picojson::value(double(rcInfoCur.nMaxQp));
    json["min_iqp"] = picojson::value(double(rcInfoCur.nMinIQp));
    json["max_iqp"] = picojson::value(double(rcInfoCur.nMaxIQp));
    json["min_iprop"] = picojson::value(double(rcInfoCur.nMinIProp));
    json["max_iprop"] = picojson::value(double(rcInfoCur.nMaxIProp));

    picojson::array encRcJson;
    for (int i = 0; i < APP_ENCODER_TYPE_MAX; i++) {
        picojson::object encRc;
        AX_U32 nEncType = CAXTypeConverter::EncoderType2Int(pAttr->stEncodeCfg[i].ePayloadType);
        encRc["encoder_type"] = picojson::value(double(nEncType));
        picojson::array valRc;
        for (AX_U32 j = 0; j < APP_RC_TYPE_MAX; j++) {
            RC_INFO_T rcInfo = pAttr->stEncodeCfg[i].stRCInfo[j];

            picojson::object rc;
            AX_U32 nRcType = CAXTypeConverter::RcMode2Int(rcInfo.eRcType);
            rc["rc_type"] = picojson::value(double(nRcType));
            rc["min_qp"] = picojson::value(double(rcInfo.nMinQp));
            rc["max_qp"] = picojson::value(double(rcInfo.nMaxQp));
            rc["min_iqp"] = picojson::value(double(rcInfo.nMinIQp));
            rc["max_iqp"] = picojson::value(double(rcInfo.nMaxIQp));
            rc["min_iprop"] = picojson::value(double(rcInfo.nMinIProp));
            rc["max_iprop"] = picojson::value(double(rcInfo.nMaxIProp));

            valRc.push_back(picojson::value(rc));
        }
        encRc["rc"] = picojson::value(valRc);
        encRcJson.push_back(picojson::value(encRc));
    }
    json["enc_rc_info"] = picojson::value(encRcJson);

    std::string strJson = picojson::value(json).serialize().data();
    sprintf(pOutBuf, strJson.data(), strJson.size());
    return strJson.size() ? AX_TRUE : AX_FALSE;
}

std::vector<std::string> CWebOptionHelper::GetChnResolutionList(AX_U8 nSnsID, AX_U32 nChnID) {
    std::vector<string> strResList;
    for (auto& res : m_mapSnsType2ResOptions[m_mapSnsID2Type[nSnsID]][nChnID]) {
        strResList.emplace_back(res);
    }

    return strResList;
}

std::string CWebOptionHelper::GetFramerateOptStr(AX_U8 nSnsID) {
    return m_mapSns2FramerateOpt[m_mapSnsID2Type[nSnsID]];
}

AX_BOOL CWebOptionHelper::GetVideoFramerateStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nBufferSize) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxOption[nSnsID]);
    AX_U8 nSize = GetVideoCount(nSnsID);
    AX_U8 aryFramerateOpts[3] = {0};
    for (AX_U8 i = 0; i < nSize; i++) {
        AX_U8 nUniChn = m_mapSns2VideoAttr[nSnsID][i].nUniChn;
        aryFramerateOpts[i] = CPrintHelper::GetInstance()->GetVencFramerate(nUniChn);
    }
    sprintf(pOutBuf, "[%d, %d, %d]", aryFramerateOpts[0], aryFramerateOpts[1], aryFramerateOpts[2]);
    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::StatVencOutBytes(AX_U8 nSnsID, AX_U32 nUniChn, AX_U32 nBytes) {
    if (m_mapSns2ChnStatInfo.find(nSnsID) == m_mapSns2ChnStatInfo.end()) {
        m_mapSns2ChnStatInfo[nSnsID][nUniChn].nStartTick = CElapsedTimer::GetInstance()->GetTickCount();
    }

    if (m_mapSns2ChnStatInfo[nSnsID].find(nUniChn) == m_mapSns2ChnStatInfo[nSnsID].end()) {
        m_mapSns2ChnStatInfo[nSnsID][nUniChn].nStartTick = CElapsedTimer::GetInstance()->GetTickCount();
    }

    if (0 == m_mapSns2ChnStatInfo[nSnsID][nUniChn].nStartTick) {
        m_mapSns2ChnStatInfo[nSnsID][nUniChn].nStartTick = CElapsedTimer::GetInstance()->GetTickCount();
    }

    m_mapSns2ChnStatInfo[nSnsID][nUniChn].nVencOutBytes += nBytes;

    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::GetAssistBitrateStr(AX_U8 nSnsID, AX_U32 nUniChn, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }

    if (m_mapSns2ChnStatInfo.find(nSnsID) == m_mapSns2ChnStatInfo.end()) {
        return AX_FALSE;
    }

    if (m_mapSns2ChnStatInfo[nSnsID].find(nUniChn) == m_mapSns2ChnStatInfo[nSnsID].end()) {
        return AX_FALSE;
    }

    AX_U64 nEndTick = CElapsedTimer::GetInstance()->GetTickCount();
    AX_U64 nGap = nEndTick - m_mapSns2ChnStatInfo[nSnsID][nUniChn].nStartTick;

    AX_F64 fBitrate = m_mapSns2ChnStatInfo[nSnsID][nUniChn].nVencOutBytes / (AX_F64)nGap * 8;

    AX_S32 nCount = snprintf(pOutBuf, nSize, "%.2fkbps", fBitrate);
    if (nCount <= 0) {
        return AX_FALSE;
    }

    m_mapSns2ChnStatInfo[nSnsID][nUniChn].nVencOutBytes = 0;
    m_mapSns2ChnStatInfo[nSnsID][nUniChn].nStartTick = nEndTick;

    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::GetOsdConfig(AX_U8 nSnsID, AX_S32 nGroup, AX_S32 nChn, std::vector<OSD_CFG_T>& vecOsdCfg) {
    std::lock_guard<std::mutex> lck(m_mtxOverlay);

    vecOsdCfg = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGroup][nChn];
    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::SetOsdConfig(AX_U8 nSnsID, AX_S32 nGroup, AX_S32 nChn, std::vector<OSD_CFG_T>& vecOsdCfg) {
    std::lock_guard<std::mutex> lck(m_mtxOverlay);

    m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGroup][nChn] = vecOsdCfg;
    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::IsOsdSwitchOpen(AX_S32 nIvpsGroup) {
    for (auto& m : m_mapSns2OsdConfig) {
        if (m.second.mapGrpChnConfig.find(nIvpsGroup) != m.second.mapGrpChnConfig.end()) {
            return m.second.bEnable;
        }
    }

    return AX_FALSE;
}

WEB_AUDIO_ATTR_T& CWebOptionHelper::GetAudio() {
    std::lock_guard<std::mutex> lck(m_mtxOverlay);
    return m_mapSns2AudioAttr[0];
}

AX_VOID CWebOptionHelper::SetAudio(const WEB_AUDIO_ATTR_T& tAudio) {
    std::lock_guard<std::mutex> lck(m_mtxOverlay);
    m_mapSns2AudioAttr[0] = tAudio;
}

AX_BOOL CWebOptionHelper::GetAiInfoStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mapSns2MtxAi[nSnsID]);

    AX_CHAR szPushStrgy[128] = {0};
    AX_CHAR szDetModelAttr[512] = {0};
    AX_CHAR szEvent[256] = {0};

    if (!GetPushStrgyStr(nSnsID, szPushStrgy, 128) || !GetDetectModelAttrStr(szDetModelAttr, 512) || !GetEventsStr(nSnsID, szEvent, 256)) {
        return AX_FALSE;
    }

    AX_S32 nCount = snprintf(pOutBuf, nSize,
                             "{ai_enable: %s, \
        detect_model: %s, \
        detect_fps: %d, \
        push_strategy: {%s}, \
        detect_only: %s, \
        %s: {%s}, \
        events: {%s}}",
                             ADAPTER_INT2BOOLSTR(m_mapSns2AiAttr[nSnsID].bEnable), GetDetectModelStr().c_str(), 10, szPushStrgy,
                             ADAPTER_INT2BOOLSTR(1), GetDetectModelStr().c_str(), szDetModelAttr, szEvent);

    return nCount > 0 ? AX_TRUE : AX_FALSE;
}
AX_BOOL CWebOptionHelper::GetPushStrgyStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }
    std::string strPushMode = GetPushModeStr(m_mapSns2AiAttr[nSnsID].tPushStategy.ePushMode);
    AX_U32 nPushCounts = m_mapSns2AiAttr[nSnsID].tPushStategy.nPushCounts;
    AX_U32 nPushIntervalMs = m_mapSns2AiAttr[nSnsID].tPushStategy.nPushIntervalMs;
    AX_S32 nCount = snprintf(pOutBuf, nSize, "push_mode: %s, push_interval:%d, push_count: %d, push_same_frame: %s", strPushMode.c_str(),
                             nPushIntervalMs, nPushCounts, ADAPTER_INT2BOOLSTR(1));

    return nCount > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CWebOptionHelper::GetDetectModelAttrStr(AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }

    AX_S32 nCount = 0;
    nCount = snprintf(pOutBuf, nSize,
                      "face_detect: {enable: %s, draw_rect: %s}, \
            body_detect: {enable: %s, draw_rect: %s}, \
            vechicle_detect: {enable: %s, draw_rect: %s}, \
            cycle_detect: {enable: %s, draw_rect: %s}, \
            plate_detect: {enable: %s, draw_rect: %s}, \
            plate_identify: {enable: %s}",
                      ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0),
                      ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0),
                      ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0), ADAPTER_INT2BOOLSTR(0));

    /*
        AX_S32 nCount = 0;
        if (E_AI_DETECT_MODEL_TYPE_FACEHUMAN == m_tAiAttr.eDetectModel) {
            nCount = snprintf(pOutBuf, nSize, "face_detect: {enable: %s, draw_rect: %s}, \
                body_detect: {enable: %s, draw_rect: %s}, \
                face_identify: {enable: %s}",
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHumanFaceSetting.tFace.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHumanFaceSetting.tFace.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHumanFaceSetting.tBody.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHumanFaceSetting.tBody.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHumanFaceSetting.nEnableFI));
        } else if (E_AI_DETECT_MODEL_TYPE_HVCFP == m_tAiAttr.eDetectModel) {
            nCount = snprintf(pOutBuf, nSize, "face_detect: {enable: %s, draw_rect: %s}, \
                body_detect: {enable: %s, draw_rect: %s}, \
                vechicle_detect: {enable: %s, draw_rect: %s}, \
                cycle_detect: {enable: %s, draw_rect: %s}, \
                plate_detect: {enable: %s, draw_rect: %s}, \
                plate_identify: {enable: %s}",
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tFace.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tFace.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tBody.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tBody.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tVechicle.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tVechicle.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tCycle.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tCycle.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tPlate.nEnable),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.tPlate.nDrawRect),
                ADAPTER_INT2BOOLSTR(m_tAiAttr.tHvcfpSetting.nEnablePI));
        }
    */
    return nCount > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CWebOptionHelper::GetEventsStr(AX_U8 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    if (nullptr == pOutBuf || 0 == nSize) {
        return AX_FALSE;
    }

    AX_S32 nCount =
        snprintf(pOutBuf, nSize,
                 "motion_detect: {enable: %s, threshold_y: %d, confidence: %d}, \
        occlusion_detect: {enable: %s, threshold_y: %d, confidence: %d}, \
        scene_change_detect: {enable: %s, threshold_y: %d, confidence: %d}",
                 ADAPTER_BOOL2BOOLSTR(m_mapSns2AiAttr[nSnsID].tEvents.tMD.bEnable), m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY,
                 m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence, ADAPTER_BOOL2BOOLSTR(m_mapSns2AiAttr[nSnsID].tEvents.tOD.bEnable),
                 m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY, m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence,
                 ADAPTER_BOOL2BOOLSTR(m_mapSns2AiAttr[nSnsID].tEvents.tSCD.bEnable), m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY,
                 m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence);

    return nCount > 0 ? AX_TRUE : AX_FALSE;
}

std::string CWebOptionHelper::GetDetectModelStr() {
    return "hvcfp";

    // if (E_AI_DETECT_MODEL_TYPE_FACEHUMAN == m_tAiAttr.eDetectModel) {
    //     return "facehuman";
    // } else if (E_AI_DETECT_MODEL_TYPE_HVCFP == m_tAiAttr.eDetectModel) {
    //     return "hvcfp";
    // } else {
    //     return "";
    // }
}

AX_BOOL CWebOptionHelper::ParseWebRequest(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq, vector<WEB_REQ_OPERATION_T>& vecWebOpr) {
    if (!CheckRequest(eReqType, pJsonReq)) {
        return AX_FALSE;
    }

    vecWebOpr.clear();

    switch (eReqType) {
        case E_REQ_TYPE_TRIGGER: {
            MprJson* pJson = (MprJson*)pJsonReq;
            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            LOG_MM_C(WEB_OPTION, "Web request: sensor %d :\n%s", nSnsID, mprJsonToString(pJson, MPR_JSON_QUOTES));
            cchar* szTgrEnable = mprGetJson(pJson, "trigger_attr.trigger");
            if (nullptr != szTgrEnable) {
                AX_BOOL bTrigger = ((szTgrEnable && strcmp(szTgrEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
                if (bTrigger) {
                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID;
                    tOperation.nChannel = -1;
                    tOperation.eReqType = E_REQ_TYPE_CAMERA;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_TRIGGER);

                    vecWebOpr.emplace_back(tOperation);
                    LOG_MM_I(WEB_OPTION, "[%d] camera trigger", nSnsID);
                }
            }
            break;
        }
        case E_REQ_TYPE_CAMERA: {
            MprJson* pJson = (MprJson*)pJsonReq;
            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            if (m_mapSns2CameraSetting.find(nSnsID) == m_mapSns2CameraSetting.end()) {
                LOG_M_E(WEB_OPTION, "Camera setting for sensor: %d not configured.");
                return AX_FALSE;
            }
            LOG_MM_C(WEB_OPTION, "Web request: sensor %d :\n%s", nSnsID, mprJsonToString(pJson, MPR_JSON_QUOTES));

            AX_U8 nSnsMode = atoi(mprGetJson(pJson, "camera_attr.sns_work_mode"));
            if (nSnsMode != m_mapSns2CameraSetting[nSnsID].nSnsMode) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = nSnsID;
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_CAMERA;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_SNS_MODE);
                tOperation.tSnsMode.nSnsMode = nSnsMode;

                vecWebOpr.emplace_back(tOperation);
                LOG_MM_C(WEB_OPTION, "[%d] sensor work mode, res changed: [%d] => [%d]", nSnsID, m_mapSns2CameraSetting[nSnsID].nSnsMode,
                         nSnsMode);
                m_mapSns2CameraSetting[nSnsID].nSnsMode = nSnsMode;
            }

            AX_U8 nRotation = atoi(mprGetJson(pJson, "camera_attr.rotation"));
            if (nRotation != m_mapSns2CameraSetting[nSnsID].nRotation) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = nSnsID;
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_CAMERA;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_ROTATION);
                tOperation.tRotation.nRotation = nRotation;
                vecWebOpr.emplace_back(tOperation);

                LOG_MM_C(WEB_OPTION, "[%d] camera rotation, res changed: [%d] => [%d]", nSnsID, m_mapSns2CameraSetting[nSnsID].nRotation,
                         nRotation);
                m_mapSns2CameraSetting[nSnsID].nRotation = nRotation;
            }

            AX_U8 nDayNightMode = atoi(mprGetJson(pJson, "camera_attr.daynight"));
            if (nDayNightMode != m_mapSns2CameraSetting[nSnsID].nDayNightMode) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = nSnsID;
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_CAMERA;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_DAYNIGHT);
                tOperation.tDaynight.nDayNightMode = nDayNightMode;
                vecWebOpr.emplace_back(tOperation);

                LOG_MM_C(WEB_OPTION, "[%d] rotation, res changed: [%d] => [%d]", nSnsID, m_mapSns2CameraSetting[nSnsID].nDayNightMode,
                         nDayNightMode);
                m_mapSns2CameraSetting[nSnsID].nDayNightMode = nDayNightMode;
            }

            // AX_U8 nNrMode = atoi(mprGetJson(pJson, "camera_attr.nr_mode"));
            // if (nNrMode != m_mapSns2CameraSetting[nSnsID].nNrMode) {
            //     WEB_REQ_OPERATION_T tOperation;
            //     tOperation.nGroup = nSnsID;
            //     tOperation.nChannel = -1;
            //     tOperation.eReqType = E_REQ_TYPE_CAMERA;
            //     tOperation.SetOperaType( E_WEB_OPERATION_TYPE_DAYNIGHT);
            //     tOperation.tNR.nNRMode = nNrMode;

            //     vecWebOpr.emplace_back(tOperation);

            //     m_mapSns2CameraSetting[nSnsID].nNrMode = nNrMode;
            // }

            cchar* szEnable = mprGetJson(pJson, "camera_attr.capture");
            AX_BOOL bCapEnable = ((szEnable && strcmp(szEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
            if (bCapEnable != m_mapSns2CameraSetting[nSnsID].bCapture) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = nSnsID;
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_CAMERA;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CAPTURE_AUTO);
                tOperation.tCapEnable.bOn = bCapEnable;
                vecWebOpr.emplace_back(tOperation);
                LOG_MM_C(WEB_OPTION, "[%d] capture_enable, changed: [%d] => [%d]", nSnsID, m_mapSns2CameraSetting[nSnsID].bCapture,
                         bCapEnable);
                m_mapSns2CameraSetting[nSnsID].bCapture = bCapEnable;
            }

            AX_U32 nFrameRate = atoi(mprGetJson(pJson, "camera_attr.framerate"));
            if (nFrameRate != m_mapSns2CameraSetting[nSnsID].fFramerate) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = nSnsID;
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_CAMERA;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CAMERA_FPS);
                tOperation.tSnsFpsAttr.fSnsFrameRate = nFrameRate;

                vecWebOpr.emplace_back(tOperation);
                LOG_MM_I(WEB_OPTION, "[%d] camera fps, changed:[%f]=> [%d]", nSnsID, m_mapSns2CameraSetting[nSnsID].fFramerate, nFrameRate);
                m_mapSns2CameraSetting[nSnsID].fFramerate = nFrameRate;
            }

            cchar* szMirrorEnable = mprGetJson(pJson, "camera_attr.mirror");
            cchar* szFlipEnable = mprGetJson(pJson, "camera_attr.flip");
            if (nullptr != szMirrorEnable && nullptr != szFlipEnable) {
                AX_BOOL bMirrorEnable = (strcmp(szMirrorEnable, "true") == 0 ? AX_TRUE : AX_FALSE);
                AX_BOOL bFlipEnable = (strcmp(szFlipEnable, "true") == 0 ? AX_TRUE : AX_FALSE);

                if (m_mapSns2CameraSetting[nSnsID].bMirror != bMirrorEnable || m_mapSns2CameraSetting[nSnsID].bFlip != bFlipEnable) {
                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID;
                    tOperation.nChannel = -1;
                    tOperation.eReqType = E_REQ_TYPE_CAMERA;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_SNS_MIRROR_FLIP);
                    tOperation.tSnsMirrorFlip.bMirror = bMirrorEnable;
                    tOperation.tSnsMirrorFlip.bFlip = bFlipEnable;

                    vecWebOpr.emplace_back(tOperation);
                    LOG_MM_I(WEB_OPTION, "[%d] camera mirror flip, changed:[%d %d]=> [%d %d]", nSnsID,
                             m_mapSns2CameraSetting[nSnsID].bMirror, m_mapSns2CameraSetting[nSnsID].bFlip, bMirrorEnable, bFlipEnable);
                    m_mapSns2CameraSetting[nSnsID].bMirror = bMirrorEnable;
                    m_mapSns2CameraSetting[nSnsID].bFlip = bFlipEnable;
                }
            }

            break;
        }
        case E_REQ_TYPE_IMAGE: {
            MprJson* pJson = (MprJson*)pJsonReq;
            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            if (m_mapSns2CameraSetting.find(nSnsID) == m_mapSns2CameraSetting.end()) {
                LOG_M_E(WEB_OPTION, "Camera setting for sensor: %d not configured.");
                return AX_FALSE;
            }
            LOG_MM_C(WEB_OPTION, "Web request: sensor %d :\n%s", nSnsID, mprJsonToString(pJson, MPR_JSON_QUOTES));

            cchar* szSharpness = mprGetJson(pJson, "image_attr.sharpness_val");
            cchar* szBrightness = mprGetJson(pJson, "image_attr.brightness_val");
            cchar* szContrast = mprGetJson(pJson, "image_attr.contrast_val");
            cchar* szaturation = mprGetJson(pJson, "image_attr.saturation_val");
            if (nullptr != szSharpness && nullptr != szBrightness && nullptr != szContrast && nullptr != szaturation) {
                AX_U8 nAutoMode = atoi(mprGetJson(pJson, "image_attr.isp_auto_mode"));
                AX_U8 nSharpness = atoi(szSharpness);
                AX_U8 nBrightness = atoi(szBrightness);
                AX_U8 nContrast = atoi(szContrast);
                AX_U8 nSaturation = atoi(szaturation);
                if (nAutoMode != m_mapSns2CameraSetting[nSnsID].tImageAttr.nAutoMode ||
                    nSharpness != m_mapSns2CameraSetting[nSnsID].tImageAttr.nSharpness ||
                    nBrightness != m_mapSns2CameraSetting[nSnsID].tImageAttr.nBrightness ||
                    nContrast != m_mapSns2CameraSetting[nSnsID].tImageAttr.nContrast ||
                    nSaturation != m_mapSns2CameraSetting[nSnsID].tImageAttr.nSaturation) {
                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID;
                    tOperation.nChannel = -1;
                    tOperation.eReqType = E_REQ_TYPE_IMAGE;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_IMAGE_ATTR);
                    tOperation.tImageAttr.nSharpness = nSharpness;
                    tOperation.tImageAttr.nBrightness = nBrightness;
                    tOperation.tImageAttr.nContrast = nContrast;
                    tOperation.tImageAttr.nSaturation = nSaturation;
                    tOperation.tImageAttr.nAutoMode = nAutoMode;

                    vecWebOpr.emplace_back(tOperation);
                    LOG_MM_C(WEB_OPTION, "[%d] image_attr, changed: [%d]-[%d %d %d %d] => [%d]-[%d %d %d %d]", nSnsID,
                             m_mapSns2CameraSetting[nSnsID].tImageAttr.nAutoMode, m_mapSns2CameraSetting[nSnsID].tImageAttr.nSharpness,
                             m_mapSns2CameraSetting[nSnsID].tImageAttr.nBrightness, m_mapSns2CameraSetting[nSnsID].tImageAttr.nContrast,
                             m_mapSns2CameraSetting[nSnsID].tImageAttr.nSaturation, nAutoMode, nSharpness, nBrightness, nContrast,
                             nSaturation);

                    m_mapSns2CameraSetting[nSnsID].tImageAttr.nSharpness = nSharpness;
                    m_mapSns2CameraSetting[nSnsID].tImageAttr.nBrightness = nBrightness;
                    m_mapSns2CameraSetting[nSnsID].tImageAttr.nContrast = nContrast;
                    m_mapSns2CameraSetting[nSnsID].tImageAttr.nSaturation = nSaturation;
                    m_mapSns2CameraSetting[nSnsID].tImageAttr.nAutoMode = nAutoMode;
                }
            }
            if (m_mapSns2CameraSetting[nSnsID].bLdcEnable) {
                cchar* szLdcEnable = mprGetJson(pJson, "ldc_attr.ldc_enable");
                cchar* szAspect = mprGetJson(pJson, "ldc_attr.aspect");
                cchar* szXRatio = mprGetJson(pJson, "ldc_attr.x_ratio");
                cchar* szYRatio = mprGetJson(pJson, "ldc_attr.y_ratio");
                cchar* szXYRatio = mprGetJson(pJson, "ldc_attr.xy_ratio");
                cchar* szDistorRatio = mprGetJson(pJson, "ldc_attr.distor_ratio");
                if (nullptr != szLdcEnable && nullptr != szAspect && nullptr != szXRatio && nullptr != szYRatio && nullptr != szXYRatio &&
                    nullptr != szDistorRatio) {
                    AX_BOOL bLdcEnable = (strcmp(szLdcEnable, "true") == 0 ? AX_TRUE : AX_FALSE);
                    AX_BOOL bLdcAspect = (strcmp(szAspect, "true") == 0 ? AX_TRUE : AX_FALSE);
                    AX_S16 nXRatio = atoi(szXRatio);
                    AX_S16 nYRatio = atoi(szYRatio);
                    AX_S16 nXYRatio = atoi(szXYRatio);
                    AX_S16 nDistorRatio = atoi(szDistorRatio);
                    if (bLdcEnable != m_mapSns2CameraSetting[nSnsID].tLdcAttr.bLdcEnable ||
                        bLdcAspect != m_mapSns2CameraSetting[nSnsID].tLdcAttr.bLdcAspect ||
                        nXRatio != m_mapSns2CameraSetting[nSnsID].tLdcAttr.nXRatio ||
                        nYRatio != m_mapSns2CameraSetting[nSnsID].tLdcAttr.nYRatio ||
                        nXYRatio != m_mapSns2CameraSetting[nSnsID].tLdcAttr.nXYRatio ||
                        nDistorRatio != m_mapSns2CameraSetting[nSnsID].tLdcAttr.nDistorRatio) {
                        WEB_REQ_OPERATION_T tOperation;
                        tOperation.nSnsID = nSnsID;
                        tOperation.nChannel = -1;
                        tOperation.eReqType = E_REQ_TYPE_IMAGE;
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_LDC_ATTR);
                        tOperation.tLdcAttr.bLdcEnable = bLdcEnable;
                        tOperation.tLdcAttr.bLdcAspect = bLdcAspect;
                        tOperation.tLdcAttr.nXRatio = nXRatio;
                        tOperation.tLdcAttr.nYRatio = nYRatio;
                        tOperation.tLdcAttr.nXYRatio = nXYRatio;
                        tOperation.tLdcAttr.nDistorRatio = nDistorRatio;

                        vecWebOpr.emplace_back(tOperation);
                        LOG_MM_C(WEB_OPTION, "[%d] ldc_attr, changed: [%d]-[%d %d %d %d %d] => [%d]-[%d %d %d %d %d]", nSnsID,
                                 m_mapSns2CameraSetting[nSnsID].tLdcAttr.bLdcEnable, m_mapSns2CameraSetting[nSnsID].tLdcAttr.bLdcAspect,
                                 m_mapSns2CameraSetting[nSnsID].tLdcAttr.nXRatio, m_mapSns2CameraSetting[nSnsID].tLdcAttr.nYRatio,
                                 m_mapSns2CameraSetting[nSnsID].tLdcAttr.nXYRatio, m_mapSns2CameraSetting[nSnsID].tLdcAttr.nDistorRatio,
                                 bLdcEnable, bLdcAspect, nXRatio, nYRatio, nXYRatio, nDistorRatio);

                        m_mapSns2CameraSetting[nSnsID].tLdcAttr.bLdcEnable = bLdcEnable;
                        m_mapSns2CameraSetting[nSnsID].tLdcAttr.bLdcAspect = bLdcAspect;
                        m_mapSns2CameraSetting[nSnsID].tLdcAttr.nXRatio = nXRatio;
                        m_mapSns2CameraSetting[nSnsID].tLdcAttr.nYRatio = nYRatio;
                        m_mapSns2CameraSetting[nSnsID].tLdcAttr.nXYRatio = nXYRatio;
                        m_mapSns2CameraSetting[nSnsID].tLdcAttr.nDistorRatio = nDistorRatio;
                    }
                }
            }
        }
        case E_REQ_TYPE_AUDIO: {
            MprJson* pJsonRoot = (MprJson*)pJsonReq;
            MprJson* pJsonObj = mprReadJsonObj(pJsonRoot, "capture_attr");
            WEB_AUDIO_ATTR_T tOldAttr = GetAudio();
            AX_U8 nCapture_volume = (AX_U8)tOldAttr.fCapture_volume;
            AX_U8 nPlay_volume = (AX_U8)tOldAttr.fPlay_volume;

            if (pJsonObj) {
                nCapture_volume = (AX_U8)atoi(mprReadJson(pJsonObj, "volume_val"));
            }
            pJsonObj = mprReadJsonObj(pJsonRoot, "play_attr");
            if (pJsonObj) {
                nPlay_volume = (AX_S32)atoi(mprReadJson(pJsonObj, "volume_val"));
            }

            if ((nCapture_volume != (AX_U8)tOldAttr.fCapture_volume) || (nPlay_volume != (AX_U8)tOldAttr.fPlay_volume)) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = 0;
                tOperation.nGroup = 0;
                tOperation.eReqType = E_REQ_TYPE_AUDIO;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AUDIO_ATTR);
                tOperation.tAudioAttr.nCapture_volume = nCapture_volume;
                tOperation.tAudioAttr.nPlay_volume = nPlay_volume;
                vecWebOpr.emplace_back(tOperation);
                m_mapSns2AudioAttr[0].fCapture_volume = nCapture_volume;
                m_mapSns2AudioAttr[0].fPlay_volume = nPlay_volume;
            }
            break;
        }

        case E_REQ_TYPE_VIDEO: {
            MprJson* pJson = (MprJson*)pJsonReq;

            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            AX_BOOL bResolutionChanged = AX_FALSE;

            AX_U8 nSize = GetVideoCount(nSnsID);
            for (AX_U8 i = 0; i < nSize; i++) {
                string strKey = "video" + to_string(i);
                MprJson* jsonObj = mprReadJsonObj(pJson, strKey.c_str());
                if (!jsonObj) {
                    LOG_M_E(WEB_OPTION, "[%d] mprReadJsonObj for %s failed!", nSnsID, strKey.c_str());
                    continue;
                }

                LOG_MM_C(WEB_OPTION, "Web request: sensor %d => %s:\n%s", nSnsID, strKey.c_str(),
                         mprJsonToString(jsonObj, MPR_JSON_QUOTES));

                WEB_VIDEO_ATTR_T tOldAttr = GetVideo(nSnsID, i);
                AX_U8 nEncoderType = (AX_S32)atoi(mprReadJson(jsonObj, "encoder_type"));
                if (nEncoderType != tOldAttr.nEncoderType) {
                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID;
                    tOperation.nGroup = 0;
                    tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][i].nUniChn;
                    tOperation.eReqType = E_REQ_TYPE_VIDEO;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_ENC_TYPE);
                    tOperation.tEncType.nEncoderType = nEncoderType;

                    vecWebOpr.emplace_back(tOperation);
                    LOG_MM_C(WEB_OPTION, "[%d,%d] encode type changed: [%d] => [ %d]", nSnsID, i, tOldAttr.nEncoderType, nEncoderType);
                    m_mapSns2VideoAttr[nSnsID][i].nEncoderType = nEncoderType;
                }

                AX_U32 nWidth = 0;
                AX_U32 nHeight = 0;
                string strResStr = mprReadJson(jsonObj, "resolution");
                if (ParseResStr(strResStr, nWidth, nHeight) && (nWidth != tOldAttr.nWidth || nHeight != tOldAttr.nHeight)) {
                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID;
                    tOperation.nGroup = 0;
                    tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][i].nUniChn; /* unique channel id corresponding to venc id */
                    tOperation.eReqType = E_REQ_TYPE_VIDEO;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_RESOLUTION);
                    tOperation.tResolution.nWidth = nWidth;
                    tOperation.tResolution.nHeight = nHeight;
                    tOperation.tResolution.nEncoderType = nEncoderType;

                    vecWebOpr.emplace_back(tOperation);
                    LOG_MM_C(WEB_OPTION, "[%d,%d] encodeType:%d, resolution changed: [%d, %d] => [%d, %d]", nSnsID, i,
                             tOperation.tResolution.nEncoderType, tOldAttr.nWidth, tOldAttr.nHeight, nWidth, nHeight);

                    m_mapSns2VideoAttr[nSnsID][i].nWidth = nWidth;
                    m_mapSns2VideoAttr[nSnsID][i].nHeight = nHeight;

                    if (!bResolutionChanged) {
                        bResolutionChanged = AX_TRUE;
                    }
                }

                cchar* szBitrate = mprReadJson(jsonObj, "bit_rate");
                if (nullptr != szBitrate) {
                    AX_U32 nBitrate = (AX_S32)atoi(szBitrate);
                    if (nBitrate != tOldAttr.nBitrate) {
                        WEB_REQ_OPERATION_T tOperation;
                        tOperation.nSnsID = nSnsID;
                        tOperation.nGroup = 0;
                        tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][i].nUniChn;
                        tOperation.eReqType = E_REQ_TYPE_VIDEO;
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_BITRATE);
                        tOperation.tBitrate.nBitrate = nBitrate;
                        tOperation.tResolution.nEncoderType = nEncoderType;

                        vecWebOpr.emplace_back(tOperation);
                        LOG_MM_C(WEB_OPTION, "[%d,%d] bitrate changed: [%d] => [%d]", nSnsID, i, tOldAttr.nBitrate, nBitrate);

                        m_mapSns2VideoAttr[nSnsID][i].nBitrate = nBitrate;
                    }
                } else {
                    LOG_M_E(WEB_OPTION, "[%d,%d] Empty bitrate value received.", nSnsID, i);
                }

                cchar* szEnable = mprReadJson(jsonObj, "enable_stream");
                AX_BOOL bEnable = ((szEnable && strcmp(szEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
                if (bEnable != tOldAttr.bEnable) {
                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID;
                    tOperation.nGroup = 0;
                    tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][i].nUniChn;
                    tOperation.eReqType = E_REQ_TYPE_VIDEO;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CHANNEL_SWITCH);
                    tOperation.tChnSwitch.bOn = bEnable;
                    tOperation.tChnSwitch.nEncoderType = nEncoderType;

                    vecWebOpr.emplace_back(tOperation);
                    LOG_MM_C(WEB_OPTION, "[%d,%d] vencChn:%d enable stream changed: [%d] => [%d]", nSnsID, i, tOperation.nChannel,
                             tOldAttr.bEnable, bEnable);

                    m_mapSns2VideoAttr[nSnsID][i].bEnable = bEnable;
                }

                cchar* cMin_qp = mprReadJson(jsonObj, "min_qp");
                cchar* cMax_qp = mprReadJson(jsonObj, "max_qp");
                cchar* cMin_iqp = mprReadJson(jsonObj, "min_iqp");
                cchar* cMax_iqp = mprReadJson(jsonObj, "max_iqp");
                cchar* cMin_iprop = mprReadJson(jsonObj, "min_iprop");
                cchar* cMax_iprop = mprReadJson(jsonObj, "max_iprop");
                cchar* cBitrate = mprReadJson(jsonObj, "bit_rate");

                if (nullptr != cMin_qp && nullptr != cMax_qp && nullptr != cMin_iqp && nullptr != cMax_iqp && nullptr != cMin_iprop &&
                    nullptr != cMax_iprop && nullptr != cBitrate) {
                    AX_U32 nRcType = atoi(mprGetJson(jsonObj, "rc_type"));
                    AX_U32 nMinQp = (AX_U32)atoi(cMin_qp);
                    AX_U32 nMaxQp = (AX_U32)atoi(cMax_qp);
                    AX_U32 nMinIQp = (AX_U32)atoi(cMin_iqp);
                    AX_U32 nMaxIQp = (AX_U32)atoi(cMax_iqp);
                    AX_U32 nMinIProp = (AX_U32)atoi(cMin_iprop);
                    AX_U32 nMaxIProp = (AX_U32)atoi(cMax_iprop);
                    AX_U32 nBitrate = (AX_U32)atoi(cBitrate);
                    RC_INFO_T rOldRcInfo;
                    tOldAttr.GetEncRcCfg(rOldRcInfo);
                    if (nRcType != tOldAttr.nRcType || nMinQp != rOldRcInfo.nMinQp || nMaxQp != rOldRcInfo.nMaxQp ||
                        nMinIQp != rOldRcInfo.nMinIQp || nMaxIQp != rOldRcInfo.nMaxIQp || nMinIProp != rOldRcInfo.nMinIProp ||
                        nMaxIProp != rOldRcInfo.nMaxIProp) {
                        WEB_REQ_OPERATION_T tOperation;
                        tOperation.nSnsID = nSnsID;
                        tOperation.nGroup = 0;
                        tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][i].nUniChn;
                        tOperation.eReqType = E_REQ_TYPE_VIDEO;
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_RC_INFO);
                        tOperation.tRcInfo.eRcType = CAXTypeConverter::FormatRcMode(nEncoderType, nRcType);
                        tOperation.tRcInfo.nMinQp = nMinQp;
                        tOperation.tRcInfo.nMaxQp = nMaxQp;
                        tOperation.tRcInfo.nMinIQp = nMinIQp;
                        tOperation.tRcInfo.nMaxIQp = nMaxIQp;
                        tOperation.tRcInfo.nMaxIProp = nMaxIProp;
                        tOperation.tRcInfo.nMinIProp = nMinIProp;
                        tOperation.tRcInfo.nBitrate = nBitrate;
                        tOperation.tRcInfo.nEncoderType = nEncoderType;

                        vecWebOpr.emplace_back(tOperation);
                        LOG_MM_C(WEB_OPTION, "[%d,%d] rc info changed: [%d- %d %d %d %d %d %d] => [%d- %d %d %d %d %d %d]", nSnsID, i,
                                 tOldAttr.nRcType, rOldRcInfo.nMinQp, rOldRcInfo.nMaxQp, rOldRcInfo.nMinIQp, rOldRcInfo.nMaxIQp,
                                 rOldRcInfo.nMinIProp, rOldRcInfo.nMaxIProp, nRcType, nMinQp, nMaxQp, nMinIQp, nMaxIQp, nMinIProp,
                                 nMaxIProp);

                        RC_INFO_T rRcInfo;
                        rRcInfo.eRcType = tOperation.tRcInfo.eRcType;
                        rRcInfo.nMinQp = nMinQp;
                        rRcInfo.nMaxQp = nMaxQp;
                        rRcInfo.nMinIQp = nMinIQp;
                        rRcInfo.nMaxIQp = nMaxIQp;
                        rRcInfo.nMinIProp = nMinIProp;
                        rRcInfo.nMaxIProp = nMaxIProp;

                        m_mapSns2VideoAttr[nSnsID][i].SetEncRcCfg(rRcInfo);
                        m_mapSns2VideoAttr[nSnsID][i].nRcType = nRcType;
                    }
                }
            }

            break;
        }
        case E_REQ_TYPE_AI: {
            MprJson* pJson = (MprJson*)pJsonReq;

            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            if (m_mapSns2AiAttr.find(nSnsID) == m_mapSns2AiAttr.end()) {
                LOG_M_E(WEB_OPTION, "AI settings for sensor: %d not configured.");
                return AX_FALSE;
            }

            cchar* szEnable = mprGetJson(pJson, "ai_attr.ai_enable");
            AX_BOOL bAiEnable = ((szEnable && strcmp(szEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
            if (bAiEnable != m_mapSns2AiAttr[nSnsID].bEnable) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nSnsID = nSnsID; /* Enable attribute only controls the sensor level's osd's display */
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_AI;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_ENABLE);
                tOperation.tAiEnable.bOn = bAiEnable;

                vecWebOpr.emplace_back(tOperation);
                LOG_MM_C(WEB_OPTION, "[%d] ai_enable, res changed: [%d] => [ %d]", nSnsID, m_mapSns2AiAttr[nSnsID].bEnable, bAiEnable);
                m_mapSns2AiAttr[nSnsID].bEnable = bAiEnable;
            }

            cchar* szPushStategy = mprGetJson(pJson, "ai_attr.push_strategy");
            if (szPushStategy) {
                WEB_REQ_OPERATION_T tOperation;
                tOperation.nGroup = nSnsID; /* Enable attribute only controls the sensor level's osd's display */
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_AI;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_PUSH_MODE);
                std::string strPushMode = mprGetJson(pJson, "ai_attr.push_strategy.push_mode");
                cchar* szPushInterval = mprGetJson(pJson, "ai_attr.push_strategy.push_interval");
                cchar* szPushCount = mprGetJson(pJson, "ai_attr.push_strategy.push_count");

                tOperation.tAiPushStategy.ePushMode = ParseResAiStr(strPushMode);
                tOperation.tAiPushStategy.nPushCounts = atoi(szPushCount);
                tOperation.tAiPushStategy.nPushIntervalMs = atoi(szPushInterval);
                vecWebOpr.emplace_back(tOperation);
                LOG_MM_C(WEB_OPTION, "[%d] push_strategy, res changed: [%d %d %d] => [%d %d %d]", nSnsID,
                         m_mapSns2AiAttr[nSnsID].tPushStategy.ePushMode, m_mapSns2AiAttr[nSnsID].tPushStategy.nPushCounts,
                         m_mapSns2AiAttr[nSnsID].tPushStategy.nPushIntervalMs, tOperation.tAiPushStategy.ePushMode,
                         tOperation.tAiPushStategy.nPushCounts, tOperation.tAiPushStategy.nPushIntervalMs);

                m_mapSns2AiAttr[nSnsID].tPushStategy.ePushMode = tOperation.tAiPushStategy.ePushMode;
                m_mapSns2AiAttr[nSnsID].tPushStategy.nPushCounts = tOperation.tAiPushStategy.nPushCounts;
                m_mapSns2AiAttr[nSnsID].tPushStategy.nPushIntervalMs = tOperation.tAiPushStategy.nPushIntervalMs;
            }

            cchar* szEvents = mprGetJson(pJson, "ai_attr.events");
            if (szEvents) {
                WEB_REQ_OPERATION_T tOperation{};
                tOperation.nSnsID = nSnsID;
                tOperation.nChannel = -1;
                tOperation.eReqType = E_REQ_TYPE_AI;
                tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_EVENT);

                cchar* szMDEnable = mprGetJson(pJson, "ai_attr.events.motion_detect.enable");
                cchar* szMDThreshold = mprGetJson(pJson, "ai_attr.events.motion_detect.threshold_y");
                cchar* szMDConfidence = mprGetJson(pJson, "ai_attr.events.motion_detect.confidence");

                AX_BOOL bMDEnable = ((szMDEnable && strcmp(szMDEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
                tOperation.tEvent.tMD.bEnable = bMDEnable;
                tOperation.tEvent.tMD.nThrsHoldY = atoi(szMDThreshold);
                tOperation.tEvent.tMD.nConfidence = atoi(szMDConfidence);

                cchar* szODEnable = mprGetJson(pJson, "ai_attr.events.occlusion_detect.enable");
                cchar* szODThreshold = mprGetJson(pJson, "ai_attr.events.occlusion_detect.threshold_y");
                cchar* szODConfidence = mprGetJson(pJson, "ai_attr.events.occlusion_detect.confidence");

                AX_BOOL bODEnable = ((szODEnable && strcmp(szODEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
                tOperation.tEvent.tOD.bEnable = bODEnable;
                tOperation.tEvent.tOD.nThrsHoldY = atoi(szODThreshold);
                tOperation.tEvent.tOD.nConfidence = atoi(szODConfidence);

                cchar* szSCDEnable = mprGetJson(pJson, "ai_attr.events.scene_change_detect.enable");
                cchar* szSCDThreshold = mprGetJson(pJson, "ai_attr.events.scene_change_detect.threshold_y");
                cchar* szSCDConfidence = mprGetJson(pJson, "ai_attr.events.scene_change_detect.confidence");

                AX_BOOL bSCDEnable = ((szSCDEnable && strcmp(szSCDEnable, "true") == 0) ? AX_TRUE : AX_FALSE);
                tOperation.tEvent.tSCD.bEnable = bSCDEnable;
                tOperation.tEvent.tSCD.nThrsHoldY = atoi(szSCDThreshold);
                tOperation.tEvent.tSCD.nConfidence = atoi(szSCDConfidence);

                vecWebOpr.emplace_back(tOperation);
                LOG_MM_C(WEB_OPTION,
                         "[%d] ai events, res changed: MD[%d %d %d] => [%d %d %d], OD[%d %d %d] => [%d %d %d], SCD[%d %d %d] => [%d %d %d]",
                         nSnsID, m_mapSns2AiAttr[nSnsID].tEvents.tMD.bEnable, m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY,
                         m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence, tOperation.tEvent.tMD.bEnable, tOperation.tEvent.tMD.nThrsHoldY,
                         tOperation.tEvent.tMD.nConfidence, m_mapSns2AiAttr[nSnsID].tEvents.tOD.bEnable,
                         m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY, m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence,
                         tOperation.tEvent.tOD.bEnable, tOperation.tEvent.tOD.nThrsHoldY, tOperation.tEvent.tOD.nConfidence,
                         m_mapSns2AiAttr[nSnsID].tEvents.tSCD.bEnable, m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY,
                         m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence, tOperation.tEvent.tSCD.bEnable,
                         tOperation.tEvent.tSCD.nThrsHoldY, tOperation.tEvent.tSCD.nConfidence);

                m_mapSns2AiAttr[nSnsID].tEvents.tMD.bEnable = tOperation.tEvent.tMD.bEnable;
                m_mapSns2AiAttr[nSnsID].tEvents.tMD.nThrsHoldY = tOperation.tEvent.tMD.nThrsHoldY;
                m_mapSns2AiAttr[nSnsID].tEvents.tMD.nConfidence = tOperation.tEvent.tMD.nConfidence;

                m_mapSns2AiAttr[nSnsID].tEvents.tOD.bEnable = tOperation.tEvent.tOD.bEnable;
                m_mapSns2AiAttr[nSnsID].tEvents.tOD.nThrsHoldY = tOperation.tEvent.tOD.nThrsHoldY;
                m_mapSns2AiAttr[nSnsID].tEvents.tOD.nConfidence = tOperation.tEvent.tOD.nConfidence;

                m_mapSns2AiAttr[nSnsID].tEvents.tSCD.bEnable = tOperation.tEvent.tSCD.bEnable;
                m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nThrsHoldY = tOperation.tEvent.tSCD.nThrsHoldY;
                m_mapSns2AiAttr[nSnsID].tEvents.tSCD.nConfidence = tOperation.tEvent.tSCD.nConfidence;
            }

            break;
        }
        case E_REQ_TYPE_OSD: {
            MprJson* pJson = (MprJson*)pJsonReq;

            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            if (m_mapSns2OsdConfig.find(nSnsID) == m_mapSns2OsdConfig.end()) {
                LOG_M_E(WEB_OPTION, "OSD settings for sensor: %d not configured.");
                return AX_FALSE;
            }

            for (AX_U8 nVideo = 0; nVideo < MAX_OSD_CHN_COUNT; nVideo++) {
                char* pStrEnd = nullptr;
                char szKey[64] = {0};
                sprintf(szKey, "overlay_attr[%d]", nVideo);
                MprJson* jsonOverlyChn = mprGetJsonObj(pJson, szKey);

                if (NULL != jsonOverlyChn) {
                    AX_U32 nIndex = atoi(mprGetJson(jsonOverlyChn, "video.id"));

                    std::pair<AX_U8, AX_U8> pairIndex = OverlayChnIndex2IvpsGrp(nSnsID, nIndex);
                    AX_U32 nIvpsGrp = pairIndex.first;
                    AX_U32 nIvpsChn = pairIndex.second;

                    std::vector<OSD_CFG_T> vecOsdCfg;
                    if (AX_FALSE == GetOsdConfig(nSnsID, nIvpsGrp, nIvpsChn, vecOsdCfg)) {
                        LOG_MM_E(WEB_OPTION, "nIvpsGrp:%d GetOsdConfig Failed.", nIvpsGrp);
                        return AX_FALSE;
                    }

                    for (AX_U32 i = 0; i < vecOsdCfg.size(); i++) {
                        OSD_CFG_T& tOsdConfig = vecOsdCfg[i];
                        OSD_TYPE_E eType = tOsdConfig.eType;
                        tOsdConfig.bChanged = AX_TRUE;

                        AX_S32 nVideoW = atoi(mprGetJson(jsonOverlyChn, "video.width"));
                        AX_S32 nVideoH = atoi(mprGetJson(jsonOverlyChn, "video.height"));

                        switch (eType) {
                            case OSD_TYPE_TIME: {
                                tOsdConfig.bEnable = (AX_BOOL)ADAPTER_BOOLSTR2INT(mprGetJson(jsonOverlyChn, "time.enable"));
                                // tOsdConfig.tTimeAttr.eFormat = atoi(mprGetJson(jsonOverlyChn, "time.format"));
                                tOsdConfig.tTimeAttr.nColor = strtol(mprGetJson(jsonOverlyChn, "time.color"), &pStrEnd, 16);
                                tOsdConfig.nBoundaryX = atoi(mprGetJson(jsonOverlyChn, "time.rect[0]"));
                                tOsdConfig.nBoundaryY = atoi(mprGetJson(jsonOverlyChn, "time.rect[1]"));
                                tOsdConfig.nBoundaryX = OverlayBoudingX(nVideoW, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                tOsdConfig.nBoundaryY = OverlayBoudingY(nVideoH, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                break;
                            }
                            case OSD_TYPE_PICTURE:
                                tOsdConfig.bEnable = (AX_BOOL)ADAPTER_BOOLSTR2INT(mprGetJson(jsonOverlyChn, "logo.enable"));
                                tOsdConfig.nBoundaryX = atoi(mprGetJson(jsonOverlyChn, "logo.rect[0]"));
                                tOsdConfig.nBoundaryY = atoi(mprGetJson(jsonOverlyChn, "logo.rect[1]"));
                                tOsdConfig.nBoundaryX = OverlayBoudingX(nVideoW, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                tOsdConfig.nBoundaryY = OverlayBoudingY(nVideoH, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);

                                break;
                            case OSD_TYPE_STRING_CHANNEL:
                                tOsdConfig.bEnable = (AX_BOOL)ADAPTER_BOOLSTR2INT(mprGetJson(jsonOverlyChn, "channel.enable"));
                                tOsdConfig.tStrAttr.nColor = strtol(mprGetJson(jsonOverlyChn, "channel.color"), &pStrEnd, 16);
                                strcpy(tOsdConfig.tStrAttr.szStr, mprGetJson(jsonOverlyChn, "channel.text"));
                                tOsdConfig.nBoundaryX = atoi(mprGetJson(jsonOverlyChn, "channel.rect[0]"));
                                tOsdConfig.nBoundaryY = atoi(mprGetJson(jsonOverlyChn, "channel.rect[1]"));
                                tOsdConfig.nBoundaryX = OverlayBoudingX(nVideoW, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                tOsdConfig.nBoundaryY = OverlayBoudingY(nVideoH, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                break;
                            case OSD_TYPE_STRING_LOCATION:
                                tOsdConfig.bEnable = (AX_BOOL)ADAPTER_BOOLSTR2INT(mprGetJson(jsonOverlyChn, "location.enable"));
                                tOsdConfig.tStrAttr.nColor = strtol(mprGetJson(jsonOverlyChn, "location.color"), &pStrEnd, 16);
                                strcpy(tOsdConfig.tStrAttr.szStr, mprGetJson(jsonOverlyChn, "location.text"));
                                tOsdConfig.nBoundaryX = atoi(mprGetJson(jsonOverlyChn, "location.rect[0]"));
                                tOsdConfig.nBoundaryY = atoi(mprGetJson(jsonOverlyChn, "location.rect[1]"));
                                tOsdConfig.nBoundaryX = OverlayBoudingX(nVideoW, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                tOsdConfig.nBoundaryY = OverlayBoudingY(nVideoH, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY,
                                                                        tOsdConfig.eAlign, tOsdConfig.eType);
                                break;
                            case OSD_TYPE_PRIVACY:
                                tOsdConfig.bEnable = (AX_BOOL)ADAPTER_BOOLSTR2INT(mprGetJson(jsonOverlyChn, "privacy.enable"));
                                tOsdConfig.tPrivacyAttr.nColor = strtol(mprGetJson(jsonOverlyChn, "privacy.color"), &pStrEnd, 16);
                                tOsdConfig.tPrivacyAttr.eType = (OSD_PRIVACY_TYPE_E)atoi(mprGetJson(jsonOverlyChn, "privacy.type"));
                                tOsdConfig.tPrivacyAttr.nLineWidth = atoi(mprGetJson(jsonOverlyChn, "privacy.linewidth"));
                                tOsdConfig.tPrivacyAttr.bSolid = (AX_BOOL)ADAPTER_BOOLSTR2INT(mprGetJson(jsonOverlyChn, "privacy.solid"));
                                tOsdConfig.tPrivacyAttr.tPt[0].x = atoi(mprGetJson(jsonOverlyChn, "privacy.points[0][0]"));
                                tOsdConfig.tPrivacyAttr.tPt[0].y = atoi(mprGetJson(jsonOverlyChn, "privacy.points[0][1]"));
                                tOsdConfig.tPrivacyAttr.tPt[1].x = atoi(mprGetJson(jsonOverlyChn, "privacy.points[1][0]"));
                                tOsdConfig.tPrivacyAttr.tPt[1].y = atoi(mprGetJson(jsonOverlyChn, "privacy.points[1][1]"));
                                tOsdConfig.tPrivacyAttr.tPt[2].x = atoi(mprGetJson(jsonOverlyChn, "privacy.points[2][0]"));
                                tOsdConfig.tPrivacyAttr.tPt[2].y = atoi(mprGetJson(jsonOverlyChn, "privacy.points[2][1]"));
                                tOsdConfig.tPrivacyAttr.tPt[3].x = atoi(mprGetJson(jsonOverlyChn, "privacy.points[3][0]"));
                                tOsdConfig.tPrivacyAttr.tPt[3].y = atoi(mprGetJson(jsonOverlyChn, "privacy.points[3][1]"));
                                break;
                            case OSD_TYPE_RECT:
                                break;
                            default:
                                break;
                        }
                    }
                    if (AX_FALSE == SetOsdConfig(nSnsID, nIvpsGrp, nIvpsChn, vecOsdCfg)) {
                        LOG_MM_E(WEB_OPTION, "nIvpsGrp:%d SetOsdConfig Failed.", nIvpsGrp);
                        return AX_FALSE;
                    }

                    WEB_REQ_OPERATION_T tOperation;
                    tOperation.nSnsID = nSnsID; /* Enable attribute only controls the sensor level's osd's display */
                    tOperation.nGroup = nIvpsGrp;
                    tOperation.nChannel = -1;
                    tOperation.eReqType = E_REQ_TYPE_OSD;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_OSD_ATTR);
                    vecWebOpr.emplace_back(tOperation);
                }
            }

            break;
        }
        case E_REQ_TYPE_GET_SYSTEM_INFO: {
            WEB_REQ_OPERATION_T tOperation;
            tOperation.eReqType = E_REQ_TYPE_GET_SYSTEM_INFO;
            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_GET_TITLE);

            vecWebOpr.emplace_back(tOperation);

            break;
        }
        case E_REQ_TYPE_GET_ASSIST_INFO: {
            MprJson* pJson = (MprJson*)pJsonReq;

            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            AX_U8 nChnID = atoi(mprGetJson(pJson, "stream"));

            WEB_REQ_OPERATION_T tOperation;
            tOperation.nSnsID = nSnsID;
            tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][nChnID].nUniChn;
            tOperation.eReqType = E_REQ_TYPE_GET_ASSIST_INFO;
            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_GET_RESOLUTION);

            vecWebOpr.emplace_back(tOperation);

            break;
        }
        case E_REQ_TYPE_CAPTURE: {
            MprJson* pJson = (MprJson*)pJsonReq;

            AX_U8 nSnsID = atoi(mprGetJson(pJson, "src_id"));
            AX_U8 nChnID = atoi(mprGetJson(pJson, "stream"));

            WEB_REQ_OPERATION_T tOperation;
            tOperation.nSnsID = nSnsID;
            tOperation.nGroup = nSnsID;
            tOperation.nChannel = m_mapSns2VideoAttr[nSnsID][nChnID].nUniChn;
            tOperation.eReqType = E_REQ_TYPE_CAPTURE;
            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CAPTURE);
            vecWebOpr.emplace_back(tOperation);

            break;
        }
        case E_REQ_TYPE_SWITCH_3A_SYNCRATIO: {
            MprJson* pJson = (MprJson*)pJsonReq;
            cchar* szEnable = mprGetJson(pJson, "sr3a");
            LOG_M_I(WEB_OPTION, "3A sync ratio szEnable: %s", szEnable);
            AX_BOOL b3ASREnable = ((szEnable && strcmp(szEnable, "true") == 0) ? AX_TRUE : AX_FALSE);

            WEB_REQ_OPERATION_T tOperation;
            tOperation.b3ASyncRationOn = b3ASREnable;
            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_SWITCH_3A_SYNCRATIO);
            vecWebOpr.emplace_back(tOperation);

            break;
        }

        default:
            LOG_MM_E(WEB_OPTION, "Invalid web request: unknown type(%d)", eReqType);
            return AX_FALSE;
    }

    return AX_TRUE;
}

AI_EVENTS_OPTION_T& CWebOptionHelper::GetAiEvent(AX_U8 nSnsID) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxAi[nSnsID]);
    return m_mapSns2AiAttr[nSnsID].tEvents;
}

AX_VOID CWebOptionHelper::SetAiEvent(AX_U8 nSnsID, AI_EVENTS_OPTION_T& tAiEvent) {
    std::lock_guard<std::mutex> lck(m_mapSns2MtxAi[nSnsID]);
    m_mapSns2AiAttr[nSnsID].tEvents = tAiEvent;
}

AX_BOOL CWebOptionHelper::CheckRequest(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq) {
    if (nullptr == pJsonReq) {
        return AX_FALSE;
    }

    MprJson* pJson = (MprJson*)pJsonReq;
    switch (eReqType) {
        case E_REQ_TYPE_CAMERA: {
            // if (nullptr == mprGetJson(pJson, "camera_attr.framerate")
            //     || nullptr == mprGetJson(pJson, "camera_attr.sns_work_mode")
            //     || nullptr == mprGetJson(pJson, "camera_attr.rotation")
            //     || nullptr == mprGetJson(pJson, "camera_attr.daynight")
            //     || nullptr == mprGetJson(pJson, "camera_attr.nr_mode")
            //     || nullptr == mprGetJson(pJson, "camera_attr.framerate")) {
            //     // LOG_M_E(WEB, "Not well-formatted json data received for camera setting request.");
            //     return AX_FALSE;
            // }
            break;
        }
        case E_REQ_TYPE_VIDEO: {
            if (nullptr == mprReadJsonObj(pJson, "res_opt_0")) {
                LOG_M_E(WEB_OPTION, "Not well-formatted json data received for video setting request.");
                return AX_FALSE;
            }
            break;
        }
        case E_REQ_TYPE_AI: {
            break;
        }
        case E_REQ_TYPE_GET_ASSIST_INFO: {
            break;
        }
        default:
            break;
    }

    return AX_TRUE;
}

AX_BOOL CWebOptionHelper::ParseResStr(string& strResolution, AX_U32& nWidth, AX_U32& nHeight) {
    AX_U8 nCount = sscanf(strResolution.c_str(), "%dx%d", &nWidth, &nHeight);
    if (2 == nCount) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

string CWebOptionHelper::GenResStr(AX_U32 nWidth, AX_U32 nHeight) {
    return to_string(nWidth) + "x" + to_string(nHeight);
}

E_AI_DETECT_PUSH_MODE_TYPE CWebOptionHelper::ParseResAiStr(string& strAiPushMode) {
    E_AI_DETECT_PUSH_MODE_TYPE mode;
    if (strAiPushMode == "FAST") {
        mode = E_AI_DETECT_PUSH_MODE_TYPE_FAST;
    } else if (strAiPushMode == "INTERVAL") {
        mode = E_AI_DETECT_PUSH_MODE_TYPE_INTERVAL;
    } else {
        mode = E_AI_DETECT_PUSH_MODE_TYPE_BEST;
    }

    return mode;
}

std::string CWebOptionHelper::GetPushModeStr(AX_S32 mode) {
    std::string strMode;
    switch (mode) {
        case E_AI_DETECT_PUSH_MODE_TYPE_FAST:
            strMode = "FAST";
            break;
        case E_AI_DETECT_PUSH_MODE_TYPE_INTERVAL:
            strMode = "INTERVAL";
            break;
        case E_AI_DETECT_PUSH_MODE_TYPE_BEST:
            strMode = "BEST_FRAME";
            break;
        default:
            strMode = "BEST_FRAME";
    }

    return strMode;
}

AX_BOOL CWebOptionHelper::GetOsdStr(AX_S32 nSnsID, AX_CHAR* pOutBuf, AX_U32 nSize) {
    std::lock_guard<std::mutex> lck(m_mtxOverlay);
    int nCount = 0;
    nCount = snprintf(pOutBuf, nSize, "{overlay_attr:[");
    for (int i = 0; i < MAX_OSD_CHN_COUNT; i++) {
        char szBuf[1024] = {0};
        std::pair<AX_U8, AX_U8> pairOsd = OverlayChnIndex2IvpsGrp(nSnsID, i);
        AX_U32 nGrp = pairOsd.first;
        AX_U32 nChn = pairOsd.second;
        if (m_mapSns2VideoAttr[nSnsID].find(i) == m_mapSns2VideoAttr[nSnsID].end()) {
            break;
        }
        AX_U32 nWidth = m_mapSns2VideoAttr[nSnsID][i].nWidth;
        AX_U32 nHeight = m_mapSns2VideoAttr[nSnsID][i].nHeight;
        AX_U8 nRotation = m_mapSns2CameraSetting[nSnsID].nRotation;
        if (AX_IVPS_ROTATION_90 == nRotation || AX_IVPS_ROTATION_270 == nRotation) {
            ::swap(nWidth, nHeight);
        }

        sprintf(szBuf, "{video:{id:%d,width:%d,height:%d},", i, nWidth, nHeight);
        nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s", szBuf);
        AX_U32 nSize = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn].size();
        OSD_CFG_T tOsdConfig{};
        for (AX_U8 n = 0; n < nSize; n++) {
            if (OSD_TYPE_TIME == m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].eType) {
                AX_U32 nFontSize = COSDStyle::GetInstance()->GetTimeFontSize(nHeight);
                m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].tTimeAttr.nFontSize = nFontSize;

                m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].nBoundaryW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
                m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].nBoundaryH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);
                tOsdConfig = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n];
                break;
            }
        }
        AX_U32 nOffsetX = 0;
        if (AX_FALSE == tOsdConfig.bChanged) {
            nOffsetX = COSDStyle::GetInstance()->GetBoundaryX(nHeight);
        } else {
            nOffsetX =
                CWebOptionHelper::OverlayOffsetX(nWidth, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX, tOsdConfig.eAlign, tOsdConfig.eType);
        }
        sprintf(
            szBuf, "time:{enable:%s,format:%d,color:\"0x%06X\",fontsize:%d,align:%d,rect:[%d,%d,%d,%d]},",
            tOsdConfig.bEnable ? "true" : "false",  // time
            tOsdConfig.tTimeAttr.eFormat, tOsdConfig.tTimeAttr.nColor & 0xFFFFFF, tOsdConfig.tTimeAttr.nFontSize, tOsdConfig.eAlign,
            nOffsetX,
            CWebOptionHelper::OverlayOffsetY(nHeight, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY, tOsdConfig.eAlign, tOsdConfig.eType),
            tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryH);
        nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s", szBuf);

        for (AX_U8 n = 0; n < nSize; n++) {
            if (OSD_TYPE_PICTURE == m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].eType) {
                tOsdConfig = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n];
                break;
            }
        }
        sprintf(
            szBuf, "logo:{enable:%s,align:%d,rect:[%d,%d,%d,%d]},",
            tOsdConfig.bEnable ? "true" : "false",  // logo
            tOsdConfig.eAlign,
            CWebOptionHelper::OverlayOffsetX(nWidth, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX, tOsdConfig.eAlign, tOsdConfig.eType),
            CWebOptionHelper::OverlayOffsetY(nHeight, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY, tOsdConfig.eAlign, tOsdConfig.eType),
            tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryH);
        nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s", szBuf);

        for (AX_U8 n = 0; n < nSize; n++) {
            if (OSD_TYPE_STRING_CHANNEL == m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].eType) {
                tOsdConfig = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n];
                break;
            }
        }
        sprintf(
            szBuf, "channel:{enable:%s,text:\"%s\",color:\"0x%06X\",fontsize:%d,align:%d,rect:[%d,%d,%d,%d]},",
            tOsdConfig.bEnable ? "true" : "false",  // channel
            tOsdConfig.tStrAttr.szStr, tOsdConfig.tStrAttr.nColor & 0xFFFFFF, tOsdConfig.tStrAttr.nFontSize, tOsdConfig.eAlign,
            CWebOptionHelper::OverlayOffsetX(nWidth, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX, tOsdConfig.eAlign, tOsdConfig.eType),
            CWebOptionHelper::OverlayOffsetY(nHeight, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY, tOsdConfig.eAlign, tOsdConfig.eType),
            tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryH);
        nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s", szBuf);

        for (AX_U8 n = 0; n < nSize; n++) {
            if (OSD_TYPE_STRING_LOCATION == m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].eType) {
                tOsdConfig = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n];
                break;
            }
        }
        sprintf(
            szBuf, "location:{enable:%s,text:\"%s\",color:\"0x%06X\",fontsize:%d,align:%d,rect:[%d,%d,%d,%d]},",
            tOsdConfig.bEnable ? "true" : "false",  // location
            tOsdConfig.tStrAttr.szStr, tOsdConfig.tStrAttr.nColor & 0xFFFFFF, tOsdConfig.tStrAttr.nFontSize, tOsdConfig.eAlign,
            CWebOptionHelper::OverlayOffsetX(nWidth, tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryX, tOsdConfig.eAlign, tOsdConfig.eType),
            CWebOptionHelper::OverlayOffsetY(nHeight, tOsdConfig.nBoundaryH, tOsdConfig.nBoundaryY, tOsdConfig.eAlign, tOsdConfig.eType),
            tOsdConfig.nBoundaryW, tOsdConfig.nBoundaryH);
        nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s", szBuf);

        for (AX_U8 n = 0; n < nSize; n++) {
            if (OSD_TYPE_PRIVACY == m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n].eType) {
                tOsdConfig = m_mapSns2OsdConfig[nSnsID].mapGrpChnConfig[nGrp][nChn][n];
                break;
            }
        }
        sprintf(szBuf, "privacy:{enable:%s,type:%d,linewidth:%u,color:\"0x%06X\",solid:%s,points:[[%d,%d],[%d,%d],[%d,%d],[%d,%d]]}}",
                tOsdConfig.bEnable ? "true" : "false",  // privacy
                tOsdConfig.tPrivacyAttr.eType, tOsdConfig.tPrivacyAttr.nLineWidth, tOsdConfig.tPrivacyAttr.nColor & 0xFFFFFF,
                tOsdConfig.tPrivacyAttr.bSolid ? "true" : "false", tOsdConfig.tPrivacyAttr.tPt[0].x, tOsdConfig.tPrivacyAttr.tPt[0].y,
                tOsdConfig.tPrivacyAttr.tPt[1].x, tOsdConfig.tPrivacyAttr.tPt[1].y, tOsdConfig.tPrivacyAttr.tPt[2].x,
                tOsdConfig.tPrivacyAttr.tPt[2].y, tOsdConfig.tPrivacyAttr.tPt[3].x, tOsdConfig.tPrivacyAttr.tPt[3].y);

        if (i < MAX_OSD_CHN_COUNT) {
            nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s,", szBuf);
        } else {
            nCount += snprintf(pOutBuf + nCount, nSize - nCount, "%s", szBuf);
        }
    }

    nCount += snprintf(pOutBuf + nCount, nSize - nCount, "]}");

    printf("json:\n %s", pOutBuf);
    return nCount > 0 ? AX_TRUE : AX_FALSE;
}

AX_S32 CWebOptionHelper::OverlayOffsetX(AX_S32 nWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, OSD_ALIGN_TYPE_E eAlign, OSD_TYPE_E eType) {
    AX_S32 Offset = 0;
    if (OSD_ALIGN_TYPE_LEFT_TOP == eAlign || OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign) {
        if (nWidth < nOsdWidth) {
            Offset = nXMargin;
        } else {
            if (nWidth - nOsdWidth > nXMargin) {
                Offset = nXMargin;
            } else {
                Offset = nWidth - nOsdWidth;
            }
        }
        Offset = ALIGN_UP(Offset, OSD_ALIGN_X_OFFSET);
        if (eType == OSD_TYPE_STRING_LOCATION || eType == OSD_TYPE_STRING_CHANNEL) {
            Offset = (Offset / OSD_ALIGN_X_OFFSET - 1) * OSD_ALIGN_X_OFFSET;
        }

    } else if (OSD_ALIGN_TYPE_RIGHT_TOP == eAlign || OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
        if (nWidth < nOsdWidth) {
            Offset = 0;
        } else {
            if (nWidth - nOsdWidth > nXMargin) {
                Offset = nWidth - (nOsdWidth + nXMargin) - (OSD_ALIGN_X_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_UP(Offset, OSD_ALIGN_X_OFFSET);
            } else {
                Offset = 0;
            }
        }
    }
    if (Offset < 0) {
        Offset = 0;
    }
    return Offset;
}
AX_S32 CWebOptionHelper::OverlayOffsetY(AX_S32 nHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, OSD_ALIGN_TYPE_E eAlign, OSD_TYPE_E eType) {
    AX_S32 Offset = 0;
    if (OSD_ALIGN_TYPE_LEFT_TOP == eAlign || OSD_ALIGN_TYPE_RIGHT_TOP == eAlign) {
        if (nHeight < nOsdHeight) {
            Offset = nYMargin;
        } else {
            if (nHeight - nOsdHeight > nYMargin) {
                Offset = nYMargin;
            } else {
                Offset = nHeight - nOsdHeight;
            }
        }
        Offset = ALIGN_UP(Offset, OSD_ALIGN_Y_OFFSET);
    } else if (OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign || OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
        if (nHeight < nOsdHeight) {
            Offset = 0;
        } else {
            if (nHeight - nOsdHeight > nYMargin) {
                Offset = nHeight - (nOsdHeight + nYMargin) - (OSD_ALIGN_Y_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_UP(Offset, OSD_ALIGN_Y_OFFSET);
            } else {
                Offset = 0;
            }
        }
    }
    if (Offset < 0) {
        Offset = 0;
    }
    return Offset;
}

AX_S32 CWebOptionHelper::OverlayBoudingX(AX_S32 nWidth, AX_S32 nOsdWidth, AX_S32 nBoudingX, OSD_ALIGN_TYPE_E eAlign, OSD_TYPE_E eType) {
    AX_S32 x = 0;
    if (OSD_ALIGN_TYPE_LEFT_TOP == eAlign || OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign) {
        if (nWidth < nOsdWidth) {
            x = nBoudingX;
        } else {
            if (nWidth - nOsdWidth > nBoudingX) {
                x = nBoudingX;
            } else {
                x = nWidth - nOsdWidth;
            }
        }
    } else if (OSD_ALIGN_TYPE_RIGHT_TOP == eAlign || OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
        if (nWidth < nOsdWidth) {
            x = 0;
        } else {
            if (nWidth - nOsdWidth > nBoudingX) {
                x = nWidth - (nOsdWidth + nBoudingX);
            } else {
                x = 0;
            }
        }
    }
    if (x < 0) {
        x = 0;
    }
    return x;
}

AX_S32 CWebOptionHelper::OverlayBoudingY(AX_S32 nHeight, AX_S32 nOsdHeight, AX_S32 nBoudingY, OSD_ALIGN_TYPE_E eAlign, OSD_TYPE_E eType) {
    AX_S32 y = 0;
    if (OSD_ALIGN_TYPE_LEFT_TOP == eAlign || OSD_ALIGN_TYPE_RIGHT_TOP == eAlign) {
        if (nHeight < nOsdHeight) {
            y = nBoudingY;
        } else {
            if (nHeight - nOsdHeight > nBoudingY) {
                y = nBoudingY;
            } else {
                y = nHeight - nOsdHeight;
            }
        }
    } else if (OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign || OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
        if (nHeight < nOsdHeight) {
            y = 0;
        } else {
            if (nHeight - nOsdHeight > nBoudingY) {
                y = nHeight - (nOsdHeight + nBoudingY);
            } else {
                y = 0;
            }
        }
    }
    if (y < 0) {
        y = 0;
    }
    return y;
}

AX_VOID CWebOptionHelper::SetIvpsGrp2VideoIndex(std::map<std::pair<AX_U8, AX_U8>, std::pair<AX_U8, AX_U8>> pairVides2Ivps) {
    m_pairVides2Ivps = pairVides2Ivps;
}

std::pair<AX_U8, AX_U8> CWebOptionHelper::OverlayChnIndex2IvpsGrp(AX_S32 nSnsID, AX_U32 nIndex) {
    pair<AX_U8, AX_U8> pairIvps = make_pair(nSnsID, nIndex);
    return m_pairVides2Ivps[pairIvps];
}

AX_BOOL CWebOptionHelper::SetRes2ResOption(SNS_TYPE_E eSnsType, AX_U32 nChnID, AX_U8 nIndex, AX_U32 nWidth, AX_U32 nHeight) {
    if (eSnsType >= E_SNS_TYPE_MAX) {
        LOG_MM_E(WEB_OPTION, "Invalid eSnsType: %d", eSnsType);
        return AX_FALSE;
    }

    string strResolution = to_string(nWidth) + "x" + to_string(nHeight);

    m_mapSnsType2ResOptions[eSnsType][nChnID][nIndex] = strResolution;

    return AX_TRUE;
}
