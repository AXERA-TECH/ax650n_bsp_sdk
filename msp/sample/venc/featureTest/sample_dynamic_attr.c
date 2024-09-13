/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_case.h"
#include "sample_unit_test.h"

static AX_BOOL gLoopExit = AX_FALSE;
static SAMPLE_VENC_SENDFRAME_PARA_T gstFrmParam[MAX_VENC_CHN_NUM];
static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];

static void SigHandler(AX_S32 signo)
{
    SAMPLE_LOG("catch signal(%d).\n", signo);
    gLoopExit = AX_TRUE;
}

AX_S32 UTestDynamicAttr(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret;
    AX_U32 chnNum;
    AX_S32 chnIdx;
    chnNum = (pCml->defaultUt == VENC_TEST_ALL_CASE) ? UT_DEFAULT_ENC_NUM : pCml->chnNum;
    AX_PAYLOAD_TYPE_E enType;
    SAMPLE_VENC_RC_E enRcMode = pCml->rcMode;
    AX_U64 gVencCaseLoopExit = 0;

    signal(SIGINT, SigHandler);

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        memset(&gstFrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_SENDFRAME_PARA_T));
        memset(&gstStrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_GETSTREAM_PARA_T));

        if (pCml->bChnCustom)
            enType = SampleGetCodecType(pCml->codecType);
        else
            enType = SampleGetCodecType(chnIdx % SAMPLE_ALL_CODEC_TYPE);

        if (SampleInvalidEnType(pCml->ut, enType, pCml->rcMode))
            continue;

        if (PT_BUTT == enType) {
            SAMPLE_LOG_ERR("chn-%d: Invalid codec type!\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        s32Ret = COMMON_VENC_Start(chnIdx, enType, enRcMode, pCml);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_Start failed.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        SampleSendFrameInit(chnIdx, enType, &gstFrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartSendFrame(&gstFrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_StartSendFrame err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        sleep(1);

        SampleGetStreamInit(chnIdx, enType, &gstStrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartGetStream(&gstStrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_StartGetStream err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        COMMON_VENC_AdjustLoopExit(&gVencCaseLoopExit, chnIdx);
        sleep(1);
    }

    while ((!gLoopExit) && (gVencCaseLoopExit != pCml->vencLoopExit))
        sleep(1);

FREE:

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        if (SampleInvalidEnType(pCml->ut, enType, pCml->rcMode))
            continue;
        COMMON_VENC_StopSendFrame(&gstFrmParam[chnIdx]);
        COMMON_VENC_SendGetDelay(chnIdx, gLoopExit, &gstFrmParam[chnIdx], &gstStrmParam[chnIdx]);
        COMMON_VENC_Stop(chnIdx);
        COMMON_VENC_StopGetStream(&gstStrmParam[chnIdx]);
    }

    pCml->vencLoopExit = 0;

    return s32Ret;
}

AX_BOOL SampleInvalidEnType(AX_S32 ut, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode)
{
    AX_BOOL ret = AX_FALSE;

    switch (ut) {
    case UT_CASE_RATE_JAM:
    case UT_CASE_BIT_RATE:
    case UT_CASE_RC_MODE:
        if (PT_JPEG == enType)
            ret = AX_TRUE;
        break;
    case UT_CASE_VENC_ROI:
    case UT_CASE_VUI:
    case UT_CASE_VIR_INTRA_INTERVAL:
    case UT_CASE_INTRA_REFRESH:
    case UT_CASE_REQUEST_IDR:
    case UT_CASE_SUPER_FRAME:
    case UT_CASE_SLICE_SPLIT:
    case UT_CASE_FRAME_RATE:
        if ((PT_MJPEG == enType) && (SAMPLE_RC_AVBR == rcMode || SAMPLE_RC_QPMAP == rcMode))
            ret = AX_TRUE;
        break;
    case UT_CASE_RESOLUTION:
        if ((PT_JPEG == enType) && (SAMPLE_RC_AVBR == rcMode || SAMPLE_RC_QPMAP == rcMode))
            ret = AX_TRUE;
        break;
    default:
        break;
    }

    return ret;
}

AX_S32 SampleTestBitrateJamStrategy(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RATE_JAM_CFG_T stRateJamParam, stGetRateJamParam;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = NULL;

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    memset(&stRateJamParam, 0, sizeof(AX_VENC_RATE_JAM_CFG_T));
    memset(&stGetRateJamParam, 0, sizeof(AX_VENC_RATE_JAM_CFG_T));

    s32Ret = AX_VENC_GetRateJamStrategy(VeChn, &stRateJamParam);
    if (s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_GetRateJamStrategy error!\n");
        return -1;
    }

    stRateJamParam.bDropFrmEn = AX_TRUE;
    stRateJamParam.enDropFrmMode = pstArg->drpFrmMode;
    stRateJamParam.u32EncFrmGaps = pstArg->encFrmGap;
    stRateJamParam.u32DropFrmThrBps = pstArg->frmThrBps;

    s32Ret = AX_VENC_SetRateJamStrategy(VeChn, &stRateJamParam);
    if (s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_SetRateJamStrategy error!\n");
        return -1;
    }

    s32Ret = AX_VENC_GetRateJamStrategy(VeChn, &stGetRateJamParam);
    if (s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_GetRateJamStrategy error!\n");
        return -1;
    }

    SAMPLE_LOG_DEBUG("chn-%d: DropFrm=%d, DropFrmMode=%d, EncFrmGaps=%d, DropFrmThrBps=%d.\n", VeChn,
                     stGetRateJamParam.bDropFrmEn, stGetRateJamParam.enDropFrmMode, stGetRateJamParam.u32EncFrmGaps,
                     stGetRateJamParam.u32DropFrmThrBps);

    return 0;
}

AX_S32 SampleTestSuperFrameStrategy(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = -1;

    SAMPLE_VENC_SENDFRAME_PARA_T *pCml = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;
    AX_VENC_SUPERFRAME_CFG_T stSupFrm, stSupFrmGet;

    s32Ret = AX_VENC_GetSuperFrameStrategy(VeChn, &stSupFrm);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_GetSuperFrameStrategy error!\n", VeChn);
        return -1;
    }

    stSupFrm.bStrategyEn = AX_TRUE;
    stSupFrm.u32SuperIFrmBitsThr = pCml->thrI;
    stSupFrm.u32SuperPFrmBitsThr = pCml->thrP;
    stSupFrm.enRcPriority = pCml->pri;

    s32Ret = AX_VENC_SetSuperFrameStrategy(VeChn, &stSupFrm);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_SetSuperFrameStrategy error!\n", VeChn);
        return -1;
    }

    s32Ret = AX_VENC_GetSuperFrameStrategy(VeChn, &stSupFrmGet);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_GetSuperFrameStrategy error!\n", VeChn);
        return -1;
    }

    return AX_SUCCESS;
}

AX_S32 SampleTestBitRate(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RC_PARAM_T stRcParam;
    SAMPLE_VENC_SENDFRAME_PARA_T *pCml = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;
    AX_U32 oldBitRate = 0;

    memset(&stRcParam, 0, sizeof(AX_VENC_RC_PARAM_T));

    if (pCml->enType == PT_JPEG)
        return 0;

    s32Ret = AX_VENC_GetRcParam(VeChn, &stRcParam);
    if (s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_GetRcParam error!\n", VeChn);
        return -1;
    }

    switch (pCml->enType) {
    case PT_H264:
        if (stRcParam.enRcMode != AX_VENC_RC_MODE_H264CBR) {
            SAMPLE_LOG_ERR("VeChn:%d enRcMode %d can't set bit rate!\n", VeChn, stRcParam.enRcMode);
            return -1;
        } else {
            oldBitRate = stRcParam.stH264Cbr.u32BitRate;
            stRcParam.stH264Cbr.u32BitRate = 6000;
        }
        break;
    case PT_H265:
        if (stRcParam.enRcMode != AX_VENC_RC_MODE_H265CBR) {
            SAMPLE_LOG_ERR("VeChn:%d enRcMode %d can't set bit rate!\n", VeChn, stRcParam.enRcMode);
            return -1;
        } else {
            oldBitRate = stRcParam.stH265Cbr.u32BitRate;
            stRcParam.stH265Cbr.u32BitRate = 6000;
        }
        break;
    case PT_MJPEG:
        if (stRcParam.enRcMode != AX_VENC_RC_MODE_MJPEGCBR) {
            SAMPLE_LOG_ERR("VeChn:%d enRcMode %d can't set bit rate!\n", VeChn, stRcParam.enRcMode);
            return -1;
        } else {
            oldBitRate = stRcParam.stMjpegCbr.u32BitRate;
            stRcParam.stMjpegCbr.u32BitRate = 6000;
        }
        break;
    default:
        break;
    }

    SAMPLE_LOG_DEBUG("VeChn:%d ------- enRcMode:%d, type %d, Get old bitrate %d ,\n", VeChn, stRcParam.enRcMode,
                     pCml->enType, oldBitRate);

    s32Ret = AX_VENC_SetRcParam(VeChn, &stRcParam);
    if (s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_SetRcParam error!s32Ret=0x%x\n", VeChn, s32Ret);
        return -1;
    }

    SAMPLE_LOG_DEBUG("VeChn:%d ------- enRcMode:%d, type %d, Set new bitrate %d ,\n", VeChn, stRcParam.enRcMode,
                     pCml->enType, stRcParam.stH264Cbr.u32BitRate);

    return 0;
}

AX_S32 SampleTestFrameRate(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RC_PARAM_T stRcParam;
    SAMPLE_VENC_SENDFRAME_PARA_T *pCml = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    memset(&stRcParam, 0, sizeof(stRcParam));

    if (pCml->enType == PT_JPEG)
        return 0;

    s32Ret = AX_VENC_GetRcParam(VeChn, &stRcParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_GetRcParam error!\n", VeChn);
        return -1;
    }

    stRcParam.stFrameRate.fSrcFrameRate = 20.0;
    stRcParam.stFrameRate.fDstFrameRate = 10.0;

    s32Ret = AX_VENC_SetRcParam(VeChn, &stRcParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("VeChn:%d AX_VENC_SetRcParam error!s32Ret=0x%x\n", VeChn, s32Ret);
        return -1;
    }

    if (PT_H265 == pCml->enType) {
        s32Ret = AX_VENC_RequestIDR(VeChn, pCml->bInsertIDR);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: AX_VENC_RequestIDR err, bInstant=%d\n", VeChn, pCml->bInsertIDR);
            return -1;
        }
    }

    return 0;
}

static void SampleInfoDynRcModeDebug(AX_S32 VencChn, AX_PAYLOAD_TYPE_E enType, AX_VENC_RC_PARAM_T *pStRcParam)
{
    if (NULL == pStRcParam) {
        SAMPLE_LOG_ERR("NULL pointer\n");
        return;
    }

    if (enType == PT_H264)
        SAMPLE_LOG_DEBUG(
            "VencChn %d, enRcMode %d, u32Gop %d, u32IQp %d, u32PQp %d, u32BQp "
            "%d\n",
            VencChn, pStRcParam->enRcMode, pStRcParam->stH264FixQp.u32Gop, pStRcParam->stH264FixQp.u32IQp,
            pStRcParam->stH264FixQp.u32PQp, pStRcParam->stH264FixQp.u32BQp);
    else if (enType == PT_H265)
        SAMPLE_LOG_DEBUG(
            "VencChn %d, enRcMode %d, u32Gop %d, u32IQp %d, u32PQp %d, u32BQp "
            "%d\n",
            VencChn, pStRcParam->enRcMode, pStRcParam->stH265FixQp.u32Gop, pStRcParam->stH265FixQp.u32IQp,
            pStRcParam->stH265FixQp.u32PQp, pStRcParam->stH265FixQp.u32BQp);
    else if (enType == PT_MJPEG)
        SAMPLE_LOG_DEBUG("VencChn %d, enRcMode %d s32FixedQp %d\n", VencChn, pStRcParam->enRcMode,
                         pStRcParam->stMjpegFixQp.s32FixedQp);

    return;
}

AX_S32 SampleDynRcMode(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_RC_PARAM_T stRcParam;

    AX_VENC_H264_FIXQP_T stH264FixQp;
    AX_VENC_H265_FIXQP_T stH265FixQp;
    AX_VENC_MJPEG_FIXQP_T stMjpegFixQp;

    AX_VENC_H264_CBR_T stH264Cbr;
    AX_VENC_H265_CBR_T stH265Cbr;
    AX_VENC_MJPEG_CBR_T stMjpegCbr;

    AX_VENC_H264_VBR_T stH264Vbr;
    AX_VENC_H265_VBR_T stH265Vbr;
    AX_VENC_MJPEG_VBR_T stMjpegVbr;

    AX_VENC_H264_AVBR_T stH264Avbr;
    AX_VENC_H265_AVBR_T stH265Avbr;

    AX_VENC_H264_QPMAP_T stH264QpMap;
    AX_VENC_H265_QPMAP_T stH265QpMap;

    AX_VENC_H264_CVBR_T stH264Cvbr;
    AX_VENC_H265_CVBR_T stH265Cvbr;

    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = NULL;

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;
    memset(&stRcParam, 0, sizeof(AX_VENC_RC_PARAM_T));

    s32Ret = AX_VENC_GetRcParam(VencChn, &stRcParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_GetRcParam:%d failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    SAMPLE_LOG_DEBUG("Old RcMode Param:\n");
    SampleInfoDynRcModeDebug(VencChn, pstArg->enType, &stRcParam);

    stRcParam.stFrameRate.fSrcFrameRate = pstArg->srcFrameRate;
    stRcParam.stFrameRate.fDstFrameRate = pstArg->dstFrameRate;

    if (SAMPLE_RC_FIXQP == pstArg->rcModeNew) {
        if (pstArg->enType == PT_H264) {
            memset(&stH264FixQp, 0, sizeof(AX_VENC_H264_FIXQP_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H264FIXQP;
            stH264FixQp.u32Gop = pstArg->gopLen - 2;
            stH264FixQp.u32IQp = pstArg->IQp;
            stH264FixQp.u32PQp = pstArg->PQp;
            stH264FixQp.u32BQp = pstArg->BQp;
            memcpy(&stRcParam.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
        } else if (pstArg->enType == PT_H265) {
            memset(&stH265FixQp, 0, sizeof(AX_VENC_H265_FIXQP_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H265FIXQP;
            stH265FixQp.u32Gop = pstArg->gopLen - 2;
            stH265FixQp.u32IQp = pstArg->IQp;
            stH265FixQp.u32PQp = pstArg->PQp;
            stH265FixQp.u32BQp = pstArg->BQp;
            memcpy(&stRcParam.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
        } else if (pstArg->enType == PT_MJPEG) {
            memset(&stMjpegFixQp, 0, sizeof(AX_VENC_MJPEG_FIXQP_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
            stMjpegFixQp.s32FixedQp = (-1 == pstArg->fixedQp) ? 30 : pstArg->fixedQp;

            memcpy(&stRcParam.stMjpegFixQp, &stMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
        } else {
            return AX_SUCCESS;
        }
    } else if (SAMPLE_RC_CBR == pstArg->rcModeNew) {
        if (pstArg->enType == PT_H264) {
            memset(&stH264Cbr, 0, sizeof(AX_VENC_H264_CBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H264CBR;
            stH264Cbr.u32Gop = pstArg->gopLen + 2;
            stH264Cbr.u32BitRate = pstArg->bitRate + 1000;
            stH264Cbr.u32MinQp = pstArg->qpMin + 2;
            stH264Cbr.u32MaxQp = pstArg->qpMax - 2;
            stH264Cbr.u32MinIQp = pstArg->qpMinI + 2;
            stH264Cbr.u32MaxIQp = pstArg->qpMaxI - 2;
            stH264Cbr.s32IntraQpDelta = 0;
            stH264Cbr.u32MaxIprop = pstArg->maxIprop;
            stH264Cbr.u32MinIprop = pstArg->minIprop;

            memcpy(&stRcParam.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
        } else if (pstArg->enType == PT_H265) {
            memset(&stH265Cbr, 0, sizeof(AX_VENC_H265_CBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H265CBR;
            stH265Cbr.u32Gop = pstArg->gopLen + 2;
            stH265Cbr.u32BitRate = pstArg->bitRate + 1000;
            stH265Cbr.u32MinQp = pstArg->qpMin + 2;
            stH265Cbr.u32MaxQp = pstArg->qpMax - 2;
            stH265Cbr.u32MinIQp = pstArg->qpMinI + 2;
            stH265Cbr.u32MaxIQp = pstArg->qpMaxI - 2;
            stH265Cbr.s32IntraQpDelta = 0;

            stH265Cbr.u32MaxIprop = pstArg->maxIprop;
            stH265Cbr.u32MinIprop = pstArg->minIprop;

            memcpy(&stRcParam.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
        } else if (pstArg->enType == PT_MJPEG) {
            memset(&stMjpegCbr, 0, sizeof(AX_VENC_MJPEG_CBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;

            stMjpegCbr.u32BitRate = pstArg->bitRate + 1000;
            stMjpegCbr.u32MinQp = pstArg->qpMin + 2;
            stMjpegCbr.u32MaxQp = pstArg->qpMax - 2;

            memcpy(&stRcParam.stMjpegCbr, &stMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
        }
    } else if (SAMPLE_RC_VBR == pstArg->rcModeNew) {
        if (pstArg->enType == PT_H264) {
            memset(&stH264Vbr, 0, sizeof(AX_VENC_H264_VBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H264VBR;
            stH264Vbr.u32Gop = pstArg->gopLen + 2;
            stH264Vbr.u32MaxBitRate = pstArg->bitRate + 1000;
            stH264Vbr.u32MinQp = pstArg->qpMin + 2;
            stH264Vbr.u32MaxQp = pstArg->qpMax - 2;
            stH264Vbr.u32MinIQp = pstArg->qpMinI + 2;
            stH264Vbr.u32MaxIQp = pstArg->qpMaxI - 2;
            stH264Vbr.s32IntraQpDelta = 0;

            memcpy(&stRcParam.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));
        } else if (pstArg->enType == PT_H265) {
            memset(&stH265Vbr, 0, sizeof(AX_VENC_H265_VBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H265VBR;
            stH265Vbr.u32Gop = pstArg->gopLen + 4;
            stH265Vbr.u32MaxBitRate = pstArg->bitRate;
            stH265Vbr.u32MinQp = pstArg->qpMin;
            stH265Vbr.u32MaxQp = pstArg->qpMax;
            stH265Vbr.u32MinIQp = pstArg->qpMinI;
            stH265Vbr.u32MaxIQp = pstArg->qpMaxI;
            stH265Vbr.s32IntraQpDelta = 0;

            memcpy(&stRcParam.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
        } else if (pstArg->enType == PT_MJPEG) {
            memset(&stMjpegVbr, 0, sizeof(AX_VENC_MJPEG_VBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;

            stMjpegVbr.u32MaxBitRate = pstArg->bitRate + 1000;
            stMjpegVbr.u32MinQp = pstArg->qpMin + 2;
            stMjpegVbr.u32MaxQp = pstArg->qpMax - 2;

            memcpy(&stRcParam.stMjpegVbr, &stMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
        }
    } else if (SAMPLE_RC_AVBR == pstArg->rcModeNew) {
        if (pstArg->enType == PT_H264) {
            memset(&stH264Avbr, 0, sizeof(AX_VENC_H264_AVBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H264AVBR;
            stH264Avbr.u32Gop = pstArg->gopLen + 2;
            stH264Avbr.u32MaxBitRate = pstArg->bitRate + 1000;
            stH264Avbr.u32MinQp = pstArg->qpMin + 2;
            stH264Avbr.u32MaxQp = pstArg->qpMax - 2;
            stH264Avbr.u32MinIQp = pstArg->qpMinI + 2;
            stH264Avbr.u32MaxIQp = pstArg->qpMaxI - 2;
            stH264Avbr.s32IntraQpDelta = 0;

            memcpy(&stRcParam.stH264AVbr, &stH264Avbr, sizeof(AX_VENC_H264_AVBR_T));
        } else if (pstArg->enType == PT_H265) {
            memset(&stH265Avbr, 0, sizeof(AX_VENC_H265_AVBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H265AVBR;
            stH265Avbr.u32Gop = pstArg->gopLen + 2;
            stH265Avbr.u32MaxBitRate = pstArg->bitRate + 1000;
            stH265Avbr.u32MinQp = pstArg->qpMin;
            stH265Avbr.u32MaxQp = pstArg->qpMax;
            stH265Avbr.u32MinIQp = pstArg->qpMinI;
            stH265Avbr.u32MaxIQp = pstArg->qpMaxI;
            stH265Avbr.s32IntraQpDelta = 0;

            memcpy(&stRcParam.stH265AVbr, &stH265Avbr, sizeof(AX_VENC_H265_AVBR_T));
        }
    } else if (SAMPLE_RC_QPMAP == pstArg->rcModeNew) {
        if (pstArg->enType == PT_H264) {
            memset(&stH264QpMap, 0, sizeof(AX_VENC_H264_QPMAP_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H264QPMAP;

            stH264QpMap.stQpmapInfo.enQpmapQpType = pstArg->qpMapQpType;
            stH264QpMap.stQpmapInfo.enQpmapBlockUnit = AX_VENC_QPMAP_BLOCK_UNIT_64x64;
            stH264QpMap.stQpmapInfo.enQpmapBlockType = pstArg->qpMapBlkType;
            stH264QpMap.stQpmapInfo.enCtbRcMode = AX_VENC_RC_CTBRC_DISABLE;

            stH264QpMap.u32Gop = pstArg->gopLen + 1;
            stH264QpMap.u32TargetBitRate = pstArg->bitRate + 2000;
            memcpy(&stRcParam.stH264QpMap, &stH264QpMap, sizeof(AX_VENC_H264_QPMAP_T));
        } else if (pstArg->enType == PT_H265) {
            memset(&stH265QpMap, 0, sizeof(AX_VENC_H265_QPMAP_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H265QPMAP;

            stH265QpMap.stQpmapInfo.enQpmapQpType = pstArg->qpMapQpType;
            stH265QpMap.stQpmapInfo.enQpmapBlockUnit = AX_VENC_QPMAP_BLOCK_UNIT_64x64;
            stH265QpMap.stQpmapInfo.enQpmapBlockType = pstArg->qpMapBlkType;
            stH265QpMap.stQpmapInfo.enCtbRcMode = AX_VENC_RC_CTBRC_DISABLE;

            stH265QpMap.u32Gop = pstArg->gopLen + 1;
            stH265QpMap.u32TargetBitRate = pstArg->bitRate + 2000;
            memcpy(&stRcParam.stH265QpMap, &stH265QpMap, sizeof(AX_VENC_H265_QPMAP_T));
        }
    } else if (SAMPLE_RC_CVBR == pstArg->rcModeNew) {
        if (pstArg->enType == PT_H264) {
            memset(&stH264Cvbr, 0, sizeof(AX_VENC_H264_CVBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H264CVBR;
            stH264Cvbr.u32Gop = pstArg->gopLen + 2;
            stH264Cvbr.u32MaxBitRate = pstArg->bitRate + 1000;
            stH264Cvbr.u32MinQp = pstArg->qpMin + 2;
            stH264Cvbr.u32MaxQp = pstArg->qpMax - 2;
            stH264Cvbr.u32MinIQp = pstArg->qpMinI + 2;
            stH264Cvbr.u32MaxIQp = pstArg->qpMaxI - 2;

            stH264Cvbr.u32MaxIprop = pstArg->maxIprop;
            stH264Cvbr.u32MinIprop = pstArg->minIprop;

            stH264Cvbr.u32MinQpDelta = pstArg->minQpDelta;
            stH264Cvbr.u32MaxQpDelta = pstArg->maxQpDelta;

            stH264Cvbr.u32LongTermMinBitrate = pstArg->ltMinBt;
            stH264Cvbr.u32LongTermMaxBitrate = pstArg->ltMaxBt;
            stH264Cvbr.u32LongTermStatTime = pstArg->ltStaTime;
            stH264Cvbr.u32ShortTermStatTime = pstArg->shtStaTime;

            memcpy(&stRcParam.stH264CVbr, &stH264Cvbr, sizeof(AX_VENC_H264_CVBR_T));
        } else if (pstArg->enType == PT_H265) {
            memset(&stH265Cvbr, 0, sizeof(AX_VENC_H265_CVBR_T));
            stRcParam.enRcMode = AX_VENC_RC_MODE_H265CVBR;
            stH265Cvbr.u32Gop = pstArg->gopLen + 2;
            stH265Cvbr.u32MaxBitRate = pstArg->bitRate + 1000;
            stH265Cvbr.u32MinQp = pstArg->qpMin + 2;
            stH265Cvbr.u32MaxQp = pstArg->qpMax - 2;
            stH265Cvbr.u32MinIQp = pstArg->qpMinI + 2;
            stH265Cvbr.u32MaxIQp = pstArg->qpMaxI - 2;

            stH265Cvbr.u32MaxIprop = pstArg->maxIprop;
            stH265Cvbr.u32MinIprop = pstArg->minIprop;

            stH265Cvbr.u32MinQpDelta = pstArg->minQpDelta;
            stH265Cvbr.u32MaxQpDelta = pstArg->maxQpDelta;

            stH265Cvbr.u32LongTermMinBitrate = pstArg->ltMinBt;
            stH265Cvbr.u32LongTermMaxBitrate = pstArg->ltMaxBt;
            stH265Cvbr.u32LongTermStatTime = pstArg->ltStaTime;
            stH265Cvbr.u32ShortTermStatTime = pstArg->shtStaTime;

            memcpy(&stRcParam.stH265CVbr, &stH265Cvbr, sizeof(AX_VENC_H265_CVBR_T));
        }
    }

    s32Ret = AX_VENC_SetRcParam(VencChn, &stRcParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_SetRcParam:%d failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    return AX_SUCCESS;
}

AX_S32 SampleDynVui(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_VUI_PARAM_T stVui;

    memset(&stVui, 0, sizeof(AX_VENC_VUI_PARAM_T));

    SAMPLE_VENC_SENDFRAME_PARA_T *pSend = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pSend->ptrPrivate;

    s32Ret = AX_VENC_GetVuiParam(VencChn, &stVui);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetVuiParam failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    stVui.stVuiVideoSignal.video_signal_type_present_flag = pCml->bSignalPresent;
    stVui.stVuiVideoSignal.video_format = pCml->videoFormat;
    stVui.stVuiVideoSignal.video_full_range_flag = pCml->bFullRange;
    stVui.stVuiVideoSignal.colour_description_present_flag = pCml->bColorPresent;
    stVui.stVuiVideoSignal.colour_primaries = pCml->colorPrimaries;
    stVui.stVuiVideoSignal.transfer_characteristics = pCml->transferCharacter;
    stVui.stVuiVideoSignal.matrix_coefficients = pCml->matrixCoeffs;

    s32Ret = AX_VENC_SetVuiParam(VencChn, &stVui);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_SetVuiParam failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    return AX_SUCCESS;
}

AX_S32 SampleVirIntraInterval(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_CHN_ATTR_T stChnAttr;

    memset(&stChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    s32Ret = AX_VENC_GetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetChnAttr failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    if ((stChnAttr.stVencAttr.enType == PT_JPEG) || (stChnAttr.stVencAttr.enType == PT_MJPEG))
        return 0;

    SAMPLE_LOG_DEBUG("chn-%d: interval = %d.\n", VencChn, stChnAttr.stGopAttr.stOneLTR.stPicSpecialConfig.s32Interval);

    stChnAttr.stGopAttr.stOneLTR.stPicSpecialConfig.s32Interval = 2;

    s32Ret = AX_VENC_SetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_SetChnAttr failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    return 0;
}

AX_S32 SampleIntraRefresh(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = NULL;
    AX_VENC_INTRA_REFRESH_T stIntraRefresh;

    memset(&stIntraRefresh, 0, sizeof(AX_VENC_INTRA_REFRESH_T));

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    s32Ret = AX_VENC_GetIntraRefresh(VencChn, &stIntraRefresh);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetIntraRefresh failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    stIntraRefresh.bRefresh = AX_TRUE;
    stIntraRefresh.u32RefreshNum = pstArg->u32RefreshNum;

    s32Ret = AX_VENC_SetIntraRefresh(VencChn, &stIntraRefresh);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_SetIntraRefresh failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    s32Ret = AX_VENC_GetIntraRefresh(VencChn, &stIntraRefresh);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetIntraRefresh failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    SAMPLE_LOG_DEBUG("chn-%d: bRefresh=%d, refreshNum=%d, reqIQp=%d, refreshMode=%d.\n", VencChn,
                     stIntraRefresh.bRefresh, stIntraRefresh.u32RefreshNum, stIntraRefresh.u32ReqIQp,
                     stIntraRefresh.enIntraRefreshMode);

    return 0;
}

AX_S32 SampleDynResolution(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_CHN_ATTR_T stChnAttr;
    AX_POOL_CONFIG_T stPoolConfig;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = NULL;

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;
    VENC_CHN VeChn = pstArg->VeChn;

    if (!pstArg->bDynRes) {
        SAMPLE_LOG_ERR("chn-%d: Disable dynamic resolution!\n", VeChn);
        return -1;
    }

    if (pstArg->fileInput) {
        fclose(pstArg->fFileIn);
        pstArg->fFileIn = NULL;

        pstArg->fFileIn = fopen(pstArg->newInput, "rb");
        if (NULL == pstArg->fFileIn) {
            SAMPLE_LOG_ERR("chn-%d: Open input file(%s) error!\n", VeChn, pstArg->newInput);
            return -1;
        }
    }

    /* update new resolution params */
    pstArg->frameSize = pstArg->blkSize = pstArg->newPicW * pstArg->newPicH * 2;
    switch (pstArg->eFmt) {
    case AX_FORMAT_YUV420_PLANAR:
        pstArg->strideY = pstArg->newPicW;
        pstArg->strideU = pstArg->strideV = pstArg->newPicW / 2;
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        pstArg->strideY = pstArg->newPicW;
        pstArg->strideU = pstArg->newPicW;
        break;
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        pstArg->strideY = 2 * pstArg->newPicW;
        break;
    default:
        SAMPLE_LOG_ERR("chn-%d: image format(%d) unsupport change stride dynamically!\n", VeChn, pstArg->eFmt);
        return -1;
    }

    pstArg->width = pstArg->newPicW;
    pstArg->height = pstArg->newPicH;

    /* use user pool */
    memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
    stPoolConfig.MetaSize = 512;
    stPoolConfig.BlkCnt = 2;
    stPoolConfig.BlkSize = pstArg->blkSize;
    stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
    memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
    strcpy((AX_CHAR *)stPoolConfig.PartitionName, "anonymous");

    pstArg->poolId = AX_POOL_CreatePool(&stPoolConfig);
    if (AX_INVALID_POOLID == pstArg->poolId) {
        SAMPLE_LOG_ERR("Create pool err.\n");
        goto CLOSE_FD;
    }

    s32Ret = AX_VENC_StopRecvFrame(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_StopRecvFrame failed! ret=0x%x\n", VencChn, s32Ret);
        goto CLOSE_FD;
    }

    s32Ret = AX_VENC_ResetChn(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_ResetChn failed! ret=0x%x\n", VencChn, s32Ret);
        goto CLOSE_FD;
    }

    memset(&stChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    s32Ret = AX_VENC_GetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetChnAttr failed! ret=0x%x\n", VencChn, s32Ret);
        goto CLOSE_FD;
    }

    SAMPLE_LOG_DEBUG("chn-%d: last resolution: %u*%u.\n", VencChn, stChnAttr.stVencAttr.u32PicWidthSrc,
                     stChnAttr.stVencAttr.u32PicHeightSrc);

    stChnAttr.stVencAttr.u32PicWidthSrc = pstArg->newPicW;
    stChnAttr.stVencAttr.u32PicHeightSrc = pstArg->newPicH;
    stChnAttr.stVencAttr.u32MaxPicWidth =
        stChnAttr.stVencAttr.u32MaxPicWidth > pstArg->newPicW ? stChnAttr.stVencAttr.u32MaxPicWidth : pstArg->newPicW;
    stChnAttr.stVencAttr.u32MaxPicHeight =
        stChnAttr.stVencAttr.u32MaxPicHeight > pstArg->newPicH ? stChnAttr.stVencAttr.u32MaxPicHeight : pstArg->newPicH;

    s32Ret = AX_VENC_SetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_SetChnAttr failed! ret=0x%x\n", VencChn, s32Ret);
        goto CLOSE_FD;
    }

    s32Ret = AX_VENC_GetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetChnAttr failed! ret=0x%x\n", VencChn, s32Ret);
        goto CLOSE_FD;
    }

    SAMPLE_LOG_DEBUG("chn-%d: new resolution: %u*%u.\n", VencChn, stChnAttr.stVencAttr.u32PicWidthSrc,
                     stChnAttr.stVencAttr.u32PicHeightSrc);

    s32Ret = AX_VENC_StartRecvFrame(VeChn, NULL);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_StartRecvFrame failed! ret=0x%x\n", VencChn, s32Ret);
        goto CLOSE_FD;
    }

    return 0;

CLOSE_FD:
    if (pstArg->fileInput) {
        fclose(pstArg->fFileIn);
        pstArg->fFileIn = NULL;
    }

    return -1;
}

AX_S32 SampleRequestIDR(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = NULL;

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    s32Ret = AX_VENC_RequestIDR(VeChn, pstArg->bInsertIDR);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_RequestIDR err, bInstant=%d\n", VeChn, pstArg->bInsertIDR);
        return -1;
    }

    SAMPLE_LOG_DEBUG("chn-%d: request IDR, bInsertIDR=%d !\n", VeChn, pstArg->bInsertIDR);

    return 0;
}

AX_S32 SampleSetUserData(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = -1;
    AX_VENC_USR_DATA_T stUsrData;
    AX_VENC_USR_DATA_T stUsrDataGet;

    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    memset(&stUsrData, 0x0, sizeof(AX_VENC_USR_DATA_T));
    memset(&stUsrDataGet, 0x0, sizeof(AX_VENC_USR_DATA_T));

    stUsrData.bEnable = AX_TRUE;
    stUsrData.u32DataSize = pstArg->uDataSize;

    stUsrData.pu8UsrData = malloc(stUsrData.u32DataSize);
    if (NULL == stUsrData.pu8UsrData) {
        SAMPLE_LOG_ERR("chn-%d: malloc user data err!\n", VeChn);
        return -1;
    }

    memset(stUsrData.pu8UsrData, 'C', stUsrData.u32DataSize);

    s32Ret = AX_VENC_SetUsrData(VeChn, &stUsrData);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_InsertUserData error!\n", VeChn);
        goto exit;
    }

    stUsrDataGet.pu8UsrData = (AX_U8 *)malloc(stUsrData.u32DataSize);
    if (NULL == stUsrDataGet.pu8UsrData) {
        SAMPLE_LOG_ERR("chn-%d: malloc user data mem err.\n", VeChn);
        s32Ret = -1;
        goto exit;
    }

    s32Ret = AX_VENC_GetUsrData(VeChn, &stUsrDataGet);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_InsertUserData error!\n", VeChn);
        goto exit;
    }

    SAMPLE_LOG("chn-%d: set usr data: bEnable %d, dataSize %d\n", VeChn, stUsrDataGet.bEnable,
               stUsrDataGet.u32DataSize);

    s32Ret = AX_SUCCESS;

exit:
    if (stUsrDataGet.pu8UsrData)
        free(stUsrDataGet.pu8UsrData);

    if (stUsrData.pu8UsrData)
        free(stUsrData.pu8UsrData);

    return s32Ret;
}

AX_S32 SampleTestSliceSplit(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg = NULL;

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    AX_VENC_SLICE_SPLIT_T stSliceSplit;

    memset(&stSliceSplit, 0, sizeof(AX_VENC_SLICE_SPLIT_T));

    s32Ret = AX_VENC_GetSliceSplit(VeChn, &stSliceSplit);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetSliceSplit err.\n", VeChn);
        return -1;
    }

    SAMPLE_LOG_DEBUG("chn-%d: bSplit=%d, SliceNum=%u.\n", VeChn, stSliceSplit.bSplit, stSliceSplit.u32LcuLineNum);

    stSliceSplit.bSplit = AX_TRUE;
    stSliceSplit.u32LcuLineNum = pstArg->sliceNum;
    s32Ret = AX_VENC_SetSliceSplit(VeChn, &stSliceSplit);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_SetSliceSplit err.\n", VeChn);
        return -1;
    }

    s32Ret = AX_VENC_GetSliceSplit(VeChn, &stSliceSplit);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_GetSliceSplit err.\n", VeChn);
        return -1;
    }

    SAMPLE_LOG_DEBUG("chn-%d: bSplit=%d, SliceNum=%u.\n", VeChn, stSliceSplit.bSplit, stSliceSplit.u32LcuLineNum);

    return 0;
}

AX_S32 SampleTestResetChn(AX_S32 VeChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_RECV_PIC_PARAM_T stRecv;

    s32Ret = AX_VENC_StopRecvFrame(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: stop recving frame error, ret=%x\n", VeChn, s32Ret);
        return -1;
    }
    s32Ret = AX_VENC_ResetChn(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: reset channel error, ret=%x\n", VeChn, s32Ret);
        return -1;
    }
    s32Ret = AX_VENC_StartRecvFrame(VeChn, &stRecv);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: start channel error, ret=%x\n", VeChn, s32Ret);
        return -1;
    }

    return 0;
}