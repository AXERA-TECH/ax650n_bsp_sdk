#ifndef PCIE_DMA_H__
#define PCIE_DMA_H__

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct PCIE_DMA_BLOCK {
    AX_U64  u64SrcAddr;  /* source address of dma task */
    AX_U64  u64DstAddr;  /* destination address of dma task */
    AX_U32  u32BlkSize;  /* data block size of dma task */
} AX_PCIE_DMA_BLOCK_T;

typedef enum PCIE_DMA_CH_TYPE {
    DMA_READ   = 0x0,
    DMA_WRITE  = 0x1,
} AX_PCIE_DMA_CH_TYPE_E;


AX_S32 AX_PCIe_OpenDmaDev(void);
AX_S32 AX_PCIe_CloseDmaDev(AX_S32 s32PCIeDmaHandle);
AX_S32 AX_PCIe_CreatDmaTask(AX_S32 s32PCIeDmaHandle, AX_PCIE_DMA_CH_TYPE_E enDir, AX_U64 u64Src, AX_U64 u64Dst, AX_U32 u32Len, AX_U32 u32Last);
AX_S32 AX_PCIe_CreatDmaMultiTask(AX_S32 s32PCIeDmaHandle, AX_BOOL bIsRead, AX_U32 u32Count, AX_PCIE_DMA_BLOCK_T stDmaBlk[]);

AX_S32 AX_PCIe_InitRcMsg(AX_U32 u32PortCnt, AX_U32 u32PortBaseNum);
AX_S32 AX_PCIe_InitEpMsg(AX_U32 u32PortCnt, AX_U32 u32PortBaseNum);

AX_S32 AX_PCIe_GetPcieDevIdArray(AX_S32 **ppDevIdArray);

AX_S32 AX_PCIe_SendDataReadyMsg(AX_S32 s32Target, AX_S32 s32Port, AX_U64 u64Addr, AX_U32 u32Size);
AX_S32 AX_PCIe_SendWriteDoneMsg(AX_S32 s32Target, AX_S32 s32Port, AX_U64 u64Addr, AX_U32 u32Size);
AX_S32 AX_PCIe_SendReadDoneMsg(AX_S32 s32Target, AX_S32 s32Port, AX_U32 u32Size);

AX_S32 AX_PCIe_WaitDataReadyMsg(AX_S32 s32Target, AX_S32 s32Port, AX_U64 *pAddr, AX_U32 *pSize, AX_S32 nTimeOut);
AX_S32 AX_PCIe_WaitWriteDoneMsg(AX_S32 s32Target, AX_S32 s32Port, AX_U64 *pAddr, AX_U32 *pSize, AX_S32 nTimeOut);
AX_S32 AX_PCIe_WaitReadDoneMsg(AX_S32 s32Target, AX_S32 s32Port, AX_S32 nTimeOut);

AX_S32 AX_PCIe_SendUserCmd(AX_S32 s32Target, AX_S32 s32Port, AX_U32 u32Cmd);
AX_S32 AX_PCIe_WaitUserCmd(AX_S32 s32Target, AX_S32 s32Port, AX_S32 nTimeOut);

AX_S32 AX_PCIe_SendRcPhyBaseAddr(AX_S32 s32Target, AX_S32 s32Port, AX_U64 u64RcPhyBaseAddr);
AX_S32 AX_PCIe_WaitRcPhyBaseAddr(AX_S32 s32Port, AX_U64 *pRcPhyBaseAddr, AX_S32 nTimeOut);

#ifdef __cplusplus
}
#endif

#endif