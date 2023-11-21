/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "help.hpp"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "ax_ivps_api.h"
#include "ax_sys_api.h"
#include "ax_vdec_api.h"
#include "ax_vo_api.h"

static AX_BOOL isRunning = AX_TRUE;
static AX_U32 exitCount = 0;

static void exit_handler(int s) {
    printf("\n====================== Caught signal: %d ======================\n", s);
    printf("please waiting to quit ...\n\n");
    isRunning = AX_FALSE;
    if (exitCount++ >= 3) {
        printf("\n======================      Force to exit    ====================== \n");
        abort();
    }
}

static void ignore_sig_pipe(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("failed to ignore SIGPIPE, sigaction");
        exit(EXIT_FAILURE);
    }
}

static AX_VOID InstallSignalHandler(AX_VOID) {
    signal(SIGINT, exit_handler);
    signal(SIGQUIT, exit_handler);
    ignore_sig_pipe();
}

VO_CHN_INFO_T InitLayout(AX_U32 nW, AX_U32 nH, AX_U32 nVideoCount) {
    VO_CHN_INFO_T stChnInfo;
    stChnInfo.nCount = nVideoCount;

    struct POINT {
        AX_U32 x, y;
    } pt = {0, 0};

    struct COORDINATE {
        AX_U32 x1, y1, x2, y2;
    } area;

    constexpr AX_U32 BORDER = 8;
    AX_VO_CHN_ATTR_T stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32FifoDepth = VO_CHN_FIFO_DEPTH;
    stChnAttr.u32Priority = 0;
    stChnAttr.bKeepPrevFr = AX_TRUE;

    AX_U32 nCols = ceil(sqrt((float)nVideoCount));
    AX_U32 nRows = ((nVideoCount % nCols) > 0) ? (nVideoCount / nCols + 1) : (nVideoCount / nCols);
    if (2 == nVideoCount) {
        nCols = 2;
        nRows = 2;
    }

    /* border for both row and col */
    const AX_U32 nAreaW = ALIGN_DOWN(((nW - pt.x - BORDER * (nCols - 1)) / nCols), 8);
    const AX_U32 nAreaH = ALIGN_DOWN(((nH - pt.y - BORDER * (nRows - 1)) / nRows), 2);

    for (AX_U32 i = 0; i < nRows; ++i) {
        for (AX_U32 j = 0; j < nCols; ++j) {
            area.x1 = pt.x + j * BORDER + j * nAreaW;
            area.y1 = pt.y + i * BORDER + i * nAreaH;
            area.x2 = area.x1 + nAreaW;
            area.y2 = area.y1 + nAreaH;

            stChnAttr.stRect.u32X = area.x1;
            stChnAttr.stRect.u32Y = area.y1;
            stChnAttr.stRect.u32Width = area.x2 - area.x1;
            stChnAttr.stRect.u32Height = area.y2 - area.y1;

            stChnInfo.arrChns[i * nCols + j] = stChnAttr;
        }
    }

    return stChnInfo;
}

URL_ARGS CAppArgs::GetUrls(INPUT_TYPE_E eInput) {
    URL_ARGS urls;
    urls.resize(MAX_STREAM_COUNT);
    for (AX_U32 i = 0; i < MAX_STREAM_COUNT; ++i) {
        if (INPUT_RTSP == eInput) {
            if (0 == (i % 4)) {
                urls[i] = std::make_pair("rtsp" + std::to_string(i + 1), "rtsp://192.168.2.12:8554/test.264");
            } else if (1 == (i % 4)) {
                urls[i] = std::make_pair("rtsp" + std::to_string(i + 1), "rtsp://192.168.2.12:8554/road1.264");
            } else if (2 == (i % 4)) {
                urls[i] = std::make_pair("rtsp" + std::to_string(i + 1), "rtsp://192.168.2.12:8554/road2.264");
            } else {
                // m_urls[i] = std::make_pair("rtsp" + std::to_string(i + 1),
                // "rtsp://192.168.2.12:8554/pedestrian_thailand_1920x1080_30fps_5Mbps.265");
                urls[i] = std::make_pair("rtsp" + std::to_string(i + 1), "rtsp://192.168.2.12:8554/road3.264");
            }
        } else {
            urls[i] = std::make_pair("file" + std::to_string(i + 1), "/opt/data/box/h264_road_1_1920x1080_25fps_noB.mp4");
        }
    }

    return urls;
}

CAppSys::CAppSys(AX_U32 vdGrpNum, AX_BOOL bVO, AX_BOOL bIVPS) : m_vdGrpNum(vdGrpNum), m_bVO(bVO), m_bIVPS(bIVPS) {
    InstallSignalHandler();

    // system("rm /opt/data/AXSyslog/syslog/*"); // syslog cannot be removed for ut cases
    printf("launching app ...\n");

    APP_LOG_ATTR_T stLog;
    memset(&stLog, 0, sizeof(stLog));
    stLog.nTarget = APP_LOG_TARGET_STDOUT;
    stLog.nLv = APP_LOG_WARN;
    strcpy(stLog.szAppName, "ut");
    AX_APP_Log_Init(&stLog);

    AX_S32 ret;
    printf("AX_SYS_Init ...\n");
    ret = AX_SYS_Init();
    if (0 != ret) {
        LOG_E("AX_SYS_Init fail, ret = 0x%x", ret);
        exit(1);
    }

    printf("AX_POOL_Exit ...\n");
    ret = AX_POOL_Exit();
    if (0 != ret) {
        LOG_E("AX_POOL_Exit fail, ret = 0x%x", ret);
        exit(1);
    }

    if (m_bVO) {
        printf("AX_VO_Init ...\n");
        ret = AX_VO_Init();
        if (0 != ret) {
            LOG_E("AX_VO_Init fail, ret = 0x%x", ret);
            exit(1);
        }
    }

    if (m_vdGrpNum > 0) {
        AX_VDEC_MOD_ATTR_T stModAttr;
        memset(&stModAttr, 0, sizeof(stModAttr));
        stModAttr.u32MaxGroupCount = m_vdGrpNum;
        stModAttr.enDecModule = AX_ENABLE_ONLY_VDEC;
        printf("AX_VDEC_Init ...\n");
        ret = AX_VDEC_Init(&stModAttr);
        if (0 != ret) {
            LOG_E("AX_VDEC_Init(%d) fail, ret = 0x%x", m_vdGrpNum, ret);
            exit(1);
        }
    }

    if (m_bIVPS) {
        printf("AX_IVPS_Init ...\n");
        ret = AX_IVPS_Init();
        if (0 != ret) {
            LOG_E("AX_IVPS_Init() fail, ret = 0x%x", ret);
            exit(1);
        }
    }
}

CAppSys::~CAppSys(AX_VOID) {
    if (m_bIVPS) {
        AX_IVPS_Deinit();
    }

    if (m_vdGrpNum > 0) {
        AX_VDEC_Deinit();
    }

    if (m_bVO) {
        AX_VO_Deinit();
    }

    AX_POOL_Exit();
    AX_SYS_Deinit();

    AX_APP_Log_DeInit();
}

AX_BOOL CAppSys::IsAppQuit(AX_VOID) {
    return isRunning ? AX_FALSE : AX_TRUE;
}
