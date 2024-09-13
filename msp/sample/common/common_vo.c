/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "common_vo.h"

static AX_S32 SAMPLE_COMM_VO_StartWbc(VO_DEV VoDev, AX_VO_WBC_ATTR_T *pstWbcAttr)
{
    AX_S32 s32Ret = 0;

    s32Ret = AX_VO_SetWBCAttr(VoDev, pstWbcAttr);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!, dev%d\n", s32Ret, VoDev);
        return s32Ret;
    }

    s32Ret = AX_VO_EnableWBC(VoDev);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!, dev%d\n", s32Ret, VoDev);
        return s32Ret;
    }

    return s32Ret;
}

static AX_S32 SAMPLE_COMM_VO_StopWbc(VO_DEV VoDev)
{
    AX_S32 s32Ret = 0;

    s32Ret = AX_VO_DisableWBC(VoDev);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!, dev%d\n", s32Ret, VoDev);
        return s32Ret;
    }

    return s32Ret;
}

static AX_VO_SYNC_INFO_T g_stSyncInfos[SAMPLE_VO_SYNC_USER_MAX] = {
    /* 0 - 720x480@60 */
    {.u16Vact = 480, .u16Vbb = 30, .u16Vfb = 9, .u16Hact = 720, .u16Hbb = 57, .u16Hfb = 19, .u16Hpw = 62, .u16Vpw = 6, .u32Pclk = 27000, .bIdv = 1, .bIhs = 1, .bIvs = 1},
    /* 1 - 720x576@60 */
    {.u16Vact = 576, .u16Vbb = 14, .u16Vfb = 20, .u16Hact = 720, .u16Hbb = 60, .u16Hfb = 22, .u16Hpw = 62, .u16Vpw = 15, .u32Pclk = 27000, .bIdv = 1, .bIhs = 0, .bIvs = 0},
    /* 2 - 800x480@60 */
    {.u16Vact = 480, .u16Vbb = 29, .u16Vfb = 13, .u16Hact = 800, .u16Hbb = 40, .u16Hfb = 40, .u16Hpw = 48, .u16Vpw = 3, .u32Pclk = 29232, .bIdv = 1, .bIhs = 1, .bIvs = 1},
    /* 3 - 640x480@60 */
    {.u16Vact = 480, .u16Vbb = 23, .u16Vfb = 10, .u16Hact = 640, .u16Hbb = 60, .u16Hfb = 96, .u16Hpw = 62, .u16Vpw = 12, .u32Pclk = 27000, .bIdv = 1, .bIhs = 0, .bIvs = 0},
};

AX_S32 SAMPLE_COMM_VO_StartDev(SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf)
{
    AX_S32 s32Ret = 0;
    AX_VO_PUB_ATTR_T stVoPubAttr = {0};

    stVoPubAttr.enMode = pstVoDevConf->enMode;
    stVoPubAttr.enIntfType  = pstVoDevConf->enVoIntfType;
    stVoPubAttr.enIntfSync  = pstVoDevConf->enIntfSync;
    if (stVoPubAttr.enIntfSync == AX_VO_OUTPUT_USER) {
        stVoPubAttr.stSyncInfo = g_stSyncInfos[pstVoDevConf->u32SyncIndex];
    }

    stVoPubAttr.enIntfFmt  = pstVoDevConf->enVoOutfmt;

    if (pstVoDevConf->u32HdmiMode == VO_HDMI_FORCE_HDMI) {
        stVoPubAttr.stHdmiAttr.bEnableHdmi = AX_TRUE;
        stVoPubAttr.stHdmiAttr.enDefaultMode = AX_HDMI_FORCE_HDMI;
    } else if (pstVoDevConf->u32HdmiMode == VO_HDMI_FORCE_HDMI) {
        stVoPubAttr.stHdmiAttr.bEnableHdmi = AX_TRUE;
        stVoPubAttr.stHdmiAttr.enDefaultMode = AX_HDMI_FORCE_DVI;
    } else {
        stVoPubAttr.stHdmiAttr.bEnableHdmi = AX_FALSE;
    }

    stVoPubAttr.u32Flags = pstVoDevConf->u32Flags;

    s32Ret = AX_VO_SetPubAttr(pstVoDevConf->u32VoDev, &stVoPubAttr);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x, dev%d\n", s32Ret, pstVoDevConf->u32VoDev);
        return s32Ret;
    }

    s32Ret = AX_VO_Enable(pstVoDevConf->u32VoDev);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x, dev%d\n", s32Ret, pstVoDevConf->u32VoDev);
        return s32Ret;
    }

    if (pstVoDevConf->setCsc) {
        s32Ret = AX_VO_SetCSC(pstVoDevConf->u32VoDev, &pstVoDevConf->vo_csc);
        if (s32Ret) {
            SAMPLE_PRT("failed with %#x, dev%d\n", s32Ret, pstVoDevConf->u32VoDev);
            return s32Ret;
        }
    }

    if (pstVoDevConf->bWbcEn) {
        s32Ret = SAMPLE_COMM_VO_StartWbc(pstVoDevConf->u32VoDev, &pstVoDevConf->stWbcAttr);
        if (s32Ret) {
            SAMPLE_PRT("failed with %#x, dev%d\n", s32Ret, pstVoDevConf->u32VoDev);
            return s32Ret;
        }
    }

    pstVoDevConf->s32InitFlag = 1;

    return s32Ret;
}

AX_S32 SAMPLE_COMM_VO_StopDev(SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf)
{
    AX_S32 s32Ret = 0;

    if (pstVoDevConf->bWbcEn) {
        s32Ret = SAMPLE_COMM_VO_StopWbc(pstVoDevConf->u32VoDev);
        if (s32Ret) {
            SAMPLE_PRT("failed with %#x, dev%d\n", s32Ret, pstVoDevConf->u32VoDev);
            return s32Ret;
        }
    }

    s32Ret = AX_VO_Disable(pstVoDevConf->u32VoDev);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x, dev%d\n", s32Ret, pstVoDevConf->u32VoDev);
        return s32Ret;
    }

    pstVoDevConf->s32InitFlag = 0;

    return s32Ret;
}

static AX_S32 SAMPLE_COMM_VO_GLayerBind(SAMPLE_VO_GRAPHIC_CONFIG_S *pstGraphicLayerConf)
{
    AX_S32 i, s32Ret = 0;
    SAMPLE_FB_CONFIG_S *pstFbConf;

    if (pstGraphicLayerConf->u32FbNum) {
        AX_U32 u32Vrefresh;

        for (i = 0; i < pstGraphicLayerConf->u32FbNum; i++) {
            pstFbConf = &pstGraphicLayerConf->stFbConf[i];
            s32Ret = AX_VO_BindGraphicLayer(pstFbConf->u32Index, pstGraphicLayerConf->bindVoDev);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, GraphicLayer:%d, VoDev:%d\n", s32Ret, pstFbConf->u32Index,
                           pstGraphicLayerConf->bindVoDev);
                return s32Ret;
            }

            s32Ret = AX_VO_GetGraphicLayerVrefresh(pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index, &u32Vrefresh);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, VoDev:%d, GraphicLayer:%d\n", s32Ret, pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index);
                return s32Ret;
            }

	    SAMPLE_PRT("VoDev%d-GraphicLayer%d set before Vrefresh:%d\n", pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index, u32Vrefresh);

            s32Ret = AX_VO_SetGraphicLayerVrefresh(pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index, pstFbConf->u32Vrefresh);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, VoDev:%d, GraphicLayer:%d, Vrefresh:%d\n", s32Ret, pstGraphicLayerConf->bindVoDev,
                           pstFbConf->u32Index, pstFbConf->u32Vrefresh);
                return s32Ret;
            }

            s32Ret = AX_VO_GetGraphicLayerVrefresh(pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index, &u32Vrefresh);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, VoDev:%d, GraphicLayer:%d\n", s32Ret, pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index);
                return s32Ret;
            }

	    SAMPLE_PRT("VoDev%d-GraphicLayer%d set after Vrefresh:%d\n", pstGraphicLayerConf->bindVoDev, pstFbConf->u32Index, u32Vrefresh);
        }

        pstGraphicLayerConf->s32InitFlag = 1;
    }

    return s32Ret;
}

static AX_S32 SAMPLE_COMM_VO_GLayerUnBind(SAMPLE_VO_GRAPHIC_CONFIG_S *pstGraphicLayerConf)
{
    AX_S32 i, s32Ret = 0;

    if (pstGraphicLayerConf->s32InitFlag) {
        for (i = pstGraphicLayerConf->u32FbNum; i >= 0; i--) {
            s32Ret = AX_VO_UnBindGraphicLayer(pstGraphicLayerConf->stFbConf[i].u32Index, pstGraphicLayerConf->bindVoDev);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, GraphicLayer:%d, VoDev:%d\n", s32Ret, pstGraphicLayerConf->stFbConf[i].u32Index,
                           pstGraphicLayerConf->bindVoDev);
            }
        }
    }

    pstGraphicLayerConf->s32InitFlag = 0;

    return s32Ret;
}

static AX_S32 SAMPLE_COMM_VO_VLayerBind(SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf)
{
    AX_S32 i, s32Ret = 0;
    VO_DEV voDev;

    for (i = 0; i < SAMPLE_VO_DEV_MAX; i++) {
        voDev = pstVoLayerConf->bindVoDev[i];
        if (voDev < SAMPLE_VO_DEV_MAX) {
            s32Ret = AX_VO_BindVideoLayer(pstVoLayerConf->u32VoLayer, voDev);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, bindVoDev[%d]: %d\n", s32Ret, i, voDev);
                break;
            }
        }
    }

    return s32Ret;
}

static AX_S32 SAMPLE_COMM_VO_VLayerUnBind(SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf)
{
    AX_S32 i, s32Ret = 0;
    VO_DEV voDev;

    for (i = 0; i < SAMPLE_VO_DEV_MAX; i++) {
        voDev = pstVoLayerConf->bindVoDev[i];
        if (voDev < SAMPLE_VO_DEV_MAX) {
            s32Ret = AX_VO_UnBindVideoLayer(pstVoLayerConf->u32VoLayer, voDev);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, bindVoDev[%d]: %d\n", s32Ret, i, voDev);
                break;
            }
        }
    }

    return s32Ret;
}

AX_S32 SAMPLE_COMM_VO_StartLayer(SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf)
{
    AX_S32 s32Ret = 0;

    s32Ret = AX_VO_CreateVideoLayer(&pstVoLayerConf->u32VoLayer);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = AX_VO_SetVideoLayerAttr(pstVoLayerConf->u32VoLayer, &pstVoLayerConf->stVoLayerAttr);
    if (s32Ret) {
        AX_VO_DestroyVideoLayer(pstVoLayerConf->u32VoLayer);
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VO_VLayerBind(pstVoLayerConf);
    if (s32Ret) {
        SAMPLE_COMM_VO_VLayerUnBind(pstVoLayerConf);
        AX_VO_DestroyVideoLayer(pstVoLayerConf->u32VoLayer);
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = AX_VO_EnableVideoLayer(pstVoLayerConf->u32VoLayer);
    if (s32Ret) {
        SAMPLE_COMM_VO_VLayerUnBind(pstVoLayerConf);
        AX_VO_DestroyVideoLayer(pstVoLayerConf->u32VoLayer);
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    pstVoLayerConf->s32InitFlag = 1;

    return s32Ret;
}

AX_S32 SAMPLE_COMM_VO_StopLayer(SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf)
{
    AX_S32 s32Ret = 0;

    s32Ret = AX_VO_DisableVideoLayer(pstVoLayerConf->u32VoLayer);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VO_VLayerUnBind(pstVoLayerConf);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = AX_VO_DestroyVideoLayer(pstVoLayerConf->u32VoLayer);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    pstVoLayerConf->s32InitFlag = 0;

    return s32Ret;
}

AX_S32 SAMPLE_VO_WIN_INFO(AX_U32 u32LayerWidth, AX_U32 u32LayerHeight, SAMPLE_VO_MODE_E enMode, AX_U32 *u32Row,
                          AX_U32 *u32Col, AX_U32 *u32Width, AX_U32 *u32Height)
{
    switch (enMode) {
    case VO_MODE_1MUX:
        *u32Row = 1;
        *u32Col = 1;
        break;
    case VO_MODE_2MUX:
        *u32Row = 1;
        *u32Col = 2;
        break;
    case VO_MODE_4MUX:
        *u32Row = 2;
        *u32Col = 2;
        break;
    case VO_MODE_8MUX:
        *u32Row = 2;
        *u32Col = 4;
        break;
    case VO_MODE_9MUX:
        *u32Row = 3;
        *u32Col = 3;
        break;
    case VO_MODE_16MUX:
        *u32Row = 4;
        *u32Col = 4;
        break;
    case VO_MODE_25MUX:
        *u32Row = 5;
        *u32Col = 5;
        break;
    case VO_MODE_36MUX:
        *u32Row = 6;
        *u32Col = 6;
        break;
    case VO_MODE_49MUX:
        *u32Row = 7;
        *u32Col = 7;
        break;
    case VO_MODE_64MUX:
        *u32Row = 8;
        *u32Col = 8;
        break;
    default:
        *u32Row = 2;
        *u32Col = 2;
        break;
    }

    *u32Width   = ALIGN_DOWN(u32LayerWidth / *u32Col, 16);
    *u32Height  = ALIGN_DOWN(u32LayerHeight / *u32Row, 2);

    SAMPLE_PRT("Win-info: {%d, %d, %dx%d}\n", *u32Row, *u32Col, *u32Width, *u32Height);

    return 0;
}

AX_S32 SAMPLE_COMM_VO_StartChn(SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf)
{
    AX_S32 i, j, k;
    AX_S32 s32Ret = 0;
    AX_U32 u32Row = 0;
    AX_U32 u32Col = 0;
    AX_U32 u32LayerWidth, u32LayerHeight;
    AX_U32 u32ChnWidth, u32ChnHeight;
    SAMPLE_VO_MODE_E enMode;
    AX_VO_CHN_ATTR_T tChnAttr;
    AX_VO_VIDEO_LAYER_ATTR_T stLayerAttr;

    enMode = pstVoLayerConf->enVoMode;

    s32Ret = AX_VO_GetVideoLayerAttr(pstVoLayerConf->u32VoLayer, &stLayerAttr);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    u32LayerWidth  = stLayerAttr.stImageSize.u32Width;
    u32LayerHeight = stLayerAttr.stImageSize.u32Height;

    s32Ret = SAMPLE_VO_WIN_INFO(u32LayerWidth, u32LayerHeight, enMode, &u32Row, &u32Col,
                                &u32ChnWidth, &u32ChnHeight);
    if (s32Ret) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return s32Ret;
    }

    SAMPLE_PRT("layer%d u32Width:%d, u32Height:%d\n", pstVoLayerConf->u32VoLayer, u32LayerWidth, u32LayerHeight);
    for (i = 0; i < u32Row; i++) {
        memset(&tChnAttr, 0, sizeof(tChnAttr));
        tChnAttr.stRect.u32Width = u32ChnWidth;
        tChnAttr.stRect.u32Height = u32ChnHeight;
        tChnAttr.stRect.u32Y = ALIGN_DOWN(tChnAttr.stRect.u32Height * i, 2);
        for (j = 0; j < u32Col; j++) {
            k = i * u32Col + j;

            tChnAttr.stRect.u32X = ALIGN_DOWN(tChnAttr.stRect.u32Width * j, 16);
            tChnAttr.u32FifoDepth = pstVoLayerConf->u32FifoDepth;
            if (k < 64)
                tChnAttr.bKeepPrevFr = !!(pstVoLayerConf->u64KeepChnPrevFrameBitmap0 & (1UL << (k % 64))) ? AX_TRUE : AX_FALSE;
            else
                tChnAttr.bKeepPrevFr = !!(pstVoLayerConf->u64KeepChnPrevFrameBitmap1 & (1UL << (k % 64))) ? AX_TRUE : AX_FALSE;

            if (((pstVoLayerConf->u32ChnFrameOut - 1) == k) || ((k == 0) && pstVoLayerConf->bNeedChnCtrl))
                tChnAttr.bInUseFrOutput = AX_TRUE;

            s32Ret = AX_VO_SetChnAttr(pstVoLayerConf->u32VoLayer, k, &tChnAttr);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!\n", s32Ret);
                return s32Ret;
            }

            s32Ret = AX_VO_EnableChn(pstVoLayerConf->u32VoLayer, k);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!\n", s32Ret);
                return s32Ret;
            }
        }
    }

    return 0;
}

AX_S32 SAMPLE_COMM_VO_StopChn(SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf)
{
    AX_S32 i;
    AX_S32 s32Ret    = 0;
    AX_U32 u32WndNum = 0;

    switch (pstVoLayerConf->enVoMode) {
    case VO_MODE_1MUX:
        u32WndNum = 1;
        break;
    case VO_MODE_2MUX:
        u32WndNum = 2;
        break;
    case VO_MODE_4MUX:
        u32WndNum = 4;
        break;
    case VO_MODE_8MUX:
        u32WndNum = 8;
        break;
    case VO_MODE_9MUX:
        u32WndNum = 9;
        break;
    case VO_MODE_16MUX:
        u32WndNum = 16;
        break;
    case VO_MODE_25MUX:
        u32WndNum = 25;
        break;
    case VO_MODE_36MUX:
        u32WndNum = 36;
        break;
    case VO_MODE_49MUX:
        u32WndNum = 49;
        break;
    case VO_MODE_64MUX:
        u32WndNum = 64;
        break;
    default:
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return -1;
    }

    for (i = 0; i < u32WndNum; i++) {
        s32Ret = AX_VO_DisableChn(pstVoLayerConf->u32VoLayer, i);
        if (s32Ret) {
            SAMPLE_PRT("failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

AX_S32 SAMPLE_COMM_VO_StartVO(SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 i, s32Ret = 0;
    SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf;
    SAMPLE_VO_GRAPHIC_CONFIG_S *pstGraphicConf;
    SAMPLE_VO_CURSOR_CONFIG_S *pstCursorLayerConf;

    if (!pstVoConf) {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return -1;
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        pstGraphicConf = &pstVoConf->stGraphicLayer[i];

        /* Set and start vo dev */
        s32Ret = SAMPLE_COMM_VO_StartDev(pstVoDevConf);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_COMM_VO_StartDev failed\n");
            goto exit;
        }

        s32Ret = SAMPLE_COMM_VO_GLayerBind(pstGraphicConf);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_COMM_VO_GLayerBind failed\n");
            goto exit;
        }
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstGraphicConf = &pstVoConf->stGraphicLayer[i];

        /* Set and start vo layer */
        s32Ret = SAMPLE_COMM_VO_StartLayer(pstVoLayerConf);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_COMM_VO_StartLayer failed\n");
            SAMPLE_COMM_VO_GLayerUnBind(pstGraphicConf);
            goto exit;
        }

        /* Set and start channels on vo layer */
        s32Ret = SAMPLE_COMM_VO_StartChn(pstVoLayerConf);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed, i = %d\n", i);
            SAMPLE_COMM_VO_StopLayer(pstVoLayerConf);
            goto exit;
        }
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstCursorLayerConf = &pstVoConf->stCursorLayer[i];
        if (pstCursorLayerConf->u32CursorLayerEn) {
            s32Ret = AX_VO_BindGraphicLayer(pstCursorLayerConf->u32FBIndex, pstCursorLayerConf->bindVoDev);
            if (s32Ret) {
                SAMPLE_PRT("failed with %#x!, GraphicLayer:%d, VoDev:%d\n", s32Ret, pstCursorLayerConf->u32FBIndex,
                       pstCursorLayerConf->bindVoDev);
                goto exit;
            }

            pstCursorLayerConf->s32InitFlag = 1;
        }
    }

exit:
    if (s32Ret) {
        for (i = 0; i < pstVoConf->u32LayerNr; i++) {
            pstVoLayerConf = &pstVoConf->stVoLayer[i];

            if (pstVoLayerConf->s32InitFlag) {
                SAMPLE_COMM_VO_StopChn(pstVoLayerConf);
                SAMPLE_COMM_VO_StopLayer(pstVoLayerConf);
                pstVoLayerConf->s32InitFlag = 0;
            }
        }

        for (i = 0; i < pstVoConf->u32VDevNr; i++) {
            pstVoDevConf = &pstVoConf->stVoDev[i];
            pstGraphicConf = &pstVoConf->stGraphicLayer[i];

            SAMPLE_COMM_VO_GLayerUnBind(pstGraphicConf);
            if (pstVoDevConf->s32InitFlag) {
                SAMPLE_COMM_VO_StopDev(pstVoDevConf);
            }
        }
    }

    SAMPLE_PRT("done, s32Ret = 0x%x\n", s32Ret);

    return s32Ret;
}

AX_S32 SAMPLE_COMM_VO_StopVO(SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 i;
    SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf;
    SAMPLE_VO_GRAPHIC_CONFIG_S *pstGraphicConf;
    SAMPLE_VO_CURSOR_CONFIG_S *pstCursorLayerConf;

    if (!pstVoConf) {
        SAMPLE_PRT("Error:argument can not be NULL\n");
        return -1;
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstCursorLayerConf = &pstVoConf->stCursorLayer[i];
        if (pstCursorLayerConf->u32CursorLayerEn && pstCursorLayerConf->s32InitFlag)
            AX_VO_UnBindGraphicLayer(pstCursorLayerConf->u32FBIndex, pstCursorLayerConf->bindVoDev);
    }

    for (i = 0; i < SAMPLE_VO_DEV_MAX; i++) {
        pstGraphicConf = &pstVoConf->stGraphicLayer[i];
        SAMPLE_COMM_VO_GLayerUnBind(pstGraphicConf);
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];

        if (pstVoLayerConf->s32InitFlag) {
            SAMPLE_COMM_VO_StopChn(pstVoLayerConf);
            SAMPLE_COMM_VO_StopLayer(pstVoLayerConf);
            pstVoLayerConf->s32InitFlag = 0;
        }
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];

        if (pstVoDevConf->s32InitFlag) {
            SAMPLE_COMM_VO_StopDev(pstVoDevConf);
        }
    }

    return 0;
}

