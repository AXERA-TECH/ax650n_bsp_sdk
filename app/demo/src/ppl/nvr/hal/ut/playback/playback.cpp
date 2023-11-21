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
#include <string.h>
#include <unistd.h>
#include <random>
#include <string>
#include <vector>
#include "AppLogApi.h"
#include "cmdline.h"
#include "help.hpp"
#include "ppl.hpp"

#define TAG "APP"
using namespace std;

static CVO *CreateVO(AX_U32 nTotalChns, AX_U32 nChnDepth);
static AX_BOOL DestoryVO(CVO *&vo);

int main(int argc, char *argv[]) {
    cmdline::parser a;
    a.add<AX_U32>("num", 'n', "number of streamer", false, 1, cmdline::range(1, MAX_STREAM_COUNT));
    a.add<string>("input", 'i', "input mp4", false, "");
    a.add<AX_U32>("ppVBCnt", '\0', "vdec pp vb num", false, 5, cmdline::range(3, 64));
    a.add<AX_U32>("voDepth", '\0', "in depth of VO chn", false, 2, cmdline::range(2, 8));
    a.add<AX_U32>("engine", '\0', "1: TDP 3: VPP", false, 3, cmdline::oneof(1, 3));

    URL_ARGS urls[INPUT_BUTT];
    urls[INPUT_FILE] = CAppArgs::GetUrls(INPUT_FILE);
    urls[INPUT_RTSP] = CAppArgs::GetUrls(INPUT_RTSP);
    for (AX_U32 i = 0; i < INPUT_BUTT; ++i) {
        for (AX_U32 j = 0; j < MAX_STREAM_COUNT; ++j) {
            a.add<string>(urls[i][j].first, '\0', "url path", false, urls[i][j].second);
        }
    }
    a.add<AX_S32>("log", 'v', "log level", false, APP_LOG_WARN);
    a.parse_check(argc, argv);

    INPUT_TYPE_E enInput = INPUT_FILE;
    const AX_U32 TOTAL_STREAM_COUNT = a.get<AX_U32>("num");
    const string fpath = a.get<string>("input");

    vector<string> strURLs(TOTAL_STREAM_COUNT);
    for (AX_U32 i = 0; i < TOTAL_STREAM_COUNT; ++i) {
        if (!fpath.empty()) {
            strURLs[i] = fpath;
        } else {
            strURLs[i] = a.get<string>(urls[enInput][i].first);
        }
    }

    AX_U32 playBackMode = 1;
    AX_U32 ppVBCnt = a.get<AX_U32>("ppVBCnt");
    AX_U32 ppDepth = 0;
    AX_U32 ivpsInDepth = 2;
    AX_U32 ivpsEngine = a.get<AX_U32>("engine");
    AX_U32 voInDepth = a.get<AX_U32>("voDepth");

    APP_SYS_INIT(TOTAL_STREAM_COUNT, AX_TRUE, AX_TRUE);
    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    LOG_C("============== APP(APP Ver: %s ) Started %s %s ==============\n", APP_BUILD_VERSION, __DATE__, __TIME__);

    LOG_M_C(TAG, "VDEC %s, PP VB num = %d, depth = %d", (1 == playBackMode) ? "playback" : "preview", ppVBCnt, ppDepth);
    LOG_M_C(TAG, "IVPS engine %d, in fifo depth = %d", ivpsEngine, ivpsInDepth);
    LOG_M_C(TAG, "VO CHN in fifo depth = %d", voInDepth);

    vector<PPL> ppls(TOTAL_STREAM_COUNT);
    AX_U32 layOut = TOTAL_STREAM_COUNT;

    CVO *vo = CreateVO(layOut, voInDepth);
    if (!vo) {
        return 1;
    }

    CDecodeTask::GetInstance()->Start();

    for (AX_U32 i = 0; i < layOut; ++i) {
        PPL_ATTR_T stAttr;
        stAttr.voChn = i;
        stAttr.enEngine = (AX_IVPS_ENGINE_E)ivpsEngine;
        stAttr.enInput = enInput;
        stAttr.strUrl = strURLs[i];
        stAttr.nW = vo->GetAttr().stChnInfo.arrChns[i].stRect.u32Width;
        stAttr.nH = vo->GetAttr().stChnInfo.arrChns[i].stRect.u32Height;
        stAttr.playBackMode = playBackMode;
        stAttr.ppVBCnt = ppVBCnt;
        stAttr.ppDepth = ppDepth;
        stAttr.nFps = -1;     /* no fps control of streamer */
        stAttr.nTimeOut = -1; /* INFINITE */
        stAttr.ivpsInDepth = ivpsInDepth;
        stAttr.ivpsBufCnt = voInDepth + 1;
        stAttr.ivpsBackupDepth = voInDepth + 1; /* backup fifo depth must be vo depth + 1 */
        stAttr.vo = vo;

        /* VDEC VB count should be IVPS backup fifo depth + 1, + 2 is recommended */
        if (stAttr.ppDepth < stAttr.ivpsBackupDepth + 2) {
            stAttr.ppDepth = stAttr.ivpsBackupDepth + 2;
        }

        if (!ppls[i].Create(stAttr)) {
            for (AX_U32 j = 0; j < i; ++j) {
                ppls[j].Destory();
            }

            return 1;
        }
    }

    AX_S32 key0, key1, key;
    AX_S32 quit = 0;
    VO_CHN chn = 0;
    AX_BOOL paused = {AX_FALSE};
    AX_VO_RECT_T rc = vo->GetAttr().stChnInfo.arrChns[chn].stRect;
    while (!IS_APP_QUIT()) {
        printf("please enter[pause(p), step(s), refresh(f), resume(r), exit(q)]:\n");
        key0 = getchar();
        key1 = getchar();
        key = (key0 == '\n') ? key1 : key0;

        switch (key) {
            case 'p':
            case 'P':
                if (vo->PauseChn(chn)) {
                    paused = AX_TRUE;
                }
                break;
            case 's':
            case 'S':
                if (paused) {
                    vo->StepChn(chn);
                }
                break;
            case 'r':
            case 'R':
                if (vo->ResumeChn(chn)) {
                    paused = AX_FALSE;
                }
                break;
            case 'f':
            case 'F':
                if (paused) {
                    ppls[chn].GetIVPS()->CropResize(AX_TRUE, {0, 0, (AX_U16)(rc.u32Width / 2), (AX_U16)(rc.u32Height / 2)});
                    vo->RefreshChn(chn);
                }
                break;
            case '0':
                ppls[chn].GetIVPS()->CropResize(AX_FALSE, {0, 0, 0, 0});
                if (paused) {
                    vo->RefreshChn(chn);
                }
                break;
            case 'q':
            case 'Q':
                quit = 1;
                break;
        }

        if (quit) {
            printf("quit ...\n");
            break;
        }
    }

    for (auto &&m : ppls) {
        if (!m.Destory()) {
            return 1;
        }
    }

    if (!DestoryVO(vo)) {
        return 1;
    }

    CDecodeTask::GetInstance()->Stop();

    LOG_C("============== APP(APP Ver: %s ) Exited %s %s ==============\n", APP_BUILD_VERSION, __DATE__, __TIME__);

    return 0;
}

static CVO *CreateVO(AX_U32 nTotalChns, AX_U32 nChnDepth) {
    VO_ATTR_T stAttr;
    stAttr.dev = 0;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = AX_VO_OUTPUT_1080P60;
    stAttr.nLayerDepth = 3;
    stAttr.nBgClr = 0x0;
    stAttr.stChnInfo = InitLayout(1920, 1080, nTotalChns);
    for (AX_U32 i = 0; i < stAttr.stChnInfo.nCount; ++i) {
        stAttr.stChnInfo.arrChns[i].u32FifoDepth = nChnDepth;
    }

    CVO *obj = CVO::CreateInstance(stAttr);
    if (!obj) {
        return nullptr;
    }

    if (!obj->Start()) {
        obj->Destory();
        return nullptr;
    }

    return obj;
}

static AX_BOOL DestoryVO(CVO *&vo) {
    if (vo) {
        if (!vo->Stop()) {
            return AX_FALSE;
        }

        vo->Destory();
        vo = nullptr;
    }

    return AX_TRUE;
}
