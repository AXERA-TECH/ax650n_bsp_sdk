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
#define AX_SAMPLE_LOG_TAG "sample_vin_ivps_venc_rtsp"

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
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2},      /* vin raw16 use */
};

AX_S32 SAMPLE_CASE_OS08A20_VIN_IVPS_VENC_RTSP(SAMPLE_PARAM_T *pSampleParam)
{
    AX_S32 nRet = 0;
    SAMPLE_GRP_T *pIVPSGrp = &gSampleGrp;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    COMMON_SYS_ARGS_T tPrivArgs = {0};
    AX_MOD_INFO_T tSrcMod = {0}, tDstMod = {0};

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
        goto EXIT_FAIL3;
    }

    /* step 3. Start a group of IVPS */
    nRet = SAMPLE_IVPS_StartGrp(pIVPSGrp);
    if (0 != nRet) {
        ALOGE("SAMPLE_IVPS_StartGrp error! ret:0x%x", nRet);
        goto EXIT_FAIL3;
    }

    /* step 4. VENC Init */

    /* step 5. IVPS link to VENC */
    nRet = SAMPLE_Ivps2VencInit(pIVPSGrp, AX_TRUE);
    if (0 != nRet) {
        ALOGE("SAMPLE_Ivps2VencInit error, ret:0x%x", nRet);
        goto EXIT_FAIL5;
    }

    /* step 6. RTSP init and start */
    AX_RTSP_ATTR_T stAttr[MAX_RTSP_MAX_CHANNEL_NUM];
    memset(&stAttr[0], 0x00, sizeof(stAttr));

    // channel 0
    for (int chn_idx = 0; chn_idx < pIVPSGrp->tPipelineAttr.nOutChnNum; chn_idx++) {
        stAttr[chn_idx].nChannel = chn_idx;
        stAttr[chn_idx].stVideoAttr.bEnable = AX_TRUE;
        stAttr[chn_idx].stVideoAttr.ePt = PT_H264;
    }
    AX_Rtsp_Init(&pIVPSGrp->pRtspObj, &stAttr[0], pIVPSGrp->tPipelineAttr.nOutChnNum, 0);
    AX_Rtsp_Start(pIVPSGrp->pRtspObj);

    /* step 7. VIN link to IVPS */
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

    /* step 8. Start VIN */
    SAMPLE_VIN_Start();

    while (!ThreadLoopStateGet()) {
        sleep(1);
    }

    /* VIN Stop & Close */
    SAMPLE_VIN_Stop();

    /* RTSP stop and delete */
    AX_Rtsp_Stop(pIVPSGrp->pRtspObj);
    AX_Rtsp_Deinit(pIVPSGrp->pRtspObj);

EXIT_FAIL6:
    SAMPLE_Ivps2VencDeinit(pIVPSGrp, AX_TRUE);
EXIT_FAIL5:
    SAMPLE_IVPS_StopGrp(pIVPSGrp);
EXIT_FAIL4:
    /* VENC */
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
    printf("\t\t0: OS08A20->VIN->IVPS->VENC->RTSP\n");
    printf("\t-n: frames to calc delta pts.\n");

    printf("\nExample:\n\t/opt/bin/sample_vin_ivps_venc_rtsp -c 0\n");
}

SAMPLE_PARAM_T gSampleParam = {
    SAMPLE_CASE_OS08A20_VIN_IVPS_VENC_RTSP_E,
    COMMON_VIN_SENSOR,
    AX_SNS_LINEAR_MODE,
    AX_TRUE,
    0,
    NULL,
    .statDeltaPtsFrmNum = 0,
};

int main(int argc, char *argv[])
{
    int c = -1, option_index = 0;
    AX_S32 nRet = -1;

    AX_BOOL isExit = AX_TRUE;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SigInt);
    signal(SIGTSTP, SigStop);
    signal(SIGQUIT, SigStop);
    signal(SIGTERM, SigStop);

    while ((c = getopt_long(argc, argv, "c:v:h:n:", long_options, &option_index)) != -1) {
        isExit = AX_FALSE;
        switch (c) {
        case 'c':
            gSampleParam.eSampleCase = (SAMPLE_CASE_E)atoi(optarg);
            break;
        case 'v':
            gSampleParam.pFrameInfo = optarg;
            break;
        case 'h':
            isExit = AX_TRUE;
            break;
        case 'n':
            gSampleParam.statDeltaPtsFrmNum = atoi(optarg);
            if (gSampleParam.statDeltaPtsFrmNum < SAMPLE_MIN_DELTAPTS_NUM) {
                ALOGE("-n not less than %d!\n", SAMPLE_MIN_DELTAPTS_NUM);
                isExit = AX_TRUE;
            }
            break;
        case LONG_OPTION_CONFIG_DUMP:
            gSampleParam.nVencDump = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (argc < 3 || isExit) {
        PrintHelp();
        exit(0);
    }
    if (gSampleParam.eSampleCase == SAMPLE_CASE_OS08A20_VIN_IVPS_VENC_RTSP_E) {
        SAMPLE_CASE_OS08A20_VIN_IVPS_VENC_RTSP(&gSampleParam);
    }
    exit(0);
}
