/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_DSP_API_H_
#define _AX_DSP_API_H_

#include <ax_base_type.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
typedef AX_S32 AX_DSP_HANDLE;
typedef enum {
    AX_DSP_SUCCESS = 0,
    AX_DSP_OPEN_FAIL = 0x80040001,
    AX_DSP_MAP_FAIL = 0x80040002,
    AX_DSP_PARA_FAIL = 0x80040003,
    AX_DSP_READ_FAIL = 0x80040004,
    AX_DSP_WRITE_FAIL = 0x80040005,
    AX_DSP_SEND_CMD_FAIL = 0x80040006,
    AX_DSP_QUERY_TIMEOUT_FAIL = 0x80040007,
    AX_DSP_POWERON_FAIL = 0x80040008,
    AX_DSP_BUSY = 0x80040009,
    AX_DSP_MAILBOX_TIMEOUT_FAIL = 0x8004000a,
    AX_DSP_MAILBOX_RW_FAIL = 0x8004000b,
    AX_DSP_ERR_BUTT
} AX_DSP_ERR_E;
/*AX_DSP memory type*/
typedef enum {
    AX_DSP_MEM_TYPE_IRAM = 0x0,
    AX_DSP_MEM_TYPE_SRAM  = 0x1,
    AX_DSP_MEM_TYPE_ITCM  = 0x2,
    AX_DSP_MEM_TYPE_DTCM  = 0x3,
    AX_DSP_MEM_TYPE_BUTT
} AX_DSP_MEM_TYPE_E;
/*AX_DSP core id*/
typedef enum {
    AX_DSP_ID_0 = 0x0,
    AX_DSP_ID_1 = 0x1,
    AX_DSP_ID_BUTT
} AX_DSP_ID_E;

/*AX_DSP  priority*/
typedef enum {
    AX_DSP_PRI_0 = 0x0,
    AX_DSP_PRI_1 = 0x1,
    AX_DSP_PRI_2 = 0x2,
    AX_DSP_PRI_3 = 0x3,
    AX_DSP_PRI_BUTT
} AX_DSP_PRI_E;

typedef enum {
    AX_DSP_CMD_INIT = 0x0,
    AX_DSP_CMD_EXIT = 0x1,
    AX_DSP_CMD_SLEEP = 0x2,
    AX_DSP_CMD_POWEROFF = 0x3,
    AX_DSP_CMD_LOGLEVEL = 0x4,
    AX_DSP_CMD_FBCDC_CHANGE = 0x5,
    AX_DSP_CMD_TEST_COPY = 0x6,
    AX_DSP_CMD_TEST_GAUSSIBLUR = 0x7,
    AX_DSP_CMD_TEST_ADD = 0x8,
    AX_DSP_CMD_OPERATOR = 0x20,
    AX_DSP_CMD_OPERATOR_RESIZE,
    AX_DSP_CMD_USER = 0x1000,
    AX_DSP_CMD_BUTT,
} AX_DSP_CMD_E;

typedef struct {
    AX_U32 u32CMD;
    AX_U32 u32MsgId;
    AX_U32 u32Body[6];
} AX_DSP_MESSAGE_T;

#define AX_DSP_MSGID_USER 0x1000 //User can use MsgID >= AX_DSP_MSGID_USER

AX_S32 AX_DSP_PowerOn(AX_DSP_ID_E enDspId);
AX_S32 AX_DSP_PowerOff(AX_DSP_ID_E enDspId);
AX_S32 AX_DSP_LoadBin(AX_DSP_ID_E enDspId, const char *pszBinFileName, AX_DSP_MEM_TYPE_E enMemType);
AX_S32 AX_DSP_EnableCore(AX_DSP_ID_E enDspId);
AX_S32 AX_DSP_DisableCore(AX_DSP_ID_E enDspId);
AX_S32 AX_DSP_PRC(AX_DSP_HANDLE *phHandle, const AX_DSP_MESSAGE_T *pstMsg, AX_DSP_ID_E enDspId, AX_DSP_PRI_E enPri);
AX_S32 AX_DSP_Query(AX_DSP_ID_E enDspId, AX_DSP_HANDLE hHandle, AX_DSP_MESSAGE_T *msg, AX_BOOL bBlock);

typedef struct {
    AX_U32 zero_bits_cnt;
    AX_U32 comp_level;
    AX_U32 pixel_format;
    AX_U32 tile_width;
    //below is not necessary, can be computed or fetched
    AX_U32  layer_no;
    AX_U32  start_position_h_layer;
    AX_U32  start_position_l_layer;
    AX_U32  end_position_h_layer;
    AX_U32  end_position_l_layer;
    AX_U32  hor_subimage_subtile_num_layer;
    AX_U32  hor_subtile_num_layer;
    AX_U32  subimage_subtile_start_pos_x_layer;
    AX_U32  ver_tile_num_layer;
} AX_DSP_FBCDC_PARAM;

AX_S32 AX_DSP_EnableFBCDC(AX_DSP_ID_E enDspId, AX_BOOL fbcEnable, AX_BOOL fbdcEnable);
AX_S32 AX_DSP_SetFBC(AX_DSP_ID_E enDspId, AX_DSP_FBCDC_PARAM *param);
AX_S32 AX_DSP_SetFBDC(AX_DSP_ID_E enDspId, AX_DSP_FBCDC_PARAM *param);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_AX_SVP_DSP_H_*/

