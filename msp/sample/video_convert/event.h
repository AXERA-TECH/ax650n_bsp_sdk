/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_EVENT_H__
#define __SAMPLE_EVENT_H__

#include "def.h"

AX_HANDLE sample_create_event(const AX_CHAR *name);
AX_BOOL sample_destory_event(AX_HANDLE handle);

AX_BOOL sample_set_event(AX_HANDLE handle);
AX_BOOL sample_reset_event(AX_HANDLE handle);
AX_BOOL sample_wait_event(AX_HANDLE handle, AX_S32 timeout);

#endif /* __SAMPLE_EVENT_H__ */