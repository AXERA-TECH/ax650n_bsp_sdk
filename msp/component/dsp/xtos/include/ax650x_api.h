/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _INC_AX630X_API_H_
#define _INC_AX630X_API_H_

unsigned int ax_dsp_base_ddr_config(void);
unsigned int ax_cpu_addr_to_dsp_addr(unsigned long long addr);
unsigned long long ax_dsp_addr_to_idma_addr(unsigned int addr);
unsigned long long ax_dsp_addr_to_cpu_addr(unsigned int addr);
void ax_dsp_log_config(unsigned int uart_id);
void ax_dsp_memlog_config(unsigned int enable);
int AX_Mailbox_Init(void);
int AX_Mailbox_Read(uint32_t *data, uint32_t size);
int AX_Mailbox_Write(int mid, uint32_t *data, uint32_t size);
int AX_Mailbox_Read_Fromid(int mid, uint32_t *data, uint32_t size, int block);
int AX_Mailbox_Deinit(void);
int AX_Mailbox_Start(void);

typedef struct {
    AX_U32 bypass;
#if 0
    AX_U32 zero_bits_cnt;
    AX_U32 comp_level;
#endif
    AX_U32 pixel_format;
    AX_U32 tile_width;
#if 0
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
#endif
} AX_FBCDC_PARAM;

AX_FBCDC_PARAM *AX_VDSP_FBC_GetInfo(void);
AX_FBCDC_PARAM *AX_VDSP_FBDC_GetInfo(void);
//AX_S32 AX_VDSP_CopyData_FBCDC(AX_U64 dst, AX_U64 src, AX_S32 s32Size);
//AX_S32 AX_VDSP_Copy2DAddr_FBCDC(AX_U64 dst, AX_U64 src, AX_S32 s32Size);
AX_S32 AX_VDSP_CheckFbcdc(AX_U64 dst, AX_U64 src, AX_S32 size);
void AX_VDSP_Update_Fbcdc_Para(void);

extern uint32_t _procid;

#endif


