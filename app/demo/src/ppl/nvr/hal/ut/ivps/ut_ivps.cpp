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
#include "observer.hpp"
#include "ppl.hpp"

#define TAG "APP"
using namespace std;

static CVO *CreateVO(AX_VOID);
static AX_BOOL DestoryVO(CVO *&vo);

int main(int argc, char *argv[]) {
    cmdline::parser a;
    a.add<AX_U32>("num", 'n', "number of streamer", false, 16, cmdline::range(1, MAX_STREAM_COUNT));
    a.add<AX_U32>("input", 'i', "0: file 1: rtsp", false, 0, cmdline::range(0, 1));
    a.add<AX_U32>("playback", '\0', "0: preview  1: playback", false, 0, cmdline::range(0, 1));
    a.add<AX_U32>("ppVBCnt", '\0', "vdec pp vb num", false, 8, cmdline::range(3, 64));
    a.add<AX_U32>("ppDepth", '\0', "vdec pp fifo depth", false, 0, cmdline::range(0, 64));
    a.add<AX_U32>("ivpsInDepth", '\0', "in depth of IVPS", false, 4, cmdline::range(2, 4));
    a.add<AX_U32>("voInDepth", '\0', "in depth of VO chn", false, 4, cmdline::range(2, 8));
    a.add<AX_U32>("engine", '\0', "1: TDP 3: VPP", false, 3, cmdline::oneof(1, 3));
    a.add<AX_U64>("loop", '\0', "0: infinite, > 0: max loop count", false, 0);
    a.add<AX_U32>("min", '\0', "min ms of random switch", false, 10);
    a.add<AX_U32>("max", '\0', "max ms of random switch", false, 120000);
    a.add<AX_S32>("fps", '\0', "fps control for file, -1: no fps, 0: auto detect fps from stream, > 0: specifed fps", false, 0);
    a.add<AX_S32>("timeout", '\0', "timeout for vdec send stream for preview mode", false, 100);
    a.add<string>("fpath", '\0', "streamer path", false, "");
    a.add<AX_U32>("getFrame", '\0', "get output fifo frame, 0: not get, 1: get", false, 1);
    a.add<AX_U32>("bVarRand", '\0', "Whether the number of output Windows varies randomly, 0: fixed, 1: varies randomly", false, 1);

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

    INPUT_TYPE_E enInput = (INPUT_TYPE_E)a.get<AX_U32>("input");
    const AX_U32 TOTAL_STREAM_COUNT = a.get<AX_U32>("num");
    const string fpath = a.get<string>("fpath");

    vector<string> strURLs(TOTAL_STREAM_COUNT);
    for (AX_U32 i = 0; i < TOTAL_STREAM_COUNT; ++i) {
        if (!fpath.empty()) {
            strURLs[i] = fpath;
        } else {
            strURLs[i] = a.get<string>(urls[enInput][i].first);
        }
    }

    AX_U32 playBackMode = a.get<AX_U32>("playback");
    AX_U32 ppVBCnt = a.get<AX_U32>("ppVBCnt");
    AX_U32 ppDepth = a.get<AX_U32>("ppDepth");
    AX_U32 ivpsInDepth = a.get<AX_U32>("ivpsInDepth");
    AX_U32 ivpsEngine = a.get<AX_U32>("engine");
    AX_U32 voInDepth = a.get<AX_U32>("voInDepth");
    AX_S32 nTimeOut = a.get<AX_S32>("timeout");
    AX_S32 nFps = a.get<AX_S32>("fps");
    AX_U32 getFrame = a.get<AX_U32>("getFrame");
    AX_U32 bVarRand = a.get<AX_U32>("bVarRand");


    AX_U64 loopCount = a.get<AX_U64>("loop");
    AX_U32 min = a.get<AX_U32>("min");
    AX_U32 max = a.get<AX_U32>("max");
    if (max < min) {
        printf("min > max\n");
        return 1;
    }

    if (min == 0 && max == 0) {
        loopCount = 1;
    }

    APP_SYS_INIT(TOTAL_STREAM_COUNT, AX_TRUE, AX_TRUE);
    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    LOG_C("============== APP(APP Ver: %s ) Started %s %s ==============\n", APP_BUILD_VERSION, __DATE__, __TIME__);

    LOG_M_C(TAG, "VDEC %s, PP VB num = %d, depth = %d, bVarRand = %d, TOTAL_STREAM_COUNT = %d", (1 == playBackMode) ? "playback" : "preview",
            ppVBCnt, ppDepth, bVarRand, TOTAL_STREAM_COUNT);
    LOG_M_C(TAG, "IVPS engine %d, in fifo depth = %d", ivpsEngine, ivpsInDepth);
    LOG_M_C(TAG, "VO CHN in fifo depth = %d", voInDepth);
    LOG_M_C(TAG, "loop = %lld, min = %d ms, max = %d ms", loopCount, min, max);

    default_random_engine randEngine;
    struct timeval t;
    gettimeofday(&t, NULL);
    randEngine.seed(1000000 * t.tv_sec + t.tv_usec);
    uniform_int_distribution<AX_U32> ud(min, max);

    vector<PPL> ppls(TOTAL_STREAM_COUNT);
    AX_U32 duration;
    AX_U64 loop = 0;
    AX_U32 layOut = 0;

    while (0 == loopCount || loop < loopCount) {
        if (IS_APP_QUIT()) {
            break;
        }

        ++loop;

        if (bVarRand) {
            layOut = ud(randEngine) % TOTAL_STREAM_COUNT;
        }

        if (layOut == 0 || !bVarRand) {
            layOut = TOTAL_STREAM_COUNT;
        }

        LOG_M_C(TAG, "+++++++++++++ layOut = %d", layOut);
        CVO *vo = CreateVO();
        if (!vo) {
            return 1;
        }

        VO_CHN_INFO_T voChns = InitLayout(1920, 1080, layOut);
        for (AX_U32 i = 0; i < voChns.nCount; ++i) {
            voChns.arrChns[i].u32FifoDepth = voInDepth;
        }

        if (getFrame) {
            CDecodeTask::GetInstance()->Start();
        }

        for (AX_U32 i = 0; i < layOut; ++i) {
            PPL_ATTR_T stAttr;
            stAttr.voChn = i;
            stAttr.enEngine = (AX_IVPS_ENGINE_E)ivpsEngine;
            stAttr.enInput = enInput;
            stAttr.strUrl = strURLs[i];
            stAttr.playBackMode = playBackMode;
            stAttr.ppVBCnt = ppVBCnt;
            stAttr.ppDepth = ppDepth;
            stAttr.nFps = nFps;
            stAttr.nTimeOut = nTimeOut;
            stAttr.ivpsInDepth = ivpsInDepth;
            stAttr.ivpsBufCnt = voInDepth + 1;
            stAttr.vo = vo;
            stAttr.voChnAttr = voChns.arrChns[i];
            if (!ppls[i].Create(stAttr)) {
                for (AX_U32 j = 0; j < i; ++j) {
                    ppls[j].Destory();
                }

                return 1;
            }
        }

        if (min == max) {
            duration = min;
        } else {
            duration = ud(randEngine);
        }

        if (0 == duration) {
            while (!IS_APP_QUIT()) {
                this_thread::sleep_for(chrono::seconds(1));
            }
        } else {
            LOG_M_C(TAG, "[%lld] layout %d, waiting %d ms ...", loop, layOut, duration);

            AX_U32 secs = duration / 1000;
            for (AX_U32 i = 0; i < secs; ++i) {
                if (IS_APP_QUIT()) {
                    break;
                }

                this_thread::sleep_for(chrono::seconds(1));
            }

            this_thread::sleep_for(chrono::milliseconds(duration % 1000));
        }

        for (AX_U32 i = 0; i < layOut; ++i) {
            if (!ppls[i].Destory()) {
                return 1;
            }
        }

        if (!DestoryVO(vo)) {
            return 1;
        }

        if (getFrame) {
            CDecodeTask::GetInstance()->Stop();
        }
    }

    LOG_C("============== APP(APP Ver: %s ) Exited %s %s ==============\n", APP_BUILD_VERSION, __DATE__, __TIME__);

    return 0;
}

static CVO *CreateVO(AX_VOID) {
    VO_ATTR_T stAttr;
    stAttr.dev = 0;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = AX_VO_OUTPUT_1080P60;
    stAttr.nLayerDepth = 3;
    stAttr.nBgClr = 0x0;
    stAttr.stChnInfo.nCount = 0;
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
