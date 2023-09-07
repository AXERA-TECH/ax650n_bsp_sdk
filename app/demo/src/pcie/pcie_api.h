#ifndef PCIE_API_H__
#define PCIE_API_H__

#include <string>

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif


typedef enum _PCIE_CMD_TYPE_E {
    PCIE_CMD_HAND_SHAKE = 0,     /* Hand Shake */
    PCIE_CMD_SWITCH_AI_E,        /* AI算法切换 */
    PCIE_CMD_AI_RESULT_E,        /* 返回AI算法结果 */
    PCIE_CMD_H264_DATA_E,        /* H264数据 */
    PCIE_CMD_H265_DATA_E         /* H265数据 */
} PCIE_CMD_TYPE_E;

typedef struct _PCIE_CMD_MSG_HEAD_T
{
    PCIE_CMD_TYPE_E nCmdType;  /* 命令类型 */
    AX_U32 nDataLen;           /* 消息体大小 */
    AX_U32 nChannel;           /* channel id */
    AX_U32 nSn;                /* 同一个channel数据的序列号 */
    AX_U16 nCheckSum;          /* check body data */
} PCIE_CMD_MSG_HEAD_T;

typedef struct _PCIE_CMD_MSG_T
{
    PCIE_CMD_MSG_HEAD_T stMsgHead;    /* 消息头 */
    AX_U8 nMsgBody[1024*800];         /* 消息体 */
} PCIE_CMD_MSG_T;

typedef enum _PCIE_ERROR_TYPE_E {
    PCIE_SUCCESS           = 0,   /* 成功 */
    PCIE_ERROR             = -1,  /* 失败 */
    PCIE_ERROR_TIMEOUT     = -2,  /* 超时 */
    PCIE_ERROR_CHECKSUM    = -3,  /* 数据校验失败 */
    PCIE_ERROR_REPEAT_DATA = -4,  /* 重复数据 */
} PCIE_ERROR_TYPE_E;

AX_S32 PCIe_Init(AX_BOOL bMaster, AX_U16 nTargetSlaveCnt, AX_U16 nChannelNum, AX_U32 nDmaBufferSize, AX_S16 nTraceData = 0, AX_S16 nRetryCount = 1);

/*
参数nTimeout: 大于0: 超时时长, 单位: 毫秒; -1: 阻塞等待，有事件之前，永远等待;  0: 不阻塞，立即返回
返回值: > 0(成功), -1(失败), -2(超时), -3(对端数据校验失败)
*/
AX_S32 PCIe_Send(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel,
                 AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeout = -1);

/*
参数nTimeout: 大于0: 超时时长, 单位: 毫秒; -1: 阻塞等待，有事件之前，永远等待;  0: 不阻塞，立即返回
返回值: > 0(成功), -1(失败), -2(超时), -3(接收到的数据校验失败)
*/
AX_S32 PCIe_Recv(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel,
                 AX_U8* pDataBuf, AX_U32 nSize, AX_S32 nTimeout = -1);

AX_S32 PCIe_DeInit();


#ifdef __cplusplus
}
#endif

#endif