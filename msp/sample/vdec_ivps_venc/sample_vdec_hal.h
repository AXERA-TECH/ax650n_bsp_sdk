/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_VDEC_HAL_H_
#define _SAMPLE_VDEC_HAL_H_


#include "ax_sys_api.h"
#include "ax_base_type.h"
#include "common_vdec_utils.h"
#include "common_vo.h"

#define BUFFER_SIZE  (8 * 1024 * 1024)

#define SIZE_ALIGN(x,align) ((((x)+(align)-1)/(align))*(align))

#ifndef VDEC_ALIGN_UP
#define VDEC_ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#endif

typedef enum {
    SAMPLE_VDEC_POLLING_STATUS_INIT = 1,
    SAMPLE_VDEC_POLLING_STATUS_START,
    SAMPLE_VDEC_POLLING_STATUS_END,
    SAMPLE_VDEC_POLLING_STATUS_SEND,
    SAMPLE_VDEC_POLLING_STATUS_EXIT,
    SAMPLE_VDEC_POLLING_STATUS_ERR,
    AX_VDEC_FIFO_STATUS_BUTT
} SAMPLE_VDEC_POLLING_STATUS_E;

typedef struct axSAMPLE_JDEC_ARGS {
    AX_VDEC_GRP VdecGrp;
    AX_U8 *pu8StreamMem;
    AX_S32 s32StreamLen;
} SAMPLE_JDEC_ARGS_T;

typedef struct axSAMPLE_VDEC_POLLING_ARGS {
    SAMPLE_VDEC_POLLING_STATUS_E pollingStat;
    AX_S32 pollingCnt;
    AX_U32 pollingTime;
    AX_BOOL pollingStart;
    AX_BOOL reSendStream[AX_VDEC_MAX_GRP_NUM];
    pthread_mutex_t pollingMutex[AX_VDEC_MAX_GRP_NUM];
} SAMPLE_VDEC_POLLING_ARGS_T;

typedef struct axSAMPLE_VDEC_FUNC_ARGS {
    AX_VDEC_GRP VdecGrp;
    AX_VIDEO_FRAME_INFO_T *pstUsrPic;
    AX_CHAR *sFile;
    AX_VDEC_GRP_ATTR_T stGrpAttr;
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd;
    SAMPLE_VDEC_USRPIC_ARGS_T tUsrPicArgs;
    SAMPLE_VDEC_POLLING_ARGS_T *pstPollingArgs;
} SAMPLE_VDEC_FUNC_ARGS_T;

typedef struct axSAMPLE_VDEC_RECV_ARGS {
    SAMPLE_VDEC_CMD_PARAM_T *pstCmd;
    SAMPLE_VDEC_POLLING_ARGS_T *pstPollingArgs;
    SAMPLE_VO_CONFIG_S *pstVoConf;
} SAMPLE_VDEC_RECV_ARGS_T;


#define SAMPLE_VDEC_MUTEXT_LOCK(mutexObj) do { \
        int ret = pthread_mutex_lock(mutexObj); \
        if (ret) { \
            SAMPLE_CRIT_LOG(" pthread_mutex_lock failed! %d\n", ret); \
            s32Ret = AX_ERR_VDEC_UNKNOWN; \
            goto ERR_RET; \
        } \
    } while (0)

#define SAMPLE_VDEC_MUTEXT_UNLOCK(mutexObj) do { \
        int ret = pthread_mutex_unlock(mutexObj); \
        if (ret) { \
            SAMPLE_CRIT_LOG("pthread_mutex_unlock failed! %d\n", ret); \
            s32Ret = AX_ERR_VDEC_UNKNOWN; \
            goto ERR_RET; \
        } \
    } while (0)

extern AX_S32 userPicTest;
AX_S32 PoolUserPicDisplay(AX_VIDEO_FRAME_INFO_T *pstUserPic);
AX_S32 PoolUserPicDeinit(AX_VIDEO_FRAME_INFO_T *pstUserPic);
AX_S32 PoolUserPicInit(AX_VIDEO_FRAME_INFO_T *pstUserPic);

void *VdecFrameFunc(void *arg);
void *_VdecRecvThread(void *arg);
AX_S32 VdecPollingExe(AX_VDEC_GRP VdGrp, SAMPLE_VDEC_CMD_PARAM_T *pstCmd);
AX_S32 VdecExitFunc(AX_VDEC_GRP VdGrp);
AX_S32 VdecUserPoolExitFunc(AX_VDEC_GRP VdGrp, SAMPLE_VDEC_CMD_PARAM_T *pstCmd);

#endif /* _SAMPLE_IVPS_HAL_H_ */
