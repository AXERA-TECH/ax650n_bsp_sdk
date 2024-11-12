/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once
#include <string.h>
#include <map>
#include <vector>
#include <tuple>

using namespace std;

#include "ax_base_type.h"
#include "rtspstream.hpp"

#define DEL_PTR(ptr) { if (ptr) { delete ptr; ptr=nullptr; } }
// #define SafeDelete(ptr) { try { delete ptr; } catch (...) { ASSERT(FALSE); } ptr=NULL; }

#define MAX_DEVICE_COUNT                (64)
#define MAX_DEVICE_VDEC_COUNT           (64+32+1)
#define MAX_DEVICE_STREAM_COUNT         (2)
#define MAX_DEVICE_PLAYBACK_COUNT       (16)
#define MAX_DEVICE_ROUNDPATROL_COUNT    (32)
#define NVR_VERSION_STR                 ("v0.1")

#define NVR_PIP_CHN_NUM                 (MAX_DEVICE_COUNT)
#define MAX_PRIMARY_DISP_CHN_COUNT      (MAX_DEVICE_COUNT + 1) /* 1: pip */

// channel
typedef AX_S32              AX_NVR_DEV_ID;
// index and devid(channel)
typedef vector<tuple<int, AX_NVR_DEV_ID>>   ax_nvr_channel_vector;


// DebugDefine
// #define USE_COLORBAR_FB0_FB1
#define USE_FFMPEGSTREAM

// Display Define
enum class AX_NVR_DISPDEV_TYPE {
    PRIMARY     = 0,
    SECOND      = 1,
};

enum class AX_NVR_VIEW_TYPE {
    PREVIEW     = 0,
    PLAYBACK    = 1,
    POLLING     = 2,
    PIP         = 3,
    OTHERS      = 99,   // without VO
};

enum class AX_NVR_VIEW_CHANGE_TYPE {
    UNKNOW     = -1,
    SHOW       = 0,    // create vo channels
    HIDE       = 1,    // destory vo channels
    UPDATE     = 2,
    MIN        = 3,
    MAX        = 4,
    MAINSUB    = 5,
};

enum class AX_NVR_VO_SPLITE_TYPE {
     UNKNOW     = -1,
     ONE        = 1,     // 1x1: 1
     FOUR       = 2,     // 2x2: 4
     EIGHT      = 7,     // 1+7: 8
     SIXTEEN    = 4,     // 4x4:16
     THIRTYSIX  = 6,     // 6x6
     SIXTYFOUR  = 8,     // 8x8:64
     MAX        = 99
};

typedef struct _AX_NVR_RECT_T {
    bool bShow = true;
    unsigned int x = {0};
    unsigned int y = {0};
    unsigned int w = {0};
    unsigned int h = {0};
} AX_NVR_RECT_T;

typedef struct _AX_NVR_VO_SPLITE_WINDOW_T {
    unsigned int nIndex;
    bool bDisp;
    AX_NVR_RECT_T rect;
} AX_NVR_VO_SPLITE_WINDOW_T;

// enum class AX_NVR_CHN_URL_TYPE {
//     MAIN       = 0,
//     SUB1       = 1,
// };

enum class AX_NVR_CHN_IDX_TYPE {
    MAIN            = 0x00,
    SUB1            = 0x01,
};

enum class AX_NVR_CHN_VIEW_TYPE {
    PREVIEW            = 0x00,
    PATROL             = 0x01,
    PLAYBACK           = 0x02,
};

enum class AX_NVR_CHN_TYPE {
    COMMON     = 0,
    FISHEYE    = 1,
};

enum class AX_NVR_TS_RUN_MODE {
    DISABLE   = 0,
    STABILITY = 1,
    UT        = 2,
};

typedef struct _AX_NVR_DEV_CHN_INFO_T {
    AX_BOOL bRecord;
    unsigned char szRtspUrl[128];
    _AX_NVR_DEV_CHN_INFO_T() {
        bRecord = AX_TRUE;
        strcpy((char*)szRtspUrl, "rtsp://192.168.2.8/test.264");
    };

} AX_NVR_DEV_CHN_INFO_T;

typedef struct _AX_NVR_DEV_INFO_T {
    AX_NVR_DEV_ID nChannelId;       // device id, channel id while preview
    AX_U8 szAlias[64];              // alias name
    AX_NVR_CHN_TYPE enType;          // common/fisheyc

    AX_NVR_CHN_IDX_TYPE enPreviewIndex;    // main/sub1 preview
    AX_BOOL bPreviewDisplay;

    AX_NVR_CHN_IDX_TYPE enPatrolIndex;    // main/sub1 pratrol
    AX_BOOL bPatrolDisplay;

    AX_NVR_DEV_CHN_INFO_T stChnMain;
    AX_NVR_DEV_CHN_INFO_T stChnSub1;

    _AX_NVR_DEV_INFO_T() {
        nChannelId = -1;
        strcpy((char*)szAlias, "remote device");
        enType = AX_NVR_CHN_TYPE::COMMON;

        enPreviewIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
        bPreviewDisplay = AX_TRUE;

        enPatrolIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
        bPatrolDisplay = AX_TRUE;

        stChnSub1.bRecord = AX_FALSE;
    };

} AX_NVR_DEV_INFO_T;

// using URL_ARGS = std::vector<std::pair<std::string, std::string>>;

enum class AX_NVR_RPATROL_TYPE {
    SPLIT       = 0,    // Multi-Grid switching
    CHANNELS    = 1,    // Multi-Channels switching
    BOTH        = 2,    // Multi-Grid-Channels switching
};

// round patrol all channels
typedef struct {
    AX_BOOL bEnable;
    AX_NVR_RPATROL_TYPE enType;
    AX_NVR_VO_SPLITE_TYPE enSpliteType;     // only for CHANNELS Type
    AX_U32 uIntelval;                       // [5, 120] second
    AX_S32 nStrategy;                       // 0: ascending; 1: round; 2: random
} AX_NVR_RPATROL_CONFIG_T;

enum class AX_NVR_DETECT_SRC_TYPE {
    NONE    = 0,
    IVPS    = 1,
    VDEC    = 2,
};

// detect
typedef struct {
    AX_U32 nPPL;
    AX_U32 nVNPU;
    AX_BOOL bTrackEnable;
} AX_NVR_DETECT_CHN_PARAM_T;

typedef struct {
    AX_BOOL bEnable;
    AX_U32 nW;
    AX_U32 nH;
    AX_U32 nSkipRate;
    AX_S32 nDepth;
    AX_S32 nChannelNum;
    AX_NVR_DETECT_CHN_PARAM_T tChnParam[3];
    std::string strModelPath;
} AX_NVR_DETECT_CONFIG_T;

// display
typedef struct {
    AX_S32 nDevId;    // if -1, disable
    AX_S32 nFBQt;
    AX_S32 nFBRect;
    AX_U32 nHDMI;
    AX_U32 nChnDepth;
    AX_U32 nLayerDepth;
    AX_U32 nTolerance;
    AX_BOOL bLink; /* linkage of vo->disp */
    AX_U32 nOnlineMode;
} AX_NVR_DISPVO_CONFIG_T;

// record
typedef struct {
    std::string strPath;
    AX_U32 uMaxDevSpace;
    AX_U32 uMaxFilePeriod;
    AX_BOOL bOnlyIFrameOnReverse; /* 是否仅倒放I帧 */
} AX_NVR_RECORD_CONFIG_T;

// remote device source
typedef struct {
    std::string strPath;
} AX_NVR_DEVICE_CONFIG_T;

typedef struct {
    AX_BOOL bSaveDisk;
    AX_U32 nFrequency; // frequency of result feedback, unit: seconds
} AX_NVR_DATA_STREAM_CONFIG_T;

// test suite
typedef struct {
    AX_NVR_TS_RUN_MODE eMode;
    std::string strStabilityPath;
    std::string strUTPath;
} AX_NVR_TEST_SUITE_CONFIG_T;

// FBC
typedef struct {
    AX_U32 nLv; // lossy level
} AX_NVR_FBC_CONFIG_T;
