/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
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
#include "sample_vo_hal.h"

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

typedef enum {
    SAMPLE_POLLING_TYPE_RESET_DESTROY,
    SAMPLE_POLLING_TYPE_DESTROY,
    SAMPLE_POLLING_TYPE_RESET,
    SAMPLE_POLLING_TYPE_BUTT,
} SAMPLE_POLLING_TYPE_E;

AX_S32 gGrpNum = 1;
AX_S32 gLoopDecodeNumber = 1;
AX_S32 gLoopExit = 0;
AX_S32 gWriteFrames = 0;
AX_S32 userPicTest = 0;

AX_S32 gTestSwitch = 0;

AX_S32 gLinkTest = 0;

SAMPLE_VDEC_POLLING_ARGS_T *pstPollingArgs = NULL;
extern AX_POOL GrpPoolId[AX_VDEC_MAX_GRP_NUM];

/*If the device is not connected to a peripheral,uses offline mode*/
static AX_BOOL gOffLine = AX_FALSE;

SAMPLE_VDEC_LINK_CMD_PARAM_T stCmd = {0};
static SAMPLE_VDEC_FUNC_ARGS_T GrpArgs[AX_VDEC_MAX_GRP_NUM];

static SAMPLE_VO_CONFIG_S g_stVoConf = {
    .u32BindMode = 0,
    .u32VDevNr = 1,
    .stVoDev = {
        {
            .u32VoDev = 0,
	    .enMode = AX_VO_MODE_ONLINE,
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
                .enEngineMode = AX_VO_ENGINE_MODE_FORCE,
                .u32EngineId = 0,
                .f32FrmRate = 60,
            },
            .u64KeepChnPrevFrameBitmap0 = ~0x0UL,
            .u64KeepChnPrevFrameBitmap1 = ~0x0UL,
        },
        {
            .bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
            .u32FifoDepth = 3,
        },
        {
            .bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
            .u32FifoDepth = 3,
        },
    },
};

static AX_S32 _LinkInit(SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    AX_S32 s32Ret = 0;
    AX_MOD_INFO_T DstMod = {0};
    AX_MOD_INFO_T SrcMod = {0};

    SAMPLE_LOG_T("start +++++++++++");
    SrcMod.enModId = AX_ID_VDEC;
    SrcMod.s32GrpId = pstCmd->uStartGrpId;
    if ((pstCmd->enDecType == PT_JPEG || pstCmd->enDecType == PT_MJPEG)
        && pstCmd->s32VdecVirtChn) {
        SrcMod.s32ChnId = pstCmd->s32VdecVirtChn;
    } else {
        SrcMod.s32ChnId = 0;
    }

    DstMod.enModId = AX_ID_IVPS;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_Link(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Link SRC:%d, DST:%d failed, s32Ret = 0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
        goto ERR_RET;
    }

    SrcMod.enModId = AX_ID_IVPS;
    SrcMod.s32GrpId = 0;
    SrcMod.s32ChnId = 0;
    DstMod.enModId = AX_ID_VO;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_Link(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Link SRC:%d, DST:%d failed, s32Ret = 0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
        goto ERR_RET;
    }

    SAMPLE_LOG_T("end +++++++++++");
ERR_RET:
    return s32Ret;
}

static AX_S32 _LinkExit(SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    AX_S32 s32Ret = 0;

    AX_MOD_INFO_T DstMod = {0};
    AX_MOD_INFO_T SrcMod = {0};

    SrcMod.enModId = AX_ID_IVPS;
    SrcMod.s32GrpId = 0;
    SrcMod.s32ChnId = 0;
    DstMod.enModId = AX_ID_VO;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_UnLink(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_UnLink SRC:%d, DST:%d failed, s32Ret=0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
        goto ERR_RET;
    }

    SAMPLE_LOG_T("ivps unlink vo ret:%x\n", s32Ret);

    SrcMod.enModId = AX_ID_VDEC;
    SrcMod.s32GrpId = pstCmd->uStartGrpId;
    if ((pstCmd->enDecType == PT_JPEG || pstCmd->enDecType == PT_MJPEG)
        && pstCmd->s32VdecVirtChn) {
        SrcMod.s32ChnId = pstCmd->s32VdecVirtChn;
    } else {
        SrcMod.s32ChnId = 0;
    }

    DstMod.enModId = AX_ID_IVPS;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_UnLink(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_UnLink SRC:%d, DST:%d failed, s32Ret=0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
        goto ERR_RET;
    }

    SAMPLE_LOG_T("vdec unlink ivps ret:%x\n", s32Ret);

ERR_RET:
    return s32Ret;
}


static void *_AX_Link_VDEC_IVPS_TEST(void *arg)
{
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = (SAMPLE_VDEC_CMD_PARAM_T *)arg;
    AX_S32 s32Ret = -1;
    AX_MOD_INFO_T DstMod = {0};
    AX_MOD_INFO_T SrcMod = {0};
    AX_VDEC_CHN linkVdecChn = AX_INVALID_ID;

    return NULL;

    sleep(3);

    SAMPLE_LOG("Before Link Exit");

    SrcMod.enModId = AX_ID_VDEC;
    SrcMod.s32GrpId = pstCmd->uStartGrpId;
    if ((pstCmd->enDecType == PT_JPEG || pstCmd->enDecType == PT_MJPEG)
        && pstCmd->s32VdecVirtChn) {
        linkVdecChn = pstCmd->s32VdecVirtChn;
    } else {
        linkVdecChn = 0;
    }
    SrcMod.s32ChnId = linkVdecChn;

    DstMod.enModId = AX_ID_IVPS;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_UnLink(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_UnLink failed. SRC:%d s32GrpId:%d s32ChnId:%d; "
                        "DST:%d s32GrpId:%d s32ChnId:%d, s32Ret=0x%x\n",
                        SrcMod.enModId, SrcMod.s32GrpId, SrcMod.s32ChnId,
                        DstMod.enModId, DstMod.s32GrpId, DstMod.s32ChnId, s32Ret);
    }
    SAMPLE_LOG("VDEC unlink IVPS ret:%x\n", s32Ret);


    sleep(3);
    SAMPLE_LOG("Before Link ");

    SrcMod.enModId = AX_ID_VDEC;
    SrcMod.s32GrpId = pstCmd->uStartGrpId;
    SrcMod.s32ChnId = linkVdecChn;

    DstMod.enModId = AX_ID_IVPS;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_Link(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Link SRC:%d, DST:%d failed, s32Ret = 0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
    }
    SAMPLE_LOG("VDEC link IVPS ret:%x\n", s32Ret);


    return NULL;
}


static void *_AX_Link_IVPS_VO_TEST(void *arg)
{
    AX_S32 s32Ret = -1;
    sleep(3);
    AX_MOD_INFO_T DstMod = {0};
    AX_MOD_INFO_T SrcMod = {0};

    return NULL;

    SAMPLE_LOG("Before Link Exit");

    SrcMod.enModId = AX_ID_IVPS;
    SrcMod.s32GrpId = 0;
    SrcMod.s32ChnId = 0;
    DstMod.enModId = AX_ID_VO;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_UnLink(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_UnLink SRC:%d, DST:%d failed, s32Ret=0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
    }
    SAMPLE_LOG("VDEC unlink IVPS ret:%x\n", s32Ret);


    sleep(3);
    SAMPLE_LOG("Before Link ");

    SrcMod.enModId = AX_ID_IVPS;
    SrcMod.s32GrpId = 0;
    SrcMod.s32ChnId = 0;
    DstMod.enModId = AX_ID_VO;
    DstMod.s32GrpId = 0;
    DstMod.s32ChnId = 0;
    s32Ret = AX_SYS_Link(&SrcMod, &DstMod);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Link SRC:%d, DST:%d failed, s32Ret=0x%x\n",
                        SrcMod.enModId, DstMod.enModId, s32Ret);
    }
    SAMPLE_LOG("VDEC link IVPS ret:%x\n", s32Ret);


    return NULL;
}


static void *_AX_Link_VDEC_IVPS_VO_TEST(void *arg)
{
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = (SAMPLE_VDEC_CMD_PARAM_T *)arg;
    AX_S32 s32Ret = -1;

    // return NULL;

    sleep(3);

    SAMPLE_LOG("Before Link Exit");

    s32Ret = _LinkExit(pstCmd);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("LinkExit error: 0x%x\n", s32Ret);
    }

    sleep(3);
    SAMPLE_LOG("Before Link ");
    s32Ret = _LinkInit(pstCmd);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("LinkInit error. 0x%x\n", s32Ret);
    }

    return NULL;
}


static void *_AX_LinkTest(void *arg)
{

    // while (1) {
    while (0) {
        if ((gTestSwitch % 2) == 1) {
            usleep(1000);
            continue;
        }

        if (gLoopExit) {
            break;
        }

        gLinkTest += 1;
        _AX_Link_VDEC_IVPS_TEST(arg);
        _AX_Link_VDEC_IVPS_VO_TEST(arg);
        _AX_Link_IVPS_VO_TEST(arg);
        gLinkTest -= 1;
    }

    gLinkTest = 0;
    return NULL;
}

AX_S32 SamplePollingReset(AX_VDEC_GRP VdGrp, SAMPLE_VDEC_CMD_PARAM_T *pstCmd, SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 s32Ret = 0;
    AX_U32 uStartGrpId;

    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL");
        return -1;
    }

    if (pstVoConf == NULL) {
        SAMPLE_CRIT_LOG("pstVoConf == NULL");
        return -1;
    }

    uStartGrpId = pstCmd->uStartGrpId;
    SAMPLE_LOG_T("start ############ VdGrp=%d", VdGrp);
    s32Ret = AX_VDEC_StopRecvStream(VdGrp);
    if (s32Ret) {
            SAMPLE_CRIT_LOG("VdGrp:%d, AX_VDEC_StopRecvStream fail! Error Code:0x%X\n", VdGrp, s32Ret);
            goto ERR_RET;
    }

    SAMPLE_LOG_T("stop vdec done +++++++++++ VdGrp=%d", VdGrp);
    while (1) {
        s32Ret = AX_VDEC_ResetGrp(VdGrp);
        if (s32Ret != AX_ERR_VDEC_BUSY) {
            break;
        }
        usleep(10000);
    }
    SAMPLE_LOG_T("reset vdec done +++++++++++ VdGrp=%d", VdGrp);

    if (VdGrp == uStartGrpId) {
        SAMPLE_LOG_T("ivps and vo reset start +++++++++++ VdGrp=%d", VdGrp);
        SampleIvpsReset();
        VoReset(pstVoConf);
        SAMPLE_LOG_T("ivps and vo reset done +++++++++++ VdGrp=%d", VdGrp);
    }

    s32Ret = AX_VDEC_StartRecvStream(VdGrp, NULL);
    if (s32Ret != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("AX_VDEC_StartRecvStream failed! ret:0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
            goto ERR_RET;
    }


    SAMPLE_LOG_T("end ############ VdGrp=%d", VdGrp);


    return AX_SUCCESS;

ERR_RET:
    return s32Ret;
}


AX_S32 SamplePollingClose(AX_VDEC_GRP VdGrp, SAMPLE_VDEC_CMD_PARAM_T *pstCmd, SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 s32Ret = 0;
    AX_U32 uStartGrpId;
    AX_VDEC_CHN VdChn = AX_INVALID_ID;

    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL");
        return -1;
    }

    if (pstVoConf == NULL) {
        SAMPLE_CRIT_LOG("pstVoConf == NULL");
        return -1;
    }

    SAMPLE_LOG_T("start +++++++++++ VdGrp=%d", VdGrp);

    if ((pstCmd->enDecType == PT_JPEG || pstCmd->enDecType == PT_MJPEG)
        && pstCmd->s32VdecVirtChn) {
        VdChn = pstCmd->s32VdecVirtChn;
    } else {
        VdChn = 0;
    }

    uStartGrpId = pstCmd->uStartGrpId;
    if (VdGrp == uStartGrpId) {
        s32Ret = _LinkExit(pstCmd);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_CRIT_LOG("LinkExit error: %x\n", s32Ret);
            goto ERR_RET;
        }

        VoDeInit(pstVoConf);

        s32Ret = SampleIvpsExit(pstCmd->bEnaIvpsBakFrm);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_CRIT_LOG("SampleIvpsExit error.\n");
            goto ERR_RET;
        }
    }

    s32Ret = AX_VDEC_StopRecvStream(VdGrp);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("VdGrp:%d, AX_VDEC_StopRecvStream fail! Error Code:0x%X\n", VdGrp, s32Ret);
        goto ERR_RET;
    }

    if (pstCmd->enFrameBufSrc == POOL_SOURCE_USER) {
        s32Ret = AX_VDEC_DetachPool(VdGrp, VdChn);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("AX_VDEC_DetachPool fail! Error Code:0x%X\n", s32Ret);
            goto ERR_RET;
        }
    }

    while (1) {
        s32Ret = AX_VDEC_DestroyGrp(VdGrp);
        if (s32Ret == AX_ERR_VDEC_BUSY) {
            SAMPLE_WARN_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                           VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
            usleep(10000);
            if (pstCmd->sWriteFrames) {
                sleep(1);
            }
            continue;
        }

        if (s32Ret != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                            VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
        }

        break;
    }

    SAMPLE_LOG_T("end +++++++++++ VdGrp=%d", VdGrp);
    return AX_SUCCESS;

ERR_RET:
    return s32Ret;
}

AX_S32 SamplePollingOpen(AX_VDEC_GRP VdGrp, SAMPLE_VDEC_CMD_PARAM_T *pstCmd, SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 s32Ret = 0;
    AX_VDEC_CHN_ATTR_T stChnAttr;
    AX_VDEC_CHN_ATTR_T *pstChnAttr = &stChnAttr;
    AX_VDEC_GRP_ATTR_T tVdecAttr = {0};
    AX_POOL_SOURCE_E enFrameBufSrc = POOL_SOURCE_PRIVATE;
    AX_S32 VdChn = 0;
    AX_U32 uStartGrpId;

    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL");
        return -1;
    }

    if (pstVoConf == NULL) {
        SAMPLE_CRIT_LOG("pstVoConf == NULL");
        return -1;
    }


    memset(pstChnAttr, 0x0, sizeof(AX_VDEC_CHN_ATTR_T));
    SAMPLE_LOG_T("start +++++++++++ VdGrp=%d", VdGrp);

    uStartGrpId = pstCmd->uStartGrpId;
    if ((pstCmd->enDecType == PT_JPEG || pstCmd->enDecType == PT_MJPEG)
        && pstCmd->s32VdecVirtChn) {
        VdChn = pstCmd->s32VdecVirtChn;
    } else {
        VdChn = 0;
    }

    if (VdGrp == uStartGrpId) {
        /*vdec link ivps*/
        s32Ret = _LinkInit(pstCmd);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_CRIT_LOG("LinkInit error.\n");
            goto ERR_RET;
        }

        s32Ret = VoInit(pstVoConf);
        if (s32Ret) {
            SAMPLE_CRIT_LOG("VoInit failed, s32Ret = %d\n", s32Ret);
            goto ERR_RET;
        }

        s32Ret = SampleIVPS_Init(pstCmd->bEnaIvpsBakFrm);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_CRIT_LOG("SampleIVPS_Init error. s32Ret:0x%x \n", s32Ret);
            goto ERR_RET;
        }
    }

    enFrameBufSrc = pstCmd->enFrameBufSrc;
    tVdecAttr.enCodecType = pstCmd->enDecType;
    tVdecAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
    tVdecAttr.u32MaxPicWidth = pstCmd->u32MaxPicWidth; // 1920;  /*Max pic width*/
    tVdecAttr.u32MaxPicHeight = pstCmd->u32MaxPicHeight; // 1080;  /*Max pic height*/
    tVdecAttr.u32StreamBufSize = BUFFER_SIZE;
    tVdecAttr.bSdkAutoFramePool = (enFrameBufSrc == POOL_SOURCE_PRIVATE) ? AX_TRUE : AX_FALSE;

    /*GROUP CREATE FOR 16 PATH*/
    s32Ret = AX_VDEC_CreateGrp(VdGrp, &tVdecAttr);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_CreateGrp failed! 0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET;
    }

    if (enFrameBufSrc == POOL_SOURCE_USER) {
        s32Ret = AX_VDEC_AttachPool(VdGrp, VdChn, GrpPoolId[VdGrp]);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("Attach pool err. 0x%x\n", s32Ret);
            goto ERR_RET_DESTROY_GRP;
        }
    }

    s32Ret = SampleVdecChnAttrSet(VdGrp, VdChn, pstCmd, pstChnAttr);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, SampleVdecChnAttrSet FAILED! ret:0x%x %s\n",
                        VdGrp, VdChn, s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_DESTROY_GRP;
    }

    s32Ret = AX_VDEC_EnableChn(VdGrp, VdChn);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, AX_VDEC_EnableChn FAILED! ret:0x%x %s\n",
                        VdGrp, VdChn, s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_DESTROY_GRP;
    }

    AX_VDEC_GRP_PARAM_T stGrpParam;
    s32Ret = AX_VDEC_GetGrpParam(VdGrp, &stGrpParam);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_GetGrpParam failed! 0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_DESTROY_GRP;
    }

    stGrpParam.f32SrcFrmRate = pstCmd->f32SrcFrmRate;
    stGrpParam.stVdecVideoParam.enVdecMode = pstCmd->enVideoMode;
    stGrpParam.stVdecVideoParam.enOutputOrder = pstCmd->enOutputOrder;
    SAMPLE_LOG("VdGrp=%d, stGrpParam.f32SrcFrmRate:%f",
                VdGrp, stGrpParam.f32SrcFrmRate);
    s32Ret = AX_VDEC_SetGrpParam(VdGrp, &stGrpParam);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_SetGrpParam failed! 0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_DESTROY_GRP;
    }

    s32Ret = AX_VDEC_SetDisplayMode(VdGrp, pstCmd->enDisplayMode);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_SetDisplayMode failed! ret:0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_DESTROY_GRP;
    }

    s32Ret = AX_VDEC_StartRecvStream(VdGrp, NULL);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("AX_VDEC_StartRecvStream failed! ret:0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_DESTROY_GRP;
    }

    SAMPLE_LOG_T("end +++++++++++ VdGrp=%d", VdGrp);
    return AX_SUCCESS;

ERR_RET_DESTROY_GRP:
    while (1) {
        s32Ret = AX_VDEC_DestroyGrp(VdGrp);
        if (s32Ret == AX_ERR_VDEC_BUSY) {
            SAMPLE_WARN_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                            VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
            usleep(10000);
            if (pstCmd->sWriteFrames) {
                sleep(1);
            }
            continue;
        }

        if (s32Ret != AX_SUCCESS) {
            SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_DestroyGrp FAILED! ret:0x%x %s",
                            VdGrp, s32Ret, AX_VdecRetStr(s32Ret));
        }

        break;
    }

ERR_RET:
    return s32Ret;
}

static void *_VdecPollingTest(void *arg)
{
    SAMPLE_VDEC_RECV_ARGS_T *pstRecvArgs = (SAMPLE_VDEC_RECV_ARGS_T *)arg;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_POLLING_ARGS_T *pstPollingArgs = NULL;
    AX_U32 uGrpCount = 0;
    AX_U32 uStartGrpId;
    AX_S32 VdGrp = 0;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 poolingNum = 0;
    AX_BOOL bStartWait = AX_TRUE;
    AX_U32 pollingTime = 0;
    AX_U32 randPollingTime = 0;
    AX_U32 pollingWaitCnt = 0;
    AX_BOOL bRestTest = AX_TRUE;
    AX_BOOL inTestNum = 0;
    SAMPLE_POLLING_TYPE_E pollingType = SAMPLE_POLLING_TYPE_RESET_DESTROY;
    AX_U32 pollingExcCnt = 0;

    if (arg == NULL) {
        SAMPLE_CRIT_LOG("arg == NULL");
        return NULL;
    }

    pstPollingArgs = pstRecvArgs->pstPollingArgs;
    pstCmd = pstRecvArgs->pstCmd;
    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL");
        return NULL;
    }

    srand(time(NULL));
    uGrpCount = pstCmd->uGrpCount;
    uStartGrpId = pstCmd->uStartGrpId;
    poolingNum = pstPollingArgs->pollingCnt;
    pollingTime = pstCmd->pollingTime ? pstCmd->pollingTime : 10;
    randPollingTime = rand() % pollingTime + 1;
    pollingWaitCnt = randPollingTime * 10;
    pollingType = pstPollingArgs->pollingType;

    SAMPLE_LOG_T("pollingType:%d poolingNum:%d pollingTime:%d, uStartGrpId:%d, uGrpCount:%d\n",
                 pollingType, poolingNum, pollingTime, uStartGrpId, uGrpCount);

    while (1) {
        if (gLoopExit) {
            SAMPLE_LOG("gLoopExit:%d, so break\n", gLoopExit);
            break;
        }

        if (uGrpCount > 1 && !pstPollingArgs->pollingStart) {
            usleep(100 * 1000);
            continue;
        } else if (bStartWait) {
            while(!gLoopExit && pollingWaitCnt > 0) {
                usleep(100 *1000);
                pollingWaitCnt--;
            }
            if (gLoopExit) {
                pstPollingArgs->pollingStat = SAMPLE_VDEC_POLLING_STATUS_EXIT;
                break;
            }
            bStartWait = AX_FALSE;
        }

        if (pollingType == SAMPLE_POLLING_TYPE_DESTROY) {
             bRestTest = AX_FALSE;
        } else if (pollingType == SAMPLE_POLLING_TYPE_RESET) {
            if (pollingExcCnt)
                bRestTest = AX_TRUE;
        } else {
            if (inTestNum == 0) {
                inTestNum = rand() % 10 + 1;
                bRestTest = AX_FALSE;
            } else {
                bRestTest = AX_TRUE;
            }
        }

        pstPollingArgs->pollingStat = SAMPLE_VDEC_POLLING_STATUS_START;
        SAMPLE_LOG_T("start polling +++++ bRestTest:%d inTestNum %d poolingNum %d randPollingTime:%d uStartGrpId:%d uGrpCount:%d",
                     bRestTest, inTestNum, poolingNum, randPollingTime, uStartGrpId, uGrpCount);
        for (VdGrp = uStartGrpId; VdGrp < uStartGrpId + uGrpCount; VdGrp++) {
             SAMPLE_VDEC_MUTEXT_LOCK(&pstPollingArgs->pollingMutex[VdGrp]);

            if (bRestTest) {
                SamplePollingReset(VdGrp, pstCmd, pstRecvArgs->pstVoConf);
                if (s32Ret != AX_SUCCESS) {
                    SAMPLE_CRIT_LOG("SampleVdecPollingReset failed. s32Ret=0x%x", s32Ret);
                    SAMPLE_VDEC_MUTEXT_UNLOCK(&pstPollingArgs->pollingMutex[VdGrp]);
                    goto ERR_RET;
                }
            } else {
                s32Ret = SamplePollingClose(VdGrp, pstCmd, pstRecvArgs->pstVoConf);
                if (s32Ret != AX_SUCCESS) {
                    SAMPLE_CRIT_LOG("SamplePollingClose failed. s32Ret=0x%x", s32Ret);
                    SAMPLE_VDEC_MUTEXT_UNLOCK(&pstPollingArgs->pollingMutex[VdGrp]);
                    goto ERR_RET;
                }

                s32Ret = SamplePollingOpen(VdGrp, pstCmd, pstRecvArgs->pstVoConf);
                if (s32Ret != AX_SUCCESS) {
                    SAMPLE_CRIT_LOG("SamplePollingOpen failed. s32Ret=0x%x", s32Ret);
                    SAMPLE_VDEC_MUTEXT_UNLOCK(&pstPollingArgs->pollingMutex[VdGrp]);
                    goto ERR_RET;
                }
            }

            pstPollingArgs->reSendStream[VdGrp] = AX_TRUE;
            SAMPLE_VDEC_MUTEXT_UNLOCK(&pstPollingArgs->pollingMutex[VdGrp]);
        }

        pstPollingArgs->pollingStat = SAMPLE_VDEC_POLLING_STATUS_END;
        poolingNum--;
        pollingExcCnt++;
        inTestNum--;
        SAMPLE_LOG_T("end polling +++++ uStartGrpId:%d uGrpCount:%d ", uStartGrpId, uGrpCount);
        randPollingTime = rand() % pollingTime + 1;
        pollingWaitCnt = randPollingTime * 10;
        while(!gLoopExit && pollingWaitCnt > 0) {
            usleep(100 *1000);
            pollingWaitCnt--;
        }
        if (gLoopExit) {
            pstPollingArgs->pollingStat = SAMPLE_VDEC_POLLING_STATUS_EXIT;
            break;
        }

        if (poolingNum <= 0) {
           pstPollingArgs->pollingStat = SAMPLE_VDEC_POLLING_STATUS_EXIT;
           SAMPLE_LOG_T("exit polling +++++");
           break;
        }
    }

ERR_RET:
    return NULL;
}

AX_S32 SAMPLE_EXIT(AX_VOID)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 sRet = -1;
    int i = 0;
    SAMPLE_VDEC_LINK_CMD_PARAM_T *pstCmd = &stCmd;
    static AX_BOOL bSampleExit = AX_FALSE;

    if (bSampleExit) goto FUNC_RET;

    SAMPLE_LOG_T("+++++++++++ start");
    if (pstCmd->stVdecCmdParam.pollingEna && pstPollingArgs) {
        while (pstPollingArgs->pollingStat == SAMPLE_VDEC_POLLING_STATUS_START) {
            usleep(100 * 1000);
        }
    }

    SAMPLE_LOG_T("+++++++++++ begin exit");
    s32Ret = _LinkExit(&pstCmd->stVdecCmdParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("LinkExit error: %x\n", s32Ret);
    }

    VoDeInit(&g_stVoConf);

    sRet = SampleIvpsExit(pstCmd->stVdecCmdParam.bEnaIvpsBakFrm);
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
            sRet = VdecUserPoolExitFunc(GrpArgs[i].VdecGrp, &pstCmd->stVdecCmdParam);
            if (AX_SUCCESS != sRet) {
                SAMPLE_CRIT_LOG("VdecUserPoolExitFunc %d FAILED! VdGrp:%d ret:0x%x\n",
                                i, GrpArgs[i].VdecGrp, sRet);
                s32Ret = sRet;
            }
        }
    }

    bSampleExit = AX_TRUE;
    SAMPLE_LOG_T("+++++++++++ end");

FUNC_RET:
    return s32Ret;
}

static void _SigInt(int sigNo)
{
    printf("Catch signal %d\n", sigNo);
    gLoopExit++;
    if (gLoopExit == 1) {
        while (gLinkTest) {
            sleep(2);
        }

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


static AX_S32 SamplePollingInit(SAMPLE_VDEC_POLLING_ARGS_T **pArgs, SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    AX_S32 s32Ret = AX_SUCCESS;
    int ret = 0;
    AX_U32 tmp_size = 0;
    SAMPLE_VDEC_POLLING_ARGS_T *pstPollingArgs = NULL;
    AX_U32 uStartGrpId = 0;
    AX_U32 uGrpCount = 0;
    AX_S32 gi = 0;
    AX_S32 tmpId = 0;

    if (pArgs == NULL) {
        SAMPLE_CRIT_LOG("pArgs == NULL\n");
        s32Ret = AX_ERR_VDEC_NULL_PTR;
        goto ERR_RET;
    }

    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL\n");
        s32Ret = AX_ERR_VDEC_NULL_PTR;
        goto ERR_RET;
    }

    tmp_size = sizeof(SAMPLE_VDEC_POLLING_ARGS_T);
    pstPollingArgs = (SAMPLE_VDEC_POLLING_ARGS_T *)calloc(1, tmp_size);
    if (NULL == pstPollingArgs) {
        SAMPLE_CRIT_LOG("calloc FAILED! size:0x%x\n", tmp_size);
        s32Ret = AX_ERR_VDEC_NOMEM;
        goto ERR_RET;
    }

    uStartGrpId = pstCmd->uStartGrpId;
    uGrpCount = pstCmd->uGrpCount;
    pstPollingArgs->pollingStat = AX_VDEC_FIFO_STATUS_BUTT;
    pstPollingArgs->pollingCnt = pstCmd->pollingCnt;
    pstPollingArgs->pollingTime = pstCmd->pollingTime;
    pstPollingArgs->pollingType = pstCmd->pollingType;
    for (gi = uStartGrpId; gi < uStartGrpId + uGrpCount; gi++) {
        ret = pthread_mutex_init(&pstPollingArgs->pollingMutex[gi], NULL);
        if (ret != 0) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pthread_mutex_init failed! 0x%x\n", gi, ret);
            s32Ret = AX_ERR_VDEC_UNKNOWN;
            goto ERR_RET_DESTROY;
        }
    }

    *pArgs = pstPollingArgs;

    return AX_SUCCESS;

ERR_RET_DESTROY:
    tmpId = gi;
    for (gi = uStartGrpId; gi < tmpId; gi++) {
        ret = pthread_mutex_destroy(&pstPollingArgs->pollingMutex[gi]);
        if (ret != 0) {
            SAMPLE_CRIT_LOG("VdGrp=%d, pthread_mutex_destroy failed! 0x%x\n", gi, ret);
        }
    }
    if (pstPollingArgs) {
        free(pstPollingArgs);
        *pArgs = pstPollingArgs = NULL;
    }
ERR_RET:
    return s32Ret;
}

static void SamplePollingDeInit(SAMPLE_VDEC_POLLING_ARGS_T *pArgs, SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    AX_U32 uStartGrpId = 0;
    AX_U32 uGrpCount = 0;
    AX_S32 gi = 0;

    if (NULL == pArgs) {
        SAMPLE_CRIT_LOG("NULL == pArgs\n");
        goto ERR_RET;
    }
    if (pstCmd == NULL) {
        SAMPLE_CRIT_LOG("pstCmd == NULL\n");
        goto ERR_RET;
    }

    uStartGrpId = pstCmd->uStartGrpId;
    uGrpCount = pstCmd->uGrpCount;

    for (gi = uStartGrpId; gi < uStartGrpId + uGrpCount; gi++)
        pthread_mutex_destroy(&pArgs->pollingMutex[gi]);

    free(pArgs);

ERR_RET:
    return;
}

int main(int argc, char *argv[])
{
    extern int optind;
    AX_S32 s32Ret = -1;
    AX_S32 sRet = 0;
    AX_PAYLOAD_TYPE_E enDecType;
    AX_CHAR *ps8StreamFile = NULL;
    AX_U32 waitCnt = 0;
    pthread_t vdecRecvTid;
    SAMPLE_VDEC_RECV_ARGS_T stRecvArgs = {0};
    pthread_t LinkTestTid;
    pthread_t pollingTid;
    AX_VDEC_MOD_ATTR_T stModAttr;

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
    SAMPLE_LOG_T("main get type %d\n", enDecType);
    if (enDecType != PT_H264 && enDecType != PT_H265 && enDecType != PT_JPEG) {
        SAMPLE_CRIT_LOG("unsupport enDecType:%d!\n", enDecType);
        goto ERR_RET;
    }

    if (pstCmd->stVdecCmdParam.pollingEna) {
        s32Ret = SamplePollingInit(&pstPollingArgs, &pstCmd->stVdecCmdParam);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_CRIT_LOG("SamplePollingInit FAILED! ret:0x%x\n", s32Ret);
            goto ERR_RET;
        }
    }

    s32Ret = AX_SYS_Init();
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Init FAILED! ret:0x%x\n", s32Ret);
        goto ERR_RET;
    }

    memset(&stModAttr, 0x0, sizeof(AX_VDEC_MOD_ATTR_T));
    stModAttr.enDecModule = pstCmd->stVdecCmdParam.enDecModule;
    stModAttr.u32MaxGroupCount = pstCmd->stVdecCmdParam.uMaxGrpCnt;
    stModAttr.VdecVirtChn = pstCmd->stVdecCmdParam.s32VdecVirtChn;

    s32Ret = AX_VDEC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("AX_VDEC_Init FAILED! ret:0x%x %s\n",
                        s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_SYS_DEINIT;
    }

    ps8StreamFile = pstCmd->stVdecCmdParam.pInputFilePath;

    /*vdec link ivps*/
    s32Ret = _LinkInit(&pstCmd->stVdecCmdParam);
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

    s32Ret = VoInit(&g_stVoConf);
    if (s32Ret) {
        SAMPLE_CRIT_LOG("VoInit failed, s32Ret = %d\n", s32Ret);
        goto ERR_RET_SYS_DEINIT;
    }

    s32Ret = SampleIVPS_Init(pstCmd->stVdecCmdParam.bEnaIvpsBakFrm);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("SampleIVPS_Init error. s32Ret:0x%x \n", s32Ret);
        goto ERR_RET_SYS_DEINIT;
    }

    printf("SampleIVPS_Init success\n");

    pthread_t chnTids[AX_VDEC_MAX_GRP_NUM];
    AX_S32 i;

    SAMPLE_LOG_T("groupCnt %d \n", pstCmd->stVdecCmdParam.uGrpCount);

    memset(&GrpArgs, 0x0, sizeof(SAMPLE_VDEC_FUNC_ARGS_T) * AX_VDEC_MAX_GRP_NUM);
    gGrpNum = pstCmd->stVdecCmdParam.pollingEna ? pstCmd->stVdecCmdParam.uGrpCount : 1;

    for (i = 0; i < gGrpNum; i++) {
        GrpArgs[i].VdecGrp = i + pstCmd->stVdecCmdParam.uStartGrpId;
        GrpArgs[i].sFile = ps8StreamFile;
        GrpArgs[i].stGrpAttr.enCodecType = enDecType;
        GrpArgs[i].pstCmd = &pstCmd->stVdecCmdParam;
        GrpArgs[i].pstPollingArgs = pstPollingArgs;
        pthread_create(&chnTids[i], NULL, VdecFrameFunc, (void *)&GrpArgs[i]);
    }

    printf("Vdec create thread ok\n");

    /*create thread for vo get frame if not display peripherals*/
    pthread_t recvTid;
    if (gOffLine) {
        if (pthread_create(&recvTid, NULL, VoGetFrameThread, &gWriteFrames) != 0) {
            SAMPLE_CRIT_LOG("pthread_create error!\n");
        }
    }


    if (pstCmd->stVdecCmdParam.pollingEna) {
        stRecvArgs.pstCmd = &pstCmd->stVdecCmdParam;
        stRecvArgs.pstPollingArgs = pstPollingArgs;
        stRecvArgs.pstVoConf = &g_stVoConf;

        if (pstCmd->stVdecCmdParam.enSelectMode == AX_VDEC_SELECT_MODE_PRIVATE) {
            if (pthread_create(&vdecRecvTid, NULL, _VdecRecvThread, (void *)&stRecvArgs) != 0) {
                SAMPLE_CRIT_LOG("pthread_create _VdecRecvThread FAILED!\n");
            }
        }

        if (pthread_create(&pollingTid, NULL, _VdecPollingTest, (void *)&stRecvArgs) != 0) {
            SAMPLE_CRIT_LOG("pthread_create error!\n");
        }
    }

    if (pthread_create(&LinkTestTid, NULL, _AX_LinkTest, (void *)&pstCmd->stVdecCmdParam) != 0) {
        SAMPLE_CRIT_LOG("pthread_create error!\n");
    }

    for (i = 0; i < gGrpNum; i++) {
        pthread_join(chnTids[i], NULL);
    }

    if (gOffLine) {
        pthread_join(recvTid, NULL);
    }

    if (pstCmd->stVdecCmdParam.pollingEna) {
        pthread_join(vdecRecvTid, NULL);
        pthread_join(pollingTid, NULL);
    }

    pthread_join(LinkTestTid, NULL);

    while(1) {
        if (gLoopExit) break;

        sleep(1);
        if (pstCmd->stVdecCmdParam.waitTime) {
            waitCnt++;
            if (waitCnt >= pstCmd->stVdecCmdParam.waitTime) {
                break;
            }
        } else {
            break;
        }
    }

    SAMPLE_EXIT();

    sRet = AX_VDEC_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_VDEC_Deinit FAILED! ret:0x%x\n", sRet);
    }

    sRet = AX_SYS_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_SYS_Deinit FAILED! ret:0x%x\n", sRet);
    }

    if (pstCmd->stVdecCmdParam.pollingEna && pstPollingArgs) {
        SamplePollingDeInit(pstPollingArgs, &pstCmd->stVdecCmdParam);
    }

    SAMPLE_LOG_T("Decode Finished! \n\n"); // Log for verify, please do not modify

    return sRet;

ERR_RET_SYS_DEINIT:
    sRet = AX_SYS_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_SYS_Deinit FAILED! ret:0x%x\n", sRet);
    }

ERR_RET:
    if (pstCmd->stVdecCmdParam.pollingEna && pstPollingArgs) {
        SamplePollingDeInit(pstPollingArgs, &pstCmd->stVdecCmdParam);
    }

    return s32Ret || sRet;
}
