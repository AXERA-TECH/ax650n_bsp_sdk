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
#include <string>
#include <vector>
#include "AppLogApi.h"
#include "ax_sys_api.h"
#include "cmdline.h"
#include "ffmpegstream.hpp"
#include "help.hpp"
#include "rtspstream.hpp"
#include "datastream_record.hpp"

#define TAG "AXDS_SAVE"
using namespace std;


#define IS_NULL(obj) (nullptr == obj ? AX_TRUE : AX_FALSE)
#define MAX_DEVICE_COUNT (32)
#define MAX_STREAM_COUNT_PER_DEV (2)
static IStream *CreateAndInitStream(const std::string &strURL, INPUT_TYPE_E enInput);
static std::unique_ptr<CDataStreamRecord> CreateAndInitDataStream(string strSavePath, AX_U8 nDevCount, AX_U8 nStreamCount, AX_U8 nDuration, AX_BOOL bClean = AX_FALSE);
static CDataStreamObserver* CreateAndInitObservers(AX_U8 nDevice, AX_U8 nStream, CDataStreamRecord* ds);
static std::vector<CDataStreamObserver*> m_vecDataStreamObs;

int main(int argc, char *argv[]) {
    /* 开启2个device，每个device2个stream(main, sub1)，--rtsp1指定device1的输入源，--rtsp2指定device2的输入源，同一个device的不同stream共享同一个输入源，间隔1分钟分割文件 */
    /* ./ut_datastream_save -n 2 -s 2 -d 1 -i 1 --rtsp1 "rtsp://192.168.2.11/road1.264" --rtsp2 "rtsp://192.168.2.11/road2.264" */

    cmdline::parser a;
    a.add<AX_U32>("device", 'n', "number of devices", false, 1, cmdline::range(1, MAX_DEVICE_COUNT));
    a.add<AX_U32>("stream", 's', "number of streams per device", false, 1, cmdline::range(1, MAX_STREAM_COUNT_PER_DEV));
    a.add<AX_U32>("input", 'i', "0: file 1: rtsp", false, 0, cmdline::range(0, 1));
    a.add<AX_U32>("duration", 'd', "time duration of each stream file, unit: minute", false, 10, cmdline::range(1, 10));
    // a.add<AX_U32>("clean", 'c', "clean files of the day", false, 0, cmdline::range(0, 1));
    URL_ARGS urls[INPUT_BUTT];
    urls[INPUT_FILE] = CAppArgs::GetUrls(INPUT_FILE);
    urls[INPUT_RTSP] = CAppArgs::GetUrls(INPUT_RTSP);

    for (AX_U32 i = 0; i < INPUT_BUTT; ++i) {
        for (AX_U32 j = 0; j < MAX_DEVICE_COUNT; ++j) {
            a.add<string>(urls[i][j].first, '\0', "url path", false, urls[i][j].second);
        }
    }
    a.add<AX_S32>("log", 'v', "log level", false, APP_LOG_WARN);
    a.parse_check(argc, argv);
    INPUT_TYPE_E enInput = (INPUT_TYPE_E)a.get<AX_U32>("input");
    const AX_U32 TOTAL_DEVICE_COUNT = a.get<AX_U32>("device");
    const AX_U8 STREAM_COUNT_PER_DEV =  a.get<AX_U32>("stream");
    const AX_U8 nDuration = a.get<AX_U32>("duration");
    // const AX_BOOL bCleanFilesOfDay = a.get<AX_U32>("clean") == 1 ? AX_TRUE : AX_FALSE;
    vector<string> strURLs(TOTAL_DEVICE_COUNT);
    for (AX_U32 i = 0; i < TOTAL_DEVICE_COUNT; ++i) {
        strURLs[i] = a.get<string>(urls[enInput][i].first);
    }

    APP_SYS_INIT(0, AX_FALSE, AX_FALSE);
    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    LOG_M_C(TAG, "Input parameters:");
    LOG_M_C(TAG, "  Device count: %d", TOTAL_DEVICE_COUNT);
    LOG_M_C(TAG, "  Stream count: %d", STREAM_COUNT_PER_DEV);
    LOG_M_C(TAG, "  File duration: %d", nDuration);
    LOG_M_C(TAG, "  Input type: %s", enInput == 1 ? "RTSP" : "FILE");
    // LOG_M_C(TAG, "  Clean files of the day: %s", bCleanFilesOfDay ? "YES" : "NO");
    for (AX_U32 i = 0; i < TOTAL_DEVICE_COUNT; ++i) {
        LOG_M_C(TAG, "  Device %d URL: %s", i + 1, strURLs[i].c_str());
    }

    std::unique_ptr<CDataStreamRecord> ds = CreateAndInitDataStream("/opt/data/datastream", TOTAL_DEVICE_COUNT, STREAM_COUNT_PER_DEV, nDuration);
    if (!ds) {
        LOG_MM_E(TAG, "Start datastream failed.");
        _exit(1);
    }

    std::vector<IStream *> streams(TOTAL_DEVICE_COUNT);
    std::vector<CDataStreamObserver*> observers(TOTAL_DEVICE_COUNT * STREAM_COUNT_PER_DEV);
    for (AX_U32 i = 0; i < TOTAL_DEVICE_COUNT; ++i) {
        IStream *stream = CreateAndInitStream(strURLs[i], enInput);
        if (!stream) {
            _exit(1);
        } else {
            streams[i] = stream;
        }

        for (AX_U32 j = 0; j < STREAM_COUNT_PER_DEV; ++j) {
            CDataStreamObserver* obs = CreateAndInitObservers(i, j, ds.get());
            if (IS_NULL(obs)) {
                LOG_MM_E(TAG, "[%d][%d] Create stream observer failed.", i, j);
                continue;
            }

            observers.push_back(obs);
            stream->RegisterObserver(obs);
        }

        stream->Start();
    }

    for (AX_U32 i = 0; i < TOTAL_DEVICE_COUNT; ++i) {
        const STREAM_INFO_T& tInfo = streams[i]->GetStreamInfo();
        for (AX_U32 j = 0; j < STREAM_COUNT_PER_DEV; ++j) {
            AXDSF_INIT_ATTR_T tStreamAttr = { nDuration /* minutes */
                                            , tInfo.stVideo.enPayload
                                            , (AX_U16)tInfo.stVideo.nFps
                                            , 0
                                            , (AX_U16)tInfo.stVideo.nWidth
                                            , (AX_U16)tInfo.stVideo.nHeight};
            if (!ds->Start(i, j, tStreamAttr)) {
                LOG_MM_E(TAG, "Start data stream failed.");
                continue;
            }
        }
    }

    while (!IS_APP_QUIT()) {
        sleep(1);
    }

    for (auto &&m : streams) {
        if (m) {
            m->Stop();
            m->DeInit();
            delete m;
        }
    }

    for (auto &&m : observers) {
        if (m) {
            delete m;
            m = nullptr;
        }
    }

    ds->StopAll();
    ds->DeInit();

    return 0;
}

static IStream *CreateAndInitStream(const std::string &strURL, INPUT_TYPE_E enInput) {
    IStream *obj = {nullptr};
    if (INPUT_FILE == enInput) {
        obj = new (nothrow) CFFMpegStream;
    } else {
        if (0 != ping4(strURL.c_str(), 4)) {
            printf("network to %s is down\n", strURL.c_str());
            return nullptr;
        }

        obj = new (nothrow) CRtspStream;
    }

    if (obj) {
        STREAM_ATTR_T stAttr;
        stAttr.strURL = strURL;
        if (!obj->Init(stAttr)) {
            delete obj;
            obj = nullptr;
        }
    }

    return obj;
}

static std::unique_ptr<CDataStreamRecord> CreateAndInitDataStream(string strSavePath, AX_U8 nDevCount, AX_U8 nStreamCount, AX_U8 nDuration, AX_BOOL bClean /*= AX_FALSE*/) {
    AXDS_RECORD_INIT_ATTR_T stAttr;
    sprintf(stAttr.szParentDir[0], "%s", strSavePath.c_str());
    stAttr.uMaxDevCnt = nDevCount;
    stAttr.uStreamCnt = nStreamCount;
    stAttr.uMaxDevSpace = 1024;
    stAttr.uMaxFilePeriod = nDuration;
    stAttr.bGenIFOnClose = AX_TRUE;

    std::unique_ptr<CDataStreamRecord> ds = make_unique<CDataStreamRecord>();
    if (ds && ds->Init(stAttr)) {
        return ds;
    }

    return nullptr;
}

static CDataStreamObserver* CreateAndInitObservers(AX_U8 nDevice, AX_U8 nStream, CDataStreamRecord* ds) {
    if (nDevice >= MAX_DEVICE_COUNT || nStream >= MAX_STREAM_COUNT_PER_DEV) {
        LOG_MM_E(TAG, "Device count %d or stream count %d is invalid.", nDevice, nStream);
        return nullptr;
    }

    CDataStreamObserver *obs = new (nothrow) CDataStreamObserver(nDevice, nStream, ds);
    if (obs) {
        return obs;
    }

    return nullptr;
}

