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
#include "ax_pcie_msg_api.h"

#define ICC_TEST_BUF_SIZE_MAX   0x1000

typedef struct ICC_structHdl {
    AX_S32 s32TgtId;
    AX_S32 s32Port;
} ICC_structHdl_T;

typedef struct ICC_TestMsgHead {
    AX_S32 s32CheckSum;
    AX_S32 s32DateLen;
} ICC_TestMsgHead_T;

typedef struct ICC_Testbuf {
    ICC_TestMsgHead_T stMsgHead;
    AX_S32 s32Date[ICC_TEST_BUF_SIZE_MAX];
} ICC_Testbuf_T;

struct ICC_TransOps {
    AX_S32(*create)(AX_VOID *pHdl, AX_CHAR *pSvrRoute, AX_S32 s32Port, AX_S32 s32Mode, AX_VOID *pPriv);
    AX_S32(*send)(ICC_structHdl_T stTransHdl, AX_VOID *pBuf, AX_U32 s32Len, AX_U32 s32BufType, AX_U32 u32Timeout);
    AX_S32(*recv)(ICC_structHdl_T stTransHdl, AX_VOID *pBuf, AX_U32 s32Len, AX_S32 nTimeOut);
    AX_S32(*release)(AX_VOID *pHdl, AX_S32 s32Mode);
};

AX_U32 CheckSum(AX_U8 *pBuffer, AX_S32 u8BufLen)
{
    AX_U32 u8Index;
    AX_U32 u8Rtn;
    AX_U32 sum = 0;
    for (u8Index = 0; u8Index < u8BufLen; u8Index++) {
        sum += pBuffer[u8Index];
    }
    u8Rtn = (sum & 0x00FF);
    return u8Rtn;
}

AX_S32 Random_fill_MsgData(AX_VOID *pstPciMsg)
{
    AX_S32 s32Fd = 0;
    AX_S32 s32Size = 0;
    AX_S32 s32Ret = 0;

    if (pstPciMsg == NULL) {
        printf("pcie msg is null\n");
        return AX_FAILURE;
    }

    srand((AX_ULONG)time(NULL));
    s32Size = (rand() % 1024 + 0);

    s32Fd = open("/dev/urandom", O_RDONLY);
    if (s32Fd < 0) {
        printf("open /dev/urandom failed\n");
        return AX_FAILURE;
    }

    s32Ret = read(s32Fd, pstPciMsg, s32Size);
    if (s32Ret < 0) {
        printf("read /dev/urandom data failed\n");
        close(s32Fd);
        return AX_FAILURE;
    }

    close(s32Fd);
    return s32Size;
}


AX_S32 ICC_pcieCreate(AX_VOID *pHdl, AX_CHAR *pSvrRoute, AX_S32 s32Port, AX_S32 s32Mode, AX_VOID *pPriv)
{
    AX_S32 s32TargetId;

    s32TargetId = atoi(pSvrRoute);
    ((ICC_structHdl_T *)pHdl)->s32TgtId = s32TargetId;
    ((ICC_structHdl_T *)pHdl)->s32Port = s32Port;

    return AX_PCIe_OpenMsgPort(s32TargetId, s32Port);
}

AX_S32 ICC_pcieSend(ICC_structHdl_T stTransHdl, AX_VOID *pBuf, AX_U32 u32Len, AX_U32 u32BufType, AX_U32 u32Timeout)
{
    return AX_PCIe_WriteMsg(stTransHdl.s32TgtId, stTransHdl.s32Port, pBuf, u32Len);
}

AX_S32 ICC_pcieRecv(ICC_structHdl_T stTransHdl, AX_VOID *pBuf, AX_U32 u32Len, AX_S32 nTimeOut)
{
    AX_S32 s32Ret;

    return s32Ret = AX_PCIe_ReadMsg(stTransHdl.s32TgtId, stTransHdl.s32Port, pBuf, u32Len, nTimeOut);
}

AX_S32 ICC_pcieRelease(AX_VOID *pHdl, AX_S32 s32Mode)
{
    ICC_structHdl_T *pstMsgHdl;

    pstMsgHdl = (ICC_structHdl_T *)pHdl;
    return AX_PCIe_CloseMsgPort(pstMsgHdl->s32TgtId, pstMsgHdl->s32Port);
}

AX_S32 AX_PCIe_CommonInit(AX_CHAR *pSvrRoute)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 s32PciLocalId;
    AX_S32 s32TargetId;

    /* The kernel driver needs to record the slot value */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    s32TargetId = atoi(pSvrRoute);
    /* We know the bus number of this device through lspci */
    s32Ret = AX_PCIe_ShareMemInit(s32TargetId);
    if (s32Ret < 0) {
        printf("host share mem init fail\n");
        return s32Ret;
    }

    return s32Ret;
}

struct ICC_TransOps icc_pcie_ops = {
    .create = ICC_pcieCreate,
    .send = ICC_pcieSend,
    .recv = ICC_pcieRecv,
    .release = ICC_pcieRelease,
};


AX_S32 main(AX_S32 argc, AX_CHAR *argv[])
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_CHAR *pSverRoute = "0";
    ICC_structHdl_T pstTransHandle;
    ICC_Testbuf_T pstTestBuf;
    AX_S32 s32Size;
    AX_U32 u32CheckSum;
    AX_S32 s32MsgLen;

    s32Ret = AX_PCIe_CommonInit(pSverRoute);
    if (s32Ret != AX_SUCCESS) {
        printf("AX PCIe common init failed\n");
    }


    s32Ret = ICC_pcieCreate(&pstTransHandle, pSverRoute, PCIE_MSGPORT_COMM_CMD + 1, 0, NULL);
    if (s32Ret != AX_SUCCESS) {
        printf("pcie channal create failed\n");
        return AX_FAILURE;
    }

    memset(&pstTestBuf, 0, sizeof(ICC_Testbuf_T));
    s32MsgLen = sizeof(ICC_Testbuf_T);
    s32Size = ICC_pcieRecv(pstTransHandle, &pstTestBuf, s32MsgLen, -1);
    if (s32Size > 0) {
        if (pstTestBuf.stMsgHead.s32CheckSum == CheckSum((void *)pstTestBuf.s32Date, s32Size - sizeof(ICC_TestMsgHead_T))) {
            printf("Check recv host send msg success\n");
        } else {
            printf("Check recv host send msg failed\n");
            s32Ret = AX_FAILURE;
            goto out;
        }
    }

    memset(&pstTestBuf, 0, sizeof(ICC_Testbuf_T));
    s32Size = Random_fill_MsgData((void *)pstTestBuf.s32Date);
    if (s32Size < 0) {
        printf("Fill Msg data failed\n");
        s32Ret = AX_FAILURE;
        goto out;
    }

    u32CheckSum = CheckSum((void *)pstTestBuf.s32Date, s32Size);
    pstTestBuf.stMsgHead.s32CheckSum = u32CheckSum;
    pstTestBuf.stMsgHead.s32DateLen = s32Size;
    s32MsgLen = sizeof(ICC_TestMsgHead_T) + s32Size;

    s32Ret = ICC_pcieSend(pstTransHandle, &pstTestBuf, s32MsgLen, 0, 0);
    if (s32Ret != AX_SUCCESS) {
        printf("host pcie send msg fail\n");
        s32Ret = AX_FAILURE;
        goto out;
    }

out:
    ICC_pcieRelease(&pstTransHandle, 0);
    return s32Ret;
}