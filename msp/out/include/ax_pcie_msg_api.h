/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef PCIE_MSG_H__
#define PCIE_MSG_H__

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AX_MAX_MAP_DEV          0x100
#define PCIE_MAX_CHIPNUM        0x100

#define PCIE_MSG_MAX_PORT_NUM       128   /* max msg port count, also you can change it */
#define PCIE_MSGPORT_COMM_CMD       10  /* we use msg port above this value */

#define AX_SUCCESS  0
#define AX_FAILURE  (-1)
#define AX_PCIE_POLL_TIMEOUT (-2)

typedef struct AX_MSG_HANDLE_ATTR {
	AX_S32 s32Target_id;
	AX_S32 s32Port;
	AX_S32 s32Remote_id[AX_MAX_MAP_DEV];
} AX_MSG_HANDLE_ATTR_T;

AX_S32 AX_PCIe_GetLocalId(AX_S32 *pPciLoacalId);
AX_S32 AX_PCIe_GetTargetId(AX_S32 *pPciTgtId, AX_S32 *pPciRmtCnt);
AX_S32 AX_PCIe_ShareMemInit(AX_S32 s32PciTgtId);
AX_S32 AX_PCIe_WriteMsg(AX_S32 s32PciTgtId, AX_S32 s32PciPort, AX_VOID *pPciMsg, AX_S32 s32Len);
AX_S32 AX_PCIe_ReadMsg(AX_S32 s32PciTgtId, AX_S32 s32PciPort, AX_VOID *pPciMsg, AX_S32 s32Len, AX_S32 nTimeOut);
AX_S32 AX_PCIe_OpenMsgPort(AX_S32 s32PciTgtId, AX_S32 s32PciPort);
AX_S32 AX_PCIe_CloseMsgPort(AX_S32 s32PciTgtId, AX_S32 s32PciPort);
AX_S32 AX_PCIe_Reset(AX_S32 s32PciTgtId);
#ifdef __cplusplus
}
#endif

#endif