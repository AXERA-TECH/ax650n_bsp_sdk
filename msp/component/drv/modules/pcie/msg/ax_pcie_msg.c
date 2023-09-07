#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include "ax_pcie_msg_api.h"
#include "ax_base_type.h"

#define PCIE_MSG_DEV    "/dev/msg_userdev"
#define	AX_IOC_MSG_BASE     'M'

#define AX_MSG_IOC_CONNECT		_IOW(AX_IOC_MSG_BASE, 1, AX_MSG_HANDLE_ATTR_T)
#define AX_MSG_IOC_CHECK		_IOW(AX_IOC_MSG_BASE, 2, AX_MSG_HANDLE_ATTR_T)
#define AX_MSG_IOC_GET_LOCAL_ID		_IOW(AX_IOC_MSG_BASE, 4, AX_MSG_HANDLE_ATTR_T)
#define AX_MSG_IOC_GET_REMOTE_ID        _IOW(AX_IOC_MSG_BASE, 5, AX_MSG_HANDLE_ATTR_T)
#define AX_MSG_IOC_ATTR_INIT		_IOW(AX_IOC_MSG_BASE, 6, AX_MSG_HANDLE_ATTR_T)
#define AX_MSG_IOC_RESET_DEVICE		_IOW(AX_IOC_MSG_BASE, 7, AX_MSG_HANDLE_ATTR_T)
#define AX_MSG_IOC_PCIE_STOP		_IOW(AX_IOC_MSG_BASE, 8, AX_MSG_HANDLE_ATTR_T)

#define PCIE_MSG_BASE_PORT          0
#define PCIE_MSG_MAX_PORT           ((PCIE_MSG_BASE_PORT)+(PCIE_MSG_MAX_PORT_NUM))

static int g_MsgFd[PCIE_MAX_CHIPNUM][PCIE_MSG_MAX_PORT_NUM+1];

#define NOOP(...)

#define AX_PCIE_DEBUG NOOP
#define AX_PCIE_INFO  printf
#define AX_PCIE_ERR   printf
#define AX_PCIE_WARN  printf


AX_S32 AX_PCIe_OpenMsgPort(AX_S32 s32PciTgtId, AX_S32 s32PciPort)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 s32MsgFd;
    AX_MSG_HANDLE_ATTR_T stAttr;

    if (s32PciTgtId >= PCIE_MAX_CHIPNUM || s32PciPort >= PCIE_MSG_MAX_PORT)
    {
        AX_PCIE_ERR("invalid pci msg port(%d,%d)!\n", s32PciTgtId, s32PciPort);
        return AX_FAILURE;
    }

    if (g_MsgFd[s32PciTgtId][s32PciPort - PCIE_MSG_BASE_PORT] > 0)
    {
        AX_PCIE_ERR("pci msg port(%d,%d) have open!\n", s32PciTgtId, s32PciPort);
        return AX_FAILURE;
    }

    s32MsgFd = open(PCIE_MSG_DEV, O_RDWR);
    if (s32MsgFd <= 0)
    {
        AX_PCIE_ERR("open pci msg dev fail!\n");
        return AX_FAILURE;
    }

    AX_PCIE_DEBUG("open port s32MsgFd = %d\n", s32MsgFd);

    stAttr.s32Target_id = s32PciTgtId;
    stAttr.s32Port      = s32PciPort;
    s32Ret = ioctl(s32MsgFd, AX_MSG_IOC_CONNECT, &stAttr);
    if (s32Ret)
    {
        AX_PCIE_ERR("AX_MSG_IOC_CONNECT AX_PCIE_ERR, target:%d, port:%d\n", s32PciTgtId, s32PciPort);
        return AX_FAILURE;
    }

    g_MsgFd[s32PciTgtId][s32PciPort - PCIE_MSG_BASE_PORT] = s32MsgFd;
    return AX_SUCCESS;
}

AX_S32 AX_PCIe_CloseMsgPort(AX_S32 s32PciTgtId, AX_S32 s32PciPort)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 s32MsgFd;
    AX_MSG_HANDLE_ATTR_T stAttr;

    if (s32PciTgtId >= PCIE_MAX_CHIPNUM || s32PciPort >= PCIE_MSG_MAX_PORT)
    {
        AX_PCIE_ERR("invalid pci msg port(%d,%d)!\n", s32PciTgtId, s32PciPort);
        return AX_FAILURE;
    }

    s32MsgFd = g_MsgFd[s32PciTgtId][s32PciPort - PCIE_MSG_BASE_PORT];
    if (s32MsgFd <= 0)
    {
        return AX_FAILURE;
    }

    stAttr.s32Target_id = s32PciTgtId;
    stAttr.s32Port      = s32PciPort;
    s32Ret = ioctl(s32MsgFd, AX_MSG_IOC_PCIE_STOP, &stAttr);
    if (s32Ret)
    {
        AX_PCIE_ERR("AX_MSG_IOC_PCIE_STOP AX_PCIE_ERR, target:%d, port:%d\n", s32PciTgtId, s32PciPort);
    }

    AX_PCIE_DEBUG("===================close port %d!\n", s32PciPort);
    close(s32MsgFd);
    g_MsgFd[s32PciTgtId][s32PciPort - PCIE_MSG_BASE_PORT] = -1;

    return AX_SUCCESS;
}

AX_S32 AX_PCIe_GetLocalId(AX_S32 *pPciLoacalId)
{
    AX_S32 s32Fd;
    AX_MSG_HANDLE_ATTR_T stAttr;

    if (pPciLoacalId == NULL)
        return AX_FAILURE;

    s32Fd = open(PCIE_MSG_DEV, O_RDWR);
    if (s32Fd <= 0)
    {
        AX_PCIE_ERR("open msg user dev fail\n");
        return AX_FAILURE;
    }

    *pPciLoacalId = ioctl(s32Fd, AX_MSG_IOC_GET_LOCAL_ID, &stAttr);
    if (*pPciLoacalId < 0) {
        AX_PCIE_ERR("get invaild local id is %d \n", *pPciLoacalId);
        close(s32Fd);
        return AX_FAILURE;
    }
    AX_PCIE_DEBUG("pci local id is %d \n", *pPciLoacalId);

    close(s32Fd);
    return 0;
}

AX_S32 AX_PCIe_GetTargetId(AX_S32 *pPciTgtId, AX_S32 *pPciRmtCnt)
{
    AX_S32 s32Fd;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 i;
    AX_MSG_HANDLE_ATTR_T stAttr;

    if (pPciTgtId == NULL || pPciRmtCnt == NULL) {
        AX_PCIE_ERR("PciTgtId or PciRmtCnt pointer is NULL\n");
        return AX_FAILURE;
    }

    s32Fd = open(PCIE_MSG_DEV, O_RDWR);
    if (s32Fd <= 0)
    {
        AX_PCIE_ERR("open msg user dev fail\n");
        return AX_FAILURE;
    }

    if (ioctl(s32Fd, AX_MSG_IOC_ATTR_INIT, &stAttr))
    {
	    AX_PCIE_ERR("initialization for stAttr failed!\n");
	    s32Ret = AX_FAILURE;
        goto out;
    }

    if (ioctl(s32Fd, AX_MSG_IOC_GET_REMOTE_ID, &stAttr))
    {
        AX_PCIE_ERR("get pci remote id fail \n");
        s32Ret = AX_FAILURE;
        goto out;
    }

    for (i = 0; i < AX_MAX_MAP_DEV-1; i++)
    {
        if (-1 == stAttr.s32Remote_id[i])
            break;
        *(pPciTgtId++) = stAttr.s32Remote_id[i];
        AX_PCIE_ERR("get pci remote id : %d \n", stAttr.s32Remote_id[i]);
    }

    *pPciRmtCnt = i;

out:
    close(s32Fd);
    return s32Ret;
}

AX_S32 AX_PCIe_ShareMemInit(AX_S32 s32PciTgtId)
{
    AX_S32 s32Fd;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_MSG_HANDLE_ATTR_T stAttr;

    s32Fd = open(PCIE_MSG_DEV, O_RDWR);
    if (s32Fd <= 0)
    {
        AX_PCIE_ERR("open pci msg dev fail!\n");
        return AX_FAILURE;
    }
    AX_PCIE_DEBUG("open msg dev ok, fd:%d\n", s32Fd);

    if (ioctl(s32Fd, AX_MSG_IOC_ATTR_INIT, &stAttr))
    {
        AX_PCIE_ERR("initialization for stAttr failed!\n");
        s32Ret = AX_FAILURE;
        goto out;
    }

    stAttr.s32Target_id = s32PciTgtId;
    AX_PCIE_DEBUG("start check pci target id:%d  ... ... ... \n", s32PciTgtId);
    if (ioctl(s32Fd, AX_MSG_IOC_CHECK, &stAttr))
    {
        AX_PCIE_ERR("share mem check fail.\n");
        s32Ret = AX_FAILURE;
        goto out;
    }
    AX_PCIE_DEBUG("have checked pci target id:%d ok ! \n", s32PciTgtId);

out:
    close(s32Fd);
    return s32Ret;
}

AX_S32 AX_PCIe_WriteMsg(AX_S32 s32PciTgtId, AX_S32 s32PciPort, AX_VOID *pPciMsg, AX_S32 s32Len)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 s32MsgFd;

    if(pPciMsg == NULL) {
        AX_PCIE_ERR("pcie msg is null\n");
        return AX_FAILURE;
    }

    if (s32PciTgtId >= PCIE_MAX_CHIPNUM || s32PciPort >= PCIE_MSG_MAX_PORT)
    {
        AX_PCIE_ERR("invalid pci msg port(%d,%d)!\n", s32PciTgtId, s32PciPort);
        return AX_FAILURE;
    }

    if (g_MsgFd[s32PciTgtId][s32PciPort-PCIE_MSG_BASE_PORT] <= 0)
    {
        AX_PCIE_ERR("you should open msg port before send message !\n");
        return AX_FAILURE;
    }
    s32MsgFd = g_MsgFd[s32PciTgtId][s32PciPort-PCIE_MSG_BASE_PORT];

    s32Ret = write(s32MsgFd, pPciMsg, s32Len);
    if (s32Ret != s32Len)
    {
        AX_PCIE_ERR("AX_PCIe_WriteMsg write_len AX_PCIE_ERR:%d\n", s32Ret);
        return AX_FAILURE;
    }
    return AX_SUCCESS;
}

AX_S32 AX_PCIe_ReadMsg(AX_S32 s32PciTgtId, AX_S32 s32PciPort, AX_VOID *pPciMsg, AX_S32 s32Len, AX_S32 nTimeOut)
{
    struct pollfd MsgEvent;
    AX_S32 s32Ret, s32PortIndex;
    AX_U32 u32MsgLen = s32Len;

    if(pPciMsg == NULL) {
        AX_PCIE_ERR("pcie read pPciMsg is null\n");
        return AX_FAILURE;
    }

    if (s32PciTgtId >= PCIE_MAX_CHIPNUM || s32PciPort >= PCIE_MSG_MAX_PORT)
    {
        AX_PCIE_ERR("invalid pci msg port(%d,%d)!\n", s32PciTgtId, s32PciPort);
        return AX_FAILURE;
    }
    s32PortIndex = s32PciPort - PCIE_MSG_BASE_PORT;

    if (g_MsgFd[s32PciTgtId][s32PortIndex] <= 0)
    {
        AX_PCIE_ERR("you should open msg port before read message!\n");
        return AX_FAILURE;
    }

    MsgEvent.fd = g_MsgFd[s32PciTgtId][s32PortIndex];
    MsgEvent.events = POLLIN | POLLRDNORM;

    s32Ret = poll(&MsgEvent, 1, nTimeOut);
    if (s32Ret < 0) {
        AX_PCIE_ERR("PCIe read msg poll error\n");
        return AX_FAILURE;
    } else if (s32Ret == 0) {
        return AX_PCIE_POLL_TIMEOUT;
    }

    if (!(MsgEvent.revents & (POLLIN | POLLRDNORM))) {
         AX_PCIE_ERR("PCIe read msg poll event error\n");
        return AX_FAILURE;
    }

    s32Ret = read(g_MsgFd[s32PciTgtId][s32PortIndex], pPciMsg, u32MsgLen);
    if (s32Ret <= 0)
    {
        return AX_FAILURE;
    }

    return s32Ret;
}

AX_S32 AX_PCIe_Reset(AX_S32 s32PciTgtId)
{
    AX_S32 s32Fd;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_MSG_HANDLE_ATTR_T stAttr;

    s32Fd = open(PCIE_MSG_DEV, O_RDWR);
    if (s32Fd <= 0)
    {
        AX_PCIE_ERR("open msg user dev fail\n");
        return AX_FAILURE;
    }

    stAttr.s32Target_id = s32PciTgtId;

    if (ioctl(s32Fd, AX_MSG_IOC_RESET_DEVICE, &stAttr))
    {
	    AX_PCIE_ERR("initialization for stAttr failed!\n");
	    return AX_FAILURE;
    }

    return s32Ret;
}



