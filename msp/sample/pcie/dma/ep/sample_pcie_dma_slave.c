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
#include <pthread.h>

#include "ax_base_type.h"
#include "ax_pcie_dma_api.h"
#include "ax_pcie_msg_api.h"

#include "pcie_dma_test.h"


#define THREAD_CNT 32
//#define PCIE_PORT_CNT 5
#define PCIE_PORT_CNT (THREAD_CNT + 1)
#define PCIE_PORT_BASE_NUM 10
#define TEST_PORT (PCIE_PORT_BASE_NUM + 0)
#define TEST_THREAD1_PORT (PCIE_PORT_BASE_NUM + 1)
#define TEST_THREAD2_PORT (PCIE_PORT_BASE_NUM + 2)
#define TEST_THREAD3_PORT (PCIE_PORT_BASE_NUM + 3)
#define TEST_THREAD4_PORT (PCIE_PORT_BASE_NUM + 4)

#define SEND_FILE   "./pcie_send_file"
#define RECV_FILE   "./pcie_recv_file"
#define LP_RECV_FILE "./pcie_lp_test_recv_file"
#define MT_RECV_FILE1 "./multi_thread_test_recv_file1"
#define MT_RECV_FILE2 "./multi_thread_test_recv_file2"
#define MT_RECV_FILE3 "./multi_thread_test_recv_file3"
#define MT_RECV_FILE4 "./multi_thread_test_recv_file4"

#define BANDWIDTH_TEST_TIMES 40

pthread_t t_dma_test1;
pthread_t t_dma_test2;
pthread_t t_dma_test3;
pthread_t t_dma_test4;

AX_U64 ThreadTestArg[10];

AX_S32 SendFileTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                    AX_U64 DstPhyAddr)
{
    AX_U32 DataSize;
    AX_S32 DataFd;
    AX_S32 ReadSize;
    AX_S32 Ret = 0;

    info("========== send file test ==========\n");

    // open test file to transfer
    DataFd = open(SEND_FILE, O_RDWR);
    if (DataFd < 0) {
        err("open file %s failed!\n", SEND_FILE);
        return -1;
    }

    DataSize = lseek(DataFd, 0, SEEK_END);
    lseek(DataFd, 0, SEEK_SET);
    info("open %s, size:%d bytes\n", SEND_FILE, DataSize);

    Ret = read(DataFd, MM_VirtualAddr, DataSize);
    if (Ret <= 0) {
        err("read file %s failed!\n", SEND_FILE);
        Ret = -1;
        goto RET;
    }

    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_WRITE, MM_PhyBaseAddr, DstPhyAddr, DataSize, 1);
    if (Ret < 0) {
        err("pcie dma write failed!\n");
        Ret = -1;
        goto RET;
    } else {
        info("pcie dma write success!\n");
    }

    //use msg mechanism
    //send a write_done message to RC
    //then wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, DataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send write_done msg success\n");

    ReadSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
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
    info("========== send file test success! ==========\n");

RET:
    close(DataFd);
    return Ret;
}


AX_S32 RecvFileTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                    AX_U64 DstPhyAddr)
{
    AX_U32 DataSize;
    AX_U64 RcPhyAddr;
    AX_S32 RECV_Fd;
    AX_S32 Ret = 0;

    info("========== recv file test ==========\n");

    //wait rc data ready
    Ret = AX_PCIe_WaitDataReadyMsg(0, Port, &RcPhyAddr, &DataSize, -1);
    if (Ret < 0) {
        return -1;
    }

    memset(MM_VirtualAddr, 0, DataSize);

    //create dma read task
    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_READ, RcPhyAddr, MM_PhyBaseAddr, DataSize, 1);
    if (Ret < 0) {
        err("pcie dma read failed!\n");
        //send a read_done message to rc
        Ret = AX_PCIe_SendReadDoneMsg(0, Port, 0);
        if (Ret < 0) {
            err("send read_done msg failed\n");
            return Ret;
        }
        return -1;
    } else {
        info("pcie dma read success!\n");
    }

    remove(RECV_FILE);
    // open test file to transfer
    RECV_Fd = open(RECV_FILE, O_RDWR | O_CREAT);
    if (RECV_Fd < 0) {
        err("open file %s failed!\n", RECV_FILE);
        return -1;
    }

    Ret = write(RECV_Fd, MM_VirtualAddr, DataSize);
    if (Ret != DataSize) {
        err("write file %s failed!\n", RECV_FILE);
        Ret = -1;
        goto RET;
    }

    info("save %d bytes received date to %s success!\n", Ret, RECV_FILE);

    //send a read_done message to rc
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send read_done msg success\n");

    Ret = 0;
    info("========== recv file test success! ==========\n");

RET:
    close(RECV_Fd);
    return Ret;
}


AX_S32 LoopbackDataTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile, AX_U8 *MM_VirtualAddr,
                        AX_U64 MM_PhyBaseAddr,
                        AX_U64 DstPhyAddr,
                        AX_U32 DataLen)
{
    //AX_S32 RandomFd;
    AX_U8 *TestDataBuffer = NULL;
    AX_U32 TestDataSize, RecvDataSize, SendDataSize;
    AX_S32 Ret = 0;
    AX_S32 DataFd;

    info("========== loopback data test ==========\n");

    TestDataSize = DataLen;

    TestDataBuffer = (AX_U8 *)malloc(TestDataSize);
    if (TestDataBuffer == NULL) {
        err("malloc failed!\n");
        return -1;
    }

#if 0
    RandomFd = open("/dev/urandom", O_RDONLY);
    Ret = read(RandomFd, TestDataBuffer, TestDataSize);
    if (Ret <= 0) {
        err("read file /dev/urandom failed!\n");
        Ret = -1;
        goto END;
    }
#else
    memset(TestDataBuffer, 'B', TestDataSize);
#endif
    memcpy(MM_VirtualAddr, TestDataBuffer, TestDataSize);

    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_WRITE, MM_PhyBaseAddr, DstPhyAddr, TestDataSize, 1);
    if (Ret < 0) {
        err("pcie dma write failed!\n");
        Ret = -1;
        goto END;
    } else {
        info("pcie dma write success!\n");
    }

    //use msg mechanism
    //send a write_done message to RC
    //and wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, TestDataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto END;
    }
    info("send write_done msg success\n");

    RecvDataSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
    if (RecvDataSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto END;
    } else if (RecvDataSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto END;
    }

    memset(MM_VirtualAddr, 0, TestDataSize);

    //create dma read task
    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_READ, DstPhyAddr, MM_PhyBaseAddr, RecvDataSize, 1);
    if (Ret < 0) {
        err("pcie dma read failed!\n");
        Ret = -1;
        goto END;
    } else {
        info("pcie dma read success!\n");
    }

#if 1
    // build wrong case
    //MM_VirtualAddr[10000] = 0;
    //MM_VirtualAddr[TestDataSize -1] = 'A';
#endif
    //read_done msg is not necessary
    if (memcmp(MM_VirtualAddr, TestDataBuffer, TestDataSize)) {
        err("loopback data compare failed\n");
        SendDataSize = 0;   /* notify host: lp test is failed */
    } else {
        info("loopback data compare success!\n");
        SendDataSize = RecvDataSize;
    }

    remove(RecvDataFile);
    // open test file to save received data
    DataFd = open(RecvDataFile, O_RDWR | O_CREAT);
    if (DataFd < 0) {
        err("open file %s failed!\n", RecvDataFile);
        Ret = -1;
        goto END;
    }

    lseek(DataFd, 0, SEEK_SET);

    Ret = write(DataFd, MM_VirtualAddr, RecvDataSize);
    if (Ret != RecvDataSize) {
        err("write file %s failed!\n", RecvDataFile);
        Ret = -1;
        goto RET;
    }

    info("save %d bytes received date to %s success!\n", Ret, RecvDataFile);

    //send a read_done message to RC
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, SendDataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    } else {
        info("send read_done msg success\n");
    }

    Ret = 0;
    info("========== data loopback test success! ==========\n");

RET:
    close(DataFd);
END:
    free(TestDataBuffer);

    return Ret;
}


struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;

    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    return temp;
}

AX_S32 DMA_BandwidthTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile, AX_U8 *MM_VirtualAddr,
                         AX_U64 MM_PhyBaseAddr,
                         AX_U64 DstPhyAddr,
                         AX_U32 DataLen)
{
    //AX_S32 RandomFd;
    AX_U8 *TestDataBuffer = NULL;
    AX_U32 TestDataSize, RecvDataSize, SendDataSize;
    AX_S32 Ret = 0;
    struct timespec ts_start, ts_end, ts_diff;
    static struct timespec w_total_ts, r_total_ts;
    static AX_S32 WriteTestTimes = 0,  ReadTestTimes = 0;
    static AX_U32 Num = 0;
    static double TotalWriteTime = 0, TotalReadTime = 0;
    double Bandwidth;

    TestDataSize = DataLen;

    TestDataBuffer = (AX_U8 *)malloc(TestDataSize);
    if (TestDataBuffer == NULL) {
        err("malloc failed!\n");
        return -1;
    }

    memset(TestDataBuffer, 'B', TestDataSize);
    memcpy(MM_VirtualAddr, TestDataBuffer, TestDataSize);

    Ret = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    if (Ret < 0)
        err("invalid clock 1\n");

    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_WRITE, MM_PhyBaseAddr, DstPhyAddr, TestDataSize, 1);
    if (Ret < 0) {
        err("pcie dma write failed!\n");
        Ret = -1;
        goto RET;
    }

    //use msg mechanism
    //send a write_done message to RC
    //and wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, TestDataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto RET;
    }

    Ret = clock_gettime(CLOCK_MONOTONIC, &ts_end);
    if (Ret < 0)
        err("invalid clock 2\n");

    ts_diff = diff(ts_start, ts_end);

    WriteTestTimes ++;
    TotalWriteTime += (ts_diff.tv_sec + (double)(ts_diff.tv_nsec) / 1000 / 1000 / 1000);
    w_total_ts.tv_sec += ts_diff.tv_sec;
    w_total_ts.tv_nsec += ts_diff.tv_nsec;

    if (WriteTestTimes == BANDWIDTH_TEST_TIMES) {
        Bandwidth = (WriteTestTimes * TestDataSize / 1024 / 1024) / TotalWriteTime;
        Num ++;

        info("\nNum%d - pcie dma write stat:\n", Num);
        info("write time: %ld.%09ld seconds\n", w_total_ts.tv_sec, w_total_ts.tv_nsec);
        info("write time: %lds %ldms\n", w_total_ts.tv_sec, w_total_ts.tv_nsec / 1000 / 1000);
        info("write bandwidth: %.1f MB/s in %f seconds\n", Bandwidth, TotalWriteTime);

        WriteTestTimes = 0;
        w_total_ts.tv_sec = 0;
        w_total_ts.tv_nsec = 0;
        TotalWriteTime = 0;
    }

    RecvDataSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
    if (RecvDataSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto RET;
    } else if (RecvDataSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto RET;
    }

    memset(MM_VirtualAddr, 0, TestDataSize);

    Ret = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    if (Ret < 0)
        err("invalid clock 3\n");

    //create dma read task
    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_READ, DstPhyAddr, MM_PhyBaseAddr, RecvDataSize, 1);
    if (Ret < 0) {
        err("pcie dma read failed!\n");
        Ret = -1;
        goto RET;
    }

    SendDataSize = RecvDataSize;

#if 0
    //read_done msg is not necessary
    if (memcmp(MM_VirtualAddr, TestDataBuffer, TestDataSize)) {
        err("loopback data compare failed\n");
        SendDataSize = 0;   /* notify host: lp test is failed */
    }
#endif

    //send a read_done message to RC
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, SendDataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    }

    Ret = clock_gettime(CLOCK_MONOTONIC, &ts_end);
    if (Ret < 0)
        err("invalid clock 4\n");

    ts_diff = diff(ts_start, ts_end);

    ReadTestTimes ++;
    TotalReadTime += (ts_diff.tv_sec + (double)(ts_diff.tv_nsec) / 1000 / 1000 / 1000);
    r_total_ts.tv_sec += ts_diff.tv_sec;
    r_total_ts.tv_nsec += ts_diff.tv_nsec;

    if (ReadTestTimes == BANDWIDTH_TEST_TIMES) {
        Bandwidth = (ReadTestTimes * TestDataSize / 1024 / 1024) / TotalReadTime;

        info("\nNum%d - pcie dma read stat:\n", Num);
        info("read time: %ld.%09ld seconds\n", r_total_ts.tv_sec, r_total_ts.tv_nsec);
        info("read time: %lds %ldms\n", r_total_ts.tv_sec, r_total_ts.tv_nsec / 1000 / 1000);
        info("read bandwidth: %.1f MB/s in %f seconds\n", Bandwidth, TotalReadTime);

        ReadTestTimes = 0;
        r_total_ts.tv_sec = 0;
        r_total_ts.tv_nsec = 0;
        TotalReadTime = 0;
    }

    Ret = 0;

RET:
    free(TestDataBuffer);

    return Ret;
}


/* remove all pcie dma API */
AX_S32 MultiThreadDebug(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile, AX_U8 *MM_VirtualAddr,
                        AX_U64 MM_PhyBaseAddr,
                        AX_U64 DstPhyAddr,
                        AX_U32 DataLen)
{
    //AX_S32 RandomFd;
    AX_U32 TestDataSize = 1000, RecvDataSize, SendDataSize = 1000;
    AX_S32 Ret = 0;

    info("========== multi thread debug ==========\n");

    sleep(0.01);

    //use msg mechanism
    //send a write_done message to RC
    //and wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, TestDataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto END;
    }
    info("send write_done msg success\n");

    RecvDataSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
    if (RecvDataSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto END;
    } else if (RecvDataSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto END;
    }

    sleep(0.01);

    //send a read_done message to RC
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, SendDataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    } else {
        info("send read_done msg success\n");
    }

    Ret = 0;
    info("========== multi thread debug success! ==========\n");

RET:
END:
    return Ret;
}

AX_S32 DMA_LinkListTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                        AX_U64 DstPhyAddr)
{
    AX_U32 DataSize;
    AX_U64 RcPhyAddr;
    AX_S32 RECV_Fd;
    AX_S32 Ret = 0;
    AX_S32 ListNum = 10;
    AX_S32 TransferSizePerList;
    AX_S32 LastTransferSize;
    AX_S32 i;
    AX_S32 Tmp;
    AX_S32 IsLast;
    AX_S32 TransferSize;
    AX_U64 SRC, DST;

    info("========== recv file: dma link list test ==========\n");

    //wait rc data ready
    Ret = AX_PCIe_WaitDataReadyMsg(0, Port, &RcPhyAddr, &DataSize, -1);
    if (Ret < 0) {
        return -1;
    }

    memset(MM_VirtualAddr, 0, DataSize);

    // test dma link list
    LastTransferSize = DataSize % ListNum;
    TransferSizePerList = DataSize / ListNum;
    if (LastTransferSize != 0)
        LastTransferSize = TransferSizePerList + LastTransferSize;
    else
        LastTransferSize = TransferSizePerList;

    if (TransferSizePerList % 4) {
        Tmp = TransferSizePerList % 4;
        TransferSizePerList = TransferSizePerList - Tmp; //ensure TransferSizePerList % 4 = 0
        LastTransferSize = LastTransferSize + Tmp * (ListNum - 1);
    }

    for (i = 0; i < ListNum; i++) {
        IsLast = (i == (ListNum - 1));
        if (IsLast)
            TransferSize = LastTransferSize;
        else
            TransferSize = TransferSizePerList;

        SRC = RcPhyAddr + i * TransferSizePerList;
        DST = MM_PhyBaseAddr + i * TransferSizePerList;

        debug("add task%d into list, src:%llx, dst:%llx, size:%d\n", i, SRC, DST, TransferSize);
        Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_READ, SRC, DST, TransferSize, IsLast);
        if (Ret < 0) {
            err("pcie dma read failed!\n");
            return -1;
        } else {
            debug("pcie dma read success!\n");
        }

        if (IsLast)
            break;
    }

    remove(RECV_FILE);
    // open test file to transfer
    RECV_Fd = open(RECV_FILE, O_RDWR | O_CREAT);
    if (RECV_Fd < 0) {
        err("open file %s failed!\n", RECV_FILE);
        return -1;
    }

    Ret = write(RECV_Fd, MM_VirtualAddr, DataSize);
    if (Ret != DataSize) {
        err("write file %s failed!\n", RECV_FILE);
        Ret = -1;
        goto RET;
    }

    info("save %d bytes received date to %s success!\n", Ret, RECV_FILE);

    //send a read_done message to rc
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send read_done msg success\n");

    Ret = 0;
    info("========== recv file: dma link list test success! ==========\n");

RET:
    close(RECV_Fd);
    return 0;
}


AX_S32 BuildMultiTaskBlock(AX_U64 SrcPhyAddr, AX_U64 DstPhyAddr, AX_U32 DataSize, AX_S32 ListCount,
                           AX_PCIE_DMA_BLOCK_T(*stDmaBlk)[])
{
    AX_S32 Ret = 0;
    AX_S32 TransferSizePerList;
    AX_S32 LastTransferSize;
    AX_S32 i;
    AX_S32 IsLast;
    AX_S32 TransferSize;
    AX_U64 SRC, DST;

    LastTransferSize = DataSize % ListCount;
    TransferSizePerList = DataSize / ListCount;
    if (LastTransferSize != 0)
        LastTransferSize = TransferSizePerList + LastTransferSize;
    else
        LastTransferSize = TransferSizePerList;

    for (i = 0; i < ListCount; i++) {
        IsLast = (i == (ListCount - 1));
        if (IsLast)
            TransferSize = LastTransferSize;
        else
            TransferSize = TransferSizePerList;

        SRC = SrcPhyAddr + i * TransferSizePerList;
        DST = DstPhyAddr + i * TransferSizePerList;

        debug("add task%d into block, src:%llx, dst:%llx, size:%d\n", i, SRC, DST, TransferSize);
        (*stDmaBlk)[i].u64SrcAddr = SRC;
        (*stDmaBlk)[i].u64DstAddr = DST;
        (*stDmaBlk)[i].u32BlkSize = TransferSize;

        if (IsLast)
            break;
    }

    return Ret;
}


AX_S32 DMA_MultiTaskTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                         AX_U64 DstPhyAddr)
{
    AX_U32 DataSize;
    AX_U64 RcPhyAddr;
    AX_S32 RECV_Fd;
    AX_S32 Ret = 0;
    AX_S32 ListNum = 10;
    AX_PCIE_DMA_BLOCK_T stDmaBlk[ListNum];

    info("========== recv file: dma multi task test ==========\n");

    //wait rc data ready
    Ret = AX_PCIe_WaitDataReadyMsg(0, Port, &RcPhyAddr, &DataSize, -1);
    if (Ret < 0) {
        return -1;
    }

    memset(MM_VirtualAddr, 0, DataSize);

    BuildMultiTaskBlock(RcPhyAddr, MM_PhyBaseAddr, DataSize, ListNum, &stDmaBlk);

    Ret = AX_PCIe_CreatDmaMultiTask(PCIeDmaHandle, AX_TRUE, ListNum, stDmaBlk);
    if (Ret < 0) {
        err("pcie dma read multi task failed!\n");
        return -1;
    } else {
        info("pcie dma read multi task success!\n");
    }

    remove(RECV_FILE);
    // open test file to transfer
    RECV_Fd = open(RECV_FILE, O_RDWR | O_CREAT);
    if (RECV_Fd < 0) {
        err("open file %s failed!\n", RECV_FILE);
        return -1;
    }

    Ret = write(RECV_Fd, MM_VirtualAddr, DataSize);
    if (Ret != DataSize) {
        err("write file %s failed!\n", RECV_FILE);
        Ret = -1;
        goto RET;
    }

    info("save %d bytes received date to %s success!\n", Ret, RECV_FILE);

    //send a read_done message to rc
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, DataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send read_done msg success\n");

    Ret = 0;
    info("========== recv file: dma multi task test success! ==========\n");

RET:
    close(RECV_Fd);
    return 0;
}


AX_S32 DMA_LinkListSendTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_U8 *MM_VirtualAddr,
                            AX_U64 MM_PhyBaseAddr,
                            AX_U64 DstPhyAddr)
{
    AX_U32 DataSize;
    AX_S32 DataFd;
    AX_S32 ReadSize;
    AX_S32 TransferSizePerList;
    AX_S32 LastTransferSize;
    AX_S32 i;
    AX_S32 Tmp;
    AX_S32 IsLast;
    AX_S32 TransferSize;
    AX_U64 SRC, DST;
    AX_S32 ListNum = 10;
    AX_S32 Ret = 0;

    info("========== send file: dma link list test ==========\n");

    // open test file to transfer
    DataFd = open(SEND_FILE, O_RDWR);
    if (DataFd < 0) {
        err("open file %s failed!\n", SEND_FILE);
        return -1;
    }

    DataSize = lseek(DataFd, 0, SEEK_END);
    lseek(DataFd, 0, SEEK_SET);
    info("open %s, size:%d bytes\n", SEND_FILE, DataSize);

    Ret = read(DataFd, MM_VirtualAddr, DataSize);
    if (Ret <= 0) {
        err("read file %s failed!\n", SEND_FILE);
        Ret = -1;
        goto RET;
    }

    // test dma link list
    LastTransferSize = DataSize % ListNum;
    TransferSizePerList = DataSize / ListNum;
    if (LastTransferSize != 0)
        LastTransferSize = TransferSizePerList + LastTransferSize;
    else
        LastTransferSize = TransferSizePerList;

    if (TransferSizePerList % 4) {
        Tmp = TransferSizePerList % 4;
        TransferSizePerList = TransferSizePerList - Tmp; //ensure TransferSizePerList % 4 = 0
        LastTransferSize = LastTransferSize + Tmp * (ListNum - 1);
    }

    for (i = 0; i < ListNum; i++) {
        IsLast = (i == (ListNum - 1));
        if (IsLast)
            TransferSize = LastTransferSize;
        else
            TransferSize = TransferSizePerList;

        SRC = MM_PhyBaseAddr + i * TransferSizePerList;
        DST = DstPhyAddr + i * TransferSizePerList;

        debug("add task%d into list, src:%llx, dst:%llx, size:%d\n", i, SRC, DST, TransferSize);
        Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_WRITE, SRC, DST, TransferSize, IsLast);
        if (Ret < 0) {
            err("pcie dma write failed!\n");
            return -1;
        } else {
            debug("pcie dma write success!\n");
        }

        if (IsLast)
            break;
    }

    //use msg mechanism
    //send a write_done message to RC
    //then wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, DataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send write_done msg success\n");

    ReadSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
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
    info("========== send file: dma link list test success! ==========\n");

RET:
    close(DataFd);
    return Ret;
}


AX_S32 DMA_MultiTaskSendTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_U8 *MM_VirtualAddr,
                             AX_U64 MM_PhyBaseAddr,
                             AX_U64 DstPhyAddr)
{
    AX_U32 DataSize;
    AX_S32 DataFd;
    AX_S32 ReadSize;
    AX_S32 ListNum = 10;
    AX_PCIE_DMA_BLOCK_T stDmaBlk[ListNum];
    AX_S32 Ret = 0;

    info("========== send file: dma multi task test ==========\n");

    // open test file to transfer
    DataFd = open(SEND_FILE, O_RDWR);
    if (DataFd < 0) {
        err("open file %s failed!\n", SEND_FILE);
        return -1;
    }

    DataSize = lseek(DataFd, 0, SEEK_END);
    lseek(DataFd, 0, SEEK_SET);
    info("open %s, size:%d bytes\n", SEND_FILE, DataSize);

    Ret = read(DataFd, MM_VirtualAddr, DataSize);
    if (Ret <= 0) {
        err("read file %s failed!\n", SEND_FILE);
        Ret = -1;
        goto RET;
    }

    // test dma multi task
    BuildMultiTaskBlock(MM_PhyBaseAddr, DstPhyAddr, DataSize, ListNum, &stDmaBlk);

    Ret = AX_PCIe_CreatDmaMultiTask(PCIeDmaHandle, AX_FALSE, ListNum, stDmaBlk);
    if (Ret < 0) {
        err("pcie dma write multi task failed!\n");
        return -1;
    } else {
        info("pcie dma write multi task success!\n");
    }

    //use msg mechanism
    //send a write_done message to RC
    //then wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, DataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto RET;
    }
    info("send write_done msg success\n");

    ReadSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
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
    info("========== send file: dma multi task test success! ==========\n");

RET:
    close(DataFd);
    return Ret;
}


AX_S32 MultiTaskLoopbackTest(AX_S32 PCIeDmaHandle, AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile,
                             AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                             AX_U64 DstPhyAddr,
                             AX_U32 DataLen)
{
    //AX_S32 RandomFd;
    AX_U8 *TestDataBuffer = NULL;
    AX_U32 TestDataSize, RecvDataSize, SendDataSize;
    AX_S32 Ret = 0;
    AX_S32 DataFd;
    AX_S32 ListNum = 10;
    AX_PCIE_DMA_BLOCK_T stDmaBlk[ListNum];

    info("========== Multi-Task loopback test ==========\n");

    TestDataSize = DataLen;

    TestDataBuffer = (AX_U8 *)malloc(TestDataSize);
    if (TestDataBuffer == NULL) {
        err("malloc failed!\n");
        return -1;
    }

#if 0
    RandomFd = open("/dev/urandom", O_RDONLY);
    Ret = read(RandomFd, TestDataBuffer, TestDataSize);
    if (Ret <= 0) {
        err("read file /dev/urandom failed!\n");
        Ret = -1;
        goto END;
    }
#else
    memset(TestDataBuffer, 'B', TestDataSize);
#endif
    memcpy(MM_VirtualAddr, TestDataBuffer, TestDataSize);

    // test dma multi task
    BuildMultiTaskBlock(MM_PhyBaseAddr, DstPhyAddr, TestDataSize, ListNum, &stDmaBlk);

    Ret = AX_PCIe_CreatDmaMultiTask(PCIeDmaHandle, AX_FALSE, ListNum, stDmaBlk);
    if (Ret < 0) {
        err("pcie dma write multi task failed!\n");
        return -1;
    } else {
        info("pcie dma write multi task success!\n");
    }

    //use msg mechanism
    //send a write_done message to RC
    //and wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, TestDataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto END;
    }
    info("send write_done msg success\n");

    RecvDataSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
    if (RecvDataSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto END;
    } else if (RecvDataSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto END;
    }

    memset(MM_VirtualAddr, 0, TestDataSize);

    // test dma multi task
    BuildMultiTaskBlock(DstPhyAddr, MM_PhyBaseAddr, RecvDataSize, ListNum, &stDmaBlk);

    Ret = AX_PCIe_CreatDmaMultiTask(PCIeDmaHandle, AX_TRUE, ListNum, stDmaBlk);
    if (Ret < 0) {
        err("pcie dma read multi task failed!\n");
        return -1;
    } else {
        info("pcie dma read multi task success!\n");
    }

#if 1
    // build wrong case
    //MM_VirtualAddr[10000] = 0;
    //MM_VirtualAddr[TestDataSize -1] = 'A';
#endif
    //read_done msg is not necessary
    if (memcmp(MM_VirtualAddr, TestDataBuffer, TestDataSize)) {
        err("loopback data compare failed\n");
        SendDataSize = 0;   /* notify host: lp test is failed */
    } else {
        info("loopback data compare success!\n");
        SendDataSize = RecvDataSize;
    }

    remove(RecvDataFile);
    // open test file to save received data
    DataFd = open(RecvDataFile, O_RDWR | O_CREAT);
    if (DataFd < 0) {
        err("open file %s failed!\n", RecvDataFile);
        Ret = -1;
        goto END;
    }

    lseek(DataFd, 0, SEEK_SET);

    Ret = write(DataFd, MM_VirtualAddr, RecvDataSize);
    if (Ret != RecvDataSize) {
        err("write file %s failed!\n", RecvDataFile);
        Ret = -1;
        goto RET;
    }

    info("save %d bytes received date to %s success!\n", Ret, RecvDataFile);

    //send a read_done message to RC
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, SendDataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    } else {
        info("send read_done msg success\n");
    }

    Ret = 0;
    info("========== Multi-Task loopback test success! ==========\n");

RET:
    close(DataFd);
END:
    free(TestDataBuffer);

    return Ret;
}


AX_VOID *PCIe_Dma_ThreadTest1(AX_VOID *Arg)
{
    AX_S32 Ret = 0;
    info("start pcie dma thread test 1\n");

    while (1) {
        // data flow: ep --> rc --> ep
        Ret = MultiTaskLoopbackTest(ThreadTestArg[0], 0, TEST_THREAD1_PORT, MT_RECV_FILE1, (AX_VOID *)ThreadTestArg[1],
                                    ThreadTestArg[2], ThreadTestArg[3], ThreadTestArg[4]);
        if (Ret < 0) {
            err("loopback data test1 failed!\n");
            return NULL;
        }
    }
}

AX_VOID *PCIe_Dma_ThreadTest2(AX_VOID *Arg)
{
    AX_S32 Ret = 0;
    info("start pcie dma thread test 2\n");

    while (1) {
        // data flow: ep --> rc --> ep
        Ret = MultiTaskLoopbackTest(ThreadTestArg[0], 0, TEST_THREAD2_PORT, MT_RECV_FILE2,
                                    (AX_VOID *)(ThreadTestArg[1] + ThreadTestArg[4]), ThreadTestArg[2] + ThreadTestArg[4],
                                    ThreadTestArg[3] + ThreadTestArg[4], ThreadTestArg[4]);
        if (Ret < 0) {
            err("loopback data test2 failed!\n");
            return NULL;
        }
    }
}

AX_VOID *PCIe_Dma_ThreadTest3(AX_VOID *Arg)
{
    AX_S32 Ret = 0;
    info("start pcie dma thread test 3\n");

    while (1) {
        // data flow: ep --> rc --> ep
        Ret = MultiTaskLoopbackTest(ThreadTestArg[0], 0, TEST_THREAD3_PORT, MT_RECV_FILE3,
                                    (AX_VOID *)(ThreadTestArg[1] + 2 * ThreadTestArg[4]), ThreadTestArg[2] + 2 * ThreadTestArg[4],
                                    ThreadTestArg[3] + 2 * ThreadTestArg[4], ThreadTestArg[4]);
        if (Ret < 0) {
            err("loopback data test3 failed!\n");
            return NULL;
        }
    }
}

AX_VOID *PCIe_Dma_ThreadTest4(AX_VOID *Arg)
{
    AX_S32 Ret = 0;
    info("start pcie dma thread test 4\n");

    while (1) {
        // data flow: ep --> rc --> ep
        Ret = MultiTaskLoopbackTest(ThreadTestArg[0], 0, TEST_THREAD4_PORT, MT_RECV_FILE4,
                                    (AX_VOID *)(ThreadTestArg[1] + 3 * ThreadTestArg[4]), ThreadTestArg[2] + 3 * ThreadTestArg[4],
                                    ThreadTestArg[3] + 3 * ThreadTestArg[4], ThreadTestArg[4]);
        if (Ret < 0) {
            err("loopback data test4 failed!\n");
            return NULL;
        }
    }
}

AX_S32 PCIe_DMA_MultiThreadTest(AX_S32 PCIeDmaHandle, AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                                AX_U64 DstPhyAddr,
                                AX_U32 DataLen)
{
    AX_S32 Ret = 0;

    info("Pcie DMA multi thread loopback test");

    ThreadTestArg[0] = PCIeDmaHandle;
    ThreadTestArg[1] = (AX_U64)MM_VirtualAddr;
    ThreadTestArg[2] = MM_PhyBaseAddr;
    ThreadTestArg[3] = DstPhyAddr;
    ThreadTestArg[4] = DataLen / 4;

    Ret = pthread_create(&t_dma_test1, NULL, PCIe_Dma_ThreadTest1, NULL);
    if (Ret != 0)
        err("can't create thread1\n");
    Ret = pthread_create(&t_dma_test2, NULL, PCIe_Dma_ThreadTest2, NULL);
    if (Ret != 0)
        err("can't create thread2\n");
    Ret = pthread_create(&t_dma_test3, NULL, PCIe_Dma_ThreadTest3, NULL);
    if (Ret != 0)
        err("can't create thread3\n");
    Ret = pthread_create(&t_dma_test4, NULL, PCIe_Dma_ThreadTest4, NULL);
    if (Ret != 0)
        err("can't create thread4\n");

    pthread_join(t_dma_test1, NULL);
    pthread_join(t_dma_test2, NULL);
    pthread_join(t_dma_test3, NULL);
    pthread_join(t_dma_test4, NULL);

    return Ret;
}


AX_S32 MultiThread32LoopbackTest(AX_S32 PCIeDmaHandle, AX_S32 ThreadId, AX_S32 Target, AX_S32 Port, AX_CHAR *RecvDataFile,
                             AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                             AX_U64 DstPhyAddr,
                             AX_U32 DataLen)
{
    //AX_S32 RandomFd;
    AX_U8 *TestDataBuffer = NULL;
    AX_U32 TestDataSize, RecvDataSize, SendDataSize;
    AX_S32 Ret = 0;
    AX_S32 DataFd = -1;
    AX_S32 SendDataFd = -1;
    AX_CHAR *SendDataFile = NULL;
#if 0
    AX_S32 ListNum = 10;
    AX_PCIE_DMA_BLOCK_T stDmaBlk[ListNum];
#endif

    debug("========== loopback test ==========\n");

    TestDataSize = DataLen;

    TestDataBuffer = (AX_U8 *)malloc(TestDataSize);
    if (TestDataBuffer == NULL) {
        err("malloc failed!\n");
        return -1;
    }

#if 0
    RandomFd = open("/dev/urandom", O_RDONLY);
    Ret = read(RandomFd, TestDataBuffer, TestDataSize);
    if (Ret <= 0) {
        err("read file /dev/urandom failed!\n");
        Ret = -1;
        goto END;
    }
#else
    memset(TestDataBuffer, 0x0  + ThreadId, TestDataSize);
#endif
    memcpy(MM_VirtualAddr, TestDataBuffer, TestDataSize);

#if 0
    // test dma multi task
    BuildMultiTaskBlock(MM_PhyBaseAddr, DstPhyAddr, TestDataSize, ListNum, &stDmaBlk);

    Ret = AX_PCIe_CreatDmaMultiTask(PCIeDmaHandle, AX_FALSE, ListNum, stDmaBlk);
    if (Ret < 0) {
        err("pcie dma write multi task failed!\n");
        return -1;
    } else {
        info("pcie dma write multi task success!\n");
    }
#else
    //create dma write task
    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_WRITE, MM_PhyBaseAddr, DstPhyAddr, TestDataSize, 1);
    if (Ret < 0) {
        err("pcie dma write failed!\n");
        return -1;
    } else {
        debug("pcie dma write success!\n");
    }
#endif

    //use msg mechanism
    //send a write_done message to RC
    //and wait RC's response(read_done)
    Ret = AX_PCIe_SendWriteDoneMsg(0, Port, DstPhyAddr, TestDataSize);
    if (Ret < 0) {
        err("send write done msg failed\n");
        Ret = -1;
        goto END;
    }
    debug("send write_done msg success\n");

    RecvDataSize = AX_PCIe_WaitReadDoneMsg(0, Port, -1);
    if (RecvDataSize < 0) {
        err("wait read done msg failed\n");
        Ret = -1;
        goto END;
    } else if (RecvDataSize == 0) {
        err("ep read data failed\n");
        Ret = -1;
        goto END;
    }
    debug("get read_done msg success\n");

    memset(MM_VirtualAddr, 0x50 + ThreadId, TestDataSize);

#if 0
    // test dma multi task
    BuildMultiTaskBlock(DstPhyAddr, MM_PhyBaseAddr, RecvDataSize, ListNum, &stDmaBlk);

    Ret = AX_PCIe_CreatDmaMultiTask(PCIeDmaHandle, AX_TRUE, ListNum, stDmaBlk);
    if (Ret < 0) {
        err("pcie dma read multi task failed!\n");
        return -1;
    } else {
        info("pcie dma read multi task success!\n");
    }
#else
    //create dma read task
    Ret = AX_PCIe_CreatDmaTask(PCIeDmaHandle, DMA_READ, DstPhyAddr, MM_PhyBaseAddr, RecvDataSize, 1);
    if (Ret < 0) {
        err("pcie dma read failed!\n");
        //send a read_done message to rc
        Ret = AX_PCIe_SendReadDoneMsg(0, Port, 0);
        if (Ret < 0) {
            err("send read_done msg failed\n");
            return Ret;
        }
        return -1;
    } else {
        debug("pcie dma read success!\n");
    }
#endif

#if 0
    // build wrong case
    if(Port == 12) {
        MM_VirtualAddr[0] = 'A';
        MM_VirtualAddr[TestDataSize -1] = 'A';
    }
#endif
    //read_done msg is not necessary
    if (memcmp(MM_VirtualAddr, TestDataBuffer, TestDataSize)) {
        err("loopback data compare failed, send size:%d, read size:%d\n", TestDataSize, RecvDataSize);
        SendDataSize = 0;   /* notify host: lp test is failed */
        RecvDataFile = "./pcie_dma_recv_failed_data";
        SendDataFile = "./pcie_dma_send_data";
    } else {
        debug("loopback data compare success!\n");
        SendDataSize = RecvDataSize;
    }

    info("recv %d bytes success!\n", RecvDataSize);

    if(RecvDataFile) {
        remove(RecvDataFile);
        // open test file to save received data
        DataFd = open(RecvDataFile, O_RDWR | O_CREAT);
        if (DataFd < 0) {
            err("open file %s failed!\n", RecvDataFile);
            Ret = -1;
            goto END;
        }

        lseek(DataFd, 0, SEEK_SET);

        Ret = write(DataFd, MM_VirtualAddr, RecvDataSize);
        if (Ret != RecvDataSize) {
            err("write file %s failed!\n", RecvDataFile);
            Ret = -1;
            goto RET;
        }

        info("save %d bytes received date to %s success!\n", Ret, RecvDataFile);
    }

    if(SendDataFile) {
        remove(SendDataFile);
        // open test file to save received data
        SendDataFd = open(SendDataFile, O_RDWR | O_CREAT);
        if (SendDataFd < 0) {
            err("open file %s failed!\n", SendDataFile);
            Ret = -1;
            goto END;
        }

        lseek(SendDataFd, 0, SEEK_SET);

        Ret = write(SendDataFd, TestDataBuffer, TestDataSize);
        if (Ret != TestDataSize) {
            err("write file %s failed!\n", SendDataFile);
            Ret = -1;
            goto RET;
        }

        info("save %d bytes received date to %s success!\n", Ret, SendDataFile);
    }

    //send a read_done message to RC
    Ret = AX_PCIe_SendReadDoneMsg(0, Port, SendDataSize);
    if (Ret < 0) {
        err("send read_done msg failed\n");
        Ret = -1;
        goto RET;
    } else {
        debug("send read_done msg success\n");
    }

    if(SendDataSize == 0) {
        Ret = -1;
        debug("========== Multi-Task loopback test failed! ==========\n");
    } else {
        Ret = 0;
        debug("========== Multi-Task loopback test success! ==========\n");
    }

RET:
    if(DataFd != -1)
        close(DataFd);
    if(SendDataFd != -1)
        close(SendDataFd);
END:
    free(TestDataBuffer);

    return Ret;
}

AX_VOID *PCIe_Dma_32Thread(AX_VOID *Arg)
{
    AX_S32 Ret = 0;
    AX_U32 TestCnt = 1;
    AX_U64 *ThreadTestArg = (AX_U64 *)Arg;
    info("start pcie dma thread %lld, port %lld, mm_vir_addr:%llx, mm_phy_addr:%llx, rc_phy_addr:%llx\n",
        ThreadTestArg[5], ThreadTestArg[6], ThreadTestArg[1], ThreadTestArg[2], ThreadTestArg[3]);

    while (1) {
        // data flow: ep --> rc --> ep
        info("\nthread %lld pressure test number: %d\n", ThreadTestArg[5], TestCnt);
        if (AX_PCIe_WaitUserCmd(0, ThreadTestArg[6], 10000) == PCIE_DMA_MultiThreadTestQuit) {
            break;
        }

        Ret = MultiThread32LoopbackTest(ThreadTestArg[0], ThreadTestArg[5], 0, ThreadTestArg[6], NULL, (AX_VOID *)ThreadTestArg[1],
                               ThreadTestArg[2], ThreadTestArg[3], ThreadTestArg[4]);
        if (Ret < 0) {
            err("loopback data failed! thread:%lld, mm_phy_addr:%llx\n", ThreadTestArg[5], ThreadTestArg[2]);
            //return NULL;
            exit(0);
        }

        TestCnt++;
    }

    pthread_exit(NULL);
}

pthread_t t_dma_test[THREAD_CNT];
AX_U64 Thread32Arg[THREAD_CNT][10];

AX_S32 PCIe_DMA_32ThreadTest(AX_S32 PCIeDmaHandle, AX_U8 *MM_VirtualAddr, AX_U64 MM_PhyBaseAddr,
                                AX_U64 DstPhyAddr,
                                AX_U32 DataLen)
{
    AX_S32 Ret = 0;
    AX_S32 i = 0;

    info("Pcie DMA %d thread loopback test\n", THREAD_CNT);

    for(i = 0; i < THREAD_CNT; i++) {
        Thread32Arg[i][0] = PCIeDmaHandle;
        Thread32Arg[i][1] = (AX_U64)MM_VirtualAddr + (DataLen / THREAD_CNT) * i;
        Thread32Arg[i][2] = MM_PhyBaseAddr + (DataLen / THREAD_CNT) * i;
        Thread32Arg[i][3] = DstPhyAddr + (DataLen / THREAD_CNT) * i;
        Thread32Arg[i][4] = DataLen / THREAD_CNT;
        Thread32Arg[i][5] = i;  //thread number
        Thread32Arg[i][6] = PCIE_PORT_BASE_NUM + 1 + i; //port

        Ret = pthread_create(&(t_dma_test[i]), NULL, PCIe_Dma_32Thread, Thread32Arg[i]);
        if (Ret != 0)
            err("can't create thread%d\n", i);
    }

    for(i = 0; i < THREAD_CNT; i++) {
        pthread_join(t_dma_test[i], NULL);
    }

    return Ret;
}

void help(void)
{
    info("usage: sample_pcie_dma_slave [-h] [-s dma-buffer-size] \n");
    info("   -h      : display usage\n");
    info("   -s      : specify dma buffer size, ex: -s10000. default is 4MB\n");

    return;
}


AX_S32 main(int argc, char *argv[])
{
    AX_S32 PCIeDmaHandle;
    AX_U64 MM_PhyBaseAddr = 0;
    AX_U8  *MM_VirtualAddr = NULL;
    AX_U64 RC_PhyBaseAddr = 0;
    AX_U64 DmaBufferSize = 0;
    AX_S32 Ret;
    AX_S32 DmaTest;
    AX_S32 opt;

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

    Ret = AX_PCIe_InitEpMsg(PCIE_PORT_CNT, PCIE_PORT_BASE_NUM);
    if (Ret < 0) {
        err("init pcie ep msg failed!\n");
        return -1;
    }

    //open pcie dma dev
    PCIeDmaHandle = AX_PCIe_OpenDmaDev();
    if (PCIeDmaHandle < 0) {
        err("open pcie dma dev failed!\n");
        return -1;
    }

    Ret = MM_BufferInit(0, &MM_VirtualAddr, &MM_PhyBaseAddr, DmaBufferSize);
    if (Ret < 0) {
        err("init video buffer failed!\n");
        goto END;
    }

    Ret = AX_PCIe_WaitRcPhyBaseAddr(TEST_PORT, &RC_PhyBaseAddr, -1);
    if (Ret < 0) {
        err("get RC base physical address failed!\n");
        goto END;
    }

    while (1) {
        debug("\nwait a user command msg...\n");
        DmaTest = AX_PCIe_WaitUserCmd(0, TEST_PORT, -1);
        if (DmaTest < 0) {
            err("wait dma test msg failed\n");
            continue;
        }

        switch (DmaTest) {
        case PCIE_DMA_FileSendTest :
            // data flow: ep --> rc
            Ret = SendFileTest(PCIeDmaHandle, 0, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr);
            if (Ret < 0) {
                err("send file test failed!\n");
                goto END;
            }
            break;
        case PCIE_DMA_FileRecvTest :
            // data flow: rc --> ep
            Ret = RecvFileTest(PCIeDmaHandle, 0, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr);
            if (Ret < 0) {
                err("recv file test failed!\n");
                goto END;
            }
            break;
        case PCIE_DMA_DataLoopbackTest :
            // data flow: ep --> rc --> ep
            Ret = LoopbackDataTest(PCIeDmaHandle, 0, TEST_PORT, LP_RECV_FILE, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr,
                                   DmaBufferSize);
            if (Ret < 0) {
                err("loopback data test failed!\n");
            }
            break;
        case PCIE_DMA_BandwidthTest :
            // data flow: ep --> rc --> ep
            Ret = DMA_BandwidthTest(PCIeDmaHandle, 0, TEST_PORT, LP_RECV_FILE, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr,
                                    DmaBufferSize);
            if (Ret < 0) {
                err("dma bandwidth test failed!\n");
            }
            break;
        case PCIE_DMA_DMALinkListTest :
            // data flow: rc --> ep
            Ret = DMA_LinkListTest(PCIeDmaHandle, 0, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr);
            if (Ret < 0) {
                err("dma link list(recv file) test failed!\n");
                goto END;
            }
            break;
        case PCIE_DMA_DMALinkListRecvTest :
            // data flow: ep --> rc
            Ret = DMA_LinkListSendTest(PCIeDmaHandle, 0, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr);
            if (Ret < 0) {
                err("dma link list(send file) test failed!\n");
                goto END;
            }
            break;
        case PCIE_DMA_DMAMultiTaskTest :
            // data flow: rc --> ep
            Ret = DMA_MultiTaskTest(PCIeDmaHandle, 0, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr);
            if (Ret < 0) {
                err("dma multi task(recv file) test failed!\n");
                goto END;
            }
            break;
        case PCIE_DMA_DMAMultiTaskRecvTest :
            // data flow: ep --> rc
            Ret = DMA_MultiTaskSendTest(PCIeDmaHandle, 0, TEST_PORT, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr);
            if (Ret < 0) {
                err("dma multi task(send file) test failed!\n");
                goto END;
            }
            break;
        case PCIE_DMA_MultiTaskLoopbackTest :
            // data flow: ep --> rc --> ep
            Ret = MultiTaskLoopbackTest(PCIeDmaHandle, 0, TEST_PORT, LP_RECV_FILE, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr,
                                        DmaBufferSize);
            if (Ret < 0) {
                err("Multi-Task loopback test failed!\n");
            }
            break;
        case PCIE_DMA_MultiThreadLoopbackTest :
            // data flow: ep --> rc --> ep
            Ret = PCIe_DMA_MultiThreadTest(PCIeDmaHandle, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr, DmaBufferSize);
            if (Ret < 0) {
                err("Multi Thread loopback test failed!\n");
            }
            break;
        case PCIE_DMA_32ThreadLoopbackTest :
            // data flow: ep --> rc --> ep
            Ret = PCIe_DMA_32ThreadTest(PCIeDmaHandle, MM_VirtualAddr, MM_PhyBaseAddr, RC_PhyBaseAddr, DmaBufferSize);
            if (Ret < 0) {
                err("%d Thread loopback test failed!\n", THREAD_CNT);
            }
            break;
        case PCIE_DMA_QuitTest :
            info("get a PCIE_DMA_QuitTest msg, quit test\n");
            goto END;
        default :
            err("unknown dma test type\n");
        }
    }

END:
    AX_PCIe_CloseDmaDev(PCIeDmaHandle);
    info("quit pcie dma test!\n");

    return 0;
}