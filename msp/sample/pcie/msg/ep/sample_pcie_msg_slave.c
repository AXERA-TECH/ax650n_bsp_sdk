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
#include <ax_pcie_msg_api.h>

#define AX_PCIE_MSG_MAXLEN      1024
#define MAX_THREAD_NUM          3

typedef struct AX_PCIE_MSGHEAD {
    AX_U32  u32Target;
    AX_U32  u32MsgType;
    AX_U32  u32MsgLen;
    AX_S32  s32RetVal;
    AX_U8   u8CheckSum;
} AX_PCIE_MSGHEAD_T;

typedef struct AX_PCIE_MSG {
    AX_PCIE_MSGHEAD_T   stMsgHead;
    AX_U8   u8MsgBody[AX_PCIE_MSG_MAXLEN];
} AX_PCIE_MSG_T;

typedef struct AX_PCIE_THREAD {
    AX_S32 s32Id;
    AX_S32 s32MaxLoop;
    AX_S32 s32TargetId;
    AX_U8  u8ThreadMsgPort;
} AX_PCIE_THREAD_T;

typedef enum AX_SAMPLE_PCIE_MSG_TYPE {
    SAMPLE_PCIE_MSG_TEST_WRITE,
    SAMPLE_PCIE_MSG_TEST_READ,
    SAMPLE_PCIE_MSG_TEST_COPY,
    SAMPLE_PCIE_MSG_RING_BUF_TEST,
    SAMPLE_PCIE_MSG_TEST_ECHO,
    SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG,
    SAMPLE_PCIE_CREATE_MULTI_THREAD_PORT,
    SAMPLE_PCIE_START_MULTI_THREAD_TEST,
    SAMPLE_PCIE_EXIT_MULTI_THREAD_TEST,
} AX_SAMPLE_PCIE_MSG_TYPE_E;

pthread_t dev_th[MAX_THREAD_NUM];

AX_PCIE_THREAD_T stThread[MAX_THREAD_NUM];

AX_U8 CheckSum(AX_U8 *pBuffer, AX_U8 u8BufLen)
{
    AX_U8 u8Index;
    AX_U8 u8Rtn;
    AX_U16 sum = 0;
    for (u8Index = 0; u8Index < u8BufLen; u8Index++) {
        sum += pBuffer[u8Index];
    }
    u8Rtn = (AX_U8)(sum & 0x00FF);
    return u8Rtn;
}

AX_S32 Random_MsgBody_Test(AX_PCIE_MSG_T *pstPciMsg)
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

    read(s32Fd, pstPciMsg->u8MsgBody, s32Size);
    if (s32Ret < 0) {
        printf("read /dev/urandom data failed\n");
        close(s32Fd);
        return AX_FAILURE;
    }

    close(s32Fd);
    return s32Size;
}

AX_S32 Sample_PCIe_Create_Multi_Thread_Port(AX_PCIE_MSG_T *pstMsg)
{
    AX_S32 s32Ret, i;

    // pstThread = malloc(sizeof(AX_PCIE_THREAD_T));
    memcpy(stThread, pstMsg->u8MsgBody, pstMsg->stMsgHead.u32MsgLen);
    for (i = 0; i < MAX_THREAD_NUM; i++) {
        s32Ret = AX_PCIe_OpenMsgPort(0, stThread[i].u8ThreadMsgPort);
        if (s32Ret < 0) {
            printf("slave open th %d msg port fail\n", i);
            return s32Ret;
        }
    }
    return s32Ret;
}

AX_VOID *MultThread_Test(void *pArg)
{
    AX_S32 s32Ret = 0;
    AX_S32 s32Size;
    AX_S32 th = *(AX_S32 *)pArg;
    AX_PCIE_MSG_T  stMsg;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);
    AX_U32 u32MsgLen = 0;
    AX_S32 s32EchoMsgLen = 0;

    printf("==============Sample_PCIe_Thread_test [%d]====================\n", th);

    while (1) {
        s32Ret = AX_PCIe_ReadMsg(0, stThread[th].u8ThreadMsgPort, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(10000);
            continue;
        }

        if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_EXIT_MULTI_THREAD_TEST) {
            printf("Sample_PCIe_Thread_test [%d] Exit!\n", th);
            stMsg.stMsgHead.u32Target = 0;
            stMsg.stMsgHead.s32RetVal = AX_SUCCESS;
            stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_ECHO;
            stMsg.stMsgHead.u32MsgLen = 0;
            s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);

            /* PCIe write msg */
            s32Ret = AX_PCIe_WriteMsg(0, stThread[th].u8ThreadMsgPort, &stMsg, s32Len);
            if (s32Ret < 0) {
                printf("slave PCIe send msg fail\n");
            }
            break;
        }

        stMsg.stMsgHead.u32MsgLen = s32Ret - sizeof(AX_PCIE_MSG_T);
        u32MsgLen = stMsg.stMsgHead.u32MsgLen;
#if 0
        for (AX_S32 i = 0; i < u32MsgLen / sizeof(stMsg.u8MsgBody[0]); i += 4) {
            printf("u8MsgBody[%d] = %x%x%x%x\n", i, stMsg.u8MsgBody[i], stMsg.u8MsgBody[i + 1], stMsg.u8MsgBody[i + 2],
                   stMsg.u8MsgBody[i + 3]);
        }
#endif
        if (stMsg.stMsgHead.u8CheckSum == CheckSum(stMsg.u8MsgBody, u32MsgLen)) {
            printf("checksum success!\n");
            memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
            s32Size = Random_MsgBody_Test(&stMsg);
            s32EchoMsgLen = s32Size;
            stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, s32EchoMsgLen);
            s32Ret = AX_SUCCESS;
        } else {
            printf("checksum fail!\n");
            s32Ret = AX_FAILURE;
        }

        stMsg.stMsgHead.u32Target = 0;
        stMsg.stMsgHead.s32RetVal = s32Ret;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_ECHO;
        stMsg.stMsgHead.u32MsgLen = s32EchoMsgLen;
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);

        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(0, stThread[th].u8ThreadMsgPort, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("slave PCIe send msg fail\n");
        }
    }
    pthread_exit(NULL);;
}

AX_S32 Sample_PCIe_Multi_Thread_Test_Start(AX_PCIE_MSG_T *pstMsg)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 th[MAX_THREAD_NUM] = {0, 1, 2};
    AX_S32 i;
    AX_S32 s32Len;

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_create(&dev_th[i], NULL, MultThread_Test, &th[i]);
    }

    pstMsg->stMsgHead.u32Target = 0;
    pstMsg->stMsgHead.s32RetVal = s32Ret;
    pstMsg->stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_ECHO;
    pstMsg->stMsgHead.u32MsgLen = 0;
    s32Len = pstMsg->stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);

    /* PCIe write msg */
    s32Ret = AX_PCIe_WriteMsg(0, PCIE_MSGPORT_COMM_CMD, pstMsg, s32Len);
    if (s32Ret < 0) {
        printf("slave PCIe send msg fail\n");
        goto out;
    }

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_join(dev_th[i], NULL);
    }

out:
    for (i = 0; i < MAX_THREAD_NUM; i++) {
        s32Ret = AX_PCIe_CloseMsgPort(0, stThread[i].u8ThreadMsgPort);
        if (s32Ret != AX_SUCCESS) {
            printf("host pcie close msg port fail\n");
            return s32Ret;
        }
    }
    return s32Ret;
}

AX_S32 main(AX_S32 argc, AX_CHAR *argv[])
{
    AX_S32 s32Ret;
    AX_S32 s32Size = 0;
    AX_PCIE_MSG_T  stMsg;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);
    AX_U8 u8TestMsgBody[4] = {0};
    AX_U8 u8TestMsgBody1[4] = {0x12, 0x23, 0x34, 0x45};
    AX_U32 u32MsgType = 0;
    AX_U32 u32MsgLen = 0;
    AX_S32 s32EchoMsgLen = 0;
    AX_S32 s32PciLocalId;
    AX_S32 i;

    printf("axera pcie slave msg transfer demo\n");

    /* Sharemem init */
    s32Ret = AX_PCIe_ShareMemInit(0);
    if (s32Ret < 0) {
        printf("slave share mem init fail\n");
        return 0;
    }

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("slave get pcie local id fail\n");
        return 0;
    }

    /* PCIe open msg port */
    s32Ret = AX_PCIe_OpenMsgPort(0, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret < 0) {
        printf("slave open msg port fail\n");
        return 0;
    }

    while (1) {

        s32EchoMsgLen = 0;

        s32Ret = AX_PCIe_ReadMsg(0, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(10000);
            continue;
        }
        stMsg.stMsgHead.u32MsgLen = (s32Ret - sizeof(AX_PCIE_MSGHEAD_T));
        u32MsgType = stMsg.stMsgHead.u32MsgType;
        u32MsgLen = stMsg.stMsgHead.u32MsgLen;
        printf("u32MsgLen : %d\n", u32MsgLen);

        switch (u32MsgType) {
        case SAMPLE_PCIE_MSG_TEST_READ: {
            printf("receive msg, u32MsgType: Read\n");
            memcpy(u8TestMsgBody, stMsg.u8MsgBody, sizeof(u8TestMsgBody));
            for (i = 0; i < u32MsgLen / sizeof(u8TestMsgBody[0]); i++) {
                printf("TestMsgBoby[%d] = %x\n", i, u8TestMsgBody[i]);
            }
            if (stMsg.stMsgHead.u8CheckSum == CheckSum(u8TestMsgBody, u32MsgLen)) {
                printf("checksum success!\n");
                s32Ret = AX_SUCCESS;
            } else {
                printf("checksum fail!\n");
                s32Ret = AX_FAILURE;
            }
            break;
        }
        case SAMPLE_PCIE_MSG_TEST_WRITE: {
            printf("receive msg, u32MsgType: Write\n");
            memcpy(stMsg.u8MsgBody, u8TestMsgBody1, sizeof(u8TestMsgBody1));
            s32EchoMsgLen = sizeof(u8TestMsgBody1);
            stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, s32EchoMsgLen);
            s32Ret = AX_SUCCESS;
            break;
        }
        case SAMPLE_PCIE_MSG_TEST_COPY: {
            printf("receive msg, u32MsgType: copy\n");
            memcpy(u8TestMsgBody, stMsg.u8MsgBody, sizeof(u8TestMsgBody));
            for (i = 0; i < u32MsgLen / sizeof(u8TestMsgBody[0]); i++) {
                printf("TestMsgBoby[%d] = %x\n", i, u8TestMsgBody[i]);
            }
            if (stMsg.stMsgHead.u8CheckSum == CheckSum(u8TestMsgBody, u32MsgLen)) {
                printf("checksum success!\n");
                memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
                memcpy(stMsg.u8MsgBody, u8TestMsgBody1, sizeof(u8TestMsgBody1));
                s32EchoMsgLen = sizeof(u8TestMsgBody1);
                stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, s32EchoMsgLen);
                s32Ret = AX_SUCCESS;
            } else {
                printf("checksum fail!\n");
                s32Ret = AX_FAILURE;
            }
            break;
        }
        case SAMPLE_PCIE_MSG_RING_BUF_TEST: {
#if 0
            for (AX_S32 i = 0; i < u32MsgLen / sizeof(stMsg.u8MsgBody[0]); i += 4) {
                printf("u8MsgBody[%d] = %x%x%x%x\n", i, stMsg.u8MsgBody[i], stMsg.u8MsgBody[i + 1], stMsg.u8MsgBody[i + 2],
                       stMsg.u8MsgBody[i + 3]);
            }
#endif
            if (stMsg.stMsgHead.u8CheckSum == CheckSum(stMsg.u8MsgBody, u32MsgLen)) {
                printf("checksum success!\n");
                memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
                s32Size = Random_MsgBody_Test(&stMsg);
                s32EchoMsgLen = s32Size;
                stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, s32EchoMsgLen);
                s32Ret = AX_SUCCESS;
            } else {
                printf("checksum fail!\n");
                s32Ret = AX_FAILURE;
            }
            break;
        }
        case SAMPLE_PCIE_CREATE_MULTI_THREAD_PORT: {
            printf("receive msg, u32MsgType: create multi thread port\n");
            s32Ret = Sample_PCIe_Create_Multi_Thread_Port(&stMsg);
            break;
        }
        case SAMPLE_PCIE_START_MULTI_THREAD_TEST: {
            printf("receive msg, u32MsgType: start multi thread\n");
            s32Ret = Sample_PCIe_Multi_Thread_Test_Start(&stMsg);
            break;
        }
        case SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG: {
            printf("receive msg, u32MsgType: EXIT stMsg Prog\n");
            s32Ret = AX_SUCCESS;
            break;
        }
        default: {
            printf("slave invalid msg, type:%d \n", stMsg.stMsgHead.u32MsgType);
            s32Ret = AX_FAILURE;
            break;
        }
        }

        stMsg.stMsgHead.u32Target = 0;
        stMsg.stMsgHead.s32RetVal = s32Ret;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_ECHO;
        stMsg.stMsgHead.u32MsgLen = s32EchoMsgLen;
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);

        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(0, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("slave PCIe send msg fail\n");
            return 0;
        }

        if (SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG == u32MsgType) {
            s32Ret = AX_PCIe_CloseMsgPort(0, PCIE_MSGPORT_COMM_CMD);
            if (s32Ret != AX_SUCCESS) {
                printf("slave pcie close msg port fail\n");
                return s32Ret;
            }
            break;
        }
    }
    // free(pstThread);
    return AX_SUCCESS;
}