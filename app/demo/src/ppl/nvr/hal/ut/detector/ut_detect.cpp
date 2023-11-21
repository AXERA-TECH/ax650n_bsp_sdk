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
#include "ax_engine_api.h"
#include "ax_engine_type.h"
#include "ax_sys_api.h"
#include "cmdline.h"
#include "detector.hpp"
#include "help.hpp"
#include "ivps.hpp"
#include "linker.hpp"
#include "rtspstream.hpp"
#include "ffmpegstream.hpp"
#include "vdec.hpp"

#define TAG "APP"
using namespace std;

#define IVPS_TOTAL_CHN_NUM (1)
#define IVPS_LINKED_CHN_ID (0)
#define IVPS_CHN_FIFO_DEPTH (2)
#define SKEL_MODEL_PATH ("/opt/etc/skelModels/1024x576/part")
static CVO *CreateAndSetupVO(AX_U32 nTotalChns);
static IStream *CreateAndInitStream(const std::string &strURL, INPUT_TYPE_E enInput);
static CVDEC *CreateAndInitDecoder(IStream *streamObj, const AX_VO_RECT_T &voChnWin);
static CIVPS *CreateAndInitIvps(const AX_VO_RECT_T &voChnWin);
static CDetectObserver *pDetectObs;
static CDetector gDetect;
static CLinker myLinker;

int main(int argc, char *argv[]) {
    cmdline::parser a;
    a.add<AX_U32>("num", 'n', "number of streamer", false, 4, cmdline::range(1, MAX_STREAM_COUNT));
    a.add<AX_U32>("input", 'i', "0: file 1: rtsp", false, 1, cmdline::range(0, 1));
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
    vector<string> strURLs(TOTAL_STREAM_COUNT);
    for (AX_U32 i = 0; i < TOTAL_STREAM_COUNT; ++i) {
        strURLs[i] = a.get<string>(urls[enInput][i].first);
    }

    APP_SYS_INIT(TOTAL_STREAM_COUNT, AX_TRUE, AX_TRUE);
    CRegionObserver *pRgnObs[TOTAL_STREAM_COUNT]{nullptr};

    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    DETECTOR_ATTR_T tDAttr;
    tDAttr.nSkipRate = 2;
    tDAttr.strModelPath = SKEL_MODEL_PATH;
    tDAttr.ePPL = AX_SKEL_PPL_HVCFP;
    tDAttr.nGrpCount = TOTAL_STREAM_COUNT;
    tDAttr.nChannelNum = 3;
    // fixme
    tDAttr.nGrp = 2;
    tDAttr.nWidth = 1024;
    tDAttr.nHeight = 576;

    AX_ENGINE_NPU_ATTR_T tNpuAttr;
    memset(&tNpuAttr, 0, sizeof(AX_ENGINE_NPU_ATTR_T));
    tNpuAttr.eHardMode = AX_ENGINE_VIRTUAL_NPU_STD;
    AX_S32 nRet = AX_ENGINE_Init(&tNpuAttr);
    if (AX_SUCCESS != nRet) {
        printf("AX_engine_init error\n");
    }
    // tDAttr.nGrp = tDstMod.nGroup;
    gDetect.Init(tDAttr);
    pDetectObs = new CDetectObserver(&gDetect);
    gDetect.Start();

    CVO *vo = CreateAndSetupVO(TOTAL_STREAM_COUNT);
    if (!vo) {
        return 1;
    }

    CDecodeTask::GetInstance()->Start();

    std::vector<CRtspStream *> streams(TOTAL_STREAM_COUNT);
    std::vector<CVDEC *> vdecs(TOTAL_STREAM_COUNT);
    std::vector<CIVPS *> ivpss(TOTAL_STREAM_COUNT);

    for (AX_U32 i = 0; i < TOTAL_STREAM_COUNT; ++i) {
        CIVPS *ivps = CreateAndInitIvps(vo->GetAttr().stChnInfo.arrChns[i].stRect);
        if (!ivps) {
            _exit(1);
        } else {
            ivpss[i] = ivps;
            pRgnObs[i] = new CRegionObserver(ivps->GetRegionHandle());
            gDetect.RegisterObserver(i, pRgnObs[i]);
        }

        IStream *stream = CreateAndInitStream(strURLs[i], enInput);
        if (!stream) {
            _exit(1);
        } else {
            streams[i] = stream;
        }

        CVDEC *vdec = CreateAndInitDecoder(stream, vo->GetAttr().stChnInfo.arrChns[i].stRect);
        if (!vdec) {
            _exit(1);
        } else {
            vdecs[i] = vdec;
        }

        stream->RegisterObserver(vdec);
        vo->SetChnFps(i, (AX_F32)stream->GetStreamInfo().stVideo.nFps);

        for (AX_S32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
            const VDEC_CHN_ATTR_T &stChnAttr = vdec->GetAttr().stChnAttr[j];
            if (stChnAttr.bEnable) {
                if (stChnAttr.bLinked) {
                    AX_MOD_INFO_T src = {AX_ID_VDEC, (AX_S32)(vdec->GetGrpId()), j};
                    AX_MOD_INFO_T dst = {AX_ID_IVPS, (AX_S32)(ivps->GetGrpId()), (AX_S32)IVPS_LINKED_CHN_ID};
                    myLinker.Link(src, dst);

                    src = dst;
                    dst = {AX_ID_VO, (AX_S32)(vo->GetAttr().voLayer), (AX_S32)i};
                    myLinker.Link(src, dst);
                } else {
                    vdec->RegisterObserver(j, pDetectObs);
                }
            }
        }

        stream->Start();
    }

    while (!IS_APP_QUIT()) {
        sleep(1);
    }

    myLinker.UnlinkAll();

    if (vo) {
        vo->Stop();
        vo->Destory();
    }

    for (auto &&m : streams) {
        if (m) {
            m->Stop();
            m->DeInit();
            delete m;
        }
    }

    for (auto &&m : ivpss) {
        if (m) {
            m->Stop();
            m->Destory();
        }
    }

    for (auto &&m : vdecs) {
        if (m) {
            m->Stop();
            m->Destory();
        }
    }

    for (AX_U32 i = 0; i < TOTAL_STREAM_COUNT; i++) {
        delete pRgnObs[i];
        pRgnObs[i] = nullptr;
    }

    if (pDetectObs) {
        delete pDetectObs;
        pDetectObs = nullptr;
    }

    gDetect.Stop();
    gDetect.DeInit();
    AX_ENGINE_Deinit();

    CDecodeTask::GetInstance()->Stop();

    return 0;
}

static CVO *CreateAndSetupVO(AX_U32 nTotalChns) {
    VO_ATTR_T stAttr;
    stAttr.dev = 0;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = AX_VO_OUTPUT_1080P60;
    stAttr.nLayerDepth = 3;
    stAttr.nBgClr = 0x0;
    stAttr.stChnInfo = InitLayout(1920, 1080, nTotalChns);
    CVO *obj = CVO::CreateInstance(stAttr);
    if (!obj) {
        return nullptr;
    }

    if (!obj->Start()) {
        obj->Destory();
        obj = nullptr;
        return nullptr;
    }

    return obj;
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

static CVDEC *CreateAndInitDecoder(IStream *streamObj, const AX_VO_RECT_T &voChnWin) {
    const STREAM_INFO_T &streamInfo = streamObj->GetStreamInfo();
    VDEC_ATTR_T stAttr;
    stAttr.enCodecType = streamInfo.stVideo.enPayload;
    stAttr.nWidth = ALIGN_UP(streamInfo.stVideo.nWidth, 16);
    stAttr.nHeight = ALIGN_UP(streamInfo.stVideo.nHeight, 16);
    stAttr.enDecodeMode = AX_VDEC_DISPLAY_MODE_PREVIEW;
    stAttr.bPrivatePool = AX_TRUE;
    stAttr.nTimeOut = (AX_VDEC_DISPLAY_MODE_PLAYBACK == stAttr.enDecodeMode) ? -1 : 100;

    for (AX_S32 j = 0; j < MAX_VDEC_CHN_NUM; ++j) {
        switch (j) {
            case 0:
                stAttr.stChnAttr[j].bEnable = AX_FALSE;
                // stAttr.stChnAttr[j].bLinked = AX_FALSE;
                // stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = 8;
                // stAttr.stChnAttr[j].stAttr.u32PicWidth = streamInfo.stVideo.nWidth;
                // stAttr.stChnAttr[j].stAttr.u32PicHeight = streamInfo.stVideo.nHeight;
                // stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                // stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_ORIGINAL;
                // stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                break;
            case 1:
                stAttr.stChnAttr[j].bEnable = AX_TRUE;
                stAttr.stChnAttr[j].bLinked = AX_TRUE;
                stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = 0;
                stAttr.stChnAttr[j].stAttr.u32PicWidth = voChnWin.u32Width;
                stAttr.stChnAttr[j].stAttr.u32PicHeight = voChnWin.u32Height;
                stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                break;
            case 2:
                stAttr.stChnAttr[j].bEnable = AX_TRUE;
                stAttr.stChnAttr[j].bLinked = AX_FALSE;
                stAttr.stChnAttr[j].stAttr.u32OutputFifoDepth = 8;
                stAttr.stChnAttr[j].stAttr.u32PicWidth = 1024;
                stAttr.stChnAttr[j].stAttr.u32PicHeight = 576;
                stAttr.stChnAttr[j].stAttr.u32FrameStride = ALIGN_UP(stAttr.stChnAttr[j].stAttr.u32PicWidth, VDEC_STRIDE_ALIGN);
                stAttr.stChnAttr[j].stAttr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                stAttr.stChnAttr[j].stAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;

                break;
            default:
                break;
        }

        if (stAttr.stChnAttr[j].bEnable) {
            AX_VDEC_CHN_ATTR_T &stChnAttr = stAttr.stChnAttr[j].stAttr;
            stChnAttr.u32FrameBufSize = CVDEC::GetBlkSize(stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stAttr.enCodecType,
                                                          &stChnAttr.stCompressInfo, stChnAttr.enImgFormat);
            stChnAttr.u32FrameBufCnt = 8;
        }
    }

    CVDEC *obj = CVDEC::CreateInstance(stAttr);
    if (obj) {
        if (!obj->Start()) {
            obj->Destory();
            return nullptr;
        }
    }

    return obj;
}

static CIVPS *CreateAndInitIvps(const AX_VO_RECT_T &voChnWin) {
    IVPS_CHN_ATTR_T stChn[IVPS_TOTAL_CHN_NUM];
    for (AX_U32 i = 0; i < IVPS_TOTAL_CHN_NUM; ++i) {
        stChn[i].enEngine = AX_IVPS_ENGINE_VPP;
        stChn[i].nWidth = voChnWin.u32Width;
        stChn[i].nHeight = voChnWin.u32Height;
        stChn[i].nStride = ALIGN_UP(stChn[i].nWidth, 16);
        stChn[i].stPoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
        if (IVPS_LINKED_CHN_ID == i) {
            stChn[i].bLinked = AX_TRUE;
            stChn[i].nFifoDepth = 0;
            stChn[i].stPoolAttr.nFrmBufNum = VO_CHN_FIFO_DEPTH + 1;
        } else {
            stChn[i].bLinked = AX_FALSE;
            stChn[i].nFifoDepth = VO_CHN_FIFO_DEPTH;
            stChn[i].stPoolAttr.nFrmBufNum = stChn[i].nFifoDepth + 1;
        }
    }

    IVPS_ATTR_T stAttr;
    stAttr.nChnNum = IVPS_TOTAL_CHN_NUM;
    for (AX_U32 i = 0; i < IVPS_TOTAL_CHN_NUM; ++i) {
        stAttr.stChnAttr[i] = std::move(stChn[i]);
    }

    CIVPS *obj = CIVPS::CreateInstance(stAttr);
    if (obj) {
        for (AX_U32 i = 0; i < IVPS_TOTAL_CHN_NUM; ++i) {
            if (!stChn[i].bLinked) {
                obj->RegisterObserver(i, pDetectObs);
            }
        }

        if (!obj->Start()) {
            obj->Destory();
            obj = nullptr;
        }
    }

    return obj;
}