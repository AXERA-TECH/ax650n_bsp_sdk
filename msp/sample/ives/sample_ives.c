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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ax_sys_api.h"
#include "sample_md.h"
#include "sample_od.h"
#include "sample_scd.h"
#include "trace.h"

enum { SAMPLE_TASK_MD = 0, SAMPLE_TASK_OD = 1, SAMPLE_TASK_SCD = 2, SAMPLE_TASK_BUTT };

enum LONG_OPTION {
    LONG_OPTION_REF_IMAGE = 10000,
    LONG_OPTION_CUR_IMAGE,
    LONG_OPTION_IMAGE_WIDTH,
    LONG_OPTION_IMAGE_HEIGHT,
    LONG_OPTION_BUTT
};

static AX_VOID ShowUsage(AX_VOID) {
    printf("usage: ./%s [options] ...\n", SAMPLE_IVES_NAME);
    printf("options:\n");
    printf("-t,           task (unsigned int [=%d])\n", SAMPLE_TASK_MD);
    printf("-h,           print this message\n");
    printf(" --ref,       reference image\n");
    printf(" --cur,       current image\n");
    printf(" --width,     image width\n");
    printf(" --height,    image width\n");
    printf("task:\n");
    printf(" %d: motion detect,       example: %s -t %d\n", SAMPLE_TASK_MD, SAMPLE_IVES_NAME, SAMPLE_TASK_MD);
    printf(" %d: occlusion detect,    example: %s -t %d\n", SAMPLE_TASK_OD, SAMPLE_IVES_NAME, SAMPLE_TASK_OD);
    printf(
        " %d: scene change detect, example: %s -t %d --ref=1280x720_1.nv12.yuv --cur=1280x720_2.nv12.yuv --width=1280 "
        "--height=720\n",
        SAMPLE_TASK_SCD, SAMPLE_IVES_NAME, SAMPLE_TASK_SCD);
}

AX_S32 main(AX_S32 argc, AX_CHAR *argv[]) {
    extern int optind;
    AX_S32 ret = 0;
    AX_S32 c;
    AX_S32 isExit = 0;
    AX_S32 task = SAMPLE_TASK_MD;
    AX_CHAR szRefImg[260] = {0};
    AX_CHAR szCurImg[260] = {0};
    AX_U32 nWidth = 1920;
    AX_U32 nHeight = 1080;

#if defined(SAMPLE_IVES_BUILD_VERSION)
    printf("IVES sample: %s build: %s %s\n", SAMPLE_IVES_BUILD_VERSION, __DATE__, __TIME__);
#endif

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {{"ref", required_argument, 0, LONG_OPTION_REF_IMAGE},
                                               {"cur", required_argument, 0, LONG_OPTION_CUR_IMAGE},
                                               {"width", required_argument, 0, LONG_OPTION_IMAGE_WIDTH},
                                               {"height", required_argument, 0, LONG_OPTION_IMAGE_HEIGHT},
                                               {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "t:h", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 't':
                task = atoi(optarg);
                if (task >= SAMPLE_TASK_BUTT) {
                    isExit = 1;
                }
                break;
            case 'h':
                isExit = 1;
                break;
            case LONG_OPTION_REF_IMAGE:
                strcpy(szRefImg, optarg);
                break;
            case LONG_OPTION_CUR_IMAGE:
                strcpy(szCurImg, optarg);
                break;
            case LONG_OPTION_IMAGE_WIDTH:
                nWidth = atoi(optarg);
                break;
            case LONG_OPTION_IMAGE_HEIGHT:
                nHeight = atoi(optarg);
                break;
            default:
                isExit = 1;
                break;
        }
    }

    if (isExit || 0 == nWidth || 0 == nHeight) {
        ShowUsage();
        exit(0);
    }

    ret = AX_SYS_Init();
    if (0 != ret) {
        ALOGE("AX_SYS_Init() fail, ret = 0x%x", ret);
        return -1;
    }

    ret = AX_IVES_Init();
    if (0 != ret) {
        ALOGE("AX_IVES_Init() fail, ret = 0x%x", ret);
        goto EXIT0;
    }

    switch (task) {
        case SAMPLE_TASK_MD:
            ret = SAMPLE_IVES_MD_ENTRY();
            break;
        case SAMPLE_TASK_OD:
            ret = SAMPLE_IVES_OD_ENTRY();
            break;
        case SAMPLE_TASK_SCD:
            ret = SAMPLE_IVES_SCD_ENTRY(szRefImg, szCurImg, nWidth, nHeight);
            break;
        default:
            break;
    }

    if (0 != ret) {
        goto EXIT1;
    }

EXIT1:
    AX_IVES_DeInit();
EXIT0:
    AX_SYS_Deinit();
    return (0 != ret) ? -1 : 0;
}
