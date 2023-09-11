/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "common_vdec_api.h"

AX_U64 g_u64GetFrmTag = 0;

int main(int argc, char *argv[])
{
    AX_S32 s32Ret = 0;
    AX_S32 sRet = 0;
    int ret = 0;

    SAMPLE_VDEC_CMD_PARAM_T *pstCmd = NULL;
    SAMPLE_VDEC_CONTEXT_T *pstVdecCtx = NULL;

    s32Ret = Sample_VdecTestBenchInit(argc, argv, &pstVdecCtx);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("Sample_VdecTestBenchInit FAILED! ret:0x%x\n", s32Ret);
        goto ERR_RET;
    }

    pstCmd = &pstVdecCtx->tCmdParam;
    ret = VdecDefaultParamsSet(pstCmd);
    if (ret) {
        SAMPLE_CRIT_LOG("VdecDefaultParamsSet FAILED! ret:0x%x\n", ret);
        goto ERR_RET;
    }

    ret = VdecCmdLineParseAndCheck(argc, argv, pstCmd, 0, 0);
    if (ret) {
        SAMPLE_CRIT_LOG("VdecCmdLineParseAndCheck FAILED! ret:0x%x\n", ret);
        goto ERR_RET;
    }

    g_u64GetFrmTag = 0;
    s32Ret = AX_SYS_Init();
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("AX_SYS_Init FAILED! ret:0x%x\n", s32Ret);
        goto ERR_RET;
    }

    AX_VDEC_MOD_ATTR_T stModAttr;
    stModAttr.enDecModule = pstCmd->enDecModule;
    stModAttr.u32MaxGroupCount = pstCmd->uMaxGrpCnt;

    s32Ret = AX_VDEC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("AX_VDEC_Init FAILED! ret:0x%x %s\n", s32Ret, AX_VdecRetStr(s32Ret));
        goto ERR_RET_SYS_DEINIT;
    }

    if (pstCmd->bJpegDecOneFrm) {
        s32Ret = Sample_VdecJpegDecodeOneFrame(pstCmd);
        if(0 != s32Ret) {
            SAMPLE_CRIT_LOG("Sample_VdecJpegDecodeOneFrame FAILED! ret:0x%x\n", s32Ret);
            goto ERR_RET_VDEC_DEINIT;
        }

        goto FUNC_QUIT;
    }

    s32Ret = Sample_VdecTestBenchMain(&pstVdecCtx);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_CRIT_LOG("Sample_VdecTestBenchMain FAILED! ret:0x%x\n", s32Ret);
        goto ERR_RET_VDEC_DEINIT;
    }

    if (NULL != pstVdecCtx) {
        for (int ci = 0; ci < AX_VDEC_MAX_CHN_NUM; ci++) {
            if (NULL != pstCmd->tChnCfg[ci].pOutputFilePath) {
                free(pstCmd->tChnCfg[ci].pOutputFilePath);
                pstCmd->tChnCfg[ci].pOutputFilePath = NULL;
            }

            if (NULL != pstCmd->tChnCfg[ci].pUsrPicFilePath) {
                free(pstCmd->tChnCfg[ci].pUsrPicFilePath);
                pstCmd->tChnCfg[ci].pUsrPicFilePath = NULL;
            }
        }

        if (NULL != pstCmd->pInputFilePath) {
            free(pstCmd->pInputFilePath);
            pstCmd->pInputFilePath = NULL;
        }

        free(pstVdecCtx);
        pstVdecCtx = NULL;
    }

FUNC_QUIT:
    sRet = AX_VDEC_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_VDEC_Deinit FAILED! ret:0x%x %s\n", sRet, AX_VdecRetStr(sRet));
        return sRet;
    }

    if (g_u64GetFrmTag) {
        SAMPLE_LOG_T("sample_vdec running status: Decode Finished! \n\n");
    } else {
        SAMPLE_CRIT_LOG("sample_vdec running status: Decode Failed! please check code \n\n");
    }

    sRet = AX_SYS_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_SYS_Deinit FAILED! ret:0x%x\n", sRet);
        return sRet;
    }


    return 0;

ERR_RET_VDEC_DEINIT:
    sRet = AX_VDEC_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_VDEC_Deinit FAILED! ret:0x%x %s\n", sRet, AX_VdecRetStr(sRet));
    }
ERR_RET_SYS_DEINIT:
    sRet = AX_SYS_Deinit();
    if (AX_SUCCESS != sRet) {
        SAMPLE_CRIT_LOG("AX_SYS_Deinit FAILED! ret:0x%x\n", sRet);
    }
ERR_RET:
    if (NULL != pstVdecCtx) {
        for (int ci = 0; ci < AX_VDEC_MAX_CHN_NUM; ci++) {
            if (NULL != pstCmd->tChnCfg[ci].pOutputFilePath) {
                free(pstCmd->tChnCfg[ci].pOutputFilePath);
                pstCmd->tChnCfg[ci].pOutputFilePath = NULL;
            }

            if (NULL != pstCmd->tChnCfg[ci].pUsrPicFilePath) {
                free(pstCmd->tChnCfg[ci].pUsrPicFilePath);
                pstCmd->tChnCfg[ci].pUsrPicFilePath = NULL;
            }
        }

        if (NULL != pstCmd->pInputFilePath) {
            free(pstCmd->pInputFilePath);
            pstCmd->pInputFilePath = NULL;
        }

        free(pstVdecCtx);
        pstVdecCtx = NULL;
    }

    return s32Ret || sRet || ret;
}
