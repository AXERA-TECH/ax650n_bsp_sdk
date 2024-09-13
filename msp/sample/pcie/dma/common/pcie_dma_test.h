/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef PCIE_DMA_TEST_H__
#define PCIE_DMA_TEST_H__

//for PCIE_DMA_Test
typedef enum DMA_Test {
    PCIE_DMA_FileSendTest = 1,
    PCIE_DMA_FileRecvTest,
    PCIE_DMA_DataLoopbackTest,
    PCIE_DMA_BandwidthTest,
    PCIE_DMA_DMALinkListTest,
    PCIE_DMA_DMALinkListRecvTest,
    PCIE_DMA_DMAMultiTaskTest,
    PCIE_DMA_DMAMultiTaskRecvTest,
    PCIE_DMA_MultiTaskLoopbackTest,
    PCIE_DMA_MultiThreadLoopbackTest,
    PCIE_DMA_32ThreadLoopbackTest,
    PCIE_DMA_MultiThreadTestQuit,
    PCIE_DMA_QuitTest,
    PCIE_DMA_TestSuccess,
    PCIE_DMA_TestFail
} DMA_Test_T;


#define NOOP(...)

#define debug NOOP
#define info  printf
#define err   printf
#define warn  printf


AX_S32 MM_BufferInit(AX_S32 TargetId, AX_U8  **MM_VirtualAddr, AX_U64 *MM_PhyBaseAddr, AX_U64 Size);
AX_U8 *MM_Phy2VirtAddr(AX_U64 PhyAddr, AX_U64 BasePhyAddr, AX_U8 *BaseVirtAddr);
AX_S32 MM_BufferDeInit(AX_S32 TargetId, AX_U8 *MM_VirtualAddr, AX_U64 Size);
#endif