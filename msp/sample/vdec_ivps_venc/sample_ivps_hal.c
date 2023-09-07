/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "ax_base_type.h"
#include "ax_sys_api.h"
#include "ax_buffer_tool.h"
#include "ax_ivps_api.h"


#ifndef AX_SUCCESS
    #define AX_SUCCESS                          0
#endif

extern AX_S32 gLoopExit;


#define SAMPLE_LOG(str, arg...)    \
    do {    \
        printf("%s: %s:%d "str"\n", "sample_vdec_ivps.c", __func__, __LINE__, ##arg); \
    } while(0)

#define SAMPLE_ERR_LOG(str, arg...)   \
    do{  \
        printf("%s: %s:%d Error! "str"\n", "sample_vdec_ivps.c", __func__, __LINE__, ##arg); \
    }while(0)


#define IVPS_BUF_POOL_MEM_SIZE (0x100000 * 40) // 40M
#define VDEC_ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

typedef struct {
    AX_U32 nSize;
    AX_U32 nCnt;
} BLK_INFO_S;


AX_U32 CalcImgSize(AX_U32 nStride, AX_U32 nW, AX_U32 nH, AX_IMG_FORMAT_E eType, AX_U32 nAlign)
{
    AX_U32 nBpp = 0;
    if (nW == 0 || nH == 0) {
        SAMPLE_LOG("Invalid width %d or height %d!", nW, nH);
        return 0;
    }

    if (0 == nStride) {
        nStride = (0 == nAlign) ? nW : VDEC_ALIGN_UP(nW, nAlign);
    } else {
        if (nAlign > 0) {
            if (nStride % nAlign) {
                SAMPLE_ERR_LOG(" stride: %u not %u aligned.!", nStride, nAlign);
                return 0;
            }
        }
    }

    switch (eType) {
    case AX_FORMAT_YUV420_PLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        nBpp = 12;
        break;
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        nBpp = 16;
        break;
    case AX_FORMAT_YUV444_PACKED:
    case AX_FORMAT_RGB888:
    case AX_FORMAT_BGR888:
        nBpp = 24;
        break;
    case AX_FORMAT_RGBA8888:
    case AX_FORMAT_ARGB8888:
        nBpp = 32;
        break;
    default:
        SAMPLE_ERR_LOG(" ERR eType : %d!",  eType);
        nBpp = 0;
        break;
    }

    return nStride * nH * nBpp / 8;
}

AX_S32 SampleIVPS_Init(int GrpNum, AX_U32 width, AX_U32 height)
{
    AX_S32 axRet = 0;
    int index = 0;
    AX_IVPS_GRP_ATTR_T  stGrpAttr = {0};
    AX_IVPS_PIPELINE_ATTR_T  stPipelineAttr = {0};
    int ch = 0;
    AX_IVPS_POOL_ATTR_T PoolAttr = {0};
    IVPS_GRP gIvpsGrpId = 0;

    /****************************IVPS Prepare*********************************/
    /*
     * 1. Create memory pool for IVPS
     */

    axRet = AX_IVPS_Init();
    if (0 != axRet) {
        SAMPLE_ERR_LOG("AX_IVPS_Init axRet:%#x\n", axRet);
        return -1;
    }

    ch = 1;
    stPipelineAttr.nOutChnNum = 1;
    stPipelineAttr.tFilter[ch][0].bEngage = AX_TRUE;
    stPipelineAttr.tFilter[ch][0].tFRC.fSrcFrameRate = 30;
    stPipelineAttr.tFilter[ch][0].tFRC.fDstFrameRate = 30;
    stPipelineAttr.tFilter[ch][0].nDstPicWidth = width;
    stPipelineAttr.tFilter[ch][0].nDstPicHeight = height;
    stPipelineAttr.tFilter[ch][0].nDstPicStride = stPipelineAttr.tFilter[ch][0].nDstPicWidth;
    stPipelineAttr.tFilter[ch][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    stPipelineAttr.tFilter[ch][0].eEngine = AX_IVPS_ENGINE_VPP;
    stPipelineAttr.tFilter[ch][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_0;
    stPipelineAttr.tFilter[ch][0].tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
    stPipelineAttr.nOutFifoDepth[ch] = 4;

    stGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;
    for(index = 0; index < GrpNum; index++) {
        gIvpsGrpId = index;
        axRet = AX_IVPS_CreateGrp(gIvpsGrpId, &stGrpAttr);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_CreateGrp axRet:%#x\n", axRet);
            return -2;
        }
        axRet = AX_IVPS_SetPipelineAttr(gIvpsGrpId, &stPipelineAttr);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_SetPipelineAttr axRet:%#x\n", axRet);
            return -20;
        }

        PoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
        PoolAttr.nFrmBufNum =12;

        axRet = AX_IVPS_SetChnPoolAttr(gIvpsGrpId, 0, &PoolAttr);
        if (IVPS_SUCC != axRet)
        {
            SAMPLE_ERR_LOG("AX_IVPS_SetGrpPoolAttr(Grp: %d) failed, ret=0x%x.", gIvpsGrpId, axRet);
            return -3;
        }

        axRet = AX_IVPS_EnableChn(gIvpsGrpId, 0);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_EnableChn ch.%d, axRet:%#x\n", 0, axRet);
            return -3;
        }

#ifdef IVPS_EZOOM_TEST_EN
        /* min_nFifoDepth: tdp:4 other engine: 3 */
        axRet = AX_IVPS_EnableBackupFrame(gIvpsGrpId, 4);
        if (IVPS_SUCC != axRet)
        {
            SAMPLE_ERR_LOG("AX_IVPS_EnableBackupFrame(Grp: %d) failed, ret=0x%x.", gIvpsGrpId, axRet);
            return -3;
        }
#endif

        axRet = AX_IVPS_StartGrp(gIvpsGrpId);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_StartGrp axRet:%#x\n", axRet);
            return -3;
        }
    }

    return 0;
}

AX_S32 SampleIvpsExit(int GrpNum)
{
    AX_S32 axRet = 0;
    int ch = 0;
    int index = 0;
    IVPS_GRP gIvpsGrpId = 0;


    for(index = 0; index < GrpNum; index++) {
        ch = 0;
        gIvpsGrpId = index;
        axRet = AX_IVPS_DisableChn(gIvpsGrpId, ch);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_DisableChn ch.%d, axRet:%#x\n", ch, axRet);
        }
#ifdef IVPS_EZOOM_TEST_EN
        axRet = AX_IVPS_DisableBackupFrame(gIvpsGrpId);
        if (IVPS_SUCC != axRet)
        {
            SAMPLE_ERR_LOG("AX_IVPS_DisableBackupFrame(Grp: %d) failed, ret=0x%x.", gIvpsGrpId, axRet);
            return -3;
        }
#endif

        axRet = AX_IVPS_StopGrp(gIvpsGrpId);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_StopGrp axRet:%#x\n", axRet);
        }
        axRet = AX_IVPS_DestoryGrp(gIvpsGrpId);
        if (0 != axRet) {
            SAMPLE_ERR_LOG("AX_IVPS_DestoryGrp axRet:%#x\n", axRet);
        }
    }

    axRet = AX_IVPS_Deinit();
    if (0 != axRet) {
        SAMPLE_ERR_LOG("AX_IVPS_Deinit axRet:%#x\n", axRet);
    }

    return 0;
}
