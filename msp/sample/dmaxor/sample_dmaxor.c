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
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include "ax_base_type.h"
#include "ax_sys_api.h"
#include "ax_dmaxor_api.h"
#include "ax_thread_signal.h"

#define DMAXOR_BLOCK 1
#define DMAXOR_NOBLOCK 0

#ifdef DEBUG
    #define PK_DBG(fmt,arg...) printf("DMAXOR: %s,%d "fmt,__func__,__LINE__,##arg)
#else
    #define PK_DBG(fmt,arg...)
#endif
#define PK_INFO(fmt,arg...) printf("DMAXOR: %s,%d "fmt,__func__,__LINE__,##arg)
#define PK_ERR(fmt,arg...) printf("DMAXOR Err: %s,%d "fmt,__func__,__LINE__,##arg)

static ThreadSignal g_signal;

typedef struct {
    AX_U64 *aVir;
    AX_U64 aPhy;
    AX_U64 u32Size;
} ADDR_BUF_T;

static int getAddr(ADDR_BUF_T *addrBuf, ADDR_BUF_T *addrBuf2, AX_U32 u32Size)
{
    int i, ret;
    AX_U64 phyAddr;
    void *src_test_virt = NULL;

    for (i = 0; i < 16; i++) {
        ret = AX_SYS_MemAlloc(&phyAddr, &src_test_virt, 0x1000, 0x1, (AX_S8 *)"DMAXOR_test");
        if (ret < 0) {
            PK_ERR("getAddr alloc failed %d\n", ret);
            return -1;
        }
        addrBuf[i].aPhy = phyAddr;
        addrBuf[i].aVir = src_test_virt;

        ret = AX_SYS_MemAlloc(&phyAddr, &src_test_virt, 0x1000, 0x1, (AX_S8 *)"DMAXOR_test");
        if (ret < 0) {
            PK_ERR("getAddr alloc failed %d\n", ret);
            return -1;
        }
        addrBuf2[i].aPhy = phyAddr;
        addrBuf2[i].aVir = src_test_virt;
    }
    return 0;
}

static void freeAddr(ADDR_BUF_T *addrBuf, ADDR_BUF_T *addrBuf2)
{
    int i;
    for (i = 0; i < 16; i++) {
        AX_SYS_MemFree(addrBuf[i].aPhy, addrBuf[i].aVir);
        AX_SYS_MemFree(addrBuf2[i].aPhy, addrBuf2[i].aVir);
    }
}

void AX_DMAXOR_Test_Signal(AX_DMAXOR_XFER_STAT_T *pXferStat, void *pCbArg)
{
    AX_DMAXOR_XFER_STAT_T *tXferStat = (AX_DMAXOR_XFER_STAT_T *)pCbArg;

    memcpy(tXferStat, pXferStat, sizeof(AX_DMAXOR_XFER_STAT_T));
    AX_ThreadSignal_Signal(&g_signal);
}

static int AX_DMAXOR_FUNC_MEMORY_INIT_SYNC(ADDR_BUF_T *u64PhySrcBuf, ADDR_BUF_T *u64PhyDstBuf)
{
    AX_S32 fd, ret = 0;
    AX_DMAXOR_MSG_T tDmaMsg;
    AX_DMAXOR_DESC_MEM_T pDescBuf;
    AX_DMAXOR_XFER_STAT_T tXferStat;
    memset(&tXferStat, 0, sizeof(AX_DMAXOR_XFER_STAT_T));

    if ((fd = AX_DMAXOR_Open(DMAXOR_BLOCK)) < 0) {
        PK_DBG("open error\n");
        return -1;
    }

    tDmaMsg.u32DescNum = 1;
    tDmaMsg.eDmaMode = AX_DMAXOR_MEMORY_INIT;
    tDmaMsg.pDescBuf = (void *)&pDescBuf;
    tDmaMsg.pCbArg = NULL;
    tDmaMsg.pfnCallBack = NULL;
    pDescBuf.u32InitVal = 0x12345678;
    pDescBuf.u64PhyDst = u64PhySrcBuf[0].aPhy;
    pDescBuf.u32Size = 0x1000;

    tXferStat.s32Id = AX_DMAXOR_Cfg(fd, &tDmaMsg);
    if (tXferStat.s32Id < 1) {
        PK_ERR("AX_DMAXOR_CFG_FAIL tXferStat.s32Id %d!\n", tXferStat.s32Id);
        goto end;
    }
    ret = AX_DMAXOR_Start(fd, tXferStat.s32Id);
    if (ret) {
        PK_ERR("AX_DMAXOR_Start failed!\n");
        goto end;
    }
    ret = AX_DMAXOR_Waitdone(fd, &tXferStat, 5000);
    if (ret) {
        PK_ERR("AX_DMAXOR_Waitdone failed!\n");
        goto end;
    }
    PK_DBG(" ok \n");
end:
    AX_DMAXOR_Close(fd);
    return ret;
}

static int AX_DMAXOR_FUNC_MEMORY_INIT_ASYNC(ADDR_BUF_T *u64PhySrcBuf, ADDR_BUF_T *u64PhyDstBuf)
{
    AX_S32 fd, ret = 0;
    AX_DMAXOR_MSG_T tDmaMsg;
    AX_DMAXOR_DESC_MEM_T pDescBuf[2];
    AX_DMAXOR_XFER_STAT_T tXferStat;
    memset(&tXferStat, 0, sizeof(AX_DMAXOR_XFER_STAT_T));

    if ((fd = AX_DMAXOR_Open(DMAXOR_NOBLOCK)) < 0) {
        PK_DBG("open error\n");
        return -1;
    }
    tDmaMsg.u32DescNum = 2;
    tDmaMsg.eDmaMode = AX_DMAXOR_MEMORY_INIT;
    tDmaMsg.pDescBuf = (void *)pDescBuf;
    tDmaMsg.pCbArg = &tXferStat;
    tDmaMsg.pfnCallBack = AX_DMAXOR_Test_Signal;
    pDescBuf[0].u32InitVal = 0x12345678;
    pDescBuf[0].u64PhyDst = u64PhySrcBuf[0].aPhy;
    pDescBuf[0].u32Size = 0x1000;
    pDescBuf[1].u32InitVal = 0x90abcdef;
    pDescBuf[1].u64PhyDst = u64PhySrcBuf[1].aPhy;
    pDescBuf[1].u32Size = 0x1000;

    tXferStat.s32Id = AX_DMAXOR_Cfg(fd, &tDmaMsg);
    if (tXferStat.s32Id < 1) {
        PK_ERR("AX_DMAXOR_CFG_FAIL tXferStat.s32Id %d!\n", tXferStat.s32Id);
        goto end;
    }
    ret = AX_DMAXOR_Start(fd, tXferStat.s32Id);
    if (ret) {
        PK_ERR("AX_DMAXOR_Start failed!\n");
        goto end;
    }
    ret = AX_ThreadSignal_Wait(&g_signal, 5000);
    if (ret) {
        PK_ERR("AX_ThreadSignal_Wait timeout\n");
        goto end;
    }
end:
    AX_DMAXOR_Close(fd);
    return ret;
}

static int AX_DMAXOR_FUNC_XOR_WB(ADDR_BUF_T *u64PhySrcBuf, ADDR_BUF_T *u64PhyDstBuf, int lliNum, bool bIsSync)
{
    AX_S32 i, j, fd, ret = 0;
    AX_DMAXOR_MSG_T tDmaMsg;
    AX_DMAXOR_DESC_XOR_T *pDescBuf;
    AX_DMAXOR_XFER_STAT_T tXferStat;
    memset(&tXferStat, 0, sizeof(AX_DMAXOR_XFER_STAT_T));

    if ((fd = AX_DMAXOR_Open(bIsSync)) < 0) {
        PK_DBG("open error\n");
        return -1;
    }

    pDescBuf = malloc(sizeof(AX_DMAXOR_DESC_XOR_T) * lliNum);
    if (!pDescBuf) {
        PK_ERR("\n");
        AX_DMAXOR_Close(fd);
        return -1;
    }
    tDmaMsg.u32DescNum = lliNum;
    tDmaMsg.eDmaMode = AX_DMAXOR_XOR_WB;
    tDmaMsg.pDescBuf = pDescBuf;
    tDmaMsg.pCbArg = &tXferStat;
    tDmaMsg.pfnCallBack = AX_DMAXOR_Test_Signal;
    for (i = 0; i < lliNum; i++) {
        for (j = 0; j < 16; j++) {
            pDescBuf->u64PhySrcBuf[j] = u64PhySrcBuf[j].aPhy;
        }
        pDescBuf->u64PhyDst = u64PhyDstBuf[i].aPhy;
        pDescBuf->u32Size = 0x1000;
        pDescBuf->u8SrcCnt = 2;
        pDescBuf++;
    }
    tXferStat.s32Id = AX_DMAXOR_Cfg(fd, &tDmaMsg);
    if (tXferStat.s32Id < 1) {
        PK_ERR("AX_DMAXOR_CFG_FAIL tXferStat.s32Id %d!\n", tXferStat.s32Id);
        goto end;
    }
    ret = AX_DMAXOR_Start(fd, tXferStat.s32Id);
    if (ret) {
        PK_ERR("AX_DMAXOR_Start failed!\n");
        goto end;
    }
    if (bIsSync) {
        PK_DBG("AX_DMAXOR_Waitdone\n");
        ret = AX_DMAXOR_Waitdone(fd, &tXferStat, 5000);
        if (ret) {
            PK_ERR("AX_DMAXOR_Waitdone failed!\n");
            goto end;
        }
    } else {
        PK_DBG("NO-BLOCK\n");
        ret = AX_ThreadSignal_Wait(&g_signal, 5000);
        if (ret) {
            PK_ERR("AX_ThreadSignal_Wait failed!\n");
            goto end;
        }
    }
    PK_DBG(" ok \n");
end:
    free((AX_DMAXOR_DESC_XOR_T *)tDmaMsg.pDescBuf);
    AX_DMAXOR_Close(fd);
    return ret;
}

static int AX_DMAXOR_FUNC_XOR_CHECK(ADDR_BUF_T *u64PhySrcBuf, ADDR_BUF_T *u64PhyDstBuf, int lliNum, bool bIsSync)
{
    AX_S32 i, j, fd, ret = 0;
    AX_DMAXOR_MSG_T tDmaMsg;
    AX_DMAXOR_DESC_XOR_T *pDescBuf;
    AX_DMAXOR_XFER_STAT_T tXferStat;
    memset(&tXferStat, 0, sizeof(AX_DMAXOR_XFER_STAT_T));

    if ((fd = AX_DMAXOR_Open(bIsSync)) < 0) {
        PK_DBG("open error\n");
        return -1;
    }

    pDescBuf = malloc(sizeof(AX_DMAXOR_DESC_XOR_T) * lliNum);
    if (!pDescBuf) {
        PK_ERR("\n");
        AX_DMAXOR_Close(fd);
        return -1;
    }
    tDmaMsg.u32DescNum = lliNum;
    tDmaMsg.eDmaMode = AX_DMAXOR_XOR_CALI;
    tDmaMsg.pDescBuf = pDescBuf;
    tDmaMsg.pCbArg = &tXferStat;
    tDmaMsg.pfnCallBack = AX_DMAXOR_Test_Signal;
    for (i = 0; i < lliNum; i++) {
        for (j = 0; j < 15; j++) {
            pDescBuf->u64PhySrcBuf[j] = u64PhySrcBuf[j].aPhy;
        }
        pDescBuf->u64PhySrcBuf[2] = u64PhyDstBuf[i].aPhy;
        pDescBuf->u64PhyDst = u64PhyDstBuf[i].aPhy;
        pDescBuf->u32Size = 0x1000;
        pDescBuf->u8SrcCnt = 3;
        pDescBuf++;
    }
    tXferStat.s32Id = AX_DMAXOR_Cfg(fd, &tDmaMsg);
    if (tXferStat.s32Id < 1) {
        PK_ERR("AX_DMAXOR_CFG_FAIL tXferStat.s32Id %d!\n", tXferStat.s32Id);
        goto end;
    }
    ret = AX_DMAXOR_Start(fd, tXferStat.s32Id);
    if (ret) {
        PK_ERR("AX_DMAXOR_Start failed!\n");
        goto end;
    }
    if (bIsSync) {
        PK_DBG("AX_DMAXOR_Waitdone\n");
        ret = AX_DMAXOR_Waitdone(fd, &tXferStat, 5000);
        if (ret) {
            PK_ERR("AX_DMAXOR_Waitdone failed!\n");
            goto end;
        }
    } else {
        PK_DBG("NO-BLOCK\n");
        ret = AX_ThreadSignal_Wait(&g_signal, 5000);
        if (ret) {
            PK_ERR("AX_ThreadSignal_Wait failed!\n");
            goto end;
        }
    }
    if (tXferStat.u64XorCali) {
        PK_ERR("u64XorCali fail \n");
    } else {
        PK_DBG("u64XorCali ok \n");
    }
end:
    free((AX_DMAXOR_DESC_XOR_T *)tDmaMsg.pDescBuf);
    AX_DMAXOR_Close(fd);
    return ret;
}

AX_S32 main()
{
    AX_S32 ret;
    ADDR_BUF_T u64PhySrcBuf[16];
    ADDR_BUF_T u64PhyDstBuf[16];

    AX_SYS_Init();
    ret = getAddr(u64PhySrcBuf, u64PhyDstBuf, 0x2000);
    if (ret) {
        PK_ERR("Error!");
        return -1;
    }
    AX_ThreadSignal_Init(&g_signal, 1);
    ret |= AX_DMAXOR_FUNC_MEMORY_INIT_SYNC(u64PhySrcBuf, u64PhyDstBuf);
    ret |= AX_DMAXOR_FUNC_MEMORY_INIT_ASYNC(u64PhySrcBuf, u64PhyDstBuf);
    ret |= AX_DMAXOR_FUNC_XOR_WB(u64PhySrcBuf, u64PhyDstBuf, 1, DMAXOR_BLOCK);
    ret |= AX_DMAXOR_FUNC_XOR_WB(u64PhySrcBuf, u64PhyDstBuf, 1, DMAXOR_NOBLOCK);
    ret |= AX_DMAXOR_FUNC_XOR_CHECK(u64PhySrcBuf, u64PhyDstBuf, 1, DMAXOR_BLOCK);
    ret |= AX_DMAXOR_FUNC_XOR_CHECK(u64PhySrcBuf, u64PhyDstBuf, 1, DMAXOR_NOBLOCK);
    AX_ThreadSignal_Destroy(&g_signal);
    freeAddr(u64PhySrcBuf, u64PhyDstBuf);
    AX_SYS_Deinit();
    if (ret)
        PK_ERR("test failed!\n");
    else
        PK_INFO("test successful.\n");
    return 0;
}
