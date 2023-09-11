/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ax_sys_api.h"
#include "ax_venc_api.h"
#include "common_venc.h"
#include "sample_case_api.h"
#include "sample_cmd_params.h"
#include "sample_pool.h"
#include "sample_unit_test.h"
#include "sample_venc_log.h"


SAMPLE_VENC_CMD_PARA_T stCmdPara;

AX_VENC_MOD_ATTR_T stModAttr = {
    .enVencType = AX_VENC_MULTI_ENCODER,
    .stModThdAttr.u32TotalThreadNum = 1,
    .stModThdAttr.bExplicitSched = AX_FALSE,
};

extern AX_BOOL gVencLogLevel;

int main(int argc, char *argv[])
{
    SAMPLE_LOG("Build at %s %s\n", __DATE__, __TIME__);

    AX_S32 s32Ret = -1;

    SampleSetDefaultParams(&stCmdPara);

    s32Ret = SampleCmdLineParse(argc, argv, &stCmdPara);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("Invalid input argument!\n");
        return -1;
    }

    gVencLogLevel = stCmdPara.logLevel;
    s32Ret = SampleMemInit(&stCmdPara);
    if (s32Ret) {
        SAMPLE_LOG_ERR("sample memory init err.\n");
        return -1;
    }

    stModAttr.stModThdAttr.u32TotalThreadNum = stCmdPara.encThdNum;

    s32Ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_Init error.\n");
        goto FREE_MEM;
    }

    SampleTestCaseStart(&stCmdPara);

    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret)
        SAMPLE_LOG_ERR("AX_VENC_Deinit failed! Error Code:0x%X\n", s32Ret);

    SampleMemDeinit(&stCmdPara);

    return 0;

FREE_MEM:
    SampleMemDeinit(&stCmdPara);

    return -1;
}
