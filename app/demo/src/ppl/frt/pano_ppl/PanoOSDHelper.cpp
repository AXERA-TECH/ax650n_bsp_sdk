/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "PanoOSDHelper.h"
#include <sys/prctl.h>
#include <chrono>
#include <map>
#include "AXFrame.hpp"
#include "AXThread.hpp"
#include "AppLogApi.h"
#include "CmdLineParser.h"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "PanoSpecConfig.h"
#include "IvpsOptionHelper.h"
#include "OptionHelper.h"
#include "SensorOptionHelper.h"
#include "OsdOptionHelper.h"

#define OSD "OSD"

using namespace AX_PANO;

COSDHelper::COSDHelper() {
}

AX_BOOL COSDHelper::StartOSD(CIVPSGrpStage* pIvpsInstance) {
    LOG_MM_I(OSD, "+++");
    AX_S32 nRet = AX_SUCCESS;
    m_pIvpsGrpInstance = pIvpsInstance;
    IVPS_GROUP_CFG_T* pGrpConfig = m_pIvpsGrpInstance->GetGrpCfg();
    AX_U8 nRgnIndex = 0;
    IVPS_GRP_T* pGrpAttr = m_pIvpsGrpInstance->GetGrpPPLAttr();

    for (AX_U32 nChn = 0; nChn < pGrpConfig->nGrpChnNum; nChn++) {
        AX_U8 nChnFilter = nChn + 1;
        if (AX_TRUE != pGrpAttr->tPipelineAttr.tFilter[nChnFilter][APP_OSD_CHANNEL_FILTER_1].bEngage) {
            /* FIXME: As OSD applies on channel filter 1 now, check engine of this filter whether configured always. */
            LOG_MM_W(OSD, "[%d][%d] Engine not configured. Please check the engine_filter_3 configuration in ivps.json.", pGrpConfig->nGrp,
                     nChn);
            continue;
        }
        std::vector<OSD_CFG_T> vecOsdCfg;
        CWebOptionHelper::GetInstance()->GetOsdConfig(pGrpConfig->nSnsSrc, pGrpConfig->nGrp, nChn, vecOsdCfg);
        for (AX_U32 i = 0; i < vecOsdCfg.size(); i++) {
            OSD_CFG_T tCfg = vecOsdCfg[i];
            IVPS_RGN_HANDLE hRgn = AX_IVPS_RGN_Create();
            if (AX_IVPS_INVALID_REGION_HANDLE != hRgn) {
                AX_U32 nIvpsGrp = pGrpConfig->nGrp;
                AX_S32 nFilter = ((nChn + 1) << 4) + APP_OSD_CHANNEL_FILTER_1;
                nRet = AX_IVPS_RGN_AttachToFilter(hRgn, nIvpsGrp, nFilter);
                if (AX_SUCCESS != nRet) {
                    LOG_MM_E(OSD, "[%d] AX_IVPS_RGN_AttachToFilter(Grp: %d, Filter: 0x%x, Handle: %d) failed, ret=0x%x", nChn, nIvpsGrp,
                             nFilter, hRgn, nRet);
                    AX_IVPS_RGN_Destroy(hRgn);
                    break;
                }
                m_arrRgnThreadParam[nRgnIndex].nGroup = nIvpsGrp;
                m_arrRgnThreadParam[nRgnIndex].nChn = nChn;
                m_arrRgnThreadParam[nRgnIndex].hRgn = hRgn;
                m_arrRgnThreadParam[nRgnIndex].nFilter = nFilter;
                m_arrRgnThreadParam[nRgnIndex].tOsdCfg = tCfg;
                UpdateOSD(&m_arrRgnThreadParam[nRgnIndex]);
                nRgnIndex++;
            }
        }
    }
    m_nRgnCount = nRgnIndex;

    return AX_TRUE;
}

AX_BOOL COSDHelper::UpdateOSD(OSD_REGION_PARAM_T* pThreadParam) {
    if (nullptr == pThreadParam) {
        return AX_FALSE;
    }
    AX_CHAR threadName[32];
    sprintf(threadName, "AppRgn_Grp%d_%d", pThreadParam->nGroup, pThreadParam->tOsdCfg.nZIndex);
    switch (pThreadParam->tOsdCfg.eType) {
        case OSD_TYPE_TIME: {
            if (!pThreadParam->m_EventThread.Start(std::bind(&COSDHelper::TimeThreadFunc, this, pThreadParam), nullptr, threadName)) {
                LOG_MM_E(OSD, "start RgnThreadFunc  failed!");
                AX_IVPS_RGN_Destroy(pThreadParam->hRgn);
                break;
            }
            break;
        };
        case OSD_TYPE_PICTURE: {
            UpdateOSDPic(pThreadParam);
            break;
        };
        case OSD_TYPE_STRING_CHANNEL:
        case OSD_TYPE_STRING_LOCATION: {
            UpdateOSDStr(pThreadParam);
            break;
        };
        case OSD_TYPE_PRIVACY: {
            UpdateOSDPri(pThreadParam);
            break;
        };
        case OSD_TYPE_RECT: {
            if (!pThreadParam->m_EventThread.Start(std::bind(&COSDHelper::RectThreadFunc, this, pThreadParam), nullptr, threadName)) {
                LOG_MM_E(OSD, "start RgnThreadFunc  failed!");
                AX_IVPS_RGN_Destroy(pThreadParam->hRgn);
                break;
            }
            break;
        }
        default: {
            LOG_MM_E(OSD, "Unknown OSD type: %d", pThreadParam->tOsdCfg.eType);
            break;
        }
    }

    return AX_TRUE;
}

AX_VOID COSDHelper::TimeThreadFunc(OSD_REGION_PARAM_T* pThreadParam) {
    AX_U32 nIvpsChn = pThreadParam->nChn;
    while (!pThreadParam->bExit) {
        if (!m_pIvpsGrpInstance->IsEnabledChannel(pThreadParam->nChn)) {
            CElapsedTimer::mSleep(500);
            continue;
        }

        AX_IVPS_RGN_DISP_GROUP_T tDisp;
        AX_U16* pArgbData = nullptr;

        memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

        tDisp.nNum = 1;
        tDisp.tChnAttr.nAlpha = 255;
        tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
        tDisp.tChnAttr.nZindex = pThreadParam->tOsdCfg.nZIndex;

        COSDHandler* pThreadParamHandle = m_osdWrapper.NewInstance();
        if (nullptr == pThreadParamHandle) {
            LOG_MM_E(OSD, "Get osd handle failed.");
            break;
        }

        static AX_CHAR strTtfFile[128] = {0};
        sprintf(strTtfFile, "%s/GB2312.ttf", GetResPath().c_str());

        if (AX_FALSE == m_osdWrapper.InitHandler(pThreadParamHandle, strTtfFile)) {
            LOG_MM_E(OSD, "AX_OSDInitHandler failed, ttf: %s.", strTtfFile);
            m_osdWrapper.ReleaseInstance(&pThreadParamHandle);
            break;
        }
        wchar_t wszOsdDate[MAX_OSD_TIME_CHAR_LEN] = {0};

        memset(&wszOsdDate[0], 0, sizeof(wchar_t) * MAX_OSD_TIME_CHAR_LEN);

        AX_S32 nCharLen = 0;
        if (nullptr == CElapsedTimer::GetCurrDateStr(&wszOsdDate[0], pThreadParam->tOsdCfg.tTimeAttr.eFormat, nCharLen)) {
            LOG_MM_E(OSD, "Failed to get current date string.");
            m_osdWrapper.ReleaseInstance(&pThreadParamHandle);
            break;
        }

        AX_U8 nIvpsGrp = m_pIvpsGrpInstance->GetGrpCfg()->nGrp;
        AX_U32 nSrcWidth = m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicWidth;
        AX_U32 nSrcHeight =
            m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicHeight;
        AX_U32 nSrcOffset = 0;
        AX_U8 nRotation = AX_IVPS_ROTATION_0;
        AX_U8 nMirror = 0;
        if (AX_IVPS_ROTATION_90 == nRotation || AX_IVPS_ROTATION_270 == nRotation) {
            ::std::swap(nSrcWidth, nSrcHeight);
        }

        if (nMirror || AX_IVPS_ROTATION_180 == nRotation) {
            nSrcOffset = ALIGN_UP(nSrcWidth, ROTATION_WIDTH_ALIGEMENT) - nSrcHeight;
        }

        AX_U32 nFontSize = COSDStyle::GetInstance()->GetTimeFontSize(nSrcHeight);

        AX_U32 nMarginX = pThreadParam->tOsdCfg.nBoundaryX;
        AX_U32 nMarginY = pThreadParam->tOsdCfg.nBoundaryY;

        OSD_ALIGN_TYPE_E eAlign = pThreadParam->tOsdCfg.eAlign;
        AX_U32 nPicOffset = nMarginX % OSD_ALIGN_WIDTH;
        AX_U32 nPicOffsetBlock = nMarginX / OSD_ALIGN_WIDTH;
        AX_U32 nARGB = pThreadParam->tOsdCfg.tTimeAttr.nColor;

        AX_U32 nPixWidth = ALIGN_UP(nFontSize * nCharLen, BASE_FONT_SIZE);
        AX_U32 nPixHeight = ALIGN_UP(nFontSize, OSD_ALIGN_HEIGHT);
        nPixWidth = ALIGN_UP(nPixWidth + nPicOffset, OSD_ALIGN_WIDTH);
        AX_U32 nPicSize = nPixWidth * nPixHeight * 2;
        AX_U32 nFontColor = nARGB;
        nFontColor |= (1 << 24);
        AX_U32 nOffsetX = nSrcOffset + CCommonUtils::OverlayOffsetX(
                                           nSrcWidth, nPixWidth, (nPicOffset > 0 ? nPicOffsetBlock * OSD_ALIGN_WIDTH : nMarginX), eAlign);
        AX_U32 nOffsetY = CCommonUtils::OverlayOffsetY(nSrcHeight, nPixHeight, nMarginY, eAlign);
        pArgbData = (AX_U16*)malloc(nPicSize);
        LOG_MM_D(OSD, "PixWidth:%d, nPixHeight:%d, nOffsetX:%d, nOffsetY:%d, nCharLen:%d", nPixWidth, nPixHeight, nOffsetX, nOffsetY,
                 nCharLen);
        if (nullptr == m_osdWrapper.GenARGB(pThreadParamHandle, (wchar_t*)&wszOsdDate[0], (AX_U16*)pArgbData, nPixWidth, nPixHeight,
                                            nPicOffset, 0, nFontSize, AX_TRUE, nFontColor, 0xFFFFFF, 0xFF000000, eAlign)) {
            LOG_MM_E(OSD, "Failed to generate bitmap for date string.");
            m_osdWrapper.ReleaseInstance(&pThreadParamHandle);
            break;
        }
        tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
        tDisp.arrDisp[0].bShow = pThreadParam->tOsdCfg.bEnable;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = (AX_F32)(nARGB >> 24) / 0xFF * 1024;
        tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPixWidth;
        tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPixHeight;
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = nOffsetX;
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = nOffsetY;
        tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
        tDisp.arrDisp[0].uDisp.tOSD.pBitmap = (AX_U8*)pArgbData;

        AX_S32 ret = AX_IVPS_RGN_Update(pThreadParam->hRgn, &tDisp);
        if (AX_SUCCESS != ret) {
            LOG_MM_E(OSD, "AX_IVPS_RGN_Update fail, ret=0x%x, hRgn=%d", ret, pThreadParam->hRgn);
        }
        LOG_MM_D(OSD,
                 "[%d][%d] OSD (TIME):hRgn:%d bEnable:%d,  nSrcWidth:%d, nSrcHeight:%d, u32BmpWidth: %d, u32BmpHeight: %d, xOffset: %d, "
                 "yOffset: "
                 "%d, alpha: %d",
                 nIvpsGrp, nIvpsChn, pThreadParam->hRgn, tDisp.arrDisp[0].bShow, nSrcWidth, nSrcHeight,
                 tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth, tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight,
                 tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset, tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset,
                 tDisp.arrDisp[0].uDisp.tOSD.u16Alpha);
        m_osdWrapper.ReleaseInstance(&pThreadParamHandle);

        if (pArgbData) {
            free(pArgbData);
            pArgbData = nullptr;
        }
        CElapsedTimer::mSleep(1000);
    }
}

AX_VOID COSDHelper::RectThreadFunc(OSD_REGION_PARAM_T* pThreadParam) {
    while (!pThreadParam->bExit) {
        if (!m_pIvpsGrpInstance->IsEnabledChannel(pThreadParam->nChn)) {
            CElapsedTimer::mSleep(500);
            continue;
        }
        AX_IVPS_RGN_DISP_GROUP_T tDisp;
        memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
        tDisp.nNum = 1;
        tDisp.tChnAttr.nAlpha = 255;
        tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
        tDisp.tChnAttr.nZindex = pThreadParam->tOsdCfg.nZIndex;

        std::vector<AX_IVPS_RGN_POLYGON_T> stRgn;
        if (APP_OSD_RECT(pThreadParam->hRgn, stRgn)) {
            AX_U32 index = 0;
            tDisp.nNum = stRgn.size();

            for (; index < stRgn.size(); ++index) {
                tDisp.arrDisp[index].eType = AX_IVPS_RGN_TYPE_RECT;
                tDisp.arrDisp[index].bShow = pThreadParam->tOsdCfg.bEnable;
                tDisp.arrDisp[index].uDisp.tPolygon = stRgn[index];
            }
            AX_S32 ret = AX_IVPS_RGN_Update(pThreadParam->hRgn, &tDisp);
            if (AX_SUCCESS != ret) {
                LOG_MM_E(OSD, "AX_IVPS_RGN_Update fail, ret=0x%x, hRgn=%d", ret, pThreadParam->hRgn);
                CElapsedTimer::mSleep(1000);
            }
        }

        CElapsedTimer::mSleep(33);
    }
}

AX_VOID COSDHelper::UpdateOSDPic(OSD_REGION_PARAM_T* pThreadParam) {
    AX_U32 nIvpsChn = pThreadParam->nChn;
    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = pThreadParam->tOsdCfg.nZIndex;
    AX_U8 nRotation = AX_IVPS_ROTATION_0;

    AX_U8 nIvpsGrp = m_pIvpsGrpInstance->GetGrpCfg()->nGrp;
    AX_U32 nSrcWidth = m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicWidth;
    AX_U32 nSrcHeight = m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicHeight;

    if (AX_IVPS_ROTATION_90 == nRotation || AX_IVPS_ROTATION_270 == nRotation) {
        ::std::swap(nSrcWidth, nSrcHeight);
    }
    AX_U32 nSrcOffset = 0;

    AX_U32 nPicWidth = ALIGN_UP(pThreadParam->tOsdCfg.tPicAttr.nWidth, OSD_ALIGN_HEIGHT);
    AX_U32 nPicHeight = ALIGN_UP(pThreadParam->tOsdCfg.tPicAttr.nHeight, OSD_ALIGN_HEIGHT);

    /* Config picture OSD */
    AX_U32 nPicMarginX = pThreadParam->tOsdCfg.nBoundaryX;
    AX_U32 nPicMarginY = pThreadParam->tOsdCfg.nBoundaryY;

    OSD_ALIGN_TYPE_E eAlign = pThreadParam->tOsdCfg.eAlign;

    AX_U32 nSrcBlock = nSrcWidth / OSD_ALIGN_X_OFFSET;
    AX_U32 nGap = nSrcWidth % OSD_ALIGN_X_OFFSET;

    AX_U32 nBlockBollowed = ceil((AX_F32)(nPicWidth + nPicMarginX - nGap) / OSD_ALIGN_X_OFFSET);
    if (nBlockBollowed < 0) {
        nBlockBollowed = 0;
    }
    AX_U32 nOffsetX = nSrcOffset + (nSrcBlock - nBlockBollowed) * OSD_ALIGN_X_OFFSET;
    AX_U32 nOffsetY = CCommonUtils::OverlayOffsetY(nSrcHeight, nPicHeight, nPicMarginY, eAlign);
    if (AX_FALSE == CCommonUtils::LoadImage(pThreadParam->tOsdCfg.tPicAttr.szFileName, &tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr,
                                            (AX_VOID**)&tDisp.arrDisp[0].uDisp.tOSD.pBitmap, nPicWidth * nPicHeight * 2)) {
        LOG_MM_E(OSD, "Load logo(%s) failed.", pThreadParam->tOsdCfg.tPicAttr.szFileName);
        return;
    }

    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
    tDisp.arrDisp[0].bShow = pThreadParam->tOsdCfg.bEnable;
    tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
    tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = 50;
    tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = nOffsetX;
    tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = nOffsetY;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPicWidth;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPicHeight;
    AX_S32 ret = AX_IVPS_RGN_Update(pThreadParam->hRgn, &tDisp);
    if (AX_SUCCESS != ret) {
        LOG_MM_E(OSD, "AX_IVPS_RGN_Update fail, ret=0x%x, hRgn=%d", ret, pThreadParam->hRgn);
    }

    if (0 != tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr) {
        AX_SYS_MemFree(tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr, tDisp.arrDisp[0].uDisp.tOSD.pBitmap);
    }
    LOG_MM_I(OSD,
             "[%d][%d] rgn:%d , OSD(PICTURE): nSrcWidth:%d, nSrcHeight:%d, u32BmpWidth: %d, u32BmpHeight: %d, xOffset: %d, yOffset: %d, "
             "alpha: %d",
             nIvpsGrp, nIvpsChn, pThreadParam->hRgn, nSrcWidth, nSrcHeight, tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth,
             tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight, tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset, tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset,
             tDisp.arrDisp[0].uDisp.tOSD.u16Alpha);
    return;
}
AX_VOID COSDHelper::UpdateOSDStr(OSD_REGION_PARAM_T* pThreadParam) {
    AX_S32 nRet = -1;
    AX_U32 nIvpsChn = pThreadParam->nChn;
    COSDHandler* pThreadParamHandle = m_osdWrapper.NewInstance();
    if (nullptr == pThreadParamHandle) {
        LOG_MM_E(OSD, "Get osd handle failed.");
        return;
    }

    static AX_CHAR strTtfFile[128] = {0};
    sprintf(strTtfFile, "%s/GB2312.ttf", GetResPath().c_str());

    if (AX_FALSE == m_osdWrapper.InitHandler(pThreadParamHandle, strTtfFile)) {
        LOG_MM_E(OSD, "AX_OSDInitHandler failed, ttf: %s.", strTtfFile);
        m_osdWrapper.ReleaseInstance(&pThreadParamHandle);
        return;
    }

    AX_U16* pArgbData = nullptr;

    wchar_t wszOsdStr[MAX_OSD_WSTR_CHAR_LEN] = {0};
    memset(&wszOsdStr[0], 0, sizeof(wchar_t) * MAX_OSD_WSTR_CHAR_LEN);

    IVPS_GRP nIvpsGrp = pThreadParam->nGroup;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = pThreadParam->tOsdCfg.nZIndex;

    swprintf(&wszOsdStr[0], MAX_OSD_WSTR_CHAR_LEN, L"%s", pThreadParam->tOsdCfg.tStrAttr.szStr);

    AX_U8 nRotation = AX_IVPS_ROTATION_0;
    AX_U8 nMirror = 0;

    AX_U32 nSrcWidth = m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicWidth;
    AX_U32 nSrcHeight = m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicHeight;

    if (AX_IVPS_ROTATION_90 == nRotation || AX_IVPS_ROTATION_270 == nRotation) {
        ::std::swap(nSrcWidth, nSrcHeight);
    }

    AX_U32 nSrcOffsetX = 0;
    if (nMirror || AX_IVPS_ROTATION_180 == nRotation) {
        nSrcOffsetX = ALIGN_UP(nSrcWidth, ROTATION_WIDTH_ALIGEMENT) - nSrcWidth;
    }

    AX_U32 nFontSize = pThreadParam->tOsdCfg.tStrAttr.nFontSize;
    AX_U32 nMarginX = pThreadParam->tOsdCfg.nBoundaryX;
    AX_U32 nMarginY = pThreadParam->tOsdCfg.nBoundaryY;
    AX_U32 nPixWidth = pThreadParam->tOsdCfg.nBoundaryW;
    AX_U32 nPixHeight = pThreadParam->tOsdCfg.nBoundaryH;
    OSD_ALIGN_TYPE_E eAlign = pThreadParam->tOsdCfg.eAlign;

    AX_U32 nPicOffset = nMarginX % OSD_ALIGN_WIDTH;
    AX_U32 nPicOffsetBlock = nMarginX / OSD_ALIGN_WIDTH;

    // nPixWidth = ALIGN_UP(nFontSize/2, BASE_FONT_SIZE) * nCharLen;
    // nPixHeight = ALIGN_UP(nFontSize, BASE_FONT_SIZE);
    nPixWidth = ALIGN_UP(nPixWidth + nPicOffset, OSD_ALIGN_WIDTH);
    nPixHeight = ALIGN_UP(nPixHeight, OSD_ALIGN_HEIGHT);

    AX_U32 nPicSize = nPixWidth * nPixHeight * 2;
    AX_U32 nFontColor = pThreadParam->tOsdCfg.tStrAttr.nColor;
    nFontColor |= (1 << 24);

    pArgbData = (AX_U16*)malloc(nPicSize);
    if (nullptr == pArgbData) {
        LOG_MM_E(OSD, "no enough memory for pArgbData");
        return;
    }

    do {
        if (pThreadParam->tOsdCfg.bEnable) {
            if (nullptr == m_osdWrapper.GenARGB(pThreadParamHandle, (wchar_t*)&wszOsdStr[0], (AX_U16*)pArgbData, nPixWidth, nPixHeight,
                                            nPicOffset, 0, nFontSize, AX_TRUE, nFontColor, 0xFFFFFF, 0xFF000000, eAlign)) {
                LOG_MM_E(OSD, "[%d][%d]Failed to generate bitmap for string: %s.", nIvpsGrp, pThreadParam->tOsdCfg.nZIndex,
                        pThreadParam->tOsdCfg.tStrAttr.szStr);
                break;
            }
        }

        tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
        tDisp.arrDisp[0].bShow = pThreadParam->tOsdCfg.bEnable;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = (AX_F32)(nFontColor >> 24) / 0xFF * 1024;
        tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPixWidth;
        tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPixHeight;
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset =
            nSrcOffsetX +
            CCommonUtils::OverlayOffsetX(nSrcWidth, nPixWidth, (nPicOffset > 0 ? nPicOffsetBlock * OSD_ALIGN_WIDTH : nMarginX), eAlign);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = CCommonUtils::OverlayOffsetY(nSrcHeight, nPixHeight, nMarginY, eAlign);
        tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
        tDisp.arrDisp[0].uDisp.tOSD.pBitmap = (AX_U8*)pArgbData;

        LOG_M_I(OSD, "[%d] hRgn: %d, srcWidth: %d, srcHeight: %d, u32BmpWidth: %d, u32BmpHeight: %d, xOffset: %d, yOffset: %d, alpha: %d",
                nIvpsGrp, pThreadParam->hRgn, nSrcWidth, nSrcHeight, tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth,
                tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight, tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset,
                tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset, tDisp.arrDisp[0].uDisp.tOSD.u16Alpha);

        /* Region update */
        nRet = AX_IVPS_RGN_Update(pThreadParam->hRgn, &tDisp);
        if ((AX_SUCCESS != nRet) && (static_cast<int>(0x800d0229) != nRet)) {
            LOG_MM_E(OSD, "[%d][0x%02x] AX_IVPS_RGN_Update fail, ret=0x%x, handle=%d", nIvpsGrp, pThreadParam->nFilter, nRet,
                     pThreadParam->hRgn);
        }

    } while (0);

    /* Free osd resource */
    if (nullptr != pArgbData) {
        free(pArgbData);
        pArgbData = nullptr;
    }

    if (nullptr != pThreadParamHandle) {
        m_osdWrapper.ReleaseInstance(&pThreadParamHandle);
    }

    return;
}

AX_VOID COSDHelper::UpdateOSDPri(OSD_REGION_PARAM_T* pThreadParam) {
    AX_S32 nRet = -1;

    IVPS_GRP nIvpsGrp = pThreadParam->nGroup;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 200;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = pThreadParam->tOsdCfg.nZIndex;

    do {
        tDisp.arrDisp[0].eType = (AX_IVPS_RGN_TYPE_E)(pThreadParam->tOsdCfg.tPrivacyAttr.eType);
        tDisp.arrDisp[0].bShow = pThreadParam->tOsdCfg.bEnable;
        if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_LINE) {
            LOG_MM_E(OSD, "AX_IVPS_RGN_TYPE_LINE");
            tDisp.arrDisp[0].uDisp.tLine.nPointNum = 2;
            tDisp.arrDisp[0].uDisp.tLine.nLineWidth = pThreadParam->tOsdCfg.tPrivacyAttr.nLineWidth;
            tDisp.arrDisp[0].uDisp.tLine.nColor = pThreadParam->tOsdCfg.tPrivacyAttr.nColor;
            tDisp.arrDisp[0].uDisp.tLine.nAlpha = 200;
            tDisp.arrDisp[0].uDisp.tLine.tPTs[0].nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].x;
            tDisp.arrDisp[0].uDisp.tLine.tPTs[0].nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].y;
            tDisp.arrDisp[0].uDisp.tLine.tPTs[1].nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[1].x;
            tDisp.arrDisp[0].uDisp.tLine.tPTs[1].nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[1].y;
        } else if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_RECT) {
            tDisp.arrDisp[0].uDisp.tPolygon.nPointNum = 4;
            tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = 0;
            tDisp.arrDisp[0].uDisp.tPolygon.nColor = pThreadParam->tOsdCfg.tPrivacyAttr.nColor;
            tDisp.arrDisp[0].uDisp.tPolygon.nAlpha = 200;
            tDisp.arrDisp[0].uDisp.tPolygon.bSolid = pThreadParam->tOsdCfg.tPrivacyAttr.bSolid;

            if (pThreadParam->tOsdCfg.tPrivacyAttr.bSolid) {
                tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = 0;
            } else {
                tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = pThreadParam->tOsdCfg.tPrivacyAttr.nLineWidth;
            }

            tDisp.arrDisp[0].uDisp.tPolygon.tRect.nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].x;
            tDisp.arrDisp[0].uDisp.tPolygon.tRect.nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].y;
            tDisp.arrDisp[0].uDisp.tPolygon.tRect.nW =
                pThreadParam->tOsdCfg.tPrivacyAttr.tPt[2].x - pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].x;
            tDisp.arrDisp[0].uDisp.tPolygon.tRect.nH =
                pThreadParam->tOsdCfg.tPrivacyAttr.tPt[2].y - pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].y;
        } else if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_POLYGON) {
            tDisp.arrDisp[0].uDisp.tPolygon.nPointNum = 4;
            tDisp.arrDisp[0].uDisp.tPolygon.nColor = pThreadParam->tOsdCfg.tPrivacyAttr.nColor;
            tDisp.arrDisp[0].uDisp.tPolygon.nAlpha = 200;
            tDisp.arrDisp[0].uDisp.tPolygon.bSolid = pThreadParam->tOsdCfg.tPrivacyAttr.bSolid;
            if (pThreadParam->tOsdCfg.tPrivacyAttr.bSolid) {
                tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = 0;
            } else {
                tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = pThreadParam->tOsdCfg.tPrivacyAttr.nLineWidth;
            }

            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[0].nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].x;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[0].nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].y;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[1].nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[1].x;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[1].nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[1].y;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[2].nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[2].x;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[2].nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[2].y;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[3].nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[3].x;
            tDisp.arrDisp[0].uDisp.tPolygon.tPTs[3].nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[3].y;
        } else if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_MOSAIC) {
            tDisp.arrDisp[0].uDisp.tMosaic.eBklSize = (AX_IVPS_MOSAIC_BLK_SIZE_E)pThreadParam->tOsdCfg.tPrivacyAttr.nLineWidth;
            tDisp.arrDisp[0].uDisp.tMosaic.tRect.nX = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].x;
            tDisp.arrDisp[0].uDisp.tMosaic.tRect.nY = pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].y;
            tDisp.arrDisp[0].uDisp.tMosaic.tRect.nW =
                pThreadParam->tOsdCfg.tPrivacyAttr.tPt[2].x - pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].x;
            tDisp.arrDisp[0].uDisp.tMosaic.tRect.nH =
                pThreadParam->tOsdCfg.tPrivacyAttr.tPt[2].y - pThreadParam->tOsdCfg.tPrivacyAttr.tPt[0].y;
        }

        /* Region update */
        nRet = AX_IVPS_RGN_Update(pThreadParam->hRgn, &tDisp);
        if ((AX_SUCCESS != nRet) && (static_cast<int>(0x800d0229) != nRet)) {
            LOG_MM_E(OSD, "[%d][0x%02x] AX_IVPS_RGN_Update fail, ret=0x%x, handle=%d", nIvpsGrp, pThreadParam->nFilter, nRet,
                     pThreadParam->hRgn);
        }

    } while (0);

    return;
}

std::string COSDHelper::GetResPath() {
#ifdef SLT
    return "/opt/bin/res/frtdemo/res/";
#else
    return "./res/";
#endif
}

AX_BOOL COSDHelper::StopOSD() {
    LOG_MM_C(OSD, "+++");
    AX_S32 nRet = AX_SUCCESS;

    for (AX_U32 i = 0; i < m_nRgnCount; i++) {
        if (AX_IVPS_INVALID_REGION_HANDLE != m_arrRgnThreadParam[i].hRgn) {
            if (OSD_TYPE_TIME == m_arrRgnThreadParam[i].tOsdCfg.eType || OSD_TYPE_RECT == m_arrRgnThreadParam[i].tOsdCfg.eType) {
                m_arrRgnThreadParam[i].bExit = AX_TRUE;
                m_arrRgnThreadParam[i].m_EventThread.Join();
            }
            nRet = AX_IVPS_RGN_DetachFromFilter(m_arrRgnThreadParam[i].hRgn, m_arrRgnThreadParam[i].nGroup, m_arrRgnThreadParam[i].nFilter);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(OSD, "AX_IVPS_RGN_DetachFromFilter(Grp: %d, Filter: %x, Handle: %d) failed, ret=0x%x",
                         m_arrRgnThreadParam[i].nGroup, m_arrRgnThreadParam[i].nFilter, m_arrRgnThreadParam[i].hRgn, nRet);
                return AX_FALSE;
            } else {
                LOG_M_I(OSD, "AX_IVPS_RGN_DetachFromFilter(Grp: %d, Filter: %x, Handle: %d) successfully.", m_arrRgnThreadParam[i].nGroup,
                        m_arrRgnThreadParam[i].nFilter, m_arrRgnThreadParam[i].hRgn);
            }
            nRet = AX_IVPS_RGN_Destroy(m_arrRgnThreadParam[i].hRgn);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(OSD, "AX_IVPS_RGN_Destroy(Handle: %d) failed, ret=0x%x", m_arrRgnThreadParam[i].hRgn, nRet);
                return AX_FALSE;
            } else {
                LOG_M_I(OSD, "AX_IVPS_RGN_Destroy(Handle: %d) successfully.", m_arrRgnThreadParam[i].hRgn, nRet);
            }
            m_arrRgnThreadParam[i].hRgn = AX_IVPS_INVALID_REGION_HANDLE;
        }
    }
    LOG_MM_C(OSD, "---");
    return AX_TRUE;
}

AX_BOOL COSDHelper::Refresh() {
    IVPS_GROUP_CFG_T* pGrpConfig = m_pIvpsGrpInstance->GetGrpCfg();
    std::vector<OSD_CFG_T> vecOsdCfg;
    for (AX_U8 nChn = 0; nChn < pGrpConfig->nGrpChnNum; nChn++) {
        CWebOptionHelper::GetInstance()->GetOsdConfig(pGrpConfig->nSnsSrc, pGrpConfig->nGrp, nChn, vecOsdCfg);
        for (AX_U32 i = 0; i < vecOsdCfg.size(); i++) {
            OSD_TYPE_E eType = vecOsdCfg[i].eType;
            for (AX_U32 j = 0; j < m_nRgnCount; j++) {
                if (nChn == m_arrRgnThreadParam[j].nChn && m_arrRgnThreadParam[j].tOsdCfg.eType == eType && OSD_TYPE_RECT != eType) {
                    m_arrRgnThreadParam[j].tOsdCfg = vecOsdCfg[i];
                    if (OSD_TYPE_TIME != eType) {
                        UpdateOSD(&m_arrRgnThreadParam[j]);
                    }
                }
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL COSDHelper::EnableAiRegion(AX_BOOL bEnable) {
    for (AX_U32 j = 0; j < m_nRgnCount; j++) {
        if (m_arrRgnThreadParam[j].tOsdCfg.eType == OSD_TYPE_RECT) {
            m_arrRgnThreadParam[j].tOsdCfg.bEnable = bEnable;
        }
    }
    return AX_TRUE;
}

AX_U8 COSDHelper::GetAttachedFilter() {
    return 0;
}

AX_BOOL COSDHelper::UpdateOSDRect(const std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn) {
    for (AX_U32 i = 0; i < m_nRgnCount; ++i) {
        if (m_arrRgnThreadParam[i].tOsdCfg.eType == OSD_TYPE_RECT) {
            SET_APP_OSD_RECT(m_arrRgnThreadParam[i].hRgn, vecRgn);
        }
    }

    return AX_TRUE;
}

AX_BOOL COSDHelper::UpdateOSDRect(const std::vector<AX_APP_ALGO_BOX_T>& vecBox) {
    for (AX_U32 i = 0; i < m_nRgnCount; i++) {
        if (m_arrRgnThreadParam[i].tOsdCfg.eType == OSD_TYPE_RECT) {
            std::vector<AX_IVPS_RGN_POLYGON_T> vecRgn;
            AX_IVPS_RGN_POLYGON_T stPolygon;
            memset(&stPolygon, 0x00, sizeof(stPolygon));

            AX_U32 nIvpsChn = m_arrRgnThreadParam[i].nChn;

            AX_U32 nSrcWidth =
                m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicWidth;
            AX_U32 nSrcHeight =
                m_pIvpsGrpInstance->GetGrpPPLAttr()->tPipelineAttr.tFilter[nIvpsChn + 1][APP_OSD_CHANNEL_FILTER_1].nDstPicHeight;
            AX_U32 nRectLineWidth = COSDStyle::GetInstance()->GetRectLineWidth(nSrcHeight);

            stPolygon.bSolid = AX_TRUE;
            stPolygon.nLineWidth = nRectLineWidth;
            stPolygon.nColor = GREEN;
            stPolygon.nAlpha = 255;

            for (auto& tBox : vecBox) {
                stPolygon.tRect.nX = tBox.fX * nSrcWidth;
                stPolygon.tRect.nY = tBox.fY * nSrcHeight;
                stPolygon.tRect.nW = tBox.fW * nSrcWidth;
                stPolygon.tRect.nH = tBox.fH * nSrcHeight;

                if (stPolygon.tRect.nW == 0 || stPolygon.tRect.nH == 0) {
                    continue;
                }

                vecRgn.push_back(stPolygon);
            }

            SET_APP_OSD_RECT(m_arrRgnThreadParam[i].hRgn, vecRgn);
        }
    }

    return AX_TRUE;
}
