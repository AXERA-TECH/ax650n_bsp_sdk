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
unsigned int ax_dsp_base_ddr_config(unsigned long long addr);
unsigned int ax_cpu_addr_to_dsp_addr(unsigned long long addr);
unsigned long long ax_dsp_addr_to_idma_addr(unsigned int addr);
unsigned long long ax_dsp_addr_to_cpu_addr(unsigned int addr);
void ax_dsp_log_config(unsigned int uart_id, unsigned int baud_rate);
void ax_dsp_memlog_config(unsigned int enable);
int mailbox_init(void);
int mailbox_read(uint32_t *data, uint32_t size);
int mailbox_write(uint32_t *data, uint32_t size);
int mailbox_deinit(void);
#endif


