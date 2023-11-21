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
#include <string>
#include <vector>
#include "vo.hpp"

#define MAX_STREAM_COUNT (32)

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

#define VO_CHN_FIFO_DEPTH (2)

#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))

/* VDEC stride align 256 */
#define VDEC_STRIDE_ALIGN AX_SHIFT_LEFT_ALIGN(8)

VO_CHN_INFO_T InitLayout(AX_U32 nW, AX_U32 nH, AX_U32 nVideoCount);
extern int ping4(const char *ip, int timeout);

class CAppSys {
public:
    CAppSys(AX_U32 vdGrpNum, AX_BOOL bVO, AX_BOOL bIVPS);
    ~CAppSys(AX_VOID);

    AX_BOOL IsAppQuit(AX_VOID);

private:
    AX_U32 m_vdGrpNum;
    AX_BOOL m_bVO;
    AX_BOOL m_bIVPS;
};

#define APP_SYS_INIT(vdecGrpNum, vo, ivps) CAppSys theApp(vdecGrpNum, vo, ivps)
#define IS_APP_QUIT theApp.IsAppQuit

typedef enum {
    INPUT_FILE = 0,
    INPUT_RTSP = 1,
    INPUT_BUTT
} INPUT_TYPE_E;

using URL_ARGS = std::vector<std::pair<std::string, std::string>>;
class CAppArgs {
public:
    CAppArgs(AX_VOID) = default;
    static URL_ARGS GetUrls(INPUT_TYPE_E eInput);
};
