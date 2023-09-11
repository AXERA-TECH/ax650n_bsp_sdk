/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_GLOBAL_H__
#define __SAMPLE_GLOBAL_H__

#include <string.h>
#include <sys/prctl.h>

#include "ax_sys_api.h"
#include "ax_venc_api.h"
#include "ax_venc_comm.h"
#include "common_venc.h"
#include "sample_venc_log.h"

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

#define SAMPLE_ALL_CODEC_TYPE   (4)
#define UT_DEFAULT_ENC_NUM      (3)
#define SAMPLE_MAX_TESTCASE_NUM (32)

#define EIGHT_BIT_LEVEL_4_TILE_SIZE (128)
#define TEN_BIT_LEVEL_5_TILE_SIZE   (160)

#define CLIP3(x, y, z) ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))
#define MIN(a, b)      ((a) < (b) ? (a) : (b))

#define MAX_CU_SIZE               (64)
#define MAX_AVC_CU_SIZE           (16)
#define VENC_LOOP_EXIT_SIGNAL_VAL (0xFFFFFFFFFFFFFFFF)

#define VENC_TEST_ALL_CASE (1024)


#define SAMPLE_VENC_MIN_BLK_CNT (1)
#define SAMPLE_VENC_MAX_BLK_CNT (100)

AX_PAYLOAD_TYPE_E SampleGetCodecType(AX_S32 codecId);

AX_VOID SampleSendFrameInit(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_SENDFRAME_PARA_T *pstArg,
                            SAMPLE_VENC_CMD_PARA_T *pCml);

AX_VOID SampleGetStreamInit(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_GETSTREAM_PARA_T *pstArg,
                            SAMPLE_VENC_CMD_PARA_T *pCml);

AX_S32 SAMPLE_VENC_StartSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg);
AX_S32 SAMPLE_VENC_StopSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg);

AX_S32 SAMPLE_VENC_StartGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg);
AX_S32 SAMPLE_VENC_StopGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg);
AX_VOID SAMPLE_VENC_PrintNaluInfo(AX_S32 VeChn, AX_VENC_STREAM_T *pstStream);

AX_VOID SAMPLE_VENC_FWRITE(const AX_VOID *ptr, AX_S32 size, AX_S32 nmemb, FILE *stream, AX_BOOL bSaveFile);
AX_VOID SAMPLE_VENC_SetThreadName(const AX_CHAR *nameFmt, ...);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif