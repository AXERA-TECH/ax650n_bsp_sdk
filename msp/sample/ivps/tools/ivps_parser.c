/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "ivps_parser.h"
#include "getopt.h"

#define PARSER_LOGI(fmt, ...) printf("\033[1;30;32mINFO   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__)

static int ivps_fmt_cfg_get(const char *str, AX_IMG_FORMAT_E *eDstPicFormat)
{
    if (!strcmp(str, "AX_FORMAT_YUV420_SEMIPLANAR"))
    {
        *eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_YUV420_SEMIPLANAR_VU"))
    {
        *eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR_VU;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_RGB565"))
    {
        *eDstPicFormat = AX_FORMAT_RGB565;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_RGB888"))
    {
        *eDstPicFormat = AX_FORMAT_RGB888;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_ARGB4444"))
    {
        *eDstPicFormat = AX_FORMAT_ARGB4444;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_ARGB1555"))
    {
        *eDstPicFormat = AX_FORMAT_ARGB1555;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_ARGB8565"))
    {
        *eDstPicFormat = AX_FORMAT_ARGB8565;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_RGBA5551"))
    {
        *eDstPicFormat = AX_FORMAT_RGBA5551;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_RGBA4444"))
    {
        *eDstPicFormat = AX_FORMAT_RGBA4444;
        return 0;
    }
    if (!strcmp(str, "AX_FORMAT_BITMAP"))
    {
        *eDstPicFormat = AX_FORMAT_BITMAP;
        return 0;
    }
    ALOGE("Invalid format: %s in ini", str);
    return -1;
}

static int ivps_grp_cfg_get(const INI_DICT *ini, const char *sect, SAMPLE_IVPS_GRP_T *pGrpCfg)
{

    char key[256];

    pGrpCfg->nIvpsGrp = ini_getint(ini, strcat_unfmt(key, sect, ":nIvpsGrp"), 0);
    pGrpCfg->tPipelineAttr.nOutChnNum = ini_getint(ini, strcat_unfmt(key, sect, ":nOutChnNum"), 0);

    PARSER_LOGI("pGrpCfg->tPipelineAttr.nOutChnNum:%d", pGrpCfg->tPipelineAttr.nOutChnNum);
    return pGrpCfg->tPipelineAttr.nOutChnNum;
}

static int ivps_gdc_ldc_attr_get(const INI_DICT *ini,
                                 const char *sect, AX_IVPS_LDC_ATTR_T *ptLdcAttr)
{
    char key[256];
    const char *str = NULL;

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":bAspect"), "null");
    PARSER_LOGI("bAspect:%s", str);
    if (strcmp(str, "AX_TRUE"))
        ptLdcAttr->bAspect = AX_FALSE;
    else
        ptLdcAttr->bAspect = AX_TRUE;

    ptLdcAttr->nXRatio = ini_getint(ini, strcat_unfmt(key, sect, ":nXRatio"), 0);
    ptLdcAttr->nYRatio = ini_getint(ini, strcat_unfmt(key, sect, ":nYRatio"), 0);
    ptLdcAttr->nXYRatio = ini_getint(ini, strcat_unfmt(key, sect, ":nXYRatio"), 0);
    ptLdcAttr->nCenterXOffset = ini_getint(ini, strcat_unfmt(key, sect, ":nCenterXOffset"), 0);
    ptLdcAttr->nCenterYOffset = ini_getint(ini, strcat_unfmt(key, sect, ":nCenterYOffset"), 0);
    ptLdcAttr->nDistortionRatio = ini_getint(ini, strcat_unfmt(key, sect, ":nDistortionRatio"), 0);
    ptLdcAttr->nSpreadCoef = ini_getint(ini, strcat_unfmt(key, sect, ":nSpreadCoef"), 0);
    return 0;
}

static int ivps_gdc_perspective_attr_get(const INI_DICT *ini,
                                         const char *sect, AX_IVPS_PERSPECTIVE_ATTR_T *ptPerspectiveAttr)
{
    char key[256];
    AX_F32 floatMatrix[9];

    ini_get_arrf32(floatMatrix, 4, ini, strcat_unfmt(key, sect, ":nPerspectiveMatrix"), 0);

    for (int i = 0; i < 9; i++)
        ptPerspectiveAttr->nMatrix[i] = (AX_U64)floatMatrix[i];
    return 0;
}

static int ivps_gdc_cfg_get(const INI_DICT *ini,
                            const char *sect, AX_IVPS_GDC_CFG_T *ptGdcCfg)
{
    char key[256];
    const char *str = NULL;

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":eDewarpType"), "null");

    if (!strcmp(str, "AX_IVPS_DEWARP_BYPASS"))
    {
        ptGdcCfg->eDewarpType = AX_IVPS_DEWARP_BYPASS;
    }
    else if (!strcmp(str, "AX_IVPS_DEWARP_MAP_USER"))
    {
        ptGdcCfg->eDewarpType = AX_IVPS_DEWARP_MAP_USER;
    }
    else if (!strcmp(str, "AX_IVPS_DEWARP_PERSPECTIVE"))
    {
        ptGdcCfg->eDewarpType = AX_IVPS_DEWARP_PERSPECTIVE;
        ivps_gdc_perspective_attr_get(ini, "PERSPECTIVE", &ptGdcCfg->tPerspectiveAttr);
    }
    else if (!strcmp(str, "AX_IVPS_DEWARP_LDC"))
    {
        ptGdcCfg->eDewarpType = AX_IVPS_DEWARP_LDC;
        ivps_gdc_ldc_attr_get(ini, "LDC", &ptGdcCfg->tLdcAttr);
    }
    else if (!strcmp(str, "AX_IVPS_DEWARP_LDC_PERSPECTIVE"))
    {
        ptGdcCfg->eDewarpType = AX_IVPS_DEWARP_LDC_PERSPECTIVE;
        ivps_gdc_perspective_attr_get(ini, "PERSPECTIVE", &ptGdcCfg->tPerspectiveAttr);
        ivps_gdc_ldc_attr_get(ini, "LDC", &ptGdcCfg->tLdcAttr);
    }
    else
    {
        ALOGE("Invalid eDewarpType:%s", str);
        return -1;
    }

    ptGdcCfg->eRotation = ini_getint(ini, strcat_unfmt(key, sect, ":eRotation"), 0);

    if (ptGdcCfg->eRotation >= AX_IVPS_ROTATION_BUTT)
    {
        ALOGE("Invalid eRotation:%d", ptGdcCfg->eRotation);
        return -2;
    }

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":bMirror"), "null");
    PARSER_LOGI("bMirror:%s", str);
    if (strcmp(str, "AX_TRUE"))
        ptGdcCfg->bMirror = AX_FALSE;
    else
        ptGdcCfg->bMirror = AX_TRUE;

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":bFlip"), "null");
    PARSER_LOGI("bFlip:%s", str);
    if (strcmp(str, "AX_TRUE"))
        ptGdcCfg->bFlip = AX_FALSE;
    else
        ptGdcCfg->bFlip = AX_TRUE;

    return 0;
}

static int ivps_filter_cfg_get(const INI_DICT *ini,
                               const char *sect, AX_IVPS_FILTER_T *pFilterCfg)
{
    char key[256];
    const char *str = NULL;
    int cropinfo[4];

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":bEngage"), "null");
    PARSER_LOGI("bEngage:%s", str);
    if (strcmp(str, "AX_TRUE"))
    {
        pFilterCfg->bEngage = AX_FALSE;
        return 0;
    }
    pFilterCfg->bEngage = AX_TRUE;

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":eEngine"), "null");
    PARSER_LOGI("%s", str);

    if (!strcmp(str, "AX_IVPS_ENGINE_TDP"))
    {
        pFilterCfg->eEngine = AX_IVPS_ENGINE_TDP;
    }
    else if (!strcmp(str, "AX_IVPS_ENGINE_GDC"))
    {
        pFilterCfg->eEngine = AX_IVPS_ENGINE_GDC;
        ivps_gdc_cfg_get(ini, "GDC", &pFilterCfg->tGdcCfg);
    }
    else if (!strcmp(str, "AX_IVPS_ENGINE_VPP"))
    {
        pFilterCfg->eEngine = AX_IVPS_ENGINE_VPP;
    }
    else if (!strcmp(str, "AX_IVPS_ENGINE_VGP"))
    {
        pFilterCfg->eEngine = AX_IVPS_ENGINE_VGP;
    }
    else if (!strcmp(str, "AX_IVPS_ENGINE_SUBSIDIARY"))
    {
        pFilterCfg->eEngine = AX_IVPS_ENGINE_SUBSIDIARY;
    }
    else
    {
        ALOGE("Invalid module: %s in ini, should be TDP,VGP,VPP,GDC", str);
    }

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":bCrop"), "null");
    PARSER_LOGI("bCrop:%s", str);
    if (!strcmp(str, "AX_TRUE"))
    {
        pFilterCfg->bCrop = AX_TRUE;
        ini_get_arrint(cropinfo, 4, ini, strcat_unfmt(key, sect, ":tCropRect"), 0);
        PARSER_LOGI("X0:%d Y0:%d W:%d H:%d", cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3]);
        pFilterCfg->tCropRect.nX = cropinfo[0];
        pFilterCfg->tCropRect.nY = cropinfo[1];
        pFilterCfg->tCropRect.nW = cropinfo[2];
        pFilterCfg->tCropRect.nH = cropinfo[3];
    }
    else
    {
        pFilterCfg->bCrop = AX_FALSE;
    }

    pFilterCfg->nDstPicWidth = ini_getint(ini, strcat_unfmt(key, sect, ":nDstPicWidth"), 0);
    LIMIT_MIN(pFilterCfg->nDstPicWidth, 2);
    pFilterCfg->nDstPicHeight = ini_getint(ini, strcat_unfmt(key, sect, ":nDstPicHeight"), 0);
    LIMIT_MIN(pFilterCfg->nDstPicHeight, 2);
    pFilterCfg->nDstPicStride = ini_getint(ini, strcat_unfmt(key, sect, ":nDstPicStride"), 0);
    LIMIT_MIN(pFilterCfg->nDstPicStride, 2);
    str = ini_get_string(ini, strcat_unfmt(key, sect, ":eDstPicFormat"), "null");
    ivps_fmt_cfg_get(str, &pFilterCfg->eDstPicFormat);
    PARSER_LOGI("nDstPicStride:%d eDstPicFormat:%x", pFilterCfg->nDstPicStride, pFilterCfg->eDstPicFormat);
    str = ini_get_string(ini, strcat_unfmt(key, sect, ":bInplace"), "null");

    if (!strcmp(str, "AX_TRUE"))
    {
        pFilterCfg->bInplace = AX_TRUE;
    }
    else
    {
        pFilterCfg->bInplace = AX_FALSE;
    }
    str = ini_get_string(ini, strcat_unfmt(key, sect, ":enCompressMode"), "null");
    if (!strcmp(str, "AX_COMPRESS_MODE_NONE"))
    {
        pFilterCfg->tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
    }
    else if (!strcmp(str, "AX_COMPRESS_MODE_LOSSLESS"))
    {
        pFilterCfg->tCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSLESS;
    }
    else if (!strcmp(str, "AX_COMPRESS_MODE_LOSSY"))
    {
        pFilterCfg->tCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
    }
    else
    {
        ALOGE("Invalid enCompressMode:%s", str);
    }
    pFilterCfg->tCompressInfo.u32CompressLevel = ini_getint(ini, strcat_unfmt(key, sect, ":u32CompressLevel"), 0);
    return 0;
}

static void ivps_pipeline_cfg_get(const INI_DICT *ini,
                                  AX_IVPS_PIPELINE_ATTR_T *pPipelineCfg, int nOutChnNum)
{
    int pipe_idx, filter_idx;
    char str[32] = {0};

    for (pipe_idx = 0; pipe_idx < nOutChnNum + 1; pipe_idx++)
    {
        for (filter_idx = 0; filter_idx < 2; filter_idx++)
        {
            snprintf(str, 32, "filter%d%d", pipe_idx, filter_idx);
            PARSER_LOGI("str:%s", str);
            ivps_filter_cfg_get(ini, str, &pPipelineCfg->tFilter[pipe_idx][filter_idx]);
        }
    }
}

int IVPS_GrpIniParser(const char *ininame, SAMPLE_IVPS_GRP_T *pGrpCfg)
{

    int nOutChnNum;
    INI_DICT *ini;
    const char *module;

    if (access(ininame, 0))
    {
        ALOGW("Invalid file name:%s", ininame);
        return -1;
    }

    PARSER_LOGI("Start to parse:%s", ininame);
    ini = iniparser_load(ininame);
    CHECK_POINTER(ini, "ini");

    module = ini_get_string(ini, "name:module", "null");
    PARSER_LOGI("%-40s : %s", "module", module);

    nOutChnNum = ivps_grp_cfg_get(ini, "GRP", pGrpCfg);
    ivps_pipeline_cfg_get(ini, &pGrpCfg->tPipelineAttr, nOutChnNum);
    PARSER_LOGI("Finish to parse: %s", ininame);

    return 0;
}

int IVPS_ChangeIniParser(const char *ininame, SAMPLE_IVPS_CHANGE_T *pChangeCfg)
{
    INI_DICT *ini;
    const char *module;

    if (access(ininame, 0))
    {
        ALOGW("Invalid file name:%s", ininame);
        return -1;
    }

    PARSER_LOGI("Start to parse:%s", ininame);
    ini = iniparser_load(ininame);
    CHECK_POINTER(ini, "ini");

    module = ini_get_string(ini, "name:module", "null");
    PARSER_LOGI("%-40s : %s", "module", module);

    pChangeCfg->nRepeatNum = ini_getint(ini, "info:nRepeatNum", -1);
    pChangeCfg->nFilterIdx = ini_getint(ini, "info:nFIlterIdx", -1);
    printf("pChangeCfg->nFIlterIdx:%d pChangeCfg->nRepeatNum:%d\n", pChangeCfg->nFilterIdx, pChangeCfg->nRepeatNum);
    ivps_filter_cfg_get(ini, "MinValue", &pChangeCfg->tMinValue);
    ivps_filter_cfg_get(ini, "MaxValue", &pChangeCfg->tMaxValue);
    pChangeCfg->nWidthStep = ini_getint(ini, "step:nWidthStep", 0);
    pChangeCfg->nHeightStep = ini_getint(ini, "step:nHeightStep", 0);
    pChangeCfg->nCropX0Step = ini_getint(ini, "step:nCropX0Step", 0);
    pChangeCfg->nCropY0Step = ini_getint(ini, "step:nCropY0Step", 0);
    pChangeCfg->nCropWStep = ini_getint(ini, "step:nCropWStep", 0);
    pChangeCfg->nCropHStep = ini_getint(ini, "step:nCropHStep", 0);
    PARSER_LOGI("Finish to parse: %s", ininame);

    return 0;
}

static int ivps_mount_mode_get(const char *str, AX_IVPS_FISHEYE_MOUNT_MODE_E *eMountMode)
{
    if (!strcmp(str, "AX_IVPS_FISHEYE_MOUNT_MODE_DESKTOP"))
    {
        *eMountMode = AX_IVPS_FISHEYE_MOUNT_MODE_DESKTOP;
        return 0;
    }
    if (!strcmp(str, "AX_IVPS_FISHEYE_MOUNT_MODE_CEILING"))
    {
        *eMountMode = AX_IVPS_FISHEYE_MOUNT_MODE_CEILING;
        return 0;
    }
    if (!strcmp(str, "AX_IVPS_FISHEYE_MOUNT_MODE_WALL"))
    {
        *eMountMode = AX_IVPS_FISHEYE_MOUNT_MODE_WALL;
        return 0;
    }

    ALOGE("Invalid mount mode: %s in ini", str);
    return -1;
}

static int ivps_view_mode_get(const char *str, AX_IVPS_FISHEYE_VIEW_MODE_E *eViewMode)
{
    if (!strcmp(str, "AX_IVPS_FISHEYE_VIEW_MODE_BYPASS"))
    {
        *eViewMode = AX_IVPS_FISHEYE_VIEW_MODE_BYPASS;
        return 0;
    }
    if (!strcmp(str, "AX_IVPS_FISHEYE_VIEW_MODE_NORMAL"))
    {
        *eViewMode = AX_IVPS_FISHEYE_VIEW_MODE_NORMAL;
        return 0;
    }
    if (!strcmp(str, "AX_IVPS_FISHEYE_VIEW_MODE_PANORAMA"))
    {
        *eViewMode = AX_IVPS_FISHEYE_VIEW_MODE_PANORAMA;
        return 0;
    }
    ALOGE("Invalid view mode: %s in ini", str);
    return -1;
}

static int IVPS_FisheyeRgnIniParser(const INI_DICT *ini, const char *sect, AX_IVPS_FISHEYE_RGN_ATTR_T *tFisheyeRgnAttr)
{
    const char *str = NULL;
    char key[256];
    int cropinfo[4];

    str = ini_get_string(ini, strcat_unfmt(key, sect, ":eViewMode"), "null");
    ivps_view_mode_get(str, &tFisheyeRgnAttr->eViewMode);

    tFisheyeRgnAttr->nPan = ini_getint(ini, strcat_unfmt(key, sect, ":nPan"), 0);
    tFisheyeRgnAttr->nTilt = ini_getint(ini, strcat_unfmt(key, sect, ":nTilt"), 0);
    tFisheyeRgnAttr->nHorZoom = ini_getint(ini, strcat_unfmt(key, sect, ":nHorZoom"), 0);
    tFisheyeRgnAttr->nVerZoom = ini_getint(ini, strcat_unfmt(key, sect, ":nVerZoom"), 0);
    tFisheyeRgnAttr->nInRadius = ini_getint(ini, strcat_unfmt(key, sect, ":nInRadius"), 0);
    tFisheyeRgnAttr->nOutRadius = ini_getint(ini, strcat_unfmt(key, sect, ":nOutRadius"), 0);

    ini_get_arrint(cropinfo, 4, ini, strcat_unfmt(key, sect, ":tOutRect"), 0);
    PARSER_LOGI("X0:%d Y0:%d W:%d H:%d", cropinfo[0], cropinfo[1], cropinfo[2], cropinfo[3]);
    tFisheyeRgnAttr->tOutRect.nX = cropinfo[0];
    tFisheyeRgnAttr->tOutRect.nY = cropinfo[1];
    tFisheyeRgnAttr->tOutRect.nW = cropinfo[2];
    tFisheyeRgnAttr->tOutRect.nH = cropinfo[3];

    return 0;
}

static int IVPS_GdcIniParser(const INI_DICT *ini, AX_IVPS_GDC_ATTR_T *pGdcCfg)
{
    const char *str = NULL;
    char name_str[32] = {0};

    str = ini_get_string(ini, "Info:eGdcType", "null");

    pGdcCfg->nSrcWidth = ini_getint(ini, "Info:nSrcWidth", 0);
    LIMIT_MIN(pGdcCfg->nSrcWidth, 2);
    pGdcCfg->nSrcHeight = ini_getint(ini, "Info:nSrcHeight", 0);
    LIMIT_MIN(pGdcCfg->nSrcHeight, 2);
    pGdcCfg->nDstStride = ini_getint(ini, "Info:nDstStride", 0);
    LIMIT_MIN(pGdcCfg->nDstStride, 2);
    pGdcCfg->nDstWidth = ini_getint(ini, "Info:nDstWidth", 0);
    LIMIT_MIN(pGdcCfg->nDstWidth, 2);
    pGdcCfg->nDstHeight = ini_getint(ini, "Info:nDstHeight", 0);
    LIMIT_MIN(pGdcCfg->nDstHeight, 2);
    str = ini_get_string(ini, "Info:eDstFormat", "null");
    ivps_fmt_cfg_get(str, &pGdcCfg->eDstFormat);

    str = ini_get_string(ini, "Fisheye:eMountMode", "null");
    ivps_mount_mode_get(str, &pGdcCfg->tFisheyeAttr.eMountMode);

    pGdcCfg->tFisheyeAttr.nHorOffset = ini_getint(ini, "Fisheye:nHorOffset", 0);
    pGdcCfg->tFisheyeAttr.nVerOffset = ini_getint(ini, "Fisheye:nVerOffset", 0);
    pGdcCfg->tFisheyeAttr.nRgnNum = ini_getint(ini, "Fisheye:nRgnNum", 0);

    for (int i = 0; i < pGdcCfg->tFisheyeAttr.nRgnNum; i++)
    {
        snprintf(name_str, 32, "FisheyeRgn%d", i);
        PARSER_LOGI("name_str:%s", name_str);
        IVPS_FisheyeRgnIniParser(ini, name_str, &pGdcCfg->tFisheyeAttr.tFisheyeRgnAttr[i]);
    }

    return 0;
}

int IVPS_DewarpIniParser(const char *ininame, SAMPLE_IVPS_DEWARP_T *pDewarpCfg)
{
    INI_DICT *ini;
    const char *module;

    if (access(ininame, 0))
    {
        ALOGW("Invalid file name:%s", ininame);
        return -1;
    }

    PARSER_LOGI("Start to parse:%s", ininame);
    ini = iniparser_load(ininame);
    CHECK_POINTER(ini, "ini");

    module = ini_get_string(ini, "name:module", "null");
    PARSER_LOGI("%-40s : %s", "module", module);

    if (!strcmp(module, "GDC"))
    {
        IVPS_GdcIniParser(ini, &pDewarpCfg->tGdcAttr);
        pDewarpCfg->eSampleType = SAMPLE_IVPS_DEWARP_GDC;
    }
    else if (!strcmp(module, "SYNC"))
    {
        pDewarpCfg->eSampleType = SAMPLE_IVPS_DEWARP_SYNC;
    }
    else
    {
        pDewarpCfg->eSampleType = SAMPLE_IVPS_DEWARP_NONE;
        ALOGE("module:%s in ini is not support!", module);
    }
    PARSER_LOGI("Finish to parse: %s", ininame);

    return 0;
}

enum LONG_OPTION
{
    LONG_OPTION_CONFIG_PIPELINE = 5000,
    LONG_OPTION_CONFIG_REGION,
    LONG_OPTION_CONFIG_CHANGE,       /* dynamic change resolution */
    LONG_OPTION_CONFIG_PIPELINE_EXT, /* cascaded with pipeline  */
    LONG_OPTION_CONFIG_CMMCOPY,
    LONG_OPTION_CONFIG_CSC,
    LONG_OPTION_CONFIG_FLIP_ROTATION,
    LONG_OPTION_CONFIG_ALPHABLEND,
    LONG_OPTION_CONFIG_ALPHABLEND_V2,
    LONG_OPTION_CONFIG_ALPHABLEND_V3,
    LONG_OPTION_CONFIG_CROPRESIZE,
    LONG_OPTION_CONFIG_CROPRESIZE_V2,
    LONG_OPTION_CONFIG_CROPRESIZE_V3,
    LONG_OPTION_CONFIG_OSD,
    LONG_OPTION_CONFIG_MOSAIC,
    LONG_OPTION_CONFIG_COVER,
    LONG_OPTION_CONFIG_DEWARP,
    LONG_OPTION_CONFIG_STREAM,
    LONG_OPTION_CONFIG_POOL_TYPE,
    LONG_OPTION_CONFIG_GRP,
    LONG_OPTION_CONFIG_CHN,
    LONG_OPTION_CONFIG_PYRALITE,
    LONG_OPTION_CONFIG_PYRALITE_MODE,
    LONG_OPTION_CONFIG_HELP,
    LONG_OPTION_CONFIG_BUTT
};

static struct option long_options[] = {
    {"pipeline", required_argument, NULL, LONG_OPTION_CONFIG_PIPELINE},
    {"pipeline_ext", required_argument, NULL, LONG_OPTION_CONFIG_PIPELINE_EXT},
    {"region", required_argument, NULL, LONG_OPTION_CONFIG_REGION},
    {"change", required_argument, NULL, LONG_OPTION_CONFIG_CHANGE},
    {"cmmcopy", required_argument, NULL, LONG_OPTION_CONFIG_CMMCOPY},
    {"csc", required_argument, NULL, LONG_OPTION_CONFIG_CSC},
    {"fliprotation", required_argument, NULL, LONG_OPTION_CONFIG_FLIP_ROTATION},
    {"alphablend", required_argument, NULL, LONG_OPTION_CONFIG_ALPHABLEND},
    {"alphablend2", required_argument, NULL, LONG_OPTION_CONFIG_ALPHABLEND_V2},
    {"alphablend3", required_argument, NULL, LONG_OPTION_CONFIG_ALPHABLEND_V3},
    {"cropresize", required_argument, NULL, LONG_OPTION_CONFIG_CROPRESIZE},
    {"cropresize2", required_argument, NULL, LONG_OPTION_CONFIG_CROPRESIZE_V2},
    {"cropresize3", required_argument, NULL, LONG_OPTION_CONFIG_CROPRESIZE_V3},
    {"osd", required_argument, NULL, LONG_OPTION_CONFIG_OSD},
    {"mosaic", required_argument, NULL, LONG_OPTION_CONFIG_MOSAIC},
    {"cover", required_argument, NULL, LONG_OPTION_CONFIG_COVER},
    {"dewarp", required_argument, NULL, LONG_OPTION_CONFIG_DEWARP},
    {"stream", required_argument, NULL, LONG_OPTION_CONFIG_STREAM},
    {"pool_type", required_argument, NULL, LONG_OPTION_CONFIG_POOL_TYPE},
    {"grp_id", required_argument, NULL, LONG_OPTION_CONFIG_GRP},
    {"ch", required_argument, NULL, LONG_OPTION_CONFIG_CHN},
    {"pyralite", required_argument, NULL, LONG_OPTION_CONFIG_PYRALITE},
    {"pyramode", required_argument, NULL, LONG_OPTION_CONFIG_PYRALITE_MODE},
    {"h", no_argument, NULL, LONG_OPTION_CONFIG_HELP},
    {NULL, 0, NULL, 0}};

static volatile AX_BOOL bThreadLoopExit;

AX_VOID ThreadLoopStateSet(AX_BOOL bValue)
{
    bThreadLoopExit = bValue;
}

AX_BOOL ThreadLoopStateGet(AX_VOID)
{
    return bThreadLoopExit;
}

static AX_VOID SigInt(AX_S32 signo)
{
    ALOGW("SigInt Catch signal %d", signo);
    ThreadLoopStateSet(AX_TRUE);
}

static AX_VOID SigStop(AX_S32 signo)
{
    ALOGW("SigStop Catch signal %d", signo);
    ThreadLoopStateSet(AX_TRUE);
}

int IVPS_ArgsParser(int argc, char *argv[], IVPS_ARG_T *ptArg)
{
    int c = -1;
    int option_index = 0;
    int nHelpIdx = -1;
    AX_BOOL isExit = AX_FALSE;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SigInt);
    signal(SIGTSTP, SigStop);

    while ((c = getopt_long(argc, argv, "v:a:g:x:s:c:p:t:n:h:r:l:", long_options, &option_index)) != -1)
    {
        isExit = AX_FALSE;
        switch (c)
        {
        case 'v':
            ptArg->pFrameInfo = optarg;
            break;
        case 'g':
            ptArg->pOverlayInfo = optarg;
            break;
        case 'x':
            ptArg->pOverlayInfo1 = optarg;
            break;
        case 's':
            ptArg->pSpAlphaFileInfo = optarg;
            break;
        case 'r':
            ptArg->nRegionNum = atoi(optarg);
            if (ptArg->nRegionNum < 0)
                ptArg->nRegionNum = 0;
            break;
        case 'n':
            ptArg->nRepeatCount = atoi(optarg);
            if (ptArg->nRepeatCount <= 0)
            {
                ALOGW("Invalid repeat count parameter, reset to default value: -1");
                ptArg->nRepeatCount = -1;
            }
            break;
        case 'l':
            ptArg->nLinkMode = atoi(optarg);
            PARSER_LOGI("LinkMode %d", ptArg->nLinkMode);
            break;
        case 'h':
            nHelpIdx = atoi(optarg);
            isExit = AX_TRUE;
            break;
        case 'a':
            ptArg->nEngineId = atoi(optarg);
            ptArg->nRepeatCount = -1;
            ptArg->bCmmCopy = AX_TRUE;
            ptArg->bCsc = AX_TRUE;
            ptArg->bAlphaBlend = AX_TRUE;
            ptArg->bAlphaBlendV2 = AX_TRUE;
            ptArg->bCropResize = AX_TRUE;
            ptArg->bCropResizeV2 = AX_TRUE;
            ptArg->bCropResizeV3 = AX_TRUE;
            ptArg->bOsd = AX_TRUE;
            ptArg->bMosaic = AX_TRUE;
            ptArg->bCover = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_PIPELINE:
            ptArg->pPipelineIni = optarg;
            break;
        case LONG_OPTION_CONFIG_REGION:
            ptArg->pRegionIni = optarg;
            break;
        case LONG_OPTION_CONFIG_CHANGE:
            ptArg->pChangeIni = optarg;
            break;
        case LONG_OPTION_CONFIG_PIPELINE_EXT:
            ptArg->pPipelineExtIni = optarg;
            break;
        case LONG_OPTION_CONFIG_CMMCOPY:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bCmmCopy = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_CSC:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bCsc = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_FLIP_ROTATION:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bFlipRotation = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_ALPHABLEND:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bAlphaBlend = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_ALPHABLEND_V2:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bAlphaBlendV2 = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_ALPHABLEND_V3:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bAlphaBlendV3 = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_CROPRESIZE:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bCropResize = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_CROPRESIZE_V2:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bCropResizeV2 = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_CROPRESIZE_V3:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bCropResizeV3 = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_OSD:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bOsd = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_MOSAIC:
            ptArg->nEngineId = atoi(optarg);
            ptArg->bMosaic = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_COVER:
            ptArg->bCover = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_DEWARP:
            ptArg->pDewarpIni = optarg;
            break;
        case LONG_OPTION_CONFIG_STREAM:
            ptArg->nStreamNum = atoi(optarg);
            break;
        case LONG_OPTION_CONFIG_POOL_TYPE:
            ptArg->ePoolSrc = atoi(optarg);
            break;
        case LONG_OPTION_CONFIG_CHN:
            CHECK_RESULT(ChnInfoParse(optarg, &ptArg->tChnInfo[ptArg->nOutChnNum]));
            ptArg->nOutChnNum++;
            break;
        case LONG_OPTION_CONFIG_PYRALITE:
            ptArg->bPyraLite = atoi(optarg);
            break;
        case LONG_OPTION_CONFIG_PYRALITE_MODE:
            ptArg->bPyraMode = atoi(optarg);
            break;
        case LONG_OPTION_CONFIG_GRP:
            ptArg->nIvpsGrp = atoi(optarg);
            break;
        case LONG_OPTION_CONFIG_HELP:
        default:
            /* ignore unknown options */
            isExit = AX_TRUE;
            break;
        }
    }

    PARSER_LOGI("pipeline_ini: %s", ptArg->pPipelineIni);
    PARSER_LOGI("region_ini: %s", ptArg->pRegionIni);

    if (isExit)
    {
        ShowUsage(nHelpIdx);
        exit(0);
    }
    return 0;
}

int IVPS_ChnInfoParser(const IVPS_ARG_T *ptArg, AX_IVPS_PIPELINE_ATTR_T *ptPpl)
{
    if (!ptArg->nOutChnNum)
    {
        ALOGW("There is not channel enabled!");
        return 0;
    }
    ptPpl->nOutChnNum = ptArg->nOutChnNum;
    PARSER_LOGI("ptArg->nOutChnNum:%d", ptArg->nOutChnNum);
    for (int i = 0; i < ptArg->nOutChnNum; i++)
    {
        PARSER_LOGI("W:%d H:%d", ptArg->tChnInfo[i].nW, ptArg->tChnInfo[i].nH);
        ptPpl->tFilter[i + 1][0].nDstPicWidth = ptArg->tChnInfo[i].nW;
        ptPpl->tFilter[i + 1][0].nDstPicHeight = ptArg->tChnInfo[i].nH;
        ptPpl->tFilter[i + 1][0].nDstPicStride = ALIGN_UP(ptArg->tChnInfo[i].nW, 16);
    }
    return 0;
}