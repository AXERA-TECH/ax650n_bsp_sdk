/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
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
#include <signal.h>

#include "ax_sys_api.h"
#include "ax_base_type.h"
#include "ax_vo_api.h"

#include "common_vo.h"
#include "common_vo_pattern.h"
#include "sample_vo_common.h"
#include "ax_vo_ini.h"

static SAMPLE_VO_LAYER_CONFIG_S g_stLayerConf = {
    .bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
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
        .u32DispatchMode = AX_VO_LAYER_OUT_TO_FIFO,
        .enPartMode = AX_VO_PART_MODE_MULTI,
    },
    .enVoMode = VO_MODE_1MUX,
    .u64KeepChnPrevFrameBitmap0 = ~0x0UL,
    .u64KeepChnPrevFrameBitmap1 = ~0x0UL,
};

static SAMPLE_VO_CONFIG_S g_stVoConf = {
    .stVoLayer = {
        {.bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},},
        {.bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},},
        {.bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},},
    },
};

AX_S32 main(AX_S32 argc, AX_CHAR *argv[])
{
    AX_S32 c;
    AX_S32 casenum = 0, layerCase = -1;
    AX_S32 s32Ret = 0;
    AX_U32 u32DevId = SAMPLE_VO_DEV0, u32CpyType;
    AX_U64 u64SampleTestBit = 0;
    AX_VO_DISPLAY_MODE_T stDisplayMode;
    AX_CHAR section[32] = {0,};

    SAMPLE_PRT("VO Sample. Build at %s %s\n", __DATE__, __TIME__);

    if (argc < 2) {
        SAMPLE_VO_Usage(argv[0]);
        return 0;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SAMPLE_VO_SigInt);
    signal(SIGTSTP, SAMPLE_VO_SigStop);
    signal(SIGQUIT, SAMPLE_VO_SigStop);

    opterr = 0;
    while ((c = getopt(argc, argv, "c:d:e:gm:l:p:")) != -1) {
        switch (c) {
        case 'c':
            if (optarg == NULL) {
                SAMPLE_VO_Usage(argv[0]);
                break;
            }
            u32CpyType = strtoul(optarg, NULL, 10);
            u64SampleTestBit |= SAMPLE_VO_MEMCPY_TEST;
            break;
        case 'd':
            if (optarg == NULL) {
                SAMPLE_VO_Usage(argv[0]);
                break;
            }
            casenum = strtoul(optarg, NULL, 10);
            u64SampleTestBit |= SAMPLE_VO_LAYER_DISPLAY_TEST;
            break;
        case 'e':
            if (optarg == NULL) {
                SAMPLE_VO_Usage(argv[0]);
                break;
            }
            u32DevId = strtoul(optarg, NULL, 10);
            if (u32DevId >= SAMPLE_VO_DEV_MAX) {
                u32DevId = SAMPLE_VO_DEV0;
            }
            u64SampleTestBit |= SAMPLE_VO_DISPLAY_MODE_QUERY_TEST;
            break;
        case 'g':
            u64SampleTestBit |= SAMPLE_VO_HDMI_HOTPLUG_TEST;
            break;
        case 'l':
            if (optarg != NULL) {
                layerCase = strtol(optarg, NULL, 10);
            }
            u64SampleTestBit |= SAMPLE_VO_LAYER_TEST;
            break;
        case 'p':
            if (optarg == NULL) {
                SAMPLE_VO_Usage(argv[0]);
                break;
            }
            casenum = strtoul(optarg, NULL, 10);
            u64SampleTestBit |= SAMPLE_VO_PLAY_TEST;
            break;
        default:
            SAMPLE_VO_Usage(argv[0]);
            SAMPLE_PRT("use default mode\n");
            break;
        }
    }

    s32Ret = AX_SYS_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_SYS_Init failed, s32Ret = 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_POOL_Exit();
    if (s32Ret) {
        SAMPLE_PRT("AX_POOL_Exit failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    if (u64SampleTestBit & SAMPLE_VO_LAYER_TEST) {
        if (layerCase >= 0) {
            sprintf(section, "[case%d]", layerCase);
            s32Ret = SAMPLE_VO_PARSE_INI(section, AX_VO_INI_S_LAYER, &g_stLayerConf);
        }

        if (!s32Ret)
            SAMPLE_VO_LAYER(&g_stLayerConf);
    }

    if (u64SampleTestBit & SAMPLE_VO_LAYER_DISPLAY_TEST) {
        sprintf(section, "[case%d]", casenum);
        s32Ret = SAMPLE_VO_PARSE_INI(section, AX_VO_INI_S_LAYER_DISPLAY, &g_stVoConf);
        if (!s32Ret)
            SAMPLE_VO_LAYER_DISPLAY(&g_stVoConf);
    }

    if (u64SampleTestBit & SAMPLE_VO_DISPLAY_MODE_QUERY_TEST) {
        s32Ret = AX_VO_Init();
        if (s32Ret) {
            SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
            goto exit0;
        }

        s32Ret = AX_VO_EnumMode(u32DevId, &stDisplayMode);
        if (s32Ret) {
            AX_VO_Deinit();
            SAMPLE_PRT("AX_VO_EnumMode failed, s32Ret = 0x%x\n", s32Ret);
            goto exit0;
        }
        AX_VO_Deinit();

        SAMPLE_VO_DISPLAY_MODE_PRINT(u32DevId, &stDisplayMode);
    }

    if (u64SampleTestBit & SAMPLE_VO_HDMI_HOTPLUG_TEST) {
        SAMPLE_VO_HDMI_HOTPLUG();
    }

    if (u64SampleTestBit & SAMPLE_VO_PLAY_TEST) {
        sprintf(section, "[case%d]", casenum);
        s32Ret = SAMPLE_VO_PARSE_INI(section, AX_VO_INI_S_LAYER_DISPLAY, &g_stVoConf);
        if (!s32Ret)
            SAMPLE_VO_PLAY(&g_stVoConf);
    }

    if (u64SampleTestBit & SAMPLE_VO_MEMCPY_TEST) {
        s32Ret = SAMPLE_VO_MEMCPY(u32CpyType);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_MEMCPY failed, s32Ret = 0x%x\n", s32Ret);
            goto exit0;
        }
    }

    while (!SAMPLE_VO_CheckSig()) {
        sleep(1);
    }

exit0:

    AX_SYS_Deinit();

    SAMPLE_PRT("done, s32Ret = 0x%x\n", s32Ret);

    return s32Ret;
}

