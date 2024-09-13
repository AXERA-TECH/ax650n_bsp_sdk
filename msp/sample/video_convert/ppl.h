/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_PPL_H__
#define __SAMPLE_PPL_H__

#include "def.h"

/* SDK init */
AX_BOOL sample_sys_init(AX_VOID);
AX_BOOL sample_sys_deinit(AX_VOID);

/* PPL: stream(demux) -> vdec -> venc -> record */
AX_HANDLE sample_create_ppl(const AX_CHAR *fpath, AX_PAYLOAD_TYPE_E payload, AX_U32 width, AX_U32 height, AX_S32 order);
AX_BOOL sample_destory_ppl(AX_HANDLE handle);

AX_BOOL sample_start_ppl(AX_HANDLE handle);
AX_BOOL sample_wait_ppl_eof(AX_HANDLE handle, AX_S32 timeout);
AX_BOOL sample_stop_ppl(AX_HANDLE handle);

#endif /* __SAMPLE_PPL_H__ */