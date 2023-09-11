/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_AENC_HAL_H_
#define _AX_AENC_HAL_H_

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_codec_comm.h"
#include "ax_comm_aio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common define */
#define AX_AENC_MAX_CHN_NUM                 16     /* support 16 channel output */
#define AX_MAX_AUDIO_FRAME_NUM              300

typedef enum {
    AX_ID_AENC_NULL    = 0x01,

    AX_ID_AENC_BUTT,
} AX_AENC_SUB_ID_E;

/* error code */
#ifndef AX_SUCCESS
#define AX_SUCCESS                          0
#endif
#define AX_ERR_AENC_INVALID_CHNID               AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_INVALID_CHNID)
#define AX_ERR_AENC_NULL_PTR                    AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_NULL_PTR)
#define AX_ERR_AENC_EXIST                       AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_EXIST)
#define AX_ERR_AENC_NOMEM                       AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_NOMEM)
#define AX_ERR_AENC_NOT_SUPPORT                 AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_NOT_SUPPORT)
#define AX_ERR_AENC_UNEXIST                     AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_UNEXIST)
#define AX_ERR_AENC_NOBUF                       AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_NOBUF)
#define AX_ERR_AENC_NOT_PERM                    AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_NOT_PERM)
#define AX_ERR_AENC_BUF_FULL                    AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_BUF_FULL)
#define AX_ERR_AENC_BUF_EMPTY                   AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_BUF_EMPTY)
#define AX_ERR_AENC_SYS_NOTREADY                AX_DEF_ERR(AX_ID_AENC, AX_ID_AENC_NULL, AX_ERR_SYS_NOTREADY)

typedef AX_S32 AENC_CHN;

typedef struct axAENC_CHN_ATTR_T {
    AX_PAYLOAD_TYPE_E   enType;             /*payload type ()*/
    AX_U32              u32PtNumPerFrm;
    AX_U32              u32BufSize;         /*buf size [2~MAX_AUDIO_FRAME_NUM]*/
    AX_VOID             *pValue;            /*point to attribute of definite audio encoder*/
    AX_LINK_MODE_E       enLinkMode;
} AX_AENC_CHN_ATTR_T;

typedef struct axAENC_ENCODER_T {
    AX_PAYLOAD_TYPE_E  enType;
    AX_U32          u32MaxFrmLen;
    AX_S8           aszName[17];            /* encoder type,be used to print proc information */
    AX_S32(*pfnOpenEncoder)(AX_VOID *pEncoderAttr,
                            AX_VOID **ppEncoder);           /* pEncoder is the handle to control the encoder */
    AX_S32(*pfnEncodeFrm)(AX_VOID *pEncoder, const AX_AUDIO_FRAME_T *pstData,
                          AX_U8 *pu8Outbuf, AX_U32 *pu32OutLen);
    AX_S32(*pfnCloseEncoder)(AX_VOID *pEncoder);
} AX_AENC_ENCODER_T;

typedef enum axAAC_TYPE_E {
    AX_AAC_TYPE_NONE = -1,
    AX_AAC_TYPE_NULL_OBJECT = 0,
    AX_AAC_TYPE_AAC_LC = 2,   /**< Low Complexity object                     */
    AX_AAC_TYPE_ER_AAC_LD = 23,        /**< Error Resilient(ER) AAC LowDelay object   */
    AX_AAC_TYPE_ER_AAC_ELD = 39,  /**< AAC Enhanced Low Delay                    */

    AX_AAC_TYPE_BUTT,
} AX_AAC_TYPE_E;

typedef enum axAAC_CHANNEL_MODE_E {
    AX_AAC_CHANNEL_MODE_INVALID = -1,
    AX_AAC_CHANNEL_MODE_UNKNOWN = 0,
    AX_AAC_CHANNEL_MODE_1 = 1,         /**< C */
    AX_AAC_CHANNEL_MODE_2 = 2,         /**< L+R */
    AX_AAC_CHANNEL_MODE_1_2 = 3,       /**< C, L+R */
    AX_AAC_CHANNEL_MODE_1_2_1 = 4,     /**< C, L+R, Rear */
    AX_AAC_CHANNEL_MODE_1_2_2 = 5,     /**< C, L+R, LS+RS */
    AX_AAC_CHANNEL_MODE_1_2_2_1 = 6,   /**< C, L+R, LS+RS, LFE */

    AX_AAC_CHANNEL_MODE_BUTT,
} AX_AAC_CHANNEL_MODE_E;

typedef struct axAENC_AAC_ENCODER_ATTR_T {
    AX_AAC_TYPE_E enAacType;
    AX_AAC_TRANS_TYPE_E enTransType;
    AX_AAC_CHANNEL_MODE_E enChnMode;
    AX_U32 u32GranuleLength;
    AX_U32 u32SampleRate;
    AX_U32 u32BitRate;
    AX_U8 u8ConfBuf[64]; /*!< Configuration buffer in binary format as an
                          AudioSpecificConfig or StreamMuxConfig according to the
                          selected transport type. */
} AX_AENC_AAC_ENCODER_ATTR_T;

typedef struct axAENC_G726_ENCODER_ATTR_T {
    AX_U32 u32BitRate;
} AX_AENC_G726_ENCODER_ATTR_T;

typedef enum axOPUS_APPLICATION_E {
    AX_OPUS_APPLICATION_UNKNOWN = 0,
    AX_OPUS_APPLICATION_VOIP = 2048,
    AX_OPUS_APPLICATION_AUDIO = 2049,
    AX_OPUS_APPLICATION_RESTRICTED_LOWDELAY = 2051,

    AX_OPUS_APPLICATION_BUTT,
} AX_OPUS_APPLICATION_E;

typedef struct axAENC_OPUS_ENCODER_ATTR_T {
    AX_OPUS_APPLICATION_E enApplication;
    AX_U32 u32SamplingRate; /* 8000, 12000, 16000, 24000 and 48000*/
    AX_S32 s32Channels; /* 1 or 2 */
    AX_S32 s32BitrateBps;
    AX_F32 f32FramesizeInMs; /* 2.5, 5, 10, 20, 40, 60, 80, 100, 120 */
} AX_AENC_OPUS_ENCODER_ATTR_T;

AX_S32 AX_AENC_Init();
AX_S32 AX_AENC_DeInit();

AX_S32 AX_AENC_CreateChn(AENC_CHN aeChn, const AX_AENC_CHN_ATTR_T *pstAttr);
AX_S32 AX_AENC_DestroyChn(AENC_CHN aeChn);

AX_S32 AX_AENC_SendFrame(AENC_CHN aeChn, const AX_AUDIO_FRAME_T *pstFrm);

AX_S32 AX_AENC_GetStream(AENC_CHN aeChn, AX_AUDIO_STREAM_T *pstStream, AX_S32 s32MilliSec);
AX_S32 AX_AENC_ReleaseStream(AENC_CHN aeChn, const AX_AUDIO_STREAM_T *pstStream);

AX_S32 AX_AENC_RegisterEncoder(AX_S32 *ps32Handle, const AX_AENC_ENCODER_T *pstEncoder);
AX_S32 AX_AENC_UnRegisterEncoder(AX_S32 s32Handle);

#ifdef __cplusplus
}
#endif

#endif
