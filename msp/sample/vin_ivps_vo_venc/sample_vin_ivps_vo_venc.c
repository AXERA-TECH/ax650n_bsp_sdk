/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <getopt.h>
#include "sample_utils.h"

#undef AX_SAMPLE_LOG_TAG
#define AX_SAMPLE_LOG_TAG "sample_vin_ivps_vo_venc"

typedef enum {
    SAMPLE_CASE_NONE  = -1,
    SAMPLE_CASE_OS08A20_VIN_IVPS_VO_VENC_E  = 0,
    SAMPLE_CASE_BUTT
} SAMPLE_CASE_E;

typedef struct {
    SAMPLE_CASE_E eSampleCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bAiispEnable;
    AX_S32 nVencDump;
    AX_CHAR *pFrameInfo;
} SAMPLE_PARAM_T;

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
enum LONG_OPTION {
    LONG_OPTION_CONFIG_DUMP = 5000,
    LONG_OPTION_CONFIG_HELP,
    LONG_OPTION_CONFIG_BUTT
};
static struct option long_options[] = {
    {"dump", required_argument, NULL, LONG_OPTION_CONFIG_DUMP},
    {NULL, 0, NULL, 0},
};

/* comm pool */
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 20},    /* vin nv21/nv21 use */
};

/* priv pool */
COMMON_SYS_POOL_CFG_T gtPrivPoolSingleOs08a20[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2},  /* vin raw16 use */
};


AX_S32 SAMPLE_CASE_OS08A20_VIN_IVPS_VO_VENC(SAMPLE_PARAM_T *pSampleParam)
{
    AX_S32 nRet = 0;
    SAMPLE_GRP_T *pIVPSGrp = &gSampleGrp;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    COMMON_SYS_ARGS_T tPrivArgs = {0};
    AX_MOD_INFO_T tSrcMod = {0}, tDstMod = {0};
    AX_VO_SIZE_T stImgSize = {1920, 1080};

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

    /* step 3. Start a group of IVPS */
    nRet = SAMPLE_IVPS_StartGrp(pIVPSGrp);
    if (0 != nRet) {
        ALOGE("SAMPLE_IVPS_StartGrp error! ret:0x%x", nRet);
        goto EXIT_FAIL3;
    }

    /* step 4. VO Init. Link IVPS to VO. */
    tSrcMod.enModId = AX_ID_IVPS;
    tSrcMod.s32GrpId = pIVPSGrp->nIvpsGrp;
    tSrcMod.s32ChnId = 0;
    if (pIVPSGrp->tPipelineAttr.tFilter[1][1].bEngage)
    {
        stImgSize.u32Width = pIVPSGrp->tPipelineAttr.tFilter[1][1].nDstPicWidth;
        stImgSize.u32Height = pIVPSGrp->tPipelineAttr.tFilter[1][1].nDstPicHeight;
    }
    else if (pIVPSGrp->tPipelineAttr.tFilter[1][0].bEngage)
    {
        stImgSize.u32Width = pIVPSGrp->tPipelineAttr.tFilter[1][0].nDstPicWidth;
        stImgSize.u32Height = pIVPSGrp->tPipelineAttr.tFilter[1][0].nDstPicHeight;
    }
    else if (pIVPSGrp->tPipelineAttr.tFilter[0][0].bEngage)
    {
        stImgSize.u32Width = pIVPSGrp->tPipelineAttr.tFilter[0][0].nDstPicWidth;
        stImgSize.u32Height = pIVPSGrp->tPipelineAttr.tFilter[0][0].nDstPicHeight;
    }
    else
    {
        ALOGE("IVPS module is Bypass!");
        goto EXIT_FAIL4;
    }
    nRet = VoInit(&tSrcMod, &stImgSize);
    if (nRet) {
        ALOGE("VoInit failed, ret:0x%x\n", nRet);
        goto EXIT_FAIL4;
    }

    /* step 5. Link VO to VENC. VENC init. */
    SampleGetVoModeInfo(&tSrcMod);
    nRet = SAMPLE_Link2VencInit(&tSrcMod, stImgSize.u32Width, stImgSize.u32Height, AX_TRUE);
    if (0 != nRet) {
        ALOGE("SAMPLE_Ivps2VencInit error, ret:0x%x", nRet);
        goto EXIT_FAIL5;
    }

    /* step 6. VIN link to IVPS */
    tSrcMod.enModId = AX_ID_VIN;
    tSrcMod.s32GrpId = 0;
    tSrcMod.s32ChnId = 0;
    tDstMod.enModId = AX_ID_IVPS;
    tDstMod.s32GrpId = pIVPSGrp->nIvpsGrp;
    tDstMod.s32ChnId = 0; /* the chn id of VENC is fix to 0 */
    nRet = AX_SYS_Link(&tSrcMod, &tDstMod);
    if (0 != nRet) {
        ALOGE("AX_SYS_Link error. ret:0x%x", nRet);
        goto EXIT_FAIL6;
    }

    /* step 7. Start VIN */
    SAMPLE_VIN_Start();

    while (!ThreadLoopStateGet()) {
        sleep(1);
    }

    /* VIN Stop & Close */
    SAMPLE_VIN_Stop();
    AX_SYS_UnLink(&tSrcMod, &tDstMod);

EXIT_FAIL6:
    SampleGetVoModeInfo(&tSrcMod);
    SAMPLE_Link2VencDeinit(&tSrcMod, AX_TRUE);
EXIT_FAIL5:
    VoDeInit();
EXIT_FAIL4:
    SAMPLE_IVPS_StopGrp(pIVPSGrp);
EXIT_FAIL3:
    SAMPLE_VIN_Close();
EXIT_FAIL2:
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
    printf("\t\t0: OS08A20->VIN->IVPS->VO->VENC\n");
    printf("\t-v Interface@Resolution@Device\n");
    printf("\t\tSet VO display interface resolution and VO device.\n");
    printf("\t\tInterface:[dpi] [bt601] [bt656] [bt1120] [mipi] [hdmi]\n");
    printf("\t\tResolution:");
    PrintVoReso();
    printf("\t\tDevice: [dev0] [dev1] [dev2]\n");

    printf("\nExample:\n\t/opt/bin/sample_vin_ivps_vo_venc -c 0\n");
    printf("\nExample:\n\t/opt/bin/sample_vin_ivps_vo_venc -c 0 -v hdmi@1080P60@dev0\n");
}

int main(int argc, char *argv[])
{
    int c = -1, option_index = 0;
    AX_S32 nRet = -1;
    SAMPLE_PARAM_T tSampleParam = {
        SAMPLE_CASE_OS08A20_VIN_IVPS_VO_VENC_E,
        COMMON_VIN_SENSOR,
        AX_SNS_LINEAR_MODE,
        AX_TRUE,
        0,
        NULL,
    };
    AX_BOOL isExit = AX_TRUE;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SigInt);
    signal(SIGTSTP, SigStop);
    signal(SIGQUIT, SigStop);
    signal(SIGTERM, SigStop);

    while ((c = getopt_long(argc, argv, "c:i:hv:s:", long_options, &option_index)) != -1) {
        isExit = AX_FALSE;
        switch (c) {
        case 'c':
            tSampleParam.eSampleCase = (SAMPLE_CASE_E)atoi(optarg);
            break;
        case 'i':
            tSampleParam.pFrameInfo = optarg;
            break;
        case 'v':
            nRet = ParseVoPubAttr(optarg);
            if (nRet) {
                isExit = AX_TRUE;
            }
            break;
        case 'h':
            isExit = AX_TRUE;
            break;
        case LONG_OPTION_CONFIG_DUMP:
            tSampleParam.nVencDump = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (argc < 3 || isExit) {
        PrintHelp();
        exit(0);
    }
    if (tSampleParam.eSampleCase == SAMPLE_CASE_OS08A20_VIN_IVPS_VO_VENC_E) {
        SAMPLE_CASE_OS08A20_VIN_IVPS_VO_VENC(&tSampleParam);
    }
    exit(0);
}
