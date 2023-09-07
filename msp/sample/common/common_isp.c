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
#include <pthread.h>
#include <sys/prctl.h>

#include "ax_vin_api.h"
#include "ax_isp_api.h"
#include "ax_isp_3a_api.h"
#include "common_type.h"
#include "common_isp.h"
#include "common_sys.h"
#include "common_hw.h"

static AX_S32 g_isp_force_loop_exit = 0;
static pthread_t gIspProcThread[AX_VIN_MAX_PIPE_NUM * MAX_CAMERAS] = {0};
static AX_ISP_AF_REGFUNCS_T s_tAfFuncs = {0};

#define AX_LIB_SENSOR_PATH  "/soc/lib/"

typedef struct _AX_SENSOR_LIB_TAB_ {
    SAMPLE_SNS_TYPE_E eSnsType;
    AX_CHAR libSnsName[32];
    AX_CHAR pSnsObjName[32];
} AX_SENSOR_LIB_TAB;

#ifdef SAMPLE_BUILD_STATIC
/* Function declaration for sensor handle */
extern AX_SENSOR_REGISTER_FUNC_T gSnsdummyObj;
extern AX_SENSOR_REGISTER_FUNC_T gSnsos08a20Obj;
extern AX_SENSOR_REGISTER_FUNC_T gSnssc910gsObj;
#else
const static AX_SENSOR_LIB_TAB s_libSensorTab[] = {
    {SAMPLE_SNS_DUMMY,               "libsns_dummy.so",          "gSnsdummyObj"},
    {OMNIVISION_OS08A20,             "libsns_os08a20.so",        "gSnsos08a20Obj"},
    {SMARTSENS_SC910GS,              "libsns_sc910gs.so",        "gSnssc910gsObj"},
    {SAMPLE_SNS_TYPE_BUTT,           "NULL",                     "NULL"},
};
#endif



/* TODO user need config device node number */
static AX_S8 COMMON_ISP_GetI2cDevNode(AX_U8 nDevId)
{
    AX_CHAR apd_plate_id[BOARD_ID_LEN] = {0};
    COMMON_SYS_GetApdPlateId(apd_plate_id);

    if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE", sizeof("ADP_RX_DPHY_2X4LANE") - 1)) {
        if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE_N", sizeof("ADP_RX_DPHY_2X4LANE_N") - 1)) {
            switch (nDevId) {
            case 0:
                return 1;
            case 2:
                return 2;
            default:
                return 1;
            }
        } else {
            switch (nDevId) {
            case 0:
                return 1;
            case 4:
                return 2;
            default:
                return 1;
            }
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DPHY_8X2LANE", sizeof("ADP_RX_DPHY_8X2LANE") - 1)
               || !strncmp(apd_plate_id, "ADP_RX_CPHY_8X2TRIO", sizeof("ADP_RX_CPHY_8X2TRIO") - 1)) {
        switch (nDevId) {
        case 0:
            return 1;
        case 1:
            return 1;
        case 2:
            return 2;
        case 3:
            return 2;
        case 4:
            return 3;
        case 5:
            return 3;
        case 6:
            return 9;
        case 7:
            return 9;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DPHY_4X4LANE", sizeof("ADP_RX_DPHY_4X4LANE") - 1)
               || !strncmp(apd_plate_id, "ADP_RX_CPHY_4X3TRIO", sizeof("ADP_RX_CPHY_4X3TRIO") - 1)) {
        switch (nDevId) {
        case 0:
            return 1;
        case 2:
            return 2;
        case 4:
            return 3;
        case 6:
            return 9;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X8_LVDS_1X16LANE", sizeof("ADP_RX_DPHY_2X8_LVDS_1X16LANE") - 1)) {
        switch (nDevId) {
        case 0:
            return 1;
        case 2:
            return 2;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DVP_1X10BIT_DPHY_1X2LANE_BT601_656_1120", sizeof("ADP_RX_DVP_1X10BIT_DPHY_1X2LANE_BT601_656_1120") - 1)) {
        switch (nDevId) {
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT1120_2X10BIT", sizeof("ADP_RX_BT1120_2X10BIT") - 1)) {
        switch (nDevId) {
        case 4:
            return 1;
        case 5:
            return 0;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT1120_2X8BIT_DPHY_2X2LANE", sizeof("ADP_RX_BT1120_2X8BIT_DPHY_2X2LANE") - 1)) {
        switch (nDevId) {
        case 3:
            return 2;
        case 4:
            return 1;
        case 5:
            return 0;
        case 7:
            return 9;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT601_656_2X10BIT_DPHY_2X2LANE", sizeof("ADP_RX_BT601_656_2X10BIT_DPHY_2X2LANE") - 1)) {
        switch (nDevId) {
        case 2:
            return 2;
        case 4:
            return 1;
        case 5:
            return 0;
        case 6:
            return 9;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT601_656_2X8BIT_DPHY_4X2LANE", sizeof("ADP_RX_BT601_656_2X8BIT_DPHY_4X2LANE") - 1)) {
        switch (nDevId) {
        case 2:
            return 2;
        case 3:
            return 2;
        case 4:
            return 1;
        case 5:
            return 0;
        case 6:
            return 9;
        case 7:
            return 9;
        default:
            return 1;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DVP_16bit_HDMI_TO_DPHY_2X4LANE", sizeof("ADP_RX_DVP_16bit_HDMI_TO_DPHY_2X4LANE") - 1)) {
        switch (nDevId) {
        case 0:
            return 1;
        default:
            return 1;
        }
    }

    return 1;
}

#ifdef SAMPLE_BUILD_STATIC
AX_SENSOR_REGISTER_FUNC_T *COMMON_ISP_GetSnsObj(SAMPLE_SNS_TYPE_E eSnsType)
{
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl = NULL;

    switch (eSnsType) {
    case SAMPLE_SNS_DUMMY:
        ptSnsHdl = &gSnsdummyObj;
        break;
    case OMNIVISION_OS08A20:
        ptSnsHdl = &gSnsos08a20Obj;
        break;
    default:
        ptSnsHdl = &gSnsos08a20Obj;
        break;
    }

    return ptSnsHdl;
}
#else
AX_SENSOR_REGISTER_FUNC_T *COMMON_ISP_GetSnsObj(SAMPLE_SNS_TYPE_E eSnsType)
{
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl = NULL;
    void *handler = NULL;
    AX_CHAR *err = NULL;
    AX_U16 i = 0;
    AX_CHAR *pSnsPath = NULL;
    AX_CHAR *pObjName = NULL;
    AX_CHAR path[128] = AX_LIB_SENSOR_PATH;

    for (i = 0; i < sizeof(s_libSensorTab) / sizeof(s_libSensorTab[0]); i++) {
        if (eSnsType == s_libSensorTab[i].eSnsType) {
            strncat(path, s_libSensorTab[i].libSnsName, sizeof(path) - strlen(AX_LIB_SENSOR_PATH));
            pSnsPath = (AX_CHAR *)path;
            pObjName = (AX_CHAR *)s_libSensorTab[i].pSnsObjName;
            break;
        }
    }

    if ((NULL != pSnsPath) && (NULL != pObjName)) {
        handler = dlopen((void *)pSnsPath, RTLD_LAZY);
        if (NULL == handler) {
            COMM_ISP_PRT("open %s fail!\n", pSnsPath);
            return NULL;
        }
        ptSnsHdl = (AX_SENSOR_REGISTER_FUNC_T *)dlsym(handler, pObjName);
        err = dlerror();
        if (NULL != err) {
            ptSnsHdl = NULL;
            COMM_ISP_PRT("dlsym %s fail!\n", pObjName);
        }
    } else {
        ptSnsHdl = NULL;
        COMM_ISP_PRT("not find eSnsType = %d\n", eSnsType);
    }

    return ptSnsHdl;
}
#endif

static AX_SNS_CONNECT_TYPE_E COMMON_ISP_GetSnsBusType(SAMPLE_SNS_TYPE_E eSnsType)
{
    AX_SNS_CONNECT_TYPE_E enBusType;

    switch (eSnsType) {
    case SAMPLE_SNS_DUMMY:
    case OMNIVISION_OS08A20:
    case SAMPLE_SNS_TYPE_BUTT:
        enBusType = ISP_SNS_CONNECT_I2C_TYPE;
        break;
    default:
        enBusType = ISP_SNS_CONNECT_I2C_TYPE;
        break;
    }

    return enBusType;
}

AX_S32 COMMON_ISP_RegisterSns(AX_U8 pipe, AX_U8 nDevId, SAMPLE_SNS_TYPE_E eSnsType)
{
    AX_S32 axRet = 0;
    AX_SNS_COMMBUS_T tSnsBusInfo = {0};
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl = NULL;
    AX_SNS_CONNECT_TYPE_E eBusType;

    /* AX ISP get sensor config */
    ptSnsHdl = COMMON_ISP_GetSnsObj(eSnsType);
    if (NULL == ptSnsHdl) {
        COMM_ISP_PRT("AX_ISP Get Sensor Object Failed!\n");
        return -1;
    }

    /* confige i2c/spi dev id */
    eBusType = COMMON_ISP_GetSnsBusType(eSnsType);

    /* ISP Register Sensor */
    axRet = AX_ISP_RegisterSensor(pipe, ptSnsHdl);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    /* confige i2c/spi dev id */
    if (ISP_SNS_CONNECT_I2C_TYPE == eBusType) {
        tSnsBusInfo.I2cDev = COMMON_ISP_GetI2cDevNode(nDevId);
    } else {
        tSnsBusInfo.SpiDev.bit4SpiDev = COMMON_ISP_GetI2cDevNode(nDevId);
        tSnsBusInfo.SpiDev.bit4SpiCs  = 0;
    }

    if (NULL != ptSnsHdl->pfn_sensor_set_bus_info) {
        axRet = ptSnsHdl->pfn_sensor_set_bus_info(pipe, tSnsBusInfo);
        if (0 != axRet) {
            COMM_ISP_PRT("set sensor bus info failed with %#x!\n", axRet);
            return axRet;
        }
        COMM_ISP_PRT("set sensor bus idx %d\n", tSnsBusInfo.I2cDev);
    } else {
        COMM_ISP_PRT("not support set sensor bus info!\n");
        return -1;
    }

    return 0;
}

AX_S32 COMMON_ISP_UnRegisterSns(AX_U8 pipe)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_UnRegisterSensor(pipe);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_SetSnsAttr(AX_U8 nPipeId, AX_U8 nDevId, AX_SNS_ATTR_T *ptSnsAttr, AX_SNS_CLK_ATTR_T *pstSnsClkAttr)
{
    AX_S32 axRet = 0;
    AX_U32 nResetGpio;

    /* confige sensor attr */
    axRet = AX_ISP_SetSnsAttr(nPipeId, ptSnsAttr);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_SetSnsAttr failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    /* confige sensor clk */
    axRet = AX_ISP_OpenSnsClk(pstSnsClkAttr->nSnsClkIdx, pstSnsClkAttr->eSnsClkRate);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_OpenSnsClk failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    /* reset sensor */
    nResetGpio = COMMON_VIN_GetSensorResetGpioNum(nDevId);
    axRet = AX_ISP_ResetSensor(nPipeId, nResetGpio);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_ResetSensor failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    return axRet;
}


AX_S32 COMMON_ISP_RegisterAeAlgLib(AX_U8 pipe, SAMPLE_SNS_TYPE_E eSnsType, AX_BOOL bUser3a,
                                   AX_ISP_AE_REGFUNCS_T *pAeFuncs)
{
    AX_S32 axRet = 0;
    AX_ISP_AE_REGFUNCS_T tAeFuncs = {0};
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl = NULL;

    /* 3a get sensor config */
    ptSnsHdl = COMMON_ISP_GetSnsObj(eSnsType);

    if (!bUser3a) {
        tAeFuncs.pfnAe_Init = AX_ISP_ALG_AeInit;
        tAeFuncs.pfnAe_Exit = AX_ISP_ALG_AeDeInit;
        tAeFuncs.pfnAe_Run  = AX_ISP_ALG_AeRun;
        tAeFuncs.pfnAe_Ctrl  = AX_ISP_ALG_AeCtrl;
    } else {
        tAeFuncs.pfnAe_Init = pAeFuncs->pfnAe_Init;
        tAeFuncs.pfnAe_Exit = pAeFuncs->pfnAe_Exit;
        tAeFuncs.pfnAe_Run  = pAeFuncs->pfnAe_Run;
        tAeFuncs.pfnAe_Ctrl  = pAeFuncs->pfnAe_Ctrl;
    }

    /* Register the sensor driver interface TO the AE library */
    axRet = AX_ISP_ALG_AeRegisterSensor(pipe, ptSnsHdl);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    /* Register ae alg */
    axRet = AX_ISP_RegisterAeLibCallback(pipe, &tAeFuncs);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register ae callback Failed, ret=0x%x.\n", axRet);
        return axRet;

    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterAeAlgLib(AX_U8 pipe)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_ALG_AeUnRegisterSensor(pipe);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP ae un register sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    axRet = AX_ISP_UnRegisterAeLibCallback(pipe);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}


AX_S32 COMMON_ISP_RegisterAwbAlgLib(AX_U8 pipe, SAMPLE_SNS_TYPE_E eSnsType, AX_BOOL bUser3a,
                                    AX_ISP_AWB_REGFUNCS_T *pAwbFuncs)
{
    AX_S32 axRet = 0;
    AX_ISP_AWB_REGFUNCS_T tAwbFuncs = {0};

    if (!bUser3a) {
        tAwbFuncs.pfnAwb_Init = AX_ISP_ALG_AwbInit;
        tAwbFuncs.pfnAwb_Exit = AX_ISP_ALG_AwbDeInit;
        tAwbFuncs.pfnAwb_Run  = AX_ISP_ALG_AwbRun;
    } else {
        tAwbFuncs.pfnAwb_Init = pAwbFuncs->pfnAwb_Init;
        tAwbFuncs.pfnAwb_Exit = pAwbFuncs->pfnAwb_Exit;
        tAwbFuncs.pfnAwb_Run  = pAwbFuncs->pfnAwb_Run;
    }
    /* Register awb alg */
    axRet = AX_ISP_RegisterAwbLibCallback(pipe, &tAwbFuncs);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register awb callback Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterAwbAlgLib(AX_U8 pipe)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_UnRegisterAwbLibCallback(pipe);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}


AX_S32 COMMON_ISP_RegisterLscAlgLib(AX_U8 pipe, SAMPLE_SNS_TYPE_E eSnsType, AX_BOOL bUser3a,
                                    AX_ISP_LSC_REGFUNCS_T *pLscFuncs)
{
    AX_S32 axRet = 0;

    /* Register Lsc alg */
    axRet = AX_ISP_RegisterLscLibCallback(pipe, pLscFuncs);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register Lsc callback Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterLscAlgLib(AX_U8 pipe)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_UnRegisterLscLibCallback(pipe);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}


AX_S32 COMMON_ISP_Init(AX_U8 pipe, SAMPLE_SNS_TYPE_E eSnsType, AX_BOOL bRegisterSns,
                       AX_BOOL bUser3a, AX_ISP_AE_REGFUNCS_T *tAeFuncs, AX_ISP_AWB_REGFUNCS_T *tAwbFuncs,
                       AX_ISP_AF_REGFUNCS_T *tAfFuncs, AX_ISP_LSC_REGFUNCS_T *tLscFuncs, AX_CHAR *pIspParamsFile)
{
    AX_S32 axRet = 0;
    AX_S32 nRet = 0;

    axRet = AX_ISP_Create(pipe);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_Init failed, ret=0x%x\n", axRet);
        return -1;
    }

    if (bRegisterSns) {
        /* ae alg register*/
        axRet = COMMON_ISP_RegisterAeAlgLib(pipe, eSnsType, bUser3a, tAeFuncs);
        if (0 != axRet) {
            COMM_ISP_PRT("RegisterAeAlgLib failed, ret=0x%x.\n", axRet);
            return -1;
        }

        /* awb alg register*/
        axRet = COMMON_ISP_RegisterAwbAlgLib(pipe, eSnsType, bUser3a, tAwbFuncs);
        if (0 != axRet) {
            COMM_ISP_PRT("RegisterAwbAlgLib failed, ret=0x%x.\n", axRet);
            return -1;
        }
    }

    nRet = AX_ISP_LoadBinParams(pipe, pIspParamsFile);
    if (0 != nRet) {
        COMM_ISP_PRT("AX_ISP_LoadBinParams failed! will use sensor.h\n");
    }

    if(AX_NULL != tAfFuncs) {
        s_tAfFuncs = *tAfFuncs;
        if (AX_NULL != s_tAfFuncs.pfnCAf_Init) {
            s_tAfFuncs.pfnCAf_Init(pipe);
        }
    }

    axRet = AX_ISP_Open(pipe);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_Open failed, ret=0x%x\n", axRet);
        return -1;
    }

    return axRet;
}

AX_S32 COMMON_ISP_DeInit(AX_U8 pipe, AX_BOOL bRegisterSns)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_Close(pipe);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_Close failed, ret=0x%x.\n", axRet);
    }

    if (bRegisterSns) {
        COMMON_ISP_UnRegisterAeAlgLib(pipe);
        COMMON_ISP_UnRegisterAwbAlgLib(pipe);
    }

    if (AX_NULL != s_tAfFuncs.pfnCAf_Exit) {
        s_tAfFuncs.pfnCAf_Exit(pipe);
        memset(&s_tAfFuncs, 0, sizeof(AX_ISP_AF_REGFUNCS_T));
    }

    axRet = AX_ISP_Destroy(pipe);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_Exit failed, ret=0x%x.\n", axRet);
    }

    return axRet;
}

static void *IspRun(void *args)
{
    AX_U32 nPipe = (AX_ULONG)args;
    AX_CHAR token[32] = {0};

    COMM_ISP_PRT("nPipe %d is running...\n", nPipe);

    snprintf(token, 32, "ISP_RUN_%u", nPipe);
    prctl(PR_SET_NAME, token);

    while (!g_isp_force_loop_exit) {
        AX_ISP_Run(nPipe);
    }
    return NULL;
}

AX_S32 COMMON_ISP_Run(const AX_VIN_DEV_BIND_PIPE_T tDevBindPipe, const SAMPLE_PIPE_INFO_T *pPipeInfo)
{
    AX_S32 axRet = 0;
    AX_S32 i = 0, j = 0;
    g_isp_force_loop_exit = 0;

    for (j = 0; j < tDevBindPipe.nNum; j++) {
        if (pPipeInfo[j].ePipeMode != SAMPLE_PIPE_MODE_FLASH_SNAP) {
            for (i = 0; i < AX_VIN_MAX_PIPE_NUM; i++) {
                if (gIspProcThread[i] == 0) {
                    pthread_create(&gIspProcThread[i], NULL, IspRun, (AX_VOID *)(AX_ULONG)(tDevBindPipe.nPipeId[j]));
                    break;
                }
            }
        }
    }


    return axRet;
}

AX_S32 COMMON_ISP_Stop(void)
{
    AX_S32 axRet = 0;
    AX_S32 i = 0;

    g_isp_force_loop_exit = 1;
    for (i = 0; i < AX_VIN_MAX_PIPE_NUM * MAX_CAMERAS; i++) {
        if (gIspProcThread[i] != 0) {
            axRet = pthread_join(gIspProcThread[i], NULL);
            if (axRet < 0) {
                COMM_ISP_PRT(" isp run thread exit failed, ret=0x%x.\n", axRet);
            }
            gIspProcThread[i] = 0;
        }
    }

    return 0;
}

AX_S32 COMMON_ISP_SetAeToManual(AX_U8 pipe)
{
    AX_S32 axRet = 0;
    AX_ISP_IQ_AE_PARAM_T tIspAeParam = {0};

    axRet = AX_ISP_IQ_GetAeParam(pipe, &tIspAeParam);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_IQ_GetAeParam failed, ret=0x%x.\n", axRet);
        return -1;
    }

    tIspAeParam.nEnable = AX_FALSE;

    axRet = AX_ISP_IQ_SetAeParam(pipe, &tIspAeParam);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_IQ_SetAeParam failed, nRet = 0x%x.\n", axRet);
        return -1;
    }

    return 0;
}