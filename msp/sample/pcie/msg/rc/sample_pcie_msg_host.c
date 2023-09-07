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
#include <sys/prctl.h>

#include "ax_base_type.h"
#include "ax_pcie_msg_api.h"

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

typedef struct AX_PCIE_MULTCARD {
    AX_PCIE_THREAD_T stThread[MAX_THREAD_NUM];
    AX_S32 s32MaxLoop;
    AX_S32 s32TargetId;
} AX_PCIE_MULTCARD_T;

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

    s32Ret = read(s32Fd, pstPciMsg->u8MsgBody, s32Size);
    if (s32Ret < 0) {
        printf("read /dev/urandom data failed\n");
        close(s32Fd);
        return AX_FAILURE;
    }

    close(s32Fd);
    return s32Size;
}

AX_S32 Sample_PCIe_Msg_Write()
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_U8 u8TestMsgBody[4] = {0xab, 0xbc, 0xcd, 0xde};
    AX_PCIE_MSG_T  stMsg;
    AX_S32 i, s32RmtChipId;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);
    AX_S32 s32PciLocalId, s32PciRmtId[PCIE_MAX_CHIPNUM], s32AllPciRmtCnt;

    printf("##############Sample_PCIe_Msg_Write################\n");

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    /* Get pcie tartget id*/
    s32Ret = AX_PCIe_GetTargetId(s32PciRmtId, &s32AllPciRmtCnt);
    if (s32Ret < 0) {
        printf("host get pcie target id fail\n");
        return s32Ret;
    }

    /* Sharemem init */
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        s32Ret = AX_PCIe_ShareMemInit(s32PciRmtId[i]);
        if (s32Ret < 0) {
            printf("host share mem init fail\n");
            return s32Ret;
        }
    }

    for (i = 0; i < s32AllPciRmtCnt; i++) {

        s32RmtChipId = s32PciRmtId[i];
        printf("s32RmtChipId = %d\n", s32RmtChipId);

        /* PCIe open msg port */
        s32Ret = AX_PCIe_OpenMsgPort(s32RmtChipId, PCIE_MSGPORT_COMM_CMD);
        if (s32Ret < 0) {
            printf("host open msg port fail\n");
            return s32Ret;
        }

        memcpy(stMsg.u8MsgBody, u8TestMsgBody, sizeof(u8TestMsgBody));
        stMsg.stMsgHead.u32Target = s32RmtChipId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_READ;
        stMsg.stMsgHead.u32MsgLen = sizeof(u8TestMsgBody);
        stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen);
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            return s32Ret;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    break;
                }
            }
        }
    }

    printf("\n#################Sample_PCIe_Msg_Write Exit########################\n");
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        s32RmtChipId = s32PciRmtId[i];
        memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
        stMsg.stMsgHead.u32Target = s32RmtChipId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG;
        stMsg.stMsgHead.u32MsgLen = 0;
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            return s32Ret;
        }
        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    break;
                }
            }
        }
        s32Ret = AX_PCIe_CloseMsgPort(s32RmtChipId, PCIE_MSGPORT_COMM_CMD);
        if (s32Ret != AX_SUCCESS) {
            printf("host pcie close msg port fail\n");
            return s32Ret;
        }
    }
    return s32Ret;
}

AX_S32 Sample_PCIe_Msg_Read()
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_U8 u8TestMsgBody[4] = {0};
    AX_PCIE_MSG_T  stMsg;
    AX_S32 i, j, s32RmtChipId;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);
    AX_S32 s32PciLocalId, s32PciRmtId[PCIE_MAX_CHIPNUM], s32AllPciRmtCnt;

    printf("################Sample_PCIe_Msg_Read################\n");

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    /* Get pcie tartget id*/
    s32Ret = AX_PCIe_GetTargetId(s32PciRmtId, &s32AllPciRmtCnt);
    if (s32Ret < 0) {
        printf("host get pcie target id fail\n");
        return s32Ret;
    }

    /* Sharemem init */
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        s32Ret = AX_PCIe_ShareMemInit(s32PciRmtId[i]);
        if (s32Ret < 0) {
            printf("host share mem init fail\n");
            return s32Ret;
        }
    }

    for (i = 0; i < s32AllPciRmtCnt; i++) {

        s32RmtChipId = s32PciRmtId[i];
        printf("s32RmtChipId = %d AllPciRmtCnt = %d\n", s32RmtChipId, s32AllPciRmtCnt);

        /* PCIe open msg port */
        s32Ret = AX_PCIe_OpenMsgPort(s32RmtChipId, PCIE_MSGPORT_COMM_CMD);
        if (s32Ret < 0) {
            printf("host open msg port fail\n");
            return s32Ret;
        }

        stMsg.stMsgHead.u32Target = s32RmtChipId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_WRITE;
        stMsg.stMsgHead.u32MsgLen = 0;
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            return s32Ret;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                stMsg.stMsgHead.u32MsgLen = (s32Ret - sizeof(AX_PCIE_MSGHEAD_T));
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    memcpy(u8TestMsgBody, stMsg.u8MsgBody, sizeof(u8TestMsgBody));
                    for (j = 0; j < sizeof(u8TestMsgBody) / sizeof(u8TestMsgBody[0]); j++) {
                        printf("TestMsgBoby[%d] = %x\n", j, u8TestMsgBody[j]);
                    }
                    if (stMsg.stMsgHead.u8CheckSum == CheckSum(u8TestMsgBody, stMsg.stMsgHead.u32MsgLen)) {
                        printf("Checksum success!\n");
                    } else {
                        printf("Checksum failed!\n");
                    }
                    break;
                }
            }
        }
    }

    printf("\n#################Sample_PCIe_Msg_Read Exit########################\n");

    for (i = 0; i < s32AllPciRmtCnt; i++) {
        s32RmtChipId = s32PciRmtId[i];
        memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
        stMsg.stMsgHead.u32Target = s32RmtChipId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG;
        stMsg.stMsgHead.u32MsgLen = 0;
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            return s32Ret;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    break;
                }
            }
        }

        s32Ret = AX_PCIe_CloseMsgPort(s32RmtChipId, PCIE_MSGPORT_COMM_CMD);
        if (s32Ret != AX_SUCCESS) {
            printf("host pcie close msg port fail\n");
            return s32Ret;
        }
    }
    return s32Ret;
}

AX_S32 Sample_PCIe_Msg_copy()
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_U8 u8TestMsgBody[4] = {0xab, 0xbc, 0xcd, 0xde};
    AX_U8 u8TestMsgBody1[4] = {0};
    AX_PCIE_MSG_T  stMsg;
    AX_S32 i, j, s32RmtChipId;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);
    AX_S32 s32PciLocalId, s32PciRmtId[PCIE_MAX_CHIPNUM], s32AllPciRmtCnt;

    printf("#################Sample_PCIe_Msg_copy##################\n");

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    /* Get pcie tartget id*/
    s32Ret = AX_PCIe_GetTargetId(s32PciRmtId, &s32AllPciRmtCnt);
    if (s32Ret < 0) {
        printf("host get pcie target id fail\n");
        return s32Ret;
    }

    /* Sharemem init */
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        s32Ret = AX_PCIe_ShareMemInit(s32PciRmtId[i]);
        if (s32Ret < 0) {
            printf("host share mem init fail\n");
            return s32Ret;
        }
    }

    for (i = 0; i < s32AllPciRmtCnt; i++) {

        s32RmtChipId = s32PciRmtId[i];
        printf("s32RmtChipId = %d\n", s32RmtChipId);

        /* PCIe open msg port */
        s32Ret = AX_PCIe_OpenMsgPort(s32RmtChipId, PCIE_MSGPORT_COMM_CMD);
        if (s32Ret < 0) {
            printf("host open msg port fail\n");
            return s32Ret;
        }

        memcpy(stMsg.u8MsgBody, u8TestMsgBody, sizeof(u8TestMsgBody));
        stMsg.stMsgHead.u32Target = s32RmtChipId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_COPY;
        stMsg.stMsgHead.u32MsgLen = sizeof(u8TestMsgBody);
        stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen);
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            return s32Ret;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                stMsg.stMsgHead.u32MsgLen = s32Ret - sizeof(AX_PCIE_MSGHEAD_T);
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    memcpy(u8TestMsgBody1, stMsg.u8MsgBody, sizeof(u8TestMsgBody1));
                    for (j = 0; j < sizeof(u8TestMsgBody1) / sizeof(u8TestMsgBody1[0]); j++) {
                        printf("TestMsgBoby[%d] = %x\n", j, u8TestMsgBody1[j]);
                    }
                    if (stMsg.stMsgHead.u8CheckSum == CheckSum(u8TestMsgBody1, stMsg.stMsgHead.u32MsgLen)) {
                        printf("Checksum success!\n");
                    } else {
                        printf("Checksum failed!\n");
                    }
                    break;
                }
            }
        }
    }

    printf("\n#################Sample_PCIe_Msg_copy Exit########################\n");

    for (i = 0; i < s32AllPciRmtCnt; i++) {
        s32RmtChipId = s32PciRmtId[i];
        memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
        stMsg.stMsgHead.u32Target = s32RmtChipId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG;
        stMsg.stMsgHead.u32MsgLen = 0;
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            return s32Ret;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32RmtChipId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    break;
                }
            }
        }

        s32Ret = AX_PCIe_CloseMsgPort(s32RmtChipId, PCIE_MSGPORT_COMM_CMD);
        if (s32Ret != AX_SUCCESS) {
            printf("host pcie close msg port fail\n");
            return s32Ret;
        }
    }

    return 0;
}

AX_VOID *RingBuffer_Test(void *pArg)
{
    AX_S32 s32Ret;
    AX_S32 s32Loop;
    AX_CHAR name[20];
    AX_PCIE_MULTCARD_T *pstMultCard = (AX_PCIE_MULTCARD_T *)pArg;
    AX_S32 s32TargetId = pstMultCard->s32TargetId;
    AX_S32 s32MaxLoop = pstMultCard->s32MaxLoop;
    AX_S32 s32Size;
    AX_PCIE_MSG_T  stMsg;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);

    printf("******TargetId = %d\n", s32TargetId);

    sprintf(name, "Target%d", s32TargetId);
    prctl(PR_SET_NAME, name);

    s32Ret = AX_PCIe_ShareMemInit(s32TargetId);
    if (s32Ret < 0) {
        printf("host share mem init fail\n");
        exit(1);
    }

    /* PCIe open msg port */
    s32Ret = AX_PCIe_OpenMsgPort(s32TargetId, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret < 0) {
        printf("host open msg port fail\n");
        exit(1);
    }

    for (s32Loop = 0; s32Loop < s32MaxLoop; s32Loop++) {
        memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
        s32Size = Random_MsgBody_Test(&stMsg);
        stMsg.stMsgHead.u32Target = s32TargetId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_RING_BUF_TEST;
        stMsg.stMsgHead.u32MsgLen = s32Size;
        stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen);
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            goto out;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                stMsg.stMsgHead.u32MsgLen = s32Ret - sizeof(AX_PCIE_MSGHEAD_T);
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
#if 0
                    for (i = 0; i < stMsg.stMsgHead.u32MsgLen / sizeof(stMsg.u8MsgBody[0]); i += 4) {
                        printf("u8MsgBody[%d] = %x%x%x%x\n", i, stMsg.u8MsgBody[i], stMsg.u8MsgBody[i + 1], stMsg.u8MsgBody[i + 2],
                               stMsg.u8MsgBody[i + 3]);
                    }
#endif
                    if (stMsg.stMsgHead.u8CheckSum == CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen)) {
                        printf("checksum success!\n");
                    } else {
                        printf("checksum failed!\n");
                    }
                    break;
                }
            }
        }
    }

printf("\n#################Sample_PCIe_Ring_Buffer_Test Exit########################\n");

    memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
    stMsg.stMsgHead.u32Target = s32TargetId;
    stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG;
    stMsg.stMsgHead.u32MsgLen = 0;
    s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
    /* PCIe write msg */
    s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
    if (s32Ret < 0) {
        printf("host pcie send msg fail\n");
        goto out;
    }
    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                printf("host write success\n");
                break;
            }
        }
    }

out:
    s32Ret = AX_PCIe_CloseMsgPort(s32TargetId, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret != AX_SUCCESS) {
        printf("host pcie close msg port fail\n");
        exit(1);
    }

    pthread_exit(NULL);
}


AX_S32 Sample_PCIe_Ring_Buffer_Test(AX_S32 s32MaxLoop)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 i;
    pthread_t mult_card[32];
    AX_PCIE_MULTCARD_T stMultCard[32];
    AX_S32 s32PciLocalId, s32PciRmtId[PCIE_MAX_CHIPNUM], s32AllPciRmtCnt;

    printf("#################Sample_PCIe_Ring_Buffer_Test##################\n");

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    /* Get pcie tartget id*/
    s32Ret = AX_PCIe_GetTargetId(s32PciRmtId, &s32AllPciRmtCnt);
    if (s32Ret < 0) {
        printf("host get pcie target id fail\n");
        return s32Ret;
    }

    for (i = 0; i < s32AllPciRmtCnt; i++) {
        stMultCard[i].s32MaxLoop = s32MaxLoop;
        stMultCard[i].s32TargetId = s32PciRmtId[i];
        pthread_create(&mult_card[i], NULL, RingBuffer_Test, &stMultCard[i]);
    }
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        pthread_join(mult_card[i], NULL);
    }

    return s32Ret;
}

AX_VOID *MultThread_Test(void *pArg)
{
    AX_S32 s32Ret = 0;
    AX_S32 s32Loop;
    AX_S32 s32Size;
    AX_PCIE_THREAD_T *pstThread = (AX_PCIE_THREAD_T *)pArg;
    AX_S32 s32MaxLoop = pstThread->s32MaxLoop;
    AX_S32 s32TargetId = pstThread->s32TargetId;
    AX_S32 s32Id = pstThread->s32Id;
    AX_PCIE_MSG_T  stMsg;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);

    printf("Sample host pcie thread test Target = %d\n", s32TargetId);

    for (s32Loop = 0; s32Loop < s32MaxLoop; s32Loop++) {
        printf("Target[%d]-th[%d]-loop[%d]\n", s32TargetId, s32Id, s32Loop);
        s32Size = Random_MsgBody_Test(&stMsg);
        stMsg.stMsgHead.u32Target = s32TargetId;
        stMsg.stMsgHead.u32MsgLen = s32Size;
        stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen);
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        s32Ret = AX_PCIe_WriteMsg(s32TargetId, pstThread->u8ThreadMsgPort, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            continue;
        }
        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32TargetId, pstThread->u8ThreadMsgPort, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(10000);
                continue;
            } else {
                stMsg.stMsgHead.u32MsgLen = s32Ret - sizeof(AX_PCIE_MSGHEAD_T);
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
#if 0
                    for (i = 0; i < stMsg.stMsgHead.u32MsgLen / sizeof(stMsg.u8MsgBody[0]); i += 4) {
                        printf("u8MsgBody[%d] = %x%x%x%x\n", i, stMsg.u8MsgBody[i], stMsg.u8MsgBody[i + 1], stMsg.u8MsgBody[i + 2],
                               stMsg.u8MsgBody[i + 3]);
                    }
#endif
                    if (stMsg.stMsgHead.u8CheckSum == CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen)) {
                        printf("checksum success!\n");
                        printf("Target[%d]-th[%d]-loop[%d] test success!\n", s32TargetId, s32Id, s32Loop);
                    } else {
                        printf("checksum failed!\n");
                        printf("Target[%d]-th[%d]-loop[%d] test failed!\n", s32TargetId,s32Id, s32Loop);
                    }
                    break;
                }
            }
        }
    }

    printf("--------------Target[%d] sample PCIe multi thead[%d] Test exit-------------\n", s32TargetId, s32Id);
    stMsg.stMsgHead.u32Target = s32TargetId;
    stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_EXIT_MULTI_THREAD_TEST;
    stMsg.stMsgHead.u32MsgLen = sizeof(pstThread->u8ThreadMsgPort);
    s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
    s32Ret = AX_PCIe_WriteMsg(s32TargetId, pstThread->u8ThreadMsgPort, &stMsg, s32Len);
    if (s32Ret < 0) {
        printf("host pcie send msg fail\n");
        exit(1);
    }
    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, pstThread->u8ThreadMsgPort, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                printf("Slave %d multi thread exit success!\n", s32TargetId);
                break;
            }
        }
    }

    printf("Target[%d] sample host pcie thread[%d] test Exit\n", s32TargetId, s32Id);
    pthread_exit(NULL);
}

AX_VOID *MultCard_MultThread_Test(void *pArg)
{
    AX_S32 s32Ret;
    AX_CHAR name[20];
    AX_PCIE_MULTCARD_T *pstMultCard = (AX_PCIE_MULTCARD_T *)pArg;
    AX_S32 s32TargetId = pstMultCard->s32TargetId;
    AX_S32 s32MaxLoop = pstMultCard->s32MaxLoop;
    AX_S32 s32MsgPort = PCIE_MSGPORT_COMM_CMD + 1;
    AX_S32 i, th;
    AX_PCIE_MSG_T  stMsg;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);
    pthread_t mult_th[MAX_THREAD_NUM];

    printf("******TargetId = %d\n", s32TargetId);

    sprintf(name, "Target%d", s32TargetId);
    prctl(PR_SET_NAME, name);

    s32Ret = AX_PCIe_ShareMemInit(s32TargetId);
    if (s32Ret < 0) {
        printf("host share mem init fail\n");
        exit(1);
    }

    /* PCIe open msg port */
    s32Ret = AX_PCIe_OpenMsgPort(s32TargetId, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret < 0) {
        printf("host open msg port fail\n");
        exit(1);
    }

    for (th = 0; th < MAX_THREAD_NUM; th++) {
        /* PCIe open msg port */
        pstMultCard->stThread[th].u8ThreadMsgPort = s32MsgPort++;
        s32Ret = AX_PCIe_OpenMsgPort(s32TargetId, pstMultCard->stThread[th].u8ThreadMsgPort);
        if (s32Ret < 0) {
            printf("host open th %d msg port fail\n", th);
            goto cmd_err;
        }
    }

    //create multi thread port
    stMsg.stMsgHead.u32Target = s32TargetId;
    stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_CREATE_MULTI_THREAD_PORT;
    stMsg.stMsgHead.u32MsgLen = sizeof(pstMultCard->stThread);
    s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
    memcpy(stMsg.u8MsgBody, pstMultCard->stThread, sizeof(pstMultCard->stThread));
    s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
    if (s32Ret < 0) {
        printf("host pcie send msg fail\n");
        goto out;
    }

    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                printf("Multi thread port create success!\n");
                break;
            }
        }
    }

    //start mulit thread
    stMsg.stMsgHead.u32Target = s32TargetId;
    stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_START_MULTI_THREAD_TEST;
    stMsg.stMsgHead.u32MsgLen = 0;
    s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
    s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
    if (s32Ret < 0) {
        printf("host pcie send msg fail\n");
        goto out;
    }

    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                printf("Start slave %d multi thread success!\n", s32TargetId);
                break;
            }
        }
    }

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pstMultCard->stThread[i].s32MaxLoop = s32MaxLoop;
        pstMultCard->stThread[i].s32TargetId = s32TargetId;
        pstMultCard->stThread[i].s32Id = i;
        pthread_create(&mult_th[i], NULL, MultThread_Test, &pstMultCard->stThread[i]);
    }
    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_join(mult_th[i], NULL);
    }

    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, 10000);
        if (s32Ret < AX_SUCCESS) {
            printf("========read fail target = %d\n", s32TargetId);
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                printf("slave %d multi thread test complete!\n", s32TargetId);
                break;
            }
        }
    }

    printf("\n#################Sample_PCIe_Thread_Test Exit %d########################\n", s32TargetId);

    memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
    stMsg.stMsgHead.u32Target = s32TargetId;
    stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG;
    stMsg.stMsgHead.u32MsgLen = 0;
    s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
    /* PCIe write msg */
    s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
    if (s32Ret < 0) {
        printf("host pcie send msg fail\n");
        goto out;
    }
    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                printf("host write success\n");
                break;
            }
        }
    }

out:
    for (th = 0; th < MAX_THREAD_NUM; th++) {
        s32Ret = AX_PCIe_CloseMsgPort(s32TargetId, pstMultCard->stThread[th].u8ThreadMsgPort);
        if (s32Ret != AX_SUCCESS) {
            printf("host pcie close msg port fail\n");
        }
    }
cmd_err:
    s32Ret = AX_PCIe_CloseMsgPort(s32TargetId, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret != AX_SUCCESS) {
        printf("host pcie close msg port fail\n");
    }

    pthread_exit(NULL);
}

AX_S32 Sample_PCIe_Thread_Test(AX_S32 s32MaxLoop)
{
    AX_S32 s32Ret;
    AX_S32 i;
    AX_S32 s32PciLocalId, s32PciRmtId[PCIE_MAX_CHIPNUM], s32AllPciRmtCnt;
    AX_PCIE_MULTCARD_T stMultCard[32];
    pthread_t mult_card[32];

    printf("#################Sample_PCIe_Thread_Test##################\n");

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    /* Get pcie tartget id*/
    s32Ret = AX_PCIe_GetTargetId(s32PciRmtId, &s32AllPciRmtCnt);
    if (s32Ret < 0) {
        printf("host get pcie target id fail\n");
        return s32Ret;
    }

    for (i = 0; i < s32AllPciRmtCnt; i++) {
        stMultCard[i].s32MaxLoop = s32MaxLoop;
        stMultCard[i].s32TargetId = s32PciRmtId[i];
        pthread_create(&mult_card[i], NULL, MultCard_MultThread_Test, &stMultCard[i]);
    }
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        pthread_join(mult_card[i], NULL);
    }

    return s32Ret;
}

int Reset_Boot_Handler(AX_S32 s32TargetId)
{
    AX_S32 s32Ret;
    char cmd[20];

    printf("===============Target %d reset boot===========\n", s32TargetId);

    sprintf(cmd, "./opt/bin/axdl %x", s32TargetId);
    system(cmd);
    sleep(5);
    s32Ret = AX_PCIe_ShareMemInit(s32TargetId);
    if (s32Ret < 0) {
        printf("host share mem init fail\n");
    }
    return s32Ret;
}

AX_VOID *Reset_Test(void *arg)
{
    AX_S32 s32Ret;
    AX_S32 s32Loop;
    AX_CHAR name[20];
    AX_S32 s32TargetId = *(AX_S32 *)(arg);
    AX_U8 u8TestMsgBody[4] = {0xab, 0xbc, 0xcd, 0xde};
    AX_U8 u8TestMsgBody1[4] = {0};
    AX_PCIE_MSG_T  stMsg;
    AX_S32 i, Lp;
    AX_S32 s32Len;
    AX_S32 s32RdLen = sizeof(AX_PCIE_MSG_T);

    printf("******TargetId = %d\n", s32TargetId);

    sprintf(name, "Target%d", s32TargetId);
    prctl(PR_SET_NAME, name);

    s32Ret = AX_PCIe_ShareMemInit(s32TargetId);
    if (s32Ret < 0) {
        printf("host share mem init fail\n");
        exit(1);
    }

    /* PCIe open msg port */
    s32Ret = AX_PCIe_OpenMsgPort(s32TargetId, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret < 0) {
        printf("host open msg port fail\n");
        exit(1);
    }

    s32Loop = (rand() % 260 + 20);
    printf("target=[%d],reset_loop = %d\n", s32TargetId, s32Loop);

    for (Lp = 0; Lp < 300; Lp++) {
        printf("target=[%d] loop[%d]\n", s32TargetId, Lp);
        if (Lp == s32Loop) {
            s32Ret = Reset_Boot_Handler(s32TargetId);
            if (s32Ret < 0)
                goto out;
        }

        memcpy(stMsg.u8MsgBody, u8TestMsgBody, sizeof(u8TestMsgBody));
        stMsg.stMsgHead.u32Target = s32TargetId;
        stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_COPY;
        stMsg.stMsgHead.u32MsgLen = sizeof(u8TestMsgBody);
        stMsg.stMsgHead.u8CheckSum = CheckSum(stMsg.u8MsgBody, stMsg.stMsgHead.u32MsgLen);
        s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
        /* PCIe write msg */
        s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
        if (s32Ret < 0) {
            printf("host pcie send msg fail\n");
            s32Ret = Reset_Boot_Handler(s32TargetId);
            if (s32Ret < 0)
                goto out;
        }

        while (1) {
            s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
            if (s32Ret < AX_SUCCESS) {
                usleep(1000);
                continue;
            } else {
                stMsg.stMsgHead.u32MsgLen = s32Ret - sizeof(AX_PCIE_MSGHEAD_T);
                if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                    memcpy(u8TestMsgBody1, stMsg.u8MsgBody, sizeof(u8TestMsgBody1));
                    for (i = 0; i < sizeof(u8TestMsgBody1) / sizeof(u8TestMsgBody1[0]); i++) {
                        printf("TestMsgBoby[%d] = %x\n", i, u8TestMsgBody1[i]);
                    }
                    if (stMsg.stMsgHead.u8CheckSum == CheckSum(u8TestMsgBody1, stMsg.stMsgHead.u32MsgLen)) {
                        printf("Checksum success!\n");
                    } else {
                        s32Ret = Reset_Boot_Handler(s32TargetId);
                        if (s32Ret < 0)
                            goto out;
                    }
                    break;
                }
            }
        }
        sleep(1);
    }

    printf("\n#################Sample_PCIe_Reset_Test Exit########################\n");

    memset(&stMsg, 0, sizeof(AX_PCIE_MSG_T));
    stMsg.stMsgHead.u32Target = s32TargetId;
    stMsg.stMsgHead.u32MsgType = SAMPLE_PCIE_MSG_TEST_EXIT_MSG_PROG;
    stMsg.stMsgHead.u32MsgLen = 0;
    s32Len = stMsg.stMsgHead.u32MsgLen + sizeof(AX_PCIE_MSGHEAD_T);
    /* PCIe write msg */
    s32Ret = AX_PCIe_WriteMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32Len);
    if (s32Ret < 0) {
        printf("host pcie send msg fail\n");
        s32Ret = Reset_Boot_Handler(s32TargetId);
        if (s32Ret < 0)
            goto out;
    }

    while (1) {
        s32Ret = AX_PCIe_ReadMsg(s32TargetId, PCIE_MSGPORT_COMM_CMD, &stMsg, s32RdLen, -1);
        if (s32Ret < AX_SUCCESS) {
            usleep(1000);
            continue;
        } else {
            if (stMsg.stMsgHead.u32MsgType == SAMPLE_PCIE_MSG_TEST_ECHO && stMsg.stMsgHead.s32RetVal == AX_SUCCESS) {
                break;
            }
        }
    }

out:
    s32Ret = AX_PCIe_CloseMsgPort(s32TargetId, PCIE_MSGPORT_COMM_CMD);
    if (s32Ret != AX_SUCCESS) {
        printf("host pcie close msg port fail\n");
        exit(1);
    }

    pthread_exit(NULL);
}

AX_S32 Sample_PCIe_Reset_Test()
{
    AX_S32 s32Ret = 0;
    AX_S32 i;
    pthread_t mult_card[32];
    AX_S32 s32PciLocalId, s32PciRmtId[PCIE_MAX_CHIPNUM], s32AllPciRmtCnt;

    printf("#################Sample_PCIe_Reset_Test##################\n");

    /* Get pcie local id */
    s32Ret = AX_PCIe_GetLocalId(&s32PciLocalId);
    if (s32Ret < 0) {
        printf("host get pcie local id fail\n");
        return s32Ret;
    }

    /* Get pcie tartget id*/
    s32Ret = AX_PCIe_GetTargetId(s32PciRmtId, &s32AllPciRmtCnt);
    if (s32Ret < 0) {
        printf("host get pcie target id fail\n");
        return s32Ret;
    }

    for (i = 0; i < s32AllPciRmtCnt; i++) {
        pthread_create(&mult_card[i], NULL, Reset_Test, &s32PciRmtId[i]);
    }
    for (i = 0; i < s32AllPciRmtCnt; i++) {
        pthread_join(mult_card[i], NULL);
    }
    return s32Ret;
}

AX_S32 main(AX_S32 argc, AX_CHAR *argv[])
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 s32MaxLoop = 0;
    AX_S8 ch;
    AX_BOOL bQuit = AX_FALSE;

    printf("axera pcie host msg transfer demo\n\n");

    if (argc > 2) {
        printf("input parameter err: More than two parameters\n\n");
        return 0;
    } else {
        if (argv[1] != NULL) {
            s32MaxLoop = strtol(argv[1], 0, 10);
        }
        printf("s32MaxLoop = %d\n", s32MaxLoop);
    }

    printf("0: Test write: host -> slave.\n");
    printf("1: Test read : host <- slave.\n");
    printf("2: Test copy : host -> slave -> host.\n");
    printf("3: Pressure test: test ring buffer.\n");
    printf("4: Multi-thread test.\n");
    printf("5: Multi-Card pcie reset test.\n");

    printf("\nplease enter one key to test: ");

    while (1) {

        ch = getchar();
        getchar();
        switch (ch) {
        case '0':
            s32Ret = Sample_PCIe_Msg_Write();
            bQuit = AX_TRUE;
            break;
        case '1':
            s32Ret = Sample_PCIe_Msg_Read();
            bQuit = AX_TRUE;
            break;
        case '2':
            s32Ret = Sample_PCIe_Msg_copy();
            bQuit = AX_TRUE;
            break;
        case '3':
            s32Ret = Sample_PCIe_Ring_Buffer_Test(s32MaxLoop);
            bQuit = AX_TRUE;
            break;
        case '4':
            s32Ret = Sample_PCIe_Thread_Test(s32MaxLoop);
            bQuit = AX_TRUE;
            break;
        case '5':
            s32Ret = Sample_PCIe_Reset_Test();
            bQuit = AX_TRUE;
            break;
        default :
            printf("input invaild! please try again.\n");
            printf("\nplease enter one key to test: ");
            break;
        }

        if (bQuit) {
            break;
        }
    }
    if (s32Ret < 0) {
        printf("Sample test failed!\n");
    } else {
        printf("Sample test success!\n");
    }
    return 0;
}