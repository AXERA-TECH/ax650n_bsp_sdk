/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/


#include "common_venc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "ax_global_type.h"
#include "ax_sys_api.h"
#include "ax_venc_api.h"
#include "ax_venc_comm.h"
#include "ax_venc_rc.h"
#include "common_venc_log.h"

static AX_U32 gTileSizeTable[] = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320};

AX_VOID COMMON_VENC_AdjustLoopExit(AX_U64 *pVencLoopExit, AX_U32 chn)
{
    (*pVencLoopExit) |= (1 << chn);
}

static void COMMON_VENC_GetStreamBufInfo_Debug(VENC_CHN VeChn)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_STREAM_BUF_INFO_T StreamBufInfo;

    memset(&StreamBufInfo, 0x0, sizeof(AX_VENC_STREAM_BUF_INFO_T));
    s32Ret = AX_VENC_GetStreamBufInfo(VeChn, &StreamBufInfo);
    if (s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_GetStreamBufInfo failed, VeChn:%d", VeChn);
        return;
    }

    SAMPLE_LOG("chan-%d: u64PhyAddr:0x%llx, pUserAddr:%p, u32BufSize:0x%x\n", VeChn, StreamBufInfo.u64PhyAddr,
               StreamBufInfo.pUserAddr, StreamBufInfo.u32BufSize);

    return;
}

static void COMMON_VENC_QueryStatus_Debug(VENC_CHN VeChn)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_CHN_STATUS_T vencStatus;

    memset(&vencStatus, 0x0, sizeof(AX_VENC_CHN_STATUS_T));
    s32Ret = AX_VENC_QueryStatus(VeChn, &vencStatus);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_QueryStatus failed, VeChn:%d", VeChn);
    } else {
        if ((0 != vencStatus.u32LeftPics) || (0 != vencStatus.u32LeftStreamBytes) ||
            (0 != vencStatus.u32LeftStreamFrames)) {
            SAMPLE_LOG("chn-%d: get status leftPics %d, leftStreamBytes %d, leftStreamFrames %d\n", VeChn,
                       vencStatus.u32LeftPics, vencStatus.u32LeftStreamBytes, vencStatus.u32LeftStreamFrames);
        }
    }

    return;
}

AX_S32 COMMON_VENC_Start(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode,
                         SAMPLE_VENC_CMD_PARA_T *pstArg)
{
    AX_S32 s32Ret;
    AX_VENC_RECV_PIC_PARAM_T stRecvParam;

    s32Ret = COMMON_VENC_Create(VeChn, enType, rcMode, pstArg);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: COMM_VENC_Creat faild with%#x!.\n", VeChn, s32Ret);
        return -1;
    }

    stRecvParam.s32RecvPicNum = -1;
    s32Ret = AX_VENC_StartRecvFrame(VeChn, &stRecvParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_StartRecvFrame failed with%#x! \n", VeChn, s32Ret);
        return -1;
    }

    return AX_SUCCESS;
}

AX_S32 COMMON_VENC_Stop(VENC_CHN VeChn)
{
    AX_S32 s32Ret;

    s32Ret = AX_VENC_StopRecvFrame(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_StopRecvFrame failed with%#x! \n", VeChn, s32Ret);
        return -1;
    }

    s32Ret = AX_VENC_DestroyChn(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_DestroyChn failed with%#x! \n", VeChn, s32Ret);
        return -1;
    }

    return AX_SUCCESS;
}

AX_S32 COMMON_VENC_Create(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode,
                          SAMPLE_VENC_CMD_PARA_T *pstArg)
{
    AX_S32 s32Ret = -1;
    AX_S32 widthSrc;
    AX_S32 heightSrc;
    AX_S32 maxPicWidth;
    AX_S32 maxPicHeight;
    AX_U32 gopLen = 30;
    AX_U32 virILen;

    AX_U32 bitRate = 2000;  // kbps
    AX_U16 qpMin = 1;
    AX_U16 qpMax = 51;
    AX_U16 qpMinI = 1;
    AX_U16 qpMaxI = 51;
    AX_U16 qpStill = 0;
    AX_U16 u32IQp = 25;
    AX_U16 u32PQp = 30;
    AX_VENC_GOP_MODE_E gopType = AX_VENC_GOPMODE_NORMALP;
    AX_S32 intraQpDelta = 0;

    AX_S32 ctbRcMode;
    AX_S32 qpMapType;
    AX_S32 qpMapBlkType;
    AX_S32 qpMapBlockUnit;

    AX_VENC_CHN_ATTR_T stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    widthSrc = pstArg->picW;
    heightSrc = pstArg->picH;
    maxPicWidth = pstArg->maxPicW;
    maxPicHeight = pstArg->maxPicH;

    rcMode = pstArg->rcMode;

    gopLen = pstArg->gopLen;
    virILen = pstArg->virILen;
    bitRate = pstArg->bitRate;
    qpMin = pstArg->qpMin;
    qpMax = pstArg->qpMax;
    qpMinI = pstArg->qpMinI;
    qpMaxI = pstArg->qpMaxI;
    qpStill = pstArg->qpStill;
    gopType = pstArg->gopMode;
    intraQpDelta = pstArg->IQpDelta;
    qpMapType = pstArg->qpMapQpType;
    qpMapBlkType = pstArg->qpMapBlkType;
    qpMapBlockUnit = pstArg->qpMapBlkUnit;
    ctbRcMode = pstArg->ctbRcMode;
    u32IQp = pstArg->IQp;
    u32PQp = pstArg->PQp;

    stVencChnAttr.stVencAttr.enType = enType;
    stVencChnAttr.stVencAttr.u32PicWidthSrc = widthSrc;   /*the input picture width*/
    stVencChnAttr.stVencAttr.u32PicHeightSrc = heightSrc; /*the input picture height*/
    stVencChnAttr.stVencAttr.u32MaxPicWidth = maxPicWidth;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = maxPicHeight;
    stVencChnAttr.stVencAttr.enLinkMode = pstArg->bLinkMode ? AX_VENC_LINK_MODE : AX_VENC_UNLINK_MODE;
    stVencChnAttr.stVencAttr.u8InFifoDepth = pstArg->inFifoDep;
    stVencChnAttr.stVencAttr.u8OutFifoDepth = pstArg->outFifoDep;
    stVencChnAttr.stVencAttr.u32BufSize = pstArg->strmBufSize;

    stVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = pstArg->srcFrameRate;
    stVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = pstArg->dstFrameRate;

    /* crop setting */
    if (pstArg->bCrop) {
        stVencChnAttr.stVencAttr.stCropCfg.bEnable = AX_TRUE;
        stVencChnAttr.stVencAttr.stCropCfg.stRect.s32X = pstArg->cropX;
        stVencChnAttr.stVencAttr.stCropCfg.stRect.s32Y = pstArg->cropY;
        stVencChnAttr.stVencAttr.stCropCfg.stRect.u32Width = pstArg->cropW;
        stVencChnAttr.stVencAttr.stCropCfg.stRect.u32Height = pstArg->cropH;
    }

    if (pstArg->bCoreCoWork)
        stVencChnAttr.stVencAttr.flag |= AX_VENC_CHN_ENABLE_MULTICORE;

    if (pstArg->bStrmCached)
        stVencChnAttr.stVencAttr.flag |= AX_VENC_STREAM_CACHED;
    if (pstArg->bAttachHdr)
        stVencChnAttr.stVencAttr.flag |= AX_VENC_HEADER_ATTACH_TO_PB;

    switch (stVencChnAttr.stVencAttr.enType) {
    case PT_H265: {
        if (VENC_STREAM_10BIT == pstArg->strmBitDep) {
            stVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_10_PROFILE;
            stVencChnAttr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_10;
        } else {
            stVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_8;
        }
        stVencChnAttr.stVencAttr.enLevel = AX_VENC_HEVC_LEVEL_5_1;
        stVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;
        if (rcMode == SAMPLE_RC_CBR) {
            AX_VENC_H265_CBR_T stH265Cbr;
            memset(&stH265Cbr, 0, sizeof(AX_VENC_H265_CBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH265Cbr.u32Gop = gopLen;
            stH265Cbr.u32BitRate = bitRate;
            stH265Cbr.u32MinQp = qpMin;
            stH265Cbr.u32MaxQp = qpMax;
            stH265Cbr.u32MinIQp = qpMinI;
            stH265Cbr.u32MaxIQp = qpMaxI;
            stH265Cbr.s32IntraQpDelta = intraQpDelta;

            stH265Cbr.u32MaxIprop = pstArg->maxIprop;
            stH265Cbr.u32MinIprop = pstArg->minIprop;

            stH265Cbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH265Cbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH265Cbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH265Cbr.stQpmapInfo.enCtbRcMode = ctbRcMode;
            memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
        } else if (rcMode == SAMPLE_RC_VBR) {
            AX_VENC_H265_VBR_T stH265Vbr;
            memset(&stH265Vbr, 0, sizeof(AX_VENC_H265_VBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265VBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH265Vbr.u32Gop = gopLen;
            stH265Vbr.u32MaxBitRate = bitRate;
            stH265Vbr.u32MinQp = qpMin;
            stH265Vbr.u32MaxQp = qpMax;
            stH265Vbr.u32MinIQp = qpMinI;
            stH265Vbr.u32MaxIQp = qpMaxI;
            stH265Vbr.s32IntraQpDelta = intraQpDelta;
            stH265Vbr.u32ChangePos = pstArg->chgPos;

            stH265Vbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH265Vbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH265Vbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH265Vbr.stQpmapInfo.enCtbRcMode = ctbRcMode;
            memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
        } else if (rcMode == SAMPLE_RC_AVBR) {
            AX_VENC_H265_AVBR_T stH265AVbr;
            memset(&stH265AVbr, 0, sizeof(AX_VENC_H265_AVBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265AVBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH265AVbr.u32Gop = gopLen;
            stH265AVbr.u32MaxBitRate = bitRate;
            stH265AVbr.u32MinQp = qpMin;
            stH265AVbr.u32MaxQp = qpMax;
            stH265AVbr.u32MinIQp = qpMinI;
            stH265AVbr.u32MaxIQp = qpMaxI;
            stH265AVbr.s32IntraQpDelta = intraQpDelta;
            stH265AVbr.u32ChangePos = pstArg->chgPos;
            stH265AVbr.u32MinStillPercent = pstArg->stillPercent;
            stH265AVbr.u32MaxStillQp = qpStill;

            stH265AVbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH265AVbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH265AVbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH265AVbr.stQpmapInfo.enCtbRcMode = ctbRcMode;

            memcpy(&stVencChnAttr.stRcAttr.stH265AVbr, &stH265AVbr, sizeof(AX_VENC_H265_AVBR_T));
        } else if (rcMode == SAMPLE_RC_CVBR) {
            AX_VENC_H265_CVBR_T stH265CVbr;
            memset(&stH265CVbr, 0, sizeof(AX_VENC_H265_CVBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CVBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH265CVbr.u32Gop = gopLen;
            stH265CVbr.u32MaxBitRate = bitRate;

            stH265CVbr.u32MinQp = qpMin;
            stH265CVbr.u32MaxQp = qpMax;
            stH265CVbr.u32MinIQp = qpMinI;
            stH265CVbr.u32MaxIQp = qpMaxI;

            stH265CVbr.u32MaxIprop = pstArg->maxIprop;
            stH265CVbr.u32MinIprop = pstArg->minIprop;

            stH265CVbr.u32MinQpDelta = pstArg->minQpDelta;
            stH265CVbr.u32MaxQpDelta = pstArg->maxQpDelta;

            stH265CVbr.u32LongTermMinBitrate = pstArg->ltMinBt;
            stH265CVbr.u32LongTermMaxBitrate = pstArg->ltMaxBt;
            stH265CVbr.u32LongTermStatTime = pstArg->ltStaTime;
            stH265CVbr.u32ShortTermStatTime = pstArg->shtStaTime;

            stH265CVbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH265CVbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH265CVbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH265CVbr.stQpmapInfo.enCtbRcMode = ctbRcMode;

            memcpy(&stVencChnAttr.stRcAttr.stH265CVbr, &stH265CVbr, sizeof(AX_VENC_H265_CVBR_T));
        } else if (rcMode == SAMPLE_RC_FIXQP) {
            AX_VENC_H265_FIXQP_T stH265FixQp;
            memset(&stH265FixQp, 0, sizeof(AX_VENC_H265_FIXQP_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265FIXQP;
            stH265FixQp.u32Gop = gopLen;
            stH265FixQp.u32IQp = u32IQp;
            stH265FixQp.u32PQp = u32PQp;
            stH265FixQp.u32BQp = 32;
            memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
        } else if (rcMode == SAMPLE_RC_QPMAP) {
            AX_VENC_H265_QPMAP_T stH265QpMap;
            memset(&stH265QpMap, 0, sizeof(AX_VENC_H265_QPMAP_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265QPMAP;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;

            stH265QpMap.stQpmapInfo.enQpmapQpType = qpMapType;
            stH265QpMap.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH265QpMap.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH265QpMap.stQpmapInfo.enCtbRcMode = ctbRcMode;

            stH265QpMap.u32Gop = gopLen;
            stH265QpMap.u32TargetBitRate = bitRate;
            memcpy(&stVencChnAttr.stRcAttr.stH265QpMap, &stH265QpMap, sizeof(AX_VENC_H265_QPMAP_T));
        }
        break;
    }

    case PT_H264: {
        if (VENC_STREAM_10BIT == pstArg->strmBitDep) {
            stVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_HIGH_10_PROFILE;
            stVencChnAttr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_10;
        } else {
            stVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_8;
        }
        stVencChnAttr.stVencAttr.enLevel = AX_VENC_H264_LEVEL_5_1;
        if (rcMode == SAMPLE_RC_CBR) {
            AX_VENC_H264_CBR_T stH264Cbr;
            memset(&stH264Cbr, 0, sizeof(AX_VENC_H264_CBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH264Cbr.u32Gop = gopLen;
            stH264Cbr.u32BitRate = bitRate;
            stH264Cbr.u32MinQp = qpMin;
            stH264Cbr.u32MaxQp = qpMax;
            stH264Cbr.u32MinIQp = qpMinI;
            stH264Cbr.u32MaxIQp = qpMaxI;
            stH264Cbr.s32IntraQpDelta = intraQpDelta;

            stH264Cbr.u32MaxIprop = pstArg->maxIprop;
            stH264Cbr.u32MinIprop = pstArg->minIprop;

            stH264Cbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH264Cbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH264Cbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH264Cbr.stQpmapInfo.enCtbRcMode = ctbRcMode;

            memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
        } else if (rcMode == SAMPLE_RC_VBR) {
            AX_VENC_H264_VBR_T stH264Vbr;
            memset(&stH264Vbr, 0, sizeof(AX_VENC_H264_VBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264VBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH264Vbr.u32Gop = gopLen;
            stH264Vbr.u32MaxBitRate = bitRate;
            stH264Vbr.u32MinQp = qpMin;
            stH264Vbr.u32MaxQp = qpMax;
            stH264Vbr.u32MinIQp = qpMinI;
            stH264Vbr.u32MaxIQp = qpMaxI;
            stH264Vbr.s32IntraQpDelta = intraQpDelta;
            stH264Vbr.u32ChangePos = pstArg->chgPos;

            stH264Vbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH264Vbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH264Vbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH264Vbr.stQpmapInfo.enCtbRcMode = ctbRcMode;

            memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));
        } else if (rcMode == SAMPLE_RC_AVBR) {
            AX_VENC_H264_AVBR_T stH264AVbr;
            memset(&stH264AVbr, 0, sizeof(AX_VENC_H264_AVBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264AVBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH264AVbr.u32Gop = gopLen;
            stH264AVbr.u32MaxBitRate = bitRate;
            stH264AVbr.u32MinQp = qpMin;
            stH264AVbr.u32MaxQp = qpMax;
            stH264AVbr.u32MinIQp = qpMinI;
            stH264AVbr.u32MaxIQp = qpMaxI;
            stH264AVbr.s32IntraQpDelta = intraQpDelta;
            stH264AVbr.u32ChangePos = pstArg->chgPos;
            stH264AVbr.u32MinStillPercent = pstArg->stillPercent;
            stH264AVbr.u32MaxStillQp = qpStill;

            stH264AVbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH264AVbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH264AVbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH264AVbr.stQpmapInfo.enCtbRcMode = ctbRcMode;

            memcpy(&stVencChnAttr.stRcAttr.stH264AVbr, &stH264AVbr, sizeof(AX_VENC_H264_AVBR_T));
        } else if (rcMode == SAMPLE_RC_CVBR) {
            AX_VENC_H264_CVBR_T stH264CVbr;
            memset(&stH264CVbr, 0, sizeof(AX_VENC_H264_CVBR_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CVBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;
            stH264CVbr.u32Gop = gopLen;
            stH264CVbr.u32MaxBitRate = bitRate;

            stH264CVbr.u32MinQp = qpMin;
            stH264CVbr.u32MaxQp = qpMax;
            stH264CVbr.u32MinIQp = qpMinI;
            stH264CVbr.u32MaxIQp = qpMaxI;

            stH264CVbr.u32MaxIprop = pstArg->maxIprop;
            stH264CVbr.u32MinIprop = pstArg->minIprop;

            stH264CVbr.u32MinQpDelta = pstArg->minQpDelta;
            stH264CVbr.u32MaxQpDelta = pstArg->maxQpDelta;

            stH264CVbr.u32LongTermMinBitrate = pstArg->ltMinBt;
            stH264CVbr.u32LongTermMaxBitrate = pstArg->ltMaxBt;
            stH264CVbr.u32LongTermStatTime = pstArg->ltStaTime;
            stH264CVbr.u32ShortTermStatTime = pstArg->shtStaTime;

            stH264CVbr.stQpmapInfo.enQpmapQpType = qpMapType;
            stH264CVbr.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH264CVbr.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH264CVbr.stQpmapInfo.enCtbRcMode = ctbRcMode;

            memcpy(&stVencChnAttr.stRcAttr.stH264CVbr, &stH264CVbr, sizeof(AX_VENC_H264_CVBR_T));
        } else if (rcMode == SAMPLE_RC_FIXQP) {
            AX_VENC_H264_FIXQP_T stH264FixQp;
            memset(&stH264FixQp, 0, sizeof(AX_VENC_H264_FIXQP_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264FIXQP;
            stH264FixQp.u32Gop = gopLen;
            stH264FixQp.u32IQp = u32IQp;
            stH264FixQp.u32PQp = u32PQp;
            stH264FixQp.u32BQp = 32;
            memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
        } else if (rcMode == SAMPLE_RC_QPMAP) {
            AX_VENC_H264_QPMAP_T stH264QpMap;
            memset(&stH264QpMap, 0, sizeof(AX_VENC_H264_QPMAP_T));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264QPMAP;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = pstArg->startQp;

            stH264QpMap.stQpmapInfo.enQpmapQpType = qpMapType;
            stH264QpMap.stQpmapInfo.enQpmapBlockUnit = qpMapBlockUnit;
            stH264QpMap.stQpmapInfo.enQpmapBlockType = qpMapBlkType;
            stH264QpMap.stQpmapInfo.enCtbRcMode = ctbRcMode;

            stH264QpMap.u32Gop = gopLen;
            stH264QpMap.u32TargetBitRate = bitRate;
            memcpy(&stVencChnAttr.stRcAttr.stH264QpMap, &stH264QpMap, sizeof(AX_VENC_H264_QPMAP_T));
        }
        break;
    }

    case PT_JPEG: {
        break;
    }

    case PT_MJPEG: {
        if (rcMode == SAMPLE_RC_CBR) {
            AX_VENC_MJPEG_CBR_T stMjpegCbr;
            memset(&stMjpegCbr, 0, sizeof(stMjpegCbr));

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;
            stMjpegCbr.u32StatTime = 1;
            stMjpegCbr.u32BitRate = bitRate;
            stMjpegCbr.u32MinQp = qpMin;
            stMjpegCbr.u32MaxQp = qpMax;
            memcpy(&stVencChnAttr.stRcAttr.stMjpegCbr, &stMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
        } else if (rcMode == SAMPLE_RC_VBR) {
            AX_VENC_MJPEG_VBR_T stMjpegVbr;
            memset(&stMjpegVbr, 0, sizeof(stMjpegVbr));

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;
            stMjpegVbr.u32StatTime = 1;
            stMjpegVbr.u32MaxBitRate = bitRate;
            stMjpegVbr.u32MinQp = qpMin;
            stMjpegVbr.u32MaxQp = qpMax;
            memcpy(&stVencChnAttr.stRcAttr.stMjpegVbr, &stMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
        } else if (rcMode == SAMPLE_RC_FIXQP) {
            AX_VENC_MJPEG_FIXQP_T stMjpegFixQp;

            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
            stMjpegFixQp.s32FixedQp = 26;
            memcpy(&stVencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
        }
        break;
    }
    default:
        SAMPLE_LOG_ERR("chn-%d: Invalid Codec Format.\n", VeChn);
        s32Ret = -1;
        goto END;
    }

    /* GOP table setting */
    switch (gopType) {
    case AX_VENC_GOPMODE_NORMALP: {
        stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;
        break;
    }
    case AX_VENC_GOPMODE_ONELTR: {
        /* Normal frame configures */
        stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_ONELTR;
        stVencChnAttr.stGopAttr.stOneLTR.stPicConfig.s32QpOffset = 0;
        stVencChnAttr.stGopAttr.stOneLTR.stPicConfig.f32QpFactor = 0.4624;
        /* long-term reference and special frame configure */
        stVencChnAttr.stGopAttr.stOneLTR.stPicSpecialConfig.s32Interval = virILen;
        stVencChnAttr.stGopAttr.stOneLTR.stPicSpecialConfig.s32QpOffset = -2;
        stVencChnAttr.stGopAttr.stOneLTR.stPicSpecialConfig.f32QpFactor = 0.4624;
        break;
    }
    case AX_VENC_GOPMODE_SVC_T: {
        /* SVC-T Configure */
        static AX_U32 sSvcTGopSize = 4;
        /*SVC-T GOP4*/
        static AX_CHAR *stSvcTCfg[] = {
            "Frame1:  P      1      0       0.4624        2        1           -1          1",
            "Frame2:  P      2      0       0.4624        1        1           -2          1",
            "Frame3:  P      3      0       0.4624        2        2           -1 -3       1 0",
            "Frame4:  P      4      0       0.4624        0        1           -4          1",
            NULL,
        };
        stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_SVC_T;
        stVencChnAttr.stGopAttr.stSvcT.u32GopSize = sSvcTGopSize;
        stVencChnAttr.stGopAttr.stSvcT.s8SvcTCfg = stSvcTCfg;
        break;
    }

    default:
        SAMPLE_LOG_ERR("chn-%d: Invalid gop type(%d).\n", VeChn, gopType);
        goto END;
    }

    /* create channel */
    s32Ret = AX_VENC_CreateChn(VeChn, &stVencChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_CreateChn failed, ret=0x%x\n", VeChn, s32Ret);
        goto END;
    }

    return AX_SUCCESS;

END:

    return s32Ret;
}

AX_VOID *COMMON_VENC_SendFrameProc(AX_VOID *arg)
{
    AX_BOOL bLoopEncode;
    AX_U32 encFrmNum;
    const AX_CHAR *fileInput;
    AX_S32 syncType;
    AX_IMG_FORMAT_E eFmt;
    AX_U32 width;
    AX_U32 height;
    AX_U32 strideY, strideU, strideV;
    AX_VIDEO_FRAME_INFO_T stFrame;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg;
    AX_S32 readSize;
    AX_S32 s32Ret;

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)arg;

    VENC_CHN VeChn = pstArg->VeChn;
    bLoopEncode = pstArg->bLoopEncode;
    encFrmNum = pstArg->encFrmNum;
    syncType = pstArg->syncType;
    eFmt = pstArg->eFmt;
    width = pstArg->width;
    height = pstArg->height;
    strideY = pstArg->strideY;
    strideU = pstArg->strideU;
    strideV = pstArg->strideV;
    fileInput = pstArg->fileInput;
    AX_S32 frameSize = pstArg->frameSize;
    AX_S32 blkSize = pstArg->blkSize;
    AX_S32 poolId = pstArg->poolId;
    FILE *fFileIn = NULL;
    AX_BLK blkId = AX_INVALID_BLOCKID;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    memset(&stFrame, 0, sizeof(stFrame));

    fFileIn = fopen(fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        return NULL;
    }

    while (pstArg->bSendFrmStart) {
        /* get block from user pool */
        blkId = AX_POOL_GetBlock(poolId, blkSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            usleep(5000);
            continue;
        }

        stFrame.stVFrame.u32FrameSize = frameSize;
        stFrame.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        stFrame.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        stFrame.stVFrame.u32BlkId[0] = blkId;
        stFrame.stVFrame.u32BlkId[1] = 0;
        stFrame.stVFrame.u32BlkId[2] = 0;
        stFrame.stVFrame.enImgFormat = eFmt;

        if (pstArg->stFbcInfo.fbcType) {
            readSize = COMMON_VENC_ReadFbcFile(fFileIn, strideY, height, &pstArg->stFbcInfo, &stFrame.stVFrame);
        } else {
            /* read frame data from yuv file */
            readSize =
                COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);
        }

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, frameSize);

        if (feof(fFileIn)) {
            if (bLoopEncode) {
                fseek(fFileIn, 0, SEEK_SET);
                if (pstArg->stFbcInfo.fbcType) {
                    readSize = COMMON_VENC_ReadFbcFile(fFileIn, strideY, height, &pstArg->stFbcInfo, &stFrame.stVFrame);
                } else {
                    /* read frame data from yuv file */
                    readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt,
                                                    (void *)stFrame.stVFrame.u64VirAddr[0]);
                }
            } else {
                SAMPLE_LOG_WARN("chn-%d: End of input file!\n", VeChn);
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        stFrame.stVFrame.u64SeqNum = pstArg->totalSendFrame + 1;
        stFrame.stVFrame.enImgFormat = eFmt;
        stFrame.stVFrame.u32Width = width;
        stFrame.stVFrame.u32Height = height;
        stFrame.stVFrame.u32PicStride[0] = strideY;
        stFrame.stVFrame.u32PicStride[1] = strideU;
        stFrame.stVFrame.u32PicStride[2] = strideV;

        s32Ret = AX_VENC_SendFrame(VeChn, &stFrame, syncType);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_WARN("chn-%d: AX_VENC_SendFrame failed, ret=%x\n", VeChn, s32Ret);

        pstArg->totalSendFrame++;

        if (pstArg->totalSendFrame == encFrmNum) {
            SAMPLE_LOG("chn-%d: Want to encode %llu frames, exit!\n", VeChn, pstArg->totalSendFrame);
            goto EXIT;
        }

        if (AX_INVALID_BLOCKID != blkId)
            AX_POOL_ReleaseBlock(blkId);
    }


EXIT:

    if (NULL != fFileIn) {
        fclose(fFileIn);
        fFileIn = NULL;
    }

    if (AX_INVALID_BLOCKID != blkId)
        AX_POOL_ReleaseBlock(blkId);

    SAMPLE_LOG("chn-%d - Total input %llu frames, Encoder exit!\n", VeChn, pstArg->totalSendFrame);

    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, VeChn);

    return NULL;
}

AX_S32 COMMON_VENC_StartSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = pthread_create(&pstArg->sendFrmPid, 0, COMMON_VENC_SendFrameProc, (AX_VOID *)pstArg);

    return s32Ret;
}

AX_S32 COMMON_VENC_StopSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg)
{
    if (pstArg->bSendFrmStart) {
        pstArg->bSendFrmStart = AX_FALSE;
        pthread_join(pstArg->sendFrmPid, 0);
    }

    return AX_SUCCESS;
}

AX_S32 COMMON_VENC_StartGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = pthread_create(&pstArg->getStrmPid, 0, COMMON_VENC_GetStreamProc, (AX_VOID *)pstArg);

    return s32Ret;
}

AX_VOID *COMMON_VENC_GetStreamProc(AX_VOID *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_STREAM_T stStream;
    FILE *pStrm = NULL;
    SAMPLE_VENC_GETSTREAM_PARA_T *pstArg = (SAMPLE_VENC_GETSTREAM_PARA_T *)arg;
    VENC_CHN VeChn = pstArg->VeChn;
    AX_S32 testId = pstArg->testId;
    AX_CHAR esName[50];
    AX_PAYLOAD_TYPE_E enType = pstArg->enType;

    memset(&stStream, 0, sizeof(stStream));
    memset(esName, 0, 50);

    if (NULL == pstArg->output) {
        if (PT_H264 == enType)
            sprintf(esName, "es_chn%d_ut%d_%s.264", VeChn, testId, pstArg->strmSuffix);
        else if (PT_H265 == enType)
            sprintf(esName, "es_chn%d_ut%d_%s.265", VeChn, testId, pstArg->strmSuffix);
        else if (PT_MJPEG == enType)
            sprintf(esName, "es_chn%d_ut%d_%s.mjpg", VeChn, testId, pstArg->strmSuffix);
        else if (PT_JPEG == enType)
            sprintf(esName, "es_chn%d_ut%d.jpg", VeChn, testId);

        pStrm = fopen(esName, "wb");
    } else {
        pStrm = fopen(pstArg->output, "wb");
    }

    if (NULL == pStrm) {
        SAMPLE_LOG_ERR("chn-%d: Open output file error!\n", VeChn);
        return NULL;
    }

    while (pstArg->bGetStrmStart) {
        if (pstArg->bQueryStatus)
            COMMON_VENC_QueryStatus_Debug(VeChn);
        s32Ret = AX_VENC_GetStream(VeChn, &stStream, pstArg->syncType);
        if (AX_SUCCESS == s32Ret) {
            s32Ret = COMMON_VENC_WriteStream(pstArg, AX_TRUE, pstArg->totalGetStream, pStrm, &stStream);
            if (AX_SUCCESS != s32Ret)
                SAMPLE_LOG_ERR("COMMON_VENC_WriteStream err.\n");

            if (pstArg->bGetStrmBufInfo)
                COMMON_VENC_GetStreamBufInfo_Debug(VeChn);

            pstArg->totalGetStream++;

            SAMPLE_LOG_DEBUG("chn-%d: get stream success, addr=%p, len=%u, codeType=%d. seqNum %lld pts %lld\n", VeChn,
                             stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.enCodingType,
                             stStream.stPack.u64SeqNum, stStream.stPack.u64PTS);

            s32Ret = AX_VENC_ReleaseStream(VeChn, &stStream);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("chn-%d: AX_VENC_ReleaseStream failed, ret=0x%x\n", VeChn, s32Ret);
                goto EXIT;
            }
        }
    }

EXIT:
    if (pStrm != NULL) {
        fclose(pStrm);
        pStrm = NULL;
    }

    SAMPLE_LOG("chn-%d: Total get %llu encoded frames. getStream Exit!\n", VeChn, pstArg->totalGetStream);
    return (void *)(intptr_t)s32Ret;
}

AX_S32 COMMON_VENC_StopGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    if (pstArg->bGetStrmStart) {
        pstArg->bGetStrmStart = AX_FALSE;
        pthread_join(pstArg->getStrmPid, 0);
    }

    return AX_SUCCESS;
}

AX_S32 COMMON_VENC_ReadFile(FILE *pFileIn, AX_S32 widthSrc, AX_S32 strideSrc, AX_S32 heightSrc, AX_IMG_FORMAT_E eFmt,
                            AX_VOID *pVaddr)
{
    AX_U32 i, rows, realRead, readSize;

    if (!pFileIn)
        return -1;
    if (!pVaddr)
        return -1;

    readSize = 0;

    switch (eFmt) {
    case AX_FORMAT_YUV420_PLANAR:
        rows = heightSrc;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc, pFileIn);
            if (realRead < strideSrc)
                break;
            readSize += realRead;
            pVaddr += strideSrc;
        }
        rows = heightSrc;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc >> 1, pFileIn);
            if (realRead < (strideSrc >> 1))
                break;
            readSize += realRead;
            pVaddr += (strideSrc >> 1);
        }
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        rows = heightSrc * 3 / 2;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc, pFileIn);
            if (realRead < strideSrc)
                break;
            readSize += realRead;
            pVaddr += strideSrc;
        }
        break;
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        rows = heightSrc;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc, pFileIn);
            if (realRead < strideSrc)
                break;
            readSize += realRead;
            pVaddr += strideSrc;
        }
        break;
    case AX_FORMAT_YUV420_PLANAR_10BIT_I010:
        rows = heightSrc;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc, pFileIn);
            if (realRead < strideSrc)
                break;
            readSize += realRead;
            pVaddr += strideSrc;
        }
        rows = heightSrc;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc >> 1, pFileIn);
            if (realRead < (strideSrc >> 1))
                break;
            readSize += realRead;
            pVaddr += (strideSrc >> 1);
        }
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010:
        rows = heightSrc * 3 / 2;
        for (i = 0; i < rows; i++) {
            realRead = fread(pVaddr, 1, strideSrc, pFileIn);
            if (realRead < strideSrc)
                break;
            readSize += realRead;
            pVaddr += strideSrc;
        }
        break;
    default:
        SAMPLE_LOG_WARN("Invalid format, eFmt = %d\n", eFmt);
    }

    return readSize;
}

AX_S32 COMMON_VENC_ReadFbcFile(FILE *pFileIn, AX_S32 strideY, AX_S32 height, SAMPLE_VENC_FBC_INFO_T *pFbc,
                               AX_VIDEO_FRAME_T *pstFrame)
{
    AX_S32 lumaData = 0, chromaData = 0;
    AX_S32 readSize = 0;

    if (VENC_FBC_LOSSLESS == pFbc->fbcType) {
        lumaData = pFbc->yHdrSize + pFbc->yPadSize;
        chromaData = pFbc->uvHdrSize + pFbc->uvPadSize;

        readSize = fread((AX_VOID *)pstFrame->u64VirAddr[0], 1, lumaData, pFileIn);
        if (readSize < lumaData)
            return -1;

        pstFrame->u64PhyAddr[1] = pstFrame->u64PhyAddr[0] + lumaData;
        pstFrame->u64VirAddr[1] = pstFrame->u64VirAddr[0] + lumaData;

        readSize += fread((AX_VOID *)pstFrame->u64VirAddr[1], 1, chromaData, pFileIn);
        if (readSize < chromaData)
            return -1;

        pstFrame->stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSLESS;
        pstFrame->u32HeaderSize[0] = pFbc->yHdrSize;
        pstFrame->u32HeaderSize[1] = pFbc->uvHdrSize;

    } else if (VENC_FBC_LOSSY == pFbc->fbcType) {
        pstFrame->stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
        pstFrame->stCompressInfo.u32CompressLevel = pFbc->compLevel;

        lumaData = (TILE_ALIGN(strideY, 128) / 128) * (TILE_ALIGN(height, 2) / 2) * gTileSizeTable[pFbc->compLevel];

        chromaData = lumaData / 2;
        SAMPLE_LOG("Luma size=%d, chroma size=%d.\n", lumaData, chromaData);

        readSize = fread((AX_VOID *)pstFrame->u64VirAddr[0], 1, lumaData + chromaData, pFileIn);
        if (readSize < (lumaData + chromaData))
            return -1;

        pstFrame->u64PhyAddr[1] = pstFrame->u64PhyAddr[0] + lumaData;
    }

    pstFrame->u32FrameSize = readSize;

    return readSize;
}

AX_S32 COMMON_VENC_SaveJpegFile(VENC_CHN VeChn, AX_S32 testId, AX_U64 totalStream, AX_U8 *pu8Addr, AX_S32 u32Len,
                                AX_BOOL bSaveFile)
{
    if (!bSaveFile)
        return 0;

    AX_CHAR esName[MAX_DIE_FILE_SIZE];
    AX_CHAR dirName[MAX_DIE_FILE_SIZE];
    AX_CHAR fileName[2 * MAX_DIE_FILE_SIZE];
    FILE *pStrm = NULL;

    if (NULL == pu8Addr) {
        SAMPLE_LOG_ERR("chn-%d: null pointer!\n", VeChn);
        return -1;
    }

    memset(esName, 0, MAX_DIE_FILE_SIZE);
    memset(dirName, 0, MAX_DIE_FILE_SIZE);
    memset(fileName, 0, 2 * MAX_DIE_FILE_SIZE);


    sprintf(dirName, "./ut_jpeg/");
    sprintf(esName, "es_chn%d_ut%d_cnt%lld.jpg", VeChn, testId, totalStream);
    sprintf(fileName, "%s%s", dirName, esName);

    if (access(dirName, 0) != 0) {
        if (mkdir(dirName, 0777)) {
            SAMPLE_LOG_ERR("mkdir %s dirName failed!\n", dirName);
            return -1;
        }
    }

    pStrm = fopen(fileName, "wb");
    if (NULL == pStrm) {
        SAMPLE_LOG_ERR("chn-%d: Open output file error!\n", VeChn);
        return -1;
    }

    fwrite(pu8Addr, 1, u32Len, pStrm);
    fflush(pStrm);

    if (pStrm != NULL) {
        fclose(pStrm);
        pStrm = NULL;
    }

    return AX_SUCCESS;
}

AX_S32 COMMON_VENC_WriteStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg, AX_BOOL bCommon, AX_U64 totalStream, FILE *pFile,
                               AX_VENC_STREAM_T *pstStream)
{
    AX_PAYLOAD_TYPE_E enType = PT_BUTT;
    VENC_CHN VeChn = 0;
    AX_S32 testId = 0;
    AX_S32 s32Ret = -1;
    AX_BOOL bSaveStrm;

    if ((NULL == pstArg) || (NULL == pstStream)) {
        SAMPLE_LOG_ERR("chn-%d: null pointer!\n", VeChn);
        return -1;
    }

    bSaveStrm = pstArg->bSaveStrm;
    if (!bSaveStrm)
        return 0;

    enType = pstArg->enType;
    VeChn = pstArg->VeChn;
    testId = pstArg->testId;

    if (bSaveStrm) {
        if (bCommon && pFile) {
            if (pstArg->gopMode != AX_VENC_GOPMODE_SVC_T) {
                fwrite(pstStream->stPack.pu8Addr, 1, pstStream->stPack.u32Len, pFile);
            } else {
                if (((0 == pstArg->temporalID) && (0 == pstStream->stPack.u32TemporalID)) ||
                    ((1 == pstArg->temporalID) &&
                     ((0 == pstStream->stPack.u32TemporalID) || (1 == pstStream->stPack.u32TemporalID))) ||
                    ((2 == pstArg->temporalID) &&
                     ((0 == pstStream->stPack.u32TemporalID) || (1 == pstStream->stPack.u32TemporalID) ||
                      (2 == pstStream->stPack.u32TemporalID))))
                    fwrite(pstStream->stPack.pu8Addr, 1, pstStream->stPack.u32Len, pFile);
            }
            fflush(pFile);
        } else {
            if (PT_JPEG == enType) {
                s32Ret = COMMON_VENC_SaveJpegFile(VeChn, testId, totalStream, pstStream->stPack.pu8Addr,
                                                  pstStream->stPack.u32Len, bSaveStrm);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_LOG_ERR("COMMON_VENC_SaveJpegFile err.\n");
                    return -1;
                }
            } else if (pFile) {
                fwrite(pstStream->stPack.pu8Addr, 1, pstStream->stPack.u32Len, pFile);
                fflush(pFile);
            }
        }
    }

    return 0;
}

AX_VOID COMMON_VENC_SendGetDelay(AX_S32 chn, AX_BOOL bExit, SAMPLE_VENC_SENDFRAME_PARA_T *pSend,
                                 SAMPLE_VENC_GETSTREAM_PARA_T *pGet)
{
    AX_S32 retryNum = 5;

    if (bExit)
        return;

    while (retryNum--) {
        if (pGet->totalGetStream >= pSend->totalSendFrame)
            break;

        SAMPLE_LOG("chn-%d: send=%llu, get=%llu, retry...\n", chn, pSend->totalSendFrame, pGet->totalGetStream);
        sleep(1);
    }

    return;
}