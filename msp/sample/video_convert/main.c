/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <ppl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log.h"

static volatile AX_S32 g_exit = 0;
static AX_VOID __sigint(AX_S32 iSigNo);
static AX_VOID print_usage(AX_VOID);

int main(int argc, char *argv[]) {
    printf("sample build at %s %d, SDK %s\n", __DATE__, __LINE__, __BUILD_VERSION__);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigint);

    AX_S32 c;
    AX_S32 quit = 0;
    AX_CHAR fpath[MAX_PATH];
    AX_S32 codec = PT_H264;
    AX_S32 log_lv = LOG_LEVEL_WARN;
    AX_S32 width = 1280;
    AX_S32 height = 720;
    AX_S32 order = 1; /* AX_VDEC_OUTPUT_ORDER_DEC */

    if (argc < 2) {
        print_usage();
        exit(0);
    }

    while ((c = getopt(argc, argv, "i:w:h:t:d:v:")) != -1) {
        quit = 0;
        switch (c) {
            case 'i':
                strcpy(fpath, optarg);
                break;
            case 't':
                codec = (AX_S32)atoi(optarg);
                if (!(PT_H264 == codec || PT_H265 == codec)) {
                    quit = 1;
                }
                break;
            case 'w':
                width = (AX_S32)atoi(optarg);
                if (width < 64) {
                    quit = 1;
                }
                break;
            case 'h':
                height = (AX_S32)atoi(optarg);
                if (height < 64) {
                    quit = 1;
                }
                break;
            case 'd':
                order = (AX_S32)atoi(optarg);
                if (order < 0 || order > 1) {
                    quit = 1;
                }
                break;
            case 'v':
                log_lv = (AX_S32)atoi(optarg);
                break;
            case '?':
                quit = 1;
                break;
            default:
                quit = 1;
                break;
        }
    }

    if (quit) {
        print_usage();
        return 1;
    }

    if (0 != access(fpath, F_OK)) {
        printf("%s not exist\n", fpath);
        return 1;
    }

    set_log_level(log_lv);

    if (!sample_sys_init()) {
        return 1;
    }

    AX_HANDLE ppl = sample_create_ppl(fpath, (AX_PAYLOAD_TYPE_E)codec, width, height, order);
    if (AX_INVALID_HANDLE == ppl) {
        sample_sys_deinit();
        return 1;
    }

    if (!sample_start_ppl(ppl)) {
        sample_destory_ppl(ppl);
        sample_sys_deinit();
        return 1;
    }

    while (!g_exit) {
        if (sample_wait_ppl_eof(ppl, 1000)) {
            break;
        }
    }

    sample_stop_ppl(ppl);
    sample_destory_ppl(ppl);
    sample_sys_deinit();
    printf("done\n");

    return 0;
}

static AX_VOID __sigint(AX_S32 iSigNo) {
    printf("Catch signal %d\n", iSigNo);
    g_exit = 1;
}

static AX_VOID print_usage(AX_VOID) {
    printf("usage: %s [OPTION] ...:\n", __BUILD_NAME__);
    printf("options:\n");
    printf("  -i\t\t file path (string *.mp4, h264, h265)\n");
    printf("  -w\t\t output width  (int >= 64 [=1280])\n");
    printf("  -h\t\t output height (int >= 64 [=720])\n");
    printf("  -t\t\t output payload type %d: H264  %d: H265 (int [=%d])\n", PT_H264, PT_H265, PT_H264);
    printf("  -d\t\t decode output order, 0: DISPLAY 1: DECCODE (int [=DECCODE])\n");
    printf("  -v\t\t log level (int [=3])\n");
    printf("  -?\t\t print this message\n");
}
