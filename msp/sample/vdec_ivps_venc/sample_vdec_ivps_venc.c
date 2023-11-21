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
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

//#include "openssl/md5.h"
#include "ax_vdec_api.h"
#include "ax_base_type.h"
#include "ax_sys_api.h"
#include "ax_buffer_tool.h"
#include "sample_ivps_hal.h"
#include "sample_vdec_hal.h"
#include "sample_venc_hal.h"

#include "common_vdec_api.h"

#undef AX_SAMPLE_LOG_TAG
#define AX_SAMPLE_LOG_TAG "sample_vdec_ivps_vo"


#undef SAMPLE_LOG
#undef SAMPLE_ERR_LOG

#define SAMPLE_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_INFO, AX_SAMPLE_LOG_TAG, AX_ID_USER, \
                           MACRO_WHITE"[SAMPLE][AX_VDEC_IVPS_VO][tid:%ld][DEBUG][%s][line:%d]: "str"\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)

#define SAMPLE_ERR_LOG(str, arg...)  do { \
        AX_SYS_LogPrint_Ex(SYS_LOG_ERROR, AX_SAMPLE_LOG_TAG, AX_ID_USER, \
                           MACRO_YELLOW"[SAMPLE][AX_VDEC_IVPS_VO][tid:%ld][ERROR][%s][line:%d]: "str"\n", \
                           gettid(), __func__, __LINE__, ##arg); \
    } while (0)


typedef struct axSAMPLE_VDEC_LINK_CMD_PARAM_T {
    SAMPLE_VDEC_CMD_PARAM_T stVdecCmdParam;
} SAMPLE_VDEC_LINK_CMD_PARAM_T;


AX_S32 gGrpNum = 1;
AX_S32 gLoopDecodeNumber = 1;
AX_S32 gLoopExit = 0;
AX_S32 gWriteFrames = 0;
AX_S32 userPicTest = 0;

AX_S32 gTestSwitch = 0;


extern AX_POOL GrpPoolId[AX_VDEC_MAX_GRP_NUM];
SAMPLE_VDEC_LINK_CMD_PARAM_T stCmd = {0};
static SAMPLE_VDEC_FUNC_ARGS_T GrpArgs[AX_VDEC_MAX_GRP_NUM];

static SAMPLE_VO_CONFIG_S g_stVoConf = {
    .u32BindMode = 0,
    .u32VDevNr = 1,
    .stVoDev = {
        {
            .u32VoDev = 0,
            .enVoIntfType = AX_VO_INTF_HDMI,
            .enIntfSync = AX_VO_OUTPUT_1080P60,
        },
        {
            .u32VoDev = 0,
            .enVoIntfType = AX_VO_INTF_HDMI,
            .enIntfSync = AX_VO_OUTPUT_1080P60,
        },
    },
    .stVoLayer = {
        {
            .enVoMode = VO_MODE_1MUX,
            .u32ChnFrameRate = 30,
            .u32FifoDepth = 3,
            .bindVoDev = {0, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
            .stVoLayerAttr = {
                .stDispRect = {0, 0, 1920, 1080},
                .stImageSize = {1920, 1080},
                .enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR,
                .enSyncMode = AX_VO_LAYER_SYNC_NORMAL,
                .u32PrimaryChnId = 0,
                .u32FifoDepth = 0,
                .u32BkClr = 0,
                .enWBMode = AX_VO_LAYER_WB_POOL,
                .u32InplaceChnId = 0,
                .u32PoolId = 0,
                .u32DispatchMode = AX_VO_LAYER_OUT_TO_LINK,
                .enPartMode = AX_VO_PART_MODE_MULTI,
                .f32FrmRate = 60,
            },
            .u64KeepChnPrevFrameBitmap0 = ~0x0UL,
            .u64KeepChnPrevFrameBitmap1 = ~0x0UL,
        },
        {.bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},},
        {.bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},},
    },
};

static AX_S32 _LinkInit(SAMPLE_VDEC_CMD_PARAM_T *pstCmd, AX_S32 GrpNum)
{
    AX_S32 s32Ret = 0;
    int i = 0;
    AX_MOD_INFO_T DstMod = {0};
    AX_MOD_INFO_T SrcMod = {0};

    for (i = 0; i < GrpNum; i++) {
        SrcMod.enModId = AX_ID_VDEC;
        SrcMod.s32GrpId = i + pstCmd->uStartGrpId;
        SrcMod.s32ChnId = 0;
        DstMod.enModId = AX_ID_IVPS;
        DstMod.s32GrpId = i + pstCmd->uStartGrpId;
        DstMod.s32ChnId = 0;
        s32Ret = AX_SYS_Link(&SrcMod, &DstMod);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("AX_SYS_Link SRC:%d, DST:%d failed, s32Ret = 0x%x\n",
                            SrcMod.enModId, DstMod.enModId, s32Ret);
            goto ERR_RET;
        }

        SrcMod.enModId = AX_ID_IVPS;
        SrcMod.s32GrpId = i + pstCmd->uStartGrpId;
        SrcMod.s32ChnId = 0;
        DstMod.enModId = AX_ID_VENC;
        DstMod.s32GrpId = 0;
        DstMod.s32ChnId = i + pstCmd->uStartGrpId;
        s32Ret = AX_SYS_Link(&SrcMod, &DstMod);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("AX_SYS_Link SRC:%d, DST:%d failed, s32Ret = 0x%x\n",
                            SrcMod.enModId, DstMod.enModId, s32Ret);
            goto ERR_RET;
        }
    }

ERR_RET:
    return s32Ret;
}

static AX_S32 _LinkExit(SAMPLE_VDEC_CMD_PARAM_T *pstCmd, AX_S32 GrpNum)
{
    AX_S32 s32Ret = 0;
    int i = 0;
    AX_MOD_INFO_T DstMod = {0};
    AX_MOD_INFO_T SrcMod = {0};

    for (i = 0; i < GrpNum; i++) {
        SrcMod.enModId = AX_ID_IVPS;
        SrcMod.s32GrpId = i + pstCmd->uStartGrpId;
        SrcMod.s32ChnId = 0;
        DstMod.enModId = AX_ID_VENC;
        DstMod.s32GrpId = 0;
        DstMod.s32ChnId = i + pstCmd->uStartGrpId;
        s32Ret = AX_SYS_UnLink(&SrcMod, &DstMod);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("AX_SYS_UnLink SRC:%d, DST:%d failed, s32Ret=0x%x\n",
                            SrcMod.enModId, DstMod.enModId, s32Ret);
            goto ERR_RET;
        }

        SAMPLE_LOG("ivps unlink venc ret:%x\n", s32Ret);

        SrcMod.enModId = AX_ID_VDEC;
        SrcMod.s32GrpId = i + pstCmd->uStartGrpId;
        SrcMod.s32ChnId = 0;
        DstMod.enModId = AX_ID_IVPS;
        DstMod.s32GrpId = i + pstCmd->uStartGrpId;
        DstMod.s32ChnId = 0;
        s32Ret = AX_SYS_UnLink(&SrcMod, &DstMod);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("AX_SYS_UnLink SRC:%d, DST:%d failed, s32Ret=0x%x\n",
                            SrcMod.enModId, DstMod.enModId, s32Ret);
            goto ERR_RET;
        }
    }

    SAMPLE_LOG("vdec unlink ivps ret:%x\n", s32Ret);

ERR_RET:
    return s32Ret;
}


AX_S32 SAMPLE_EXIT(AX_VOID)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 sRet = -1;
    int i = 0;
    SAMPLE_VDEC_LINK_CMD_PARAM_T *pstCmd = &stCmd;
    static AX_BOOL bSampleExit = AX_FALSE;

    if (bSampleExit) goto FUNC_RET;

    s32Ret = _LinkExit(&pstCmd->stVdecCmdParam, gGrpNum);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("LinkExit error: %x\n", s32Ret);
    }


    SAMPLE_VencDeinit(gGrpNum);
    sRet = SampleIvpsExit(gGrpNum);
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("SampleIvpsExit error.\n");
        s32Ret = sRet;
    }

    for (i = 0; i < gGrpNum; i++) {
        if (pstCmd->stVdecCmdParam.enFrameBufSrc == POOL_SOURCE_PRIVATE) {
            sRet = VdecExitFunc(GrpArgs[i].VdecGrp);
            if (AX_SUCCESS != sRet) {
                SAMPLE_CRIT_LOG("VdecExitFunc %d FAILED! VdGrp:%d ret:0x%x\n",
                                i, GrpArgs[i].VdecGrp, sRet);
                s32Ret = sRet;
            }
        } else if (pstCmd->stVdecCmdParam.enFrameBufSrc == POOL_SOURCE_USER) {
            sRet = VdecUserPoolExitFunc(GrpArgs[i].VdecGrp);
            if (AX_SUCCESS != sRet) {
                SAMPLE_CRIT_LOG("VdecUserPoolExitFunc %d FAILED! VdGrp:%d ret:0x%x\n",
                                i, GrpArgs[i].VdecGrp, sRet);
                s32Ret = sRet;
            }
        }
    }

    sRet = AX_VDEC_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_VDEC_Deinit FAILED! ret:0x%x\n", sRet);
    }

    sRet = AX_SYS_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_SYS_Deinit FAILED! ret:0x%x\n", sRet);
    }

    bSampleExit = AX_TRUE;

FUNC_RET:
    return s32Ret;
}

static void _SigInt(int sigNo)
{
    SAMPLE_LOG_T("Catch signal %d\n", sigNo);
    gLoopExit++;
    if (gLoopExit == 1) {
        SAMPLE_EXIT();
    }
    else if (gLoopExit > 3) {
        exit(0);
    }

    return ;
}

static void _SigIntZ(int sigNo)
{
    gTestSwitch++;
    printf("Catch signal %d, gTestSwitch:%d\n", sigNo, gTestSwitch);

    return ;
}

int main(int argc, char *argv[])
{
    extern int optind;
    AX_S32 s32Ret = -1;
    AX_S32 sRet = 0;
    AX_PAYLOAD_TYPE_E enDecType;
    AX_CHAR *psStreamFile = NULL;
    AX_U32 waitCnt = 0;
    int GrpNum = 1;
    AX_U32 width = 0;
    AX_U32 height = 0;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, _SigInt); /* ctrl + c */
    signal(SIGQUIT, _SigInt); /* ctrl + \ */
    signal(SIGTERM, _SigInt);

    signal(SIGTSTP, _SigIntZ); /* ctrl + z */

    SAMPLE_VDEC_LINK_CMD_PARAM_T *pstCmd = &stCmd;

    sRet = VdecDefaultParamsSet(&pstCmd->stVdecCmdParam);
    if (sRet) {
        SAMPLE_CRIT_LOG("VdecDefaultParamsSet FAILED! ret:0x%x\n", sRet);
        goto ERR_RET;
    }

    sRet = VdecCmdLineParseAndCheck(argc, argv, &pstCmd->stVdecCmdParam, 0, 1);
    if (sRet) {
        SAMPLE_CRIT_LOG("VdecCmdLineParseAndCheck FAILED! ret:0x%x\n", sRet);
        goto ERR_RET;
    }

    enDecType = pstCmd->stVdecCmdParam.enDecType;
    gGrpNum = GrpNum = pstCmd->stVdecCmdParam.uGrpCount;
    SAMPLE_LOG_T("main get type %d\n", enDecType);
    if (enDecType != PT_H264 && enDecType != PT_H265 && enDecType != PT_JPEG) {
        SAMPLE_CRIT_LOG("unsupport enDecType:%d!\n", enDecType);
        goto ERR_RET;
    }

    if (pstCmd->stVdecCmdParam.enInputMode != AX_VDEC_INPUT_MODE_FRAME) {
        SAMPLE_CRIT_LOG("unsupport enInputMode:%d!\n", pstCmd->stVdecCmdParam.enInputMode);
        goto ERR_RET;
    }

    s32Ret = AX_SYS_Init();
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Init FAILED! ret:0x%x\n", s32Ret);
        goto ERR_RET;
    }

    s32Ret = AX_VDEC_Init(NULL);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("AX_VDEC_Init FAILED! ret:0x%x %s\n",
                        s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_SYS_DEINIT;
    }

    psStreamFile = pstCmd->stVdecCmdParam.pInputFilePath;

    /*vdec link ivps*/
    s32Ret = _LinkInit(&pstCmd->stVdecCmdParam, GrpNum);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("LinkInit error.\n");
        goto ERR_RET_SYS_DEINIT;
    }

    g_stVoConf.stVoLayer[0].u32ChnFrameRate = 0;
    g_stVoConf.stVoLayer[0].u64KeepChnPrevFrameBitmap0 = ~0x0UL;
    g_stVoConf.stVoLayer[0].u64KeepChnPrevFrameBitmap1 = ~0x0UL;

    SAMPLE_LOG(".stVoLayer[0].u32ChnFrameRate:%d, "
                ".u64KeepChnPrevFrameBitmap0:0x%llx\n",
                g_stVoConf.stVoLayer[0].u32ChnFrameRate,
                g_stVoConf.stVoLayer[0].u64KeepChnPrevFrameBitmap0);

    width = pstCmd->stVdecCmdParam.tChnCfg[0].u32PicWidth;
    height = pstCmd->stVdecCmdParam.tChnCfg[0].u32PicHeight;

    SAMPLE_LOG_T("============ width %d, height:%d  =========", width, height);

    s32Ret = SampleIVPS_Init(GrpNum, width, height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("SampleIVPS_Init error. s32Ret:0x%x \n", s32Ret);
        goto ERR_RET_SYS_DEINIT;
    }

    SAMPLE_VencInit(GrpNum, width, height, AX_ID_VENC, pstCmd->stVdecCmdParam.sWriteFrames);

    pthread_t chnTids[AX_VDEC_MAX_GRP_NUM];
    AX_S32 i;

    SAMPLE_LOG_T("groupCnt %d \n", pstCmd->stVdecCmdParam.uGrpCount);

    memset(&GrpArgs, 0x0, sizeof(SAMPLE_VDEC_FUNC_ARGS_T) * AX_VDEC_MAX_GRP_NUM);

    for (i = 0; i < GrpNum; i++) {
        GrpArgs[i].VdecGrp = i + pstCmd->stVdecCmdParam.uStartGrpId;
        GrpArgs[i].sFile = psStreamFile;
        GrpArgs[i].stGrpAttr.enCodecType = enDecType;
        GrpArgs[i].pstCmd = &pstCmd->stVdecCmdParam;
        pthread_create(&chnTids[i], NULL, VdecFrameFunc, (void *)&GrpArgs[i]);
    }

    for (i = 0; i < gGrpNum; i++) {
        pthread_join(chnTids[i], NULL);
    }

    while(!gLoopExit && waitCnt <= pstCmd->stVdecCmdParam.waitTime) {
        sleep(1);
        if (pstCmd->stVdecCmdParam.waitTime)
            waitCnt++;
    }

    SAMPLE_EXIT();

    SAMPLE_LOG_T("Decode Finished! \n\n"); // Log for verify, please do not modify

    return sRet;

ERR_RET_SYS_DEINIT:
    sRet = AX_SYS_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_SYS_Deinit FAILED! ret:0x%x\n", sRet);
    }

ERR_RET:

    return s32Ret || sRet;
}
