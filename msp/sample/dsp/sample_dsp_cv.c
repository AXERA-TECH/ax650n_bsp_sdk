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
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "ax_sys_api.h"
#include "util.h"
#include <ax_dsp_api.h>
#include <ax_dsp_cv_api.h>

#define USE_SAMPLE_FILE

//#define USE_DEBUG_FUNCTION

typedef struct {
    unsigned int u32Width;
    unsigned int u32Height;
} SAMPLE_DSP_RESOLUTION_RATION_S;

static SAMPLE_DSP_RESOLUTION_RATION_S resolution_ratio[] = {
    {
        .u32Width = 4096,
        .u32Height = 2160,
    },
    {
        .u32Width = 3840,
        .u32Height = 2160,
    },
    {
        .u32Width = 2560,
        .u32Height = 1600,
    },
    {
        .u32Width = 2560,
        .u32Height = 1440,
    },
    {
        .u32Width = 1920,
        .u32Height = 1200,
    },
    {
        .u32Width = 1920,
        .u32Height = 1080,
    },
    {
        .u32Width = 1600,
        .u32Height = 1200,
    },
    {
        .u32Width = 1600,
        .u32Height = 900,
    },
    {
        .u32Width = 1280,
        .u32Height = 1024,
    },
    {
        .u32Width = 1280,
        .u32Height = 720,
    },
    {
        .u32Width = 1024,
        .u32Height = 768,
    },
    {
        .u32Width = 1024,
        .u32Height = 576,
    },
    {
        .u32Width = 960,
        .u32Height = 540,
    },
    {
        .u32Width = 800,
        .u32Height = 600,
    },
    {
        .u32Width = 800,
        .u32Height = 480,
    },
    {
        .u32Width = 640,
        .u32Height = 480,
    },
    {
        .u32Width = 640,
        .u32Height = 320,
    },
    {
        .u32Width = 540,
        .u32Height = 960,
    },
    {
        .u32Width = 480,
        .u32Height = 640,
    },
    {
        .u32Width = 320,
        .u32Height = 640,
    },
    {
        .u32Width = 320,
        .u32Height = 240,
    },
};

extern AX_S32 DSP_LoadPic(const AX_CHAR *pszBinFileName, AX_VOID *vaddr);
extern AX_S32 DSP_SavePic(const char *pszBinFileName, AX_VOID *vaddr, AX_U32 size);

#ifdef USE_DEBUG_FUNCTION
static void DUMP_MEMORY(AX_MEM_INFO_T *meminfo, int offset, int size)
{
    int i;
    AX_U64 phy_addr = meminfo->u64PhyAddr + offset;
    AX_U64 vir_addr = meminfo->u64VirAddr + offset;

    printf("======== ADDR 0x%llX, size %d ========\n", phy_addr, size);
    for (i = 0; i < size; i += 0x10) {
        printf("%09llX + 0x%04X: %08X %08X %08X %08X\n", phy_addr + i, i,
               *(unsigned int *)(vir_addr + i), *(unsigned int *)(vir_addr + i + 0x4),
               *(unsigned int *)(vir_addr + i + 0x8), *(unsigned int *)(vir_addr + i + 0xC));
    }
}

static int DSP_CV_DATA_CHECK(AX_MEM_INFO_T *dst_addr, AX_MEM_INFO_T *src_addr, int offset, int size, int h, int w,
                             int precision)
{
    int row, col, err_offset;
    int cmp_flag = 1;
    unsigned long err_cnt = 0;
    unsigned char *dst = (unsigned char *)(dst_addr->u64VirAddr + offset);
    unsigned char *src = (unsigned char *)(src_addr->u64VirAddr + offset);

    if (size != h * w) {
        vdsp_err("%s: size %d bytes, width %d, height %d\n", __func__, size, w, h);
        return -1;
    }

    for (row = 0; row < h; ++row) {
        for (col = 0; col < w; ++col) {
            if (abs(dst[row * w + col] - src[row * w + col]) > precision && err_cnt < 10) {
                err_cnt++;
                vdsp_err("(%d, %d)=%dst byte compare bad:(0x%X, 0x%X), vir(0x%llX 0x%llX), phy(0x%llX 0x%llX)\n", row, col,
                         (row * w + col), dst[row * w + col], src[row * w + col], (AX_U64)&dst[row * w + col], (AX_U64)&src[row * w + col],
                         (dst_addr->u64PhyAddr + offset + row * w + col), (src_addr->u64PhyAddr + offset + row * w + col));
                if (cmp_flag) {
                    err_offset = row * w + col;
                }
                cmp_flag = 0;
            }
        }
    }

    vdsp_debug("-----------------------------------------------\n");
    if (cmp_flag) {
        vdsp_debug("total successful!\n");
        return 0;
    } else {
        vdsp_err("Dst[0x%llX 0x%llX], Src[0x%llX 0x%llX], offset 0x%X, size 0x%X, %dx%d, precision %d\n",
                 dst_addr->u64PhyAddr, dst_addr->u64VirAddr, src_addr->u64PhyAddr, src_addr->u64VirAddr, offset, size, w, h, precision);
        return err_offset;
    }
}
#endif

#define DSP_RESIZE_SRC_PATH       "/opt/data/dsp/resize"
#define DSP_RESIZE_DST_PATH       DSP_RESIZE_SRC_PATH
int SAMPLE_DSP_TestResize(AX_DSP_ID_E dspid)
{
    int ret = -1;
    int i, j, ration_num;
    unsigned int nSrcImgSize;//, nDstImgSize;
    char pSrcImgFile[128] = {0};
    char pDstImgFile[128] = {0};
    char pDstPath[64] = {0};
    AX_DSP_CV_RESIZE_PARAM_T param = {0};
    AX_DSP_IMAGE_T tSrcFrame = {0};
    AX_DSP_IMAGE_T tDstFrame = {0};
    struct stat statbuf;
    struct timeval tv_start, tv_end;

    if (stat(DSP_RESIZE_DST_PATH, &statbuf) != 0) {
        if (mkdir(DSP_RESIZE_DST_PATH, 0755) != 0 && errno != EEXIST) {
            vdsp_err("cannot create folder[%s]\n", DSP_RESIZE_DST_PATH);
            return -1;
        }
    } else {
        vdsp_debug("folder[%s] is exist\n", DSP_RESIZE_DST_PATH);
    }

    ration_num = sizeof(resolution_ratio) / sizeof(SAMPLE_DSP_RESOLUTION_RATION_S);
    vdsp_debug("%s: ration_num %d\n", __func__, ration_num);

    ret = AX_DSP_CV_Init(AX_DSP_ID_0);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }

    for (j = 0; j < ration_num; j++) {
        sprintf(pSrcImgFile, "%s/%dx%d.nv12.yuv", DSP_RESIZE_SRC_PATH, resolution_ratio[j].u32Width,
                resolution_ratio[j].u32Height);
        tSrcFrame.width = resolution_ratio[j].u32Width;
        tSrcFrame.height = resolution_ratio[j].u32Height;
        nSrcImgSize = tSrcFrame.width * tSrcFrame.height * 3 / 2;
        tSrcFrame.u32Size = nSrcImgSize;
        tSrcFrame.stride = tSrcFrame.width;

#ifdef USE_SAMPLE_FILE
        if (!LoadImage(pSrcImgFile, (AX_U64 *)&tSrcFrame.BufAddr[0].u64PhyAddr, (AX_VOID **)&tSrcFrame.BufAddr[0].u64VirAddr,
                       &nSrcImgSize)) {
            return -1;
        }
        printf("%d: %dx%d file %s load succ.\n", j, resolution_ratio[j].u32Width, resolution_ratio[j].u32Height,
               pSrcImgFile);
#else
        ret = AX_SYS_MemAlloc((AX_U64 *)&tSrcFrame.BufAddr[0].u64PhyAddr, (AX_VOID **)&tSrcFrame.BufAddr[0].u64VirAddr,
                              tSrcFrame.u32Size, SAMPLE_PHY_MEM_ALIGN_SIZE, NULL);
        if (0 != ret) {
            vdsp_err("%s AX_SYS_MemAlloc(0x%X) fail, ret=0x%x\n", __func__, tSrcFrame.u32Size, ret);
            return -1;
        }
        memset((AX_VOID *)tSrcFrame.BufAddr[0].u64VirAddr, 'Y', tSrcFrame.u32Size);
#endif

        tSrcFrame.BufAddr[0].u32Size = tSrcFrame.stride * tSrcFrame.height;
        tSrcFrame.BufAddr[1].u64PhyAddr = tSrcFrame.BufAddr[0].u64PhyAddr + tSrcFrame.BufAddr[0].u32Size;   // UV
        tSrcFrame.BufAddr[1].u64VirAddr = tSrcFrame.BufAddr[0].u64VirAddr + tSrcFrame.BufAddr[0].u32Size;
        tSrcFrame.BufAddr[1].u32Size = tSrcFrame.stride * tSrcFrame.height / 2;

#ifndef USE_SAMPLE_FILE
        memset((AX_VOID *)tSrcFrame.BufAddr[1].u64VirAddr, 'U', tSrcFrame.BufAddr[1].u32Size);
        {
            char *ptr = tSrcFrame.BufAddr[1].u64VirAddr;
            int k;
            for (k = 0; k < tSrcFrame.BufAddr[1].u32Size / 2; k++)
                ptr[2 * k + 1] = 'V';
        }
#endif

        memset((AX_VOID *)pDstPath, 0, sizeof(pDstPath));
        sprintf(pDstPath, "%s/src_%dx%d", DSP_RESIZE_DST_PATH, tSrcFrame.width, tSrcFrame.height);
        if (stat(pDstPath, &statbuf) != 0) {
            if (mkdir(pDstPath, 0755) != 0 && errno != EEXIST) {
                vdsp_err("cannot create folder[%s]\n", pDstPath);
                AX_SYS_MemFree(tSrcFrame.BufAddr[0].u64PhyAddr, (AX_VOID *)tSrcFrame.BufAddr[0].u64VirAddr);
                return -1;
            }
        } else {
            vdsp_debug("folder[%s] is exist\n", pDstPath);
        }

        for (i = 0; i < ration_num; i ++) {
            memset(&tDstFrame, 0, sizeof(tDstFrame));
            tDstFrame.width = resolution_ratio[i].u32Width;
            tDstFrame.height = resolution_ratio[i].u32Height;
            tDstFrame.stride  = tDstFrame.width;
            tDstFrame.u32Size = tDstFrame.stride * tDstFrame.height * 3 / 2;
            ret = AX_SYS_MemAlloc((AX_U64 *)&tDstFrame.BufAddr[0].u64PhyAddr, (AX_VOID **)&tDstFrame.BufAddr[0].u64VirAddr,
                                  tDstFrame.u32Size, SAMPLE_PHY_MEM_ALIGN_SIZE, NULL);
            if (0 != ret) {
                vdsp_err("%s AX_SYS_MemAlloc(0x%X) fail, ret=0x%x\n", __func__, tDstFrame.u32Size, ret);
                AX_SYS_MemFree(tSrcFrame.BufAddr[0].u64PhyAddr, (AX_VOID *)tSrcFrame.BufAddr[0].u64VirAddr);
                return -1;
            } else {
                tDstFrame.BufAddr[0].u32Size = tDstFrame.stride * tDstFrame.height;
                tDstFrame.BufAddr[1].u64PhyAddr = tDstFrame.BufAddr[0].u64PhyAddr + tDstFrame.BufAddr[0].u32Size;   // UV
                tDstFrame.BufAddr[1].u64VirAddr = tDstFrame.BufAddr[0].u64VirAddr + tDstFrame.BufAddr[0].u32Size;
                tDstFrame.BufAddr[1].u32Size = tDstFrame.stride * tDstFrame.height / 2;
                memset((AX_VOID *)tDstFrame.BufAddr[0].u64VirAddr, 0, tDstFrame.u32Size);
                vdsp_debug("dst %dx%d, stride: %d, phy: 0x%llX\n", tDstFrame.width, tDstFrame.height, tDstFrame.stride,
                           tDstFrame.BufAddr[0].u64PhyAddr);
            }
            param.src_height = tSrcFrame.height;
            param.src_width = tSrcFrame.width;
            param.src_stride = tSrcFrame.stride;
            param.dst_height = tDstFrame.height;
            param.dst_width = tDstFrame.width;
            param.dst_stride = tDstFrame.stride;
            param.inter_type = AX_DSP_CV_RESIZE_INNER_TYPE_BILINEAR;   //0|1|2
            param.format = AX_DSP_CV_FORMAT_NV12;

            vdsp_debug("src: %dx%d Y phy: 0x%llX, stride: %d, UV phy: 0x%llX, stride: %d\n", tSrcFrame.width, tSrcFrame.height,
                       tSrcFrame.BufAddr[0].u64PhyAddr, tSrcFrame.stride, tSrcFrame.BufAddr[1].u64PhyAddr, tSrcFrame.stride);
            vdsp_debug("dst: %dx%d Y phy: 0x%llX, stride: %d, UV phy: 0x%llX, stride: %d\n", tDstFrame.width, tDstFrame.height,
                       tDstFrame.BufAddr[0].u64PhyAddr, tDstFrame.stride, tDstFrame.BufAddr[1].u64PhyAddr, tDstFrame.stride);

            gettimeofday(&tv_start, NULL);
            ret = AX_DSP_CV_ResizeAll(AX_DSP_ID_0, &tSrcFrame.BufAddr[0], &tDstFrame.BufAddr[0], &param);
            gettimeofday(&tv_end, NULL);
            if (0 != ret) {
                vdsp_err("AX_DSP_Algo_Resize fail, ret=0x%x\n", ret);
                goto __END_OF_RESIZE;
            }
            printf("NV12 Resize %dx%d -> %dx%d, DSP cost time: tv_usec: %ld\n",
                   tSrcFrame.width, tSrcFrame.height, tDstFrame.width, tDstFrame.height,
                   (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);

            memset((AX_VOID *)pDstImgFile, 0, sizeof(pDstImgFile));
            sprintf(pDstImgFile, "%s/dst_%dx%d.yuv", pDstPath, tDstFrame.stride, tDstFrame.height);
            AX_SYS_MemFree(tDstFrame.BufAddr[0].u64PhyAddr, (AX_VOID *)tDstFrame.BufAddr[0].u64VirAddr);
        }
        AX_SYS_MemFree(tSrcFrame.BufAddr[0].u64PhyAddr, (AX_VOID *)tSrcFrame.BufAddr[0].u64VirAddr);
    }

    if (0 == ret) {
        printf("SAMPLE_DSP%d_TestResize PASS\n", dspid);
        AX_DSP_CV_Exit(dspid);
        return 0;
    }

__END_OF_RESIZE:
    AX_SYS_MemFree(tSrcFrame.BufAddr[0].u64PhyAddr, (AX_VOID *)tSrcFrame.BufAddr[0].u64VirAddr);
    AX_SYS_MemFree(tDstFrame.BufAddr[0].u64PhyAddr, (AX_VOID *)tDstFrame.BufAddr[0].u64VirAddr);

    AX_DSP_CV_Exit(dspid);

    return ret;
}
int SAMPLE_DSP_TestCvtColor(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc, phyDst;
    AX_VOID *vSrc, *vDst;
    AX_DSP_CV_CVTCOLOR_PARAM_T param;
    AX_S32 size, dstsize;
    struct timeval tv_start, tv_end;
    AX_MEM_INFO_T in_buf[3];
    AX_MEM_INFO_T out_buf[3];

    param.src_width = 3840;
    param.src_height = 2160;
    param.src_stride = param.src_stride_uv = 3840;
    param.dst_stride = param.src_width * 2;
    dstsize = param.dst_stride * param.src_height;
    size = param.src_stride * param.src_height + param.src_stride_uv * param.src_height / 2;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phySrc, &vSrc, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        AX_SYS_MemFree(phySrc, vSrc);
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
#ifdef USE_SAMPLE_FILE
    size = DSP_LoadPic("/opt/data/dsp/csc/3840x2160.nv12.yuv", vSrc);
    if (size < 0) {
        AX_SYS_MemFree(phySrc, vSrc);
        AX_SYS_MemFree(phyDst, vDst);
        printf("%s load pic error %d\n", __func__, size);
        return -1;
    }
    printf("/opt/data/dsp/csc/3840x2160.nv12.yuv loaded!\n");
#else
    unsigned char *ptr = (unsigned char *)vSrc;
    for (i = 0; i < param.src_width * param.src_height; i++)
        ptr[i] = 'Y';
    ptr += param.src_stride * param.src_height;
    for (i = 0; i < param.src_stride_uv * param.src_height / 2; i += 2) {
        ptr[i] = 'U';
        ptr[i + 1] = 'V';
    }
#endif

    in_buf[0].u64PhyAddr = phySrc;
    in_buf[1].u64PhyAddr = phySrc + param.src_stride * param.src_height;
    out_buf[0].u64PhyAddr = phyDst;

    srand(time(0));
    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst, 0, dstsize);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_CvtColor(dspid, AX_DSP_CV_CVTCOLOR_NV12_YUYV, in_buf, out_buf, &param);
        if (ret != 0) {
            AX_SYS_MemFree(phySrc, vSrc);
            AX_SYS_MemFree(phyDst, vDst);
            printf("%s PRC error %x\n", __func__, ret);
            return -1;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    DSP_SavePic("/opt/data/dsp/csc/3840x2160.yuyv.yuv", vDst, dstsize);
    printf("/opt/data/dsp/csc/3840x2160.yuyv.yuv saved!\n");

#ifndef USE_SAMPLE_FILE
    {
        int k;
        char *ptr = vDst;
        for (k = 0; k < dstsize; k += 4) {
            if (ptr[k] != 'Y' || ptr[k + 1] != 'U' || ptr[k + 2] != 'Y' || ptr[k + 3] != 'V') {
                printf("[%d]=%c\n", k, ptr[k]);
                break;
            }
        }
    }
#endif

    if (0 == ret) {
        printf("SAMPLE_DSP%d_TestCvtColor PASS\n", dspid);
    }

    AX_DSP_CV_Exit(dspid);

    AX_SYS_MemFree(phySrc, vSrc);
    AX_SYS_MemFree(phyDst, vDst);
    return 0;
}

//#define USE_SAMPLE_FILE_JOINTLR
int SAMPLE_DSP_jointLR(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc, phySrc2, phyDst;
    AX_VOID *vSrc, *vSrc2, *vDst;
    AX_DSP_CV_JOINTLR_PARAM_T param;
    AX_S32 size, dstsize;
    struct timeval tv_start, tv_end;

    param.width_l = 1920;
    param.width_r = 1920;
    param.height = 2160;
    param.src_stride_l = 3840;
    param.src_stride_r = 3840;
    param.dst_stride = 3840 * 2;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    size = param.src_stride_l * param.height;
    ret = AX_SYS_MemAlloc(&phySrc, &vSrc, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    size = param.src_stride_r * param.height;
    ret = AX_SYS_MemAlloc(&phySrc2, &vSrc2, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        AX_SYS_MemFree(phySrc, vSrc);
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    dstsize = param.dst_stride * param.height;
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        AX_SYS_MemFree(phySrc, vSrc);
        AX_SYS_MemFree(phySrc2, vSrc2);
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
#ifdef USE_SAMPLE_FILE_JOINTLR
    size = DSP_LoadPic("/opt/data/dsp/csc/1920x2160_a.yuv", vSrc);
    if (size < 0) {
        printf("%s load pic error %d\n", __func__, size);
        return -1;
    }
    printf("1920x2160_a.yuv loaded!\n");
    size = DSP_LoadPic("/opt/data/dsp/csc/1920x2160_b.yuv", vSrc2);
    if (size < 0) {
        printf("%s load pic error %d\n", __func__, size);
        return -1;
    }
    printf("1920x2160_b.yuv loaded!\n");
#else
    memset(vSrc, 0x1A, 1920 * 2160 * 2);
    memset(vSrc2, 0xc5, 1920 * 2160 * 2);
#endif

    param.phySrcL = phySrc;
    param.phySrcR = phySrc2;
    param.phyDst = phyDst;

    for (i = 0; i < 1; i++) {
        memset(vDst, 0, dstsize);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_JointLR(dspid, &param);
        if (ret != 0) {
            AX_SYS_MemFree(phySrc, vSrc);
            AX_SYS_MemFree(phySrc2, vSrc2);
            AX_SYS_MemFree(phyDst, vDst);
            AX_DSP_CV_Exit(dspid);
            printf("%s PRC error %x\n", __func__, ret);
            return -1;
        }
        gettimeofday(&tv_end, NULL);
        printf("JointLR MASK DSP cost time: tv_usec: %ld\n",
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
#ifdef USE_SAMPLE_FILE_JOINTLR
        DSP_SavePic("/opt/data/dsp/csc/3840x2160_uyvy.yuv", vDst, dstsize);
        printf("3840x2160_uyvy.yuv saved!\n");
#else
        printf("Waiting...\n");
        //COMPARE DATA
        {
            int j, k;
            unsigned char *ptr = vDst;
            for (k = 0; k < 2160; k++) {
                for (j = 0; j < 3840 * 2; j++) {
                    ptr = vDst + k * 3840 * 2 + j;
                    if (j < 3840 && *ptr != 0x1A) {
                        ret = -2;
                        break;
                    } else if (j >= 3840 && *ptr != 0xC5) {
                        ret = -2;
                        break;
                    }
                }
                if (ret)break;
            }
            if (!ret)
                printf("SAMPLE_DSP%d_jointLR PASS\n", dspid);
        }
#endif
    }
    AX_SYS_MemFree(phySrc, vSrc);
    AX_SYS_MemFree(phySrc2, vSrc2);
    AX_SYS_MemFree(phyDst, vDst);

    AX_DSP_CV_Exit(dspid);
    return 0;
}

//#define USE_SAMPLE_FILE_KVM
#ifdef USE_SAMPLE_FILE_KVM
int SAMPLE_DSP_TestCvtColor_KVM(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_U64 phyDst;
    AX_VOID *vDst;
    AX_DSP_CV_CVTCOLOR_PARAM_T param;
    AX_S32 size_y, dstsize;
    struct timeval tv_start, tv_end;
    AX_MEM_INFO_T in_buf[3];
    AX_MEM_INFO_T out_buf[3];
    AX_U64 phySrc_yu, phySrc_vu;
    AX_VOID *vSrc_yu, *vSrc_vu;

    param.src_width = 3840;
    param.src_height = 2160;
    param.src_stride = param.src_stride_uv = 3840;
    param.dst_stride = param.src_width * 4;
    dstsize = param.dst_stride * param.src_height;
    size_y = param.src_stride * param.src_height;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_yu, &vSrc_yu, size_y * 3 / 2, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_vu, &vSrc_vu, size_y * 3 / 2, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    size_y = DSP_LoadPic("/opt/data/dsp/csc/3840x2160_YU0.nv12", vSrc_yu);
    if (size_y < 0) {
        printf("%s load pic error %d\n", __func__, size_y);
        return -1;
    }
    printf("/opt/data/dsp/csc/3840x2160_YU0.nv12 loaded!\n");
    size_y = DSP_LoadPic("/opt/data/dsp/csc/3840x2160_VU1.nv12", vSrc_vu);
    if (size_y < 0) {
        printf("%s load pic error %d\n", __func__, size_y);
        return -1;
    }
    printf("/opt/data/dsp/csc/3840x2160_VU1.nv12 loaded!\n");

    size_y = param.src_stride * param.src_height;
    memset(vDst, 0, dstsize);
    {
        param.src_height /= 2; //=1080

        in_buf[0].u64PhyAddr = phySrc_yu;           //Y
        in_buf[1].u64PhyAddr = phySrc_yu + size_y;  //U
        in_buf[2].u64PhyAddr = phySrc_vu;           //V
        out_buf[0].u64PhyAddr = phyDst;             //RGB
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_CvtColor(dspid, AX_DSP_CV_CVTCOLOR_YUV444_RGBX, in_buf, out_buf, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            return -1;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);

        in_buf[0].u64PhyAddr = phySrc_yu + size_y / 2;  //Y
        in_buf[1].u64PhyAddr = phySrc_vu + size_y;      //U
        in_buf[2].u64PhyAddr = phySrc_vu + size_y / 2;  //V
        out_buf[0].u64PhyAddr = phyDst + size_y * 2;    //RGB
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_CvtColor(dspid, AX_DSP_CV_CVTCOLOR_YUV444_RGBX, in_buf, out_buf, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            return -1;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }
#define REF_C
#ifdef REF_C
    printf("Waiting...\n");
    {
        AX_U64 phyDst2;
        AX_VOID *vDst2;

        ret = AX_SYS_MemAlloc(&phyDst2, &vDst2, dstsize, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            return -1;
        }

        int color_r, color_g, color_b;
        unsigned char *ptr = (unsigned char *)vDst2;
        unsigned char *ptry, *ptru, *ptrv;
        int i;

        ptry = (unsigned char *)vSrc_yu;
        ptru = (unsigned char *)vSrc_yu + size_y;
        ptrv = (unsigned char *)vSrc_vu;

        for (i = 0; i < size_y / 2; i++) {
            color_r = (int)((int)ptry[i] + 1.5748f * ((int)ptrv[i] - 128.0f) + 0.5f);
            color_g = (int)((int)ptry[i] - 0.1873f * ((int)ptru[i] - 128.0f) - 0.4681f * ((int)ptrv[i] - 128.0f) + 0.5f);
            color_b = (int)((int)ptry[i] + 1.8556f * ((int)ptru[i] - 128.0f) + 0.5f);
            ptr[4 * i] = color_r;
            ptr[4 * i + 1] = color_g;
            ptr[4 * i + 2] = color_b;
            ptr[4 * i + 3] = 255;
        }

        ptry = (unsigned char *)vSrc_yu + size_y / 2;
        ptru = (unsigned char *)vSrc_vu + size_y;
        ptrv = (unsigned char *)vSrc_vu + size_y / 2;
        ptr = vDst2 + size_y * 2;

        for (i = 0; i < size_y / 2; i++) {
            color_r = (int)((int)ptry[i] + 1.5748f * ((int)ptrv[i] - 128.0f) + 0.5f);
            color_g = (int)((int)ptry[i] - 0.1873f * ((int)ptru[i] - 128.0f) - 0.4681f * ((int)ptrv[i] - 128.0f) + 0.5f);
            color_b = (int)((int)ptry[i] + 1.8556f * ((int)ptru[i] - 128.0f) + 0.5f);
            ptr[4 * i] = color_r;
            ptr[4 * i + 1] = color_g;
            ptr[4 * i + 2] = color_b;
            ptr[4 * i + 3] = 255;
        }

        //COMPARE DATA
        ptr = vDst;
        ptry = vDst2;
        for (i = 0; i < dstsize; i++) {
            if (abs(ptr[i] - ptry[i]) > 1) {
                printf("[%6d]%d = %3d, should be %3d\n", i, i % 4, ptr[i], ptry[i]);
                {
                    int j = (i / 4) * 4;
                    printf("RGB:");
                    printf("%d,", ptr[j]);
                    printf("%d,", ptr[j + 1]);
                    printf("%d,", ptr[j + 2]);
                    printf("%d\n", ptr[j + 3]);
                    printf("YUV:");
                    if (j < size_y * 2) {
                        ptry = (unsigned char *)vSrc_yu;
                        ptru = (unsigned char *)vSrc_yu + size_y;
                        ptrv = (unsigned char *)vSrc_vu;
                    } else {
                        ptry = (unsigned char *)vSrc_yu + size_y / 2;
                        ptru = (unsigned char *)vSrc_vu + size_y;
                        ptrv = (unsigned char *)vSrc_vu + size_y / 2;
                    }
                    printf("%d,", ptry[(j % (size_y * 2)) / 4]);
                    printf("%d,", ptru[(j % (size_y * 2)) / 4]);
                    printf("%d\n", ptrv[(j % (size_y * 2)) / 4]);
                }
                break;
            }
        }
        AX_SYS_MemFree(phyDst2, vDst2);
    }
#endif

    AX_DSP_CV_Exit(dspid);

    if (0 == ret) {
        DSP_SavePic("/opt/data/dsp/csc/3840x2160.rgb", vDst, dstsize);
        printf("/opt/data/dsp/csc/3840x2160.rgb saved!\n");
        printf("SAMPLE_DSP%d_TestCvtColor_KVM PASS\n", dspid);
    }

    AX_SYS_MemFree(phySrc_yu, vSrc_yu);
    AX_SYS_MemFree(phySrc_vu, vSrc_vu);
    AX_SYS_MemFree(phyDst, vDst);
    return ret;
}
#else
int SAMPLE_DSP_TestCvtColor_KVM(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc_y, phySrc_u, phySrc_v, phyDst;
    AX_VOID *vSrc_y, *vSrc_u, *vSrc_v, *vDst;
    AX_DSP_CV_CVTCOLOR_PARAM_T param;
    AX_S32 size_y, size_u, size_v, dstsize;
    struct timeval tv_start, tv_end;
    AX_MEM_INFO_T in_buf[3];
    AX_MEM_INFO_T out_buf[3];

    param.src_width = 3840 + 16;
    param.src_height = 2160;
    param.src_stride = param.src_stride_uv = 3840 + 16;
    param.dst_stride = param.src_width * 4;
    dstsize = param.dst_stride * param.src_height;
    size_y = param.src_stride * param.src_height;
    size_u = param.src_stride_uv * param.src_height;
    size_v = param.src_stride_uv * param.src_height;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_y, &vSrc_y, size_y, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_u, &vSrc_u, size_u, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_v, &vSrc_v, size_v, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    unsigned char *ptr = (unsigned char *)vSrc_y;
    for (i = 0; i < param.src_stride * param.src_height; i++)
        ptr[i] = 'Y';
    ptr = (unsigned char *)vSrc_u;
    for (i = 0; i < param.src_stride_uv * param.src_height; i++)
        ptr[i] = 'U';
    ptr = (unsigned char *)vSrc_v;
    for (i = 0; i < param.src_stride_uv * param.src_height; i++)
        ptr[i] = 'V';
    int color_r = (int)((int)'Y' + 1.5748f * ((int)'V' - 128.0f) + 0.5f);
    int color_g = (int)((int)'Y' - 0.1873f * ((int)'U' - 128.0f) - 0.4681f * ((int)'V' - 128.0f) + 0.5f);
    int color_b = (int)((int)'Y' + 1.8556f * ((int)'U' - 128.0f) + 0.5f);
    printf("r,g,b = 0x%x, 0x%x, 0x%x\n", color_r, color_g, color_b);

    in_buf[0].u64PhyAddr = phySrc_y;
    in_buf[1].u64PhyAddr = phySrc_u;
    in_buf[2].u64PhyAddr = phySrc_v;
    out_buf[0].u64PhyAddr = phyDst;

    srand(time(0));
    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst, 0, dstsize);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_CvtColor(dspid, AX_DSP_CV_CVTCOLOR_YUV444_RGBX, in_buf, out_buf, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            return -1;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    AX_DSP_CV_Exit(dspid);

    printf("Waiting...\n");
    //COMPARE DATA
    ptr = vDst;
    for (i = 0; i < dstsize; i += 4) {
        if (abs(ptr[i] - color_r) > 0) {
            printf("[%6d]%d = %3d, %3d\n", i, i % 4, ptr[i], color_r);
            ret = -1;
        } else if (abs(ptr[i + 1] - color_g) > 0) {
            printf("[%6d]%d = %3d, %3d\n", i, i % 4, ptr[i + 1], color_g);
            ret = -1;
        } else if (abs(ptr[i + 2] - color_b) > 0) {
            printf("[%6d]%d = %3d, %3d\n", i, i % 4, ptr[i + 2], color_b);
            ret = -1;
        } else if (abs(ptr[i + 3] - 255) > 0) {
            printf("[%6d]%d = %3d, %3d\n", i, i % 4, ptr[i + 3], 255);
            ret = -1;
        }
    }

    if (0 == ret) {
        printf("SAMPLE_DSP%d_TestCvtColor_KVM PASS\n", dspid);
    }

    AX_SYS_MemFree(phySrc_y, vSrc_y);
    AX_SYS_MemFree(phySrc_u, vSrc_u);
    AX_SYS_MemFree(phySrc_v, vSrc_v);
    AX_SYS_MemFree(phyDst, vDst);
    return ret;
}
#endif
//Window size: 2*n+1
void SAD(unsigned char *Limg, unsigned char *Rimg, unsigned char *Oimg, int w, int h, int n, int range)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned int bestCost = 999999;
            unsigned int bestDisparity = 0;
            for (int d = 0; d <= range; d++) {
                unsigned int cost = 0;
                for (int i = -n; i <= n; i++) {
                    for (int j = -n; j <= n; j++) {
                        int yy, xx, xxd;
                        yy = y + i;
                        if (yy < 0) yy = 0;
                        if (yy >= h) yy = h - 1;

                        xx = x + j;
                        if (xx < 0) xx = 0;
                        if (xx >= w) xx = w - 1;

                        xxd = xx - d;
                        if (xxd < 0) xxd = 0;
                        if (xxd >= w) xxd = w - 1;
                        cost += abs((int)(Limg[yy * w + xx] - Rimg[yy * w + xxd]));
                    }
                }
                if (cost < bestCost) {
                    bestCost = cost;
                    bestDisparity = d;
                }
                Oimg[y * w + x] = bestDisparity; //*4?
            }
        }
    }
}
int SAMPLE_DSP_TestSAD(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc_l, phySrc_r, phyDst;
    AX_VOID *vSrc_l, *vSrc_r, *vDst;
    AX_DSP_CV_SAD_PARAM_T param;
    AX_S32 size_l, size_r, dstsize;
    struct timeval tv_start, tv_end;
    int range = 10;

    param.width = 1280;
    param.height = 720;
    param.src_stride_l = param.src_stride_r = param.dst_stride = 1280;
    dstsize = param.dst_stride * param.height;
    size_l = param.src_stride_l * param.height;
    size_r = param.src_stride_r * param.height;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAllocCached(&phySrc_l, &vSrc_l, size_l, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext0;
    }
    ret = AX_SYS_MemAllocCached(&phySrc_r, &vSrc_r, size_r, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext2;
    }
    {
        int j = 0;
        unsigned char *ptr = vSrc_l;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.width; j++)
                ptr[i * param.src_stride_l + j] = rand() % 256; //j % 256;  //
        ptr = vSrc_r;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.width; j++)
                ptr[i * param.src_stride_r + j] = rand() % 256; //(j + 6) % 256;   //
        AX_SYS_MflushCache(phySrc_l, vSrc_l, size_l);
        AX_SYS_MflushCache(phySrc_r, vSrc_r, size_r);
    }

    param.phySrcL = phySrc_l;
    param.phySrcR = phySrc_r;
    param.phyDst = phyDst;
    param.kernel_size = 7;  //up to 9
    param.range = range;

    srand(time(0));
    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst, 0, dstsize);
        AX_SYS_MflushCache(phyDst, vDst, dstsize);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_SAD(dspid, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            goto ext3;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    {
        AX_U64 phy2;
        AX_VOID *virt2;
        ret = AX_SYS_MemAllocCached(&phy2, &virt2, dstsize, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            goto ext3;
        }
        printf("Wait................\n");
        SAD(vSrc_l, vSrc_r, virt2, param.width, param.height, param.kernel_size / 2, range);

        //COMPARE_DATA
        unsigned char *ptr = vDst;
        unsigned char *ptr2 = virt2;
        ret = 0;
        for (i = 0; i < param.height; i++)
            for (int j = 0; j < param.width - param.kernel_size / 2; j++) {
                ptr = vDst + i * param.dst_stride + j;
                ptr2 = virt2 + i * param.dst_stride + j;
                if (abs(*ptr - *ptr2) > 0) {
                    printf("(%3d, %3d) = %3d, %3d\n", i, j, *ptr, *ptr2);
                    ret++;
                    if (ret > 64)break;
                }
            }

        AX_SYS_MflushCache(phy2, virt2, dstsize);
        //save data if needed
//        DSP_SavePic("/opt/data/dsp/comp.gray", virt2, dstsize);
//        printf("/opt/data/dsp/comp.gray saved.\n");
        AX_SYS_MemFree(phy2, virt2);
    }

    //save data if needed
//    DSP_SavePic("/opt/data/dsp/512x512.gray", vDst, dstsize);
//    printf("/opt/data/dsp/512x512.gray saved.\n");

    if (0 == ret) {
        printf("SAMPLE_DSP%d_SAD PASS\n", dspid);
    }

ext3:
    AX_SYS_MemFree(phyDst, vDst);
ext2:
    AX_SYS_MemFree(phySrc_r, vSrc_r);
ext1:
    AX_SYS_MemFree(phySrc_l, vSrc_l);
ext0:
    AX_DSP_CV_Exit(dspid);

    return ret;
}
static float clampf(float value, float min, float max)
{
    if (value < min) {
        value = min;
    }
    if (value > max) {
        value = max;
    }
    return value;
}
int SAMPLE_DSP_KVM_SPLIT(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc, phyDst_l, phyDst_r;
    AX_VOID *vSrc, *vDst_l, *vDst_r;
    AX_DSP_CV_SPLIT_YUV444_PARAM_T param;
    AX_S32 size, size_l, size_r;
    struct timeval tv_start, tv_end;
    AX_U64 pd1, pd2;
    AX_VOID *vd1, *vd2;

    param.width = 3840;
    param.height = 2160;
    param.src_stride = 3840 * 3;
    param.dst_stride_l = 3840;
    param.dst_stride_r = 3840;
    size = param.src_stride * param.height;
    size_l = param.dst_stride_l * param.height * 3 / 2;
    size_r = param.dst_stride_r * param.height * 3 / 2;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        ret = -1;
        goto ext0;
    }
    ret = AX_SYS_MemAlloc(&phySrc, &vSrc, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phyDst_l, &vDst_l, size_l, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext2;
    }
    ret = AX_SYS_MemAlloc(&phyDst_r, &vDst_r, size_r, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext3;
    }

    param.phySrc = phySrc;
    param.phyDstL = phyDst_l;
    param.phyDstR = phyDst_r;
    param.para = 3;

    unsigned char *ptr = (unsigned char *)vSrc;
    srand(time(0));
    for (i = 0; i < param.src_stride * param.height; i += 3) {
        ptr[i] = rand() % 256;
        ptr[i + 1] = rand() % 256;
        ptr[i + 2] = rand() % 256;
    }

    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst_l, 0, size_l);
        memset(vDst_r, 0, size_r);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_SPLIT_YUV444(dspid, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            ret = -1;
            goto ext4;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    printf("Waiting...\n");
    {
        ret = AX_SYS_MemAlloc(&pd1, &vd1, size_l, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            ret = -1;
            goto ext5;
        }
        ret = AX_SYS_MemAlloc(&pd2, &vd2, size_r, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            ret = -1;
            goto ext6;
        }
        unsigned char *src_rgb_ptr, *dst_y_ptr, *dst_u_ptr, *dst_v_ptr;
        int v_stride;
        src_rgb_ptr = vSrc;
        dst_y_ptr = vd1;
        dst_u_ptr = vd2;
        for (int iy = 0; iy < param.height; iy++) {
            if (iy % 2 == 0) {
                dst_v_ptr = dst_y_ptr + param.dst_stride_l * param.height;
                v_stride = param.dst_stride_l;
            } else {
                dst_v_ptr = dst_u_ptr + param.dst_stride_r * param.height;
                v_stride = param.dst_stride_r;
            }

            for (int ix = 0; ix < param.width; ix++) {
                int r = src_rgb_ptr[iy * param.src_stride + 3 * ix + 0];
                int g = src_rgb_ptr[iy * param.src_stride + 3 * ix + 1];
                int b = src_rgb_ptr[iy * param.src_stride + 3 * ix + 2];
                if (param.para < 3) {
                    float color_y = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                    float color_u = (b - color_y) * 0.5389f + 128.0f;
                    float color_v = (r - color_y) * 0.6350f + 128.0f;

                    color_u = clampf(color_u, 0.0f, 255.0f);
                    color_v = clampf(color_v, 0.0f, 255.0f);

                    dst_y_ptr[iy * param.dst_stride_l + ix] = (int)(color_y + 0.5f);
                    dst_u_ptr[iy * param.dst_stride_r + ix] = (int)(color_u);
                    dst_v_ptr[(iy / 2) * v_stride + ix] = (int)(color_v);
                } else {
                    dst_y_ptr[iy * param.dst_stride_l + ix] = r;
                    dst_u_ptr[iy * param.dst_stride_r + ix] = g;
                    dst_v_ptr[(iy / 2) * v_stride + ix] = b;
                }
            }
        }
    }

    //COMPARE DATA
    ptr = vDst_l;
    unsigned char *ptr2 = vd1;
    ret = 0;
    for (i = 0; i < size_l; i += 1) {
        if (abs(ptr[i] - ptr2[i]) > 1) {
            printf("L(%3d, %3d) = %3d, %3d\n", i % param.dst_stride_l, i / param.dst_stride_l, ptr[i], ptr2[i]);
            ret++;
            if (ret > 16)break;
        }
    }
    ptr = vDst_r;
    ptr2 = vd2;
    ret = 0;
    for (i = 0; i < size_r; i += 1) {
        if (abs(ptr[i] - ptr2[i]) > 1) {
            printf("R(%3d, %3d) = %3d, %3d\n", i % param.dst_stride_r, i / param.dst_stride_r, ptr[i], ptr2[i]);
            ret++;
            if (ret > 16)break;
        }
    }

    if (0 == ret) {
        printf("SAMPLE_DSP%d_KVM_SPLIT PASS\n", dspid);
    }

ext6:
    AX_SYS_MemFree(pd2, vd2);
ext5:
    AX_SYS_MemFree(pd1, vd1);
ext4:
    AX_SYS_MemFree(phyDst_r, vDst_r);
ext3:
    AX_SYS_MemFree(phyDst_l, vDst_l);
ext2:
    AX_SYS_MemFree(phySrc, vSrc);
ext1:
    AX_DSP_CV_Exit(dspid);
ext0:
    return ret;
}
int SAMPLE_DSP_KVM_SPLIT_2()
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc, phyDst_l, phyDst_r;
    AX_VOID *vSrc, *vDst_l, *vDst_r;
    AX_DSP_CV_SPLIT_YUV444_PARAM_T param;
    AX_S32 size, size_l, size_r;
    struct timeval tv_start, tv_end;
    AX_U64 pd1, pd2;
    AX_VOID *vd1, *vd2;

    param.width = 3840;
    param.height = 2160;
    param.src_stride = 3840 * 3;
    param.dst_stride_l = 3840;
    param.dst_stride_r = 3840;
    size = param.src_stride * param.height;
    size_l = param.dst_stride_l * param.height * 3 / 2;
    size_r = param.dst_stride_r * param.height * 3 / 2;

    ret = AX_DSP_CV_Init(0);
    ret += AX_DSP_CV_Init(1);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        ret = -1;
        goto ext0;
    }
    ret = AX_SYS_MemAlloc(&phySrc, &vSrc, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phyDst_l, &vDst_l, size_l, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext2;
    }
    ret = AX_SYS_MemAlloc(&phyDst_r, &vDst_r, size_r, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext3;
    }

    param.phySrc = phySrc;
    param.phyDstL = phyDst_l;
    param.phyDstR = phyDst_r;

    unsigned char *ptr = (unsigned char *)vSrc;
    srand(time(0));
    for (i = 0; i < param.src_stride * param.height; i += 3) {
        ptr[i] = rand() % 256;
        ptr[i + 1] = rand() % 256;
        ptr[i + 2] = rand() % 256;
    }

    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst_l, 0, size_l);
        memset(vDst_r, 0, size_r);
        param.para = 1;
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_SPLIT_YUV444_2(&param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            ret = -1;
            goto ext4;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    printf("Waiting...\n");
    {
        ret = AX_SYS_MemAlloc(&pd1, &vd1, size_l, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            ret = -1;
            goto ext5;
        }
        ret = AX_SYS_MemAlloc(&pd2, &vd2, size_r, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            ret = -1;
            goto ext6;
        }
        unsigned char *src_rgb_ptr, *dst_y_ptr, *dst_u_ptr, *dst_v_ptr;
        int v_stride;
        src_rgb_ptr = vSrc;
        dst_y_ptr = vd1;
        dst_u_ptr = vd2;
        for (int iy = 0; iy < param.height; iy++) {
            if (iy % 2 == 0) {
                dst_v_ptr = dst_y_ptr + param.dst_stride_l * param.height;
                v_stride = param.dst_stride_l;
            } else {
                dst_v_ptr = dst_u_ptr + param.dst_stride_r * param.height;
                v_stride = param.dst_stride_r;
            }

            for (int ix = 0; ix < param.width; ix++) {
                int r = src_rgb_ptr[iy * param.src_stride + 3 * ix + 0];
                int g = src_rgb_ptr[iy * param.src_stride + 3 * ix + 1];
                int b = src_rgb_ptr[iy * param.src_stride + 3 * ix + 2];
                if (param.para < 3) {
                    float color_y = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                    float color_u = (b - color_y) * 0.5389f + 128.0f;
                    float color_v = (r - color_y) * 0.6350f + 128.0f;

                    color_u = clampf(color_u, 0.0f, 255.0f);
                    color_v = clampf(color_v, 0.0f, 255.0f);

                    dst_y_ptr[iy * param.dst_stride_l + ix] = (int)(color_y + 0.5f);
                    dst_u_ptr[iy * param.dst_stride_r + ix] = (int)(color_u);
                    dst_v_ptr[(iy / 2) * v_stride + ix] = (int)(color_v);
                } else {
                    dst_y_ptr[iy * param.dst_stride_l + ix] = r;
                    dst_u_ptr[iy * param.dst_stride_r + ix] = g;
                    dst_v_ptr[(iy / 2) * v_stride + ix] = b;
                }
            }
        }
    }

    //COMPARE DATA
    ptr = vDst_l;
    unsigned char *ptr2 = vd1;
    ret = 0;
    for (i = 0; i < size_l; i += 1) {
        if (abs(ptr[i] - ptr2[i]) > 1) {
            printf("L(%3d, %3d) = %3d, %3d\n", i % param.dst_stride_l, i / param.dst_stride_l, ptr[i], ptr2[i]);
            ret++;
            if (ret > 16)break;
        }
    }
    ptr = vDst_r;
    ptr2 = vd2;
    ret = 0;
    for (i = 0; i < size_r; i += 1) {
        if (abs(ptr[i] - ptr2[i]) > 1) {
            printf("R(%3d, %3d) = %3d, %3d\n", i % param.dst_stride_r, i / param.dst_stride_r, ptr[i], ptr2[i]);
            ret++;
            if (ret > 16)break;
        }
    }

    if (0 == ret) {
        printf("SAMPLE_DSP_KVM_SPLIT_2 PASS\n");
    }

ext6:
    AX_SYS_MemFree(pd2, vd2);
ext5:
    AX_SYS_MemFree(pd1, vd1);
ext4:
    AX_SYS_MemFree(phyDst_r, vDst_r);
ext3:
    AX_SYS_MemFree(phyDst_l, vDst_l);
ext2:
    AX_SYS_MemFree(phySrc, vSrc);
ext1:
    AX_DSP_CV_Exit(1);
ext0:
    AX_DSP_CV_Exit(0);
    return ret;
}

int SAMPLE_DSP_KVM_COMBINE(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phyDst, phySrc_l, phySrc_r;
    AX_VOID *vDst, *vSrc_l, *vSrc_r;
    AX_DSP_CV_SPLIT_YUV444_PARAM_T param;
    AX_S32 size, size_l, size_r;
    struct timeval tv_start, tv_end;
    AX_U64 pd1;
    AX_VOID *vd1;

    param.width = 3840;
    param.height = 2160;
    param.src_stride = 3840 * 3;
    param.dst_stride_l = 3840;
    param.dst_stride_r = 3840;
    size = param.src_stride * param.height;
    size_l = param.dst_stride_l * param.height * 3 / 2;
    size_r = param.dst_stride_r * param.height * 3 / 2;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        ret = -1;
        goto ext0;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_l, &vSrc_l, size_l, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext2;
    }
    ret = AX_SYS_MemAlloc(&phySrc_r, &vSrc_r, size_r, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext3;
    }

    param.phySrc = phyDst;
    param.phyDstL = phySrc_l;
    param.phyDstR = phySrc_r;
    param.para = 0;

    unsigned char *ptr = (unsigned char *)vSrc_l;
    srand(time(0));
    for (i = 0; i < param.dst_stride_l * param.height; i++) {
        ptr[i] = rand() % 256;
    }
    ptr = (unsigned char *)vSrc_r;
    for (i = 0; i < param.dst_stride_r * param.height; i++) {
        ptr[i] = rand() % 256;
    }

    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst, 0, size);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_COMBINE_YUV444(dspid, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            ret = -1;
            goto ext4;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    printf("Waiting...\n");
    {
        ret = AX_SYS_MemAlloc(&pd1, &vd1, size, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            ret = -1;
            goto ext5;
        }
        unsigned char *src_y_ptr, *src_u_ptr, *src_v_ptr, *dst_rgb_ptr;
        int v_stride;
        src_y_ptr = vSrc_l;
        src_u_ptr = vSrc_r;
        dst_rgb_ptr = vd1;
        for (int iy = 0; iy < param.height; iy++) {
            if (iy % 2 == 0) {
                src_v_ptr = src_y_ptr + param.dst_stride_l * param.height;
                v_stride = param.dst_stride_l;
            } else {
                src_v_ptr = src_u_ptr + param.dst_stride_r * param.height;
                v_stride = param.dst_stride_r;
            }
            for (int ix = 0; ix < param.width; ix++) {
                int color_y = src_y_ptr[iy     * param.dst_stride_l + ix];
                int color_u = src_u_ptr[iy     * param.dst_stride_r + ix];
                int color_v = src_v_ptr[iy / 2 * v_stride + ix];

                int color_r = (int)(color_y + 1.5748f * (color_v - 128.0f) + 0.5f);
                int color_g = (int)(color_y - 0.1873f * (color_u - 128.0f) - 0.4681f * (color_v - 128.0f) + 0.5f);
                int color_b = (int)(color_y + 1.8556f * (color_u - 128.0f) + 0.5f);

                color_r = clampf(color_r, 0, 255);
                color_g = clampf(color_g, 0, 255);
                color_b = clampf(color_b, 0, 255);

                dst_rgb_ptr[iy * param.src_stride + 3 * ix + 0] = color_r;
                dst_rgb_ptr[iy * param.src_stride + 3 * ix + 1] = color_g;
                dst_rgb_ptr[iy * param.src_stride + 3 * ix + 2] = color_b;
            }
        }
    }

    //COMPARE DATA
    ptr = vDst;
    unsigned char *ptr2 = vd1;
    ret = 0;
    for (i = 0; i < size; i += 1) {
        if (abs(ptr[i] - ptr2[i]) > 1) {
            printf("L(%3d, %3d) = %3d, %3d\n", i / param.src_stride, i % param.src_stride, ptr[i], ptr2[i]);
            ret++;
            if (ret > 16)break;
        }
    }

    if (0 == ret) {
        printf("SAMPLE_DSP%d_KVM_COMBINE PASS\n", dspid);
    }

ext5:
    AX_SYS_MemFree(pd1, vd1);
ext4:
    AX_SYS_MemFree(phySrc_r, vSrc_r);
ext3:
    AX_SYS_MemFree(phySrc_l, vSrc_l);
ext2:
    AX_SYS_MemFree(phyDst, vDst);
ext1:
    AX_DSP_CV_Exit(dspid);
ext0:
    return ret;
}
int SAMPLE_DSP_KVM_COMBINE_2()
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phyDst, phySrc_l, phySrc_r;
    AX_VOID *vDst, *vSrc_l, *vSrc_r;
    AX_DSP_CV_SPLIT_YUV444_PARAM_T param;
    AX_S32 size, size_l, size_r;
    struct timeval tv_start, tv_end;
    AX_U64 pd1;
    AX_VOID *vd1;

    param.width = 3840;
    param.height = 2160;
    param.src_stride = 3840 * 3;
    param.dst_stride_l = 3840;
    param.dst_stride_r = 3840;
    size = param.src_stride * param.height;
    size_l = param.dst_stride_l * param.height * 3 / 2;
    size_r = param.dst_stride_r * param.height * 3 / 2;

    ret = AX_DSP_CV_Init(0);
    ret += AX_DSP_CV_Init(1);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        ret = -1;
        goto ext0;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phySrc_l, &vSrc_l, size_l, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext2;
    }
    ret = AX_SYS_MemAlloc(&phySrc_r, &vSrc_r, size_r, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        ret = -1;
        goto ext3;
    }

    param.phySrc = phyDst;
    param.phyDstL = phySrc_l;
    param.phyDstR = phySrc_r;

    unsigned char *ptr = (unsigned char *)vSrc_l;
    srand(time(0));
    for (i = 0; i < param.dst_stride_l * param.height; i++) {
        ptr[i] = rand() % 256;
    }
    ptr = (unsigned char *)vSrc_r;
    for (i = 0; i < param.dst_stride_r * param.height; i++) {
        ptr[i] = rand() % 256;
    }

    for (i = 0; i < 1 + rand(); i++) {
        memset(vDst, 0, size);
        gettimeofday(&tv_start, NULL);
        param.para = 1;
        ret = AX_DSP_CV_COMBINE_YUV444_2(&param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            ret = -1;
            goto ext4;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    printf("Waiting...\n");
    {
        ret = AX_SYS_MemAlloc(&pd1, &vd1, size, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            ret = -1;
            goto ext5;
        }
        unsigned char *src_y_ptr, *src_u_ptr, *src_v_ptr, *dst_rgb_ptr;
        int v_stride;
        src_y_ptr = vSrc_l;
        src_u_ptr = vSrc_r;
        dst_rgb_ptr = vd1;
        for (int iy = 0; iy < param.height; iy++) {
            if (iy % 2 == 0) {
                src_v_ptr = src_y_ptr + param.dst_stride_l * param.height;
                v_stride = param.dst_stride_l;
            } else {
                src_v_ptr = src_u_ptr + param.dst_stride_r * param.height;
                v_stride = param.dst_stride_r;
            }
            for (int ix = 0; ix < param.width; ix++) {
                int color_y = src_y_ptr[iy     * param.dst_stride_l + ix];
                int color_u = src_u_ptr[iy     * param.dst_stride_r + ix];
                int color_v = src_v_ptr[iy / 2 * v_stride + ix];

                int color_r = (int)(color_y + 1.5748f * (color_v - 128.0f) + 0.5f);
                int color_g = (int)(color_y - 0.1873f * (color_u - 128.0f) - 0.4681f * (color_v - 128.0f) + 0.5f);
                int color_b = (int)(color_y + 1.8556f * (color_u - 128.0f) + 0.5f);

                color_r = clampf(color_r, 0, 255);
                color_g = clampf(color_g, 0, 255);
                color_b = clampf(color_b, 0, 255);

                dst_rgb_ptr[iy * param.src_stride + 3 * ix + 0] = color_r;
                dst_rgb_ptr[iy * param.src_stride + 3 * ix + 1] = color_g;
                dst_rgb_ptr[iy * param.src_stride + 3 * ix + 2] = color_b;
            }
        }
    }

    //COMPARE DATA
    ptr = vDst;
    unsigned char *ptr2 = vd1;
    ret = 0;
    for (i = 0; i < size; i += 1) {
        if (abs(ptr[i] - ptr2[i]) > 1) {
            printf("L(%3d, %3d) = %3d, %3d\n", i / param.src_stride, i % param.src_stride, ptr[i], ptr2[i]);
            ret++;
            if (ret > 16)break;
        }
    }

    if (0 == ret) {
        printf("SAMPLE_DSP_KVM_COMBINE_2 PASS\n");
    }

ext5:
    AX_SYS_MemFree(pd1, vd1);
ext4:
    AX_SYS_MemFree(phySrc_r, vSrc_r);
ext3:
    AX_SYS_MemFree(phySrc_l, vSrc_l);
ext2:
    AX_SYS_MemFree(phyDst, vDst);
ext1:
    AX_DSP_CV_Exit(1);
ext0:
    AX_DSP_CV_Exit(0);
    return ret;
}
int SAMPLE_DSP_TestMAP(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i, j;
    AX_U64 phySrc, phyMap, phyDst;
    AX_VOID *vSrc, *vMap, *vDst;
    AX_DSP_CV_MAP_PARAM_T param;
    AX_S32 size, mapsize, dstsize;
    struct timeval tv_start, tv_end;

    param.width = 3840;
    param.height = 2160;
    param.src_stride = param.dst_stride = 3840;
    size = param.src_stride * param.height;
    mapsize = 256;
    dstsize = param.dst_stride * param.height;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAllocCached(&phySrc, &vSrc, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext0;
    }
    ret = AX_SYS_MemAllocCached(&phyMap, &vMap, mapsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext2;
    }
    {
        unsigned char *ptr = vSrc;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.width; j++)
                ptr[i * param.src_stride + j] = rand() % 256;
        ptr = vMap;
        for (i = 0; i < mapsize; i++)
            ptr[i] = rand() % 256;
        AX_SYS_MflushCache(phySrc, vSrc, size);
        AX_SYS_MflushCache(phyMap, vMap, mapsize);
    }

    param.phySrc = phySrc;
    param.phyMap = phyMap;
    param.phyDst = phyDst;

    srand(time(0));
    for (i = 0; i < 1 + rand() % 10; i++) {
        memset(vDst, 0, dstsize);
        AX_SYS_MflushCache(phyDst, vDst, dstsize);
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_MAP(dspid, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            goto ext3;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    {
        AX_U64 phy2;
        AX_VOID *virt2;
        ret = AX_SYS_MemAllocCached(&phy2, &virt2, dstsize, 128, (AX_S8 *)"dsp_test");
        if (ret < 0) {
            printf("%s alloc failed %x\n", __func__, ret);
            goto ext3;
        }
        printf("Wait................\n");
        {
            char *src_ptr = vSrc;
            char *map_ptr = vMap;
            char *dst_ptr = virt2;

            for (i = 0; i < param.height; i++) {
                for (j = 0; j < param.width; j++) {
                    unsigned int id = (unsigned int)src_ptr[i * param.src_stride + j];
                    dst_ptr[i * param.dst_stride + j] = map_ptr[id];
                }
            }
        }

        //COMPARE_DATA
        unsigned char *ptr = vDst;
        unsigned char *ptr2 = virt2;
        ret = 0;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.width; j++) {
                ptr = vDst + i * param.dst_stride + j;
                ptr2 = virt2 + i * param.dst_stride + j;
                if (abs(*ptr - *ptr2) > 0) {
                    printf("(%3d, %3d) = %3d, %3d\n", i, j, *ptr, *ptr2);
                    ret++;
                    if (ret > 64)break;
                }
            }
        AX_SYS_MemFree(phy2, virt2);
    }

    if (0 == ret) {
        printf("SAMPLE_DSP%d_MAP PASS\n", dspid);
    }

ext3:
    AX_SYS_MemFree(phyDst, vDst);
ext2:
    AX_SYS_MemFree(phyMap, vMap);
ext1:
    AX_SYS_MemFree(phySrc, vSrc);
ext0:
    AX_DSP_CV_Exit(dspid);

    return ret;
}
int SAMPLE_DSP_TestNV12COPY(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i, j;
    AX_U64 phySrc, phyDst;
    AX_VOID *vSrc, *vDst;
    AX_DSP_CV_NV12COPY_PARAM_T param;
    AX_S32 size, dstsize;
    struct timeval tv_start, tv_end;

    param.src_height = 720;
    param.src_stride = 1280;
    param.dst_width = 3840;
    param.dst_height = 2160;
    param.dst_stride = 3840;
    param.src_x = 0;
    param.src_y = 0;
    param.win_w = 1280;
    param.win_h = 720;
    param.dst_x = 1920;
    param.dst_y = 100;
    size = param.src_stride * param.src_height * 3 / 2;
    dstsize = param.dst_stride * param.dst_height * 3 / 2;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAllocCached(&phySrc, &vSrc, size, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext0;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext2;
    }
    {
        unsigned char *ptr = vSrc;
        for (i = 0; i < param.win_h; i++)
            for (j = 0; j < param.win_w; j++) {
                ptr[i * param.src_stride + j] = 80;
                ptr[param.src_stride * (param.src_height + i / 2) + j] = 80 * ((j + 1) % 2);
            }
        ptr = vDst;
        for (i = 0; i < param.dst_height; i++)
            for (j = 0; j < param.dst_width; j++) {
                ptr[i * param.dst_stride + j] = 0x10;
                ptr[param.dst_stride * (param.dst_height + i / 2) + j] = 0x80;
            }
        AX_SYS_MflushCache(phySrc, vSrc, size);
        AX_SYS_MflushCache(phyDst, vDst, dstsize);

        DSP_SavePic("/opt/data/dsp/src.nv12", vSrc, size);
        printf("/opt/data/dsp/src.nv12 saved.\n");
    }

    param.phySrc_y = phySrc;
    param.phySrc_uv = phySrc + param.src_stride * param.src_height;
    param.phyDst_y = phyDst;
    param.phyDst_uv = phyDst + param.dst_stride * param.dst_height;

    srand(time(0));
    for (i = 0; i < 1; i++) {
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_NV12COPY(dspid, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            goto ext3;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    DSP_SavePic("/opt/data/dsp/cp.nv12", vDst, dstsize);
    printf("/opt/data/dsp/cp.nv12 saved.\n");

    if (0 == ret) {
        printf("SAMPLE_DSP%d_TestNV12COPY PASS\n", dspid);
    }

ext3:
    AX_SYS_MemFree(phyDst, vDst);
ext2:
    AX_SYS_MemFree(phySrc, vSrc);
ext0:
    AX_DSP_CV_Exit(dspid);

    return ret;
}

int SAMPLE_DSP_TestNV12Blending(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i, j;
    AX_U64 phySrc, phySrc2, phyDst;
    AX_VOID *vSrc, *vSrc2, *vDst;
    AX_DSP_CV_NV12Blending_PARAM_T param;
    AX_DSP_CV_COPY_PARAM_T cp_param;
    AX_S32 size_nv12, size_rgba, dstsize;
    struct timeval tv_start, tv_end;

    //left (1280) part is blending with rgba frame
    param.width = 3840;
    param.height = 2160;
    param.src_stride_NV12 = 3840;
    param.src_stride_RGBA = 3840 * 4;
    param.dst_stride = 3840 * 4;
    param.src_x = 0;
    param.src_y = 0;
    param.win_w = 1280;
    param.win_h = 2160;
    param.para = 0; //to RGBA
    size_nv12 = param.src_stride_NV12 * param.height * 3 / 2;
    size_rgba = param.src_stride_RGBA * param.height;
    dstsize = param.dst_stride * param.height;

    ret = AX_DSP_CV_Init(dspid);
    if (0 != ret) {
        vdsp_err("setup_dsp fail, ret=0x%x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAllocCached(&phySrc, &vSrc, size_nv12, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext0;
    }
    ret = AX_SYS_MemAllocCached(&phySrc2, &vSrc2, size_rgba, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, dstsize, 128, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        goto ext2;
    }
    {
        unsigned char *ptr = vSrc;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.width; j++) {
                ptr[i * param.src_stride_NV12 + j] = 0x10;
                ptr[param.src_stride_NV12 * (param.height + i / 2) + j] = 0x80;
            }
        ptr = vSrc2;
        for (i = 0; i < param.height; i++)
            for (j = 0; j < param.src_stride_RGBA; j += 4) {
                if (j < param.win_w * 4) {
                    ptr[i * param.src_stride_RGBA + j] = 255;
                    ptr[i * param.src_stride_RGBA + j + 1] = 255;
                    ptr[i * param.src_stride_RGBA + j + 2] = 255;
                    ptr[i * param.src_stride_RGBA + j + 3] = i % 256;
                } else {
                    ptr[i * param.src_stride_RGBA + j] = 0;
                    ptr[i * param.src_stride_RGBA + j + 1] = 255;
                    ptr[i * param.src_stride_RGBA + j + 2] = 0;
                    ptr[i * param.src_stride_RGBA + j + 3] = 255;
                }
            }
        AX_SYS_MflushCache(phySrc, vSrc, size_nv12);
        AX_SYS_MflushCache(phySrc2, vSrc2, size_rgba);
        memset(vDst, 0, dstsize);
    }

    param.phySrc_NV12 = phySrc;
    param.phySrc_RGBA = phySrc2;
    param.phyDst = phyDst;

    //right part copy from rgba frame
    cp_param.phySrc = param.phySrc_RGBA + param.win_w * 4;
    cp_param.phyDst = param.phyDst + param.win_w * 4;
    cp_param.win_w = param.width * 4 - param.win_w * 4;
    cp_param.win_h = param.height;
    cp_param.para = 0;
    cp_param.src_stride = param.src_stride_RGBA;
    cp_param.dst_stride = param.dst_stride;

    srand(time(0));
    for (i = 0; i < 1; i++) {
        gettimeofday(&tv_start, NULL);
        ret = AX_DSP_CV_NV12Blending(dspid, &param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            goto ext3;
        }
        ret = AX_DSP_CV_COPY(dspid, &cp_param);
        if (ret != 0) {
            printf("%s PRC error %x\n", __func__, ret);
            goto ext3;
        }
        gettimeofday(&tv_end, NULL);
        printf("%s DSP cost time: tv_usec: %ld\n", __func__,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    }

    DSP_SavePic("/opt/data/dsp/src1.nv12", vSrc, size_nv12);
    printf("/opt/data/dsp/src1.nv12 saved.\n");
    DSP_SavePic("/opt/data/dsp/src2.nv12", vSrc2, size_rgba);
    printf("/opt/data/dsp/src2.nv12 saved.\n");
    DSP_SavePic("/opt/data/dsp/bl.nv12", vDst, dstsize);
    printf("/opt/data/dsp/bl.nv12 saved.\n");

    if (0 == ret) {
        printf("SAMPLE_DSP%d_TestNV12Blending PASS\n", dspid);
    }

ext3:
    AX_SYS_MemFree(phyDst, vDst);
ext2:
    AX_SYS_MemFree(phySrc2, vSrc2);
ext1:
    AX_SYS_MemFree(phySrc, vSrc);
ext0:
    AX_DSP_CV_Exit(dspid);

    return ret;
}
