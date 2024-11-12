/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_CAM_H__
#define __COMMON_CAM_H__

#include "ax_base_type.h"
#include "common_vin.h"
#include "common_isp.h"
#include "common_sys.h"
#include "ax_engine_api.h"
#include <pthread.h>

#define MAX_FILE_NAME_CHAR_SIZE       (128)

#ifndef COMM_CAM_PRT
#define COMM_CAM_PRT(fmt...)   \
do {\
    printf("[COMM_CAM][%s][%5d] ", __FUNCTION__, __LINE__);\
    printf(fmt);\
}while(0)
#endif

typedef struct {
    /* common parameters */
    AX_BOOL                             bOpen;
    AX_SNS_HDR_MODE_E                   eHdrMode;
    SAMPLE_SNS_TYPE_E                   eSnsType;
    COMMON_VIN_MODE_E                   eSysMode;

    AX_U32                              nRxDev;
    AX_U8                               nDevId;
    AX_U8                               nPipeId;

    /* Resource Control Parameters */
    AX_BOOL                             bRegisterSns;
    AX_BOOL                             bEnableDev;     /* loadraw mode, it is not necessary to enable dev */
    AX_BOOL                             bDevOnly;
    AX_BOOL                             bEnableFlash;

    /* Isp processing thread */
    pthread_t                           tIspProcThread[AX_VIN_MAX_PIPE_NUM];
    pthread_t                           tIspAFProcThread;

    /* Module Attribute Parameters */
    AX_MIPI_RX_DEV_T                    tMipiRx;
    AX_SNS_ATTR_T                       tSnsAttr;
    AX_SNS_CLK_ATTR_T                   tSnsClkAttr;
    AX_VIN_DEV_ATTR_T                   tDevAttr;
    AX_VIN_DEV_BIND_PIPE_T              tDevBindPipe;
    AX_FRAME_INTERRUPT_ATTR_T           tDevFrmIntAttr;
    AX_VIN_PIPE_ATTR_T                  tPipeAttr;
    SAMPLE_PIPE_INFO_T                  tPipeInfo[AX_VIN_MAX_PIPE_NUM];
    AX_VIN_CHN_ATTR_T                   tChnAttr[AX_VIN_MAX_CHN_NUM];

    AX_VIN_POWER_SYNC_ATTR_T            tPowerAttr;
    AX_VIN_SYNC_SIGNAL_ATTR_T           tVsyncAttr;
    AX_VIN_SYNC_SIGNAL_ATTR_T           tHsyncAttr;
    AX_VIN_LIGHT_SYNC_INFO_T            tLightSyncInfo;
    AX_VIN_STROBE_LIGHT_TIMING_ATTR_T   tSnapStrobeAttr;
    AX_VIN_FLASH_LIGHT_TIMING_ATTR_T    tSnapFlashAttr;

    /* 3A Parameters */
    AX_BOOL                             bUser3a;
    AX_ISP_AE_REGFUNCS_T                tAeFuncs;
    AX_ISP_AWB_REGFUNCS_T               tAwbFuncs;
    AX_ISP_AF_REGFUNCS_T                tAfFuncs;
    AX_ISP_LSC_REGFUNCS_T               tLscFuncs;
} AX_CAMERA_T;

AX_S32 COMMON_NPU_Init(AX_ENGINE_NPU_MODE_T eHardMode);
AX_S32 COMMON_CAM_Init(AX_VOID);
AX_S32 COMMON_CAM_Deinit(AX_VOID);
AX_S32 COMMON_CAM_PrivPoolInit(COMMON_SYS_ARGS_T *pPrivPoolArgs);

AX_S32 COMMON_CAM_Open(AX_CAMERA_T *pCamList, AX_U8 Num);
AX_S32 COMMON_CAM_Close(AX_CAMERA_T *pCamList, AX_U8 Num);
AX_S32 COMMON_CAM_Run(AX_CAMERA_T *pCam, AX_U8 Num);
AX_S32 COMMON_CAM_Stop(AX_VOID);

AX_S32 COMMON_CAM_CaptureFrameProc(AX_U32 nCapturePipeId, const AX_IMG_INFO_T *pImgInfo[]);
#endif //__COMMON_CAM_H__
