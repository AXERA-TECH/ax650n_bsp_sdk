#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "ax_base_type.h"
#include "ax_pcie_dma_api.h"
#include "ax_pcie_msg_api.h"


#define PCIE_DMA_DEV "/dev/ax_pcie_dma_transfer"

#define PCIE_DMA_MSG_PORT    1

// ioctl cmd
#define PCIE_BASE       'H'
#define PCIE_DMA_TRANS        _IOW(PCIE_BASE, 1, struct PCIE_DMA_REQ)
#define PCIE_DMA_MULTI_TASK   _IOW(PCIE_BASE, 4, PCIE_DMA_TASK_T)


#define NOOP(...)

#define AX_PCIE_DEBUG NOOP
#define AX_PCIE_INFO  printf
#define AX_PCIE_ERR   printf
#define AX_PCIE_WARN  printf


AX_S32 PciRmtId[PCIE_MAX_CHIPNUM];
AX_S32 AllPciRmtCnt;


typedef enum DMA_MSG_TYPE {
    PCIE_DMA_RC_ADDR = 1,
    PCIE_DMA_READ_DONE,
    PCIE_DMA_WRITE_DONE,
    PCIE_DMA_DATA_READY,
    PCIE_DMA_USER_CMD
} DMA_MSG_TYPE_T;

typedef struct PCIeDmaMsg {
    DMA_MSG_TYPE_T DmaMsgType;
    AX_U8  Data[64]; //struct PCIeDmaMsgData
} PCIE_DMA_MSG_T;

typedef struct PCIeDmaMsgData {
    AX_U64 Addr;
    AX_U32 Size;
    AX_U32 Cmd;
} PCIE_DMA_MSG_DATA_T;

typedef struct PCIE_DMA_TASK {
    AX_U32           Count;     /* total dma task number */
    AX_BOOL          bRead;     /* dam task is  read or write data */
    AX_PCIE_DMA_BLOCK_T *pBlock;
} PCIE_DMA_TASK_T;

typedef struct PCIE_DMA_REQ {
    AX_U32  Dir;
    AX_U64  Src;
    AX_U64  Dst;
    AX_U32  Len;
    AX_U32  Last;
} PCIE_DMA_REQ_T;


AX_S32 AX_PCIe_SendDmaMsg(AX_S32 Target, AX_S32 Port, DMA_MSG_TYPE_T DMA_MSG_Type, PCIE_DMA_MSG_DATA_T Data);
AX_S32 AX_PCIe_RecvDmaMsg(AX_S32 Target, AX_S32 Port, PCIE_DMA_MSG_T *PCIeDmaMsg, AX_S32 nTimeOut);


/**
 * @description: init pcie msg for rc
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_InitRcMsg(AX_U32 PortCnt, AX_U32 PortBaseNum)
{
    AX_S32 Ret = 0;
    AX_S32 i, j;
    AX_S32 RmtChipId;
    AX_S32 PciLocalId;

    AX_PCIE_INFO("axera pcie rc msg init...\n");

    /* Get pcie local id */
    Ret = AX_PCIe_GetLocalId(&PciLocalId);
    if (Ret < 0) {
        AX_PCIE_ERR("host get pcie local id fail\n");
        return -1;
    }

    /* Get pcie tartget id*/
    Ret = AX_PCIe_GetTargetId(PciRmtId, &AllPciRmtCnt);
    if (Ret < 0) {
        AX_PCIE_ERR("host get pcie target id fail\n");
        return -1;
    }
    AX_PCIE_INFO("get %d ep dev\n", AllPciRmtCnt);

    /* Sharemem init */
    for (i = 0; i < AllPciRmtCnt; i++) {
        Ret = AX_PCIe_ShareMemInit(PciRmtId[i]);
        if (Ret < 0) {
            AX_PCIE_ERR("host share mem init fail\n");
            return -1;
        }
    }

    for (i = 0; i < AllPciRmtCnt; i++) {

        RmtChipId = PciRmtId[i];
        AX_PCIE_INFO("RmtChipId = %d\n", RmtChipId);

        for (j = 0; j < PortCnt; j++) {
            /* PCIe open msg port */
            Ret = AX_PCIe_OpenMsgPort(RmtChipId, PortBaseNum + j);
            if (Ret < 0) {
                AX_PCIE_ERR("open msg port fail\n");
                return -1;
            }
        }
    }

    AX_PCIE_INFO("axera pcie rc msg init ok\n");

    return 0;
}


/**
 * @description: init pcie msg for ep
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_InitEpMsg(AX_U32 PortCnt, AX_U32 PortBaseNum)
{
    AX_S32 Ret;
    AX_S32 i;
    AX_S32 PciLocalId;

    AX_PCIE_INFO("axera pcie slave msg init...\n");

    /* Sharemem init */
    Ret = AX_PCIe_ShareMemInit(0);
    if (Ret < 0) {
        AX_PCIE_ERR("slave share mem init fail\n");
        return -1;
    }

    /* Get pcie local id */
    Ret = AX_PCIe_GetLocalId(&PciLocalId);
    if (Ret < 0) {
        AX_PCIE_ERR("slave get pcie local id fail\n");
        return -1;
    }

    for (i = 0; i < PortCnt; i++) {
        /* PCIe open msg port */
        Ret = AX_PCIe_OpenMsgPort(0, PortBaseNum + i);
        if (Ret < 0) {
            AX_PCIE_ERR("open msg port fail\n");
            return -1;
        }
    }

    AX_PCIE_INFO("axera pcie slave msg init ok\n");
    return 0;
}


/**
 * @description: get all pcie device id
 * @param {AX_S32 **} PcieDevId: store pcie device id
 * @return: pcie device count
 */
AX_S32 AX_PCIe_GetPcieDevIdArray(AX_S32 **PcieDevId)
{
    if (PcieDevId && AllPciRmtCnt > 0) {
        *PcieDevId = PciRmtId;
        return AllPciRmtCnt;
    } else {
        AX_PCIE_ERR("NULL pointer\n");
        return -1;
    }
}


/**
 * @description: send dma msg
 * @param {AX_S32} Target: target ID
 * @param {DMA_MSG_TYPE_T} DMA_MSG_Type: dma msg type
 * @param {PCIE_DMA_MSG_DATA_T} Data: dma msg data
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_SendDmaMsg(AX_S32 Target, AX_S32 Port, DMA_MSG_TYPE_T DMA_MSG_Type, PCIE_DMA_MSG_DATA_T Data)
{
    PCIE_DMA_MSG_T PCIeDmaMsg;
    AX_S32  Ret;
    AX_S32  s32Len = 0;

    memset(&PCIeDmaMsg, 0, sizeof(PCIE_DMA_MSG_T));

    // fill pcie dma msg
    PCIeDmaMsg.DmaMsgType = DMA_MSG_Type;
    memcpy(PCIeDmaMsg.Data, (void *)&Data, sizeof(PCIE_DMA_MSG_DATA_T));
    s32Len = sizeof(PCIE_DMA_MSG_T);

    // send pcie msg
    Ret = AX_PCIe_WriteMsg(Target, Port, &PCIeDmaMsg, s32Len);
    if (Ret < 0) {
        AX_PCIE_ERR("send dma msg failed\n");
        return -1;
    }
    AX_PCIE_DEBUG("send dma msg success\n");

    return 0;
}


/**
 * @description: recv msg for dma
 * @param {AX_S32} Target: target ID
 * @param {PCIE_DMA_MSG_T} *PCIeDmaMsg: dma msg
 * @return: success = read size, failure = -1
 */
AX_S32 AX_PCIe_RecvDmaMsg(AX_S32 Target, AX_S32 Port, PCIE_DMA_MSG_T *PCIeDmaMsg, AX_S32 nTimeOut)
{
    AX_S32 s32Len = sizeof(PCIE_DMA_MSG_T);

    return AX_PCIe_ReadMsg(Target, Port, PCIeDmaMsg, s32Len, nTimeOut);
}


/**
 * @description: send data ready msg to ep. rc will call it if want to send data to ep.
 * @param {AX_S32} Target: target pcie device
 * @param {AX_U64} Addr: physical address
 * @param {AX_U32} Size: size
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_SendDataReadyMsg(AX_S32 Target, AX_S32 Port, AX_U64 Addr, AX_U32 Size)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_DATA_T Data;

    Data.Addr = Addr;
    Data.Size = Size;

    Ret = AX_PCIe_SendDmaMsg(Target, Port, PCIE_DMA_DATA_READY, Data);
    if (Ret < 0) {
        AX_PCIE_ERR("send PCIE_DMA_DATA_READY msg failed\n");
        return -1;
    }

    return 0;
}


/**
 * @description: send write done msg. ep will call it after writing data to rc
 * @param {AX_S32} Target: 0
 * @param {AX_U64} Addr: physical address
 * @param {AX_U32} Size: write size
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_SendWriteDoneMsg(AX_S32 Target, AX_S32 Port, AX_U64 Addr, AX_U32 Size)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_DATA_T Data;

    Data.Addr = Addr;
    Data.Size = Size;

    Ret = AX_PCIe_SendDmaMsg(Target, Port, PCIE_DMA_WRITE_DONE, Data);
    if (Ret < 0) {
        AX_PCIE_ERR("send PCIE_DMA_WRITE_DONE msg failed\n");
        return -1;
    }

    return 0;
}


/**
 * @description: send read done msg to ep/rc
 * @param {AX_S32} Target: target ID
 * @param {AX_U32} Size: read size
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_SendReadDoneMsg(AX_S32 Target, AX_S32 Port, AX_U32 Size)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_DATA_T Data;

    Data.Size = Size;

    Ret = AX_PCIe_SendDmaMsg(Target, Port, PCIE_DMA_READ_DONE, Data);
    if (Ret < 0) {
        AX_PCIE_ERR("send PCIE_DMA_READ_DONE msg failed\n");
        return -1;
    }

    return 0;
}


/**
 * @description: wait write done msg from ep. ep will send this msg after writing data to rc.
 * @param {AX_S32} Target: target pcie device
 * @param {AX_U64} *Addr: physical address
 * @param {AX_U32} *Size: write size
 * @return: success = 0, failure = -1 timeout = -2
 */
AX_S32 AX_PCIe_WaitWriteDoneMsg(AX_S32 Target, AX_S32 Port, AX_U64 *Addr, AX_U32 *Size, AX_S32 nTimeOut)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_T PCIeDmaMsg;
    PCIE_DMA_MSG_DATA_T Data;

    Ret = AX_PCIe_RecvDmaMsg(Target, Port, &PCIeDmaMsg, nTimeOut);
    if (Ret > 0) {
        if (PCIeDmaMsg.DmaMsgType == PCIE_DMA_WRITE_DONE) {
            AX_PCIE_DEBUG("get a PCIE_DMA_WRITE_DONE msg\n");
            memcpy(&Data, PCIeDmaMsg.Data, sizeof(PCIE_DMA_MSG_DATA_T));
            *Addr = Data.Addr;
            *Size = Data.Size;
            AX_PCIE_DEBUG("data addr: %llx, size: %d bytes\n", Data.Addr, Data.Size);
            return 0;
        } else {
            AX_PCIE_ERR("get a unknown type dma msg\n");
            return -1;
        }
    } else if (Ret == AX_PCIE_POLL_TIMEOUT) {
        return Ret;
    } else {
        AX_PCIE_ERR("get PCIE_DMA_WRITE_DONE msg failed\n");
        return Ret;
    }
}


/**
 * @description: wait data ready msg from rc. if rc want to send data to ep,
 *               rc will send this msg to ep first.
 * @param {AX_S32} Target: 0
 * @param {AX_U64} *Addr: physical address
 * @param {AX_U32} *Size: data size
 * @return: success = 0, failure = -1, timeout = -2
 */
AX_S32 AX_PCIe_WaitDataReadyMsg(AX_S32 Target, AX_S32 Port, AX_U64 *Addr, AX_U32 *Size, AX_S32 nTimeOut)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_T PCIeDmaMsg;
    PCIE_DMA_MSG_DATA_T Data;

    Ret = AX_PCIe_RecvDmaMsg(Target, Port, &PCIeDmaMsg, nTimeOut);
    if (Ret > 0) {
        if (PCIeDmaMsg.DmaMsgType == PCIE_DMA_DATA_READY) {
            AX_PCIE_DEBUG("get a PCIE_DMA_DATA_READY msg\n");
            memcpy(&Data, PCIeDmaMsg.Data, sizeof(PCIE_DMA_MSG_DATA_T));
            *Addr = Data.Addr;
            *Size = Data.Size;
            AX_PCIE_DEBUG("data addr: %llx, size: %d bytes\n", Data.Addr, Data.Size);
            return 0;
        } else {
            AX_PCIE_ERR("get a unknown type dma msg\n");
            return -1;
        }
    } else if (Ret == AX_PCIE_POLL_TIMEOUT) {
        return Ret;
    } else {
        AX_PCIE_ERR("get PCIE_DMA_DATA_READY msg failed\n");
        return Ret;
    }
}


/**
 * @description: wait read done msg
 * @param {AX_S32} Target: target ID
 * @return read data size, failure = -1, timeout = -2
 */
AX_S32 AX_PCIe_WaitReadDoneMsg(AX_S32 Target, AX_S32 Port, AX_S32 nTimeOut)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_T PCIeDmaMsg;
    PCIE_DMA_MSG_DATA_T Data;

    Ret = AX_PCIe_RecvDmaMsg(Target, Port, &PCIeDmaMsg, nTimeOut);
    if (Ret > 0) {
        if (PCIeDmaMsg.DmaMsgType == PCIE_DMA_READ_DONE) {
            AX_PCIE_DEBUG("get a PCIE_DMA_READ_DONE msg\n");
            memcpy(&Data, PCIeDmaMsg.Data, sizeof(PCIE_DMA_MSG_DATA_T));
            return Data.Size;
        } else {
            AX_PCIE_ERR("get a unknown dma msg\n");
            return -1;
        }
    } else if (Ret == AX_PCIE_POLL_TIMEOUT) {
        return Ret;
    } else {
        AX_PCIE_ERR("get PCIE_DMA_READ_DONE msg failed\n");
        return Ret;
    }
}


/**
 * @description: send user cmd msg to ep/rc
 * @param {AX_S32} Target: target ID
 * @param {AX_U32} Cmd: command
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_SendUserCmd(AX_S32 Target, AX_S32 Port, AX_U32 Cmd)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_DATA_T Data;

    Data.Cmd = Cmd;

    Ret = AX_PCIe_SendDmaMsg(Target, Port, PCIE_DMA_USER_CMD, Data);
    if (Ret < 0) {
        AX_PCIE_ERR("send PCIE_DMA_USER_CMD msg failed\n");
        return -1;
    }

    return 0;
}


/**
 * @description: wait user cmd msg in ep
 * @param {AX_S32} Target: 0
 * @return command type failure = -1, timeout = -2
 */
AX_S32 AX_PCIe_WaitUserCmd(AX_S32 Target, AX_S32 Port, AX_S32 nTimeOut)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_T PCIeDmaMsg;
    PCIE_DMA_MSG_DATA_T Data;

    Ret = AX_PCIe_RecvDmaMsg(Target, Port, &PCIeDmaMsg, nTimeOut);
    if (Ret > 0) {
        if (PCIeDmaMsg.DmaMsgType == PCIE_DMA_USER_CMD) {
            AX_PCIE_DEBUG("get a PCIE_DMA_USER_CMD msg\n");
            memcpy(&Data, PCIeDmaMsg.Data, sizeof(PCIE_DMA_MSG_DATA_T));
            return Data.Cmd;
        } else {
            AX_PCIE_ERR("get a unknown dma msg\n");
            return -1;
        }
    } else if (Ret == AX_PCIE_POLL_TIMEOUT) {
        return Ret;
    } else {
        AX_PCIE_ERR("get PCIE_DMA_USER_CMD msg failed\n");
        return Ret;
    }
}


/**
 * @description: send physical base address to ep
 * @param {AX_S32} Target: target ep device
 * @param {AX_U64} RcPhyBaseAddr: physical base address
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_SendRcPhyBaseAddr(AX_S32 Target, AX_S32 Port, AX_U64 RcPhyBaseAddr)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_DATA_T Data;

    Data.Addr = RcPhyBaseAddr;

    Ret = AX_PCIe_SendDmaMsg(Target, Port, PCIE_DMA_RC_ADDR, Data);
    if (Ret < 0) {
        AX_PCIE_ERR("send PCIE_DMA_RC_ADDR msg fail\n");
        return -1;
    }
    AX_PCIE_DEBUG("send PCIE_DMA_RC_ADDR msg success\n");

    return 0;
}


/**
 * @description: get physical base address from RC, ep dma transfer will use it
 * @param {AX_U64} *RcPhyBaseAddr
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_WaitRcPhyBaseAddr(AX_S32 Port, AX_U64 *RcPhyBaseAddr, AX_S32 nTimeOut)
{
    AX_S32  Ret;
    PCIE_DMA_MSG_T PCIeDmaMsg;
    PCIE_DMA_MSG_DATA_T Data;

    AX_PCIE_DEBUG("wait PCIE_DMA_RC_ADDR msg...\n");

    Ret = AX_PCIe_RecvDmaMsg(0, Port, &PCIeDmaMsg, nTimeOut);
    if (Ret > 0) {
        if (PCIeDmaMsg.DmaMsgType == PCIE_DMA_RC_ADDR) {
            AX_PCIE_DEBUG("get a PCIE_DMA_RC_ADDR msg\n");
            memcpy(&Data, PCIeDmaMsg.Data, sizeof(PCIE_DMA_MSG_DATA_T));
            *RcPhyBaseAddr = Data.Addr;
        } else {
            AX_PCIE_ERR("get a unknown dma msg\n");
            return -1;
        }
    } else if (Ret == AX_PCIE_POLL_TIMEOUT) {
        return Ret;
    } else {
        AX_PCIE_ERR("get PCIE_DMA_RC_ADDR msg failed\n");
        return Ret;
    }

    AX_PCIE_DEBUG("pcie rc mm physical base addr:%p\n", (void *)(*RcPhyBaseAddr));

    return 0;
}


/**
 * @description: create one pcie dma task
 * @param {AX_S32} PCIeDmaHandle: pcie dma device handler
 * @param {AX_U32} Dir: DMA_READ or DMA_WRITE
 * @param {AX_U64} Src: source address
 * @param {AX_U64} Dst: destination address
 * @param {AX_U32} Len: length(byte)
 * @param {AX_U32} Last: if the task is last set it to 1, otherwise set it to 0
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_CreatDmaTask(AX_S32 PCIeDmaHandle, AX_PCIE_DMA_CH_TYPE_E Dir, AX_U64 Src, AX_U64 Dst, AX_U32 Len, AX_U32 Last)
{
    struct PCIE_DMA_REQ DMA_Req;
    AX_S32 Ret = 0;

    DMA_Req.Dir = Dir;
    DMA_Req.Src = Src;
    DMA_Req.Dst = Dst;
    DMA_Req.Len = Len;
    DMA_Req.Last = Last;

    Ret = ioctl(PCIeDmaHandle, PCIE_DMA_TRANS, &DMA_Req);
    if (Ret) {
        AX_PCIE_ERR("pcie dma transfer fail\n");
        return -1;
    } else {
        AX_PCIE_DEBUG("pcie dma transfer success\n");
    }

    return 0;
}


/**
 * @description: create multiple dma task in one time.
 * @param {AX_S32} PCIeDmaHandle: pcie dma device handler
 * @param {AX_BOOL} IsRead: dma read = true, dma write = false
 * @param {AX_U32} Count: the number of dma task
 * @param {AX_PCIE_DMA_BLOCK_T} stDmaBlk: dma task blocks
 * @return: success = 0, failure = -1
 */
AX_S32 AX_PCIe_CreatDmaMultiTask(AX_S32 PCIeDmaHandle, AX_BOOL IsRead, AX_U32 Count, AX_PCIE_DMA_BLOCK_T stDmaBlk[])
{
    PCIE_DMA_TASK_T  stTask;
    AX_S32 Ret = 0;

    stTask.pBlock   = &stDmaBlk[0];
    stTask.Count    = Count;
    stTask.bRead    = IsRead;

    Ret = ioctl(PCIeDmaHandle, PCIE_DMA_MULTI_TASK, &stTask);
    if (Ret) {
        AX_PCIE_ERR("pcie dma multi task transfer failed\n");
        return -1;
    } else {
        AX_PCIE_DEBUG("pcie dma multi task transfer success\n");
    }

    return 0;
}


/**
 * @description: open pcie dma device
 * @return PCIeDmaHandle: if open successfully, return pcie dma device handler.
 */
AX_S32 AX_PCIe_OpenDmaDev(void)
{
    AX_S32 PCIeDmaHandle;

    PCIeDmaHandle = open(PCIE_DMA_DEV, O_RDWR);
    if (PCIeDmaHandle < 0) {
        AX_PCIE_ERR("open %s failed!\n", PCIE_DMA_DEV);
        return -1;
    }

    return PCIeDmaHandle;
}


AX_S32 AX_PCIe_CloseDmaDev(AX_S32 PCIeDmaHandle)
{
    AX_S32 Ret;

    Ret = close(PCIeDmaHandle);
    if (Ret < 0) {
        AX_PCIE_ERR("close %s failed!\n", PCIE_DMA_DEV);
        return -1;
    }

    return 0;
}