/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _IVPS_PARSER_H_
#define _IVPS_PARSER_H_
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "ini_dict.h"
#include "ini_parser.h"
#include "ivps_util.h"
#include "ivps_help.h"
#include "sample_ivps_pipeline.h"
#include "sample_ivps_dewarp.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define iniparser_getval(val, func, ini, default, tmpstr, fmt, ...) \
    {                                                               \
        sprintf(tmpstr, fmt, __VA_ARGS__);                          \
        val = func(ini, tmpstr, default);                           \
    }                                                               \
    while (0)

    void print_arr_f32(AX_F32 *arr, int ele_num, const char *arr_name);
    void print_arr_s32(AX_S32 *arr, int ele_num, const char *arr_name);
    void print_arr_u8(AX_U8 *arr, int ele_num, const char *arr_name);

    typedef struct
    {
        char *pPipelineIni;    /* --pipeline */
        char *pRegionIni;      /* --region */
        char *pChangeIni;      /* --change */
        char *pPipelineExtIni; /* --pipeline_ext */
        char *pDewarpIni;      /* --dewarp */

        AX_S32 nRepeatCount; /* -n */
        AX_S32 nRegionNum;   /* -r */
        AX_S32 nStreamNum;

        AX_U8 nLinkMode; /* -l */

        char *pFrameInfo;   /* -v */
        char *pOverlayInfo; /* -g */
        char *pOverlayInfo1; /* -x */
        char *pSpAlphaFileInfo; /* -s */

        AX_U8 nEngineId;
        AX_BOOL bCmmCopy;      /* --cmmcopy */
        AX_BOOL bCsc;          /* --csc */
        AX_BOOL bFlipRotation; /* --fliprotation */
        AX_BOOL bCropResize;   /* --cropresize */
        AX_BOOL bAlphaBlend;   /* --alphablend */
        AX_BOOL bOsd;          /* --osd */
        AX_BOOL bMosaic;          /* --mosaic */
        AX_BOOL bCover;        /* --cover */
        AX_BOOL bCropResizeV2; /* --cropresize2 */
        AX_BOOL bCropResizeV3; /* --cropresize3 */
        AX_BOOL bAlphaBlendV2;   /* --alphablend2 */
        AX_BOOL bAlphaBlendV3;   /* --alphablend3*/
        AX_POOL_SOURCE_E ePoolSrc;   /* --pool_type */
        AX_U8 bPyraLite;   /* --pyralite */
        AX_BOOL bPyraMode;   /* --pyralite_mode */
        AX_U8 nOutChnNum;
        IVPS_GRP nIvpsGrp;
        IVPS_CHN_INFO_T tChnInfo[5];
    } IVPS_ARG_T;

    int IVPS_ArgsParser(int argc, char *argv[], IVPS_ARG_T *ptArg);
    int IVPS_GrpIniParser(const char *ininame, SAMPLE_IVPS_GRP_T *pGrpCfg);
    int IVPS_ChangeIniParser(const char *ininame, SAMPLE_IVPS_CHANGE_T *pChangeCfg);
    int IVPS_DewarpIniParser(const char *ininame, SAMPLE_IVPS_DEWARP_T *pDewarpCfg);
    int IVPS_ChnInfoParser(const IVPS_ARG_T *ptArg, AX_IVPS_PIPELINE_ATTR_T *ptPpl);

    AX_BOOL ThreadLoopStateGet(AX_VOID);
    AX_VOID ThreadLoopStateSet(AX_BOOL bValue);

#ifdef __cplusplus
}
#endif

#endif /* _IVPS_PARSER_H_ */