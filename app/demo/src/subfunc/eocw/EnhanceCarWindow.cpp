/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#include "EnhanceCarWindow.h"
#include "AppLogApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include "sample_utils.h"
#define EOCW "EOCW"

AX_ENGINE_HANDLE gEnginehandle = NULL;
AX_S32 gDmaChn = -1;
AX_SAMPLE_ENHANCE_CFG_T gtSampleEnhanceCfg = {
    .pModelPath = (AX_S8 *)"/opt/data/eocw/SC910_SDR2D_1088x576_12b_LCG_ISP1_A4-8X_F000_00000000001_000000_AX650_dec.axmodel",
    .pCarWindowPath = (AX_S8 *)"",
    .pCarWindowMaskPath = (AX_S8 *)"/opt/data/eocw/mask_1ch.bin",
    .pCarWindowEnhancePath = (AX_S8 *)"",
    .tEngineIOData = {0},
};

#define AX_EOCW_CMM_ALIGN_SIZE 64
const char* AX_EOCW_CMM_TOKEN_NAME = "eocw";

static AX_VOID __read_file_to_cmm(const AX_S8 *filename, AX_U32 *size_out, AX_VOID *ptr)
{
    FILE *fp = AX_NULL;
    AX_S32 size = 0;
    // AX_CHAR *ptr = AX_NULL;

    fp = fopen((const AX_CHAR *)filename, "rb");
    if (NULL == fp) {
        LOG_M_E(EOCW, "Error:Open [%s] file fail!", filename);
        return;
    }

    // 求得文件的大小
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    *size_out = size;
    rewind(fp);
    // 申请一块能装下整个文件的空间
    // ptr = (AX_CHAR *)malloc(size);
    // 读文件
    if (AX_NULL == ptr) {
        fclose(fp);
        return;
    }

    fread(ptr, 1, size, fp); // 每次读一个，共读size次
    fclose(fp);
}

static AX_VOID *__read_file(const AX_S8 *filename, AX_U32 *size_out)
{
    FILE *fp = AX_NULL;
    AX_S32 size = 0;
    AX_CHAR *ptr = AX_NULL;

    fp = fopen((const AX_CHAR *)filename, "rb");
    if (NULL == fp) {
        LOG_M_E(EOCW, "Error:Open [%s] file fail!", filename);
        return AX_NULL;
    }

    // 求得文件的大小
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    *size_out = size;
    rewind(fp);
    // 申请一块能装下整个文件的空间
    ptr = (AX_CHAR *)malloc(size);
    // 读文件
    if (AX_NULL == ptr) {
        fclose(fp);
        return AX_NULL;
    }

    fread(ptr, 1, size, fp); // 每次读一个，共读size次
    fclose(fp);
    return ptr;
}

// static AX_VOID __print_engine_io_info(AX_ENGINE_IO_INFO_T* io_info) {
//     if (io_info != NULL) {
//         LOG_M_D(EOCW, "*** nInputSize=%d", io_info->nInputSize);
//         for (AX_U32 i = 0; i < io_info->nInputSize; ++i) {
//             AX_ENGINE_IOMETA_T meta = io_info->pInputs[i];
//             LOG_M_D(EOCW, "****** [%d] name=%s size=%d", i, meta.pName, meta.nSize);
//         }

//         LOG_M_D(EOCW, "*** nOutputSize=%d", io_info->nOutputSize);
//         for (AX_U32 i = 0; i < io_info->nOutputSize; ++i) {
//             AX_ENGINE_IOMETA_T meta = io_info->pOutputs[i];
//             LOG_M_D(EOCW, "****** [%d] name=%s size=%d", i, meta.pName, meta.nSize);
//         }
//     } else {
//         LOG_M_E(EOCW, "io_info is nullptr.");
//     }
// }

void free_io_index(AX_ENGINE_IO_BUFFER_T* io_buf, size_t index)
{
    for (size_t i = 0; i < index; ++i) {
        AX_ENGINE_IO_BUFFER_T* pBuf = io_buf + i;
        AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
        pBuf->phyAddr = 0;
        pBuf->pVirAddr = AX_NULL;
    }
}

void __deprepare_engine_io(AX_ENGINE_IO_T* io_data)
{
    if (io_data->nInputSize != 0) {
        for (size_t j = 0; j < io_data->nInputSize; ++j) {
            AX_ENGINE_IO_BUFFER_T* pBuf = io_data->pInputs + j;
            AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
        }
        free(io_data->pInputs);
    }

    if (io_data->nOutputSize != 0) {
        for (size_t j = 0; j < io_data->nOutputSize; ++j) {
            AX_ENGINE_IO_BUFFER_T* pBuf = io_data->pOutputs + j;
            AX_SYS_MemFree(pBuf->phyAddr, pBuf->pVirAddr);
        }
        free(io_data->pOutputs);
    }
}

static AX_BOOL __prepare_engine_io(AX_ENGINE_IO_INFO_T* info, AX_ENGINE_IO_T* io_data)
{
    AX_BOOL bRet = AX_FALSE;
    AX_S32 ret = 0;
    do {
        AX_BOOL bbRet = AX_TRUE;

        memset(io_data, 0, sizeof(*io_data));
        // input
        if (info->nInputSize != 0) {
            io_data->pInputs = (AX_ENGINE_IO_BUFFER_T *)malloc(sizeof(AX_ENGINE_IO_BUFFER_T)*info->nInputSize);
            io_data->nInputSize = info->nInputSize;
            for (AX_U32 i = 0; i < info->nInputSize; ++i) {
                AX_ENGINE_IOMETA_T meta = info->pInputs[i];
                AX_ENGINE_IO_BUFFER_T *buffer = &io_data->pInputs[i];
                ret = AX_SYS_MemAlloc((AX_U64*)(&buffer->phyAddr),
                                    &buffer->pVirAddr,
                                    meta.nSize,
                                    AX_EOCW_CMM_ALIGN_SIZE,
                                    (const AX_S8*)(AX_EOCW_CMM_TOKEN_NAME));
                if (ret != 0) {
                    // free_io_index(io_data->pInputs, i);
                    LOG_M_E(EOCW, "Allocate input{%d} { phy: %p, vir: %p, size: %lu Bytes } failed, ret=0x%x.",
                            i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize, ret);
                    bbRet = AX_FALSE;
                    break;
                }
            }
        }
        bRet = bbRet;
        if (!bRet) {
            break;
        }

        // output
        if (info->nOutputSize != 0) {
            io_data->pOutputs = (AX_ENGINE_IO_BUFFER_T *)malloc(sizeof(AX_ENGINE_IO_BUFFER_T)*info->nOutputSize);
            io_data->nOutputSize = info->nOutputSize;
            for (AX_U32 i = 0; i < info->nOutputSize; ++i)
            {
                AX_ENGINE_IOMETA_T meta = info->pOutputs[i];
                AX_ENGINE_IO_BUFFER_T * buffer = &io_data->pOutputs[i];
                buffer->nSize = meta.nSize;
                ret = AX_SYS_MemAlloc((AX_U64*)(&buffer->phyAddr),
                                    &buffer->pVirAddr,
                                    meta.nSize,
                                    AX_EOCW_CMM_ALIGN_SIZE, (const AX_S8*)(AX_EOCW_CMM_TOKEN_NAME));
                if (ret != 0)
                {
                    LOG_M_E(EOCW, "Allocate output{%d} { phy: %p, vir: %p, size: %lu Bytes } failed, ret=0x%x.",
                            i, (void*)buffer->phyAddr, buffer->pVirAddr, (long)meta.nSize, ret);
                    bbRet = AX_FALSE;
                    break;
                }
            }
        }
        bRet = bbRet;
        if (!bRet) {
            break;
        }
    } while (0);
    return bRet;
}

static AX_BOOL __push_input_offset_mask(AX_ENGINE_IO_INFO_T* info_t, AX_ENGINE_IO_T* io_t)
{
    AX_BOOL bRet = AX_FALSE;
    AX_VOID *pMaskData = AX_NULL;
    do {

        // input offset: ppl_offset-model_bias=1024-600
        AX_U16 offset = 1024-600;
        // [0] offset_in
        memcpy(io_t->pInputs[IDX_OFFSET_IN].pVirAddr, &offset, sizeof(AX_U16));
        // [1] offset_out
        memcpy(io_t->pInputs[IDX_OFFSET_OUT].pVirAddr, &offset, sizeof(AX_U16));

        // [3] mask data
        AX_S8 *pMaskFile = gtSampleEnhanceCfg.pCarWindowMaskPath;
        AX_U32 nMaskSize = 0;
        __read_file_to_cmm(pMaskFile, &nMaskSize, io_t->pInputs[IDX_ROI_MASK].pVirAddr);
        if (nMaskSize != info_t->pInputs[IDX_ROI_MASK].nSize) {
            LOG_M_E(EOCW, "The input mask data size is not matched with tensor {file: %s, name: %s, size: %d}.",
                    pMaskFile, info_t->pInputs[IDX_ROI_MASK].pName, info_t->pInputs[IDX_ROI_MASK].nSize);
            break;
        }

        bRet = AX_TRUE;
    } while(0);

    if (pMaskData != AX_NULL) {
        free(pMaskData);
    }
    return bRet;
}

// static AX_BOOL __debug_push_input_raw(AX_ENGINE_IO_INFO_T* info_t, AX_ENGINE_IO_T* io_t) {
//     AX_BOOL bRet = AX_FALSE;
//     AX_VOID *pRawData = AX_NULL;
//     do {
//         // [2] raw data
//         AX_U32 index = 2;
//         AX_U32 nRawSize = 0;
//         AX_S8 *pRawFile = gtSampleEnhanceCfg.pCarWindowPath;
//         if (AX_NULL != pRawFile) {
//             pRawData = __read_file(pRawFile, &nRawSize);
//             if (!pRawData) {
//                 LOG_M_E(EOCW, "read mask file %s failed", pRawFile);
//                 break;
//             }
//             if (nRawSize != info_t->pInputs[index].nSize) {
//                 LOG_M_E(EOCW, "The input mask data size is not matched with tensor {file: %s, name: %s, size: %d}.",
//                         pRawFile, info_t->pInputs[index].pName, info_t->pInputs[index].nSize);
//                 break;
//             }
//             memcpy(io_t->pInputs[index].pVirAddr, pRawData, nRawSize);
//         }

//         bRet = AX_TRUE;
//     } while(0);

//     if (pRawData != AX_NULL) {
//         free(pRawData);
//     }
//     return bRet;
// }

// static AX_BOOL __debug_post_process(AX_ENGINE_IO_INFO_T* info_t, AX_ENGINE_IO_T* io_t) {

//     AX_BOOL bRet = AX_FALSE;
//     FILE *fp = NULL;
//     do {
//         AX_S8 *pEnhanceRawFile = gtSampleEnhanceCfg.pCarWindowEnhancePath;
//         if (AX_NULL != pEnhanceRawFile) {
//             fp = fopen((const char*)pEnhanceRawFile, "wb");
//             if (!fp) {
//                 LOG_M_E(EOCW, "ERROR Fail to open file:%s!", pEnhanceRawFile);
//                 break;
//             }

// // for test
// #if 0
//             AX_U32 index = 2;
//             AX_U32 nSize = info_t->pInputs[index].nSize;
//             AX_ENGINE_IO_BUFFER_T *buffer = &io_t->pInputs[index];
// #else
//             AX_U32 index = 0;
//             AX_U32 nSize = info_t->pOutputs[index].nSize;
//             AX_ENGINE_IO_BUFFER_T *buffer = &io_t->pOutputs[index];
// #endif
//             LOG_M_E(EOCW, "file=%s, size=%d", pEnhanceRawFile, info_t->pInputs[index].nSize);
//             fwrite((AX_VOID *)buffer->pVirAddr, 1, nSize, fp);
//         }

//         bRet = AX_TRUE;
//     } while(0);

//     if (fp != NULL) {
//         fclose(fp);
//     }
//     return bRet;
// }

// static AX_BOOL __debug_save_dma_file(const AX_S8 *pFile, AX_U64 u64PhyAddr, AX_U32 u32Size) {
//     FILE *fp = NULL;
//     // AX_S8 *pFile = (AX_S8 *)"/opt/bin/ENHANCE/_copy_refFrame_out.raw";
//     if (AX_NULL != pFile) {
//         fp = fopen((const char*)pFile, "wb");
//         if (!fp) {
//             LOG_M_E(EOCW, "ERROR Fail to open file:%s!", pFile);
//             return AX_FALSE;
//         }

//         AX_U64 u64VirAddr = (AX_ULONG)AX_SYS_Mmap(u64PhyAddr, u32Size);
//         fwrite((AX_VOID *)((AX_ULONG)u64VirAddr), 1, u32Size, fp);
//         AX_SYS_Munmap((AX_VOID *)(AX_ULONG)u64VirAddr, u32Size);
//     }

//     if (fp != NULL) {
//         fclose(fp);
//     }

//     return AX_TRUE;
// }

static AX_SAMPLE_ENHANCE_CW_RECT_T __normalizationRoi(AX_U32 nX, AX_U32 nY, AX_U32 nW, AX_U32 nH, AX_U32 nImgW, AX_U32 nImgH) {
    AX_SAMPLE_ENHANCE_CW_RECT_T roi;

#if 1
    roi.nX = nX;
    roi.nY = nY;
    roi.nW = nW;
    roi.nH = nH;
#else
    const AX_U32 ALIGN_SIZE = 2;
    const AX_U32 ROI_W = 1088;
    const AX_U32 ROI_H = 576;

    AX_U32 Cx = ((nX - ALIGN_SIZE + 1)/ALIGN_SIZE)*ALIGN_SIZE;
    Cx = (Cx - ROI_W/2) < 0 ? (ROI_W/2) : Cx;
    Cx = (Cx + ROI_W/2) > nImgW ? (nImgW - ROI_W/2) : Cx;

    AX_U32 Cy = ((nY - ALIGN_SIZE + 1)/ALIGN_SIZE)*ALIGN_SIZE;
    Cy = (Cy - ROI_H/2) < 0 ? (ROI_H/2) : Cy;
    Cy = (Cy + ROI_H/2) > nImgH ? (nImgH - ROI_H/2) : Cy;
    AX_U32 Cw = ROI_W;
    AX_U32 Ch = ROI_H;

    roi.nX = Cx;
    roi.nY = Cy;
    roi.nW = Cw;
    roi.nH = Ch;
#endif
    return roi;
}

static AX_BOOL __cropRoi(AX_S32 s32DmaChn, AX_SAMPLE_ENHANCE_CW_RECT_T *pRoi, AX_IMG_INFO_T *pImgInfo, AX_ENGINE_IO_T *pEngineIO) {
/*
    // 16bitRaw
    AX_DMADIM_DESC_XD_T tDimDescCrop = {
        .u16Ntiles = {576, 0, 0},
        .tSrcInfo = {
            .u32Imgw = 1088*2,
            .u32Stride = {3840*2, 0, 0},
        },
        .tDstInfo = {
            .u32Imgw = 1088*2,
            .u32Stride = {1088*2, 0, 0},
        },
    };
*/
    AX_BOOL bRet = AX_FALSE;
    do {
        AX_S32 ret = 0;

        // TODO: params verify
        // - roi
        LOG_M_D(EOCW, "roi=(%d,%d,%d,%d)", pRoi->nX, pRoi->nY, pRoi->nW, pRoi->nH);
        // - image frame
        AX_VIDEO_FRAME_T stFrame = pImgInfo->tFrameInfo.stVFrame;
        LOG_M_D(EOCW, "frame=(%d,%d)", stFrame.u32Width, stFrame.u32Height);
        // - engine io
        LOG_M_D(EOCW, "engine=(insize=%d)", pEngineIO->nInputSize);

#if 1
        AX_DMADIM_MSG_T tDmaMsg;
        tDmaMsg.u32DescNum = 1;
        tDmaMsg.eEndian = AX_DMADIM_ENDIAN_DEF;
        tDmaMsg.eDmaMode = AX_DMADIM_2D;

        AX_DMADIM_DESC_XD_T tDescBuf;
        tDescBuf.u16Ntiles[0] = pRoi->nH;
        // src: ref-frame raw
        tDescBuf.tSrcInfo.u32Imgw = pRoi->nW*2;
        tDescBuf.tSrcInfo.u32Stride[0] = stFrame.u32Width*2;
        tDescBuf.tSrcInfo.u64PhyAddr = stFrame.u64PhyAddr[0] + pRoi->nY*stFrame.u32Width*2 + pRoi->nX*2;
        // dst: roi
        tDescBuf.tDstInfo.u32Imgw = pRoi->nW*2;
        tDescBuf.tDstInfo.u32Stride[0] = pRoi->nW*2;
        tDescBuf.tDstInfo.u64PhyAddr = pEngineIO->pInputs[IDX_ROI_RAW].phyAddr;

        tDmaMsg.pDescBuf = &tDescBuf;

        AX_DMADIM_XFER_STAT_T tXferStat;
        // memset(&tXferStat, 0, sizeof(AX_DMADIM_XFER_STAT_T));
        tXferStat.s32Id = AX_DMADIM_Cfg(s32DmaChn, &tDmaMsg);
        if (tXferStat.s32Id <= 0) {
            LOG_M_E(EOCW, "AX_DMADIM_Cfg failed, ret=0x%x", tXferStat.s32Id);
            break;
        }

        // start
        ret = AX_DMADIM_Start(s32DmaChn, tXferStat.s32Id);
        if (ret) {
            LOG_M_E(EOCW, "AX_DMADIM_Start failed, ret=0x%x", ret);
            break;
        }

        // sync mode: waitdone
        ret = AX_DMADIM_Waitdone(s32DmaChn, &tXferStat, 5000);
        if (ret) {
            LOG_M_E(EOCW, "AX_DMADIM_Waitdone failed, ret=0x%x", ret);
            break;
        }
#else
        AX_S8 *pRawFile = gtSampleEnhanceCfg.pCarWindowPath;
        AX_U32 nSize = 0;
        if (AX_NULL != pRawFile) {
            __read_file_to_cmm(pRawFile, &nSize, pEngineIO->pInputs[IDX_ROI_RAW].pVirAddr);
        }
#endif

#if 0
        // __debug_save_dma_file((const AX_S8 *)"/opt/bin/ENHANCE/0817_frame.raw", stFrame.u64PhyAddr[0], stFrame.u32Width*stFrame.u32Height*2);
        // __debug_save_dma_file((const AX_S8 *)"/opt/bin/ENHANCE/0817_frame_dma_crop.raw", tDescBuf.tDstInfo.u64PhyAddr, pRoi->nW*pRoi->nH*2);
        __debug_save_dma_file((const AX_S8 *)"/opt/bin/ENHANCE/roi_crop.raw", pEngineIO->pInputs[IDX_ROI_RAW].phyAddr, pRoi->nW*pRoi->nH*2);
#endif
        bRet = AX_TRUE;
    } while(0);

    return bRet;
}

static AX_BOOL __enhanceRoi(AX_ENGINE_HANDLE handle, AX_ENGINE_IO_T *pEngineIO) {
    AX_BOOL bRet = AX_FALSE;
    do {

#if 1
        AX_S32 ret = 0;
        ret = AX_ENGINE_RunSync(handle, pEngineIO);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_RunSync failed, ret=0x%x", ret);
            break;
        }
#else
        // fake enhace out
        AX_S8 *pRawFile = gtSampleEnhanceCfg.pCarWindowPath;
        AX_U32 nSize = 0;
        if (AX_NULL != pRawFile) {
            __read_file_to_cmm(pRawFile, &nSize, pEngineIO->pOutputs[IDX_ENHANCE_ROI_RAW].pVirAddr);
        }
#endif

#ifdef __ENHANCE_DEBUG
        // debug: save enhance raw
        AX_ENGINE_IO_INFO_T* io_info = NULL;
        ret = AX_ENGINE_GetIOInfo(*handle, &io_info);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_GetIOInfo failed, ret=0x%x", ret);
            break;
        }
        __debug_post_process(io_info, pEngineIO);
#endif
        bRet = AX_TRUE;
    } while(0);
    return bRet;
}

static AX_BOOL __blendRoi(AX_S32 s32DmaChn, AX_SAMPLE_ENHANCE_CW_RECT_T *pRoi, AX_IMG_INFO_T *pImgInfo, AX_ENGINE_IO_T *pEngineIO) {
/*
    AX_DMADIM_DESC_XD_T tDimDescCrop = {
        .u16Ntiles = {576, 0, 0},
        // roi
        .tSrcInfo = {
            .u32Imgw = 1088*2,
            .u32Stride = {1088*2, 0, 0},
        },
        // ref-frame
        .tDstInfo = {
            .u32Imgw = 1088*2,
            .u32Stride = {3840*2, 0, 0},
        },
    };
*/
    AX_BOOL bRet = AX_FALSE;
    do {
        AX_S32 ret = 0;

        // TODO: params verify
        // - roi
        LOG_M_D(EOCW, "roi=(%d,%d,%d,%d)", pRoi->nX, pRoi->nY, pRoi->nW, pRoi->nH);
        // - image frame
        AX_VIDEO_FRAME_T stFrame = pImgInfo->tFrameInfo.stVFrame;
        LOG_M_D(EOCW, "frame=(%d,%d)", stFrame.u32Width, stFrame.u32Height);
        // - engine io
        LOG_M_D(EOCW, "engine=(insize=%d)", pEngineIO->nOutputSize);

        AX_DMADIM_MSG_T tDmaMsg;
        tDmaMsg.u32DescNum = 1;
        tDmaMsg.eEndian = AX_DMADIM_ENDIAN_DEF;
        tDmaMsg.eDmaMode = AX_DMADIM_2D;

        AX_DMADIM_DESC_XD_T tDescBuf;
        tDescBuf.u16Ntiles[0] = pRoi->nH;
        // src: enhance engine output
        tDescBuf.tSrcInfo.u32Imgw = pRoi->nW*2;
        tDescBuf.tSrcInfo.u32Stride[0] = pRoi->nW*2;
        tDescBuf.tSrcInfo.u64PhyAddr = pEngineIO->pOutputs[IDX_ENHANCE_ROI_RAW].phyAddr;
        // tDescBuf.tSrcInfo.u64PhyAddr = pEngineIO->pInputs[IDX_ROI_RAW].phyAddr;
        // dst: ref-frame raw
        tDescBuf.tDstInfo.u32Imgw = pRoi->nW*2;
        tDescBuf.tDstInfo.u32Stride[0] = stFrame.u32Width*2;
        tDescBuf.tDstInfo.u64PhyAddr = stFrame.u64PhyAddr[0] + pRoi->nY*stFrame.u32Width*2 + pRoi->nX*2;
        tDmaMsg.pDescBuf = &tDescBuf;

        AX_DMADIM_XFER_STAT_T tXferStat;
        // memset(&tXferStat, 0, sizeof(AX_DMADIM_XFER_STAT_T));
        tXferStat.s32Id = AX_DMADIM_Cfg(s32DmaChn, &tDmaMsg);
        if (tXferStat.s32Id <= 0) {
            LOG_M_E(EOCW, "AX_DMADIM_Cfg failed, ret=0x%x", tXferStat.s32Id);
            break;
        }

        // start
        ret = AX_DMADIM_Start(s32DmaChn, tXferStat.s32Id);
        if (ret) {
            LOG_M_E(EOCW, "AX_DMADIM_Start failed, ret=0x%x", ret);
            break;
        }

        // SYNC mode: waitdone
        ret = AX_DMADIM_Waitdone(s32DmaChn, &tXferStat, 5000);
        if (ret) {
            LOG_M_E(EOCW, "AX_DMADIM_Waitdone failed, ret=0x%x", ret);
            break;
        }

#if 0
        __debug_save_dma_file((const AX_S8 *)"/opt/bin/ENHANCE/roi_enhance.raw", pEngineIO->pOutputs[IDX_ENHANCE_ROI_RAW].phyAddr, pRoi->nW*pRoi->nH*2);
        __debug_save_dma_file((const AX_S8 *)"/opt/bin/ENHANCE/frame_enhance.raw", stFrame.u64PhyAddr[0], stFrame.u32Width*stFrame.u32Height*2);
#endif

        bRet = AX_TRUE;
    } while(0);

    return bRet;
}

AX_U64 __getTickcount(AX_VOID)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

AX_BOOL SAMPLE_ENHANCE_Init(AX_ENHANCE_CONFIG_T *tEnhanceCfg, AX_U32 nEnhanceCfgCnt) {
// AX_BOOL SAMPLE_ENHANCE_Init(AX_S8 *pModelPath, AX_S8 *pCarWindowMaskPath) {

    AX_BOOL bRet = AX_FALSE;
    AX_VOID *pModelData = AX_NULL;
    LOG_M_D(EOCW, "%s +++ ", __func__);



    do {
        AX_S32 ret = 0;
        AX_ENGINE_IO_T *pEngineIO = &gtSampleEnhanceCfg.tEngineIOData;

        if (nEnhanceCfgCnt > 0) {
            AX_ENHANCE_CONFIG_T &enchance = tEnhanceCfg[0];
            gtSampleEnhanceCfg.pModelPath = (AX_S8 *)enchance.szModel;
            gtSampleEnhanceCfg.pCarWindowMaskPath = (AX_S8 *)enchance.szMask;
        }

        LOG_M_E(EOCW, "model=%s", gtSampleEnhanceCfg.pModelPath);
        LOG_M_E(EOCW, "mask=%s", gtSampleEnhanceCfg.pCarWindowMaskPath);

        AX_U32 nModelSize = 0;
        AX_S8 *pModelFile = gtSampleEnhanceCfg.pModelPath;
        if (AX_NULL == pModelFile) {
            LOG_M_E(EOCW, "model file is null");
            break;
        }

        // 1. init
        // - ax_engine
#if 0
        AX_ENGINE_NPU_ATTR_T npu_attr;
        memset(&npu_attr, 0, sizeof(npu_attr));
        npu_attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
        ret = AX_ENGINE_Init(&npu_attr);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_Init failed, ret=0x%x", ret);
            break;
        }
#endif
        // - ax_dmadim: SYNC mode
        gDmaChn = AX_DMADIM_Open(AX_TRUE);
        if (gDmaChn < 0) {
            LOG_M_E(EOCW, "AX_DMADIM_Open failed, ret=0x%x", gDmaChn);
            break;
        }

        // 2. create
        /* load model to memory */
        pModelData = __read_file(pModelFile, &nModelSize);
        if (!pModelData) {
            LOG_M_E(EOCW, "read model file %s failed", pModelFile);
            break;
        }

        /* create model axengine handle */
        ret = AX_ENGINE_CreateHandle(&gEnginehandle, pModelData, nModelSize);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_CreateHandle failed, ret=0x%x", ret);
            break;
        }

        // 3. create context
        ret = AX_ENGINE_CreateContext(gEnginehandle);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_CreateContext failed, ret=0x%x", ret);
            break;
        }

        // 5. set io
        AX_ENGINE_IO_INFO_T* io_info = NULL;
        ret = AX_ENGINE_GetIOInfo(gEnginehandle, &io_info);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_GetIOInfo failed, ret=0x%x", ret);
            break;
        }
        // __print_engine_io_info(io_info);

        // 6. alloc io
        if (!__prepare_engine_io(io_info, pEngineIO)) {
            LOG_M_E(EOCW, "prepare engine io failed");
            break;
        }

        // 7. input offset and mask
        if (!__push_input_offset_mask(io_info, pEngineIO)) {
            LOG_M_E(EOCW, "push engine input offset and mask failed");
            break;
        }

        // // 8. debug, input car window raw
        // if (!__debug_push_input_raw(io_info, pEngineIO)) {
        //     LOG_M_E(EOCW, "push engine input raw failed");
        //     break;
        // }

        bRet = AX_TRUE;
    } while(0);

    if (pModelData != AX_NULL) {
        free(pModelData);
    }

    LOG_M_D(EOCW, "%s --- ", __func__);
    return bRet;
}

AX_BOOL SAMPLE_ENHANCE_DeInit(AX_VOID) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(EOCW, "%s +++ ", __func__);
    do {
        AX_S32 ret = 0;

        __deprepare_engine_io(&gtSampleEnhanceCfg.tEngineIOData);

        ret = AX_ENGINE_DestroyHandle(gEnginehandle);
        if (0 != ret) {
            LOG_M_E(EOCW, "AX_ENGINE_DestroyHandle failed, ret=0x%x", ret);
        }

        if (gDmaChn >= 0) {
            ret = AX_DMADIM_Close(gDmaChn);
            if (0 != ret) {
                LOG_M_E(EOCW, "AX_DMADIM_Close failed, ret=0x%x", ret);
            }
        }

        bRet = AX_TRUE;
    } while(0);

    LOG_M_D(EOCW, "%s --- ", __func__);
    return bRet;
}

AX_BOOL SAMPLE_ENHANCE_Run(AX_SAMPLE_ENHANCE_CW_RECT_T *pRoi, AX_U32 nRoiSize, const AX_IMG_INFO_T *pImgInfo[]) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(EOCW, "%s +++ ", __func__);
    do {
        AX_BOOL bbRet = AX_TRUE;

        AX_IMG_INFO_T *pImgInfo_ = (AX_IMG_INFO_T *)pImgInfo;

        AX_VIDEO_FRAME_T stFrame = pImgInfo_->tFrameInfo.stVFrame;

        for (AX_U32 i = 0; i < nRoiSize; ++i) {
            // normalization ROI
            AX_SAMPLE_ENHANCE_CW_RECT_T *pRoiItem = pRoi + i;
            AX_SAMPLE_ENHANCE_CW_RECT_T stRoi = __normalizationRoi(pRoiItem->nX, pRoiItem->nY, pRoiItem->nW, pRoiItem->nH,
                                                                stFrame.u32Width, stFrame.u32Height);

            LOG_M_D(EOCW, "**** Image:%dx%d PhyAddr=0x%llx size=%d Roi:(%d,%d,%d,%d)",
                        stFrame.u32Width, stFrame.u32Height, stFrame.u64PhyAddr[0], stFrame.u32FrameSize,
                        stRoi.nX, stRoi.nY, stRoi.nW, stRoi.nH);

            {
                AX_U64 tick1 = __getTickcount();
                // crop ROI
                if (!__cropRoi(gDmaChn, &stRoi, pImgInfo_, &gtSampleEnhanceCfg.tEngineIOData)) {
                    bbRet = AX_FALSE;
                    break;
                }
                AX_U64 tick2 = __getTickcount();
                LOG_M_D(EOCW, "roi[%d] __cropRoi done! %lld us", i, tick2 - tick1);
            }

            {
                AX_U64 tick1 = __getTickcount();
                // enhance ROI
                if (!__enhanceRoi(gEnginehandle, &gtSampleEnhanceCfg.tEngineIOData)) {
                    bbRet = AX_FALSE;
                    break;
                }
                AX_U64 tick2 = __getTickcount();
                LOG_M_D(EOCW, "roi[%d] __enhanceRoi done! %lld us", i, tick2 - tick1);
            }


            {
                AX_U64 tick1 = __getTickcount();// blend ROI
                if (!__blendRoi(gDmaChn, &stRoi, pImgInfo_, &gtSampleEnhanceCfg.tEngineIOData)) {
                    bbRet = AX_FALSE;
                    break;
                }
                AX_U64 tick2 = __getTickcount();
                LOG_M_D(EOCW, "roi[%d] __blendRoi done! %lld us", i, tick2 - tick1);}
            }

        bRet = bbRet;
    } while(0);
    LOG_M_D(EOCW, "%s --- ", __func__);
    return bRet;
}