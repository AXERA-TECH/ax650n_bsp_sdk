/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once

#include "ax_global_type.h"


#define RTP_VERSION		2
#define RTP_MAXPAYLOAD  1400

#define AX_RTP_HEADER_LEN 20

#pragma pack(4)
typedef struct
{
	AX_U8  v_p_x_cc;
    AX_U8  payload;
	AX_U16 seq_num;
	AX_U32 timestamp;
	AX_U32 ssrc;
    AX_U64 u64SeqNum;
}RTP_HEADER;
#pragma pack()

typedef struct _AX_MEDIA_DATA_T
{
    AX_U8   *mediaData;
    AX_S32   mediaDataLen;
    AX_U32   timestamp;
    AX_S32   type;
    AX_U64   u64SeqNum;
}AX_MEDIA_DATA_T;


