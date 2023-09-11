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
#include <sys/prctl.h>

#include "common_vdec_api.h"


AX_S32 SampleVdecChnAttrSet(AX_VDEC_GRP VdGrp, AX_VDEC_CHN VdChn,
                            SAMPLE_VDEC_CMD_PARAM_T *pstCmd, AX_VDEC_CHN_ATTR_T *pstChnAttr)
{
    AX_S32 sRet = AX_SUCCESS;
    AX_VDEC_CHN_ATTR_T *pstVdChnAttr = NULL;

    if (pstCmd == NULL) {
        sRet = AX_ERR_VDEC_NULL_PTR;
        SAMPLE_CRIT_LOG("pstCmd == NULL");
        goto ERR_RET;
    }

    if (pstChnAttr == NULL) {
        sRet = AX_ERR_VDEC_NULL_PTR;
        SAMPLE_CRIT_LOG("pstChnAttr == NULL");
        goto ERR_RET;
    }

    if (VdGrp >= AX_VDEC_MAX_GRP_NUM) {
        sRet = AX_ERR_VDEC_INVALID_GRPID;
        SAMPLE_CRIT_LOG("VdGrp:%d >= AX_VDEC_MAX_GRP_NUM:%d", VdGrp, AX_VDEC_MAX_GRP_NUM);
        goto ERR_RET;
    }

    if (VdChn >= AX_DEC_MAX_CHN_NUM) {
        sRet = AX_ERR_VDEC_INVALID_CHNID;
        SAMPLE_CRIT_LOG("VdChn:%d >= AX_DEC_MAX_CHN_NUM:%d", VdChn, AX_DEC_MAX_CHN_NUM);
        goto ERR_RET;
    }

    pstVdChnAttr = pstChnAttr;

    pstVdChnAttr->enImgFormat = pstCmd->tChnCfg[VdChn].enImgFormat;
    pstVdChnAttr->enOutputMode = pstCmd->tChnCfg[VdChn].enOutputMode;
    pstVdChnAttr->u32PicWidth = pstCmd->tChnCfg[VdChn].u32PicWidth;
    pstVdChnAttr->u32PicHeight = pstCmd->tChnCfg[VdChn].u32PicHeight;

    pstVdChnAttr->u32FrameStride = pstCmd->tChnCfg[VdChn].u32FrameStride;
    pstVdChnAttr->u32FramePadding = pstCmd->tChnCfg[VdChn].u32FramePadding;
    pstVdChnAttr->u32CropX = pstCmd->tChnCfg[VdChn].u32CropX;
    pstVdChnAttr->u32CropY = pstCmd->tChnCfg[VdChn].u32CropY;

    pstVdChnAttr->u32ScaleRatioX = pstCmd->tChnCfg[VdChn].u32ScaleRatioX;
    pstVdChnAttr->u32ScaleRatioY = pstCmd->tChnCfg[VdChn].u32ScaleRatioY;

    pstVdChnAttr->u32OutputFifoDepth = pstCmd->tChnCfg[VdChn].u32OutputFifoDepth;
    pstVdChnAttr->enOutputMode = pstCmd->tChnCfg[VdChn].enOutputMode;

    pstVdChnAttr->enImgFormat = pstCmd->tChnCfg[VdChn].enImgFormat;

    if (pstCmd->tChnCfg[VdChn].uCompressLevel) {
        pstVdChnAttr->stCompressInfo.u32CompressLevel = pstCmd->tChnCfg[VdChn].uCompressLevel;
        pstVdChnAttr->stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
    }

    AX_U32 uWidth = pstVdChnAttr->u32PicWidth;
    AX_U32 uPixBits = 8;

    if (pstCmd->tChnCfg[VdChn].enImgFormat == AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010) {
        uPixBits = 10;
        if (pstVdChnAttr->stCompressInfo.enCompressMode == AX_COMPRESS_MODE_LOSSY)
            pstVdChnAttr->u32FrameStride = AX_COMM_ALIGN(AX_COMM_ALIGN(uWidth, 128) * uPixBits, AX_VDEC_WIDTH_ALIGN * 8) / 8;
        else
            pstVdChnAttr->u32FrameStride = AX_COMM_ALIGN(uWidth * 2, AX_VDEC_WIDTH_ALIGN);
    } else if (pstCmd->tChnCfg[VdChn].enImgFormat == AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P101010) {
        uPixBits = 10;
        if (pstVdChnAttr->stCompressInfo.enCompressMode == AX_COMPRESS_MODE_LOSSY)
            pstVdChnAttr->u32FrameStride = AX_COMM_ALIGN(AX_COMM_ALIGN(uWidth, 128) * uPixBits, AX_VDEC_WIDTH_ALIGN * 8) / 8;
        else
            pstVdChnAttr->u32FrameStride = AX_COMM_ALIGN(uWidth * 10 / 8, AX_VDEC_WIDTH_ALIGN);
    } else {
        uPixBits = 8;
        if (pstVdChnAttr->u32FrameStride == 0) {
            if (pstCmd->enDecType == PT_H264 || pstCmd->enDecType == PT_H265) {
                pstVdChnAttr->u32FrameStride = AX_COMM_ALIGN(uWidth * uPixBits, AX_VDEC_WIDTH_ALIGN * 8) / 8;
            } else {
                pstVdChnAttr->u32FrameStride = AX_COMM_ALIGN(uWidth * uPixBits, AX_JDEC_WIDTH_ALIGN * 8) / 8;
            }
        }
    }

    pstVdChnAttr->u32FramePadding = pstCmd->tChnCfg[VdChn].u32FramePadding;
    pstVdChnAttr->stOutputFrmRate.f32DstFrmRate = pstCmd->tChnCfg[VdChn].f32DstFrmRate;
    pstVdChnAttr->stOutputFrmRate.bFrmRateCtrl = pstCmd->tChnCfg[VdChn].bFrmRateCtrl;

    if (pstCmd->enFrameBufSrc == POOL_SOURCE_PRIVATE) {
        pstVdChnAttr->u32FrameBufSize = AX_VDEC_GetPicBufferSize(pstVdChnAttr->u32FrameStride,
                                                                pstVdChnAttr->u32PicHeight,
                                                                pstVdChnAttr->enImgFormat,
                                                                &pstVdChnAttr->stCompressInfo,
                                                                pstCmd->enDecType);
        pstVdChnAttr->u32FrameBufCnt = pstCmd->tChnCfg[VdChn].u32FrameBufCnt;
    }

    SAMPLE_LOG("VdGrp=%d, VdChn=%d, u32PicWidth=%d, u32PicHeight=%d, "
                "u32FrameStride=0x%x, u32FramePadding=0x%x, enFrameBufSrc=0x%x,"
                "u32FrameBufSize=%d, u32FrameBufCnt=%d",
                VdGrp, VdChn,
                pstVdChnAttr->u32PicWidth,
                pstVdChnAttr->u32PicHeight,
                pstVdChnAttr->u32FrameStride,
                pstVdChnAttr->u32FramePadding,
                pstCmd->enFrameBufSrc,
                pstVdChnAttr->u32FrameBufSize,
                pstVdChnAttr->u32FrameBufCnt);

    SAMPLE_LOG("VdGrp=%d, VdChn=%d, u32OutputFifoDepth=%d",
                VdGrp, VdChn, pstVdChnAttr->u32OutputFifoDepth);

    sRet = AX_VDEC_SetChnAttr(VdGrp, VdChn, pstVdChnAttr);
    if (sRet != AX_SUCCESS) {
        SAMPLE_CRIT_LOG("VdGrp=%d, AX_VDEC_SetChnAttr FAILED! ret:0x%x %s\n",
                        VdGrp, sRet, AX_VdecRetStr(sRet));
        goto ERR_RET;
    }


ERR_RET:
    return sRet;
}

AX_S32 SampleVdecDestroyUserPool(AX_VDEC_GRP VdGrp, AX_VDEC_CHN VdChn, AX_POOL PoolId)
{
    AX_S32 sRet = AX_SUCCESS;

    if (VdGrp >= AX_VDEC_MAX_GRP_NUM) {
        sRet = AX_ERR_VDEC_INVALID_GRPID;
        SAMPLE_CRIT_LOG("VdGrp:%d >= AX_VDEC_MAX_GRP_NUM:%d", VdGrp, AX_VDEC_MAX_GRP_NUM);
        goto ERR_RET;
    }

    if (VdChn >= AX_DEC_MAX_CHN_NUM) {
        sRet = AX_ERR_VDEC_INVALID_CHNID;
        SAMPLE_CRIT_LOG("VdChn:%d >= AX_DEC_MAX_CHN_NUM:%d", VdChn, AX_DEC_MAX_CHN_NUM);
        goto ERR_RET;
    }

    if (PoolId == AX_INVALID_POOLID) {
        sRet = AX_ERR_VDEC_ILLEGAL_PARAM;
        SAMPLE_CRIT_LOG("PoolId == AX_INVALID_POOLID");
        goto ERR_RET;
    }

    sRet = AX_VDEC_DetachPool(VdGrp, VdChn);
    if (sRet) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId:%d, AX_VDEC_DetachPool FAILED! ret:0x%x %s",
                        VdGrp, VdChn, PoolId, sRet, AX_VdecRetStr(sRet));
        goto ERR_RET;
    }

    sRet = AX_POOL_DestroyPool(PoolId);
    if (sRet) {
        SAMPLE_CRIT_LOG("VdGrp=%d, VdChn=%d, PoolId:%d, AX_POOL_DestroyPool FAILED! ret:0x%x %s",
                        VdGrp, VdChn, PoolId, sRet, AX_VdecRetStr(sRet));
        goto ERR_RET;
    }


ERR_RET:
    return sRet;
}

AX_VOID SampleVdecFeatureTest(AX_VDEC_GRP VdGrp, SAMPLE_VDEC_CMD_PARAM_T *pstCmd)
{
    AX_S32 ret = 0;
    AX_S32 i = 0;
    AX_U8* pu8Addr = NULL;

    if (!pstCmd)
        return;

    if (pstCmd->bGetUserData) {
        AX_VDEC_USERDATA_T stUserData;
        AX_U32 outInfoLen = 32;
        AX_U32 tmpLen = 0;
        memset(&stUserData, 0x0, sizeof(AX_VDEC_USERDATA_T));
        /* AX_VDEC_GetUserData can be called anywhere, and is used here
         * only for demonstration and testing purposes.
         */
        ret = AX_VDEC_GetUserData(VdGrp, &stUserData);
        if (AX_SUCCESS == ret) {
            if (stUserData.bValid && stUserData.pu8Addr) {
                SAMPLE_LOG_T("================= VdGrp=%d, AX_VDEC_GetUserData u32UserDataCnt:%d, u32Len:%d",
                             VdGrp, stUserData.u32UserDataCnt, stUserData.u32Len);
                pu8Addr = stUserData.pu8Addr;
                for (i = 0; i < stUserData.u32UserDataCnt; i++) {
                    tmpLen = stUserData.u32DataLen[i] > outInfoLen ? outInfoLen : stUserData.u32DataLen[i];
                    pu8Addr[tmpLen] = '\0';
                    SAMPLE_LOG_T("SEI%d pu8Addr:%p, u32DataLen:%d, data: %s",
                                i, pu8Addr, stUserData.u32DataLen[i], pu8Addr);
                    pu8Addr += stUserData.u32DataLen[i];
                }
            }
        } else {
            SAMPLE_CRIT_LOG("VdGrp=%d,  AX_VDEC_GetUserData FAILED! ret:0x%x", VdGrp, ret);
        }
    }

    if (pstCmd->bGetRbInfo) {
        AX_VDEC_STREAM_BUF_INFO_T stStreamBufInfo;
        memset(&stStreamBufInfo, 0x0, sizeof(AX_VDEC_STREAM_BUF_INFO_T));
        ret = AX_VDEC_GetStreamBufInfo(VdGrp, &stStreamBufInfo);
        if (AX_SUCCESS == ret)
            SAMPLE_LOG_T("================= VdGrp=%d, AX_VDEC_GetStreamBufInfo: "
                         "phyStart: 0x%llx, virStart: %p, totalSize: 0x%x, readAbleSize: 0x%x, "
                         "writeAbleSize: 0x%x, readOffset: 0x%x, writeOffset: 0x%x",
                         VdGrp, stStreamBufInfo.phyStart,
                         stStreamBufInfo.virStart,
                         stStreamBufInfo.totalSize,
                         stStreamBufInfo.readAbleSize,
                         stStreamBufInfo.writeAbleSize,
                         stStreamBufInfo.readOffset,
                         stStreamBufInfo.writeOffset);
        else
            SAMPLE_CRIT_LOG("VdGrp=%d,  AX_VDEC_GetUserData FAILED! ret:0x%x", VdGrp, ret);
    }

    if (pstCmd->bQueryStatus) {
        AX_VDEC_GRP_STATUS_T stGrpStatus;
        memset(&stGrpStatus, 0x0, sizeof(AX_VDEC_GRP_STATUS_T));
        ret = AX_VDEC_QueryStatus(VdGrp, &stGrpStatus);
        if (AX_SUCCESS == ret)
            SAMPLE_LOG_T("================= VdGrp=%d, AX_VDEC_QueryStatus: \n"
                         "enCodecType: %d, u32LeftStreamBytes: %d, u32LeftStreamFrames: %d, "
                         "u32LeftPics: %d-%d-%d, bStartRecvStream: %d, u32RecvStreamFrames: %d, "
                         "u32DecodeStreamFrames: %d, u32PicWidth: %d, u32PicHeight: %d, "
                         "s32PicSizeErrSet: %d, s32StreamUnsprt: %d, s32PackErr: %d, "
                         "s32PicBufSizeErrSet: %d",
                         VdGrp, stGrpStatus.enCodecType, stGrpStatus.u32LeftStreamBytes,
                         stGrpStatus.u32LeftStreamFrames, stGrpStatus.u32LeftPics[0],
                         stGrpStatus.u32LeftPics[1], stGrpStatus.u32LeftPics[2],
                         stGrpStatus.bStartRecvStream, stGrpStatus.u32RecvStreamFrames,
                         stGrpStatus.u32DecodeStreamFrames, stGrpStatus.u32PicWidth,
                         stGrpStatus.u32PicHeight, stGrpStatus.stVdecDecErr.s32PicSizeErrSet,
                         stGrpStatus.stVdecDecErr.s32StreamUnsprt, stGrpStatus.stVdecDecErr.s32PackErr,
                         stGrpStatus.stVdecDecErr.s32PicBufSizeErrSet);
        else
            SAMPLE_CRIT_LOG("VdGrp=%d,  AX_VDEC_QueryStatus FAILED! ret:0x%x", VdGrp, ret);
    }

    if (pstCmd->bGetVuiParam) {
        AX_VDEC_VUI_PARAM_T stVuiParam;
        memset(&stVuiParam, 0x0, sizeof(AX_VDEC_VUI_PARAM_T));
        ret = AX_VDEC_GetVuiParam(VdGrp, &stVuiParam);
        if (AX_SUCCESS == ret) {
            SAMPLE_LOG_T("================= VdGrp=%d, AX_VDEC_GetVuiParam: "
                         "time_scale: %u, num_units_in_tick: %d",
                         VdGrp,
                         stVuiParam.stVuiTimeInfo.time_scale,
                         stVuiParam.stVuiTimeInfo.num_units_in_tick);
        }
        else if (AX_ERR_VDEC_QUEUE_EMPTY != ret){
            SAMPLE_CRIT_LOG("VdGrp=%d,  AX_VDEC_GetVuiParam FAILED! ret:0x%x", VdGrp, ret);
        }
    }
}
