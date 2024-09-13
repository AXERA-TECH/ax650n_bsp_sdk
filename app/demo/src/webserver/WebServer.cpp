
/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "WebServer.h"
#include <sys/prctl.h>
#include <map>
#include "AudioOptionHelper.h"
#include "AudioWrapper.hpp"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "IModule.h"
#include "IPPLBuilder.h"
#include "SensorOptionHelper.h"
#include "WebOptionHelper.h"
#include "appweb.h"
#include "arraysize.h"
#include "http.h"
#define WEB "WEB SERVER"
#define JSON2INT(val) picojson::value(double(val))
#define JSON2BOOL(val) picojson::value(bool(val))
#define JSON2STRING(val) picojson::value(std::string(val))

#define RESPONSE_STATUS_OK "200"
#define RESPONSE_STATUS_AUTH_FAIL "401"
#define RESPONSE_STATUS_INVALID_REQ "400"
#define PARAM_KEY_PREVIEW_SNS_SRC "src_id"
#define PARAM_KEY_PREVIEW_CHANNEL "stream"
#define PARAM_KEY_APP_NAME "appName"
#define PARAM_KEY_APP_VERSION "appVersion"
#define PARAM_KEY_SDK_VERSION "sdkVersion"
#define PARAM_KEY_SETTING_CAPABILITY "capInfo"
#define PARAM_KEY_SNS_MODE "snsMode"
#define PARAM_KEY_PANO_SNS_ID "panoSnsId"
using namespace std;

extern string g_SDKVersion;

static MprList* g_pClients = nullptr;
std::mutex g_mtxWSData;
std::mutex g_mtxWebOprProcess;
static std::map<AX_U8, std::map<AX_U8, std::pair<AX_U8, AX_U8>>> m_sMapPrevChn2UniChn; /* {SnsID: {PrevID: (UniChn, CodecType)}} */
static std::map<AX_U8, AX_U8> g_mapSns2CurrPrevChn;
static std::map<string, string> g_mapUserInfo;
static std::map<string, string> g_mapUser2Token;
static std::map<string, AX_U16> g_mapToken2Data;
static CWebServer* s_pWebInstance = CWebServer::GetInstance();

typedef struct {
    HttpConn* conn{nullptr};
    void* packet{nullptr};
} WSMsg_T;

typedef struct {
    AX_U32 nMagic{0x54495841};  // "AXIT" by default
    AX_U32 nDatalen{0};
    AX_U64 nPts{0};
} PTS_HEADER_T;

static void* MprListGetNextItem(MprList* lp, int* next) {
    void* item = nullptr;
    int index = 0;

    if (lp == 0 || lp->length == 0) {
        return nullptr;
    }
    index = *next;
    if (index < lp->length) {
        item = lp->items[index];
        *next = ++index;
        return item;
    }
    return nullptr;
}

/* http event callback */
static void SendHttpData(WSMsg_T* msg) {
    HttpConn* stream = msg->conn;
    CAXRingElement* pData = static_cast<CAXRingElement*>(msg->packet);
    delete msg;
    msg = nullptr;

    if ((mprLookupItem(g_pClients, stream) < 0) || (pData == nullptr)) {
        if (pData && pData->pParent) {
            pData->pParent->Free(pData);
        }
        return;
    }

    /* pData->pBuf and pData->nSize is not stable, so save them to local varible */
    AX_U8* pBuf = pData->pBuf;
    AX_U32 nSize = pData->nSize + pData->nHeadSize;

    do {
        if (stream == nullptr || stream->connError || stream->timeout != 0 || !pBuf || nSize == 0) {
            break;
        }

        ssize nRet = httpSendBlock(stream, WS_MSG_BINARY, (cchar*)pBuf, nSize, HTTP_BLOCK);
        if (nRet >= 0) {
            break;
        }
        switch (nRet) {
            case MPR_ERR_TIMEOUT:
                LOG_MM_E(WEB, "httpSendBlock() return ERR_TIMEOUT.");
                break;
            case MPR_ERR_MEMORY:
                LOG_MM_E(WEB, "httpSendBlock() return ERR_MEMORY.");
                break;
            case MPR_ERR_BAD_STATE:
                LOG_MM_E(WEB, "httpSendBlock() return MPR_ERR_BAD_STATE.");
                break;
            case MPR_ERR_BAD_ARGS:
                LOG_MM_E(WEB, "httpSendBlock() return MPR_ERR_BAD_ARGS.");
                break;
            case MPR_ERR_WONT_FIT:
                LOG_MM_E(WEB, "httpSendBlock() return MPR_ERR_WONT_FIT.");
                break;
            default:
                LOG_MM_E(WEB, "httpSendBlock failed.");
                break;
        }
    } while (false);

    if (pData && pData->pParent) {
        pData->pParent->Free(pData);
    }
}

#define TALK_RECV_PCM_BLOCK_SIZE (1024 * 4)
static void RecieveHttpData(WSMsg_T* msg) {
    HttpConn* stream = msg->conn;
    delete msg;
    msg = nullptr;

    do {
        if (stream == nullptr || stream->connError || stream->timeout != 0) {
            break;
        }

        if ((stream->state > HTTP_STATE_BEGIN && stream->state < HTTP_STATE_COMPLETE)) {
            AX_U8 szBuf[TALK_RECV_PCM_BLOCK_SIZE] = {0};
            ssize nReadSize = httpReadBlock(stream, (char*)szBuf, TALK_RECV_PCM_BLOCK_SIZE, 0, HTTP_NON_BLOCK);
            if (nReadSize > 0) {
                // FIXME: webapp only support PCM
                AX_APP_Audio_Play(APP_AUDIO_WEB_TALK_CHANNEL(), PT_LPCM, szBuf, nReadSize);
            }
        }
    } while (false);
}

static AX_U16 GenKeyData(AX_U8 nSnsID, AX_U8 nChnID) {
    return (AX_U16)(nSnsID | (AX_U16)nChnID << 8);
}

static AX_S32 GetSnsIDFromWS(HttpConn* conn) {
    if (!conn) {
        return -1;
    }

    MprBuf* pWSData = (MprBuf*)httpGetWebSocketData((HttpConn*)conn);
    if (pWSData) {
        std::lock_guard<std::mutex> guard(g_mtxWSData);

        AX_U16 nWSData = (AX_U16)mprGetUint16FromBuf(pWSData);
        mprAdjustBufStart(pWSData, -2);

        return (AX_S32)(nWSData & 0xFF);
    }

    return -1;
}

static AX_S32 GetChnFromWS(HttpConn* conn) {
    if (!conn) {
        return -1;
    }

    MprBuf* pWSData = (MprBuf*)httpGetWebSocketData((HttpConn*)conn);
    if (pWSData) {
        std::lock_guard<std::mutex> guard(g_mtxWSData);

        AX_U16 nWSData = (AX_U16)mprGetUint16FromBuf(pWSData);
        mprAdjustBufStart(pWSData, -2);

        return (AX_S32)(nWSData >> 8);
    }

    return -1;
}

static AX_U8 GetUniChnFromToken(cchar* token, AX_U8 nSnsID) {
    AX_U16 nData = g_mapToken2Data[(string)token];
    return (AX_U8)((0 == nSnsID) ? nData & 0x00FF : nData >> 8);
}

static AX_VOID SaveTokenData(cchar* token, AX_U8 nSnsID, AX_U8 nUniChn) {
    AX_U16 nData = 0;
    std::map<string, AX_U16>::iterator itFind = g_mapToken2Data.find(token);
    if (itFind != g_mapToken2Data.end()) {
        nData = g_mapToken2Data[(string)token];
    }

    AX_U8 nSns0Chn = nData & 0x00FF;
    AX_U8 nSns1Chn = nData >> 8;

    if (0 == nSnsID) {
        nSns0Chn = nUniChn;
    } else {
        nSns1Chn = nUniChn;
    }

    g_mapToken2Data[(string)token] = (AX_U16)(nSns0Chn | (AX_U16)nSns1Chn << 8);
}

static AX_S32 GetUniChn(AX_U8 nSnsID, AX_U8 nPrevIndex) {
    if (m_sMapPrevChn2UniChn.find(nSnsID) == m_sMapPrevChn2UniChn.end()) {
        return -1;
    }

    return std::get<0>(m_sMapPrevChn2UniChn[nSnsID][nPrevIndex]);
}

static bool CheckUser(HttpConn* conn, cchar* user, cchar* pwd) {
    std::map<string, string>::iterator itFind = g_mapUserInfo.find(user);
    if (itFind != g_mapUserInfo.end()) {
        return strcmp(itFind->second.c_str(), pwd) == 0;
    }

    return false;
}

static cchar* GenToken(string user, string pwd) {
    uint64 nTickcount = mprGetHiResTicks();
    string strTokenKey = sfmt("%s_%lld", user.c_str(), nTickcount);
    g_mapUser2Token[strTokenKey] = mprGetSHABase64(sfmt("token:%s-%s-%lld", user.c_str(), pwd.c_str(), nTickcount));

    return g_mapUser2Token[strTokenKey].c_str();
}

static MprJson* ConstructBaseResponse(cchar* pszStatus, cchar* pszToken) {
    MprJson* pResponseBody = mprCreateJson(MPR_JSON_OBJ);
    mprWriteJson(pResponseBody, "data", "", MPR_JSON_OBJ);
    if (pszToken) {
        mprWriteJson(mprGetJsonObj(pResponseBody, "data"), "token", pszToken, MPR_JSON_STRING);
    }
    mprWriteJson(pResponseBody, "meta", "", MPR_JSON_OBJ);
    mprWriteJson(mprGetJsonObj(pResponseBody, "meta"), "status", pszStatus, MPR_JSON_NUMBER);

    return pResponseBody;
}

static cchar* GetTokenFromConn(HttpConn* conn, AX_BOOL bFromHeader) {
    if (!conn) {
        return nullptr;
    }

    cchar* szToken = nullptr;
    if (bFromHeader) {
        szToken = (httpGetHeader(conn, "Authorization"));
    } else {
        szToken = (httpGetParam(conn, "token", nullptr));
    }

    char* p = nullptr;
    while ((p = schr(szToken, ' ')) != 0) {
        *p = '+';
    }

    return szToken;
}

static AX_BOOL IsAuthorized(HttpConn* conn, AX_BOOL bGetTokenFromHeader) {
    cchar* szToken = GetTokenFromConn(conn, bGetTokenFromHeader);
    if (0 == szToken || strlen(szToken) == 0) {
        return AX_FALSE;
    }

    for (const auto& kv : g_mapUser2Token) {
        if (strcmp(kv.second.c_str(), szToken) == 0) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

static AX_VOID ResponseUnauthorized(HttpConn* conn) {
    HttpQueue* q = conn->writeq;
    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_AUTH_FAIL, 0);

    httpSetContentType(conn, "application/json");
    httpWrite(q, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);
}

static AX_VOID ResponseError(HttpConn* conn, const AX_CHAR* szErrorCode) {
    HttpQueue* q = conn->writeq;
    MprJson* pResponseBody = ConstructBaseResponse(szErrorCode, 0);

    httpSetContentType(conn, "application/json");
    httpWrite(q, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);
}

static AX_VOID WebNotifier(HttpConn* conn, AX_S32 event, AX_S32 arg) {
    if ((event == HTTP_EVENT_APP_CLOSE) || (event == HTTP_EVENT_ERROR) || (event == HTTP_EVENT_DESTROY)) {
        AX_S32 nIndex = mprRemoveItem(g_pClients, conn);
        if (nIndex >= 0) {
            LOG_MM_D(WEB, "remove connection %p, index=%d", conn, nIndex);
            CWebServer::GetInstance()->UpdateConnStatus();
        }
    }
}

static void LoginAction(HttpConn* conn) {
    cchar* strUser = httpGetParam(conn, "username", "unspecified");
    cchar* strPwd = httpGetParam(conn, "password", "unspecified");

    cchar* szToken = nullptr;
    string strStatus;
    bool bAuthRet = CheckUser(conn, strUser, strPwd);
    if (!bAuthRet) {
        strStatus = RESPONSE_STATUS_AUTH_FAIL;
    } else {
        strStatus = RESPONSE_STATUS_OK;
        szToken = GenToken(strUser, strPwd);
    }

    MprJson* pResponseBody = ConstructBaseResponse(strStatus.c_str(), szToken);

    AX_BOOL bDualSnsMode = (APP_WEB_SHOW_SENSOR_COUNT() == 1) ? AX_FALSE : AX_TRUE;
    mprWriteJson(mprGetJsonObj(pResponseBody, "data"), PARAM_KEY_SNS_MODE, (bDualSnsMode ? "1" : "0"), MPR_JSON_STRING);

    string strPanoSnsId = (APP_WEB_PANO_SENSOR_ID() == -1 ) ? "-1" : to_string(APP_WEB_PANO_SENSOR_ID());
    mprWriteJson(mprGetJsonObj(pResponseBody, "data"), PARAM_KEY_PANO_SNS_ID, strPanoSnsId.c_str(), MPR_JSON_STRING);

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void CapabilityAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);

    if (strcmp(conn->rx->method, "GET") == 0) {
        AX_CHAR szStr[1024] = {0};
        if (CWebOptionHelper::GetInstance()->GetCapSettingStr(szStr, 1024)) {
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), PARAM_KEY_SETTING_CAPABILITY, mprParseJson(szStr));
        }

        httpSetContentType(conn, "application/json");
        httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

        httpSetStatus(conn, 200);
        httpFinalize(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void PreviewInfoAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    AX_BOOL bTrigger = CWebOptionHelper::GetInstance()->GetCamera(0).bTriggerEnable;
    AX_BOOL bCapture = CWebOptionHelper::GetInstance()->GetCamera(0).bCaptureEnable;
    AX_BOOL bFlash = CWebOptionHelper::GetInstance()->GetCamera(0).bFlashEnable;
    string szTrigger = (bTrigger ? "true" : "false");
    string szCapture = (bCapture ? "true" : "false");
    string szFlash = (bFlash ? "true" : "false");

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    if (strcmp(conn->rx->method, "GET") == 0) {
        AX_U8 nSnsCnt = APP_WEB_SHOW_SENSOR_COUNT();
        AX_CHAR arrStreamStr[2][16] = {0};
        for (AX_U8 i = 0; i < 2; i++) {
            if (i < nSnsCnt) {
                AX_U8 nPrevChnCount = m_sMapPrevChn2UniChn[i].size();
                if (1 == nPrevChnCount) {
                    sprintf(arrStreamStr[i], "[%d]", 0);
                } else if (2 == nPrevChnCount) {
                    sprintf(arrStreamStr[i], "[%d, %d]", 0, 1);
                } else if (3 == nPrevChnCount) {
                    sprintf(arrStreamStr[i], "[%d, %d, %d]", 0, 1, 2);
                }
            } else {
                sprintf(arrStreamStr[i], "[0, 1, 2]");
            }
        }

        AX_CHAR arrCodecType[2][16] = {0};
        for (AX_U8 i = 0; i < 2; i++) {
            if (i < nSnsCnt) {
                AX_U8 nPrevChnCount = m_sMapPrevChn2UniChn[i].size();
                if (1 == nPrevChnCount) {
                    sprintf(arrCodecType[i], "[%d]", std::get<1>(m_sMapPrevChn2UniChn[i][0]));
                } else if (2 == nPrevChnCount) {
                    sprintf(arrCodecType[i], "[%d, %d]", std::get<1>(m_sMapPrevChn2UniChn[i][0]), std::get<1>(m_sMapPrevChn2UniChn[i][1]));
                } else if (3 == nPrevChnCount) {
                    sprintf(arrCodecType[i], "[%d, %d, %d]", std::get<1>(m_sMapPrevChn2UniChn[i][0]),
                            std::get<1>(m_sMapPrevChn2UniChn[i][1]), std::get<1>(m_sMapPrevChn2UniChn[i][2]));
                }
            } else {
                sprintf(arrCodecType[i], "[0, 0, 1]");
            }
        }

        static constexpr AX_U8 nFramerateStringBUfferSize = 32;
        AX_CHAR arrVideoFPS[2][nFramerateStringBUfferSize] = {0};
        for (AX_U8 i = 0; i < 2; i++) {
            if (i < nSnsCnt) {
                CWebOptionHelper::GetInstance()->GetVideoFramerateStr(i, arrVideoFPS[i], nFramerateStringBUfferSize);
            } else {
                sprintf(arrVideoFPS[i], "[15, 15, 15]");
            }
        }

        AX_APP_AUDIO_ATTR_T stAttr;
        AX_APP_Audio_GetAttr(&stAttr);
        const AX_CHAR* szAudioCapture = (stAttr.stCapAttr.bEnable) ? "true" : "false";
        // TODO: aenc channel index
        AX_CHAR szData[512] = {0};
        string aencType = "aac";
        AX_APP_AUDIO_ENCODER_ATTR_T stEncodeAttr;
        memset(&stEncodeAttr, 0, sizeof(AX_APP_AUDIO_ENCODER_ATTR_T));
        if (stAttr.stCapAttr.bEnable) {
            AX_APP_AUDIO_CHAN_E eChannel = APP_AUDIO_WEB_STREAM_CHANNEL();
            AX_APP_Audio_GetEncoderAttr(eChannel, &stEncodeAttr);
            switch (stEncodeAttr.eType) {
                case PT_G711A:
                    aencType = "g711a";
                    break;
                case PT_G711U:
                    aencType = "g711u";
                    break;
                case PT_LPCM:
                    aencType = "lpcm";
                    break;
                default:
                    aencType = "aac";
                    break;
            }
        }

        sprintf(szData,
                "{%s: %d, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %s, %s: %d, "
                "%s: %d, %s :%s, %s :%s}",
                "sns_num", nSnsCnt, "sns0_codec", arrCodecType[0], /* 0: VENC; 1: MJPEG */
                "sns1_codec", arrCodecType[1],                     /* 0: VENC; 1: MJPEG */
                "sns0_video_fps", arrVideoFPS[0], "sns1_video_fps", arrVideoFPS[1], "stream0_list", arrStreamStr[0], "stream1_list",
                arrStreamStr[1], "ai_enable", "false", "searchimg", "false", "detect_mode", "hvcfp", "osd_enable", "true", "capture_enable",
                szCapture.c_str(), "aenc_enable", szAudioCapture, "aenc_type", &aencType[0], "aenc_sample_rate", stEncodeAttr.eSampleRate,
                "aenc_bit_width", stEncodeAttr.eBitWidth, "trigger_enable", szTrigger.c_str(), "flash_enable", szFlash.c_str());

        LOG_MM_I(WEB, "szData:%s", szData);

        mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "info", mprParseJson(szData));

        httpSetContentType(conn, "application/json");
        httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

        LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

        httpSetStatus(conn, 200);
        httpFinalize(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
    }

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void SwitchChnAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }

    AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
    AX_U8 nPrevIndex = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_CHANNEL, "0"));

    LOG_MM_I(WEB, "Sensor %d switching to channel %d.", nSnsID, nPrevIndex);

    g_mapSns2CurrPrevChn[nSnsID] = nPrevIndex;
    AX_U8 nUniChn = GetUniChn(nSnsID, nPrevIndex);
    if (-1 == nUniChn) {
        LOG_MM_W(WEB, "Can not find channel id for this request(Switch Channel <sns:%d, chn:%d>)", nSnsID, nPrevIndex);
        ResponseError(conn, RESPONSE_STATUS_INVALID_REQ);
        return;
    }

    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        LOG_MM_I(WEB, "Sensor %d save unique chn %d to token %p", nSnsID, nUniChn, szToken);
        SaveTokenData(szToken, nSnsID, nUniChn);
    }

    MprJson* pResponse = ConstructBaseResponse(RESPONSE_STATUS_OK, szToken);

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponse, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void WSPreviewAction(HttpConn* conn) {
    LOG_MM_D(WEB, "WSPreview setup conn=%p.", conn);

    if (!IsAuthorized(conn, AX_FALSE)) {
        LOG_MM_E(WEB, "Unauthorized, try to login again.");

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U8 nSrcID = 0;
    if (strstr(conn->rx->uri, "/preview_1") != nullptr) {
        nSrcID = 1;
    }

    LOG_MM(WEB, "Websocket preview request src id: %d", nSrcID);

    cchar* szToken = GetTokenFromConn(conn, AX_FALSE);
    if (nullptr != szToken && strlen(szToken) > 0) {
        AX_U8 nSnsID = nSrcID;
        AX_U8 nUniChnID = GetUniChnFromToken(szToken, nSnsID);

        MprBuf* buf = mprCreateBuf(32, 0);
        mprAddRoot(buf);
        mprPutUint16ToBuf(buf, GenKeyData(nSnsID, nUniChnID));
        mprRemoveRoot(buf);

        httpSetWebSocketData(conn, buf);
        LOG_MM(WEB, "[Sns:%d][UniChn:%d] preview connected: %p", nSnsID, nUniChnID, conn);
    }

    AX_S32 nIndex = mprAddItem(g_pClients, conn);
    LOG_MM_D(WEB, "connected %p, index=%d", conn, nIndex);
    httpSetConnNotifier(conn, WebNotifier);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void SaveWSConnection(HttpConn* conn, AX_U8 nSnsID, AX_U8 nUniChnID) {
    MprBuf* buf = mprCreateBuf(32, 0);
    AX_U16 nChnID = (nSnsID | ((nUniChnID & 0x00FF) << 8));
    mprAddRoot(buf);
    mprPutUint16ToBuf(buf, nChnID);
    mprRemoveRoot(buf);
    httpSetWebSocketData(conn, buf);

    AX_S32 nIndex = mprAddItem(g_pClients, conn);
    LOG_MM_D(WEB, "connected %p, index=%d", conn, nIndex);
    httpSetConnNotifier(conn, WebNotifier);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void WSCaptureAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        LOG_MM_E(WEB, "Unauthorized, try to login again.");

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U8 nSrcID = 0; /* Dual sensor's capture shares one connection */
    LOG_MM_I(WEB, "[%d] Capture stream %d setup conn=%p.", nSrcID, s_pWebInstance->GetCaptureChannel(), conn);
    SaveWSConnection(conn, nSrcID, s_pWebInstance->GetCaptureChannel());

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void WSSnapshotAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        LOG_MM_E(WEB, "Unauthorized, try to login again.");

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U8 nSrcID = 0; /* Dual sensor's capture shares one connection */
    AX_U8 nChnnelID = s_pWebInstance->GetSnapshotChannel();

    LOG_MM_I(WEB, "[%d] Snapshot picture %d setup.", nSrcID, nChnnelID);
    SaveWSConnection(conn, nSrcID, nChnnelID);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void TriggerAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_S32 nHttpStatusCode = 200;

    static AX_BOOL bActionProcessing = AX_FALSE; /* Avoiding multiple web operations */

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        if (strcmp(conn->rx->method, "GET") == 0) {
        } else if (!bActionProcessing) {
            bActionProcessing = AX_TRUE;

            MprJson* jsonTrigger = httpGetParams(conn);

            LOG_MM_I(WEB, "Web request: %s", mprJsonToString(jsonTrigger, MPR_JSON_QUOTES));

            s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_TRIGGER, jsonTrigger);

            bActionProcessing = AX_FALSE;
        } else if (bActionProcessing) {
            // Response status: Not Acceptable
            nHttpStatusCode = 406;
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void SnapshotAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
    AX_U8 nPrevIndex = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_CHANNEL, "0"));
    LOG_MM_I(WEB, "Request one snapshot, nSnsID:%d.", nSnsID);
    using AXSnapshotDataCallback = std::function<AX_VOID(AX_VOID * data, AX_U32 size)>;
    static AX_BOOL bActionProcessing = AX_FALSE;
    AX_S32 nHttpStatusCode = 200;
    if (!bActionProcessing) {
        bActionProcessing = AX_TRUE;
        MprJson* jsonSnapshot = httpGetParams(conn);
        LOG_MM_I(WEB, "Web request: %s", mprJsonToString(jsonSnapshot, MPR_JSON_QUOTES));
        std::pair<AX_U8, AXSnapshotDataCallback> tSnapshotData = std::make_pair(
            nPrevIndex, std::bind(&CWebServer::SendSnapshotData, s_pWebInstance, std::placeholders::_1, std::placeholders::_2));
        s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_CAPTURE, jsonSnapshot, (AX_VOID**)&tSnapshotData);
        bActionProcessing = AX_FALSE;
    } else if (bActionProcessing) {
        // Response status: Not Acceptable
        nHttpStatusCode = 406;
        LOG_MM_W(WEB, "Snapshot already in process, please wait.");
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void WSAudioAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        LOG_M_E(WEB, "Unauthorized, try to login again.");

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U8 nSnsID = 0;
    AX_U8 nUniChnID = s_pWebInstance->GetAencChannel();
    SaveWSConnection(conn, nSnsID, nUniChnID);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void WSTalkAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        LOG_M_E(WEB, "Unauthorized, try to login again.");

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U8 nUniChnID = s_pWebInstance->GetTalkChannel();
    LOG_M_I(WEB, "Websocket talk channel id: %d", nUniChnID);
    SaveWSConnection(conn, 0, nUniChnID);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void WSEventsAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        LOG_MM_E(WEB, "Unauthorized, try to login again.");

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_U16 nChnID = (0x0 /*sensor id*/ | ((WS_EVENTS_CHANNEL & 0x00FF) << 8));
    LOG_MM_D(WEB, "nChnID:%d", nChnID);
    SaveWSConnection(conn, 0, WS_EVENTS_CHANNEL);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void AssistInfoAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);

    if (strcmp(conn->rx->method, "GET") == 0) {
        std::lock_guard<std::mutex> guard(g_mtxWebOprProcess);

        AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
        AX_U8 nPrevIndex = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_CHANNEL, "0"));

        MprJson* jsonRequest = httpGetParams(conn);
        AX_CHAR szResolution[16] = {0};

        if (s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_GET_ASSIST_INFO, jsonRequest, (AX_VOID**)&szResolution[0])) {
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "assist_res", mprParseJson(szResolution));
        } else {
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "assist_res", mprParseJson("--"));
        }

        AX_CHAR szBitrate[16] = {0};
        if (CWebOptionHelper::GetInstance()->GetAssistBitrateStr(nSnsID, GetUniChn(nSnsID, nPrevIndex), szBitrate, 16)) {
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "assist_bitrate", mprParseJson(szBitrate));
        } else {
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "assist_bitrate", mprParseJson("--"));
            // LOG_MM_E(WEB, "[%d][%d] Get assist bitrate info failed.", nSnsID, nPrevIndex);
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void SystemAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    LOG_MM_D(WEB, "Request system info.");

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);

    if (strcmp(conn->rx->method, "GET") == 0) {
        MprJson* jsonRequest = httpGetParams(conn);
        AX_CHAR szTitle[64] = {0};
        AX_CHAR szOutTitle[64] = {0};
        if (s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_GET_SYSTEM_INFO, jsonRequest, (AX_VOID**)&szOutTitle[0])) {
            sprintf(szTitle, "%s", szOutTitle);
        } else {
            LOG_MM_W(WEB, "Failed to retrieve web title specification.");
            sprintf(szTitle, "%s", "FRTDemo");
        }

        string strDispVer(APP_BUILD_VERSION);
        mprWriteJson(mprGetJsonObj(pResponseBody, "data"), PARAM_KEY_APP_NAME, szTitle, MPR_JSON_STRING);
        mprWriteJson(mprGetJsonObj(pResponseBody, "data"), PARAM_KEY_APP_VERSION, strDispVer.c_str(), MPR_JSON_STRING);
        mprWriteJson(mprGetJsonObj(pResponseBody, "data"), PARAM_KEY_SDK_VERSION, g_SDKVersion.c_str(), MPR_JSON_STRING);

        httpSetContentType(conn, "application/json");
        httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

        httpSetStatus(conn, 200);
        httpFinalize(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static AX_VOID CameraAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_S32 nHttpStatusCode = 200;

    static AX_BOOL bActionProcessing = AX_FALSE; /* Avoiding multiple web operations */

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        if (strcmp(conn->rx->method, "GET") == 0) {
            AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));

            AX_CHAR szStr[1024] = {0};
            if (CWebOptionHelper::GetInstance()->GetCameraStr(nSnsID, szStr, 1024)) {
                mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "camera_attr", mprParseJson(szStr));
            }

            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "framerate_opts",
                            mprParseJson(CWebOptionHelper::GetInstance()->GetFramerateOptStr(nSnsID).c_str()));

            LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));
        } else if (!bActionProcessing) {
            bActionProcessing = AX_TRUE;

            MprJson* jsonCamera = httpGetParams(conn);
            LOG_MM_I(WEB, "Web request: %s", mprJsonToString(jsonCamera, MPR_JSON_QUOTES));

            s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_CAMERA, jsonCamera);

            bActionProcessing = AX_FALSE;
        } else if (bActionProcessing) {
            // Response status: Not Acceptable
            nHttpStatusCode = 406;
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static AX_VOID ImageAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    AX_S32 nHttpStatusCode = 200;

    static AX_BOOL bActionProcessing = AX_FALSE; /* Avoiding multiple web operations */

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        if (strcmp(conn->rx->method, "GET") == 0) {
            AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));

            AX_CHAR szStr[1024] = {0};
            if (CWebOptionHelper::GetInstance()->GetImageStr(nSnsID, szStr, 1024)) {
                mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "image_attr", mprParseJson(szStr));
            }
            if (CWebOptionHelper::GetInstance()->GetLdcStr(nSnsID, szStr, 1024)) {
                mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "ldc_attr", mprParseJson(szStr));
            }

            LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));
        } else if (!bActionProcessing) {
            bActionProcessing = AX_TRUE;

            MprJson* jsonCamera = httpGetParams(conn);
            LOG_MM_I(WEB, "Web request: %s", mprJsonToString(jsonCamera, MPR_JSON_QUOTES));

            s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_IMAGE, jsonCamera);

            bActionProcessing = AX_FALSE;
        } else if (bActionProcessing) {
            // Response status: Not Acceptable
            nHttpStatusCode = 406;
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void AudioAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        if (strcmp(conn->rx->method, "GET") == 0) {
            const AX_U32 MAX_CHAR = 512;
            AX_CHAR szAudio[MAX_CHAR] = {0};
            WEB_AUDIO_ATTR_T tAudioAttr = CWebOptionHelper::GetInstance()->GetAudio();
            sprintf(szAudio, "{capture_attr:{volume_val: %0.2f}}, play_attr:{volume_val: %0.2f}}", tAudioAttr.fCapture_volume,
                    tAudioAttr.fPlay_volume);
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "info", mprParseJson(szAudio));

            LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));
        } else {
            MprJson* jsonVideo = httpGetParams(conn);
            s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_AUDIO, jsonVideo);
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void VideoAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        if (strcmp(conn->rx->method, "GET") == 0) {
            AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));

            AX_CHAR szCapList[16] = {0};
            AX_U8 nSize = CWebOptionHelper::GetInstance()->GetVideoCount(nSnsID);

            if (1 == nSize) {
                sprintf(szCapList, "[1,0,0,0]");
            } else if (2 == nSize) {
                sprintf(szCapList, "[1,1,0,0]");
            } else if (3 == nSize) {
                sprintf(szCapList, "[1,1,1,0]");
            }
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "cap_list", mprParseJson(szCapList));
            const AX_U32 MAX_CHAR = 2048;
            AX_CHAR szVideo[MAX_CHAR] = {0};
            for (AX_U8 i = 0; i < nSize; i++) {
                if (CWebOptionHelper::GetInstance()->GetVideoStr(nSnsID, i, szVideo, MAX_CHAR)) {
                    string strKey = "video" + to_string(i);
                    mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), strKey.c_str(), mprParseJson(szVideo));
                }
            }

            LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));
        } else {
            MprJson* jsonVideo = httpGetParams(conn);
            s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_VIDEO, jsonVideo);
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void AiAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
    if (strcmp(conn->rx->method, "GET") == 0) {
        AX_CHAR szAiAttr[1024] = {0};
        if (!CWebOptionHelper::GetInstance()->GetAiInfoStr(nSnsID, szAiAttr, 1024)) {
            LOG_MM_E(WEB, "Get AI info failed.");
            return;
        }

        mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "ai_attr", mprParseJson(szAiAttr));
        LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));
    } else {
        MprJson* jsonAi = httpGetParams(conn);
        LOG_MM_I(WEB, "Web request: %s", mprJsonToString(jsonAi, MPR_JSON_QUOTES));

        s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_AI, jsonAi);
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void OverlayAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);

        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
        return;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    cchar* szToken = GetTokenFromConn(conn, AX_TRUE);
    if (0 != szToken && strlen(szToken) > 0) {
        if (strcmp(conn->rx->method, "GET") == 0) {
            AX_U8 nSnsID = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
            AX_CHAR szStr[4096] = {0};
            CWebOptionHelper::GetInstance()->GetOsdStr(nSnsID, szStr, 4096);
            MprJson* jsonOverlyAttr = mprParseJson(szStr);
            mprWriteJsonObj(mprGetJsonObj(pResponseBody, "data"), "overlay_attr", mprGetJsonObj(jsonOverlyAttr, "overlay_attr"));

            LOG_MM_I(WEB, "resp:%s", mprJsonToString(pResponseBody, MPR_JSON_QUOTES));
        } else {
            MprJson* jsonOsd = httpGetParams(conn);
            LOG_MM_I(WEB, "Web request: %s", mprJsonToString(jsonOsd, MPR_JSON_QUOTES));

            s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_OSD, jsonOsd);
        }
    }

    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, 200);
    httpFinalize(conn);

    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
}

static void Switch3ASyncRatioAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }

    static AX_BOOL bActionProcessing = AX_FALSE;
    AX_S32 nHttpStatusCode = 200;
    if (!bActionProcessing) {
        bActionProcessing = AX_TRUE;
        MprJson* json3ASR = httpGetParams(conn);
        LOG_MM_I(WEB, "Web request: %s", mprJsonToString(json3ASR, MPR_JSON_QUOTES));
        s_pWebInstance->GetPPLInstance()->ProcessWebOprs(E_REQ_TYPE_SWITCH_3A_SYNCRATIO, json3ASR);
        bActionProcessing = AX_FALSE;
    } else if (bActionProcessing) {
        // Response status: Not Acceptable
        nHttpStatusCode = 406;
    }

    MprJson* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK, 0);
    httpSetContentType(conn, "application/json");
    httpWrite(conn->writeq, mprJsonToString(pResponseBody, MPR_JSON_QUOTES));

    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);
}

/////////////////////////////////////////////////////////////////////////////////////////
typedef struct _HTTP_ACTION_INFO {
    cchar* name;
    HttpAction action;
} HTTP_ACTION_INFO;

const HTTP_ACTION_INFO g_httpActionInfo[] = {{"/action/login", LoginAction},
                                             {"/action/setting/capability", CapabilityAction},
                                             {"/action/preview/assist", AssistInfoAction},
                                             {"/action/setting/system", SystemAction},
                                             {"/action/setting/camera", CameraAction},
                                             {"/action/setting/image", ImageAction},
                                             {"/action/setting/audio", AudioAction},
                                             {"/action/setting/video", VideoAction},
                                             {"/action/setting/ai", AiAction},
                                             {"/action/setting/overlay", OverlayAction},
                                             {"/action/preview/info", PreviewInfoAction},
                                             {"/action/preview/stream", SwitchChnAction},
                                             {"/action/preview/snapshot", SnapshotAction},
                                             {"/action/preview/trigger", TriggerAction},
                                             {"/action/preview/sync_ratio_3a", Switch3ASyncRatioAction},
                                             {"/audio_0", WSAudioAction},
                                             {"/talk", WSTalkAction},
                                             {"/preview_0", WSPreviewAction},
                                             {"/preview_1", WSPreviewAction},
                                             {"/capture_0", WSCaptureAction},
                                             {"/capture_1", WSSnapshotAction},
                                             {"/events", WSEventsAction}};

CWebServer::CWebServer() : m_bServerStarted(AX_FALSE), m_pAppwebThread(nullptr) {
    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; i++) {
        m_arrConnStatus[i] = AX_FALSE;
    }
}

CWebServer::~CWebServer(AX_VOID) {
    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; i++) {
        if (m_arrChannelData[i].pRingBuffer) {
            delete m_arrChannelData[i].pRingBuffer;
        }
    }
}

AX_BOOL CWebServer::Init() {
    g_mapUserInfo["admin"] = "admin";
    char szName[64] = {0};
    sprintf(szName, "EVENTS_CH%d", WS_EVENTS_CHANNEL);
    RequestRingbuf(WS_EVENTS_CHANNEL, MAX_EVENTS_CHN_SIZE, AX_WEB_EVENTS_RING_BUFF_COUNT, szName);
    return AX_TRUE;
}

AX_BOOL CWebServer::DeInit() {
    return AX_TRUE;
}

AX_BOOL CWebServer::Start() {
    LOG_MM_I(WEB, "+++");

    m_pAppwebThread = new thread(WebServerThreadFunc, this);

    LOG_MM_I(WEB, "---");

    return (m_pAppwebThread) ? AX_TRUE : AX_FALSE;
}

AX_BOOL CWebServer::Stop() {
    LOG_MM_I(WEB, "+++");

    // Make sure logout first
    LogOut();

    m_bServerStarted = AX_FALSE;
    if (m_pSendDataThread) {
        if (m_pSendDataThread->joinable()) {
            m_pSendDataThread->join();
        }
        delete m_pSendDataThread;
        m_pSendDataThread = nullptr;
    }
    if (m_pRecvDataThread) {
        if (m_pRecvDataThread->joinable()) {
            m_pRecvDataThread->join();
        }
        delete m_pRecvDataThread;
        m_pRecvDataThread = nullptr;
    }
    if (m_pAppwebThread) {
        mprShutdown(MPR_EXIT_NORMAL, -1, MPR_EXIT_TIMEOUT);
        if (m_pAppwebThread->joinable()) {
            m_pAppwebThread->join();
        }
        delete m_pAppwebThread;
        m_pAppwebThread = nullptr;
    }

    LOG_MM_I(WEB, "---");
    return AX_TRUE;
}

AX_VOID CWebServer::RestartPreview() {
    // WEB_EVENTS_DATA_T tEvent;
    // tEvent.eType = E_WEB_EVENTS_TYPE_ReStartPreview;
    // g_webserver.SendEventsData(&tEvent);
}

AX_BOOL CWebServer::RequestRingbuf(AX_U32 nUniChn, AX_U32 nElementBuffSize, AX_U32 nBuffCount, string strName) {
    if (nUniChn >= MAX_WS_CONN_NUM) {
        return AX_FALSE;
    }

    WS_CHANNEL_DATA_T& tChnData = m_arrChannelData[nUniChn];
    tChnData.nChannel = nUniChn;
    if (tChnData.pRingBuffer != nullptr) {
        delete tChnData.pRingBuffer;
        tChnData.pRingBuffer = nullptr;
    }
    tChnData.pRingBuffer = new CAXRingBuffer(nElementBuffSize, nBuffCount, strName.c_str());

    return tChnData.pRingBuffer == nullptr ? AX_FALSE : AX_TRUE;
}

void* CWebServer::WebServerThreadFunc(void* pThis) {
    LOG_MM_I(WEB, "+++");

    prctl(PR_SET_NAME, "APPWEB_Main");

    CWebServer* pWebServer = (CWebServer*)pThis;
    Mpr* pMpr = nullptr;

    do {
        if ((pMpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD)) == 0) {
            LOG_MM_E(WEB, "Cannot create runtime.");
            break;
        }

        if (httpCreate(HTTP_CLIENT_SIDE | HTTP_SERVER_SIDE) == 0) {
            LOG_MM_E(WEB, "Cannot create the HTTP services.");
            break;
        }

        if (maLoadModules() < 0) {
            LOG_MM_E(WEB, "Cannot load modules.");
            break;
        }

        g_pClients = mprCreateList(0, 0);
        mprAddRoot(g_pClients);
        mprStart();

        string strConfigDir = CCommonUtils::GetPPLConfigDir();
        if (strConfigDir.empty()) {
            LOG_MM_E(WEB, "Get configuration directory failed.");
            break;
        }
        strConfigDir += "/appweb.conf";

        if (maParseConfig(strConfigDir.c_str()) < 0) {
            LOG_MM_E(WEB, "Cannot parse the config file %s.", strConfigDir.c_str());
            break;
        }

        for (AX_U32 i = 0; i < sizeof(g_httpActionInfo) / sizeof(HTTP_ACTION_INFO); i++) {
            httpDefineAction(g_httpActionInfo[i].name, g_httpActionInfo[i].action);
        }

        if (httpStartEndpoints() < 0) {
            LOG_MM_E(WEB, "Cannot start the web server.");
            break;
        }

        LOG_M(WEB, "Appweb started.");

        pWebServer->m_bServerStarted = AX_TRUE;
        pWebServer->m_pSendDataThread = new thread(SendDataThreadFunc, pWebServer);
        pWebServer->m_pRecvDataThread = new thread(RecvDataThreadFunc, pWebServer);

        mprServiceEvents(-1, 0);
    } while (false);

    LOG_MM_I(WEB, "---");

    return nullptr;
}

void* CWebServer::SendDataThreadFunc(void* pThis) {
    LOG_MM_I(WEB, "+++");
    prctl(PR_SET_NAME, "IPC_APPWEB_SendWSData");

    CWebServer* pWebServer = (CWebServer*)pThis;
    while (pWebServer->m_bServerStarted) {
        pWebServer->SendWSData();
        CElapsedTimer::mSleep(10);
        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
    }

    LOG_MM_I(WEB, "---");
    return nullptr;
}

void* CWebServer::RecvDataThreadFunc(void* pThis) {
    LOG_MM_I(WEB, "+++");
    prctl(PR_SET_NAME, "IPC_APPWEB_RecvWSData");

    CWebServer* pWebServer = (CWebServer*)pThis;
    while (pWebServer->m_bServerStarted) {
        pWebServer->RecvWSData();
        CElapsedTimer::mSleep(10);
        if (mprNeedYield()) {
            mprYield(MPR_YIELD_DEFAULT);
        }
    }

    LOG_MM_I(WEB, "---");
    return nullptr;
}

AX_VOID CWebServer::SendWSData(AX_VOID) {
    CWebServer* pWebServer = this;
    AX_S32 nSnsID = 0;
    AX_S32 nUniChannel = 0;
    AX_BOOL arrDataStatus[MAX_WS_CONN_NUM] = {AX_FALSE};
    HttpConn* client = nullptr;

    // gPrintHelper.Remove(E_PH_MOD_WEB_CONN, 0);
    mprLock(g_pClients->mutex);
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        LOG_MM_D(WEB, "connect %p send data +++", client);

        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            LOG_MM_D(WEB, "connect %p is closed", client);
            continue;
        }

        nSnsID = GetSnsIDFromWS(client);
        if (nSnsID == -1 || nSnsID >= 2) {
            LOG_MM_D(WEB, "connect %p nSnsID = %d is invalid", client, nSnsID);
            continue;
        }

        nUniChannel = GetChnFromWS(client);
        if (nUniChannel == -1 || nUniChannel >= MAX_WS_CONN_NUM) {
            LOG_MM_D(WEB, "connect %p nUniChannel = %d is invalid", client, nUniChannel);
            continue;
        }
        // gPrintHelper.Add(E_PH_MOD_WEB_CONN, 0, 0);
        {
            std::lock_guard<std::mutex> guard(pWebServer->m_mtxConnStatus);
            if (!pWebServer->m_arrConnStatus[nUniChannel]) {
                pWebServer->m_arrConnStatus[nUniChannel] = AX_TRUE;
            }
        }

        if (!pWebServer->m_arrChannelData[nUniChannel].pRingBuffer) {
            /* Ringbuff is null */
            if (nUniChannel == 0) {
                LOG_MM_D(WEB, "connect %p nUniChannel = %d, pRingBuffer is empty", client, nUniChannel);
            }
            continue;
        }

        CAXRingElement* pData = pWebServer->m_arrChannelData[nUniChannel].pRingBuffer->Get();
        if (!pData) {
            /* Ringbuff is empty */
            if (nUniChannel == 0) {
                LOG_MM_D(WEB, "connect %p nUniChannel = %d pdata is empty", client, nUniChannel);
            }
            continue;
        }

        arrDataStatus[nUniChannel] = AX_TRUE;  // got data
        AX_U32 limit = client->rx->route->limits->webSocketsFrameSize;
        if (pData->nSize >= (AX_U32)limit) {
            LOG_MM_E(WEB, "Websocket data size(%d) exceeding max frame size(%d).", pData->nSize, limit);
        }

        if (nUniChannel == 0) {
            LOG_MM_D(WEB, "connect %p send data ---", client);
        }

        {
            WSMsg_T* msg = new WSMsg_T;
            msg->conn = client;
            msg->packet = pData;
            auto pEvent =
                mprCreateEvent(client->dispatcher, "ws", 0, (void*)SendHttpData, (void*)msg, MPR_EVENT_STATIC_DATA | MPR_EVENT_ALWAYS);
            if (!pEvent) {
                pData->pParent->Free(pData);
                delete msg;
                msg = nullptr;
            }
        }
    }
    mprUnlock(g_pClients->mutex);

    pWebServer->UpdateConnStatus();
    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; i++) {
        if (arrDataStatus[i] && pWebServer->m_arrChannelData[i].pRingBuffer) {
            pWebServer->m_arrChannelData[i].pRingBuffer->Pop(AX_FALSE);
        }
    }
}

AX_VOID CWebServer::RecvWSData(AX_VOID) {
    // only recive web talk data.
    if (!m_bAudioPlayAvailable) {
        return;
    }

    HttpConn* client = nullptr;
    AX_U8 nTalkChannel = GetTalkChannel();

    mprLock(g_pClients->mutex);
    for (AX_S32 next = 0; (client = (HttpConn*)mprGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }

        MprBuf* pWSData = (MprBuf*)httpGetWebSocketData(client);
        AX_U8 nChannel = 0;
        if (!pWSData) {
            continue;
        } else {
            AX_U16 nWSData = (AX_U16)mprGetUint16FromBuf(pWSData);
            mprAdjustBufStart(pWSData, -2);  // restore the start pointer to offfset 0
            nChannel = nWSData >> 8;
            if (nChannel != nTalkChannel) {
                continue;
            }
        }
        WSMsg_T* msg = new WSMsg_T;
        msg->conn = client;
        auto pEvent =
            mprCreateEvent(client->dispatcher, "ws", 0, (void*)RecieveHttpData, (void*)msg, MPR_EVENT_STATIC_DATA | MPR_EVENT_ALWAYS);
        if (!pEvent) {
            delete msg;
            msg = nullptr;
        }
    }
    mprUnlock(g_pClients->mutex);
}

AX_VOID CWebServer::SendPreviewData(AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts /*= 0*/, AX_BOOL bIFrame /*= AX_FALSE*/) {
    if (!m_bServerStarted) {
        return;
    }

    {
        /* Waiting for reading thread to refresh the websock conn status */
        std::lock_guard<std::mutex> guard(m_mtxConnStatus);
        AX_BOOL bConnect = m_arrConnStatus[nUniChn];
        if (!bConnect) {
            if (nUniChn == 0) {
                LOG_MM_D(WEB, "[%d] is not connected", nUniChn);
            }
            return;
        }
    }

    AX_BOOL bSuc = AX_FALSE;

    if (m_bAudioCaptureAvailable) {
        PTS_HEADER_T tHeader;
        tHeader.nDatalen = size;
        tHeader.nPts = nPts;
        CAXRingElement ele((AX_U8*)data, size, nPts, bIFrame, (AX_U8*)&tHeader, (AX_U32)(sizeof(tHeader)));
        bSuc = m_arrChannelData[nUniChn].pRingBuffer->Put(ele);
    } else {
        CAXRingElement ele((AX_U8*)data, size, nPts, bIFrame);
        bSuc = m_arrChannelData[nUniChn].pRingBuffer->Put(ele);
    }

    if (nUniChn == 0 && !bSuc) {
        LOG_MM_W(WEB, "[%d] put data failed", nUniChn);
    }
}

AX_VOID CWebServer::SendPushImgData(AX_U8 nSnsID, AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts /*= 0*/,
                                    AX_BOOL bIFrame /*= AX_TRUE*/, JPEG_DATA_INFO_T* pJpegInfo /*= nullptr*/) {
    if (!m_bServerStarted) {
        return;
    }

    if (0 == nSnsID && !m_arrCaptureEnable[nSnsID]) {
        return;
    }

    if (1 == nSnsID && !m_arrCaptureEnable[nSnsID]) {
        return;
    }

    {
        /* Waiting for reading thread to refresh the websock conn status */
        std::lock_guard<std::mutex> guard(m_mtxConnStatus);
        AX_BOOL bConnect = m_arrConnStatus[m_nCaptureChannel];
        if (!bConnect) {
            return;
        }
    }

    LOG_MM_D(WEB, "[%d] Send capture data, size=%d", m_nCaptureChannel, size);

    if (nullptr != pJpegInfo) {
        AX_CHAR szJsonData[256] = {0};
        /* construct json data info */
        sprintf(szJsonData,
                "{\"snsId\": %d, \"type\": %d, \"attribute\": {\"src\": %d, \"width\": %d, \"height\": %d, \"plate\": {\"num\": "
                "\"unknown\", \"color\": \"unknown\"} }}",
                nSnsID, pJpegInfo->eType, pJpegInfo->tCaptureInfo.tHeaderInfo.nSnsSrc, pJpegInfo->tCaptureInfo.tHeaderInfo.nWidth,
                pJpegInfo->tCaptureInfo.tHeaderInfo.nHeight);

        AX_U32 nJsnLen = strlen(szJsonData);

        /* construct jpeg head */
        JpegHead tJpegHead;
        tJpegHead.nJsonLen = nJsnLen > 0 ? nJsnLen + 1 : 0;
        tJpegHead.nTotalLen = 4 /*magic*/ + 4 /*total len*/ + 4 /*tag*/ + 4 /*json len*/ + tJpegHead.nJsonLen;

        strcpy(tJpegHead.szJsonData, szJsonData);

        CAXRingElement _ele((AX_U8*)data, size, nPts, bIFrame, (AX_U8*)&tJpegHead, tJpegHead.nTotalLen);
        m_arrChannelData[m_nCaptureChannel].pRingBuffer->Put(_ele);
    } else {
        CAXRingElement ele((AX_U8*)data, size, nPts, bIFrame);
        m_arrChannelData[m_nCaptureChannel].pRingBuffer->Put(ele);
    }
}

AX_VOID CWebServer::SendCaptureData(AX_U8 nSnsID, AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts /*= 0*/,
                                    AX_BOOL bIFrame /*= AX_TRUE*/, JPEG_DATA_INFO_T* pJpegInfo /*= nullptr*/) {
    if (!m_bServerStarted) {
        return;
    }

    if (0 == nSnsID && !m_arrCaptureEnable[nSnsID]) {
        return;
    }

    if (1 == nSnsID && !m_arrCaptureEnable[nSnsID]) {
        return;
    }

    {
        /* Waiting for reading thread to refresh the websock conn status */
        std::lock_guard<std::mutex> guard(m_mtxConnStatus);
        AX_BOOL bConnect = m_arrConnStatus[m_nCaptureChannel];
        if (!bConnect) {
            return;
        }
    }

    LOG_MM_D(WEB, "[%d] Send capture data, size=%d", m_nCaptureChannel, size);
    JPEG_TYPE_E eType = m_mapJencChnType[nUniChn];
    if (nullptr != pJpegInfo) {
        AX_CHAR szJsonData[256] = {0};
        /* construct json data info */
        sprintf(szJsonData, "{\"snsId\": %d, \"type\": %d, \"attribute\": {\"src\": %d, \"width\": %d, \"height\": %d}}", nSnsID, eType,
                pJpegInfo->tCaptureInfo.tHeaderInfo.nSnsSrc, pJpegInfo->tCaptureInfo.tHeaderInfo.nWidth,
                pJpegInfo->tCaptureInfo.tHeaderInfo.nHeight);

        AX_U32 nJsnLen = strlen(szJsonData);

        /* construct jpeg head */
        JpegHead tJpegHead;
        tJpegHead.nJsonLen = nJsnLen > 0 ? nJsnLen + 1 : 0;
        tJpegHead.nTotalLen = 4 /*magic*/ + 4 /*total len*/ + 4 /*tag*/ + 4 /*json len*/ + tJpegHead.nJsonLen;

        strcpy(tJpegHead.szJsonData, szJsonData);

        CAXRingElement _ele((AX_U8*)data, size, nPts, bIFrame, (AX_U8*)&tJpegHead, tJpegHead.nTotalLen);
        m_arrChannelData[m_nCaptureChannel].pRingBuffer->Put(_ele);
    } else {
        CAXRingElement ele((AX_U8*)data, size, nPts, bIFrame);
        m_arrChannelData[m_nCaptureChannel].pRingBuffer->Put(ele);
    }
}

AX_VOID CWebServer::SendSnapshotData(AX_VOID* data, AX_U32 size) {
    if (!m_bServerStarted) {
        return;
    }

    AX_U8 nSnapshotChannelID = GetSnapshotChannel();
    HttpConn* client = nullptr;
    // no mprLock(g_pClients->mutex) because sync call by web server snapshot command.
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }

        AX_U8 nUniChannel = GetChnFromWS(client);
        if (nUniChannel != nSnapshotChannelID) {
            continue;
        }

        LOG_MM_I(WEB, "[%d] Send snapshot data, size=%d", nSnapshotChannelID, size);
        ssize nRet = httpSendBlock(client, WS_MSG_BINARY, (cchar*)data, size, HTTP_BLOCK);
        // Only Send data to the first connection.
        if (nRet >= 0) {
            break;
        }
    }
}

AX_VOID CWebServer::SendAudioData(AX_U8 nUniChn, AX_VOID* data, AX_U32 size, AX_U64 nPts /*= 0*/) {
    // AAC/PCM711A
    if (!m_bServerStarted) {
        return;
    }

    AX_U8 nAencChannel = GetAencChannel();
    {
        /* Waiting for reading thread to refresh the websock conn status */
        std::lock_guard<std::mutex> guard(m_mtxConnStatus);
        AX_BOOL bConnect = m_arrConnStatus[nAencChannel];
        if (!bConnect) {
            return;
        }
    }
    PTS_HEADER_T tHeader;
    tHeader.nDatalen = size;
    tHeader.nPts = nPts;

    CAXRingElement ele((AX_U8*)data, size, nPts, AX_TRUE, (AX_U8*)&tHeader, (AX_U32)(sizeof(tHeader)));
    m_arrChannelData[nAencChannel].pRingBuffer->Put(ele);
}

AX_BOOL CWebServer::SendEventsData(WEB_EVENTS_DATA_T* data) {
    if (!m_bServerStarted) {
        return AX_FALSE;
    }
    {
        /* Waiting for reading thread to refresh the websock conn status */
        std::lock_guard<std::mutex> guard(m_mtxConnStatus);
        AX_BOOL bConnect = m_arrConnStatus[WS_EVENTS_CHANNEL];
        if (!bConnect) {
            return AX_FALSE;
        }
    }

    AX_U8 nChnnelID = m_arrChannelData[WS_EVENTS_CHANNEL].nInnerIndex;
    string strEventsJson;
    if (E_WEB_EVENTS_TYPE_ReStartPreview == data->eType || E_WEB_EVENTS_TYPE_LogOut == data->eType) {
        strEventsJson = FormatPreviewEventsJson(data);
    } else {
        strEventsJson = FormatIVESEventsJson(data);
    }
    CAXRingElement ele((AX_U8*)strEventsJson.c_str(), strEventsJson.length(), nChnnelID);
    m_arrChannelData[WS_EVENTS_CHANNEL].pRingBuffer->Put(ele);

    return AX_TRUE;
}

std::string CWebServer::FormatPreviewEventsJson(WEB_EVENTS_DATA_T* pEvent) {
    AX_CHAR szEventJson[MAX_EVENTS_CHN_SIZE] = {0};
    AX_CHAR szDate[64] = {0};
    CElapsedTimer::GetInstance()->GetLocalTime(szDate, 16);
    snprintf(szEventJson, MAX_EVENTS_CHN_SIZE, "{\"events\": [{\"type\": %d, \"sensor\":%d, \"date\": \"%s\"}]}", pEvent->eType,
             pEvent->nReserved, szDate);

    std::string strEvent;
    strEvent = szEventJson;

    return strEvent;
}

std::string CWebServer::FormatIVESEventsJson(WEB_EVENTS_DATA_T* pEvent) {
    AX_CHAR szEventJson[MAX_EVENTS_CHN_SIZE] = {0};
    AX_CHAR szDate[64] = {0};
    CElapsedTimer::GetInstance()->GetLocalTime(szDate, 16);

    if (E_WEB_EVENTS_TYPE_MD == pEvent->eType) {
        snprintf(szEventJson, MAX_EVENTS_CHN_SIZE, "{\"events\": [{\"type\": %d, \"sensor\":%d, \"date\": \"%s\"}]}", pEvent->eType,
                 pEvent->nReserved, szDate);
    } else if (E_WEB_EVENTS_TYPE_OD == pEvent->eType) {
        snprintf(szEventJson, MAX_EVENTS_CHN_SIZE, "{\"events\": [{\"type\": %d, \"sensor\":%d,\"date\": \"%s\"}]}", pEvent->eType,
                 pEvent->nReserved, szDate);
    } else if (E_WEB_EVENTS_TYPE_SCD == pEvent->eType) {
        snprintf(szEventJson, MAX_EVENTS_CHN_SIZE, "{\"events\": [{\"type\": %d, \"sensor\":%d, \"date\": \"%s\"}]}", pEvent->eType,
                 pEvent->nReserved, szDate);
    } else if (E_WEB_EVENTS_TYPE_ReStartPreview == pEvent->eType) {
        snprintf(szEventJson, MAX_EVENTS_CHN_SIZE, "{\"events\": [{\"type\": %d, \"sensor\":%d, \"date\": \"%s\"}]}", pEvent->eType,
                 pEvent->nReserved, szDate);
    }
    std::string strEvent;
    strEvent = szEventJson;

    return strEvent;
}

/* TODO: PrevChn -> UniChn mapping must be registered in order by modules */
static AX_U8 s_arrSnsPrevInnerIndex[2] = {0, 0};
AX_U8 CWebServer::RegistPreviewChnMappingInOrder(AX_U8 nSnsID, AX_U8 nUniChn, AX_U8 nType) {
    m_sMapPrevChn2UniChn[nSnsID][s_arrSnsPrevInnerIndex[nSnsID]] = std::make_pair(nUniChn, nType);
    return s_arrSnsPrevInnerIndex[nSnsID]++;
}

AX_VOID CWebServer::UpdateMediaTypeInPreviewChnMap(AX_U8 nSnsID, AX_U8 nUniChn, AX_U8 nType) {
    /* {SnsID: {PrevID: (UniChn, CodecType)}} */
    auto& snsIDMap = m_sMapPrevChn2UniChn[nSnsID];
    for (auto& item : snsIDMap) {
        // {PrevID: {UniChn, CodecType}}
        if (item.second.first == nUniChn) item.second.second = nType;
    }
}

AX_VOID CWebServer::RegistUniCaptureChn(AX_S8 nCaptureChn, JPEG_TYPE_E eType) {
    if (-1 == m_nCaptureChannel) {
        /* Applies the first setting value */
        m_nCaptureChannel = nCaptureChn;
    }
    m_mapJencChnType[nCaptureChn] = eType;
}

AX_VOID CWebServer::UpdateConnStatus(AX_VOID) {
    AX_S32 nUniChannel = -1;
    AX_BOOL arrConnStatus[MAX_WS_CONN_NUM] = {AX_FALSE};
    HttpConn* client = nullptr;

    mprLock(g_pClients->mutex);
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }

        nUniChannel = GetChnFromWS(client);
        if (nUniChannel == -1 || nUniChannel >= MAX_WS_CONN_NUM) {
            continue;
        }

        arrConnStatus[nUniChannel] = AX_TRUE;
    }
    mprUnlock(g_pClients->mutex);

    std::lock_guard<std::mutex> guard(m_mtxConnStatus);
    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; ++i) {
        if (!arrConnStatus[i]) {
            m_arrConnStatus[i] = AX_FALSE;
        } else {
            m_arrConnStatus[i] = AX_TRUE;
        }
    }
}

AX_VOID CWebServer::LogOut() {
    WEB_EVENTS_DATA_T tEvent;
    tEvent.eType = E_WEB_EVENTS_TYPE_LogOut;
    if (SendEventsData(&tEvent)) {
        CElapsedTimer::mSleep(1500);  // Wait for Logout done
    }
}
