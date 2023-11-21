/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "ax_base_type.h"
#include "common_vo.h"
#include "ax_vo_ini.h"

#define VO_INI_PATH "/opt/etc/vo.ini"
#define INI_LINE_LEN (256)

typedef struct SPLIT_STR {
    AX_U32 u32StrNr;
    AX_CHAR *pStr[SAMPLE_VO_DEV_MAX];
} SPLIT_STR_T;

struct ax_img_fmt_name {
    AX_IMG_FORMAT_E fmt;
    AX_CHAR *name;
};

static struct ax_img_fmt_name vo_img_fmt_str[] = {
    /* YUV400 8 bit */
    {AX_FORMAT_YUV400, "AX_FORMAT_YUV400"},

    /* YUV420 8 bit */
    {AX_FORMAT_YUV420_PLANAR, "AX_FORMAT_YUV420_PLANAR"},
    {AX_FORMAT_YUV420_PLANAR_VU, "AX_FORMAT_YUV420_PLANAR_VU"},
    {AX_FORMAT_YUV420_SEMIPLANAR, "AX_FORMAT_YUV420_SEMIPLANAR"},
    {AX_FORMAT_YUV420_SEMIPLANAR_VU, "AX_FORMAT_YUV420_SEMIPLANAR_VU"},

    /* YUV422 8 bit */
    {AX_FORMAT_YUV422_PLANAR, "AX_FORMAT_YUV422_PLANAR"},
    {AX_FORMAT_YUV422_PLANAR_VU, "AX_FORMAT_YUV422_PLANAR_VU"},
    {AX_FORMAT_YUV422_SEMIPLANAR, "AX_FORMAT_YUV422_SEMIPLANAR"},
    {AX_FORMAT_YUV422_SEMIPLANAR_VU, "AX_FORMAT_YUV422_SEMIPLANAR_VU"},
    {AX_FORMAT_YUV422_INTERLEAVED_YUVY, "AX_FORMAT_YUV422_INTERLEAVED_YUVY"},
    {AX_FORMAT_YUV422_INTERLEAVED_YUYV, "AX_FORMAT_YUV422_INTERLEAVED_YUYV"},
    {AX_FORMAT_YUV422_INTERLEAVED_UYVY, "AX_FORMAT_YUV422_INTERLEAVED_UYVY"},
    {AX_FORMAT_YUV422_INTERLEAVED_VYUY, "AX_FORMAT_YUV422_INTERLEAVED_VYUY"},
    {AX_FORMAT_YUV422_INTERLEAVED_YVYU, "AX_FORMAT_YUV422_INTERLEAVED_YVYU"},

    /* YUV444 8 bit */
    {AX_FORMAT_YUV444_PLANAR, "AX_FORMAT_YUV444_PLANAR"},
    {AX_FORMAT_YUV444_PLANAR_VU, "AX_FORMAT_YUV444_PLANAR_VU"},
    {AX_FORMAT_YUV444_SEMIPLANAR, "AX_FORMAT_YUV444_SEMIPLANAR"},
    {AX_FORMAT_YUV444_SEMIPLANAR_VU, "AX_FORMAT_YUV444_SEMIPLANAR_VU"},
    {AX_FORMAT_YUV444_PACKED, "AX_FORMAT_YUV444_PACKED"},

    /* YUV 10 bit */
    {AX_FORMAT_YUV400_10BIT, "AX_FORMAT_YUV400_10BIT"},
    {AX_FORMAT_YUV420_PLANAR_10BIT_UV_PACKED_4Y5B, "AX_FORMAT_YUV420_PLANAR_10BIT_UV_PACKED_4Y5B"},
    {AX_FORMAT_YUV420_PLANAR_10BIT_I010, "AX_FORMAT_YUV420_PLANAR_10BIT_I010"},
    {AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P101010, "AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P101010"},
    {AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010, "AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010"},
    {AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P016, "AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P016"},
    {AX_FORMAT_YUV420_SEMIPLANAR_10BIT_I016, "AX_FORMAT_YUV420_SEMIPLANAR_10BIT_I016"},
    {AX_FORMAT_YUV444_PACKED_10BIT_P010, "AX_FORMAT_YUV444_PACKED_10BIT_P010"},
    {AX_FORMAT_YUV444_PACKED_10BIT_P101010, "AX_FORMAT_YUV444_PACKED_10BIT_P101010"},

    /* BAYER RAW */
    {AX_FORMAT_BAYER_RAW_8BPP, "AX_FORMAT_BAYER_RAW_8BPP"},
    {AX_FORMAT_BAYER_RAW_10BPP, "AX_FORMAT_BAYER_RAW_10BPP"},
    {AX_FORMAT_BAYER_RAW_12BPP, "AX_FORMAT_BAYER_RAW_12BPP"},
    {AX_FORMAT_BAYER_RAW_14BPP, "AX_FORMAT_BAYER_RAW_14BPP"},
    {AX_FORMAT_BAYER_RAW_16BPP, "AX_FORMAT_BAYER_RAW_16BPP"},

    /* RGB Format */
    {AX_FORMAT_RGB565, "AX_FORMAT_RGB565"},
    {AX_FORMAT_RGB888, "AX_FORMAT_RGB888"},
    {AX_FORMAT_KRGB444, "AX_FORMAT_KRGB444"},
    {AX_FORMAT_KRGB555, "AX_FORMAT_KRGB555"},
    {AX_FORMAT_KRGB888, "AX_FORMAT_KRGB888"},
    {AX_FORMAT_BGR888, "AX_FORMAT_BGR888"},
    {AX_FORMAT_BGR565, "AX_FORMAT_BGR565"},

    {AX_FORMAT_ARGB4444, "AX_FORMAT_ARGB4444"},
    {AX_FORMAT_ARGB1555, "AX_FORMAT_ARGB1555"},
    {AX_FORMAT_ARGB8888, "AX_FORMAT_ARGB8888"},
    {AX_FORMAT_ARGB8565, "AX_FORMAT_ARGB8565"},
    {AX_FORMAT_RGBA8888, "AX_FORMAT_RGBA8888"},
    {AX_FORMAT_RGBA5551, "AX_FORMAT_RGBA5551"},
    {AX_FORMAT_RGBA4444, "AX_FORMAT_RGBA4444"},

    {AX_FORMAT_BITMAP, "AX_FORMAT_BITMAP"},
};

static AX_CHAR *vo_csc_matrix_str[AX_VO_CSC_MATRIX_BUTT] = {
    "VO_CSC_MATRIX_IDENTITY",
    "VO_CSC_MATRIX_BT601_TO_BT601",
    "VO_CSC_MATRIX_BT601_TO_BT709",
    "VO_CSC_MATRIX_BT709_TO_BT709",
    "VO_CSC_MATRIX_BT709_TO_BT601",
    "VO_CSC_MATRIX_BT601_TO_RGB_PC",
    "VO_CSC_MATRIX_BT709_TO_RGB_PC",
    "VO_CSC_MATRIX_RGB_TO_BT601_PC",
    "VO_CSC_MATRIX_RGB_TO_BT709_PC",
    "VO_CSC_MATRIX_RGB_TO_BT2020_PC",
    "VO_CSC_MATRIX_BT2020_TO_RGB_PC",
    "VO_CSC_MATRIX_RGB_TO_BT601_TV",
    "VO_CSC_MATRIX_RGB_TO_BT709_TV",
};

static AX_CHAR *vo_intf_sync_str[AX_VO_OUTPUT_BUTT] = {
    "VO_OUTPUT_576P50",
    "VO_OUTPUT_480P60",
    "VO_OUTPUT_720P50",
    "VO_OUTPUT_720P60",

    "VO_OUTPUT_1080P24",
    "VO_OUTPUT_1080P25",
    "VO_OUTPUT_1080P30",
    "VO_OUTPUT_1080P50",
    "VO_OUTPUT_1080P60",

    "VO_OUTPUT_640x480_60",
    "VO_OUTPUT_800x600_60",
    "VO_OUTPUT_1024x768_60",
    "VO_OUTPUT_1280x1024_60",
    "VO_OUTPUT_1366x768_60",
    "VO_OUTPUT_1280x800_60",
    "VO_OUTPUT_1440x900_60",
    "VO_OUTPUT_1600x1200_60",
    "VO_OUTPUT_1680x1050_60",
    "VO_OUTPUT_1920x1200_60",
    "VO_OUTPUT_2560x1600_60",

    "VO_OUTPUT_3840x2160_24",
    "VO_OUTPUT_3840x2160_25",
    "VO_OUTPUT_3840x2160_30",
    "VO_OUTPUT_3840x2160_50",
    "VO_OUTPUT_3840x2160_60",
    "VO_OUTPUT_4096x2160_24",
    "VO_OUTPUT_4096x2160_25",
    "VO_OUTPUT_4096x2160_30",
    "VO_OUTPUT_4096x2160_50",
    "VO_OUTPUT_4096x2160_60",

    "VO_OUTPUT_720x1280_60",
    "VO_OUTPUT_1080x1920_60",
    "VO_OUTPUT_USER",
};

static AX_CHAR *vo_intf_type_str[AX_VO_INTF_BUTT] = {
    "VO_INTF_DPI",
    "VO_INTF_BT601",
    "VO_INTF_BT656",
    "VO_INTF_BT1120",
    "VO_INTF_DSI",
    "VO_INTF_HDMI",
};

static AX_CHAR *vo_mode_str[AX_VO_MODE_BUTT] = {
    "VO_OFFLINE",
    "VO_ONLINE",
};

static AX_CHAR *vo_hdmi_mode[VO_HDMI_MODE_BUTT] = {
    "VO_HDMI_AUTO",
    "VO_HDMI_FORCE_HDMI",
    "VO_HDMI_FORCE_DVI",
};

static AX_CHAR *vo_intf_out_fmt[AX_VO_OUT_FMT_BUTT] = {
    [AX_VO_OUT_FMT_UNUSED] = "AX_VO_OUT_FMT_UNUSED",
    [AX_VO_OUT_FMT_RGB565] = "AX_VO_OUT_FMT_RGB565",
    [AX_VO_OUT_FMT_RGB666] = "AX_VO_OUT_FMT_RGB666",
    [AX_VO_OUT_FMT_RGB666LP] = "AX_VO_OUT_FMT_RGB666LP",
    [AX_VO_OUT_FMT_RGB888] = "AX_VO_OUT_FMT_RGB888",
    [AX_VO_OUT_FMT_RGB101010] = "AX_VO_OUT_FMT_RGB101010",
    [AX_VO_OUT_FMT_YUV422] = "AX_VO_OUT_FMT_YUV422",
    [AX_VO_OUT_FMT_YUV422_10] = "AX_VO_OUT_FMT_YUV422_10",
};

static AX_CHAR *vo_fbcdc_mode_str[AX_COMPRESS_MODE_BUTT] = {
    "AX_COMPRESS_MODE_NONE",
    "AX_COMPRESS_MODE_LOSSLESS",
    "AX_COMPRESS_MODE_LOSSY",
};

static AX_CHAR *layer_part_mode_str[AX_VO_PART_MODE_BUTT] = {
    "VO_PART_MODE_SINGLE",
    "VO_PART_MODE_MULTI",
};

static AX_CHAR *layer_VO_mode_str[VO_MODE_BUTT] = {
    "VO_MODE_1MUX",
    "VO_MODE_2MUX",
    "VO_MODE_4MUX",
    "VO_MODE_8MUX",
    "VO_MODE_9MUX",
    "VO_MODE_16MUX",
    "VO_MODE_25MUX",
    "VO_MODE_36MUX",
    "VO_MODE_49MUX",
    "VO_MODE_64MUX",
};

static AX_CHAR *layer_dispatch_mode_str[AX_VO_LAYER_DISPATCH_MODE_BUTT - 1] = {
    "VO_LAYER_OUT_TO_FIFO",
    "VO_LAYER_OUT_TO_LINK",
};

static AX_CHAR *layer_wb_mode_str[AX_VO_LAYER_WB_BUF_BUTT] = {
    "VO_LAYER_WB_POOL",
    "VO_LAYER_WB_INPLACE"
};

static AX_CHAR *layer_sync_mode_str[AX_VO_LAYER_SYNC_BUTT] = {
    "VO_LAYER_SYNC_NORMAL",
    "VO_LAYER_SYNC_SHUTTLE",
    "VO_LAYER_SYNC_GROUPING",
    "VO_LAYER_SYNC_PRIMARY"
};

static AX_CHAR *wbc_src_str[AX_VO_WBC_SOURCE_BUTT] = {
    "VO_WBC_SOURCE_DEV",
    "VO_WBC_SOURCE_VIDEO"
};

static AX_CHAR *wbc_mode_str[AX_VO_WBC_MODE_BUTT] = {
    "VO_WBC_MODE_NORMAL",
    "VO_WBC_MODE_DROP_REPEAT"
};

static AX_CHAR *section_type_str[AX_VO_INI_S_BUTT] = {
    "layer",
    "layer_display",
};

static AX_CHAR *key_str[AX_VO_INI_K_BUTT] = {
    "sectionType",
    "VoDev",

    /* channel keys */
    "chn_vo_mode",
    "chn_fbdc_mode",
    "chn_fbdc_level",
    "chn_file_name",
    "chn_frame_rate",
    "chn_frame_nr",

    /* layer keys */
    "layer_disp_X",
    "layer_disp_Y",
    "layer_disp_width",
    "layer_disp_height",
    "layer_img_width",
    "layer_img_height",
    "layer_img_fmt",
    "layer_sync_sode",
    "layer_primary_chnId",
    "layer_frame_rate",
    "layer_fifo_depth",
    "layer_BkClr",
    "layer_WB_mode",
    "layer_inplace_chnId",
    "layer_dispatch_mode",
    "layer_keepChnPrevFrameBitmap0",
    "layer_keepChnPrevFrameBitmap1",
    "layer_part_mode",
    "layer_engine_id",
    "layer_bind_mode",
    "layer_fbc_mode",
    "layer_fbc_level",
    "layer_chn_frame_out",
    "layer_chn_ctrl",

    /* display keys */
    "disp_work_mode",
    "disp_interface_type",
    "disp_sync_type",
    "disp_sync_user_index",
    "disp_hdmi_mode",
    "disp_wbc_enable",
    "disp_wbc_type",
    "disp_wbc_mode",
    "disp_wbc_frame_rate",
    "disp_wbc_frame_nr",
    "disp_graphic_fb_conf", /* fbx:wxh@fmt@colorkey */
    "disp_csc_matrix",
    "disp_luma",
    "disp_contrast",
    "disp_hue",
    "disp_satuature",
    "disp_cursor_enable",
    "disp_cursor_x",
    "disp_cursor_y",
    "disp_cursor_width",
    "disp_cursor_height",
    "disp_cursor_fb_index",
    "disp_cursor_move",
    "disp_out_fmt",
};

static AX_S32 VO_INI_FB_CONF_PARSE(AX_CHAR *pStr, SAMPLE_FB_CONFIG_S *pstFbConf)
{
    AX_U32 u32StrNr = 0;
    AX_CHAR *pStrSplit[4] = {NULL, NULL, NULL, NULL};
    AX_CHAR *pStrKey = ":@", *pStrTmp, *pSaveStr;

    memset(pstFbConf, 0, sizeof(*pstFbConf));

    pStrTmp = strtok_r(pStr, pStrKey, &pSaveStr);
    while (pStrTmp && u32StrNr < 4) {
        SAMPLE_PRT("split:%s\n", pStrTmp);
        pStrSplit[u32StrNr++] = pStrTmp;
        pStrTmp = strtok_r(NULL, pStrKey, &pSaveStr);
    }

    if (u32StrNr < 3) {
        SAMPLE_PRT("fb config invalid, u32StrNr:%d\n", u32StrNr);
        return -1;
    }

    if (strlen(pStrSplit[0]) > 2) {
        pstFbConf->u32Index = strtoul(&pStrSplit[0][2], NULL, 10);
    } else {
        SAMPLE_PRT("fb index-str(%s) invalid\n", pStrSplit[0]);
        return -1;
    }

    pstFbConf->u32ResoW = strtoul(pStrSplit[1], &pStrTmp, 10);
    if (!pStrTmp) {
        SAMPLE_PRT("fb reso-str(%s) invalid\n", pStrSplit[1]);
        return -1;
    }
    pStrTmp++;
    pstFbConf->u32ResoH = strtoul(pStrTmp, NULL, 10);

    if (strstr(pStrSplit[2], "argb8888")) {
        pstFbConf->u32Fmt = AX_FORMAT_ARGB8888;
    } else if (strstr(pStrSplit[2], "argb1555")) {
        pstFbConf->u32Fmt = AX_FORMAT_ARGB1555;
    } else if (strstr(pStrSplit[2], "rgba8888")) {
        pstFbConf->u32Fmt = AX_FORMAT_RGBA8888;
    } else if (strstr(pStrSplit[2], "rgba5551")) {
        pstFbConf->u32Fmt = AX_FORMAT_RGBA5551;
    } else {
        SAMPLE_PRT("fb fmt-str(%s) invalid\n", pStrSplit[2]);
        return -1;
    }

    if (u32StrNr > 3 && strlen(pStrSplit[3]) > 2) {
        pstFbConf->u32ColorKey = strtoul(&pStrSplit[3][2], &pStrTmp, 16);
        if (pStrTmp) {
            pStrTmp++;
            pstFbConf->u32ColorKeyInv = strtoul(pStrTmp, NULL, 10) ? 1 : 0;
        }
        pstFbConf->u32ColorKeyEn = 1;
    }

    SAMPLE_PRT("fb%d reso:%dx%d fmt:0x%x, ck:0x%x-%d-%d\n", pstFbConf->u32Index,
               pstFbConf->u32ResoW, pstFbConf->u32ResoH, pstFbConf->u32Fmt,
               pstFbConf->u32ColorKey, pstFbConf->u32ColorKeyInv, pstFbConf->u32ColorKeyEn);

    return 0;
}

static AX_VOID VO_INI_GRAPHIC_PARSE(AX_CHAR *pStr, SAMPLE_VO_GRAPHIC_CONFIG_S *pstGraphConf)
{
    AX_U32 i, u32StrNr = 0, u32FbMax;
    AX_CHAR *pStrSplit[4] = {NULL, NULL, NULL, NULL};
    AX_CHAR *pStrKey = "-", *pStrTmp, *pSaveStr;

    pStrTmp = strtok_r(pStr, pStrKey, &pSaveStr);
    while (pStrTmp && u32StrNr < 4) {
        /* SAMPLE_PRT("split:%s\n", pStrTmp); */
        pStrSplit[u32StrNr++] = pStrTmp;
        pStrTmp = strtok_r(NULL, pStrKey, &pSaveStr);
    }

    u32FbMax = u32StrNr > SAMPLE_FB_PER_DEV_MAX ? SAMPLE_FB_PER_DEV_MAX : u32StrNr;

    pstGraphConf->u32FbNum = 0;

    for (i = 0; i < u32FbMax; i++) {
        if (!VO_INI_FB_CONF_PARSE(pStrSplit[i], &pstGraphConf->stFbConf[pstGraphConf->u32FbNum])) {
            pstGraphConf->u32FbNum += 1;
        }
    }
}

static AX_VOID VO_INI_KVAL_SPLIT(AX_CHAR *pStr, SPLIT_STR_T *pstSplitStr)
{
    AX_CHAR *pStrTmp = NULL, *pSaveStr = NULL;

    memset(pstSplitStr, 0, sizeof(*pstSplitStr));

    pStrTmp = strtok_r(pStr, " ,", &pSaveStr);
    while (pStrTmp && pstSplitStr->u32StrNr <= SAMPLE_VO_DEV_MAX) {
        /* SAMPLE_PRT("split:%s\n", pStrTmp); */
        pstSplitStr->pStr[pstSplitStr->u32StrNr++] = pStrTmp;
        pStrTmp = strtok_r(NULL, " ,", &pSaveStr);
    }
}

static AX_S32 VO_INI_GET_KEY_VAL(AX_CHAR *pstr, AX_VOID *pConfig,
                                 AX_VO_INI_SECTION_E type, AX_VO_INI_KEY_E key)
{
    SAMPLE_VO_CONFIG_S *pVoConfig = AX_NULL;
    SAMPLE_VO_LAYER_CONFIG_S *pLayerConf = AX_NULL;
    AX_U32 u32ValNr;
    AX_CHAR *tmp_str;
    AX_S32 i, ret = AX_SUCCESS;
    SPLIT_STR_T stSplitStr;

    if (type == AX_VO_INI_S_LAYER && key > AX_VO_INI_K_L_MAX) {
        SAMPLE_PRT("section type:%d, key:%d is invalid.\n", type, key)
        return -1;
    }

    while (*pstr && isspace(*pstr)) {
        pstr++;
    }

    /* replace \r \n to 0. */
    tmp_str = strrchr(pstr, '\r');
    if (tmp_str != AX_NULL) {
        *tmp_str = 0;
    }

    tmp_str = strrchr(pstr, '\n');
    if (tmp_str != AX_NULL) {
        *tmp_str = 0;
    }

    VO_INI_KVAL_SPLIT(pstr, &stSplitStr);

    switch (key) {
    case AX_VO_INI_K_VO_DEVICE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                pVoConfig->stVoDev[i].u32VoDev = strtoul(stSplitStr.pStr[i], NULL, 0);
            }

            pVoConfig->u32VDevNr = stSplitStr.u32StrNr;
        }
        break;

    case AX_VO_INI_K_C_VO_MODE: {
        SAMPLE_VO_MODE_E mode;

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (mode = VO_MODE_1MUX; mode < VO_MODE_BUTT; mode++) {
                if (!strcmp(stSplitStr.pStr[i], layer_VO_mode_str[mode])) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                        pVoConfig->stVoLayer[i].enVoMode = mode;
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        pLayerConf->enVoMode = mode;
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_C_FILE_NAME:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                strncpy(pLayerConf->chnFileName, stSplitStr.pStr[i], VO_NAME_LEN - 1);
                pLayerConf->chnFileName[VO_NAME_LEN - 1] = 0;
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            strncpy(pLayerConf->chnFileName, stSplitStr.pStr[0], VO_NAME_LEN - 1);
            pLayerConf->chnFileName[VO_NAME_LEN - 1] = 0;
        }
        break;

    case AX_VO_INI_K_C_FRAME_RATE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->u32ChnFrameRate = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->u32ChnFrameRate = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_C_FRAME_NR:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->u32ChnFrameNr = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->u32ChnFrameNr = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_DISP_X:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.stDispRect.u32X = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.stDispRect.u32X = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_DISP_Y:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.stDispRect.u32Y = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.stDispRect.u32Y = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_DISP_WIDTH:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.stDispRect.u32Width = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.stDispRect.u32Width = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_DISP_HEIGHT:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.stDispRect.u32Height = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.stDispRect.u32Height = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_IMG_WIDTH:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.stImageSize.u32Width = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.stImageSize.u32Width = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_IMG_HEIGHT:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.stImageSize.u32Height = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.stImageSize.u32Height = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_IMG_FMT: {
        AX_S32 index, size = sizeof(vo_img_fmt_str) / sizeof(vo_img_fmt_str[0]);

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (index = 0; index < size; index++) {
                if (!strcmp(stSplitStr.pStr[i], vo_img_fmt_str[index].name)) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                        pVoConfig->stVoLayer[i].stVoLayerAttr.enPixFmt = vo_img_fmt_str[index].fmt;
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        pLayerConf->stVoLayerAttr.enPixFmt = vo_img_fmt_str[index].fmt;
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_L_SYNC_MODE: {
        AX_VO_LAYER_SYNC_MODE_E mode;

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (mode = AX_VO_LAYER_SYNC_NORMAL; mode < AX_VO_LAYER_SYNC_BUTT; mode++) {
                if (!strcmp(stSplitStr.pStr[i], layer_sync_mode_str[mode])) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                        pVoConfig->stVoLayer[i].stVoLayerAttr.enSyncMode = mode;
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        pLayerConf->stVoLayerAttr.enSyncMode = mode;
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_L_PRIMERY_CHNID:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.u32PrimaryChnId = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.u32PrimaryChnId = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_FRAME_RATE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.f32FrmRate = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.f32FrmRate = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_FIFO_DEPTH:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.u32FifoDepth = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.u32FifoDepth = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_BKCLR:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.u32BkClr = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.u32BkClr = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_WB_MODE: {
        AX_VO_LAYER_WB_MODE_E mode;

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (mode = AX_VO_LAYER_WB_POOL; mode < AX_VO_LAYER_WB_BUF_BUTT; mode++) {
                if (!strcmp(stSplitStr.pStr[i], layer_wb_mode_str[mode])) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                        pVoConfig->stVoLayer[i].stVoLayerAttr.enWBMode = mode;
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        pLayerConf->stVoLayerAttr.enWBMode = mode;
                    }
                }
            }
        }

        break;
    }

    case AX_VO_INI_K_L_INPLACE_CHNID:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->stVoLayerAttr.u32InplaceChnId = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->stVoLayerAttr.u32InplaceChnId = strtoul(stSplitStr.pStr[0], NULL, 0);
        }
        break;

    case AX_VO_INI_K_L_DISPATCH_MODE: {
        AX_VO_LAYER_DISPATCH_MODE_E mode;

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (mode = AX_VO_LAYER_OUT_TO_FIFO; mode < AX_VO_LAYER_DISPATCH_MODE_BUTT; mode++) {
                if (!strcmp(stSplitStr.pStr[i], layer_dispatch_mode_str[mode - 1])) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                        pVoConfig->stVoLayer[i].stVoLayerAttr.u32DispatchMode = mode;
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        pLayerConf->stVoLayerAttr.u32DispatchMode = mode;
                    }
                }
            }
        }

        break;
    }

    case AX_VO_INI_K_L_KEEP_PREV_CHNS_BITMAP0:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->u64KeepChnPrevFrameBitmap0 = strtoul(stSplitStr.pStr[i], NULL, 16);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->u64KeepChnPrevFrameBitmap0 = strtoul(stSplitStr.pStr[0], NULL, 16);
        }
        break;

    case AX_VO_INI_K_L_KEEP_PREV_CHNS_BITMAP1:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->u64KeepChnPrevFrameBitmap1 = strtoul(stSplitStr.pStr[i], NULL, 16);
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->u64KeepChnPrevFrameBitmap1 = strtoul(stSplitStr.pStr[0], NULL, 16);
        }
        break;

    case AX_VO_INI_K_L_PART_MODE: {
        AX_VO_PART_MODE_E mode;

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (mode = AX_VO_PART_MODE_SINGLE; mode < AX_VO_PART_MODE_BUTT; mode++) {
                if (!strcmp(stSplitStr.pStr[i], layer_part_mode_str[mode])) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                        pVoConfig->stVoLayer[i].stVoLayerAttr.enPartMode = mode;
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        pLayerConf->stVoLayerAttr.enPartMode = mode;
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_L_ENGINE_ID: {
        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                pVoConfig->stVoLayer[i].stVoLayerAttr.enEngineMode = AX_VO_ENGINE_MODE_FORCE;
                pVoConfig->stVoLayer[i].stVoLayerAttr.u32EngineId = strtoul(stSplitStr.pStr[i], NULL, 10);
            } else {
                pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                pLayerConf->stVoLayerAttr.enEngineMode = AX_VO_ENGINE_MODE_FORCE;
                pLayerConf->stVoLayerAttr.u32EngineId = strtoul(stSplitStr.pStr[0], NULL, 10);
            }
        }
        break;
    }

    case AX_VO_INI_K_L_BIND_MODE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            pVoConfig->u32BindMode = strtoul(stSplitStr.pStr[0], NULL, 0) ? 1 : 0;
        }
        break;

    case AX_VO_INI_K_L_FBC_MODE:
    case AX_VO_INI_K_C_FBDC_MODE: {
        AX_COMPRESS_MODE_E mode;

        u32ValNr = (type == AX_VO_INI_S_LAYER_DISPLAY) ? stSplitStr.u32StrNr : 1;

        for (i = 0; i < u32ValNr; i++) {
            for (mode = AX_COMPRESS_MODE_NONE; mode < AX_COMPRESS_MODE_BUTT; mode++) {
                if (!strcmp(stSplitStr.pStr[i], vo_fbcdc_mode_str[mode])) {
                    if (type == AX_VO_INI_S_LAYER_DISPLAY) {
                        if (key == AX_VO_INI_K_C_FBDC_MODE) {
                            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                            pVoConfig->stVoLayer[i].chnCompressInfo.enCompressMode = mode;
                        }
                    } else {
                        pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
                        if (key == AX_VO_INI_K_L_FBC_MODE) {
                            pLayerConf->stVoLayerAttr.stCompressInfo.enCompressMode = mode;
                        } else {
                            pLayerConf->chnCompressInfo.enCompressMode = mode;
                        }
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_L_FBC_LEVEL:
    case AX_VO_INI_K_C_FBDC_LEVEL:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            if (key == AX_VO_INI_K_C_FBDC_LEVEL) {
                pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
                u32ValNr = stSplitStr.u32StrNr;
                for (i = 0; i < u32ValNr; i++) {
                    pLayerConf = &pVoConfig->stVoLayer[i];
                    pLayerConf->chnCompressInfo.u32CompressLevel = strtoul(stSplitStr.pStr[i], NULL, 0);
                }
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            if (key == AX_VO_INI_K_C_FBDC_LEVEL) {
                pLayerConf->chnCompressInfo.u32CompressLevel = strtoul(stSplitStr.pStr[0], NULL, 0);
            } else {
                pLayerConf->stVoLayerAttr.stCompressInfo.u32CompressLevel = strtoul(stSplitStr.pStr[0], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_L_CHN_FRAME_OUT:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->u32ChnFrameOut = strtoul(stSplitStr.pStr[i], NULL, 0) + 1;
            }
        } else {
            pLayerConf = (SAMPLE_VO_LAYER_CONFIG_S *)pConfig;
            pLayerConf->u32ChnFrameOut = strtoul(stSplitStr.pStr[0], NULL, 0) + 1;
        }
        break;

    case AX_VO_INI_K_L_CHN_CTRL:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pLayerConf = &pVoConfig->stVoLayer[i];
                pLayerConf->bNeedChnCtrl = strtoul(stSplitStr.pStr[i], NULL, 0) ? AX_TRUE : AX_FALSE;
            }
        }
        break;

    case AX_VO_INI_K_D_WORK_MODE: {
        AX_U32 u32Mode;

        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                for (u32Mode = AX_VO_MODE_OFFLINE; u32Mode < AX_VO_MODE_BUTT; u32Mode++) {
                    if (!strcmp(stSplitStr.pStr[i], vo_mode_str[u32Mode])) {
                        pVoConfig->stVoDev[i].enMode = u32Mode;
                    }
                }
            }
        }

        break;
    }

    case AX_VO_INI_K_D_INTERFACE: {
        AX_VO_INTF_TYPE_E mode;

        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                for (mode = AX_VO_INTF_DPI; mode < AX_VO_INTF_BUTT; mode++) {
                    if (!strcmp(stSplitStr.pStr[i], vo_intf_type_str[mode])) {
                        pVoConfig->stVoDev[i].enVoIntfType = mode;
                    }
                }
            }
        }

        break;
    }

    case AX_VO_INI_K_D_SYNC_TYPE: {
        AX_VO_INTF_SYNC_E sync;

        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                for (sync = AX_VO_OUTPUT_576P50; sync < AX_VO_OUTPUT_BUTT; sync++) {
                    if (!strcmp(stSplitStr.pStr[i], vo_intf_sync_str[sync])) {
                        pVoConfig->stVoDev[i].enIntfSync = sync;
                        break;
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_D_SYNC_USER_INDEX: {
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                pVoConfig->stVoDev[i].u32SyncIndex = strtoul(stSplitStr.pStr[i], NULL, 0);
                if (pVoConfig->stVoDev[i].u32SyncIndex >= SAMPLE_VO_SYNC_USER_MAX) {
                    SAMPLE_PRT("VoDev[%d] u32SyncIndex(%d) invalid\n", i, pVoConfig->stVoDev[i].u32SyncIndex);
                    ret = -1;
                    break;
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_D_HDMI_MODE: {
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            AX_U32 u32HdmiMode;

            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                for (u32HdmiMode = VO_HDMI_AUTO; u32HdmiMode < VO_HDMI_MODE_BUTT; u32HdmiMode++) {
                    if (!strcmp(stSplitStr.pStr[i], vo_hdmi_mode[u32HdmiMode])) {
                        pVoConfig->stVoDev[i].u32HdmiMode = u32HdmiMode;
                        break;
                    }
                }
            }
        }
        break;
    }

    case AX_VO_INI_K_D_WBC_ENABLE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].bWbcEn = strtoul(stSplitStr.pStr[i], NULL, 0) ? AX_TRUE : AX_FALSE;
            }
        }
        break;

    case AX_VO_INI_K_D_WBC_TYPE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            AX_VO_WBC_SOURCE_TYPE_E enSrc;

            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                for (enSrc = AX_VO_WBC_SOURCE_DEV; enSrc < AX_VO_WBC_SOURCE_BUTT; enSrc++) {
                    if (!strcmp(stSplitStr.pStr[i], wbc_src_str[enSrc])) {
                        pVoConfig->stVoDev[i].stWbcAttr.enSourceType = enSrc;
                    }
                }
            }
        }
        break;

    case AX_VO_INI_K_D_WBC_MODE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            AX_VO_WBC_MODE_E enMode;

            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                for (enMode = AX_VO_WBC_MODE_NORMAL; enMode < AX_VO_WBC_MODE_BUTT; enMode++) {
                    if (!strcmp(stSplitStr.pStr[i], wbc_mode_str[enMode])) {
                        pVoConfig->stVoDev[i].stWbcAttr.enMode = enMode;
                    }
                }
            }
        }
        break;

    case AX_VO_INI_K_D_WBC_FRAME_RATE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].stWbcAttr.f32FrameRate = (AX_F32)strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_D_WBC_FRAME_NR:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].u32WbcFrmaeNr = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;
    case AX_VO_INI_K_D_GRAPHIC_FB_CONF:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            SAMPLE_PRT("u32ValNr:%d\n", u32ValNr);
            for (i = 0; i < u32ValNr; i++) {
                SAMPLE_PRT("%s\n", stSplitStr.pStr[i]);
                VO_INI_GRAPHIC_PARSE(stSplitStr.pStr[i], &pVoConfig->stGraphicLayer[i]);
            }
        }
        break;
    case AX_VO_INI_K_D_CSC_MATRIX:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            AX_VO_CSC_MATRIX_E csc_matrix;

            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                for (csc_matrix = AX_VO_CSC_MATRIX_IDENTITY; csc_matrix < AX_VO_CSC_MATRIX_BUTT; csc_matrix++) {
                    if (!strcmp(stSplitStr.pStr[i], vo_csc_matrix_str[csc_matrix])) {
                        pVoConfig->stVoDev[i].vo_csc.enCscMatrix = csc_matrix;
                        pVoConfig->stVoDev[i].setCsc = true;
                    }
                }
            }
        }
        break;

    case AX_VO_INI_K_D_LUMA:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].vo_csc.u32Luma = strtoul(stSplitStr.pStr[i], NULL, 0);
                pVoConfig->stVoDev[i].setCsc = true;
            }
        }
        break;

    case AX_VO_INI_K_D_CONTRAST:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].vo_csc.u32Contrast = strtoul(stSplitStr.pStr[i], NULL, 0);
                pVoConfig->stVoDev[i].setCsc = true;
            }
        }
        break;

    case AX_VO_INI_K_D_HUE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].vo_csc.u32Hue = strtoul(stSplitStr.pStr[i], NULL, 0);
                pVoConfig->stVoDev[i].setCsc = true;
            }
        }
        break;

    case AX_VO_INI_K_D_SATUATURE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stVoDev[i].vo_csc.u32Satuature = strtoul(stSplitStr.pStr[i], NULL, 0);
                pVoConfig->stVoDev[i].setCsc = true;
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_ENABLE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32CursorLayerEn = strtoul(stSplitStr.pStr[i], NULL, 0) ? 1 : 0;
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_X:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32X = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_Y:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32Y = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_WIDTH:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32Width = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_HEIGHT:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32Height = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_FBID:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32FBIndex = strtoul(stSplitStr.pStr[i], NULL, 0);
            }
        }
        break;

    case AX_VO_INI_K_D_CURSOR_MOVE:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;
            u32ValNr = stSplitStr.u32StrNr;
            for (i = 0; i < u32ValNr; i++) {
                pVoConfig->stCursorLayer[i].u32CursorMoveEn = strtoul(stSplitStr.pStr[i], NULL, 0) ? 1 : 0;
            }
        }
        break;

    case AX_VO_INI_K_D_OUT_FMT:
        if (type == AX_VO_INI_S_LAYER_DISPLAY) {
            AX_VO_OUT_FMT_E fmt;
            pVoConfig = (SAMPLE_VO_CONFIG_S *)pConfig;

            for (i = 0; i < stSplitStr.u32StrNr; i++) {
                for (fmt = AX_VO_OUT_FMT_UNUSED; fmt < AX_VO_OUT_FMT_BUTT; fmt++) {
                    if (!strcmp(stSplitStr.pStr[i], vo_intf_out_fmt[fmt])) {
                        pVoConfig->stVoDev[i].enVoOutfmt = fmt;
                        break;
                    }
                }
                if (fmt >= AX_VO_OUT_FMT_BUTT) {
                    SAMPLE_PRT("Unknown vo out format [%s] for dev%d\n",
                               stSplitStr.pStr[i], pVoConfig->stVoDev[i].u32VoDev);
                }
            }
        }

    default:
        break;
    }

    return ret;
}

AX_S32 SAMPLE_VO_PARSE_INI(const AX_CHAR *section, AX_VO_INI_SECTION_E type, AX_VOID *pConfig)
{
    AX_CHAR line[INI_LINE_LEN] = {0,};
    FILE *fp = AX_NULL;
    AX_CHAR *pstr, *tmp_str;
    AX_BOOL findSection = AX_FALSE, findKey;
    AX_S32 section_len, len;
    AX_S32 key_state;
    AX_VO_INI_KEY_E max_key, key;
    AX_S32 ret = AX_SUCCESS;

    if (section == AX_NULL || pConfig == AX_NULL) {
        SAMPLE_PRT("section:%p, pConfig:%p is invalid.\n", section, pConfig)
        return -1;
    }

    section_len = strlen(section);

    switch (type) {
    case AX_VO_INI_S_LAYER:
        max_key = AX_VO_INI_K_D_BASE;
        break;
    case AX_VO_INI_S_LAYER_DISPLAY:
        max_key = AX_VO_INI_K_BUTT;
        break;
    default:
        SAMPLE_PRT("unknow section type.\n");
        return -1;
    }

    if ((fp = fopen(VO_INI_PATH, "r")) == AX_NULL) {
        SAMPLE_PRT("Fail to open file!\n");
        return -1;
    }

    fseek(fp, 0, SEEK_SET);

    while (!feof(fp)) {
        pstr = fgets(line, INI_LINE_LEN, fp);
        if (pstr == AX_NULL) {
            SAMPLE_PRT("Read line of ini file failed.\n");
            ret = -1;
            break;
        }
        findKey = AX_FALSE;

        if (findSection) {
            if (line[0] == '[') {
                SAMPLE_PRT("section end.\n");
                break;
            }
            for (key = AX_VO_INI_K_VO_DEVICE; key < max_key; key++) {
                pstr = strstr(line, key_str[key]);
                if (pstr == AX_NULL)
                    continue;
                findKey = AX_TRUE;
                break;
            }
            if (findKey) {
                pstr = strchr(line, '=');
                if (pstr == AX_NULL) {
                    ret = -1;
                    SAMPLE_PRT("Doesn't find = opt.\n");
                    break;
                }
                pstr++;
                key_state = VO_INI_GET_KEY_VAL(pstr, pConfig, type, key);
                if (key_state) {
                    ret = -1;
                    SAMPLE_PRT("Get %s section key:%s failed.\n", section, key_str[key]);
                    break;
                }
            }
        } else {
            if (!strncmp(line, section, section_len)) {
                pstr = fgets(line, INI_LINE_LEN, fp);
                if (pstr == AX_NULL) {
                    SAMPLE_PRT("fgets failed.\n");
                    ret = -1;
                    break;
                }
                len = strlen(key_str[AX_VO_INI_K_SECTION_TYPE]);
                key_state = strncmp(line, key_str[AX_VO_INI_K_SECTION_TYPE], len);
                if (key_state) {
                    ret = -1;
                    SAMPLE_PRT("Doesn't find section type key.\n");
                    break;
                }
                pstr = strchr(line + len, '=');
                if (pstr == AX_NULL) {
                    ret = -1;
                    SAMPLE_PRT("Doesn't find = opt.\n");
                    break;
                }
                pstr++;
                /* skip spaces */
                while (*pstr && isspace(*pstr))
                    pstr++;
                /* replace \r \n to 0. */
                tmp_str = strrchr(pstr, '\r');
                if (tmp_str != AX_NULL)
                    *tmp_str = 0;
                tmp_str = strrchr(pstr, '\n');
                if (tmp_str != AX_NULL)
                    *tmp_str = 0;
                if (strcmp(pstr, section_type_str[type])) {
                    ret = -1;
                    SAMPLE_PRT("invalid section type.\n");
                    break;
                }
                findSection = AX_TRUE;
            }
        }
    }

    fclose(fp);

    return ret;
}
