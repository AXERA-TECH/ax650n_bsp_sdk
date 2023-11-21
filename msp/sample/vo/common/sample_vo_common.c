/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "ax_sys_api.h"
#include "ax_base_type.h"
#include "ax_vo_api.h"
#include "ax_ivps_api.h"

#include "common_vo.h"
#include "common_vo_pattern.h"
#include "sample_vo_common.h"

typedef struct axSAMPLE_VO_CHN_THREAD_PARAM {
    pthread_t ThreadID;

    AX_BOOL bNeedCtrl;
    pthread_t ChnCtrlThreadID;

    AX_U32 u32Udelay;

    AX_FRAME_COMPRESS_INFO_T chnCompressInfo;
    AX_CHAR chnFileName[VO_NAME_LEN];
    AX_U32 u32SendCnt;
    AX_U32 u32ThreadForceStop;

    AX_U32 u32FrameRate;
    AX_U32 u32FrameMax;

    AX_U32 u32LayerID;
    AX_U32 u32ChnID;
    AX_POOL u32UserPoolId;
} SAMPLE_VO_CHN_THREAD_PARAM_T;

typedef struct axSAMPLE_VO_VIDEOLAYER_THREAD_PARAM {
    SAMPLE_VO_CHN_THREAD_PARAM_T stVoChnThreadParm[AX_VO_CHN_MAX];

    AX_U32 u32ThreadForceStop;
    AX_U32 u32LayerID;
    AX_U32 u32ChnID;
    AX_U32 u32FrameCnt;

    pthread_t ThreadID;
} SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T;

typedef struct axSAMPLE_VO_WBC_THREAD_PARAM {
    AX_U32 u32ThreadForceStop;
    AX_U32 u32Wbc;
    AX_U32 u32FrameCnt;

    pthread_t ThreadID;
} SAMPLE_VO_WBC_THREAD_PARAM_T;

typedef struct axSAMPLE_VO_CURSOR_PARAM {
    AX_U32 u32StartX;
    AX_U32 u32StartY;
    AX_U32 u32Width;
    AX_U32 u32Height;

    AX_U32 u32FbIndex;

    AX_U32 u32CursorLayerEn;
    AX_U32 u32CursorMoveEn;
} SAMPLE_VO_CURSOR_PARAM_T;


typedef struct axSAMPLE_VO_CURSOR_MOVE_THREAD_PARAM {
    SAMPLE_VO_CURSOR_PARAM_T stCursorInfo[SAMPLE_VO_DEV_MAX];

    pthread_t ThreadID;
    AX_U32 u32ThreadForceStop;
} SAMPLE_VO_CURSOR_MOVE_THREAD_PARAM_T;

static int SAMPLE_VO_POOL_DESTROY(AX_U32 u32PoolID)
{
    return AX_POOL_DestroyPool(u32PoolID);
}

static AX_S32 SAMPLE_VO_CREATE_POOL(AX_U32 u32BlkCnt, AX_U64 u64BlkSize, AX_U64 u64MetaSize, AX_U32 *pPoolID)
{
    AX_POOL_CONFIG_T stPoolCfg = {0};

    stPoolCfg.MetaSize = u64MetaSize;
    stPoolCfg.BlkCnt = u32BlkCnt;
    stPoolCfg.BlkSize = u64BlkSize;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((char *)stPoolCfg.PartitionName, "anonymous");

    *pPoolID = AX_POOL_CreatePool(&stPoolCfg);
    if (*pPoolID == AX_INVALID_POOLID) {
        SAMPLE_PRT("AX_POOL_CreatePool failed, u32BlkCnt = %d, u64BlkSize = 0x%llx, u64MetaSize = 0x%llx\n", u32BlkCnt,
                   u64BlkSize, u64MetaSize);
        return -1;
    }

    SAMPLE_PRT("u32BlkCnt = %d, u64BlkSize = 0x%llx, pPoolID = %d\n", u32BlkCnt, u64BlkSize, *pPoolID);

    return 0;
}

static AX_S32 SAMPLE_VO_FB_CONF(AX_U32 u32Type, SAMPLE_FB_CONFIG_S *pstFbConf)
{
    AX_S32 i, j, s32Fd, s32Ret = 0;
    AX_U16 *u16Pixel;
    AX_U32 *u32Pixel;
    AX_U32 u32FbIndex, u32Width, u32Height, u32Fmt;
    AX_CHAR fbPath[32];
    AX_U8 *pShowScreen;
    struct fb_fix_screeninfo stFix;
    struct fb_var_screeninfo stVar;
    struct fb_bitfield r = {16, 8, 0};
    struct fb_bitfield g = {8, 8, 0};
    struct fb_bitfield b = {0, 8, 0};
    struct fb_bitfield a = {24, 8, 0};
    AX_IVPS_RGN_CANVAS_INFO_T stCanvas = {0,};
    AX_IVPS_GDI_ATTR_T stAttr = {0,};
    AX_IVPS_RECT_T stRect = {0,};
    AX_FB_COLORKEY_T stColorKey;

    u32FbIndex = pstFbConf->u32Index;
    u32Width = pstFbConf->u32ResoW;
    u32Height = pstFbConf->u32ResoH;
    u32Fmt = pstFbConf->u32Fmt;

    stColorKey.u16Enable = pstFbConf->u32ColorKeyEn;
    stColorKey.u16Inv = pstFbConf->u32ColorKeyInv;
    stColorKey.u32KeyLow = pstFbConf->u32ColorKey;
    stColorKey.u32KeyHigh = pstFbConf->u32ColorKey;

    if (u32Fmt == AX_FORMAT_ARGB1555) {
        r.offset = 10;
        r.length = 5;
        g.offset = 5;
        g.length = 5;
        b.offset = 0;
        b.length = 5;
        a.offset = 15;
        a.length = 1;
    }

    /* 1.Open framebuffer device */
    snprintf(fbPath, sizeof(fbPath), "/dev/fb%d", u32FbIndex);
    s32Fd = open(fbPath, O_RDWR);
    if (s32Fd < 0) {
        SAMPLE_PRT("open %s failed, err:%s\n", fbPath, strerror(errno));
        return s32Fd;
    }

    /* 2.Get the variable screen info */
    s32Ret = ioctl(s32Fd, FBIOGET_VSCREENINFO, &stVar);
    if (s32Ret < 0) {
        SAMPLE_PRT("get variable screen info from fb%d failed\n", u32FbIndex);
        goto exit;
    }

    /* 3.Modify the variable screen info, the screen size: u32Width*u32Height, the
     * virtual screen size: u32Width*(u32Height*2), the pixel format: ARGB8888
     */
    stVar.xres = stVar.xres_virtual = u32Width;
    stVar.yres = u32Height;
    stVar.yres_virtual = u32Height * 2;
    stVar.transp = a;
    stVar.red = r;
    stVar.green = g;
    stVar.blue = b;
    stVar.bits_per_pixel = (u32Fmt == AX_FORMAT_ARGB1555) ? 16 : 32;

    /* 4.Set the variable screeninfo */
    s32Ret = ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVar);
    if (s32Ret < 0) {
        SAMPLE_PRT("put variable screen info to fb%d failed\n", u32FbIndex);
        goto exit;
    }

    /* 5.Get the fix screen info */
    s32Ret = ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFix);
    if (s32Ret < 0) {
        SAMPLE_PRT("get fix screen info from fb%d failed\n", u32FbIndex);
        goto exit;
    }

    /* 6.Map the physical video memory for user use */
    pShowScreen = mmap(NULL, stFix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
    if (pShowScreen == (AX_U8 *) - 1) {
        SAMPLE_PRT("map fb%d failed\n", u32FbIndex);
        goto exit;
    }

    if (u32Type) { /* GUI */
        if (u32Fmt != AX_FORMAT_ARGB8888) {
            SAMPLE_PRT("fb%d fmt(0x%x) invalid\n", u32FbIndex, u32Fmt);
            s32Ret = -1;
            goto exit;
        }

        SAMPLE_Fill_Color(u32Fmt, u32Width, u32Height * 2, u32Width * 4, pShowScreen);
        for (i = u32Height / 4; i < u32Height * 3 / 4; i++) {
            u32Pixel = (AX_U32 *)(pShowScreen + i * u32Width * 4);
            for (j = u32Width / 3; j < u32Width * 2 / 3; j++) {
                if (stColorKey.u16Enable) {
                    u32Pixel[j] &= ~(0xff << 24);
                } else {
                    u32Pixel[j] = stColorKey.u32KeyLow;
                }
            }
        }

    } else { /* Track Frame */
        for (i = 0; i < u32Height; i++) {
            if (u32Fmt == AX_FORMAT_ARGB8888) {
                u32Pixel = (AX_U32 *)(pShowScreen + i * u32Width * 4);
                for (j = 0; j < u32Width; j++) {
                    u32Pixel[j] = (0xFF << 24) | (((stColorKey.u32KeyLow >> 16) & 0xFF) << 16) |
                                  (((stColorKey.u32KeyLow >> 8) & 0xFF) << 8) |
                                  (((stColorKey.u32KeyLow >> 0) & 0xFF) << 0);
                }

            } else {
                u16Pixel = (AX_U16 *)(pShowScreen + i * u32Width * 2);
                for (j = 0; j < u32Width; j++) {
                    u16Pixel[j] = (0x1 << 15) | (((stColorKey.u32KeyLow >> 16) & 0x1F) << 10) |
                                  (((stColorKey.u32KeyLow >> 8) & 0x1F) << 5) |
                                  (((stColorKey.u32KeyLow >> 0) & 0x1F) << 0);
                }
            }
        }

        stCanvas.pVirAddr = pShowScreen;
        stCanvas.nPhyAddr = stFix.smem_start;
        stCanvas.nStride = u32Width;
        stCanvas.nW = u32Width;
        stCanvas.nH = u32Height;
        stCanvas.eFormat = u32Fmt;

        stAttr.nThick = 4;
        stAttr.nAlpha = 0xFF;
        stAttr.nColor = 0xFF;

        stRect.nX = u32Width / 2;
        stRect.nY = u32Height / 2;
        stRect.nW = 50;
        stRect.nH = 30;

        s32Ret = AX_IVPS_DrawRect(&stCanvas, stAttr, stRect);
        if (s32Ret) {
            SAMPLE_PRT("draw rect to fb%d failed\n", u32FbIndex);
            goto exit;
        }
    }

    munmap(pShowScreen, stFix.smem_len);

    /* 7.set colorkey */
    s32Ret = ioctl(s32Fd, AX_FBIOPUT_COLORKEY, &stColorKey);
    if (s32Ret < 0) {
        SAMPLE_PRT("set fb%d colorkey failed!\n", u32FbIndex);
        goto exit;
    }

    SAMPLE_PRT("init fb%d done\n", u32FbIndex);

exit:
    close(s32Fd);

    return s32Ret;
}

static AX_S32 SAMPLE_VO_CURSOR_INIT(AX_U32 u32StartX, AX_U32 u32StartY, AX_U32 u32Width, AX_U32 u32Height,
                                    AX_U32 u32FbIndex)
{
    AX_S32 i, j, s32Fd, s32Ret = 0;
    AX_U32 *u32Pixel;
    AX_CHAR fbPath[32];
    AX_U8 *pShowScreen;
    struct fb_fix_screeninfo stFix;
    struct fb_var_screeninfo stVar;
    struct fb_bitfield stR32 = {16, 8, 0};
    struct fb_bitfield stG32 = {8, 8, 0};
    struct fb_bitfield stB32 = {0, 8, 0};
    struct fb_bitfield stA32 = {24, 8, 0};
    AX_FB_CURSOR_POS_T stPos;
    AX_FB_CURSOR_RES_T stRes;
    AX_U16 show = 1;

    /* 1.Open framebuffer device */
    snprintf(fbPath, sizeof(fbPath), "/dev/fb%d", u32FbIndex);
    s32Fd = open(fbPath, O_RDWR);
    if (s32Fd < 0) {
        SAMPLE_PRT("open %s failed, err:%s\n", fbPath, strerror(errno));
        return s32Fd;
    }

    /* 2.Get the fix screen info */
    s32Ret = ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFix);
    if (s32Ret < 0) {
        SAMPLE_PRT("get fix screen info from fb%d failed\n", u32FbIndex);
        goto exit;
    }

    SAMPLE_PRT("cursor fb%d smem_len:%d line_length:%d\n", u32FbIndex, stFix.smem_len, stFix.line_length);

    /* 3.Get the variable screen info */
    s32Ret = ioctl(s32Fd, FBIOGET_VSCREENINFO, &stVar);
    if (s32Ret < 0) {
        SAMPLE_PRT("get variable screen info from fb%d failed\n", u32FbIndex);
        goto exit;
    }

    /* 4.Modify the variable screen info, the screen size: u32Width*u32Height, the
     * virtual screen size: u32Width*(u32Height*2), the pixel format: ARGB8888
     */
    stVar.xres = stVar.xres_virtual = stFix.line_length / 4;
    stVar.yres = stFix.smem_len / stFix.line_length;
    stVar.yres_virtual = stVar.yres ;
    stVar.transp = stA32;
    stVar.red = stR32;
    stVar.green = stG32;
    stVar.blue = stB32;
    stVar.bits_per_pixel = 32;

    /* 5.Set the variable screeninfo */
    s32Ret = ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVar);
    if (s32Ret < 0) {
        SAMPLE_PRT("put variable screen info to fb%d failed\n", u32FbIndex);
        goto exit;
    }

    /* 6.Map the physical video memory for user use , fill red color */
    pShowScreen = mmap(NULL, stFix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
    if (pShowScreen == (AX_U8 *) - 1) {
        SAMPLE_PRT("map fb%d failed\n", u32FbIndex);
        goto exit;
    }

    for (i = 0; i < u32Height; i++) {
        u32Pixel = (AX_U32 *)(pShowScreen + i * stFix.line_length);
        for (j = 0; j < u32Width; j++) {
            u32Pixel[j] = 0xff << 24 | 0xc0 << 16 ;
        }
    }

    munmap(pShowScreen, stFix.smem_len);

    /* 7.set the cursor display pos */
    stPos.u16X = u32StartX;
    stPos.u16Y = u32StartY;
    s32Ret = ioctl(s32Fd, AX_FBIOPUT_CURSOR_POS, &stPos);
    if (s32Ret < 0) {
        SAMPLE_PRT("put cursor pos fb%d failed\n", u32FbIndex);
        goto exit;
    }

    stRes.u32Width = u32Width;
    stRes.u32Height = u32Height;
    s32Ret = ioctl(s32Fd, AX_FBIOPUT_CURSOR_RES, &stRes);
    if (s32Ret < 0) {
        SAMPLE_PRT("put cursor res fb%d failed\n", u32FbIndex);
        goto exit;
    }

    if (u32FbIndex == 7) {
        show = 0;
    }

    /* 8.set the cursor show flag */
    s32Ret = ioctl(s32Fd, AX_FBIOPUT_CURSOR_SHOW, &show);
    if (s32Ret < 0) {
        SAMPLE_PRT("put cursor show fb%d failed\n", u32FbIndex);
        goto exit;
    }

    SAMPLE_PRT("init cursor fb%d done rect [%d %d %d %d] line_length:%d\n", u32FbIndex, u32StartX, u32StartY, u32Width,
               u32Height, stFix.line_length);

exit:
    close(s32Fd);

    return s32Ret;
}

static AX_VOID * SAMPLE_VO_CURSOR_MOVE_THREAD(AX_VOID *pData)
{
    AX_S32 s32Fd = -1, s32DevFd[SAMPLE_VO_DEV_MAX] = {-1};
    AX_S32 i, s32Ret = 0;
    AX_U32 u32FbIndex = 0;
    AX_CHAR fbPath[32];
    AX_FB_CURSOR_POS_T stPos;
    AX_U16 show = 1;
    AX_S32 x, y;
    AX_U32 u32StartX, u32StartY;
    SAMPLE_VO_CURSOR_MOVE_THREAD_PARAM_T *pstThreadParam = (SAMPLE_VO_CURSOR_MOVE_THREAD_PARAM_T *)pData;

    /* 1.Open framebuffer device */
    for (i = 0; i < SAMPLE_VO_DEV_MAX; i++) {
        if (pstThreadParam->stCursorInfo[i].u32CursorLayerEn) {
            snprintf(fbPath, sizeof(fbPath), "/dev/fb%d", pstThreadParam->stCursorInfo[i].u32FbIndex);
            s32DevFd[i] = open(fbPath, O_RDWR);
            if (s32DevFd[i] < 0) {
                SAMPLE_PRT("open %s failed, err:%s\n", fbPath, strerror(errno));
                goto exit;
            }
        }
    }

    while (!pstThreadParam->u32ThreadForceStop) {
        for (i = 0; i < SAMPLE_VO_DEV_MAX; i++) {
            if (pstThreadParam->stCursorInfo[i].u32CursorLayerEn) {
                s32Fd = s32DevFd[i];
                u32StartX = pstThreadParam->stCursorInfo[i].u32StartX;
                u32StartY = pstThreadParam->stCursorInfo[i].u32StartY;
                show = 1;

                for (x = 0; x < 8; x++) {
                    for (y = 0; y < 8; y++) {
                        /* 2.set the cursor display pos */
                        stPos.u16X = u32StartX + (x * 100);
                        stPos.u16Y = u32StartY + (y * 100);
                        s32Ret = ioctl(s32Fd, AX_FBIOPUT_CURSOR_POS, &stPos);
                        if (s32Ret < 0) {
                            SAMPLE_PRT("put cursor pos fb%d failed\n", u32FbIndex);
                            goto exit;
                        }

                        /* 3.set the cursor show flag */
                        s32Ret = ioctl(s32Fd, AX_FBIOPUT_CURSOR_SHOW, &show);
                        if (s32Ret < 0) {
                            SAMPLE_PRT("put cursor show fb%d failed\n", u32FbIndex);
                            goto exit;
                        }
                        usleep(100000);
                        if (pstThreadParam->u32ThreadForceStop) {
                            goto exit;
                        }
                    }
                }

                /* 3.close the layer cursor display */
                show = 0;
                s32Ret = ioctl(s32Fd, AX_FBIOPUT_CURSOR_SHOW, &show);
                if (s32Ret < 0) {
                    SAMPLE_PRT("put cursor show fb%d failed\n", u32FbIndex);
                    goto exit;
                }
            }
        }
    }

exit:
    for (i = 0; i < SAMPLE_VO_DEV_MAX; i++) {
        if (s32DevFd[i] != -1) {
            close(s32DevFd[i]);
            s32DevFd[i] = -1;
        }
    }

    return NULL;
}


#define CHN_IMGS_PATH ""
static AX_S32 load_img_file(AX_VOID *u64VirAddr, AX_U32 frameSize, AX_CHAR *img_name)
{
    AX_S32 ret = AX_SUCCESS;
    FILE *filep = NULL;
    char filename[VO_PATH_LEN] = "";
    AX_U32 nmemb;

    snprintf(filename, VO_PATH_LEN, CHN_IMGS_PATH "%s", img_name);
    filep = fopen(filename, "r");
    if (filep == NULL) {
        SAMPLE_PRT("Error! failed to open file %s.\n", filename);
        return -ENOENT;
    }

    nmemb = (frameSize + VO_FILE_BLOCK_SIZE - 1) / VO_FILE_BLOCK_SIZE;
    fread(u64VirAddr, VO_FILE_BLOCK_SIZE, nmemb, filep);
    if (ferror(filep)) {
        SAMPLE_PRT("Error! failed to read file %s.\n", filename);
        ret = -EBADF;
    }

    fclose(filep);

    return ret;
}

static AX_VOID *SAMPLE_VO_CHN_THREAD(AX_VOID *pData)
{
    AX_S32 s32Ret = 0;
    AX_VIDEO_FRAME_T stFrame = {0};
    AX_U32 u32FrameSize;
    AX_U32 u32LayerID, u32ChnID;
    AX_BLK BlkId;
    AX_VO_CHN_ATTR_T stChnAttr;
    AX_VOID *u64VirAddr;
    AX_U64 u64PhysAddr;
    AX_VO_QUERY_STATUS_T stStatus = {0};
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam = (SAMPLE_VO_CHN_THREAD_PARAM_T *)pData;

    u32LayerID = pstChnThreadParam->u32LayerID;
    u32ChnID = pstChnThreadParam->u32ChnID;

    s32Ret = AX_VO_GetChnAttr(u32LayerID, u32ChnID, &stChnAttr);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d AX_VO_GetChnAttr failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }
    SAMPLE_PRT("layer%d-chn%d u32Width = %d, u32Height = %d\n",
               u32LayerID, u32ChnID,
               stChnAttr.stRect.u32Width,
               stChnAttr.stRect.u32Height);

    stFrame.stCompressInfo = pstChnThreadParam->chnCompressInfo;
    stFrame.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    stFrame.u32Width = stChnAttr.stRect.u32Width;
    stFrame.u32Height = stChnAttr.stRect.u32Height;
    stFrame.u32PicStride[0] = stChnAttr.stRect.u32Width;
    u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 3 / 2;

    while (!pstChnThreadParam->u32ThreadForceStop) {
        if (!pstChnThreadParam->u32SendCnt) {
            usleep(10000);
            continue;
        }

        BlkId = AX_POOL_GetBlock(pstChnThreadParam->u32UserPoolId, u32FrameSize, NULL);
        if (AX_INVALID_BLOCKID == BlkId) {
            //SAMPLE_PRT("layer%d-chn%d AX_POOL_GetBlock failed\n", u32LayerID, u32ChnID);
            usleep(10000);
            continue;
        }

        stFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(BlkId);
        stFrame.u64VirAddr[0] = 0;

        u64PhysAddr = AX_POOL_Handle2PhysAddr(BlkId);
        if (!u64PhysAddr) {
            SAMPLE_PRT("AX_POOL_Handle2PhysAddr failed, BlkId = 0x%x\n", BlkId);
            goto lbl0;
        }

        u64VirAddr = AX_SYS_Mmap(u64PhysAddr, u32FrameSize);
        if (!u64VirAddr) {
            SAMPLE_PRT("AX_SYS_Mmap failed, u64PhysAddr = 0x%llx, u32FrameSize = 0x%x\n", u64PhysAddr, u32FrameSize);
            goto lbl0;
        }

        if (stFrame.stCompressInfo.enCompressMode == AX_COMPRESS_MODE_NONE) {
            SAMPLE_Fill_Color(stFrame.enImgFormat,
                              stFrame.u32Width,
                              stFrame.u32Height,
                              stFrame.u32PicStride[0],
                              u64VirAddr);
        } else {
            s32Ret = load_img_file(u64VirAddr, u32FrameSize, pstChnThreadParam->chnFileName);
            if (s32Ret) {
                SAMPLE_PRT("layer%d-chn%d load file failed, s32Ret = 0x%x\n",
                           u32LayerID, u32ChnID, s32Ret);
            }
        }

        stFrame.u32BlkId[0] = BlkId;
        stFrame.u32BlkId[1] = AX_INVALID_BLOCKID;
        s32Ret = AX_VO_SendFrame(u32LayerID, u32ChnID, &stFrame, 0);
        if (s32Ret)
            SAMPLE_PRT("layer%d-chn%d AX_VO_SendFrame failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);

        AX_SYS_Munmap(u64VirAddr, u32FrameSize);

        pstChnThreadParam->u32SendCnt -= 1;

        /* Query Channel Status */
        s32Ret = AX_VO_QueryChnStatus(u32LayerID, u32ChnID, &stStatus);
        if (s32Ret)
            SAMPLE_PRT("layer%d-chn%d AX_VO_QueryChnStatus failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);

        usleep(pstChnThreadParam->u32Udelay);

lbl0:
        AX_POOL_ReleaseBlock(BlkId);
    }

exit:
    SAMPLE_PRT("layer%d-chn%d exit, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);

    return NULL;
}

static AX_S32 SAMPLE_Fill_IMG(AX_S32 s32Fd, AX_U32 u32Width, AX_U32 u32Height, AX_U8 *u8Mem)
{
    AX_S32 s32Ret;
    AX_U32 u32FrameSize = u32Width * u32Height * 3 / 2;

retry:
    s32Ret = read(s32Fd, u8Mem, u32FrameSize);
    if (s32Ret != u32FrameSize) {
        if (s32Ret < 0) {
            SAMPLE_PRT("read file failed, err: %s\n", strerror(errno));
            return s32Ret;
        }

        lseek(s32Fd, 0, SEEK_SET);
        goto retry;
    }

    return 0;
}

static AX_S32 SAMPLE_VO_POOL_FILL_IMG(AX_CHAR *pPath, AX_U32 u32PoolId, AX_U32 u32Width, AX_U32 u32Height,
                                      AX_U32 u32FNum, AX_VIDEO_FRAME_T *pstFrames)
{
    AX_S32 i, s32Fd, s32Ret;
    AX_U32 u32FrameSize;
    AX_BLK BlkId;
    AX_VOID *u64VirAddr;
    AX_U64 u64PhysAddr;

    u32FrameSize = u32Width * u32Height * 3 / 2;

    s32Fd = open(pPath, O_RDONLY);
    if (s32Fd < 0) {
        SAMPLE_PRT("open %s failed, %s\n", pPath, strerror(errno));
        return -1;
    }

    for (i = 0; i < u32FNum; i++) {
        BlkId = AX_POOL_GetBlock(u32PoolId, u32FrameSize, NULL);
        if (AX_INVALID_BLOCKID == BlkId) {
            s32Ret = -1;
            SAMPLE_PRT("get block failed from pool%d, i = %d\n", u32PoolId, i);
            goto exit;
        }

        u64PhysAddr = AX_POOL_Handle2PhysAddr(BlkId);
        if (!u64PhysAddr) {
            s32Ret = -1;
            SAMPLE_PRT("AX_POOL_Handle2PhysAddr failed, BlkId = 0x%x\n", BlkId);
            goto exit;
        }

        u64VirAddr = AX_SYS_Mmap(u64PhysAddr, u32FrameSize);
        if (!u64VirAddr) {
            s32Ret = -1;
            SAMPLE_PRT("AX_SYS_Mmap failed, u64PhysAddr = 0x%llx, u32FrameSize = 0x%x\n", u64PhysAddr, u32FrameSize);
            goto exit;
        }

        s32Ret = SAMPLE_Fill_IMG(s32Fd, u32Width, u32Height, u64VirAddr);
        if (s32Ret) {
            s32Ret = -1;
            SAMPLE_PRT("SAMPLE_Fill_IMG failed, i = %d\n", i);
            goto exit;
        }

        AX_SYS_Munmap(u64VirAddr, u32FrameSize);

        pstFrames[i].enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        pstFrames[i].u32Width = u32Width;
        pstFrames[i].u32Height = u32Height;
        pstFrames[i].u32PicStride[0] = u32Width;
        pstFrames[i].u64PhyAddr[0] = u64PhysAddr;
        pstFrames[i].u32BlkId[0] = BlkId;
        pstFrames[i].u32BlkId[1] = AX_INVALID_BLOCKID;
    }

exit:
    close(s32Fd);

    return s32Ret;
}

static AX_S32 SAMPLE_VO_GET_CHN_CURFR_SEQ(AX_U32 u32LayerID, AX_U32 u32ChnID, AX_U64 *u64SeqNum)
{
    AX_S32 s32Ret = 0;
    AX_VIDEO_FRAME_T stFrame;

    s32Ret = AX_VO_GetChnFrame(u32LayerID, u32ChnID, &stFrame, -1);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d AX_VO_GetChnFrame failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }

    *u64SeqNum = stFrame.u64SeqNum;

    s32Ret = AX_VO_ReleaseChnFrame(u32LayerID, u32ChnID, &stFrame);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d AX_VO_ReleaseChnFrame failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }

exit:

    return s32Ret;
}

static AX_VOID *SAMPLE_VO_PLAY_CTRL_THREAD(AX_VOID *pData)
{
    AX_S32 j, s32Ret = 0;
    AX_U32 u32LayerID, u32ChnID;
    AX_U64 u64SeqNum = 0, u64LastSeqNum;
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam = (SAMPLE_VO_CHN_THREAD_PARAM_T *)pData;
    AX_VO_QUERY_STATUS_T stChnStat;
    AX_VO_CHN_ATTR_T stChnAttr;

    u32LayerID = pstChnThreadParam->u32LayerID;
    u32ChnID = pstChnThreadParam->u32ChnID;

    s32Ret = AX_VO_GetChnAttr(u32LayerID, u32ChnID, &stChnAttr);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d AX_VO_GetChnAttr failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }

    SAMPLE_PRT("layer%d-chn%d attr:%d-%d-%d-%d-%d-%d-%d-%d\n", u32LayerID, u32ChnID,
               stChnAttr.stRect.u32X, stChnAttr.stRect.u32Y,
               stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height,
               stChnAttr.u32FifoDepth, stChnAttr.u32Priority,
               stChnAttr.bKeepPrevFr, stChnAttr.bInUseFrOutput);

    while (!pstChnThreadParam->u32ThreadForceStop) {
        s32Ret = SAMPLE_VO_GET_CHN_CURFR_SEQ(u32LayerID, u32ChnID, &u64SeqNum);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d SAMPLE_VO_GET_CHN_CURFR_SEQ failed\n", u32LayerID, u32ChnID);
            goto exit;
        }

        u64LastSeqNum = u64SeqNum;

        sleep(1);

        s32Ret = SAMPLE_VO_GET_CHN_CURFR_SEQ(u32LayerID, u32ChnID, &u64SeqNum);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d SAMPLE_VO_GET_CHN_CURFR_SEQ failed\n", u32LayerID, u32ChnID);
            goto exit;
        }

        SAMPLE_PRT("layer%d-chn%d handle %lld-frames per 1s\n", u32LayerID, u32ChnID, u64SeqNum - u64LastSeqNum);

        /* pause test */
        SAMPLE_PRT("layer%d-chn%d pause\n", u32LayerID, u32ChnID);
        s32Ret = AX_VO_PauseChn(u32LayerID, u32ChnID);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_PauseChn failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
            goto exit;
        }

        usleep(100000);

        s32Ret = SAMPLE_VO_GET_CHN_CURFR_SEQ(u32LayerID, u32ChnID, &u64SeqNum);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d SAMPLE_VO_GET_CHN_CURFR_SEQ failed\n", u32LayerID, u32ChnID);
            goto exit;
        }

        u64LastSeqNum = u64SeqNum;

        /* step test */
        SAMPLE_PRT("layer%d-chn%d step\n", u32LayerID, u32ChnID);
        for (j = 0; j < 10; j++) {
            s32Ret = AX_VO_StepChn(u32LayerID, u32ChnID);
            if (s32Ret) {
                SAMPLE_PRT("layer%d-chn%d AX_VO_StepChn failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
                goto exit;
            }

            sleep(1);

            s32Ret = SAMPLE_VO_GET_CHN_CURFR_SEQ(u32LayerID, u32ChnID, &u64SeqNum);
            if (s32Ret) {
                SAMPLE_PRT("layer%d-chn%d SAMPLE_VO_GET_CHN_CURFR_SEQ failed\n", u32LayerID, u32ChnID);
                goto exit;
            }

            SAMPLE_PRT("layer%d-chn%d u64LastSeqNum:%lld u64SeqNum:%lld\n", u32LayerID, u32ChnID, u64LastSeqNum, u64SeqNum);

            if (u64SeqNum - u64LastSeqNum != 1) {
                SAMPLE_PRT("layer%d-chn%d step failed\n", u32LayerID, u32ChnID);
                goto exit;
            }

            u64LastSeqNum = u64SeqNum;
        }

        /* query chn-status test */
        SAMPLE_PRT("layer%d-chn%d query chn stat\n", u32LayerID, u32ChnID);
        s32Ret = AX_VO_QueryChnStatus(u32LayerID, u32ChnID, &stChnStat);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_QueryChnStatus failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
            goto exit;
        }
        SAMPLE_PRT("layer%d-chn%d u32ChnBufUsed:%d\n", u32LayerID, u32ChnID, stChnStat.u32ChnBufUsed);

        /* resume test */
        SAMPLE_PRT("layer%d-chn%d resume\n", u32LayerID, u32ChnID);
        s32Ret = AX_VO_ResumeChn(u32LayerID, u32ChnID);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_ResumeChn failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
            goto exit;
        }

        sleep(1);

        /* hide test */
        SAMPLE_PRT("layer%d-chn%d hide\n", u32LayerID, u32ChnID);
        s32Ret = AX_VO_HideChn(u32LayerID, u32ChnID);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_ResumeChn failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
            goto exit;
        }

        sleep(2);

        /* show test */
        SAMPLE_PRT("layer%d-chn%d show\n", u32LayerID, u32ChnID);
        s32Ret = AX_VO_ShowChn(u32LayerID, u32ChnID);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_ResumeChn failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
            goto exit;
        }

        break;
    }

exit:
    SAMPLE_PRT("layer%d-chn%d play-ctrl %s\n", u32LayerID, u32ChnID, s32Ret ? "failed" : "success");

    return NULL;
}

static AX_VOID *SAMPLE_VO_PLAY_THREAD(AX_VOID *pData)
{
    AX_S32 i = 0, s32Ret = 0;
    AX_VIDEO_FRAME_T stFrames[SAMPLE_VO_FRAME_MAX] = {0}, *pstFrame;
    AX_U32 u32LayerID, u32ChnID;
    AX_U32 u32FrameRate, u32FrameMax = SAMPLE_VO_FRAME_MAX;
    AX_U64 u64PTS, u64SeqNum = 0;
    AX_VO_CHN_ATTR_T stChnAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam = (SAMPLE_VO_CHN_THREAD_PARAM_T *)pData;
    AX_S32 num = 0;

    u32LayerID = pstChnThreadParam->u32LayerID;
    u32ChnID = pstChnThreadParam->u32ChnID;

    s32Ret = AX_VO_GetChnAttr(u32LayerID, u32ChnID, &stChnAttr);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d AX_VO_GetChnAttr failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }
    SAMPLE_PRT("layer%d-chn%d u32Width = %d, u32Height = %d\n",
               u32LayerID, u32ChnID,
               stChnAttr.stRect.u32Width,
               stChnAttr.stRect.u32Height);

    if ((pstChnThreadParam->u32FrameMax > 0) && (pstChnThreadParam->u32FrameMax < SAMPLE_VO_FRAME_MAX)) {
        u32FrameMax = pstChnThreadParam->u32FrameMax;
    }

    s32Ret = SAMPLE_VO_POOL_FILL_IMG(pstChnThreadParam->chnFileName, pstChnThreadParam->u32UserPoolId,
                                     stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height,
                                     u32FrameMax, stFrames);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_VO_POOL_FILL_IMG failed\n");
        goto exit;
    }

    u32FrameRate = pstChnThreadParam->u32FrameRate ? pstChnThreadParam->u32FrameRate : 30;
    AX_VO_SetChnFrameRate(u32LayerID, u32ChnID, u32FrameRate);

    SAMPLE_PRT("u32FrameRate = %d, u32FrameMax = %d\n", u32FrameRate, u32FrameMax);

    while (!pstChnThreadParam->u32ThreadForceStop) {
        pstFrame = &stFrames[i % u32FrameMax];
        pstFrame->u64PTS = u64PTS;
        pstFrame->u64SeqNum = u64SeqNum;
        pstFrame->u32FrameFlag |= AX_FRM_FLG_FR_CTRL;

        s32Ret = AX_VO_SendFrame(u32LayerID, u32ChnID, pstFrame, -1);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_SendFrame failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        }

        u64PTS += 1000000 / u32FrameRate;
        u64SeqNum += 1;
        i++;

        if (pstChnThreadParam->bNeedCtrl && !pstChnThreadParam->ChnCtrlThreadID) {
            pthread_create(&pstChnThreadParam->ChnCtrlThreadID, NULL, SAMPLE_VO_PLAY_CTRL_THREAD, pstChnThreadParam);
        }
    }

exit:
    SAMPLE_PRT("layer%d-chn%d exit, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);

    for (num = 0; num < u32FrameMax; num++) {
        if (stFrames[num].u32BlkId[0]) {
            AX_POOL_ReleaseBlock(stFrames[num].u32BlkId[0]);
        }
    }

    return NULL;
}

static AX_VOID *SAMPLE_VO_GET_LAYER_FRAME_THREAD(AX_VOID *pData)
{
    AX_S32 i, s32Ret = 0;
    AX_VIDEO_FRAME_T stFrame = {0};
    AX_U32 u32LayerID;
    AX_VOID *pVirAddr;
    AX_U32 u32GetCnt = 0, u32FrameSize = 0;
    AX_CHAR OutFile[128] = {0};
    AX_S32 s32Fd = -1;
    fd_set fds;
    FILE *pstFile = NULL;
    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam;
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T *pstThreadParam = (SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T *)pData;
    u32LayerID = pstThreadParam->u32LayerID;

    s32Ret = AX_VO_GetLayerFd(u32LayerID, &s32Fd);
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_GetLayerFd failed, s32Ret = 0x%x\n", s32Ret);
        return NULL;
    }

    FD_ZERO(&fds);
    FD_SET(s32Fd, &fds);

    while (!pstThreadParam->u32ThreadForceStop) {
        s32Ret = select(s32Fd + 1, &fds, NULL, NULL, &timeout);
        if (s32Ret < 0) {
            SAMPLE_PRT("select err\n");
            break;

        } else if (0 == s32Ret) {
            continue;

        } else {
            if (FD_ISSET(s32Fd, &fds)) {
                memset(&stFrame, 0, sizeof(stFrame));
                s32Ret = AX_VO_GetLayerFrame(u32LayerID, &stFrame, 0);
                if (s32Ret)
                    continue;

                if (stFrame.enImgFormat == AX_FORMAT_YUV420_SEMIPLANAR)
                    u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 3 / 2;

                pVirAddr = AX_SYS_Mmap(stFrame.u64PhyAddr[0], u32FrameSize);
                if (!pVirAddr) {
                    SAMPLE_PRT("AX_SYS_Mmap failed\n");
                    goto lbl0;
                }

                SAMPLE_PRT("pVirAddr: 0x%px, u64PhyAddr: 0x%llx, u32FrameSize: 0x%x\n", pVirAddr, stFrame.u64PhyAddr[0], u32FrameSize);

                sprintf(OutFile, "layer%d_%d_%d_%d.yuv", u32LayerID, stFrame.u32Width, stFrame.u32Height, u32GetCnt);
                pstFile = fopen(OutFile, "wb");
                if (pstFile) {
                    fwrite(pVirAddr, u32FrameSize, 1, pstFile);
                    fclose(pstFile);
                    SAMPLE_PRT("write %s done\n", OutFile);
                }

                s32Ret = AX_SYS_Munmap(pVirAddr, u32FrameSize);
                if (s32Ret)
                    SAMPLE_PRT("AX_SYS_Munmap failed, s32Ret=0x%x\n", s32Ret);

lbl0:
                u32GetCnt += 1;
                s32Ret = AX_VO_ReleaseLayerFrame(u32LayerID, &stFrame);
                if (s32Ret)
                    SAMPLE_PRT("AX_VO_ReleaseLayerFrame failed, u32LayerID = %d, s32Ret = 0x%x\n", u32LayerID, s32Ret);

                if (u32GetCnt == pstThreadParam->u32FrameCnt)
                    break;
            }
        }
    }

    for (i = 0; i < AX_VO_CHN_MAX; i++) {
        pstChnThreadParam = &pstThreadParam->stVoChnThreadParm[i];
        if (pstChnThreadParam->ThreadID)
            pstChnThreadParam->u32ThreadForceStop = 1;
        else
            break;
    }

    SAMPLE_PRT("layer%d exit\n", u32LayerID);

    return NULL;
}

static AX_VOID *SAMPLE_VO_GET_CHN_FRAME_THREAD(AX_VOID *pData)
{
    AX_S32 s32Ret = 0;
    AX_VIDEO_FRAME_T stFrame;
    AX_U32 u32LayerID, u32ChnID;
    AX_VOID *pVirAddr;
    AX_U32 u32GetCnt = 0, u32FrameSize = 0;
    AX_CHAR OutFile[128] = {0};
    FILE *pstFile = NULL;
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T *pstThreadParam = (SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T *)pData;

    u32LayerID = pstThreadParam->u32LayerID;
    u32ChnID = pstThreadParam->u32ChnID;

    SAMPLE_PRT("layer%d-chn%d enter\n", u32LayerID, u32ChnID);

    while (!pstThreadParam->u32ThreadForceStop) {
        memset(&stFrame, 0, sizeof(stFrame));
        s32Ret = AX_VO_GetChnFrame(u32LayerID, u32ChnID, &stFrame, -1);
        if (s32Ret)
            continue;

        if (stFrame.enImgFormat == AX_FORMAT_YUV420_SEMIPLANAR)
            u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 3 / 2;

        pVirAddr = AX_SYS_Mmap(stFrame.u64PhyAddr[0], u32FrameSize);
        if (!pVirAddr) {
            SAMPLE_PRT("AX_SYS_Mmap failed\n");
            goto lbl0;
        }

        SAMPLE_PRT("pVirAddr: 0x%px, u64PhyAddr: 0x%llx, u32FrameSize: 0x%x\n", pVirAddr, stFrame.u64PhyAddr[0], u32FrameSize);

        sprintf(OutFile, "layer%d_chn%d_%d_%d_%d.yuv", u32LayerID, u32ChnID, stFrame.u32Width, stFrame.u32Height, u32GetCnt);
        pstFile = fopen(OutFile, "wb");
        if (pstFile) {
            fwrite(pVirAddr, u32FrameSize, 1, pstFile);
            fclose(pstFile);
            SAMPLE_PRT("write %s done\n", OutFile);
        }

        s32Ret = AX_SYS_Munmap(pVirAddr, u32FrameSize);
        if (s32Ret)
            SAMPLE_PRT("AX_SYS_Munmap failed, s32Ret=0x%x\n", s32Ret);

lbl0:
        u32GetCnt += 1;
        s32Ret = AX_VO_ReleaseChnFrame(u32LayerID, u32ChnID, &stFrame);
        if (s32Ret)
            SAMPLE_PRT("AX_VO_ReleaseLayerFrame failed, u32LayerID = %d, s32Ret = 0x%x\n", u32LayerID, s32Ret);

        if (u32GetCnt == pstThreadParam->u32FrameCnt)
            break;
    }

    SAMPLE_PRT("layer%d exit\n", u32LayerID);

    return NULL;
}

static AX_VOID *SAMPLE_VO_LAYER_OUT_PROC_THREAD(AX_VOID *pData)
{
    AX_S32 s32Ret = 0;
    AX_VIDEO_FRAME_T stFrame = {0};
    AX_U32 u32LayerID;
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T *pstSndLayerThreadParam = (SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T *)pData;
    u32LayerID = pstSndLayerThreadParam->u32LayerID;

    SAMPLE_PRT("layer%d start\n", u32LayerID);

    while (!pstSndLayerThreadParam->u32ThreadForceStop) {
        s32Ret = AX_VO_GetLayerFrame(u32LayerID, &stFrame, 150);
        if (s32Ret) {
            continue;
        }

        s32Ret = AX_VO_SendFrame2Disp(u32LayerID, &stFrame, 0);
        if (s32Ret) {
            SAMPLE_PRT("AX_VO_SendFrame2Disp failed, u32LayerID = %d, s32Ret = 0x%x\n", u32LayerID, s32Ret);
        }

        s32Ret = AX_VO_ReleaseLayerFrame(u32LayerID, &stFrame);
        if (s32Ret) {
            SAMPLE_PRT("AX_VO_ReleaseLayerFrame failed, u32LayerID = %d, s32Ret = 0x%x\n", u32LayerID, s32Ret);
        }
    }

    SAMPLE_PRT("layer%d exit\n", u32LayerID);

    return NULL;
}

static AX_VOID *SAMPLE_VO_WBC_THREAD(AX_VOID *pData)
{
    AX_S32 s32Ret = 0;
    AX_U32 u32WbcId = 0;
    AX_U32 u32GetCnt = 0, u32FrameSize = 0;
    AX_S32 s32Fd = -1;
    AX_VIDEO_FRAME_T stFrame = {0};
    AX_VOID *pVirAddr;
    AX_CHAR OutFile[128] = {0};
    fd_set fds;
    FILE *pstFile = NULL;
    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    SAMPLE_VO_WBC_THREAD_PARAM_T *pstThreadParam = (SAMPLE_VO_WBC_THREAD_PARAM_T *)pData;

    u32WbcId = pstThreadParam->u32Wbc;

    s32Ret = AX_VO_GetWbcFd(u32WbcId, &s32Fd);
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_GetLayerFd failed, s32Ret = 0x%x\n", s32Ret);
        return NULL;
    }

    FD_ZERO(&fds);
    FD_SET(s32Fd, &fds);

    while (!pstThreadParam->u32ThreadForceStop) {
        s32Ret = select(s32Fd + 1, &fds, NULL, NULL, &timeout);
        if (s32Ret < 0) {
            SAMPLE_PRT("select err\n");
            break;

        } else if (0 == s32Ret) {
            continue;

        } else {
            if (FD_ISSET(s32Fd, &fds)) {
                memset(&stFrame, 0, sizeof(stFrame));
                s32Ret = AX_VO_GetWBCFrame(u32WbcId, &stFrame, 0);
                if (s32Ret) {
                    continue;
                }

                if (stFrame.enImgFormat == AX_FORMAT_YUV420_SEMIPLANAR) {
                    u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 3 / 2;
                } else if (stFrame.enImgFormat == AX_FORMAT_RGB888) {
                    u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 3;
                } else if (stFrame.enImgFormat == AX_FORMAT_ARGB8888) {
                    u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 4;
                } else {
                    SAMPLE_PRT("save of this data format is not supported, fmt: 0x%x\n", stFrame.enImgFormat);
                    goto lbl0;
                }

                pVirAddr = AX_SYS_Mmap(stFrame.u64PhyAddr[0], u32FrameSize);
                if (!pVirAddr) {
                    SAMPLE_PRT("AX_SYS_Mmap failed\n");
                    goto lbl0;
                }

                SAMPLE_PRT("pVirAddr: 0x%px, u64PhyAddr: 0x%llx, u32FrameSize: 0x%x\n", pVirAddr, stFrame.u64PhyAddr[0], u32FrameSize);

                if (!pstFile) {
                    sprintf(OutFile, "wbc%d_%d_%d_%d.yuv", u32WbcId, stFrame.u32Width, stFrame.u32Height, pstThreadParam->u32FrameCnt);
                    pstFile = fopen(OutFile, "wb");
                    if (!pstFile) {
                        SAMPLE_PRT("fopen %s failed, err: %s\n", OutFile, strerror(errno));
                        goto lbl1;
                    }
                }

                fwrite(pVirAddr, u32FrameSize, 1, pstFile);

lbl1:
                s32Ret = AX_SYS_Munmap(pVirAddr, u32FrameSize);
                if (s32Ret) {
                    SAMPLE_PRT("AX_SYS_Munmap failed, s32Ret=0x%x\n", s32Ret);
                }

lbl0:
                u32GetCnt += 1;
                s32Ret = AX_VO_ReleaseWBCFrame(u32WbcId, &stFrame);
                if (s32Ret)
                    SAMPLE_PRT("AX_VO_ReleaseLayerFrame failed, u32WbcId = %d, s32Ret = 0x%x\n", u32WbcId, s32Ret);

                if (pstThreadParam->u32FrameCnt && u32GetCnt == pstThreadParam->u32FrameCnt) {
                    break;
                }
            }
        }
    }

    if (pstFile) {
        fclose(pstFile);
    }

    SAMPLE_PRT("Wbc%d exit\n", u32WbcId);

    return NULL;
}

static AX_S32 gLoopExit = 0;

AX_VOID SAMPLE_VO_SigInt(AX_S32 s32SigNo)
{
    SAMPLE_PRT("Catch signal %d\n", s32SigNo);
    gLoopExit = 1;
}

AX_VOID SAMPLE_VO_SigStop(AX_S32 s32SigNo)
{
    SAMPLE_PRT("Catch signal %d\n", s32SigNo);
    gLoopExit = 1;
}

AX_S32 SAMPLE_VO_CheckSig(AX_VOID)
{
    return gLoopExit;
}

AX_VOID SAMPLE_VO_Usage(AX_CHAR *name)
{
    SAMPLE_PRT("command:\n");
    SAMPLE_PRT("\t-p: play\n");
    SAMPLE_PRT("\tnumber: select test case number for play in /opt/etc/vo.ini\n");
    SAMPLE_PRT("Example:\n");
    SAMPLE_PRT("\tsample_vo -p 10\n");
    SAMPLE_PRT("\n");

    SAMPLE_PRT("\t-l: get videolayer Image\n");
    SAMPLE_PRT("\tnumber: select test case number for videolayer in /opt/etc/vo.ini\n");
    SAMPLE_PRT("Example:\n");
    SAMPLE_PRT("\tsample_vo -l 1\n");
    SAMPLE_PRT("\n");

    SAMPLE_PRT("\t-d: videolayer dispaly test\n");
    SAMPLE_PRT("\tnumber: select test case number for display in /opt/etc/vo.ini\n");;
    SAMPLE_PRT("Example:\n");
    SAMPLE_PRT("\tsample_vo -d 0\n");
    SAMPLE_PRT("\n");

    SAMPLE_PRT("\t-e: enumerate resolutions of dispaly device.\n");
    SAMPLE_PRT("\tnumber: supported display device number which is 0 1 or 2\n");;
    SAMPLE_PRT("Example:\n");
    SAMPLE_PRT("\tsample_vo -e 0\n");
    SAMPLE_PRT("\n");

    SAMPLE_PRT("\t-g: listening hdmi hot plug.\n");
    SAMPLE_PRT("Example:\n");
    SAMPLE_PRT("\tsample_vo -g\n");
    SAMPLE_PRT("\n");

    SAMPLE_PRT("\t-c: vo memcpy.\n");
    SAMPLE_PRT("\tnumber: 0:memcpy_1d, 2:memcpy_2d\n");;
    SAMPLE_PRT("Example:\n");
    SAMPLE_PRT("\tsample_vo -c 0\n");
}

AX_S32 SAMPLE_VO_LAYER(SAMPLE_VO_LAYER_CONFIG_S *pstLayerConf)
{
    AX_S32 i, s32Chns, s32Ret = 0;
    AX_U32 u32Row, u32Col;
    AX_U32 u32ChnWidth, u32ChnHeight;
    AX_U32 u32LayerWidth = 0;
    AX_U32 u32LayerHeight = 0;
    AX_U64 u64BlkSize = 0;
    AX_VO_VIDEO_LAYER_ATTR_T *pstVoLayerAttr = &pstLayerConf->stVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam;
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T stLayerThreadParam = {.u32FrameCnt = 1,};

    u32LayerWidth = pstVoLayerAttr->stImageSize.u32Width;
    u32LayerHeight = pstVoLayerAttr->stImageSize.u32Height;

    s32Ret = SAMPLE_VO_WIN_INFO(u32LayerWidth, u32LayerHeight, pstLayerConf->enVoMode, &u32Row, &u32Col,
                                &u32ChnWidth, &u32ChnHeight);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_VO_WIN_INFO failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    u64BlkSize = (AX_U64)u32LayerWidth * u32LayerHeight * 3 / 2;
    s32Ret = SAMPLE_VO_CREATE_POOL(3, u64BlkSize, 512, &pstLayerConf->u32LayerPoolId);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    s32Chns = u32Row * u32Col;
    u64BlkSize = (AX_U64)u32ChnWidth * u32ChnHeight * 3 / 2;
    s32Ret = SAMPLE_VO_CREATE_POOL(s32Chns * 2, u64BlkSize, 512, &pstLayerConf->u32ChnPoolId);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", s32Ret);
        goto exit1;
    }

    pstVoLayerAttr->u32PoolId = pstLayerConf->u32LayerPoolId;

    SAMPLE_PRT("u32LayerPoolId = %d, u32ChnPoolId = %d\n", pstLayerConf->u32LayerPoolId, pstLayerConf->u32ChnPoolId);

    s32Ret = SAMPLE_COMM_VO_StartLayer(pstLayerConf);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartLayer failed\n");
        goto exit2;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(pstLayerConf);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed, layer = %d\n", pstLayerConf->u32VoLayer);
        goto exit3;
    }

    stLayerThreadParam.u32LayerID = pstLayerConf->u32VoLayer;
    stLayerThreadParam.u32ThreadForceStop = 0;
    pthread_create(&stLayerThreadParam.ThreadID, NULL, SAMPLE_VO_GET_LAYER_FRAME_THREAD, &stLayerThreadParam);

    for (i = 0; i < s32Chns; i++) {
        pstChnThreadParam = &stLayerThreadParam.stVoChnThreadParm[i];
        pstChnThreadParam->u32LayerID = pstLayerConf->u32VoLayer;
        pstChnThreadParam->u32ChnID = i;
        pstChnThreadParam->u32Udelay = 20000;
        pstChnThreadParam->chnCompressInfo = pstLayerConf->chnCompressInfo;
        strcpy(pstChnThreadParam->chnFileName, pstLayerConf->chnFileName);
        pstChnThreadParam->u32SendCnt = stLayerThreadParam.u32FrameCnt;
        pstChnThreadParam->u32ThreadForceStop = 0;
        pstChnThreadParam->u32UserPoolId = pstLayerConf->u32ChnPoolId;
        pthread_create(&pstChnThreadParam->ThreadID, NULL, SAMPLE_VO_CHN_THREAD, pstChnThreadParam);
    }

    pthread_join(stLayerThreadParam.ThreadID, NULL);
    SAMPLE_PRT("layer%d-thread get frame done\n", pstLayerConf->u32VoLayer);

    for (i = 0; i < s32Chns; i++) {
        pstChnThreadParam = &stLayerThreadParam.stVoChnThreadParm[i];
        if (pstChnThreadParam->ThreadID) {
            pthread_join(pstChnThreadParam->ThreadID, NULL);
            SAMPLE_PRT("layer%d-chn%d-thread send frame done\n", pstLayerConf->u32VoLayer, i);
        }
    }

    SAMPLE_PRT("VO test Finished success!\n");

    SAMPLE_COMM_VO_StopChn(pstLayerConf);

exit3:
    SAMPLE_COMM_VO_StopLayer(pstLayerConf);
exit2:
    SAMPLE_VO_POOL_DESTROY(pstLayerConf->u32LayerPoolId);
exit1:
    SAMPLE_VO_POOL_DESTROY(pstLayerConf->u32ChnPoolId);
exit0:
    SAMPLE_PRT("layer%d exit done\n", pstLayerConf->u32VoLayer);

    return s32Ret;
}

AX_S32 SAMPLE_VO_LAYER_DISPLAY(SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_BOOL bNeedWbc = AX_FALSE;
    AX_S32 i, j, s32Chns, s32Ret = 0;
    AX_U32 u32Row, u32Col;
    AX_U32 u32ChnWidth, u32ChnHeight;
    AX_U32 u32LayerWidth = 0;
    AX_U32 u32LayerHeight = 0;
    AX_U64 u64BlkSize = 0, u64BlkNr;
    SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf;
    AX_VO_VIDEO_LAYER_ATTR_T *pstVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam;
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T stLayerThreadParam[SAMPLE_VO_DEV_MAX];
    SAMPLE_VO_WBC_THREAD_PARAM_T stWbcThreadParam[SAMPLE_VO_DEV_MAX] = {0,};

    s32Ret = AX_VO_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
        return s32Ret;
    }

    pstVoConf->u32LayerNr = pstVoConf->u32VDevNr;
    if (pstVoConf->u32BindMode) {
        pstVoConf->u32LayerNr = 1;
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        pstVoLayerConf = &pstVoConf->stVoLayer[i];

        if (pstVoConf->u32BindMode) {
            for (j = 0; j < pstVoConf->u32VDevNr; j++) {
                pstVoLayerConf->bindVoDev[j] = pstVoConf->stVoDev[j].u32VoDev;
                if (pstVoConf->stVoDev[j].bWbcEn) {
                    bNeedWbc = pstVoConf->stVoDev[j].bWbcEn;
                }
            }
        } else {
            pstVoLayerConf->bindVoDev[0] = pstVoDevConf->u32VoDev;
            bNeedWbc = pstVoDevConf->bWbcEn;
        }

        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        u32LayerWidth = pstVoLayerAttr->stImageSize.u32Width;
        u32LayerHeight = pstVoLayerAttr->stImageSize.u32Height;

        SAMPLE_VO_WIN_INFO(u32LayerWidth, u32LayerHeight, pstVoLayerConf->enVoMode,
                           &u32Row, &u32Col, &u32ChnWidth, &u32ChnHeight);

        s32Chns = u32Row * u32Col;
        u64BlkSize = (AX_U64)u32ChnWidth * u32ChnHeight * 3 / 2;
        s32Ret = SAMPLE_VO_CREATE_POOL(s32Chns * 2, u64BlkSize, 512, &pstVoLayerConf->u32ChnPoolId);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", s32Ret);
            goto exit0;
        }

        u64BlkSize = (AX_U64)ALIGN_UP(u32LayerWidth, 16) * ALIGN_UP(u32LayerHeight, 2) * 3 / 2;
        u64BlkNr = bNeedWbc ? 8 : 3;
        s32Ret = SAMPLE_VO_CREATE_POOL(u64BlkNr, u64BlkSize, 512, &pstVoLayerConf->u32LayerPoolId);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, i:%d, s32Ret:0x%x\n", i, s32Ret);
            goto exit0;
        }

        pstVoLayerConf->s32Chns = s32Chns;
        pstVoLayerAttr->u32FifoDepth = pstVoLayerConf->u32FifoDepth;
        pstVoLayerAttr->u32PoolId = pstVoLayerConf->u32LayerPoolId;

        SAMPLE_PRT("u32LayerPoolId = %d, u32ChnPoolId = %d\n", pstVoLayerConf->u32LayerPoolId, pstVoLayerConf->u32ChnPoolId);
    }

    s32Ret = SAMPLE_COMM_VO_StartVO(pstVoConf);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed, i:%d, s32Ret:0x%x\n", i, s32Ret);
        goto exit0;
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        if (pstVoDevConf->bWbcEn) {
            stWbcThreadParam[i].u32Wbc = pstVoDevConf->u32VoDev;
            stWbcThreadParam[i].u32FrameCnt = pstVoDevConf->u32WbcFrmaeNr;
            pthread_create(&stWbcThreadParam[i].ThreadID, NULL, SAMPLE_VO_WBC_THREAD, &stWbcThreadParam[i]);
        }
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        for (j = 0; j < pstVoLayerConf->s32Chns; j++) {
            stLayerThreadParam[i].u32FrameCnt = ~0;
            pstChnThreadParam = &stLayerThreadParam[i].stVoChnThreadParm[j];
            pstChnThreadParam->u32LayerID = pstVoLayerConf->u32VoLayer;
            pstChnThreadParam->u32ChnID = j;
            pstChnThreadParam->u32Udelay = 5000;
            pstChnThreadParam->u32SendCnt = stLayerThreadParam[i].u32FrameCnt;
            pstChnThreadParam->u32ThreadForceStop = 0;
            pstChnThreadParam->u32UserPoolId = pstVoLayerConf->u32ChnPoolId;
            pthread_create(&pstChnThreadParam->ThreadID, NULL, SAMPLE_VO_CHN_THREAD, pstChnThreadParam);
        }
    }

    while (!gLoopExit) {
        sleep(1);
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        if (pstVoDevConf->bWbcEn && stWbcThreadParam[i].ThreadID) {
            stWbcThreadParam[i].u32ThreadForceStop = 1;
            pthread_join(stWbcThreadParam[i].ThreadID, NULL);
            SAMPLE_PRT("wbc%d done\n", stWbcThreadParam[i].u32Wbc);
        }
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        for (j = 0; j < pstVoLayerConf->s32Chns; j++) {
            pstChnThreadParam = &stLayerThreadParam[i].stVoChnThreadParm[j];
            if (pstChnThreadParam->ThreadID) {
                pstChnThreadParam->u32ThreadForceStop = 1;
                pthread_join(pstChnThreadParam->ThreadID, NULL);
                SAMPLE_PRT("layer%d-chn%d-thread send frame done\n", pstVoLayerConf->u32VoLayer, j);
            }
        }
    }

    SAMPLE_COMM_VO_StopVO(pstVoConf);

    SAMPLE_PRT("VO test Finished success!\n");

exit0:
    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        SAMPLE_VO_POOL_DESTROY(pstVoLayerConf->u32ChnPoolId);
        SAMPLE_VO_POOL_DESTROY(pstVoLayerConf->u32LayerPoolId);
    }

    AX_VO_Deinit();

    return s32Ret;
}

AX_VOID SAMPLE_VO_DISPLAY_MODE_PRINT(AX_U32 u32DevId, AX_VO_DISPLAY_MODE_T *pstDisplayMode)
{
    AX_S32 i;
    AX_U32 u32Type;

    for (i = 0; i < pstDisplayMode->u16ModesNum; i++) {
        u32Type = pstDisplayMode->stModes[i].u32Type;
        SAMPLE_PRT("display%d-mode(%s): %d %d %d %d %d %d %d %d %d %d %x\n", u32DevId,
                   (u32Type == VO_DISPLAY_TYPE_HDMIA) ? "hdmi" : ((u32Type == VO_DISPLAY_TYPE_DSI) ? "dsi" : "virt"),
                   pstDisplayMode->stModes[i].u32Clock, pstDisplayMode->stModes[i].u16Refresh,
                   pstDisplayMode->stModes[i].u16HDisplay, pstDisplayMode->stModes[i].u16HSyncStart,
                   pstDisplayMode->stModes[i].u16HSyncEnd, pstDisplayMode->stModes[i].u16HTotal,
                   pstDisplayMode->stModes[i].u16VDisplay, pstDisplayMode->stModes[i].u16VSyncStart,
                   pstDisplayMode->stModes[i].u16VSyncEnd, pstDisplayMode->stModes[i].u16VTotal,
                   pstDisplayMode->stModes[i].u32Flags);
    }

    SAMPLE_PRT("VO test Finished success!\n");
}

static AX_VOID SAMPLE_HDMI_CallBack(AX_HDMI_EVENT_TYPE_E enEvent, AX_VOID *pPrivateData)
{
    AX_S32 i, s32Ret;
    AX_HDMI_EDID_T stEdidData;
    AX_HDMI_ID_E eHdmi;

    if (!pPrivateData)
        return;

    eHdmi = *(AX_HDMI_ID_E *)pPrivateData;

    SAMPLE_PRT("HDMI%d %s\n", eHdmi, enEvent == AX_HDMI_EVENT_HOTPLUG ? "HOTPLUG-IN" : "HOTPLUG-OUT");

    if (enEvent != AX_HDMI_EVENT_HOTPLUG) {
        return;
    }

    s32Ret = AX_VO_HDMI_Force_GetEDID(eHdmi, &stEdidData);
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_HDMI_Force_GetEDID failed, s32Ret = 0x%x\n", s32Ret);
        return;
    }

    SAMPLE_PRT("edid %s, length:%d\n", stEdidData.bEdidValid ? "valid" : "invalid", stEdidData.u32EdidLength);

    for (i = 0; (i < stEdidData.u32EdidLength) && stEdidData.bEdidValid; i++) {
        if (i % 16 == 0) {
            printf("\n\t\t\t");
        }

        printf("%.2hhx", stEdidData.u8Edid[i]);
    }

    printf("\n");
}

static AX_HDMI_ID_E eHdmi0 = AX_HDMI_ID0;
static AX_HDMI_ID_E eHdmi1 = AX_HDMI_ID1;

static AX_HDMI_CALLBACK_FUNC_T g_sTHdmi0CallBACK = {
    .pfnHdmiEventCallback = SAMPLE_HDMI_CallBack,
    .pPrivateData = &eHdmi0,
};

static AX_HDMI_CALLBACK_FUNC_T g_sTHdmi1CallBACK = {
    .pfnHdmiEventCallback = SAMPLE_HDMI_CallBack,
    .pPrivateData = &eHdmi1,
};

AX_VOID SAMPLE_VO_HDMI_HOTPLUG(AX_VOID)
{
    AX_S32 s32Ret = 0;

    s32Ret = AX_VO_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
        return;
    }

    s32Ret = AX_VO_HDMI_RegCallbackFunc(eHdmi0, &g_sTHdmi0CallBACK);
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_HDMI_RegCallbackFunc failed, s32Ret = 0x%x\n", s32Ret);
        goto exit1;
    }

    s32Ret = AX_VO_HDMI_RegCallbackFunc(eHdmi1, &g_sTHdmi1CallBACK);
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_HDMI_RegCallbackFunc failed, s32Ret = 0x%x\n", s32Ret);
        goto exit2;
    }

    while (!gLoopExit) {
        sleep(1);
    }

    AX_VO_HDMI_UnRegCallbackFunc(eHdmi1, &g_sTHdmi1CallBACK);
    SAMPLE_PRT("VO test Finished success!\n");
exit2:
    AX_VO_HDMI_UnRegCallbackFunc(eHdmi0, &g_sTHdmi0CallBACK);
exit1:
    AX_VO_Deinit();
}

AX_S32 SAMPLE_VO_PLAY(SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_BOOL bNeedWbc = AX_FALSE;
    AX_BOOL bMoveEn = AX_FALSE;
    AX_S32 i, j, s32Chns, s32Ret = 0;
    AX_U32 u32ChnFrameNr;
    AX_U32 u32Row, u32Col;
    AX_U32 u32ChnWidth, u32ChnHeight;
    AX_U32 u32LayerWidth = 0;
    AX_U32 u32LayerHeight = 0;
    AX_U64 u64BlkSize = 0, u64BlkNr;
    SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf;
    SAMPLE_VO_GRAPHIC_CONFIG_S *pstGraphicConf;
    SAMPLE_VO_CURSOR_CONFIG_S *pstCursorLayerConf;
    AX_VO_VIDEO_LAYER_ATTR_T *pstVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_T *pstChnThreadParam;
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T stLayerThreadParam[SAMPLE_VO_DEV_MAX] = {0,};
    SAMPLE_VO_VIDEOLAYER_THREAD_PARAM_T stLayerOutProcThreadParm[SAMPLE_VO_DEV_MAX] = {0,};
    SAMPLE_VO_WBC_THREAD_PARAM_T stWbcThreadParam[SAMPLE_VO_DEV_MAX] = {0,};
    SAMPLE_VO_CURSOR_MOVE_THREAD_PARAM_T stCursorMoveThreadParam;

    s32Ret = AX_VO_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
        return s32Ret;
    }

    pstVoConf->u32LayerNr = pstVoConf->u32VDevNr;
    if (pstVoConf->u32BindMode) {
        pstVoConf->u32LayerNr = 1;
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        if (pstVoConf->u32BindMode) {
            for (j = 0; j < pstVoConf->u32VDevNr; j++) {
                pstVoLayerConf->bindVoDev[j] = pstVoConf->stVoDev[j].u32VoDev;
                if (pstVoConf->stVoDev[j].bWbcEn) {
                    bNeedWbc = pstVoConf->stVoDev[j].bWbcEn;
                }
            }
        } else {
            pstVoLayerConf->bindVoDev[0] = pstVoDevConf->u32VoDev;
            bNeedWbc = pstVoDevConf->bWbcEn;
        }

        /* Forced settings */
        pstVoLayerConf->enVoMode = VO_MODE_1MUX;
        pstVoLayerConf->u64KeepChnPrevFrameBitmap0 = 0x0UL;
        pstVoLayerConf->u64KeepChnPrevFrameBitmap1 = 0x0UL;
        pstVoLayerAttr->enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR;
        pstVoLayerAttr->enSyncMode = AX_VO_LAYER_SYNC_NORMAL;
        pstVoLayerAttr->enWBMode = AX_VO_LAYER_WB_POOL;
        pstVoLayerAttr->u32DispatchMode = pstVoLayerAttr->bDisplayPreProcess ? AX_VO_LAYER_OUT_TO_FIFO :
                                          AX_VO_LAYER_OUT_TO_LINK;
        pstVoLayerAttr->enPartMode = AX_VO_PART_MODE_MULTI;

        u32LayerWidth = pstVoLayerAttr->stImageSize.u32Width;
        u32LayerHeight = pstVoLayerAttr->stImageSize.u32Height;

        SAMPLE_VO_WIN_INFO(u32LayerWidth, u32LayerHeight, pstVoLayerConf->enVoMode,
                           &u32Row, &u32Col, &u32ChnWidth, &u32ChnHeight);

        s32Chns = u32Row * u32Col;
        u32ChnFrameNr = pstVoLayerConf->u32ChnFrameNr;
        if (!u32ChnFrameNr || u32ChnFrameNr > SAMPLE_VO_FRAME_MAX) {
            u32ChnFrameNr = SAMPLE_VO_FRAME_MAX;
        }
        u64BlkSize = (AX_U64)u32ChnWidth * u32ChnHeight * 3 / 2;
        s32Ret = SAMPLE_VO_CREATE_POOL(u32ChnFrameNr, u64BlkSize, 512, &pstVoLayerConf->u32ChnPoolId);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", s32Ret);
            goto exit0;
        }

        u64BlkSize = (AX_U64)u32LayerWidth * u32LayerHeight * 3 / 2;
        u64BlkNr = bNeedWbc ? 8 : 5;
        s32Ret = SAMPLE_VO_CREATE_POOL(u64BlkNr, u64BlkSize, 512, &pstVoLayerConf->u32LayerPoolId);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, i:%d, s32Ret:0x%x\n", i, s32Ret);
            goto exit0;
        }

        pstVoLayerConf->s32Chns = s32Chns;
        pstVoLayerAttr->u32FifoDepth = pstVoLayerConf->u32FifoDepth;
        pstVoLayerAttr->u32PoolId = pstVoLayerConf->u32LayerPoolId;

        SAMPLE_PRT("u32LayerPoolId = %d, u32ChnPoolId = %d\n", pstVoLayerConf->u32LayerPoolId, pstVoLayerConf->u32ChnPoolId);
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        pstGraphicConf = &pstVoConf->stGraphicLayer[i];
        if (pstGraphicConf->u32FbNum) {
            AX_U32 u32Type = 1;

            pstGraphicConf->bindVoDev = pstVoDevConf->u32VoDev;

            for (j = 0; j < pstGraphicConf->u32FbNum; j += 1) {
                if (pstGraphicConf->u32FbNum > 1) {
                    u32Type = j ? 1 : 0;
                }

                s32Ret = SAMPLE_VO_FB_CONF(u32Type, &pstGraphicConf->stFbConf[j]);
                if (s32Ret) {
                    SAMPLE_PRT("SAMPLE_VO_FB_INIT failed, s32Ret:0x%x\n", s32Ret);
                    goto exit0;
                }
            }
        }
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstCursorLayerConf = &pstVoConf->stCursorLayer[i];
        if (pstCursorLayerConf->u32CursorLayerEn) {
            pstCursorLayerConf->bindVoDev = pstVoConf->stVoDev[i].u32VoDev;
            s32Ret = SAMPLE_VO_CURSOR_INIT(pstCursorLayerConf->u32X, pstCursorLayerConf->u32Y,
                                       pstCursorLayerConf->u32Width, pstCursorLayerConf->u32Height,
                                       pstCursorLayerConf->u32FBIndex);

            if (s32Ret) {
                SAMPLE_PRT("SAMPLE_VO_CURSOR_INIT failed, s32Ret:0x%x\n", s32Ret);
                goto exit0;
            }

            stCursorMoveThreadParam.stCursorInfo[i].u32StartX = pstCursorLayerConf->u32X;
            stCursorMoveThreadParam.stCursorInfo[i].u32StartY = pstCursorLayerConf->u32Y;
            stCursorMoveThreadParam.stCursorInfo[i].u32Width = pstCursorLayerConf->u32Width;
            stCursorMoveThreadParam.stCursorInfo[i].u32Height = pstCursorLayerConf->u32Height;
            stCursorMoveThreadParam.stCursorInfo[i].u32FbIndex = pstCursorLayerConf->u32FBIndex;
            stCursorMoveThreadParam.stCursorInfo[i].u32CursorLayerEn = pstCursorLayerConf->u32CursorLayerEn;
            stCursorMoveThreadParam.stCursorInfo[i].u32CursorMoveEn = pstCursorLayerConf->u32CursorMoveEn;

            bMoveEn |= pstCursorLayerConf->u32CursorMoveEn;
        }
    }

    s32Ret = SAMPLE_COMM_VO_StartVO(pstVoConf);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed, i:%d, s32Ret:0x%x\n", i, s32Ret);
        goto exit0;
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        if (pstVoDevConf->bWbcEn) {
            stWbcThreadParam[i].u32Wbc = pstVoDevConf->u32VoDev;
            stWbcThreadParam[i].u32FrameCnt = pstVoDevConf->u32WbcFrmaeNr;
            pthread_create(&stWbcThreadParam[i].ThreadID, NULL, SAMPLE_VO_WBC_THREAD, &stWbcThreadParam[i]);
        }
    }

    if (bMoveEn) {
        stCursorMoveThreadParam.u32ThreadForceStop = 0;
        pthread_create(&stCursorMoveThreadParam.ThreadID, NULL, SAMPLE_VO_CURSOR_MOVE_THREAD, &stCursorMoveThreadParam);
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        for (j = 0; j < pstVoLayerConf->s32Chns; j++) {
            pstChnThreadParam = &stLayerThreadParam[i].stVoChnThreadParm[j];
            pstChnThreadParam->u32LayerID = pstVoLayerConf->u32VoLayer;
            pstChnThreadParam->u32ChnID = j;
            pstChnThreadParam->u32FrameRate = pstVoLayerConf->u32ChnFrameRate;
            pstChnThreadParam->u32FrameMax = u32ChnFrameNr;
            strcpy(pstChnThreadParam->chnFileName, pstVoLayerConf->chnFileName);
            pstChnThreadParam->u32ThreadForceStop = 0;
            pstChnThreadParam->u32UserPoolId = pstVoLayerConf->u32ChnPoolId;
            pstChnThreadParam->bNeedCtrl = ((j == 0) && pstVoLayerConf->bNeedChnCtrl) ? AX_TRUE : AX_FALSE;
            pthread_create(&pstChnThreadParam->ThreadID, NULL, SAMPLE_VO_PLAY_THREAD, pstChnThreadParam);
            if ((pstVoLayerConf->u32ChnFrameOut - 1) == j) {
                stLayerThreadParam[i].u32LayerID = pstChnThreadParam->u32LayerID;
                stLayerThreadParam[i].u32ChnID = j;
                stLayerThreadParam[i].u32FrameCnt = 5;
                stLayerThreadParam[i].u32ThreadForceStop = 0;
                pthread_create(&stLayerThreadParam[i].ThreadID, NULL, SAMPLE_VO_GET_CHN_FRAME_THREAD, &stLayerThreadParam[i]);
            }
        }

        if (pstVoLayerAttr->bDisplayPreProcess) {
            pthread_attr_t stAttr;
            struct sched_param stParam;

            stLayerOutProcThreadParm[i].u32LayerID = pstVoLayerConf->u32VoLayer;
            stLayerOutProcThreadParm[i].u32ThreadForceStop = 0;

            pthread_attr_init(&stAttr);
            pthread_attr_setinheritsched(&stAttr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy(&stAttr, SCHED_FIFO);
            stParam.sched_priority = sched_get_priority_max(SCHED_FIFO);
            pthread_attr_setschedparam(&stAttr, &stParam);

            pthread_create(&stLayerOutProcThreadParm[i].ThreadID, &stAttr, SAMPLE_VO_LAYER_OUT_PROC_THREAD,
                           &stLayerOutProcThreadParm[i]);
            pthread_attr_destroy(&stAttr);
        }
    }

    while (!gLoopExit) {
        sleep(1);
    }

    for (i = 0; i < pstVoConf->u32VDevNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        if (pstVoDevConf->bWbcEn && stWbcThreadParam[i].ThreadID) {
            stWbcThreadParam[i].u32ThreadForceStop = 1;
            pthread_join(stWbcThreadParam[i].ThreadID, NULL);
            SAMPLE_PRT("wbc%d done\n", stWbcThreadParam[i].u32Wbc);
        }
    }

    if (bMoveEn) {
        stCursorMoveThreadParam.u32ThreadForceStop = 1;
        pthread_join(stCursorMoveThreadParam.ThreadID, NULL);
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        if (pstVoLayerAttr->bDisplayPreProcess) {
            stLayerOutProcThreadParm[i].u32ThreadForceStop = 0;
            pthread_join(stLayerOutProcThreadParm[i].ThreadID, NULL);
            SAMPLE_PRT("layer%d-out-proc-thread done\n", stLayerOutProcThreadParm[i].u32LayerID);
        }

        for (j = 0; j < pstVoLayerConf->s32Chns; j++) {
            pstChnThreadParam = &stLayerThreadParam[i].stVoChnThreadParm[j];
            if (pstChnThreadParam->ThreadID) {
                pstChnThreadParam->u32ThreadForceStop = 1;
                pthread_join(pstChnThreadParam->ThreadID, NULL);
                SAMPLE_PRT("layer%d-chn%d-thread send frame done\n", pstVoLayerConf->u32VoLayer, j);
                if (pstChnThreadParam->bNeedCtrl && pstChnThreadParam->ChnCtrlThreadID) {
                    pthread_join(pstChnThreadParam->ChnCtrlThreadID, NULL);
                    SAMPLE_PRT("layer%d-chn%d-ctrl-thread done\n", pstVoLayerConf->u32VoLayer, j);
                }
            }

            if (stLayerThreadParam[i].ThreadID) {
                stLayerThreadParam[i].u32ThreadForceStop = 1;
                pthread_join(stLayerThreadParam[i].ThreadID, NULL);
                SAMPLE_PRT("layer%d-thread get chn frame done\n", pstVoLayerConf->u32VoLayer);
            }
        }
    }

    SAMPLE_COMM_VO_StopVO(pstVoConf);

    SAMPLE_PRT("VO test Finished success!\n");

exit0:
    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        SAMPLE_VO_POOL_DESTROY(pstVoLayerConf->u32ChnPoolId);
        SAMPLE_VO_POOL_DESTROY(pstVoLayerConf->u32LayerPoolId);
    }

    AX_VO_Deinit();

    return s32Ret;
}

AX_S32 SAMPLE_VO_MEMCPY(AX_U32 u32Type)
{
    AX_S32 i, s32Ret;
    AX_U32 u32Width, u32Height, u32Size;
    AX_U8 *pSrcTmp, *pDstTmp;
    AX_U64 u64SrcPaddr, u64DstPaddr;
    AX_VOID *pSrcVaddr = NULL, *pDstVaddr = NULL;
    AX_VO_MEMCPY_T stSrc, stDst;

    u32Width = 4096;
    u32Height = 6000;
    u32Size = u32Width * u32Height;

    stSrc.stSize.u32Width = u32Width;
    stSrc.stSize.u32Height = u32Height;
    stDst.stSize = stSrc.stSize;

    s32Ret = AX_SYS_MemAlloc(&u64SrcPaddr, &pSrcVaddr, u32Size, 16, (const AX_S8 *)"VO-MEMCPY-SRC");
    if (s32Ret) {
        SAMPLE_PRT("alloc src memory failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    for (i = 0, pSrcTmp = pSrcVaddr; i < u32Size; i++) {
        pSrcTmp[i] = i % 256;
    }

    s32Ret = AX_SYS_MemAlloc(&u64DstPaddr, &pDstVaddr, u32Size, 16, (const AX_S8 *)"VO-MEMCPY-DST");
    if (s32Ret) {
        SAMPLE_PRT("alloc src memory failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    memset(pDstVaddr, 0, u32Size);

    SAMPLE_PRT("u64SrcPaddr:0x%llx, u64DstPaddr:0x%llx\n", u64SrcPaddr, u64DstPaddr);

    stSrc.u64PhyAddr = u64SrcPaddr;
    stDst.u64PhyAddr = u64DstPaddr;

    s32Ret = AX_VO_MemCpy_Open();
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_MemCpy_Open failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    if (u32Type) {
        s32Ret = AX_VO_MemCpy_2D(&stSrc, &stDst);
        if (s32Ret) {
            SAMPLE_PRT("AX_VO_MemCpy_2D failed, s32Ret = 0x%x\n", s32Ret);
            goto exit1;
        }

    } else {
        s32Ret = AX_VO_MemCpy_1D(u64SrcPaddr, u64DstPaddr, u32Size);
        if (s32Ret) {
            SAMPLE_PRT("AX_VO_MemCpy_1D failed, s32Ret = 0x%x\n", s32Ret);
            goto exit1;
        }
    }

    for (i = 0, pSrcTmp = pSrcVaddr, pDstTmp = pDstVaddr; i < u32Size; i++) {
        if (pSrcTmp[i] != pDstTmp[i]) {
            SAMPLE_PRT("pDstTmp[%d] != pDstTmp[%d], MemCpy failed\n", i, i);
            s32Ret = -1;
            break;
        }
    }

exit1:
    if (AX_VO_MemCpy_Close()) {
        SAMPLE_PRT("AX_VO_MemCpy_Close failed, s32Ret = 0x%x\n", s32Ret);
    }

exit0:
    if (pSrcVaddr)
        AX_SYS_MemFree(u64SrcPaddr, pSrcVaddr);

    if (pSrcVaddr)
        AX_SYS_MemFree(u64DstPaddr, pDstVaddr);

    SAMPLE_PRT("VO memcpy test Finished %s!\n", s32Ret ? "failed" : "success");

    return s32Ret;
}

