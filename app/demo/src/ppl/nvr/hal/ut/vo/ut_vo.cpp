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
#include <string.h>
#include <unistd.h>
#include <string>
#include "AppLogApi.h"
#include "ax_ivps_api.h"
#include "cmdline.h"
#include "help.hpp"

#define TAG "APP"
using namespace std;

static AX_BOOL ShowFrame(CVO *pVO, const char *image, AX_POOL pool);
static AX_POOL CreatePool(const VO_CHN_INFO_T &stChnInfo);
static AX_BOOL DestoryPool(AX_POOL &pool);

int main(int argc, char *argv[]) {
    cmdline::parser a;
    a.add<string>("image", 'i', "yuv image to display, must be 1920x1080", false, "/opt/data/1920x1080.nv12.yuv");
    a.add<AX_S32>("log", 'v', "log level", false, APP_LOG_WARN);
    a.parse_check(argc, argv);
    const string strImage = a.get<string>("image");
    if (::access(strImage.c_str(), F_OK | R_OK) < 0) {
        printf("image %s not exist!\n", strImage.c_str());
        return 1;
    }

    APP_SYS_INIT(0, AX_TRUE, AX_FALSE);
    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    constexpr AX_U32 videos[] = {1, 4, 9, 16, 32, 64};
    constexpr AX_U32 MAX_LAYOUT_NUM = sizeof(videos) / sizeof(videos[0]);
    AX_U64 nLayout = 0;
    VO_ATTR_T stAttr;
    CVO *pVO = {nullptr};
    VO_CHN_INFO_T stChnInfo[MAX_LAYOUT_NUM];
    AX_POOL pool[MAX_LAYOUT_NUM];

    /* calculate video channel rectage and create input vb pools */
    for (AX_U32 i = 0; i < MAX_LAYOUT_NUM; ++i) {
        stChnInfo[i] = InitLayout(1920, 1080, videos[i]);
        for (AX_U32 j = 0; j < stChnInfo[i].nCount; ++j) {
            stChnInfo[i].arrFps[j] = 60;
        }

        pool[i] = CreatePool(stChnInfo[i]);
        if (AX_INVALID_POOLID == pool[i]) {
            goto __EXIT__;
        }
    }

    stAttr.dev = 0;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = AX_VO_OUTPUT_1080P60;
    stAttr.nLayerDepth = 3;
    stAttr.nBgClr = 0x0;
    stAttr.stChnInfo = stChnInfo[0];
    pVO = CVO::CreateInstance(stAttr);
    if (!pVO) {
        goto __EXIT__;
    }

    if (!pVO->Start()) {
        goto __EXIT__;
    }

    while (1) {
        if (IS_APP_QUIT()) {
            break;
        }

        AX_U32 n = (++nLayout % MAX_LAYOUT_NUM);
        const VO_CHN_INFO_T &stChn = stChnInfo[n];

        if (!pVO->UpdateChnInfo(stChn)) {
            break;
        }

        if (!ShowFrame(pVO, strImage.c_str(), pool[n])) {
            break;
        }

        sleep(1);
    }

__EXIT__:
    if (pVO) {
        pVO->Stop();
        pVO->Destory();
    }

    for (auto &&m : pool) {
        DestoryPool(m);
    }

    return 0;
}

static AX_POOL CreatePool(const VO_CHN_INFO_T &stChnInfo) {
    AX_POOL_CONFIG_T stPoolCfg;
    memset(&stPoolCfg, 0, sizeof(stPoolCfg));
    stPoolCfg.MetaSize = 4096;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    stPoolCfg.BlkSize = ALIGN_UP(stChnInfo.arrChns[0].stRect.u32Width, 16) * stChnInfo.arrChns[0].stRect.u32Height * 3 / 2;
    stPoolCfg.BlkCnt = stChnInfo.nCount;

    return AX_POOL_CreatePool(&stPoolCfg);
}

static AX_BOOL DestoryPool(AX_POOL &pool) {
    if (AX_INVALID_POOLID == pool) {
        return AX_TRUE;
    }

    AX_S32 ret = AX_POOL_DestroyPool(pool);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_POOL_DestroyPool(pool %d) fail, ret = 0x%x", pool, ret);
        return AX_FALSE;
    }

    pool = AX_INVALID_POOLID;
    return AX_TRUE;
}

static AX_BOOL ShowFrame(CVO *pVO, const char *image, AX_POOL pool) {
    const VO_ATTR_T &stAttr = pVO->GetAttr();

    CAXFrame axSrcFrame;
    AX_VIDEO_FRAME_T &src = axSrcFrame.stFrame.stVFrame.stVFrame;
    src.u32Width = 1920;
    src.u32Height = 1080;
    src.u32PicStride[0] = src.u32Width;
    src.u32PicStride[1] = src.u32Width;
    src.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    src.u32FrameSize = axSrcFrame.GetFrameSize();
    AX_S32 ret = AX_SYS_MemAlloc(&src.u64PhyAddr[0], (AX_VOID **)&src.u64VirAddr[0], src.u32FrameSize, 128, (const AX_S8 *)"videoimg");
    if (0 != ret) {
        LOG_M_E(TAG, "%s: AX_SYS_MemAlloc(%d) fail, ret = 0x%x", __func__, src.u32FrameSize, ret);
        return AX_FALSE;
    } else {
        src.u64PhyAddr[1] = src.u64PhyAddr[0] + src.u32PicStride[0] * src.u32Height;
        src.u64VirAddr[1] = src.u64VirAddr[0] + src.u32PicStride[0] * src.u32Height;
    }

    if (!axSrcFrame.LoadFile(image)) {
        AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID *)src.u64VirAddr[0]);
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < stAttr.stChnInfo.nCount; ++i) {
        CAXFrame axDstFrame;
        AX_VIDEO_FRAME_T &dst = axDstFrame.stFrame.stVFrame.stVFrame;
        dst.u32Width = stAttr.stChnInfo.arrChns[i].stRect.u32Width;
        dst.u32Height = stAttr.stChnInfo.arrChns[i].stRect.u32Height;
        dst.u32PicStride[0] = ALIGN_UP(dst.u32Width, 16);
        dst.u32PicStride[1] = ALIGN_UP(dst.u32Width, 16);
        dst.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        dst.u32FrameSize = axDstFrame.GetFrameSize();

        AX_BLK blkId = AX_POOL_GetBlock(pool, dst.u32FrameSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            LOG_M_E(TAG, "%s: AX_POOL_GetBlock(pool %d blkSize %d) fail", __func__, pool, dst.u32FrameSize);
            return AX_FALSE;
        } else {
            dst.u32BlkId[0] = blkId;
        }

        dst.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        if (0 == dst.u64PhyAddr[0]) {
            LOG_M_E(TAG, "%s: AX_POOL_Handle2PhysAddr(blkId 0x%x) fail", __func__, blkId);
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID *)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        dst.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        if (0 == dst.u64VirAddr[0]) {
            LOG_M_E(TAG, "%s: AX_POOL_GetBlockVirAddr(blkId 0x%x) fail", __func__, blkId);
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID *)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        dst.u64PhyAddr[1] = dst.u64PhyAddr[0] + dst.u32PicStride[0] * dst.u32Height;
        dst.u64VirAddr[1] = dst.u64VirAddr[0] + dst.u32PicStride[0] * dst.u32Height;

        AX_IVPS_ASPECT_RATIO_T aspect;
        memset(&aspect, 0, sizeof(aspect));
        aspect.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;
        ret = AX_IVPS_CropResizeTdp(&src, &dst, &aspect);
        if (0 != ret) {
            LOG_M_E(TAG, "%s: AX_IVPS_CropResizeTdp() fail, ret = 0x%x", __func__, ret);
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID *)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        LOG_M_N(TAG, "SendFrame to voChn %d", i);
        AX_BOOL bOpr = pVO->SendFrame(i, axDstFrame, -1);
        if (!bOpr) {
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID *)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        AX_POOL_ReleaseBlock(blkId);
    }

    AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID *)src.u64VirAddr[0]);
    return AX_TRUE;
}
