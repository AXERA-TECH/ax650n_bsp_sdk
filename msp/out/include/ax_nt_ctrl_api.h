/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _DEF_NT_CTRL_API_H_
#define _DEF_NT_CTRL_API_H_

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

AX_S32 AX_NT_CtrlInit(AX_U32 nPort);
AX_S32 AX_NT_CtrlDeInit(void);

#ifdef __cplusplus
}
#endif

#endif //_DEF_NT_CTRL_API_H_
