/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "PanoSpecConfig.h"
#include "OptionHelper.h"
#include "WebOptionHelper.h"

using namespace AX_PANO;

AX_BOOL CPANOSPEC::Init() {
    std::vector<OSD_CFG_T> vecOsdCfg;
    int nCharLen = 0;
    wchar_t wszOsdStr[MAX_OSD_WSTR_CHAR_LEN] = {0};

    OSD_CFG_T tOsdConfig;
    {
        /* VENC Primary*/
        vecOsdCfg.clear();
        // TIME
        tOsdConfig.bEnable = AX_TRUE;
        tOsdConfig.eType = OSD_TYPE_TIME;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_LEFT_TOP;
        tOsdConfig.nZIndex = 0;
        tOsdConfig.tTimeAttr.eFormat = OSD_DATE_FORMAT_YYMMDDHHmmSS;
        tOsdConfig.tTimeAttr.nColor = 0xFFFFFF;
        tOsdConfig.tTimeAttr.nFontSize = 128;
        tOsdConfig.nBoundaryX = 48;
        tOsdConfig.nBoundaryY = 20;
        nCharLen = 19;
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.tTimeAttr.nFontSize / 2, BASE_FONT_SIZE) * nCharLen;
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.tTimeAttr.nFontSize, BASE_FONT_SIZE);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.nBoundaryW, OSD_ALIGN_WIDTH);
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.nBoundaryH, OSD_ALIGN_HEIGHT);

        vecOsdCfg.push_back(tOsdConfig);

        // LOGO
        tOsdConfig.bEnable = AX_TRUE;
        tOsdConfig.eType = OSD_TYPE_PICTURE;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_RIGHT_BOTTOM;
        tOsdConfig.nZIndex = 1;
        strcpy(tOsdConfig.tPicAttr.szFileName, "./res/axera_logo_256x64.argb1555");
        tOsdConfig.tPicAttr.nWidth = 256;
        tOsdConfig.tPicAttr.nHeight = 64;
        tOsdConfig.nBoundaryX = 32;
        tOsdConfig.nBoundaryY = 48;
        tOsdConfig.nBoundaryW = 256;
        tOsdConfig.nBoundaryH = 64;

        vecOsdCfg.push_back(tOsdConfig);

        /*JENC*/
        CWebOptionHelper::GetInstance()->SetOsdConfig(0, 2, 0, vecOsdCfg);

        // Channel
        tOsdConfig.bEnable = AX_FALSE;
        tOsdConfig.eType = OSD_TYPE_STRING_CHANNEL;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_RIGHT_TOP;
        tOsdConfig.nZIndex = 2;
        tOsdConfig.tStrAttr.nColor = 0xFFFFFF;
        sprintf(tOsdConfig.tStrAttr.szStr, "CHANNEL-*");
        tOsdConfig.nBoundaryX = 20;
        tOsdConfig.nBoundaryY = 20;
        tOsdConfig.tStrAttr.nFontSize = 40;
        nCharLen = swprintf(&wszOsdStr[0], MAX_OSD_WSTR_CHAR_LEN, L"%s", tOsdConfig.tStrAttr.szStr);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize / 2, BASE_FONT_SIZE) * nCharLen;
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize, BASE_FONT_SIZE);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.nBoundaryW, OSD_ALIGN_WIDTH);
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.nBoundaryH, OSD_ALIGN_HEIGHT);

        vecOsdCfg.push_back(tOsdConfig);

        // Location
        tOsdConfig.bEnable = AX_FALSE;
        tOsdConfig.eType = OSD_TYPE_STRING_LOCATION;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_LEFT_BOTTOM;
        tOsdConfig.nZIndex = 3;
        tOsdConfig.tStrAttr.nColor = 0xFFFFFF;
        sprintf(tOsdConfig.tStrAttr.szStr, "LOCATION-*");
        tOsdConfig.nBoundaryX = 20;
        tOsdConfig.nBoundaryY = 20;
        tOsdConfig.tStrAttr.nFontSize = 40;
        nCharLen = swprintf(&wszOsdStr[0], MAX_OSD_WSTR_CHAR_LEN, L"%s", tOsdConfig.tStrAttr.szStr);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize / 2, BASE_FONT_SIZE) * nCharLen;
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize, BASE_FONT_SIZE);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.nBoundaryW, OSD_ALIGN_WIDTH);
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.nBoundaryH, OSD_ALIGN_HEIGHT);

        vecOsdCfg.push_back(tOsdConfig);

        // Privacy
        tOsdConfig.bEnable = AX_FALSE;
        tOsdConfig.eType = OSD_TYPE_PRIVACY;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_LEFT_TOP;
        tOsdConfig.nBoundaryX = 0;
        tOsdConfig.nBoundaryY = 0;
        tOsdConfig.nBoundaryW = -1;
        tOsdConfig.nBoundaryH = -1;
        tOsdConfig.nZIndex = 4;
        tOsdConfig.tPrivacyAttr.eType = OSD_PRIVACY_TYPE_RECT;
        tOsdConfig.tPrivacyAttr.nLineWidth = 2;
        tOsdConfig.tPrivacyAttr.nColor = 0xFFFFFF;
        tOsdConfig.tPrivacyAttr.bSolid = AX_FALSE;
        int nSlide = 200;
        tOsdConfig.tPrivacyAttr.tPt[0].x = 20;
        tOsdConfig.tPrivacyAttr.tPt[0].y = 200;  // nHeight / 2 - nSlide;
        tOsdConfig.tPrivacyAttr.tPt[1].x = 20 + nSlide * 2;
        tOsdConfig.tPrivacyAttr.tPt[1].y = 200;  // nHeight / 2 - nSlide;
        tOsdConfig.tPrivacyAttr.tPt[2].x = 20 + nSlide * 2;
        tOsdConfig.tPrivacyAttr.tPt[2].y = 400;  // nHeight / 2 + nSlide;
        tOsdConfig.tPrivacyAttr.tPt[3].x = 20;
        tOsdConfig.tPrivacyAttr.tPt[3].y = 400;  // nHeight / 2 + nSlide;

        vecOsdCfg.push_back(tOsdConfig);

        // Rect
        tOsdConfig.bEnable = AX_TRUE;
        tOsdConfig.nZIndex = 5;
        tOsdConfig.eType = OSD_TYPE_RECT;
        vecOsdCfg.push_back(tOsdConfig);

        CWebOptionHelper::GetInstance()->SetOsdConfig(0, 3, 0, vecOsdCfg);
    }
    {
        /*VENC Secondary*/
        vecOsdCfg.clear();
        // TIME
        tOsdConfig.bEnable = AX_TRUE;
        tOsdConfig.eType = OSD_TYPE_TIME;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_LEFT_TOP;
        tOsdConfig.nZIndex = 0;
        tOsdConfig.tTimeAttr.eFormat = OSD_DATE_FORMAT_YYMMDDHHmmSS;
        tOsdConfig.tTimeAttr.nColor = 0xFFFFFF;
        tOsdConfig.tTimeAttr.nFontSize = 64;
        tOsdConfig.nBoundaryX = 14;
        tOsdConfig.nBoundaryY = 8;
        nCharLen = 19;
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.tTimeAttr.nFontSize / 2, BASE_FONT_SIZE) * nCharLen;
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.tTimeAttr.nFontSize, BASE_FONT_SIZE);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.nBoundaryW, OSD_ALIGN_WIDTH);
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.nBoundaryH, OSD_ALIGN_HEIGHT);

        vecOsdCfg.push_back(tOsdConfig);

        // LOGO
        tOsdConfig.bEnable = AX_TRUE;
        tOsdConfig.eType = OSD_TYPE_PICTURE;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_RIGHT_BOTTOM;
        tOsdConfig.nZIndex = 1;
        strcpy(tOsdConfig.tPicAttr.szFileName, "./res/axera_logo_96x28.argb1555");
        tOsdConfig.tPicAttr.nWidth = 96;
        tOsdConfig.tPicAttr.nHeight = 28;
        tOsdConfig.nBoundaryX = 8;
        tOsdConfig.nBoundaryY = 8;
        tOsdConfig.nBoundaryW = 96;
        tOsdConfig.nBoundaryH = 28;

        vecOsdCfg.push_back(tOsdConfig);

        // Channel
        tOsdConfig.bEnable = AX_FALSE;
        tOsdConfig.eType = OSD_TYPE_STRING_CHANNEL;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_RIGHT_TOP;
        tOsdConfig.nZIndex = 2;
        tOsdConfig.tStrAttr.nColor = 0xFFFFFF;
        sprintf(tOsdConfig.tStrAttr.szStr, "CHANNEL-*");
        tOsdConfig.nBoundaryX = 8;
        tOsdConfig.nBoundaryY = 8;
        tOsdConfig.tStrAttr.nFontSize = 20;
        nCharLen = swprintf(&wszOsdStr[0], MAX_OSD_WSTR_CHAR_LEN, L"%s", tOsdConfig.tStrAttr.szStr);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize / 2, BASE_FONT_SIZE) * nCharLen;
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize, BASE_FONT_SIZE);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.nBoundaryW, OSD_ALIGN_WIDTH);
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.nBoundaryH, OSD_ALIGN_HEIGHT);

        vecOsdCfg.push_back(tOsdConfig);

        // Location
        tOsdConfig.bEnable = AX_FALSE;
        tOsdConfig.eType = OSD_TYPE_STRING_LOCATION;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_LEFT_BOTTOM;
        tOsdConfig.nZIndex = 3;
        tOsdConfig.tStrAttr.nColor = 0xFFFFFF;
        sprintf(tOsdConfig.tStrAttr.szStr, "LOCATION-*");
        tOsdConfig.nBoundaryX = 20;
        tOsdConfig.nBoundaryY = 20;
        tOsdConfig.tStrAttr.nFontSize = 20;
        nCharLen = swprintf(&wszOsdStr[0], MAX_OSD_WSTR_CHAR_LEN, L"%s", tOsdConfig.tStrAttr.szStr);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize / 2, BASE_FONT_SIZE) * nCharLen;
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.tStrAttr.nFontSize, BASE_FONT_SIZE);
        tOsdConfig.nBoundaryW = ALIGN_UP(tOsdConfig.nBoundaryW, OSD_ALIGN_WIDTH);
        tOsdConfig.nBoundaryH = ALIGN_UP(tOsdConfig.nBoundaryH, OSD_ALIGN_HEIGHT);

        vecOsdCfg.push_back(tOsdConfig);

        // Privacy
        tOsdConfig.bEnable = AX_FALSE;
        tOsdConfig.eType = OSD_TYPE_PRIVACY;
        tOsdConfig.eAlign = OSD_ALIGN_TYPE_LEFT_TOP;
        tOsdConfig.nBoundaryX = 0;
        tOsdConfig.nBoundaryY = 0;
        tOsdConfig.nBoundaryW = -1;
        tOsdConfig.nBoundaryH = -1;
        tOsdConfig.nZIndex = 4;
        tOsdConfig.tPrivacyAttr.eType = OSD_PRIVACY_TYPE_RECT;
        tOsdConfig.tPrivacyAttr.nLineWidth = 2;
        tOsdConfig.tPrivacyAttr.nColor = 0xFFFFFF;
        tOsdConfig.tPrivacyAttr.bSolid = AX_FALSE;
        int nSlide = 50;
        tOsdConfig.tPrivacyAttr.tPt[0].x = 20;
        tOsdConfig.tPrivacyAttr.tPt[0].y = 100;  // nHeight / 2 - nSlide;
        tOsdConfig.tPrivacyAttr.tPt[1].x = 20 + nSlide * 2;
        tOsdConfig.tPrivacyAttr.tPt[1].y = 100;  // nHeight / 2 - nSlide;
        tOsdConfig.tPrivacyAttr.tPt[2].x = 20 + nSlide * 2;
        tOsdConfig.tPrivacyAttr.tPt[2].y = 200;  // nHeight / 2 + nSlide;
        tOsdConfig.tPrivacyAttr.tPt[3].x = 20;
        tOsdConfig.tPrivacyAttr.tPt[3].y = 200;  // nHeight / 2 + nSlide;

        vecOsdCfg.push_back(tOsdConfig);

        // Rect
        tOsdConfig.bEnable = AX_TRUE;
        tOsdConfig.nZIndex = 5;
        tOsdConfig.eType = OSD_TYPE_RECT;
        vecOsdCfg.push_back(tOsdConfig);
        CWebOptionHelper::GetInstance()->SetOsdConfig(0, 4, 0, vecOsdCfg);
        CWebOptionHelper::GetInstance()->SetOsdConfig(0, 6, 0, vecOsdCfg);
    }

    VideoChnIndex2IvpsGrp();
    // VideoRectChnIndex2IvpsGrp();

    return AX_TRUE;
}

AX_VOID CPANOSPEC::VideoChnIndex2IvpsGrp() {
    /*pair[sensorID, videoIndex] = pair[ivpsGrp, ivpsChn] */
    std::map<pair<AX_U8, AX_U8>, pair<AX_U8, AX_U8>> pairVides2Ivps;
    pair<AX_U8, AX_U8> tVenc = make_pair(0, 0);
    pair<AX_U8, AX_U8> tIvps = make_pair(3, 0);
    pairVides2Ivps[tVenc] = tIvps;

    tVenc = make_pair(0, 1);
    tIvps = make_pair(4, 0);
    pairVides2Ivps[tVenc] = tIvps;

    tVenc = make_pair(0, 2);
    tIvps = make_pair(6, 0);
    pairVides2Ivps[tVenc] = tIvps;

    CWebOptionHelper::GetInstance()->SetIvpsGrp2VideoIndex(pairVides2Ivps);
}
