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
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "ax_vin_api.h"
#include "ax_isp_api.h"
#include "ax_vin_error_code.h"
#include "ax_mipi_rx_api.h"
#include "common_cam.h"
#include "common_sys.h"
#include "common_type.h"
#include "ax_isp_3a_api.h"

static pthread_t gDispatchThread[MAX_CAMERAS] = {0};
static AX_S32 g_dispatcher_loop_exit = 0;

AX_S32 COMMON_NPU_Init(AX_ENGINE_NPU_MODE_T eHardMode)
{
    AX_S32 axRet = 0;

    /* NPU Init */
    AX_ENGINE_NPU_ATTR_T npu_attr;
    memset(&npu_attr, 0x0, sizeof(npu_attr));
    npu_attr.eHardMode = eHardMode;
    axRet = AX_ENGINE_Init(&npu_attr);
    if (0 != axRet) {
        COMM_CAM_PRT("AX_ENGINE_Init failed, ret=0x%x.\n", axRet);
        return -1;
    }
    return 0;
}

AX_S32 COMMON_CAM_PrivPoolInit(COMMON_SYS_ARGS_T *pPrivPoolArgs)
{
    AX_S32 axRet = 0;
    AX_POOL_FLOORPLAN_T tPoolFloorPlan = {0};

    if (pPrivPoolArgs == NULL) {
        return -1;
    }

    /* Calc Pool BlkSize/BlkCnt */
    axRet = COMMON_SYS_CalcPool(pPrivPoolArgs->pPoolCfg, pPrivPoolArgs->nPoolCfgCnt, &tPoolFloorPlan);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_SYS_CalcPool failed, ret=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetPoolAttr(&tPoolFloorPlan);
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_SetPoolAttr fail!Error Code:0x%X\n", axRet);
        return -1;
    } else {
        printf("AX_VIN_SetPoolAttr success!\n");
    }

    return 0;
}

AX_S32 COMMON_CAM_Init(AX_VOID)
{
    AX_S32 axRet = 0;

    /* VIN Init */
    axRet = AX_VIN_Init();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_Init failed, ret=0x%x.\n", axRet);
        return -1;
    }

    /* MIPI init */
    axRet = AX_MIPI_RX_Init();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_MIPI_RX_Init failed, ret=0x%x.\n", axRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_CAM_Deinit(AX_VOID)
{
    AX_S32 axRet = 0;

    axRet = AX_MIPI_RX_DeInit();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_MIPI_RX_DeInit failed, ret=0x%x.\n", axRet);
        return -1 ;
    }

    axRet = AX_VIN_Deinit();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_DeInit failed, ret=0x%x.\n", axRet);
        return -1 ;
    }

    axRet = AX_ENGINE_Deinit();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_ENGINE_Deinit failed, ret=0x%x.\n", axRet);
        return -1 ;
    }

    return axRet;
}

static AX_S32 __common_cam_open(AX_CAMERA_T *pCam)
{
    AX_S32 i = 0;
    AX_S32 axRet = 0;
    AX_U8 nPipeId = 0;
    AX_U8 nDevId = pCam->nDevId;
    AX_MIPI_RX_DEV_E nRxDev = pCam->nRxDev;
    SAMPLE_SNS_TYPE_E eSnsType = pCam->eSnsType;

    axRet = COMMON_VIN_StartMipi(nRxDev, &pCam->tMipiRx);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StartMipi failed, r-et=0x%x.\n", axRet);
        return -1;
    }

    axRet = COMMON_VIN_CreateDev(nDevId, nRxDev, &pCam->tDevAttr, &pCam->tDevBindPipe, &pCam->tDevFrmIntAttr);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_CreateDev failed, ret=0x%x.\n", axRet);
        return -1;
    }

    if (pCam->bEnableFlash == AX_TRUE) {
        axRet = COMMON_VIN_StartOutsideDev(nDevId, &pCam->tPowerAttr, &pCam->tVsyncAttr, &pCam->tHsyncAttr,
                                                &pCam->tLightSyncInfo, &pCam->tSnapStrobeAttr, &pCam->tSnapFlashAttr);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_VIN_StartOutsideDev failed, ret=0x%x.\n", axRet);
            return -1;
        }
    }

    if (!pCam->bDevOnly) {
        for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
            nPipeId = pCam->tDevBindPipe.nPipeId[i];
            pCam->tPipeAttr.bAiIspEnable = pCam->tPipeInfo[i].bAiispEnable;
            axRet = COMMON_VIN_SetPipeAttr(pCam->eSysMode, nPipeId, &pCam->tPipeAttr);
            if (0 != axRet) {
                COMM_CAM_PRT("COMMON_ISP_SetPipeAttr failed, ret=0x%x.\n", axRet);
                return -1;
            }

            if (pCam->bRegisterSns) {
                axRet = COMMON_ISP_RegisterSns(nPipeId, nDevId, eSnsType);
                if (0 != axRet) {
                    COMM_CAM_PRT("COMMON_ISP_RegisterSns failed, ret=0x%x.\n", axRet);
                    return -1;
                }
                axRet = COMMON_ISP_SetSnsAttr(nPipeId, nDevId, &pCam->tSnsAttr, &pCam->tSnsClkAttr);
                if (0 != axRet) {
                    COMM_CAM_PRT("COMMON_ISP_SetSnsAttr failed, ret=0x%x.\n", axRet);
                    return -1;
                }
            }

            axRet = COMMON_ISP_Init(nPipeId, eSnsType, pCam->bRegisterSns, pCam->bUser3a,
                                    &pCam->tAeFuncs, &pCam->tAwbFuncs, &pCam->tAfFuncs,&pCam->tLscFuncs,
                                    pCam->tPipeInfo[i].szBinPath);
            if (0 != axRet) {
                COMM_CAM_PRT("COMMON_ISP_StartIsp failed, axRet = 0x%x.\n", axRet);
                return -1;
            }

            axRet = COMMON_VIN_StartChn(nPipeId, pCam->tChnAttr);
            if (0 != axRet) {
                COMM_CAM_PRT("COMMON_ISP_StartChn failed, nRet = 0x%x.\n", axRet);
                return -1;
            }

            axRet = AX_VIN_StartPipe(nPipeId);
            if (0 != axRet) {
                COMM_CAM_PRT("AX_VIN_StartPipe failed, ret=0x%x\n", axRet);
                return -1;
            }

            /* When there are multiple pipe, only the first pipe needs AE */
            if ((0 < i) && (pCam->bEnableFlash == AX_FALSE) && (AX_FALSE == pCam->bUser3a)) {
                axRet = COMMON_ISP_SetAeToManual(nPipeId);
                if (0 != axRet) {
                    COMM_CAM_PRT("COMMON_ISP_SetAeToManual failed, ret=0x%x\n", axRet);
                    return -1;
                }
            }
        }
    }

    axRet = COMMON_VIN_StartDev(nDevId, pCam->bEnableDev, &pCam->tDevAttr);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StartDev failed, ret=0x%x.\n", axRet);
        return -1;
    }

    if (pCam->bRegisterSns) {
        for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
            axRet = AX_ISP_StreamOn(pCam->tDevBindPipe.nPipeId[i]);
            if (0 != axRet) {
                COMM_CAM_PRT(" failed, ret=0x%x.\n", axRet);
                return -1;
            }
        }
    }

    return 0;
}

static AX_S32 __common_cam_close(AX_CAMERA_T *pCam)
{
    AX_U8 i = 0;
    AX_S32 axRet = 0;
    AX_U8 nPipeId = pCam->nPipeId;
    AX_U8 nDevId = pCam->nDevId;
    AX_MIPI_RX_DEV_E nRxDev = pCam->nRxDev;

    if (pCam->bEnableFlash == AX_TRUE) {
        axRet = COMMON_VIN_StopOutsideDev(nDevId);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_VIN_StopOutsideDev failed, ret=0x%x.\n", axRet);
            return -1;
        }
    }

    axRet = COMMON_VIN_StopDev(nDevId, pCam->bEnableDev);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StopDev failed, ret=0x%x.\n", axRet);
    }

    if (pCam->bRegisterSns) {
        for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
            AX_ISP_StreamOff(pCam->tDevBindPipe.nPipeId[i]);
        }
    }

    axRet = AX_ISP_CloseSnsClk(pCam->tSnsClkAttr.nSnsClkIdx);
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_CloseSnsClk failed, ret=0x%x.\n", axRet);
    }

    for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
        nPipeId = pCam->tDevBindPipe.nPipeId[i];
        axRet = AX_VIN_StopPipe(nPipeId);
        if (0 != axRet) {
            COMM_CAM_PRT("AX_VIN_StopPipe failed, ret=0x%x.\n", axRet);
        }

        COMMON_VIN_StopChn(nPipeId);

        COMMON_ISP_DeInit(nPipeId, pCam->bRegisterSns);

        COMMON_ISP_UnRegisterSns(nPipeId);

        AX_VIN_DestroyPipe(nPipeId);
    }

    axRet = COMMON_VIN_StopMipi(nRxDev);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StopMipi failed, ret=0x%x.\n", axRet);
    }

    axRet = COMMON_VIN_DestroyDev(nDevId);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_DestroyDev failed, ret=0x%x.\n", axRet);
    }

    COMM_CAM_PRT("%s: nDevId %d: exit.\n", __func__, nDevId);

    return AX_SUCCESS;
}

AX_S32 COMMON_CAM_Open(AX_CAMERA_T *pCamList, AX_U8 Num)
{
    AX_U16 i = 0;
    if (pCamList == NULL) {
        return -1;
    }

    for (i = 0; i < Num; i++) {
        if (AX_SUCCESS == __common_cam_open(&pCamList[i])) {
            pCamList[i].bOpen = AX_TRUE;
            COMM_CAM_PRT("camera %d is open\n", i);
        } else {
            goto EXIT;
        }
    }
    return 0;
EXIT:
    for (i = 0; i < Num; i++) {
        if (!pCamList[i].bOpen) {
            continue;
        }
        __common_cam_close(&pCamList[i]);
    }
    return -1;
}

AX_S32 COMMON_CAM_Close(AX_CAMERA_T *pCamList, AX_U8 Num)
{
    AX_U16 i = 0;
    if (pCamList == NULL) {
        return -1;
    }

    for (i = 0; i < Num; i++) {
        if (!pCamList[i].bOpen) {
            continue;
        }
        if (AX_SUCCESS == __common_cam_close(&pCamList[i])) {
            COMM_CAM_PRT("camera %d is close\n", i);
            pCamList[i].bOpen = AX_FALSE;
        } else {
            return -1;
        }
    }

    return 0;
}

static AX_BOOL SeqNumIsMatch(AX_U8 nDevId, AX_IMG_INFO_T *frameBufferArr,  AX_U64 *frameSeqs, AX_U64 maxFrameSeq,
                             AX_SNS_HDR_MODE_E eHdrMode)
{
    AX_BOOL frameSeqNotMatch = AX_FALSE;
    AX_S32 j = 0;

    do {
        frameSeqNotMatch = AX_FALSE;
        for (j = 0; j < eHdrMode; j++) {
            if (frameSeqs[j] < maxFrameSeq) {
                COMM_ISP_PRT("FrameSeq(%lld) doesn't match (max_frame_seq:%lld), drop blk_id: 0x%x\n",
                             frameSeqs[j], maxFrameSeq, frameBufferArr[j].tFrameInfo.stVFrame.u32BlkId[0]);
                AX_VIN_ReleaseDevFrame(nDevId, j, frameBufferArr + j);
                AX_VIN_GetDevFrame(nDevId, j, frameBufferArr + j, -1);
                frameSeqNotMatch = AX_TRUE;
            }
        }

        if (frameSeqNotMatch) {
            for (j = 0; j < eHdrMode; j++) {
                frameSeqs[j] = frameBufferArr[j].tFrameInfo.stVFrame.u64SeqNum;
                if (frameSeqs[j] > maxFrameSeq) {
                    maxFrameSeq = frameSeqs[j];
                }
            }
        }
    } while (frameSeqNotMatch);

    return AX_TRUE;
}

static AX_S32 SysFrameDispatch(AX_U8 nDevId, AX_CAMERA_T *pCam, AX_SNS_HDR_MODE_E eHdrMode)
{
    AX_S32 axRet = 0;
    AX_S32 j = 0;
    AX_S32 cnt = 0;
    AX_S32 timeOutMs = 1000;
    AX_U32  nPipeId = 0;
    AX_U64 maxFrameSeq = 0;
    AX_IMG_INFO_T frameBufferArr[AX_SNS_HDR_FRAME_MAX] = {0};
    AX_VIN_DEV_BIND_PIPE_T *ptDevBindPipe = &pCam->tDevBindPipe;
    AX_U64 frameSeqs[AX_SNS_HDR_FRAME_MAX] = {0};
    AX_BOOL isMatch = AX_FALSE;

    for (j = 0; j < eHdrMode; j++) {
        axRet = AX_VIN_GetDevFrame(nDevId, j, frameBufferArr + j, timeOutMs);
        if (axRet != 0) {
            if (AX_ERR_VIN_RES_EMPTY == axRet) {
                COMM_CAM_PRT("nonblock error, 0x%x\n", axRet);
                return axRet;
            }

            usleep(10 * 1000);
            AX_VIN_ReleaseDevFrame(nDevId, j, frameBufferArr + j);
            return axRet;
        }

        frameSeqs[j] = frameBufferArr[j].tFrameInfo.stVFrame.u64SeqNum;
        if (frameSeqs[j] > maxFrameSeq) {
            maxFrameSeq = frameSeqs[j];
        }
    }

    for (cnt = 0; cnt < ptDevBindPipe->nNum; cnt++) {
        nPipeId = ptDevBindPipe->nPipeId[cnt];
        if ((pCam->bEnableFlash == AX_TRUE) && (nPipeId != frameBufferArr[0].tIspInfo.tExpInfo.nPipeId)) {
            continue;
        }
        if (pCam->tPipeInfo[nPipeId].ePipeMode == SAMPLE_PIPE_MODE_FLASH_SNAP) {
            /* ITS flash lamp snap case */
            /* find the capture raw frame, this is just a sample. */
            if ((frameBufferArr[0].tFrameInfo.stVFrame.u64SeqNum % 100 == 0) || (pCam->bEnableFlash == AX_TRUE)) {
                COMMON_CAM_CaptureFrameProc(nPipeId, (const AX_IMG_INFO_T **)&frameBufferArr);
            }
        } else {
            isMatch = SeqNumIsMatch(nDevId, frameBufferArr, frameSeqs, maxFrameSeq, eHdrMode);
            if (isMatch == AX_TRUE) {
                axRet = AX_VIN_SendRawFrame(ptDevBindPipe->nPipeId[cnt], AX_VIN_FRAME_SOURCE_ID_IFE, eHdrMode,
                                            (const AX_IMG_INFO_T **)&frameBufferArr, 0);
                if (axRet != 0) {
                    COMM_CAM_PRT("Send Pipe raw frame failed\n");
                }
            }
        }
    }

    for (j = 0; j < eHdrMode; j++) {
        AX_VIN_ReleaseDevFrame(nDevId, j, frameBufferArr + j);
    }

    return 0;
}

static void *DispatchThread(void *args)
{
    AX_CHAR token[32] = {0};
    AX_CAMERA_T *pCam = (AX_CAMERA_T *)args;

    AX_U8 nDevId = pCam->nDevId;
    AX_SNS_HDR_MODE_E eHdrMode = pCam->eHdrMode;

    snprintf(token, 32, "RAW_DISP_%u", nDevId);
    prctl(PR_SET_NAME, token);

    while (!g_dispatcher_loop_exit) {
        SysFrameDispatch(nDevId, pCam, eHdrMode);
    }

    return NULL;
}

AX_S32 COMMON_CAM_Run(AX_CAMERA_T *pCamList, AX_U8 Num)
{
    AX_S32 axRet = 0;
    AX_S32 i = 0;
    g_dispatcher_loop_exit = 0;

    for (i = 0; i < Num; i++) {
        if (!pCamList[i].bDevOnly) {
            if (pCamList[i].bOpen) {
                COMMON_ISP_Run(pCamList[i].tDevBindPipe, &pCamList[i].tPipeInfo[0]);

                if (pCamList[i].bEnableDev) {
                    pthread_create(&gDispatchThread[i], NULL, DispatchThread, (AX_VOID *)(&pCamList[i]));
                }
            }
        }
    }

    return axRet;
}

AX_S32 COMMON_CAM_Stop(void)
{
    AX_S32 axRet = 0;
    AX_S32 i = 0;
    g_dispatcher_loop_exit = 1;

    for (i = 0; i < MAX_CAMERAS; i++) {
        if (gDispatchThread[i] != 0) {
            axRet = pthread_join(gDispatchThread[i], NULL);
            if (axRet < 0) {
                COMM_CAM_PRT(" dispacher thread exit failed, ret=0x%x.\n", axRet);
            }
            gDispatchThread[i] = 0;
        }
    }
    COMMON_ISP_Stop();

    return 0;
}

AX_S32 COMMON_CAM_CaptureFrameProc(AX_U32 nCapturePipeId, const AX_IMG_INFO_T *pImgInfo[])
{
    AX_S32 axRet = 0;
    AX_IMG_INFO_T capture_img_info = {0};
    AX_U32 nRefPipeId = 0;
    AX_ISP_IQ_AE_PARAM_T tUserCaptureFrameAeParam;
    AX_ISP_IQ_AWB_PARAM_T tUserCaptureFrameAwbParam;
    AX_ISP_IQ_AINR_PARAM_T  tUserCaptureFrameAinrParam;

    /* use your capture raw frame's ae in manual mode, this is just a sample*/
    AX_ISP_IQ_GetAeParam(nRefPipeId, &tUserCaptureFrameAeParam);
    tUserCaptureFrameAeParam.nEnable = AX_FALSE;
    AX_ISP_IQ_SetAeParam(nCapturePipeId, &tUserCaptureFrameAeParam);

    /* use your capture raw frame's awb in manual mode, this is just a sample*/
    AX_ISP_IQ_GetAwbParam(nRefPipeId, &tUserCaptureFrameAwbParam);
    tUserCaptureFrameAwbParam.nEnable = AX_FALSE;
    AX_ISP_IQ_SetAwbParam(nCapturePipeId, &tUserCaptureFrameAwbParam);

    /* use your capture raw frame's ainr in manual mode, this is just a sample, to meet the capture frame rate*/
    axRet = AX_ISP_IQ_GetAinrParam(nCapturePipeId, &tUserCaptureFrameAinrParam);
    tUserCaptureFrameAinrParam.nAutoMode = AX_FALSE;
    if (tUserCaptureFrameAinrParam.tDummyParam.nModelNum > 0) {
        strncpy(tUserCaptureFrameAinrParam.tManualParam.szModelName,
                tUserCaptureFrameAinrParam.tDummyParam.tModelTable[0].tMeta.szModelName,
                sizeof(tUserCaptureFrameAinrParam.tManualParam.szModelName));
        strncpy(tUserCaptureFrameAinrParam.tManualParam.szModelPath,
                tUserCaptureFrameAinrParam.tDummyParam.tModelTable[0].tMeta.szModelPath,
                sizeof(tUserCaptureFrameAinrParam.tManualParam.szModelPath));
    }
    axRet = AX_ISP_IQ_SetAinrParam(nCapturePipeId, &tUserCaptureFrameAinrParam);
    if (0 != axRet) {
        COMM_CAM_PRT("Set Pipe ainr param failed, axRet[%d]\n", axRet);
    }

    /* 1. first send raw frame*/
    AX_ISP_RunOnce(nCapturePipeId);

    axRet = AX_VIN_SendRawFrame(nCapturePipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_SNS_LINEAR_MODE,
                                pImgInfo, 0);
    if (axRet != 0) {
        COMM_CAM_PRT("Send Pipe raw frame failed");
        return axRet;
    }
    /* The first frame data is invalid for the user */
    axRet = AX_VIN_GetYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info, 3000);
    if (axRet != 0) {
        COMM_CAM_PRT("func:%s, return error!.\n", __func__);
        return axRet;
    }
    AX_VIN_ReleaseYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info);

    /* 2. second send raw frame*/
    /* use your capture raw frame's ainr in manual mode, this is just a sample, to meet the capture frame rate*/
    AX_ISP_IQ_GetAinrParam(nCapturePipeId, &tUserCaptureFrameAinrParam);
    tUserCaptureFrameAinrParam.nAutoMode = AX_FALSE;
    if (tUserCaptureFrameAinrParam.tAutoParam.nAutoModelNum > 0) {
        strncpy(tUserCaptureFrameAinrParam.tManualParam.szModelName,
                tUserCaptureFrameAinrParam.tAutoParam.tAutoModelTable[0].tMeta.szModelName,
                sizeof(tUserCaptureFrameAinrParam.tManualParam.szModelName));
        strncpy(tUserCaptureFrameAinrParam.tManualParam.szModelPath,
                tUserCaptureFrameAinrParam.tAutoParam.tAutoModelTable[0].tMeta.szModelPath,
                sizeof(tUserCaptureFrameAinrParam.tManualParam.szModelPath));
    }
    AX_ISP_IQ_SetAinrParam(nCapturePipeId, &tUserCaptureFrameAinrParam);

    AX_ISP_RunOnce(nCapturePipeId);

    axRet = AX_VIN_SendRawFrame(nCapturePipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_SNS_LINEAR_MODE,
                                pImgInfo, 0);
    if (axRet != 0) {
        COMM_CAM_PRT("Send Pipe raw frame failed");
        return axRet;
    }

    /* The second frame data is the final result frame */
    axRet = AX_VIN_GetYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info, 3000);
    if (axRet != 0) {
        COMM_CAM_PRT("func:%s, return error!.\n", __func__);
        return axRet;
    }

    /* Users can use second YUV frame for application development */

    /* User Code */
    /* ...... */

    AX_VIN_ReleaseYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info);

    COMM_CAM_PRT("Capture Frame Proc success.\n");
    return AX_SUCCESS;
}
