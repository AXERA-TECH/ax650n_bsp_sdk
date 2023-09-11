/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "sample_vin_hal.h"
#include "ax_vo_api.h"
#include "sample_vo_hal.h"

#undef AX_SAMPLE_LOG_TAG
#define AX_SAMPLE_LOG_TAG "sample_vin_vo"

typedef enum {
    SAMPLE_CASE_NONE  = -1,
    SAMPLE_CASE_OS08A20_VIN_VO_E  = 0,
    SAMPLE_CASE_BUTT
} SAMPLE_CASE_E;

typedef struct {
    SAMPLE_CASE_E eSampleCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bAiispEnable;
} SAMPLE_PARAM_T;

/* comm pool */
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 20},    /* vin nv21/nv21 use */
};

/* priv pool */
COMMON_SYS_POOL_CFG_T gtPrivPoolSingleOs08a20[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2},  /* vin raw16 use */
};

static volatile AX_S32 gLoopExit = 0;

static AX_VOID __sigint(int iSigNo)
{
    COMM_ISP_PRT("Catch signal %d\n", iSigNo);
    gLoopExit = 1;

    return ;
}

AX_S32 SAMPLE_CASE_OS08A20_VIN_VO(SAMPLE_PARAM_T *pSampleParam, AX_VIN_CHN_ID_E vinChn)
{
    AX_S32 nRet = 0;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    COMMON_SYS_ARGS_T tPrivArgs = {0};
    AX_MOD_INFO_T stModeInfo = {.enModId = AX_ID_VIN,
                                .s32GrpId = 0,
                                .s32ChnId = AX_VIN_CHN_ID_MAIN};
    AX_VO_SIZE_T stImgSize;
    AX_VIN_CHN_ATTR_T camChnAttr = { 0, };
    AX_U8 camPipeId = 0;

    /* step 1. SYS Init & Create memory pool */
    tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20) / sizeof(gtSysCommPoolSingleOs08a20[0]);
    tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs08a20;

    nRet = COMMON_SYS_Init(&tCommonArgs);
    if (nRet) {
        COMM_ISP_PRT("COMMON_SYS_Init fail, ret:0x%x", nRet);
        goto EXIT_FAIL;
    }

    /* step 2. VIN Init & Open */
    tPrivArgs.nPoolCfgCnt = sizeof(gtPrivPoolSingleOs08a20) / sizeof(gtPrivPoolSingleOs08a20[0]);
    tPrivArgs.pPoolCfg = gtPrivPoolSingleOs08a20;
    nRet = SAMPLE_VIN_Init(SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20, pSampleParam->eSysMode, pSampleParam->eHdrMode,
                           pSampleParam->bAiispEnable, &tPrivArgs);
    if (0 != nRet) {
        ALOGE("SAMPLE_VIN_Init failed, ret:0x%x", nRet);
        goto EXIT_FAIL1;
    }
    nRet = SAMPLE_VIN_Open();
    if (0 != nRet) {
        ALOGE("SAMPLE_VIN_Open failed, ret:0x%x", nRet);
        goto EXIT_FAIL2;
    }

    /* step 3. VO Init*/
    /* step 4. VIN link to VO */
    nRet = AX_VIN_GetChnAttr(camPipeId, vinChn, &camChnAttr);
    if (nRet) {
        ALOGE("AX_VIN_GetChnAttr failed, nRet = 0x%x\n", nRet);
        goto EXIT_FAIL2;
    }
    stImgSize.u32Width = camChnAttr.nWidthStride;
    stImgSize.u32Height = camChnAttr.nHeight;
    stModeInfo.s32ChnId = vinChn;
    nRet = VoInit(&stModeInfo, &stImgSize);
    if (nRet) {
        ALOGE("VoInit failed, ret:0x%x\n", nRet);
        goto EXIT_FAIL2;
    }

    /* step 5. Start VIN */
    SAMPLE_VIN_Start();

    while (!gLoopExit) {
        sleep(1);
    }

    /* VIN Stop & Close */
    SAMPLE_VIN_Stop();
    VoDeInit();

EXIT_FAIL2:
    SAMPLE_VIN_Close();
    SAMPLE_VIN_DeInit();
EXIT_FAIL1:
    COMMON_SYS_DeInit();
EXIT_FAIL:
    return nRet;
}

AX_VOID PrintHelp()
{
    printf("Command:\n");
    printf("\t-c: Sample Case:\n");
    printf("\t\t0: OS08A20->VIN->VO\n");
    printf("\t-s: select vin channel id:\n");
    printf("\t\tmin:%d, max:%d\n", AX_VIN_CHN_ID_MAIN, AX_VIN_CHN_ID_MAX - 1);
    printf("\t-v Interface@Resolution@Device\n");
    printf("\t\tInterface:[dpi] [bt601] [bt656] [bt1120] [mipi] [hdmi]\n");
    printf("\t\tResolution:");
    PrintVoReso();
    printf("\t\tDevice: [dev0] [dev1] [dev2]\n");

    printf("\nExample:\n\t/opt/bin/sample_vin_vo -c 0 \n");
    printf("\nExample:\n\t/opt/bin/sample_vin_vo -c 0 -s 0 -v hdmi@1080P60@dev0\n");
}

int main(int argc, char *argv[])
{
    int c = -1;
    SAMPLE_PARAM_T tSampleParam = {
        SAMPLE_CASE_OS08A20_VIN_VO_E,
        COMMON_VIN_SENSOR,
        AX_SNS_LINEAR_MODE,
        AX_TRUE,
    };
    AX_VIN_CHN_ID_E vinChn = AX_VIN_CHN_ID_MAIN;
    AX_BOOL isExit = AX_FALSE;
    AX_S32 s32Ret = 0;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigint);

    while ((c = getopt(argc, argv, "c:m:e:d:a:hv:s:")) != -1 && !isExit) {
        isExit = AX_FALSE;
        switch (c) {
        case 'c':
            tSampleParam.eSampleCase = (SAMPLE_CASE_E)atoi(optarg);
            break;
        case 'v':
            s32Ret = ParseVoPubAttr(optarg);
            if (s32Ret) {
                isExit = AX_TRUE;
            }
            break;
        case 's':
            vinChn = atoi(optarg);
            if (vinChn <= AX_VIN_CHN_ID_INVALID || vinChn >= AX_VIN_CHN_ID_MAX) {
                ALOGE("unsupported vin channel id:%d\n", vinChn);
                isExit = AX_TRUE;
            }
            break;
        case 'h':
            isExit = AX_TRUE;
            break;
        default:
            break;
        }
    }

    if (argc < 3 || isExit) {
        PrintHelp();
        exit(0);
    }
    if (tSampleParam.eSampleCase == SAMPLE_CASE_OS08A20_VIN_VO_E) {
        SAMPLE_CASE_OS08A20_VIN_VO(&tSampleParam, vinChn);
    }
    exit(0);
}
