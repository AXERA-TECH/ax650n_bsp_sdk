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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "ax_isp_api.h"
#include "ax_vin_error_code.h"
#include "sample_vin_hal.h"

static AX_CAMERA_T gCams[MAX_CAMERAS] = {0};
COMMON_SYS_ARGS_T gCommonArgs = {0};


static AX_VOID set_pipe_hdr_mode(AX_U32 *pHdrSel, AX_SNS_HDR_MODE_E eHdrMode)
{
    if (NULL == pHdrSel) {
        return;
    }

    switch (eHdrMode) {
    case AX_SNS_LINEAR_MODE:
        *pHdrSel = 0x1;
        break;

    case AX_SNS_HDR_2X_MODE:
        *pHdrSel = 0x1 | 0x2;
        break;

    case AX_SNS_HDR_3X_MODE:
        *pHdrSel = 0x1 | 0x2 | 0x4;
        break;

    case AX_SNS_HDR_4X_MODE:
        *pHdrSel = 0x1 | 0x2 | 0x4 | 0x8;
        break;

    default:
        *pHdrSel = 0x1;
        break;
    }
}


static AX_U32 __vin_cfg_cams_params(SAMPLE_VIN_HAL_CASE_E eVinCase,
                                    COMMON_VIN_MODE_E eSysMode,
                                    AX_SNS_HDR_MODE_E eHdrMode,
                                    AX_BOOL bAiispEnable,
                                    COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32              i = 0;
    SAMPLE_SNS_TYPE_E   eSnsType = OMNIVISION_OS08A20;

    COMM_ISP_PRT("eVinCase %d, eSysMode %d, eHdrMode %d\n", eVinCase, eSysMode, eHdrMode);

    if (eVinCase == SAMPLE_VIN_HAL_CASE_SINGLE_DUMMY) {
        pCommonArgs->nCamCnt = 1;
        eSnsType = SAMPLE_SNS_DUMMY;
        COMMON_VIN_GetSnsConfig(SAMPLE_SNS_DUMMY,
                                &gCams[0].tMipiRx,
                                &gCams[0].tSnsAttr,
                                &gCams[0].tSnsClkAttr,
                                &gCams[0].tDevAttr,
                                &gCams[0].tPipeAttr, gCams[0].tChnAttr);
    } else if (eVinCase == SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20) {
        pCommonArgs->nCamCnt = 1;
        eSnsType = OMNIVISION_OS08A20;
        COMMON_VIN_GetSnsConfig(OMNIVISION_OS08A20,
                                &gCams[0].tMipiRx,
                                &gCams[0].tSnsAttr,
                                &gCams[0].tSnsClkAttr,
                                &gCams[0].tDevAttr,
                                &gCams[0].tPipeAttr, gCams[0].tChnAttr);

        gCams[0].nDevId = 0;
        gCams[0].nRxDev = 0;
        gCams[0].nPipeId = 0;
        gCams[0].tSnsClkAttr.nSnsClkIdx = 0;
        gCams[0].tDevBindPipe.nNum =  1;
        gCams[0].tDevBindPipe.nPipeId[0] = gCams[0].nPipeId;
        set_pipe_hdr_mode(&gCams[0].tDevBindPipe.nHDRSel[0], eHdrMode);

    }

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        gCams[i].eSnsType = eSnsType;
        gCams[i].tSnsAttr.eSnsMode = eHdrMode;
        gCams[i].tDevAttr.eSnsMode = eHdrMode;
        gCams[i].tPipeAttr.eSnsMode = eHdrMode;
        gCams[i].eHdrMode = eHdrMode;
        gCams[i].eSysMode = eSysMode;
        gCams[i].tPipeAttr.bAiIspEnable = bAiispEnable;
        for (AX_S32 j = 0; j < AX_VIN_MAX_PIPE_NUM; j++) {
            gCams[i].tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            gCams[i].tPipeInfo[j].bAiispEnable = bAiispEnable;
            strncpy(gCams[i].tPipeInfo[j].szBinPath, "null.bin", sizeof(gCams[i].tPipeInfo[j].szBinPath));
        }

        if (eHdrMode > AX_SNS_LINEAR_MODE) {
            gCams[i].tDevAttr.eSnsOutputMode = AX_SNS_DOL_HDR;
        } else {
            gCams[i].tSnsAttr.eRawType = AX_RT_RAW12;
            gCams[i].tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            gCams[i].tPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        }
        gCams[i].bRegisterSns = AX_TRUE;
        if (COMMON_VIN_LOADRAW == eSysMode) {
            gCams[i].bEnableDev = AX_FALSE;
        } else {
            gCams[i].bEnableDev = AX_TRUE;
        }
    }

    return 0;
}

AX_S32 SAMPLE_VIN_Init(SAMPLE_VIN_HAL_CASE_E eVinCase, COMMON_VIN_MODE_E eSysMode, AX_SNS_HDR_MODE_E eHdrMode,
                       AX_BOOL bAiispEnable, COMMON_SYS_ARGS_T *pPrivPool)
{
    AX_S32 nRet = 0;

    if (AX_TRUE == bAiispEnable) {
        if (eVinCase != SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20) {
            COMM_ISP_PRT("when parameter -a 1,parameter -c: ISP Pipeline Case must be 1\n");
            exit(0);
        }
    }
    if (eVinCase >= SAMPLE_VIN_HAL_CASE_BUTT || eVinCase <= SAMPLE_VIN_HAL_CASE_NONE) {
        COMM_ISP_PRT("error sys case : %d\n", eVinCase);
        exit(0);
    }
    if (eSysMode >= COMMON_VIN_BUTT || eSysMode <= COMMON_VIN_NONE) {
        COMM_ISP_PRT("error sys mode : %d\n", eSysMode);
        exit(0);
    }

    __vin_cfg_cams_params(eVinCase, eSysMode, eHdrMode,
                          bAiispEnable, &gCommonArgs);
    nRet = COMMON_NPU_Init(AX_ENGINE_VIRTUAL_NPU_BIG_LITTLE);
    if (nRet) {
        return nRet;
    }
    nRet = COMMON_CAM_Init();
    if (nRet) {
        return nRet;
    }
    if (pPrivPool) {
        nRet = COMMON_CAM_PrivPoolInit(pPrivPool);
        if (nRet) {
            return nRet;
        }
    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_VIN_Open(AX_VOID)
{
    AX_CAMERA_T *pCam = &gCams[0];
    AX_U8 nCamCnt = gCommonArgs.nCamCnt;
    AX_S32 nRet = 0;
    AX_S32 i = 0, j = 0;

    nRet = COMMON_CAM_Open(pCam, nCamCnt);

    /* Stream default port 6000, Ctrl default port 8082 */
    nRet = COMMON_NT_Init(6000, 8082);
    if (nRet) {
        return nRet;
    }
    /* update pipe attribute */
    for (i = 0; i < gCommonArgs.nCamCnt; i++) {
        for (j = 0; j < gCams[i].tDevBindPipe.nNum; j++) {
            COMMON_NT_UpdateSource(gCams[i].tDevBindPipe.nPipeId[j]);
        }
    }

    return nRet;
}

AX_S32 SAMPLE_VIN_Close(AX_VOID)
{
    AX_CAMERA_T *pCam = &gCams[0];
    AX_U8 nCamCnt = gCommonArgs.nCamCnt;

    return COMMON_CAM_Close(pCam, nCamCnt);
}

AX_S32 SAMPLE_VIN_Start(AX_VOID)
{
    AX_CAMERA_T *pCam = &gCams[0];
    AX_U8 nCamCnt = gCommonArgs.nCamCnt;

    return COMMON_CAM_Run(pCam, nCamCnt);
}

AX_S32 SAMPLE_VIN_Stop(AX_VOID)
{
    return COMMON_CAM_Stop();
}

AX_S32 SAMPLE_VIN_DeInit(AX_VOID)
{
    COMMON_NT_DeInit();
    COMMON_CAM_Deinit();

    return AX_SUCCESS;
}
