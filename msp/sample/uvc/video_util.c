/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <sys/prctl.h>
#include "video_util.h"

/* isp tuning */
#include "ax_vin_error_code.h"
#include "common_nt.h"

static AX_CAMERA_T gUVCCams[MAX_UVC_CAMERAS];
static const AX_VENC_RECV_PIC_PARAM_T gStLinkRecv[JENC_NUM_COMM];

static SAMPLE_JPEGENC_RCMODE enRcMode = JPEGENC_CBR;

extern volatile int uvc_exit;
pthread_t gDispatchFramePid[MAX_UVC_CAMERAS];

static stYUVColor color_arr[COLOR_COUNT] = {
    {255, 128, 128},    //white
    {0, 128, 128},      //black
    {76, 85, 255},      //red
    {75, 85, 74},       //green
    {29, 255, 107},     //blue
    {52, 170, 181},     //purple
};

/* comm pool */
static COMMON_SYS_POOL_CFG_T stSysCommPoolSingleDummySdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 10},    /*vin nv21/nv21 use */
};

static COMMON_SYS_POOL_CFG_T stSysCommPoolSingleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 10},    /*vin nv21/nv21 use */
};

static COMMON_SYS_POOL_CFG_T stSysCommPoolDoubleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 10 * 2},    /*vin nv21/nv21 use */
};

static COMMON_SYS_POOL_CFG_T stSysCommPoolDoubleOs08a20Hdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 50},        /* vin nv21/nv21 use */
};

/* private pool */
static COMMON_SYS_POOL_CFG_T stPrivPoolSingleDummySdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25},      /*vin raw16 use */
};

static COMMON_SYS_POOL_CFG_T stPrivPoolSingleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2},      /*vin raw16 use */
};

static COMMON_SYS_POOL_CFG_T stPrivPoolDoubleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2},      /*vin raw16 use */
};

static COMMON_SYS_POOL_CFG_T stPrivPoolDoubleOs08a20Hdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2},      /* vin raw16 use */
};

static AX_VOID uvc_set_pipe_hdr_mode(AX_U32 *pHdrSel, AX_SNS_HDR_MODE_E eHdrMode)
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

static AX_VOID uvc_set_vin_attr(AX_CAMERA_T *pCam, SAMPLE_SNS_TYPE_E eSnsType, AX_SNS_HDR_MODE_E eHdrMode,
                              COMMON_VIN_MODE_E eSysMode, AX_BOOL bAiispEnable)
{
    pCam->eSnsType = eSnsType;
    pCam->tSnsAttr.eSnsMode = eHdrMode;
    pCam->tDevAttr.eSnsMode = eHdrMode;
    pCam->eHdrMode = eHdrMode;
    pCam->eSysMode = eSysMode;
    pCam->tPipeAttr.eSnsMode = eHdrMode;
    pCam->tPipeAttr.bAiIspEnable = bAiispEnable;
    if (eHdrMode > AX_SNS_LINEAR_MODE) {
        pCam->tDevAttr.eSnsOutputMode = AX_SNS_DOL_HDR;
    } else {
        pCam->tSnsAttr.eRawType = AX_RT_RAW12;
        pCam->tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        pCam->tPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
    }

    if (COMMON_VIN_TPG == eSysMode) {
        pCam->tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_TPG;
    }

    if (COMMON_VIN_LOADRAW == eSysMode) {
        pCam->bEnableDev = AX_FALSE;
    } else {
        pCam->bEnableDev = AX_TRUE;
    }

    pCam->bRegisterSns = AX_TRUE;

    return;
}

static AX_U32 uvc_case_single_dummy(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        UVC_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0;
    AX_CAMERA_T *pCam = NULL;

    pCam = &pCamList[0];
    pCommonArgs->nCamCnt = 1;

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        for (AX_S32 j = 0; j < AX_VIN_MAX_PIPE_NUM; j++) {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
            strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }

    return 0;
}

static AX_U32 uvc_init_syscase_single_os08a20(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        UVC_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pCam = &pCamList[0];
    COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum =  1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    uvc_set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    uvc_set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
        if (pCam->tPipeInfo[j].bAiispEnable) {
            if (eHdrMode <= AX_SNS_LINEAR_MODE) {
                strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_sdr_ai3d-t2d_to_t3dnr_30fps.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            } else {
                strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_hdr_2x_ainr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
        } else {
            strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }
    return 0;
}

static AX_U32 uvc_init_syscase_double_os08a20(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        UVC_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0, j = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_CHAR apd_plate_id[BOARD_ID_LEN] = {0};

    pCommonArgs->nCamCnt = 2;
    COMMON_SYS_GetApdPlateId(apd_plate_id);

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        if (i == 0) {
            pCam->nDevId = 0;
            pCam->nRxDev = 0;
            pCam->nPipeId = 0;
            pCam->tSnsClkAttr.nSnsClkIdx = 0;
        } else if (i == 1) {
            if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE", sizeof("ADP_RX_DPHY_2X4LANE") - 1)) {
                if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE_N", sizeof("ADP_RX_DPHY_2X4LANE_N") - 1)) {
                    pCam->nDevId = 2;
                    pCam->nRxDev = 2;
                    pCam->nPipeId = 1;
                    pCam->tSnsClkAttr.nSnsClkIdx = 1;
                } else {
                    pCam->nDevId = 4;
                    pCam->nRxDev = 4;
                    pCam->nPipeId = 1;
                    pCam->tSnsClkAttr.nSnsClkIdx = 1;
                }
            }
        }

        pCam->tDevBindPipe.nNum =  1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
        uvc_set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        uvc_set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
            if (pCam->tPipeInfo[j].bAiispEnable) {
                if (eHdrMode <= AX_SNS_LINEAR_MODE) {
                    strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_sdr_ai3d_t2dnr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
                } else {
                    strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_hdr_2x_ainr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
                }
            } else {
                strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
        }
    }

    return 0;
}

AX_S32 set_jpeg_param(AX_U32 chn)
{
    AX_S32 s32Ret = -1;
    AX_VENC_JPEG_PARAM_T stJpegParam;
    memset(&stJpegParam, 0, sizeof(stJpegParam));
    s32Ret = AX_VENC_GetJpegParam(chn, &stJpegParam);
    if (AX_SUCCESS != s32Ret) {
        printf("AX_VENC_GetJpegParam:%d failed, error type 0x%x!\n", chn, s32Ret);
        return s32Ret;
    }

    stJpegParam.u32Qfactor = gS32QPLevel;
    /* Use user set qtable. Qtable example */
    if (gS32QTableEnable) {
        memcpy(stJpegParam.u8YQt, QTableLuminance, sizeof(QTableLuminance));
        memcpy(stJpegParam.u8CbCrQt, QTableChrominance, sizeof(QTableChrominance));
    }

    s32Ret = AX_VENC_SetJpegParam(chn, &stJpegParam);
    if (AX_SUCCESS != s32Ret) {
        printf("AX_VENC_SetJpegParam:%d failed, error type 0x%x!\n", chn, s32Ret);
        return s32Ret;
    }

    return AX_SUCCESS;
}

AX_S32 set_rc_param(AX_U32 chn, AX_PAYLOAD_TYPE_E type, AX_U32 bitrate)
{
#ifdef VIDEO_ENABLE_RC_DYNAMIC
    AX_S32 s32Ret = -1;
    AX_VENC_RC_PARAM_T stRcParam;

    if (type == PT_MJPEG) {
        s32Ret = AX_VENC_GetRcParam(chn, &stRcParam);
        if (AX_SUCCESS != s32Ret) {
            printf("AX_VENC_GetRcParam:%d failed, error type 0x%x!\n", chn, s32Ret);
            return s32Ret;
        }

        if (enRcMode == JPEGENC_CBR) {
            stRcParam.stMjpegCbr.u32BitRate = bitrate;
            stRcParam.stMjpegCbr.u32MinQp  = gU32QPMin;
            stRcParam.stMjpegCbr.u32MaxQp  = gU32QPMax;
        } else if (enRcMode == JPEGENC_VBR) {
            stRcParam.stMjpegVbr.u32MaxBitRate  = bitrate;
            stRcParam.stMjpegVbr.u32MinQp  = gU32QPMin;
            stRcParam.stMjpegVbr.u32MaxQp  = gU32QPMax;
        } else if (enRcMode == JPEGENC_FIXQP) {
            stRcParam.stMjpegFixQp.s32FixedQp = gU32FixedQP;
        }

        s32Ret = AX_VENC_SetRcParam(chn, &stRcParam);
        if (AX_SUCCESS != s32Ret) {
            printf("AX_VENC_SetRcParam:%d failed, error type 0x%x!\n", chn, s32Ret);
            return s32Ret;
        }
    }
#endif
    return AX_SUCCESS;
}

AX_S32 jenc_chn_attr_init(struct uvc_device *dev, AX_S32 input_width, AX_S32 input_height, AX_IMG_FORMAT_E format, \
                          AX_S32 output_width, AX_S32 output_height, AX_U32 enc_chn_id, AX_U32 bitrate)
{
    printf("%s +++\n", __func__);
    if (NULL == dev) {
        printf("jenc_chn_attr_init failed, dev is NULL!\n");
        return -1;
    }

    AX_VENC_CHN_ATTR_T stJencChnAttr;
    AX_VENC_MJPEG_CBR_T stMjpegCbrAttr;
    AX_VENC_MJPEG_VBR_T stMjpegVbrAttr;
    AX_VENC_MJPEG_FIXQP_T stMjpegFixQpAttr;
    AX_VENC_JPEG_PARAM_T stJpegParam;

    AX_S32 s32Ret = -1;

    AX_S32 s32InputWidth = input_width;
    AX_S32 s32InputHeight = input_height;

    memset(&stJencChnAttr, 0, sizeof(stJencChnAttr));
    memset(&stMjpegCbrAttr, 0, sizeof(stMjpegCbrAttr));
    memset(&stMjpegVbrAttr, 0, sizeof(stMjpegVbrAttr));
    memset(&stMjpegFixQpAttr, 0, sizeof(stMjpegFixQpAttr));
    memset(&stJpegParam, 0, sizeof(stJpegParam));

    stJencChnAttr.stVencAttr.u32MaxPicWidth = ALIGN_UP(s32InputWidth, UVC_ENCODER_FBC_WIDTH_ALIGN_VAL);
    stJencChnAttr.stVencAttr.u32MaxPicHeight = ALIGN_UP(s32InputHeight, UVC_ENCODER_FBC_WIDTH_ALIGN_VAL);

    if (0 == s32InputWidth) {
        s32InputWidth = u32DefaultSrcPixelWidth;
    }

    if (0 == s32InputHeight) {
        s32InputHeight = u32DefaultSrcPixelHeight;
    }

    stJencChnAttr.stVencAttr.enMemSource = AX_MEMORY_SOURCE_POOL;
    stJencChnAttr.stVencAttr.u32PicWidthSrc = s32InputWidth;
    stJencChnAttr.stVencAttr.u32PicHeightSrc = s32InputHeight;
    stJencChnAttr.stVencAttr.u32BufSize = s32InputWidth * s32InputHeight * 3 / 2;

    if (0 == output_width) {
        printf("WARNNING, output_width == 0 !\n");
        return s32Ret;
    }

    if (input_width < output_width) {
        printf("WARNNING, unsupport input_width < output_width!\n");
        return s32Ret;
    }

    if (0 == output_height) {
        printf("WARNNING, output_height == 0!\n");
        return s32Ret;
    }

    if (input_height < output_height) {
        printf("WARNNING,   unsupport input_height < output_height!\n");
        return s32Ret;
    }

    stJencChnAttr.stVencAttr.u32PicWidthSrc = input_width;
    stJencChnAttr.stVencAttr.u32PicHeightSrc = input_height;

    stJencChnAttr.stVencAttr.u8InFifoDepth = 1;
    stJencChnAttr.stVencAttr.u8OutFifoDepth = 1;
    if (dev->is_link) {
        stJencChnAttr.stVencAttr.enLinkMode = AX_VENC_LINK_MODE;
    } else {
        stJencChnAttr.stVencAttr.enLinkMode = AX_VENC_UNLINK_MODE;
    }

    stJencChnAttr.stVencAttr.enType = PT_MJPEG;
    stJencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = f32SrcVideoFrameRate;
    stJencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = f32DstVideoFrameRate;

    switch (stJencChnAttr.stVencAttr.enType) {
    case PT_MJPEG: {
        if (JPEGENC_CBR  == enRcMode) {
            stJencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;
            stMjpegCbrAttr.u32StatTime = 1;
            stMjpegCbrAttr.u32BitRate = bitrate;
            stMjpegCbrAttr.u32MinQp = gU32QPMin;
            stMjpegCbrAttr.u32MaxQp = gU32QPMax;
            memcpy(&stJencChnAttr.stRcAttr.stMjpegCbr, &stMjpegCbrAttr, sizeof(AX_VENC_MJPEG_CBR_T));
        } else if (JPEGENC_VBR == enRcMode) {
            stJencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;
            stMjpegVbrAttr.u32StatTime = 1;
            stMjpegVbrAttr.u32MaxBitRate = bitrate;
            stMjpegVbrAttr.u32MinQp = gU32QPMin;
            stMjpegVbrAttr.u32MaxQp = gU32QPMax;
            memcpy(&stJencChnAttr.stRcAttr.stMjpegVbr, &stMjpegVbrAttr, sizeof(AX_VENC_MJPEG_VBR_T));
        } else if (JPEGENC_FIXQP == enRcMode) {
            stJencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
            stMjpegFixQpAttr.s32FixedQp = gU32FixedQP;
            memcpy(&stJencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegFixQpAttr, sizeof(AX_VENC_MJPEG_FIXQP_T));
        }
    }
    break;
    default:
        printf("invalid codec type, 0x%x!\n", stJencChnAttr.stVencAttr.enType);
        return s32Ret;
    }

    if (1 == gS32EnableCrop) {
        stJencChnAttr.stVencAttr.stCropCfg.bEnable = 1;
        stJencChnAttr.stVencAttr.stCropCfg.stRect.s32X = 0;
        stJencChnAttr.stVencAttr.stCropCfg.stRect.s32Y = 0;
        stJencChnAttr.stVencAttr.stCropCfg.stRect.u32Height = output_height;
        stJencChnAttr.stVencAttr.stCropCfg.stRect.u32Width = output_width;
    }

    s32Ret = AX_VENC_CreateChn(dev->venc_stream_param.VeChn, &stJencChnAttr);
    if (AX_SUCCESS != s32Ret) {
        printf("AX_VENC_CreateChn %d   failed! error type = 0x%x\n", dev->venc_stream_param.VeChn, s32Ret);
        return s32Ret;
    }

    set_rc_param(dev->venc_stream_param.VeChn, PT_MJPEG, bitrate);

    set_jpeg_param(dev->venc_stream_param.VeChn);

    s32Ret = AX_VENC_StartRecvFrame(dev->venc_stream_param.VeChn, &(gStLinkRecv[dev->venc_stream_param.VeChn]));
    if (AX_SUCCESS != s32Ret) {
        printf("start recv frame failed, error type = 0x%x!\n", s32Ret);
        return s32Ret;
    }

    dev->venc_stream_param.bThreadStart = AX_TRUE;

    pthread_create(&dev->get_stream_pid, NULL, venc_get_stream, (void *)dev);
    printf("%s ---\n", __func__);
    return AX_SUCCESS;
}

AX_S32 video_init(struct uvc_device **dev, UVC_SYS_CASE_E sys_case, COMMON_SYS_ARGS_T sys_arg,
                  COMMON_VIN_MODE_E vin_mode, \
                  AX_SNS_HDR_MODE_E hdr_mode, SAMPLE_SNS_TYPE_E sns_type, AX_S32 isp_tuning, AX_S32 aiisp_enable)
{
    printf("%s +++\n", __func__);
    if (NULL == dev) {
        printf("video_init failed, dev is NULL\n");
        return -1;
    }

    UVC_SYS_CASE_E eSysCase = sys_case;
    COMMON_SYS_ARGS_T stSysArg = {0};
    COMMON_SYS_ARGS_T stPrivPoolArg = {0};
    AX_SNS_HDR_MODE_E eHdrMode = hdr_mode;
    COMMON_VIN_MODE_E eVinMode = vin_mode;
    SAMPLE_SNS_TYPE_E eSnsType = sns_type;

    AX_S32 s32Ret = -1;

    if (UVC_SYS_CASE_BUTT <= eSysCase ||  UVC_SYS_CASE_NONE >= eSysCase) {
        printf("error , no such sys case type!\n");
        return s32Ret;
    }

    if (COMMON_VIN_BUTT <= eVinMode ||  COMMON_VIN_NONE >= eVinMode) {
        printf("error , no such sys mode type!\n");
        return s32Ret;
    }

    s32Ret = init_sys_config(eSysCase, &stSysArg, &stPrivPoolArg, eVinMode, eHdrMode, eSnsType, aiisp_enable);
    if (0 != s32Ret) {
        printf("init_sys_config failed , error type = 0x%x\n", s32Ret);
        return s32Ret;
    }

#ifdef ENABLE_UVC_DEBUG
    printf("nCamCnt=%d, nPoolCfgCnt=%d, pPoolCfg->nBlkCnt=%d,pPoolCfg->nFmt=%d, stSysArg.pPoolCfg->nHeight=%d, pPoolCfg->nWidth=%d,pPoolCfg->nWidthStride=%d",
           \
           stSysArg.nCamCnt, stSysArg.nPoolCfgCnt, stSysArg.pPoolCfg->nBlkCnt, \
           stSysArg.pPoolCfg->nFmt, stSysArg.pPoolCfg->nHeight, stSysArg.pPoolCfg->nWidth, stSysArg.pPoolCfg->nWidthStride);
#endif

    s32Ret = COMMON_SYS_Init(&stSysArg);
    if (AX_SUCCESS != s32Ret) {
        printf("COMMON_SYS_Init failed, error type = 0x%x\n", s32Ret);
        goto EXIT;
    }

    if (aiisp_enable) {
        s32Ret = COMMON_NPU_Init(AX_ENGINE_VIRTUAL_NPU_BIG_LITTLE);
        if (s32Ret) {
            printf("COMMON_NPU_Init fail, ret:0x%x", s32Ret);
            goto EXIT;
        }
    }

    s32Ret = COMMON_CAM_Init();
    if (AX_SUCCESS != s32Ret) {
        printf("COMMON_CAM_Init failed, error type = 0x%x\n", s32Ret);
        goto EXIT;
    }
    s32Ret = COMMON_CAM_PrivPoolInit(&stPrivPoolArg);
    if (s32Ret) {
        printf("COMMON_CAM_PrivPoolInit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }
    s32Ret = COMMON_CAM_Open(&gUVCCams[0], stSysArg.nCamCnt);
    if (s32Ret) {
        goto EXIT;
    }

    for (int i = 0; i < stSysArg.nCamCnt; i++) {
        dev[i]->stUvcCfgYUYV.pStUVCCamera = &gUVCCams[i];
        dev[i]->pipe = gUVCCams[i].nPipeId;
    }

    if (isp_tuning) {
        s32Ret = COMMON_NT_Init(6000, 8082);
        if (s32Ret) {
            printf("COMMON_NT_Init fail, ret:0x%x", s32Ret);
            goto EXIT;
        }
        /* update pipe attribute */
        for (int i = 0; i < stSysArg.nCamCnt; i++) {
            for (int j = 0; j < gUVCCams[i].tDevBindPipe.nNum; j++) {
                COMMON_NT_UpdateSource(gUVCCams[i].tDevBindPipe.nPipeId[j]);
            }
        }
    }

    COMMON_CAM_Run(&gUVCCams[0], stSysArg.nCamCnt);

    printf("%s ---\n", __func__);
    return AX_SUCCESS;

EXIT:
    video_deinit(isp_tuning);
    return s32Ret;
}

AX_VOID video_deinit(AX_S32 isp_tuning)
{
    COMMON_CAM_Stop();
    COMMON_CAM_Close(&gUVCCams[0], MAX_UVC_CAMERAS);

    if (isp_tuning) {
        COMMON_NT_DeInit();
    }

    COMMON_CAM_Deinit();
    COMMON_SYS_DeInit();
}

AX_S32 init_sys_config(UVC_SYS_CASE_E sys_case, COMMON_SYS_ARGS_T *sys_arg, COMMON_SYS_ARGS_T *priv_pool_arg,
                       COMMON_VIN_MODE_E vin_mode,
                       AX_SNS_HDR_MODE_E hdr_mode, SAMPLE_SNS_TYPE_E sns_type, AX_S32 aiisp_enable)
{
    printf("%s +++\n", __func__);

    if (NULL == sys_arg || NULL == priv_pool_arg) {
        printf("sys_arg is NULL\n");
        return -1;
    }

	AX_CAMERA_T *pCamList = &gUVCCams[0];
	UVC_VIN_PARAM_T stVinParam = {0};
	stVinParam.bAiispEnable = aiisp_enable;
	stVinParam.eHdrMode = hdr_mode;
	stVinParam.eSysCase = sys_case;
	stVinParam.eSysMode = vin_mode;
	stVinParam.nDumpFrameNum = 0;

    printf("sys_case %d, vin_mode %d, hdr_mode %d, sns_type %d, aiisp_enable %d\n", sys_case, \
           vin_mode, hdr_mode, sns_type, aiisp_enable);

	if (UVC_SYS_CASE_SINGLE_DUMMY == stVinParam.eSysCase) {
		sys_arg->nPoolCfgCnt = sizeof(stSysCommPoolSingleDummySdr) / sizeof(stSysCommPoolSingleDummySdr[0]);
		sys_arg->pPoolCfg = stSysCommPoolSingleDummySdr;
		priv_pool_arg->nPoolCfgCnt = sizeof(stPrivPoolSingleDummySdr) / sizeof(stPrivPoolSingleDummySdr[0]);
		priv_pool_arg->pPoolCfg = stPrivPoolSingleDummySdr;

		uvc_case_single_dummy(pCamList, sns_type, &stVinParam, sys_arg);
    } else if (UVC_SYS_CASE_SINGLE_OS08A20 == stVinParam.eSysCase) {
		sys_arg->nPoolCfgCnt = sizeof(stSysCommPoolSingleOs08a20Sdr) / sizeof(stSysCommPoolSingleOs08a20Sdr[0]);
		sys_arg->pPoolCfg = stSysCommPoolSingleOs08a20Sdr;
		priv_pool_arg->nPoolCfgCnt = sizeof(stPrivPoolSingleOs08a20Sdr) / sizeof(stPrivPoolSingleOs08a20Sdr[0]);
		priv_pool_arg->pPoolCfg = stPrivPoolSingleOs08a20Sdr;

		uvc_init_syscase_single_os08a20(pCamList, sns_type, &stVinParam, sys_arg);
    } else if (UVC_SYS_CASE_DUAL_OS08A20 == stVinParam.eSysCase) {
		if (AX_SNS_LINEAR_MODE == stVinParam.eHdrMode) {
			sys_arg->nPoolCfgCnt = sizeof(stSysCommPoolDoubleOs08a20Sdr) / sizeof(stSysCommPoolDoubleOs08a20Sdr[0]);
			sys_arg->pPoolCfg = stSysCommPoolDoubleOs08a20Sdr;
		} else {
			sys_arg->nPoolCfgCnt = sizeof(stSysCommPoolDoubleOs08a20Hdr) / sizeof(stSysCommPoolDoubleOs08a20Hdr[0]);
			sys_arg->pPoolCfg = stSysCommPoolDoubleOs08a20Hdr;
		}

		if (AX_SNS_LINEAR_MODE == stVinParam.eHdrMode){
			priv_pool_arg->nPoolCfgCnt = sizeof(stPrivPoolDoubleOs08a20Sdr) / sizeof(stPrivPoolDoubleOs08a20Sdr[0]);
			priv_pool_arg->pPoolCfg = stPrivPoolDoubleOs08a20Sdr;
		} else {
			priv_pool_arg->nPoolCfgCnt = sizeof(stPrivPoolDoubleOs08a20Hdr) / sizeof(stPrivPoolDoubleOs08a20Hdr[0]);
			priv_pool_arg->pPoolCfg = stPrivPoolDoubleOs08a20Hdr;
		}

		uvc_init_syscase_double_os08a20(pCamList, sns_type, &stVinParam, sys_arg);
	} else {
		printf("uvc sys case is not supported!\n");
		return -1;
    }

    printf("%s ---\n", __func__);
    return 0;
}

AX_S32 venc_init(AX_S32 enc_format)
{
    printf("%s +++\n", __func__);

    AX_S32 s32Ret = -1;
    AX_VENC_MOD_ATTR_T stVencModAttr = {0};

    if (V4L2_PIX_FMT_MJPEG == enc_format) {
        stVencModAttr.enVencType = AX_VENC_JPEG_ENCODER;
    } else if (V4L2_PIX_FMT_H264 == enc_format) {
        stVencModAttr.enVencType = AX_VENC_VIDEO_ENCODER;
    } else if ((V4L2_PIX_FMT_MJPEG + V4L2_PIX_FMT_H264) <= enc_format) {
        stVencModAttr.enVencType = AX_VENC_MULTI_ENCODER;
    }
    stVencModAttr.stModThdAttr.u32TotalThreadNum = 1;
    stVencModAttr.stModThdAttr.bExplicitSched = AX_FALSE;

    s32Ret = AX_VENC_Init(&stVencModAttr);
    if (AX_SUCCESS != s32Ret) {
        printf("AX_VENC_Init failed, s32Ret=0x%x\n", s32Ret);
        return s32Ret;
    }

    printf("%s ---\n", __func__);
    return AX_SUCCESS;
}

AX_S32 venc_deinit(AX_VOID)
{
    printf("%s +++\n", __func__);

    AX_S32 s32Ret = -1;
    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret) {
        printf("AX_VENC_Deinit failed, s32Ret=0x%x\n", s32Ret);
    }

    printf("%s ---\n", __func__);
    return s32Ret;
}

AX_S32 venc_chn_attr_init(struct uvc_device *dev, AX_S32 input_width, AX_S32 input_height, AX_IMG_FORMAT_E fomat, \
                          AX_S32 output_width, AX_S32 output_height, AX_U32 venc_chn_id, AX_PAYLOAD_TYPE_E payload_type, \
                          AX_U32 bitrate)
{
    printf("%s +++\n", __func__);

    if (NULL == dev) {
        printf("venc_chn_attr_init failed, dev is NULL\n");
        return -1;
    }

    AX_S32 s32Ret = -1;
    AX_VENC_CHN_ATTR_T stVencChnAttr = {0};
    VIDEO_CONFIG_T stVencCfg = {0};

    stVencCfg.stRCInfo.eRCType = VENC_RC_CBR;
    stVencCfg.nGOP = 30;
    stVencCfg.nBitrate = bitrate;
    stVencCfg.stRCInfo.nMinQp = 10;
    stVencCfg.stRCInfo.nMaxQp = 51;
    stVencCfg.stRCInfo.nMinIQp = 10;
    stVencCfg.stRCInfo.nMaxIQp = 51;
    stVencCfg.stRCInfo.nMinIProp = 10;
    stVencCfg.stRCInfo.nMaxIProp = 40;
    stVencCfg.stRCInfo.nIntraQpDelta = -2;

    /*TODO: fix resolution logic*/
    if (0 >= input_width  || input_width > u32DefaultSrcPixelWidth) {
        stVencCfg.nInWidth = u32DefaultSrcPixelWidth;
    } else {
        stVencCfg.nInWidth = input_width;
    }

    if (0 >= input_height || input_height > u32DefaultSrcPixelHeight) {
        stVencCfg.nInHeight = u32DefaultSrcPixelHeight;
    } else {
        stVencCfg.nInHeight = input_height;
    }

    if (0 == output_width) {
        printf("WARNNING, output_width == 0 !\n");
        return s32Ret;
    }

    if (input_width < output_width) {
        printf("WARNNING, unsupport input_width < output_width!\n");
        return s32Ret;
    }

    if (0 == output_height) {
        printf("WARNNING, output_height == 0!\n");
        return s32Ret;
    }

    if (input_height < output_height) {
        printf("WARNNING,   unsupport input_height < output_height!\n");
        return s32Ret;
    }

    stVencCfg.nStride = stVencCfg.nInWidth;
    stVencCfg.ePayloadType = payload_type;
    stVencCfg.nSrcFrameRate = f32SrcVideoFrameRate;
    stVencCfg.nDstFrameRate = f32DstVideoFrameRate;

    stVencCfg.nOffsetCropW = output_width;
    stVencCfg.nOffsetCropH = output_height;
    stVencCfg.nOffsetCropX = (input_width - output_width) / 2;
    stVencCfg.nOffsetCropY = (input_height - output_height) / 2;

    memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    stVencChnAttr.stVencAttr.u32MaxPicWidth = ALIGN_UP(input_width, UVC_ENCODER_FBC_WIDTH_ALIGN_VAL);
    stVencChnAttr.stVencAttr.u32MaxPicHeight = ALIGN_UP(input_height, UVC_ENCODER_FBC_WIDTH_ALIGN_VAL);

    stVencChnAttr.stVencAttr.u32PicWidthSrc = stVencCfg.nInWidth;
    stVencChnAttr.stVencAttr.u32PicHeightSrc = stVencCfg.nInHeight;

    stVencChnAttr.stVencAttr.stCropCfg.bEnable = 1;
    stVencChnAttr.stVencAttr.stCropCfg.stRect.s32X = stVencCfg.nOffsetCropX;
    stVencChnAttr.stVencAttr.stCropCfg.stRect.s32Y = stVencCfg.nOffsetCropY;
    stVencChnAttr.stVencAttr.stCropCfg.stRect.u32Width = stVencCfg.nOffsetCropW;
    stVencChnAttr.stVencAttr.stCropCfg.stRect.u32Height = stVencCfg.nOffsetCropH;

    stVencChnAttr.stVencAttr.enMemSource = AX_MEMORY_SOURCE_POOL;

    /*stream buffer size*/
    stVencChnAttr.stVencAttr.u32BufSize = stVencCfg.nStride * stVencCfg.nInHeight * 3 / 2;

    //stVencChnAttr.stVencAttr.u32MbLinesPerSlice = 0;
    stVencChnAttr.stVencAttr.u8InFifoDepth = 1;
    stVencChnAttr.stVencAttr.u8OutFifoDepth = 1;

    /*link mode*/
    if (dev->is_link) {
        stVencChnAttr.stVencAttr.enLinkMode = AX_VENC_LINK_MODE;
    } else {
        stVencChnAttr.stVencAttr.enLinkMode = AX_VENC_UNLINK_MODE;
    }

    //stVencChnAttr.stVencAttr.u32GdrDuration = 0;
    stVencChnAttr.stVencAttr.enType = stVencCfg.ePayloadType;

    /*GOP Setting*/
    stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;

    stVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = stVencCfg.nSrcFrameRate;
    stVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = stVencCfg.nDstFrameRate;

    switch (stVencChnAttr.stVencAttr.enType) {
    case PT_H265: {
        stVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_PROFILE;
        stVencChnAttr.stVencAttr.enLevel = AX_VENC_HEVC_LEVEL_5_1;
        stVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

        if (VENC_RC_CBR == stVencCfg.stRCInfo.eRCType) {
            AX_VENC_H265_CBR_T stH265Cbr = {0};

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH265Cbr.u32Gop = stVencCfg.nGOP;

            stH265Cbr.u32BitRate  = stVencCfg.nBitrate;

            stH265Cbr.u32MinQp = stVencCfg.stRCInfo.nMinQp;
            stH265Cbr.u32MaxQp = stVencCfg.stRCInfo.nMaxQp;
            stH265Cbr.u32MinIQp = stVencCfg.stRCInfo.nMinIQp;
            stH265Cbr.u32MaxIQp = stVencCfg.stRCInfo.nMaxIQp;
            stH265Cbr.u32MinIprop = stVencCfg.stRCInfo.nMinIProp;
            stH265Cbr.u32MaxIprop = stVencCfg.stRCInfo.nMaxIProp;

            stH265Cbr.s32IntraQpDelta = stVencCfg.stRCInfo.nIntraQpDelta;

            memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
        } else if (VENC_RC_VBR ==  stVencCfg.stRCInfo.eRCType) {
            AX_VENC_H265_VBR_T stH265Vbr = {0};

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265VBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH265Vbr.u32Gop = stVencCfg.nGOP;

            stH265Vbr.u32MaxBitRate = stVencCfg.nBitrate;

            stH265Vbr.u32MinQp = stVencCfg.stRCInfo.nMinQp;
            stH265Vbr.u32MaxQp = stVencCfg.stRCInfo.nMaxQp;
            stH265Vbr.u32MinIQp = stVencCfg.stRCInfo.nMinIQp;
            stH265Vbr.u32MaxIQp = stVencCfg.stRCInfo.nMaxIQp;
            stH265Vbr.u32MinIQp = stVencCfg.stRCInfo.nMinIProp;
            stH265Vbr.u32MaxIQp = stVencCfg.stRCInfo.nMaxIProp;

            stH265Vbr.s32IntraQpDelta = stVencCfg.stRCInfo.nIntraQpDelta;

            memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
        } else if (VENC_RC_FIXQP == stVencCfg.stRCInfo.eRCType) {
            AX_VENC_H265_FIXQP_T stH265FixQp = {0};

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265FIXQP;

            stH265FixQp.u32Gop = stVencCfg.nGOP;

            stH265FixQp.u32IQp = 25;
            stH265FixQp.u32PQp = 30;
            stH265FixQp.u32BQp = 32;

            memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
        } else {
            printf("eRCtype is not supported!\n");
            return s32Ret;
        }
        break;
    }
    case PT_H264: {
        stVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_MAIN_PROFILE;
        stVencChnAttr.stVencAttr.enLevel = AX_VENC_H264_LEVEL_5_2;
        if (VENC_RC_CBR == stVencCfg.stRCInfo.eRCType) {
            AX_VENC_H264_CBR_T stH264Cbr = {0};
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH264Cbr.u32Gop = stVencCfg.nGOP;

            stH264Cbr.u32BitRate = stVencCfg.nBitrate;

            stH264Cbr.u32MinQp = stVencCfg.stRCInfo.nMinQp;
            stH264Cbr.u32MaxQp = stVencCfg.stRCInfo.nMaxQp;
            stH264Cbr.u32MinIQp = stVencCfg.stRCInfo.nMinIQp;
            stH264Cbr.u32MaxIQp = stVencCfg.stRCInfo.nMaxIQp;
            stH264Cbr.u32MinIprop = stVencCfg.stRCInfo.nMinIProp;
            stH264Cbr.u32MaxIprop = stVencCfg.stRCInfo.nMaxIProp;

            stH264Cbr.s32IntraQpDelta = stVencCfg.stRCInfo.nIntraQpDelta;

            memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
        } else if (VENC_RC_VBR == stVencCfg.stRCInfo.eRCType) {
            AX_VENC_H264_VBR_T stH264Vbr = {0};

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264VBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH264Vbr.u32Gop = stVencCfg.nGOP;

            stH264Vbr.u32MaxBitRate = stVencCfg.nBitrate;

            stH264Vbr.u32MinQp = stVencCfg.stRCInfo.nMinQp;
            stH264Vbr.u32MaxQp = stVencCfg.stRCInfo.nMaxQp;
            stH264Vbr.u32MinIQp = stVencCfg.stRCInfo.nMinIQp;
            stH264Vbr.u32MaxIQp = stVencCfg.stRCInfo.nMaxIQp;

            stH264Vbr.s32IntraQpDelta = stVencCfg.stRCInfo.nIntraQpDelta;

            memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));
        } else if (VENC_RC_FIXQP == stVencCfg.stRCInfo.eRCType) {
            AX_VENC_H264_FIXQP_T  stH264FixQp = {0};

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264FIXQP;

            stH264FixQp.u32Gop = stVencCfg.nGOP;

            stH264FixQp.u32IQp = 25;
            stH264FixQp.u32PQp = 30;
            stH264FixQp.u32BQp = 32;

            memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
        } else {
            printf("eRCtype is not supported!\n");
            return s32Ret;
        }
        break;
    }
    default:
        printf("payload type is not supported!\n");
        return s32Ret;
    }

    s32Ret = AX_VENC_CreateChn(dev->venc_stream_param.VeChn, &stVencChnAttr);
    if (AX_SUCCESS != s32Ret) {
        printf("VencChn %d AX_VENC_CreateChn failed, s32Ret=0x%x", dev->venc_stream_param.VeChn, s32Ret);
        return s32Ret;
    }

    s32Ret = AX_VENC_StartRecvFrame(dev->venc_stream_param.VeChn, &(gStLinkRecv[dev->venc_stream_param.VeChn]));
    if (AX_SUCCESS != s32Ret) {
        printf("start recv frame failed, error type = 0x%x!\n", s32Ret);
        return s32Ret;
    }

    dev->venc_stream_param.bThreadStart = AX_TRUE;

    pthread_create(&dev->get_stream_pid, NULL, venc_get_stream, (void *)dev);

    printf("%s ---\n", __func__);
    return s32Ret;
}

AX_S32 venc_chn_deinit(struct uvc_device *dev, AX_S32 enc_format)
{
    printf("%s +++\n", __func__);

    if (NULL == dev) {
        printf("venc_chn_deinit failed, dev is NULL!\n");
        return -1;
    }

    AX_S32 s32Ret = -1;

    if ((V4L2_PIX_FMT_H264 == enc_format) || (V4L2_PIX_FMT_MJPEG == enc_format) || (V4L2_PIX_FMT_H265 == enc_format)) {
        dev->venc_stream_param.bThreadStart = AX_FALSE;
    } else {
        printf("enc_format is not supported!\n");
        return -1;
    }

    s32Ret = AX_VENC_StopRecvFrame(dev->venc_stream_param.VeChn);
    if (s32Ret < 0) {
        printf("AX_VENC_StopRecvFrame failed, error type = 0x%x!\n", s32Ret);
    }

    usleep(50 * 1000);

    s32Ret = AX_VENC_DestroyChn(dev->venc_stream_param.VeChn);
    if (s32Ret < 0) {
        printf("AX_VENC_DestroyChn failed, error type = 0x%x!\n", s32Ret);
    }

    printf("%s ---\n", __func__);
    return s32Ret;
}

#ifdef TEST_LATENCY
    AX_U64 g_u64GetVencLatency = 0;
    AX_U64 g_u64GetVencCount = 0;
    AX_U64 g_u64GetVinLatency = 0;
    AX_U64 g_u64GetVinCount = 0;
#endif

AX_VOID *venc_get_stream(AX_VOID *arg)
{
    printf("%s +++\n", __func__);

    prctl(PR_SET_NAME, "venc_get_stream");

    AX_S32 s32Ret = -1;
    AX_S32 timeOutMs = 200;
    struct uvc_device *dev;
    dev = (struct uvc_device *)arg;
    AX_IMG_INFO_T stImgYUV420 = {0};
    AX_VENC_STREAM_T stStream;
    memset(&stStream, 0, sizeof(AX_VENC_STREAM_T));

    if (PT_H264 == dev->venc_stream_param.enPayloadType || PT_H265 == dev->venc_stream_param.enPayloadType) {
        printf("AX_VENC_RequestIDR ...\n");
        AX_VENC_RequestIDR(dev->venc_stream_param.VeChn, AX_TRUE);
    }

#ifdef TEST_LATENCY
    g_u64GetVencLatency = 0;
    g_u64GetVencCount = 0;
    g_u64GetVinLatency = 0;
    g_u64GetVinCount = 0;
#endif
    while (!uvc_exit && (AX_TRUE == dev->venc_stream_param.bThreadStart)) {
        if (!dev->is_link) {
            s32Ret = AX_VIN_GetYuvFrame(dev->pipe, dev->vin_chn, &stImgYUV420, timeOutMs);
            if (0 != s32Ret) {
                printf("AX_VIN_GetYuvFrame failed, s32Ret = 0x%x\n", s32Ret);
                continue;
            }
#ifdef TEST_LATENCY
            AX_U64 u64CurPtsVin = 0;
            AX_SYS_GetCurPTS(&u64CurPtsVin);
            if (g_u64GetVinCount < 1000) {
                g_u64GetVinLatency += u64CurPtsVin - stImgYUV420.tFrameInfo.stVFrame.u64PTS;
                g_u64GetVinCount++;
                if (g_u64GetVinCount == 1000) {
                    printf("===============get vin frame: avg latency: %llu\n", g_u64GetVinLatency / 1000);
                    g_u64GetVinLatency = 0;
                    g_u64GetVinCount = 0;
                }
            }
#endif
            s32Ret = AX_VENC_SendFrame(dev->venc_stream_param.vencMod.s32ChnId, &stImgYUV420.tFrameInfo, 0);
            if (0 != s32Ret) {
                printf("AX_VENC_SendFrame failed, s32ret = 0x%x\n", s32Ret);
                goto _release;
            }
        }

        s32Ret = AX_VENC_GetStream(dev->venc_stream_param.VeChn, &stStream, gSyncType);
        switch (s32Ret) {
        case AX_SUCCESS: {
#ifdef TEST_LATENCY
            AX_U64 u64CurPts = 0;
            AX_SYS_GetCurPTS(&u64CurPts);
            if (g_u64GetVencCount < 1000) {
                g_u64GetVencLatency += u64CurPts - stStream.stPack.u64PTS;
                g_u64GetVencCount++;
                if (g_u64GetVencCount == 1000) {
                    printf("===============get venc stream: avg latency: %llu\n", g_u64GetVencLatency / 1000);
                    g_u64GetVencLatency = 0;
                    g_u64GetVencCount = 0;
                }
            }
#endif
            pthread_mutex_lock(&dev->img_mutex);
            uvc_img_cache_put(dev, stStream.stPack.pu8Addr, stStream.stPack.u32Len, \
                              stStream.stPack.u64SeqNum, stStream.stPack.u64PTS,
                              (AX_VENC_INTRA_FRAME == stStream.stPack.enCodingType) ? AX_TRUE : AX_FALSE);
            pthread_mutex_unlock(&dev->img_mutex);

            s32Ret = AX_VENC_ReleaseStream(dev->venc_stream_param.VeChn, &stStream);
            if (AX_SUCCESS != s32Ret) {
                printf("VENC[%d]: AX_VENC_ReleaseStream failed, error type 0x%x!\n", dev->venc_stream_param.VeChn, s32Ret);
            }
        }
        break;

        case AX_ERR_VENC_FLOW_END:
            printf("venc flow end!\n");
            dev->venc_stream_param.bThreadStart = AX_FALSE;
            break;

        case AX_ERR_VENC_BUF_EMPTY:
            printf("venc buf empty!\n");
            break;

        default:
            printf("VENC[%d]: error type 0x%x\n", dev->venc_stream_param.VeChn, s32Ret);
        }
_release:
        if (!dev->is_link) {
            s32Ret = AX_VIN_ReleaseYuvFrame(dev->stUvcCfgYUYV.pStUVCCamera->nPipeId, dev->stUvcCfgYUYV.s32Chn, &stImgYUV420);
            if (AX_SUCCESS  != s32Ret) {
                printf("AX_VIN_ReleaseYuvFrame failed, s32Ret=0x%x\n", s32Ret);
            }
        }
    }

    printf("%s ---\n", __func__);

    return (void *)(intptr_t)s32Ret;
}

AX_S32 link_vin_venc_mod(struct uvc_device *dev)
{
    if (NULL == dev) {
        printf("link_vin_venc_mod failed, dev is NULL\n");
        return -1;
    }

    AX_S32 s32Ret = -1;

    s32Ret = AX_SYS_Link(&(dev->venc_stream_param.vinMod), &(dev->venc_stream_param.vencMod));
    if (AX_SUCCESS != s32Ret) {
        printf("AX_SYS_Link failed! error type 0x%x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

AX_S32 unlink_vin_venc_mod(struct uvc_device *dev)
{
    if (NULL == dev) {
        printf("unlink_vin_venc_mod failed, dev is NULL\n");
        return -1;
    }

    AX_S32 s32Ret = -1;

    s32Ret = AX_SYS_UnLink(&(dev->venc_stream_param.vinMod), &(dev->venc_stream_param.vencMod));
    if (AX_SUCCESS != s32Ret) {
        printf("AX_SYS_Unlink failed! error type 0x%x\n", s32Ret);
        return s32Ret;
    }

    return AX_SUCCESS;
}

AX_S32 uncompressed_chn_init(struct uvc_device *dev, AX_S32 input_width, AX_S32 input_height,
                             AX_IMG_FORMAT_E src_format, AX_IMG_FORMAT_E dst_format, AX_S32 output_width, \
                             AX_S32 output_height, AX_U32 chn_id)
{
    printf("%s +++\n", __func__);

    if (NULL == dev) {
        printf("uncompressed_chn_init failed, dev is NULL\n");
        return -1;
    }

    dev->stUvcCfgYUYV.u32Width = input_width;
    dev->stUvcCfgYUYV.u32Height = input_height;
    dev->stUvcCfgYUYV.s32MemSize = input_width * input_height * 2;
    dev->stUvcCfgYUYV.s32Chn = chn_id;
    dev->stUvcCfgYUYV.isExit = 0;
    dev->stUvcCfgYUYV.u64FrameSeqNo = 0;
#ifdef SUPPORT_DSP_CSC
    dev->stUvcCfgYUYV.stDspCVParam.src_width = input_width;
    dev->stUvcCfgYUYV.stDspCVParam.src_height = input_height;
    dev->stUvcCfgYUYV.stDspCVParam.src_stride = dev->stUvcCfgYUYV.stDspCVParam.src_width;
    dev->stUvcCfgYUYV.stDspCVParam.src_stride_uv = dev->stUvcCfgYUYV.stDspCVParam.src_stride;
    dev->stUvcCfgYUYV.stDspCVParam.dst_stride = dev->stUvcCfgYUYV.stDspCVParam.src_stride * 2;
#endif
    pthread_create(&dev->get_stream_pid, NULL, get_uncompressed_stream, (void *)dev);

    printf("%s ---\n", __func__);

    return AX_SUCCESS;
}

AX_VOID uncompressed_chn_deinit(struct uvc_device *dev)
{
    if (NULL == dev) {
        printf("uncompressed_chn_deinit failed, dev is NULL!\n");
        return;
    }
    dev->stUvcCfgYUYV.isExit = 1;
}

AX_VOID *get_uncompressed_stream(AX_VOID *arg)
{
    printf("%s +++\n", __func__);

    prctl(PR_SET_NAME, "get_uncompressed_stream");
    AX_S32 s32Ret = 0;
    AX_S32 timeOutMs = 200;
    AX_IMG_INFO_T stImageInfo420 = {0};

    struct uvc_device *dev = (struct uvc_device *)arg;

#ifdef SUPPORT_DSP_CSC
    AX_U64 u64PhyAddrDst = 0;
    AX_VOID *pVirAddrDst = NULL;

    AX_MEM_INFO_T srcBuffer[3] = {0};
    AX_MEM_INFO_T dstBuffer[3] = {0};
    s32Ret = AX_SYS_MemAlloc(&u64PhyAddrDst, &pVirAddrDst, dev->stUvcCfgYUYV.u32Width * dev->stUvcCfgYUYV.u32Height * 2, \
                             128, (AX_S8 *)"uvc_dsp_csc");
    if (AX_SUCCESS != s32Ret) {
        printf("AX_SYS_MemAlloc failed , s32Ret=0x%x\n", s32Ret);
        return NULL;
    }
    dstBuffer[0].u64PhyAddr = u64PhyAddrDst;
#else
    AX_U8 *dst_img = (AX_U8 *)malloc(dev->stUvcCfgYUYV.u32Width * dev->stUvcCfgYUYV.u32Height * 2);
    if (NULL == dst_img) {
        printf("no enough memory for dst_img!\n");
        return NULL;
    }
#endif
    while (!uvc_exit && !dev->stUvcCfgYUYV.isExit) {
        s32Ret = AX_VIN_GetYuvFrame(dev->stUvcCfgYUYV.pStUVCCamera->nPipeId, dev->stUvcCfgYUYV.s32Chn, &stImageInfo420,
                                    timeOutMs);
        if (AX_SUCCESS != s32Ret) {
            printf("AX_VIN_GetYuvFrame fail, nRet:0x%x\n", s32Ret);
            continue;
        }

        if ((dev->stUvcCfgYUYV.u32Width != stImageInfo420.tFrameInfo.stVFrame.u32Width) || \
            (dev->stUvcCfgYUYV.u32Height != stImageInfo420.tFrameInfo.stVFrame.u32Height)) {
            goto _release;
        }
#ifdef SUPPORT_DSP_CSC
        srcBuffer[0].u64PhyAddr = stImageInfo420.tFrameInfo.stVFrame.u64PhyAddr[0];
        srcBuffer[1].u64PhyAddr = stImageInfo420.tFrameInfo.stVFrame.u64PhyAddr[0] + dev->stUvcCfgYUYV.u32Width *
                                  dev->stUvcCfgYUYV.u32Height;
        s32Ret = AX_DSP_CV_CvtColor(dev->stUvcCfgYUYV.u8DspId, AX_DSP_CV_CVTCOLOR_NV12_YUYV, srcBuffer, dstBuffer,
                                    &dev->stUvcCfgYUYV.stDspCVParam);
        if (AX_SUCCESS != s32Ret) {
            printf("AX_DSP_CV_CvtColor failed, s32Ret=0x%x\n", s32Ret);
            goto _release;
        }
#else
        s32Ret = uvc_nv12_yuv422(stImageInfo420.tFrameInfo.stVFrame.u32Width, stImageInfo420.tFrameInfo.stVFrame.u32Height, \
                                 (AX_U8 *)AX_POOL_GetBlockVirAddr(stImageInfo420.tFrameInfo.stVFrame.u32BlkId[0]), dst_img);
        if (AX_SUCCESS != s32Ret) {
            printf("uvc_nv12_yuv422 failed, s32Ret=0x%x\n", s32Ret);
            goto _release;
        }
#endif
        pthread_mutex_lock(&dev->img_mutex);
#ifdef SUPPORT_DSP_CSC
        uvc_img_cache_put(dev, pVirAddrDst, dev->stUvcCfgYUYV.u32Width * dev->stUvcCfgYUYV.u32Height * 2,
                          stImageInfo420.tFrameInfo.stVFrame.u64SeqNum, \
                          stImageInfo420.tFrameInfo.stVFrame.u64PTS, AX_TRUE);
#else
        uvc_img_cache_put(dev, dst_img, dev->stUvcCfgYUYV.u32Width * dev->stUvcCfgYUYV.u32Height * 2,
                          stImageInfo420.tFrameInfo.stVFrame.u64SeqNum, \
                          stImageInfo420.tFrameInfo.stVFrame.u64PTS, AX_TRUE);
#endif
        pthread_mutex_unlock(&dev->img_mutex);

_release:
        /*sleep for 1 ms for AX_VIN_ReleaseYuvFrame, otherwise the api invoke will cause segment fault*/
        usleep(1000);
        s32Ret = AX_VIN_ReleaseYuvFrame(dev->stUvcCfgYUYV.pStUVCCamera->nPipeId, dev->stUvcCfgYUYV.s32Chn, &stImageInfo420);
        if (AX_SUCCESS  != s32Ret) {
            printf("AX_VIN_ReleaseYuvFrame failed, s32Ret=0x%x\n", s32Ret);
            dev->stUvcCfgYUYV.isExit = 1;
            uvc_exit = 1;
        }
    }
#ifdef SUPPORT_DSP_CSC
    if ((0 != u64PhyAddrDst) && (NULL != pVirAddrDst)) {
        AX_SYS_MemFree(u64PhyAddrDst, pVirAddrDst);
        u64PhyAddrDst = 0;
        pVirAddrDst = NULL;
    }
#else
    if (NULL != dst_img) {
        free(dst_img);
        dst_img = NULL;
    }
#endif
    printf("%s ---\n", __func__);
    return (void *)(intptr_t)s32Ret;
}

AX_S32 uvc_set_chn_attr(AX_U8 pipe, AX_VIN_CHN_ID_E chn, AX_VIN_CHN_ATTR_T *chn_attr)
{
    return AX_VIN_SetChnAttr(pipe, chn, chn_attr);
}

AX_S32 uvc_get_chn_attr(AX_U8 pipe, AX_VIN_CHN_ID_E chn, AX_VIN_CHN_ATTR_T *chn_attr)
{
    return AX_VIN_GetChnAttr(pipe, chn, chn_attr);
}

#ifdef SUPPORT_DSP_CSC
AX_S32 uvc_init_dsp(AX_DSP_ID_E dsp_id, const AX_CHAR *itcm, const AX_CHAR *sram)
{
    AX_S32 s32Ret = -1;

    if ((dsp_id > AX_DSP_ID_1) || (dsp_id < AX_DSP_ID_0)) {
        printf("invalid dsp id\n");
        return -1;
    }

    if ((NULL == itcm) || (NULL == sram)) {
        printf("file path of dsp bin is NULL\n");
        return -1;
    }

    s32Ret = AX_DSP_PowerOn(dsp_id);
    if (AX_DSP_SUCCESS != s32Ret) {
        printf("AX_DSP_PowerOn failed, s32Ret=0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_DSP_LoadBin(dsp_id, itcm, AX_DSP_MEM_TYPE_ITCM);
    if (AX_DSP_SUCCESS != s32Ret) {
        printf("AX_DSP_LoadBin failed, s32Ret=0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_DSP_LoadBin(dsp_id, sram, AX_DSP_MEM_TYPE_SRAM);
    if (AX_DSP_SUCCESS != s32Ret) {
        printf("AX_DSP_LoadBin failed, s32Ret=0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_DSP_EnableCore(dsp_id);
    if (AX_DSP_SUCCESS != s32Ret) {
        printf("AX_DSP_EnableCore failed, s32Ret=0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_DSP_CV_Init(dsp_id);
    if (AX_DSP_SUCCESS != s32Ret) {
        printf("AX_DSP_CV_Init failed, s32Ret=0x%x\n", s32Ret);
        return -1;
    }

    return 0;
}

AX_S32 uvc_deinit_dsp(AX_DSP_ID_E dsp_id)
{
    AX_DSP_CV_Exit(dsp_id);
    AX_DSP_DisableCore(dsp_id);
    AX_DSP_PowerOff(dsp_id);
    return 0;
}
#else
AX_S32 uvc_nv12_yuv422(AX_U32 width, AX_U32 height, AX_U8 *src_img, AX_U8 *dst_img)
{
    if (NULL == src_img || NULL == dst_img) {
        printf("invalid image pointer!\n");
        return -1;
    }

    AX_S32 y_src_index = 0;
    AX_S32 uv_src_index = 0;
    AX_S32 dst_index = 0;

    AX_U8 *src_uv = src_img + width * height;

    for (AX_S32 i = 0; i < height / 2; i++) {
        AX_S32 uv_width = width;
        for (AX_S32 j = 0; j < uv_width / 2; j++) {
            //Y
            dst_img[dst_index] = src_img[y_src_index];
            dst_img[dst_index + uv_width * 2] = src_img[y_src_index + uv_width];
            dst_index++;
            y_src_index++;

            //U
            dst_img[dst_index] = src_uv[uv_src_index];
            dst_img[dst_index + uv_width * 2] = src_uv[uv_src_index];
            dst_index++;

            //Y
            dst_img[dst_index] = src_img[y_src_index];
            dst_img[dst_index + uv_width * 2] = src_img[y_src_index + uv_width];
            dst_index++;
            y_src_index++;

            //V
            dst_img[dst_index] = src_uv[uv_src_index + 1];
            dst_img[dst_index + uv_width * 2] = src_uv[uv_src_index + 1];
            dst_index++;

            uv_src_index += 2;
        }
        dst_index += uv_width * 2;
        y_src_index += uv_width;
    }
    return AX_SUCCESS;
}
#endif

AX_S32 uvc_draw_yuv422_color_stripe(AX_S32 width, AX_S32 height, AX_U8 *img)
{
    if (NULL == img) {
        printf("invalid image pointer!\n");
        return -1;
    }

    AX_S32 dst_index = 0;
    AX_S32 color_index = 0;
    stYUVColor color = color_arr[color_index];

    for (AX_S32 i = 0; i < height / 2; i++) {
        AX_S32 uv_width = width;
        for (AX_S32 j = 0; j < uv_width / 2; j++) {
            //Y
            img[dst_index] = color.y;
            img[dst_index + uv_width * 2] = color.y;
            dst_index++;

            //U
            img[dst_index] = color.u;
            img[dst_index + uv_width * 2] = color.u;
            dst_index++;

            //Y
            img[dst_index] = color.y;
            img[dst_index + uv_width * 2] = color.y;
            dst_index++;

            //V
            img[dst_index] = color.v;
            img[dst_index + uv_width * 2] = color.v;
            dst_index++;
        }
        dst_index += uv_width * 2;

        if (0 == (i + 1) % (height / 2 / COLOR_COUNT)) {
            color_index++;
            color = color_arr[color_index];
        }
    }
    return 0;
}
