/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#include "ax_base_type.h"
#include "ax_pcie_dma_api.h"
#include "ax_pcie_msg_api.h"

#include "pcie_dma_test.h"


#define THREAD_CNT 32
// #define PCIE_PORT_CNT 5
#define PCIE_PORT_CNT (THREAD_CNT + 1)
#define PCIE_PORT_BASE_NUM 10
#define TEST_PORT (PCIE_PORT_BASE_NUM + 0)
#define TEST_THREAD1_PORT (PCIE_PORT_BASE_NUM + 1)
#define TEST_THREAD2_PORT (PCIE_PORT_BASE_NUM + 2)
#define TEST_THREAD3_PORT (PCIE_PORT_BASE_NUM + 3)
#define TEST_THREAD4_PORT (PCIE_PORT_BASE_NUM + 4)

#define RECV_FILE   "./pcie_recv_file"
#define SEND_FILE   "./pcie_send_file"
#define LP_RECV_FILE "./pcie_lp_test_recv_file"
#define MT_RECV_FILE1 "./multi_thread_test_recv_file1"
#define MT_RECV_FILE2 "./multi_thread_test_recv_file2"
#define MT_RECV_FILE3 "./multi_thread_test_recv_file3"
#define MT_RECV_FILE4 "./multi_thread_test_recv_file4"

typedef struct AX_PCIE_THREAD {
    AX_S32 s32Id;
    AX_S32 s32MaxLoop;
    AX_S32 s32TargetId;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;
    AX_U64 DmaBufferSize;
    AX_U8  u8ThreadMsgPort;
} AX_PCIE_THREAD_T;

typedef struct AX_PCIE_MULTCARD {
    AX_PCIE_THREAD_T stThread[THREAD_CNT];
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;
    AX_U64 DmaBufferSize;
    AX_S32 s32MaxLoop;
    AX_S32 s32TargetId;
} AX_PCIE_MULTCARD_T;


AX_S32 Exit = 0;
AX_S32 g_CtrlCount = 0;

AX_S32 RecvFileTest(AX_S32 Target, AX_S32 Port, AX_U64 MM_PhyBaseAddr, AX_U8  *MM_VirtualAddr)
{
    AX_U32 DataSize;
    AX_U64 PhyAddr;
    AX_U8 *VirtAddr;
    AX_S32 DataFd;
    AX_S32 Ret = 0;

    info("=========== recv file test ===========\n");

    Ret = AX_PCIe_WaitWriteDoneMsg(Target, Port, &PhyAddr, &DataSize, -1);
    if (Ret < 0) {
        return -1;
    }
    VirtAddr = MM_Phy2VirtAddr(PhyAddr, MM_PhyBaseAddr, MM_VirtualAddr);
    // err("video buffer virtual addr:%lx\n", (long)(VirtAddr));
    // err("data in mm buffer: %s\n", VirtAddr);

    remove(RECV_FILE);
    // open file to save received data
    DataFd = open(RECV_FILE, O_RDWR | O_CREAT);
    if (DataFd < 0) {
        err("open file %s failed!\n", RECV_FILE);
        return -1;
    }

    lseek(DataFd, 0, SEEK_SET);

    Ret = write(DataFd, VirtAddr, DataSize);
    if (Ret != DataSize) {
        err("write file %s failed!\n", RECV_FILE);
        Ret = -1;
        goto RET;
    }
    memset(VirtAddr, 0, DataSize);
    info("save %d bytes received date to %s success!\n", Ret, RECV_FILE);

    //send a read_done message to ep
    Ret = AX_PCIe_SendReadDoneMsg(Target, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send read_done msg success\n");

    Ret = 0;
    info("=========== recv file test success! ===========\n");

RET:
    close(DataFd);
    return Ret;
}


AX_S32 SendFileTest(AX_S32 Target, AX_S32 Port, AX_U8  *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr)
{
    AX_S32 SendDataSize;
    AX_S32 SendDataFd;
    AX_S32 ReadSize;
    AX_S32 Ret = 0;

    info("=========== send file test ===========\n");

    // open test file to transfer
    SendDataFd = open(SEND_FILE, O_RDWR);
    if (SendDataFd < 0) {
        err("open file %s failed!\n", SEND_FILE);
        return -1;
    }

    SendDataSize = lseek(SendDataFd, 0, SEEK_END);
    lseek(SendDataFd, 0, SEEK_SET);
    info("open %s, size:%d bytes\n", SEND_FILE, SendDataSize);

    Ret = read(SendDataFd, MM_VirtualAddr, SendDataSize);
    if (Ret <= 0) {
        err("read file %s failed!\n", SEND_FILE);
        Ret = -1;
        goto RET;
    }

    //send a data ready message to ep
    Ret = AX_PCIe_SendDataReadyMsg(Target, Port, MM_PhyBaseAddr, SendDataSize);
    if (Ret < 0) {
        err("send PCIE_DMA_DATA_READY msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send PCIE_DMA_DATA_READY msg success\n");

    ReadSize = AX_PCIe_WaitReadDoneMsg(Target, Port, -1);
    if (ReadSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto RET;
    } else if (ReadSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto RET;
    }

    Ret = 0;
    info("=========== send file test success! ===========\n");

RET:
    close(SendDataFd);
    return Ret;
}


AX_S32 LoopbackDataTest(AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile, AX_U64 MM_PhyBaseAddr,
                        AX_U8  *MM_VirtualAddr)
{
    AX_U32 DataSize;
    AX_U64 PhyAddr;
    AX_U8 *VirtAddr;
    AX_S32 DataFd = -1;
    AX_S32 Ret = 0;
    AX_S32 ReadSize;

    info("=========== test data loopback ===========\n");
    do {
        Ret = AX_PCIe_WaitWriteDoneMsg(Target, Port, &PhyAddr, &DataSize, -1);
        if (Ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
    } while (Ret < 0);

    VirtAddr = MM_Phy2VirtAddr(PhyAddr, MM_PhyBaseAddr, MM_VirtualAddr);

    info("recv %d bytes success!\n", DataSize);

    if(RecvDataFile) {
        remove(RecvDataFile);
        // open test file to save received data
        DataFd = open(RecvDataFile, O_RDWR | O_CREAT);
        if (DataFd < 0) {
            err("open file %s failed!\n", RecvDataFile);
            return -1;
        }

        lseek(DataFd, 0, SEEK_SET);

        Ret = write(DataFd, VirtAddr, DataSize);
        if (Ret != DataSize) {
            err("write file %s failed!\n", RecvDataFile);
            Ret = -1;
            goto RET;
        }

        info("save %d bytes received date to %s success!\n", Ret, RecvDataFile);
    }

    //send a read_done message to ep
    Ret = AX_PCIe_SendReadDoneMsg(Target, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    } else {
        err("send read_done msg success\n");
    }

    do {
        ReadSize = AX_PCIe_WaitReadDoneMsg(Target, Port, -1);
        if (ReadSize < 0) {
            if (errno == EINTR) {
                continue;
            }
            err("wait read done msg failed \n");
            Ret = -1;
            goto RET;
        } else if (ReadSize == 0) {
            err("ep read data failed\n");
            Ret = -1;
            goto RET;
        }
    } while (ReadSize < 0);

    Ret = 0;
    info("=========== loopback data success! ===========\n");

RET:
    if(DataFd != -1)
        close(DataFd);
    return Ret;
}


AX_S32 BandwidthTest(AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile, AX_U64 MM_PhyBaseAddr,
                     AX_U8  *MM_VirtualAddr)
{
    AX_U32 DataSize;
    AX_U64 PhyAddr;
    AX_S32 Ret = 0;
    AX_S32 ReadSize;

    Ret = AX_PCIe_WaitWriteDoneMsg(Target, Port, &PhyAddr, &DataSize, -1);
    if (Ret < 0) {
        return -1;
    }

    info("host received %d bytes success!\n", DataSize);

    //send a read_done message to ep
    Ret = AX_PCIe_SendReadDoneMsg(Target, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    }

    ReadSize = AX_PCIe_WaitReadDoneMsg(Target, Port, -1);
    if (ReadSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto RET;
    } else if (ReadSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto RET;
    }

    info("host send %d bytes success!\n", ReadSize);

    Ret = 0;

RET:
    return Ret;
}

AX_S32 LoopbackTest(AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile, AX_U64 MM_PhyBaseAddr,
                        AX_U8  *MM_VirtualAddr)
{
    AX_U32 DataSize;
    AX_U64 PhyAddr;
    AX_U8 *VirtAddr;
    AX_S32 DataFd = -1;
    AX_S32 Ret = 0;
    AX_S32 ReadSize;

    debug("=========== loopback test ===========\n");

    Ret = AX_PCIe_WaitWriteDoneMsg(Target, Port, &PhyAddr, &DataSize, -1);
    if (Ret < 0) {
        return -1;
    }
    VirtAddr = MM_Phy2VirtAddr(PhyAddr, MM_PhyBaseAddr, MM_VirtualAddr);

    debug("get write_done msg success\n");
    info("recv %d bytes success!\n", DataSize);

    //send a read_done message to ep
    Ret = AX_PCIe_SendReadDoneMsg(Target, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    } else {
        debug("send read_done msg success\n");
    }

    ReadSize = AX_PCIe_WaitReadDoneMsg(Target, Port, -1);
    if (ReadSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto RET;
    } else if (ReadSize == 0) {
        err("ep read data failed\n");
        RecvDataFile = "./pcie_dma_send_data";
    }
    debug("get read_done msg success\n");

    if(RecvDataFile) {
        remove(RecvDataFile);
        // open test file to save received data
        DataFd = open(RecvDataFile, O_RDWR | O_CREAT);
        if (DataFd < 0) {
            err("open file %s failed!\n", RecvDataFile);
            return -1;
        }

        lseek(DataFd, 0, SEEK_SET);

        Ret = write(DataFd, VirtAddr, DataSize);
        if (Ret != DataSize) {
            err("write file %s failed!\n", RecvDataFile);
            Ret = -1;
            goto RET;
        }

        info("save %d bytes received date to %s success!\n", Ret, RecvDataFile);
    }

    if(ReadSize == 0) {
        Ret = -1;
        err("=========== loopback data failed! ===========\n");
    } else {
        Ret = 0;
        debug("=========== loopback data success! ===========\n");
    }

RET:
    if(DataFd != -1)
        close(DataFd);

    return Ret;
}

AX_VOID *PCIe_Dma_32Thread(AX_VOID *pArg)
{
    AX_S32 Ret = 0;
    AX_U32 TestCnt = 1;
    AX_PCIE_THREAD_T *pstThread = (AX_PCIE_THREAD_T *)pArg;
    AX_S32 Target = pstThread->s32TargetId;
    AX_U8  *MM_VirtualAddr = pstThread->MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr = pstThread->MM_PhyBaseAddr;

    info("start pcie dma thread %d\n", pstThread->s32Id);

    while (Exit != 1) {
        info("\nthread %d pressure test number: %d\n", pstThread->s32Id, TestCnt);
        // data flow: ep --> rc --> ep
        AX_PCIe_SendUserCmd(Target, pstThread->u8ThreadMsgPort, PCIE_DMA_32ThreadLoopbackTest);
        Ret = LoopbackTest(Target, pstThread->u8ThreadMsgPort, NULL, MM_PhyBaseAddr,
                               MM_VirtualAddr);
        if (Ret < 0) {
            err("loopback data thread%d failed!\n", pstThread->s32Id);
            return NULL;
        }

        TestCnt++;
    }
    AX_PCIe_SendUserCmd(Target, pstThread->u8ThreadMsgPort, PCIE_DMA_MultiThreadTestQuit);
    pthread_exit(NULL);
}

AX_S32 Sample_PCIe_DmaFileReceivingTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_FileSendTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_FileSendTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_FileSendTest) msg success\n");
        Ret = RecvFileTest(Target, TEST_PORT, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("recv file test failed!\n");
        }
    }

    return Ret;
}

AX_S32 Sample_PCIe_DmaFileSendingTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_FileRecvTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_FileRecvTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_FileRecvTest) msg success\n");
        Ret = SendFileTest(Target, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr);
        if (Ret < 0) {
            err("send file test failed!\n");
        }
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaDataLoopbackTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_DataLoopbackTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_DataLoopbackTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_DataLoopbackTest) msg success\n");
        Ret = LoopbackDataTest(Target, TEST_PORT, LP_RECV_FILE, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("loopback data test failed!\n");
        }
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaLinkListSendTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_DMALinkListTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_DMALinkListTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_DMALinkListTest) msg success\n");
        Ret = SendFileTest(Target, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr);
        if (Ret < 0) {
            err("send file test failed!\n");
        }
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaLinkRecvTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_DMALinkListRecvTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_DMALinkListRecvTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_DMALinkListRecvTest) msg success\n");
        Ret = RecvFileTest(Target, TEST_PORT, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("recv file test failed!\n");
        }
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaMultiTaskSendTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_DMAMultiTaskTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_DMAMultiTaskTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_DMAMultiTaskTest) msg success\n");
        Ret = SendFileTest(Target, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr);
        if (Ret < 0) {
            err("send file test failed!\n");
        }
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaMultiTaskRecvTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_DMAMultiTaskRecvTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_DMAMultiTaskRecvTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_DMAMultiTaskRecvTest) msg success\n");
        Ret = RecvFileTest(Target, TEST_PORT, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("recv file test failed!\n");
        }
    }
    return Ret;
}

AX_S32 Sample_PCIe_Dma_MultTaskLoopBackTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;
    AX_U8  *MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        MM_VirtualAddr = pstMultCard[i].MM_VirtualAddr;
        MM_PhyBaseAddr = pstMultCard[i].MM_PhyBaseAddr;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_MultiTaskLoopbackTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_MultiTaskLoopbackTest) msg fail\n");
            return Ret;
        }
        info("send UserCmd(PCIE_DMA_MultiTaskLoopbackTest) msg success\n");
        Ret = LoopbackDataTest(Target, TEST_PORT, LP_RECV_FILE, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("Multi-Task loopback test failed!\n");
        }
    }
    return Ret;
}

AX_VOID *PCIe_DmaPressureTest(AX_VOID *pArg)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_PCIE_MULTCARD_T *pstMultCard = (AX_PCIE_MULTCARD_T *)pArg;
    AX_S32 Target = pstMultCard->s32TargetId;
    AX_U8  *MM_VirtualAddr = pstMultCard->MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr = pstMultCard->MM_PhyBaseAddr;
    AX_U32 TestCnt = 1;

    while (Exit != 1) {
        info("\npressure test number: %d\n", TestCnt);
        // data flow: ep --> rc --> ep
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_DataLoopbackTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_DataLoopbackTest) msg fail\n");
            break;
        }
        debug("send UserCmd(PCIE_DMA_DataLoopbackTest) msg success\n");
        Ret = LoopbackDataTest(Target, TEST_PORT, LP_RECV_FILE, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("loopback data test failed!\n");
            break;
        }
        TestCnt++;
    }
    pthread_exit(NULL);
}

AX_VOID *PCIe_DmaBandWidthTest(AX_VOID *pArg)
{

    AX_S32 Ret = AX_SUCCESS;
    AX_PCIE_MULTCARD_T *pstMultCard = (AX_PCIE_MULTCARD_T *)pArg;
    AX_S32 Target = pstMultCard->s32TargetId;
    AX_U8  *MM_VirtualAddr = pstMultCard->MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr = pstMultCard->MM_PhyBaseAddr;
    AX_U32 TestCnt = 1;

    while (Exit != 1) {
        info("\nbandwidth pressure test number: %d\n", TestCnt);
        // data flow: ep --> rc --> ep
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_BandwidthTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_BandwidthTest) msg fail\n");
            break;
        }

        Ret = BandwidthTest(Target, TEST_PORT, LP_RECV_FILE, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("bandwidth test failed!\n");
            break;
        }

        TestCnt++;
    }
    pthread_exit(NULL);
}

AX_VOID *PCIe_DmaMultiTaskPressureTest(AX_VOID *pArg)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_PCIE_MULTCARD_T *pstMultCard = (AX_PCIE_MULTCARD_T *)pArg;
    AX_S32 Target = pstMultCard->s32TargetId;
    AX_U8  *MM_VirtualAddr = pstMultCard->MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr = pstMultCard->MM_PhyBaseAddr;
    AX_U32 TestCnt = 1;

    while (Exit != 1) {
        info("\npressure test number: %d\n", TestCnt);
        // data flow: ep --> rc --> ep
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_MultiTaskLoopbackTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_MultiTaskLoopbackTest) msg fail\n");
            break;
        }
        debug("send UserCmd(PCIE_DMA_MultiTaskLoopbackTest) msg success\n");
        Ret = LoopbackDataTest(Target, TEST_PORT, LP_RECV_FILE, MM_PhyBaseAddr, MM_VirtualAddr);
        if (Ret < 0) {
            err("loopback data test failed!\n");
            break;
        }
        TestCnt++;
    }
    pthread_exit(NULL);
}


AX_VOID *PCIe_DmaMultiThreadTest(AX_VOID *pArg)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    pthread_t mult_th[THREAD_CNT];
    AX_PCIE_MULTCARD_T *pstMultCard = (AX_PCIE_MULTCARD_T *)pArg;
    AX_S32 Target = pstMultCard->s32TargetId;
    AX_U8  *MM_VirtualAddr = pstMultCard->MM_VirtualAddr;
    AX_U64 MM_PhyBaseAddr = pstMultCard->MM_PhyBaseAddr;

    for (i = 0; i < THREAD_CNT; i++) {
        pstMultCard->stThread[i].s32TargetId = Target;
        pstMultCard->stThread[i].MM_VirtualAddr = MM_VirtualAddr;
        pstMultCard->stThread[i].MM_PhyBaseAddr = MM_PhyBaseAddr;
        pstMultCard->stThread[i].s32Id = i;
        pstMultCard->stThread[i].u8ThreadMsgPort = PCIE_PORT_BASE_NUM + 1 + i;
        pthread_create(&mult_th[i], NULL, PCIe_Dma_32Thread, &pstMultCard->stThread[i]);
    }

    sleep(0.5);

    Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_32ThreadLoopbackTest);
    if (Ret < 0) {
        err("send UserCmd(PCIE_DMA_32ThreadLoopbackTest) msg fail\n");
        exit(1);
    }
    info("send UserCmd(PCIE_DMA_32ThreadLoopbackTest) msg success\n");

    for (i = 0; i < THREAD_CNT; i++) {
        pthread_join(mult_th[i], NULL);
    }
    info("all thread end\n");
    pthread_exit(NULL);
}

AX_S32 Sample_PCIe_DmaPressureTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    pthread_t mult_card[32];

    for (i = 0; i < PcieDevCount; i++) {
        pthread_create(&mult_card[i], NULL, PCIe_DmaPressureTest, &pstMultCard[i]);
    }
    for (i = 0; i < PcieDevCount; i++) {
        pthread_join(mult_card[i], NULL);
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaBandWidthTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    pthread_t mult_card[32];

    for (i = 0; i < PcieDevCount; i++) {
        pthread_create(&mult_card[i], NULL, PCIe_DmaBandWidthTest, &pstMultCard[i]);
    }
    for (i = 0; i < PcieDevCount; i++) {
        pthread_join(mult_card[i], NULL);
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaMultiTaskPressureTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    pthread_t mult_card[32];

    for (i = 0; i < PcieDevCount; i++) {
        pthread_create(&mult_card[i], NULL, PCIe_DmaMultiTaskPressureTest, &pstMultCard[i]);
    }
    for (i = 0; i < PcieDevCount; i++) {
        pthread_join(mult_card[i], NULL);
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaMultiThreadTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    pthread_t mult_card[32];

    for (i = 0; i < PcieDevCount; i++) {
        pthread_create(&mult_card[i], NULL, PCIe_DmaMultiThreadTest, &pstMultCard[i]);
    }
    for (i = 0; i < PcieDevCount; i++) {
        pthread_join(mult_card[i], NULL);
    }
    return Ret;
}

AX_S32 Sample_PCIe_DmaQuitTest(AX_PCIE_MULTCARD_T *pstMultCard, AX_S32 PcieDevCount)
{
    AX_S32 Ret = AX_SUCCESS;
    AX_S32 i;
    AX_S32 Target;

    for (i = 0; i < PcieDevCount; i++) {
        Target = pstMultCard[i].s32TargetId;
        Ret = AX_PCIe_SendUserCmd(Target, TEST_PORT, PCIE_DMA_QuitTest);
        if (Ret < 0) {
            err("send UserCmd(PCIE_DMA_QuitTest) msg fail\n");
            return Ret;
        }
        debug("send UserCmd(PCIE_DMA_QuitTest) msg success\n");
    }
    return Ret;
}

void help(void)
{
    info("usage: sample_pcie_dma_host [-h] [-s<dma-buffer-size>] \n");
    info("   -h      : display usage\n");
    info("   -s      : specify dma buffer size, ex: -s10000. default is 4MB\n");

    return;
}

void HelpInRun(void)
{
    info("s: send file test.\n");
    info("r: recv file test.\n");
    info("l: loopback data test.\n");
    info("d: dma link list send test.\n");
    info("e: dma link list recv test.\n");
    info("m: dma multi-task send test.\n");
    info("w: dma multi-task recv test.\n");
    info("x: dma multi-task loopback test.\n");
    info("p: pressure test.\n");
    info("b: multi-task pressure test.\n");
    info("t: multi-thread test.\n");
    info("i: pcie dma bandwidth test.\n");
    info("q: quit test, ep app will also exit.\n");

    return;
}


void sigint_handler(int sig)
{
    info("\nget a ctrl+c\n");
    Exit = 1;

    /* Force exit */
    g_CtrlCount++;
    if (g_CtrlCount >= 3) {
        printf("============Force exit============\n");
        exit(0);
    }
}


AX_S32 main(int argc, char *argv[])
{
    AX_U64 DmaBufferSize = 0;
    AX_PCIE_MULTCARD_T stMultCard[32];
    AX_S32  Ret = 0;
    AX_S32  key;
    AX_S32 *PcieDevId = NULL;
    AX_S32 PcieDevCount;
    AX_S32 Target;
    AX_S32 opt;
    AX_S32 i;
    AX_S32 Quit = AX_FALSE;
    struct sigaction act;

    info("axera pcie dma transfer test\n\n");

    DmaBufferSize = (1000 * 1000 * 4);
    while ((opt = getopt(argc, argv, "s::h")) != -1) {
        switch (opt) {
        case 's' :
            if (optarg != NULL)
                DmaBufferSize = atoi(optarg);
            break;
        case 'h' :
            help();
            return 0;
        default  :
            err("unknown option.\n");
            help();
            return 0;
        }
    }

    act.sa_handler = sigint_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    Ret = AX_PCIe_InitRcMsg(PCIE_PORT_CNT, PCIE_PORT_BASE_NUM);
    if (Ret == -1) {
        err("init pcie rc msg failed!\n");
        return -1;
    }

    PcieDevCount = AX_PCIe_GetPcieDevIdArray(&PcieDevId);
    if (PcieDevCount <= 0) {
        err("get pcie dev id array failed!\n");
        return -1;
    }

    for (i = 0; i < PcieDevCount; i++) {
        Target = PcieDevId[i];
        stMultCard[i].s32TargetId = Target;
        Ret = MM_BufferInit(Target, &stMultCard[i].MM_VirtualAddr, &stMultCard[i].MM_PhyBaseAddr, DmaBufferSize);
        if (Ret < 0) {
            err("init RC physical base address failed!\n");
            return -1;
        }
        stMultCard[i].DmaBufferSize = DmaBufferSize;

        Ret = AX_PCIe_SendRcPhyBaseAddr(Target, TEST_PORT, stMultCard[i].MM_PhyBaseAddr);
        if (Ret < 0) {
            err("send RC physical base physical address to EP failed!\n");
            goto out;
        }
    }

    while (1) {
        info("\nplease enter one key to test: ");
        key = getchar();
        getchar();

        switch (key) {
        case 'r':
            // EP DMA write, data flow: EP ---> RC
            info("pcie dma file receiving test.\n");
            Sample_PCIe_DmaFileReceivingTest(stMultCard, PcieDevCount);
            break;
        case 's':
            // EP DMA read, data flow: RC ---> EP
            info("pcie dma file sending test.\n");
            Sample_PCIe_DmaFileSendingTest(stMultCard, PcieDevCount);
            break;
        case 'l':
            // data flow: ep --> rc --> ep
            info("pcie dma data loopback test.\n");
            Sample_PCIe_DmaDataLoopbackTest(stMultCard, PcieDevCount);
            break;
        case 'd':
            // EP DMA link list, data flow: RC ---> EP
            info("pcie dma link list send test.\n");
            Sample_PCIe_DmaLinkListSendTest(stMultCard, PcieDevCount);
            break;
        case 'e':
            // EP DMA link list, data flow: EP ---> RC
            info("pcie dma link list recv test.\n");
            Sample_PCIe_DmaLinkRecvTest(stMultCard, PcieDevCount);
            break;
        case 'm':
            // EP DMA multi-task, data flow: RC ---> EP
            info("pcie dma multi-task send test.\n");
            Sample_PCIe_DmaMultiTaskSendTest(stMultCard, PcieDevCount);
            break;
        case 'w':
            // EP DMA multi-task, data flow: EP ---> RC
            info("pcie dma multi-task recv test.\n");
            Sample_PCIe_DmaMultiTaskRecvTest(stMultCard, PcieDevCount);
            break;
        case 'x':
            // EP DMA multi-task, data flow: ep --> rc --> ep
            info("pcie dma multi-task loopback test.\n");
            Sample_PCIe_Dma_MultTaskLoopBackTest(stMultCard, PcieDevCount);
            break;
        case 'p':
            info("pcie dma pressure test\n");
            Exit = 0;
            g_CtrlCount = 0;
            Sample_PCIe_DmaPressureTest(stMultCard, PcieDevCount);
            info("exit test\n");
            break;
        case 'i':
            info("pcie dma bandwidth test\n");
            Exit = 0;
            g_CtrlCount = 0;
            Sample_PCIe_DmaBandWidthTest(stMultCard, PcieDevCount);
            info("exit test\n");
            break;
        case 'b':
            info("pcie dma multi-task pressure test\n");
            Exit = 0;
            g_CtrlCount = 0;
            Sample_PCIe_DmaMultiTaskPressureTest(stMultCard, PcieDevCount);
            info("exit test\n");
            break;
        case 't':
            // EP DMA multi thread test, data flow: ep --> rc --> ep
            Exit = 0;
            g_CtrlCount = 0;
            Ret = Sample_PCIe_DmaMultiThreadTest(stMultCard, PcieDevCount);
            if (Ret < 0) {
                err("Multi-Thread loopback test failed!\n");
            }
            break;
        case 'q':
            // quit test
            Sample_PCIe_DmaQuitTest(stMultCard, PcieDevCount);
            info("quit test!\n");
            Quit = AX_TRUE;
            break;
        case 'h':
            info("usage:\n");
            HelpInRun();
            break;
        default :
            err("invalid input, please enter following key:\n");
            HelpInRun();
            break;
        }

        if (Quit == AX_TRUE)
            break;
    }

out:
    for (i = 0; i < PcieDevCount; i++) {
        Target = PcieDevId[i];
        MM_BufferDeInit(Target, stMultCard[i].MM_VirtualAddr, stMultCard[i].DmaBufferSize);
    }

    for (i = 0; i < PCIE_PORT_CNT; i++) {
        Target = PcieDevId[i];
        AX_PCIe_CloseMsgPort(Target, PCIE_PORT_BASE_NUM + i);
    }
    return 0;
}