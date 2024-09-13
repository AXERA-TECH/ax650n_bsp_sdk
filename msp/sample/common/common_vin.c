/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>

#include "ax_sensor_struct.h"
#include "ax_buffer_tool.h"
// #include "common_sys.h"
#include "common_vin.h"
#include "common_type.h"
#include "common_config.h"

AX_S32 COMMON_VIN_GetSnsConfig(SAMPLE_SNS_TYPE_E eSnsType,
                               AX_MIPI_RX_DEV_T *ptMipiRx, AX_SNS_ATTR_T *ptSnsAttr,
                               AX_SNS_CLK_ATTR_T *ptSnsClkAttr, AX_VIN_DEV_ATTR_T *pDevAttr,
                               AX_VIN_PIPE_ATTR_T *pPipeAttr, AX_VIN_CHN_ATTR_T *pChnAttr)
{
    switch (eSnsType) {
    case SAMPLE_SNS_DUMMY:
        memcpy(ptMipiRx, &gDummyMipiRx, sizeof(AX_MIPI_RX_DEV_T));
        memcpy(ptSnsAttr, &gDummySnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(pDevAttr, &gDummyDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gDummyPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gDummyChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[1], &gDummyChn1Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[2], &gDummyChn2Attr, sizeof(AX_VIN_CHN_ATTR_T));
        break;
    case OMNIVISION_OS08A20:
        memcpy(ptMipiRx, &gOs08a20MipiRx, sizeof(AX_MIPI_RX_DEV_T));
        memcpy(ptSnsAttr, &gOs08a20SnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gOs08a20SnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gOs08a20DevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gOs08a20PipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gOs08a20Chn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[1], &gOs08a20Chn1Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[2], &gOs08a20Chn2Attr, sizeof(AX_VIN_CHN_ATTR_T));
        break;
    case SMARTSENS_SC910GS:
        memcpy(ptMipiRx, &gSc910gsMipiRx, sizeof(AX_MIPI_RX_DEV_T));
        memcpy(ptSnsAttr, &gSc910gsSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSc910gsSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gSc910gsDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gSc910gsPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gSc910gsChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[1], &gSc910gsChn1Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[2], &gSc910gsChn2Attr, sizeof(AX_VIN_CHN_ATTR_T));
        break;
    default:
        memcpy(ptMipiRx, &gOs08a20MipiRx, sizeof(AX_MIPI_RX_DEV_T));
        memcpy(ptSnsAttr, &gOs08a20SnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gOs08a20SnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gOs08a20DevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gOs08a20PipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gOs08a20Chn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[1], &gOs08a20Chn1Attr, sizeof(AX_VIN_CHN_ATTR_T));
        memcpy(&pChnAttr[2], &gOs08a20Chn2Attr, sizeof(AX_VIN_CHN_ATTR_T));
        break;
    }

    return 0;
}

AX_S32 COMMON_VIN_StartMipi(AX_U8 nRxDev, AX_MIPI_RX_DEV_T *ptMipiRx)
{
    AX_S32 nRet = 0;
    AX_MIPI_RX_DEV_T  tMipiRx = {0};

    memcpy(&tMipiRx, ptMipiRx, sizeof(AX_MIPI_RX_DEV_T));
    AX_MIPI_RX_SetLaneCombo(AX_LANE_COMBO_MODE_4);


    nRet = AX_MIPI_RX_SetAttr(nRxDev, &tMipiRx);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_MIPI_RX_SetAttr failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_MIPI_RX_Reset(nRxDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_MIPI_RX_Reset, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_MIPI_RX_Start(nRxDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_MIPI_RX_Start failed, ret=0x%x.\n", nRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_VIN_StopMipi(AX_U8 nRxDev)
{
    AX_S32 axRet;

    axRet = AX_MIPI_RX_Stop(nRxDev);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_MIPI_RX_Stop failed, ret=0x%x.\n", axRet);
    }

    return 0;
}

AX_S32 COMMON_VIN_GetOutsideConfig(SAMPLE_SNS_TYPE_E eSnsType, AX_VIN_POWER_SYNC_ATTR_T *ptPowerAttr,
                              AX_VIN_SYNC_SIGNAL_ATTR_T *ptVsyncAttr, AX_VIN_SYNC_SIGNAL_ATTR_T *ptHsyncAttr,
                              AX_VIN_LIGHT_SYNC_INFO_T *ptLightSyncInfo, AX_VIN_STROBE_LIGHT_TIMING_ATTR_T *ptSnapStrobeAttr,
                              AX_VIN_FLASH_LIGHT_TIMING_ATTR_T *ptSnapFlashAttr)
{
    switch (eSnsType) {
    case SAMPLE_SNS_DUMMY:
        break;
    case OMNIVISION_OS08A20:
        memcpy(ptPowerAttr, &gSc910gsPowerAttr, sizeof(AX_VIN_POWER_SYNC_ATTR_T));
        memcpy(ptVsyncAttr, &gSc910gsVsyncAttr, sizeof(AX_VIN_SYNC_SIGNAL_ATTR_T));
        memcpy(ptHsyncAttr, &gSc910gsHsyncAttr, sizeof(AX_VIN_SYNC_SIGNAL_ATTR_T));
        memcpy(ptLightSyncInfo, &gSc910gsLightSyncInfo, sizeof(AX_VIN_LIGHT_SYNC_INFO_T));
        memcpy(ptSnapStrobeAttr, &gSc910gsSnapStrobeAttr, sizeof(AX_VIN_STROBE_LIGHT_TIMING_ATTR_T));
        memcpy(ptSnapFlashAttr, &gSc910gsSnapFlashAttr, sizeof(AX_VIN_FLASH_LIGHT_TIMING_ATTR_T));
        break;
    case SMARTSENS_SC910GS:
        memcpy(ptPowerAttr, &gSc910gsPowerAttr, sizeof(AX_VIN_POWER_SYNC_ATTR_T));
        memcpy(ptVsyncAttr, &gSc910gsVsyncAttr, sizeof(AX_VIN_SYNC_SIGNAL_ATTR_T));
        memcpy(ptHsyncAttr, &gSc910gsHsyncAttr, sizeof(AX_VIN_SYNC_SIGNAL_ATTR_T));
        memcpy(ptLightSyncInfo, &gSc910gsLightSyncInfo, sizeof(AX_VIN_LIGHT_SYNC_INFO_T));
        memcpy(ptSnapStrobeAttr, &gSc910gsSnapStrobeAttr, sizeof(AX_VIN_STROBE_LIGHT_TIMING_ATTR_T));
        memcpy(ptSnapFlashAttr, &gSc910gsSnapFlashAttr, sizeof(AX_VIN_FLASH_LIGHT_TIMING_ATTR_T));
        break;
    default:
        memcpy(ptPowerAttr, &gSc910gsPowerAttr, sizeof(AX_VIN_POWER_SYNC_ATTR_T));
        memcpy(ptVsyncAttr, &gSc910gsVsyncAttr, sizeof(AX_VIN_SYNC_SIGNAL_ATTR_T));
        memcpy(ptHsyncAttr, &gSc910gsHsyncAttr, sizeof(AX_VIN_SYNC_SIGNAL_ATTR_T));
        memcpy(ptLightSyncInfo, &gSc910gsLightSyncInfo, sizeof(AX_VIN_LIGHT_SYNC_INFO_T));
        memcpy(ptSnapStrobeAttr, &gSc910gsSnapStrobeAttr, sizeof(AX_VIN_STROBE_LIGHT_TIMING_ATTR_T));
        memcpy(ptSnapFlashAttr, &gSc910gsSnapFlashAttr, sizeof(AX_VIN_FLASH_LIGHT_TIMING_ATTR_T));
        break;
    }

    return 0;
}

AX_S32 COMMON_VIN_StartOutsideDev(AX_U8 devId, AX_VIN_POWER_SYNC_ATTR_T *ptSyncPowerAttr,
                              AX_VIN_SYNC_SIGNAL_ATTR_T *ptVsyncAttr, AX_VIN_SYNC_SIGNAL_ATTR_T *ptHsyncAttr,
                              AX_VIN_LIGHT_SYNC_INFO_T *ptLightSyncInfo, AX_VIN_STROBE_LIGHT_TIMING_ATTR_T *ptSnapStrobeAttr,
                              AX_VIN_FLASH_LIGHT_TIMING_ATTR_T *ptSnapFlashAttr)
{
    AX_S32 axRet;

    axRet = AX_VIN_SetSyncPowerAttr(devId, ptSyncPowerAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_SetSyncTriggerMode failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetLightSyncInfo(devId, ptLightSyncInfo);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_SetLightSyncInfo failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetVSyncAttr(devId, ptVsyncAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_SetVSyncAttr failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_EnableVSync(devId);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_EnableVSync failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetHSyncAttr(devId, ptHsyncAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_SetHSyncAttr failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_EnableHSync(devId);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_EnableHSync failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetStrobeTimingAttr(devId, LIGHT_STROBE, ptSnapStrobeAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_SetStrobeTimingAttr failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_EnableStrobe(devId, LIGHT_STROBE);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_EnableStrobe failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetFlashTimingAttr(devId, LIGHT_FLASH, ptSnapFlashAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_SetFlashTimingAttr failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_VIN_StopOutsideDev(AX_U8 devId)
{
    AX_S32 axRet;

    axRet = AX_VIN_DisableVSync(devId);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_EnableVSync failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_DisableHSync(devId);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_EnableHSync failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_DisableStrobe(devId, LIGHT_STROBE);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_EnableStrobe failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_VIN_StartDev(AX_U8 devId, AX_BOOL bEnableDev, AX_VIN_DEV_ATTR_T *pDevAttr)
{
    AX_S32 nRet = 0;
    AX_VIN_DEV_DUMP_ATTR_T  tDumpAttr = {0};

    if (bEnableDev) {
        if (AX_VIN_DEV_OFFLINE == pDevAttr->eDevMode) {
            tDumpAttr.bEnable = AX_TRUE;
            tDumpAttr.nDepth = 3;
            tDumpAttr.eDumpType = AX_VIN_DUMP_QUEUE_TYPE_DEV;
            nRet = AX_VIN_SetDevDumpAttr(devId, &tDumpAttr);
            if (0 != nRet) {
                COMM_VIN_PRT(" AX_VIN_SetDevDumpAttr failed, ret=0x%x.\n", nRet);
                return -1;
            }
        }

        nRet = AX_VIN_EnableDev(devId);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_EnableDev failed, ret=0x%x.\n", nRet);
            return -1;
        }
    }

    return 0;
}

AX_S32 COMMON_VIN_StopDev(AX_U8 devId, AX_BOOL bEnableDev)
{
    AX_S32 axRet;
    AX_VIN_DEV_ATTR_T tDevAttr = {0};
    AX_VIN_DEV_DUMP_ATTR_T tDumpAttr = {0};

    AX_VIN_GetDevAttr(devId, &tDevAttr);

    if (bEnableDev) {
        axRet = AX_VIN_DisableDev(devId);
        if (0 != axRet) {
            COMM_VIN_PRT("AX_VIN_DisableDev failed, devId=%d, ret=0x%x.\n", devId, axRet);
        }

        if (AX_VIN_DEV_OFFLINE == tDevAttr.eDevMode) {
            tDumpAttr.bEnable = AX_FALSE;
            axRet = AX_VIN_SetDevDumpAttr(devId, &tDumpAttr);
            if (0 != axRet) {
                COMM_VIN_PRT(" AX_VIN_SetSnsDumpAttr failed, ret=0x%x.\n", axRet);
            }
        }

    }

    return 0;
}

AX_S32 COMMON_VIN_CreateDev(AX_U8 devId, AX_U32 nRxDev, AX_VIN_DEV_ATTR_T *pDevAttr,
                            AX_VIN_DEV_BIND_PIPE_T *ptDevBindPipe, AX_FRAME_INTERRUPT_ATTR_T *ptDevFrmIntAttr)
{
    AX_S32 nRet = 0;

    nRet = AX_VIN_CreateDev(devId, pDevAttr);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_CreateDev failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_VIN_SetDevAttr(devId, pDevAttr);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_CreateDev failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_VIN_SetDevBindPipe(devId, ptDevBindPipe);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_SetDevBindPipe failed, ret=0x%x\n", nRet);
        return -1;
    }

    nRet = AX_VIN_SetDevBindMipi(devId, nRxDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_SetDevBindMipi failed, ret=0x%x\n", nRet);
        return -1;
    }

    /* configure the attribute of early reporting of frame interrupts */
    if (AX_VIN_DEV_WORK_MODE_1MULTIPLEX < pDevAttr->eDevWorkMode) {
        nRet = AX_VIN_SetDevFrameInterruptAttr(devId, ptDevFrmIntAttr);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_SetDevFrameInterruptAttr failed, ret=0x%x\n", nRet);
            return -1;
        }
    }

    return 0;
}

AX_S32 COMMON_VIN_DestroyDev(AX_U8 devId)
{
    AX_S32 axRet;

    axRet = AX_VIN_DestroyDev(devId);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_DestroyDev failed, devId=%d, ret=0x%x.\n", devId, axRet);
    }

    return 0;
}

AX_S32 COMMON_VIN_SetPipeAttr(COMMON_VIN_MODE_E eSysMode, AX_U8 nPipeId, AX_VIN_PIPE_ATTR_T *pPipeAttr)
{
    AX_S32 axRet;
    AX_VIN_DUMP_ATTR_T sPipeDumpAttr = {0};
    AX_VIN_DUMP_ATTR_T *pPipeDump = &sPipeDumpAttr;

    axRet = AX_VIN_CreatePipe(nPipeId, pPipeAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_CreatePipe failed, ret=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetPipeAttr(nPipeId, pPipeAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VI_SetPipeAttr failed, nRet = 0x%x.\n", axRet);
        return -1;
    }

    if (COMMON_VIN_LOADRAW == eSysMode) {
        axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_USER);
        if (axRet)
            COMM_VIN_PRT("pipe frame source set failed....\n");

        pPipeDump->bEnable = AX_TRUE;
        pPipeDump->nDepth = 1;
        axRet = AX_VIN_SetPipeDumpAttr(nPipeId, AX_VIN_PIPE_DUMP_NODE_MAIN, pPipeDump);
        if (axRet)
            COMM_VIN_PRT("pipe dump attr set failed....\n");
    }

    return 0;
}

AX_S32 COMMON_VIN_StartChn(AX_U8 pipe, AX_VIN_CHN_ATTR_T *ptChnAttr)
{
    AX_S32 nRet = 0;
    AX_S32 chn = 0;

    for (chn = 0; chn < AX_VIN_CHN_ID_MAX; chn++) {
        nRet = AX_VIN_SetChnAttr(pipe, chn, &ptChnAttr[chn]);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_SetChnAttr failed, nRet=0x%x.\n", nRet);
            return -1;
        }

        nRet = AX_VIN_EnableChn(pipe, chn);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_EnableChn failed, nRet=0x%x.\n", nRet);
            return -1;
        }
    }

    return 0;
}

AX_S32 COMMON_VIN_StopChn(AX_U8 pipe)
{
    AX_S32 nRet = 0;
    AX_S32 chn = 0;

    for (chn = 0; chn < AX_VIN_CHN_ID_MAX; chn++) {
        nRet = AX_VIN_DisableChn(pipe, chn);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_DisableChn failed, nRet=0x%x.\n", nRet);
            return -1;
        }
    }

    return 0;
}
