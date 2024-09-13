/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __VIDEO_UTIL_H
#define __VIDEO_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>

#include "ax_sys_api.h"
#include "ax_venc_api.h"

#include "common_venc.h"
#include "ax_global_type.h"

#include "uvc.h"
#include "common_vin.h"
#include "common_cam.h"
#include "common_sys.h"

static const AX_U32 u32MaxPixelWidth = 16384;
static const AX_U32 u32MaxPixelHeight = 16384;
static const AX_U32 u32DefaultSrcPixelWidth = 3840;
static const AX_U32 u32DefaultSrcPixelHeight = 2160;

static const AX_U32 u32StrideAlign = 64;
static const AX_F32 f32SrcVideoFrameRate = 30.0;
static const AX_F32 f32DstVideoFrameRate = 30.0;

/*QP:0-51, QPLevel:0-99, the lower qp/qp level value, the higher quality*/
static const AX_U32 gU32QPMin = 10;
static const AX_U32 gU32QPMax = 22;
static const AX_U32 gU32FixedQP = 22;
static const AX_S32 gS32QPLevel = 90;

static const AX_S32 gS32EnableCrop = 0;
static const AX_S32 gS32QTableEnable = 0;

static const AX_S32 gSyncType = -1;

#define UVC_SNS_OS08A20_MAX_WIDTH  3840
#define UVC_SNS_OS08A20_MAX_HEIGHT 2160

#define JENC_NUM_COMM  64
#define ISP_JENC_NUM 3

#define VENC_NUM_COMM  64

#define MAX_UVC_CAMERAS 2

#define VIDEO_ENABLE_RC_DYNAMIC

#define UVC_ENCODER_FBC_WIDTH_ALIGN_VAL (256)
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

#define COLOR_COUNT 6

extern struct uvc_device* udev[MAX_UVC_CAMERAS];

typedef struct stYUVColor_ {
    AX_U8 y;
    AX_U8 u;
    AX_U8 v;
}stYUVColor;

typedef enum _SAMPLE_JPEGENC_RCMODE
{
      JPEGENC_RC_NONE,
      JPEGENC_CBR = 1,
      JPEGENC_VBR = 2,
      JPEGENC_FIXQP,
      JPEGENC_RC_BUTT
} SAMPLE_JPEGENC_RCMODE;

typedef enum {
    UVC_SYS_CASE_NONE  = -1,
    UVC_SYS_CASE_SINGLE_OS08A20  = 0,
    UVC_SYS_CASE_DUAL_OS08A20 = 1,
    UVC_SYS_CASE_SINGLE_DUMMY = 2,
    UVC_SYS_CASE_DUAL_DUMMY = 3,
    UVC_SYS_CASE_BUTT
} UVC_SYS_CASE_E;

typedef struct _SAMPLE_JENC_GETSTREAM_PARA
{
    AX_BOOL bThreadStart;
    VENC_CHN VeChn;
    AX_MOD_INFO_S ispMod;
    AX_MOD_INFO_S jencMod;
    AX_PAYLOAD_TYPE_E enCodecFormat;
} SAMPLE_JENC_GETSTREAM_PARA_T;

typedef enum RcType_
{
    VENC_RC_NONE = -1,
    VENC_RC_CBR = 1,
    VENC_RC_VBR = 2,
    VENC_RC_AVBR = 3,
    VENC_RC_QPMAP = 4,
    VENC_RC_FIXQP = 5,
    VENC_RC_MAX
}RcType;

typedef struct _stRCInfo
{
    RcType eRCType;
    AX_U32 nMinQp;
    AX_U32 nMaxQp;
    AX_U32 nMinIQp;
    AX_U32 nMaxIQp;
    AX_U32 nMaxIProp;
    AX_U32 nMinIProp;
    AX_S32 nIntraQpDelta;
} RC_INFO_T;

typedef struct _stVideoConfig
{
    AX_PAYLOAD_TYPE_E ePayloadType;
    AX_U32 nGOP;
    AX_F32 nSrcFrameRate;
    AX_F32 nDstFrameRate;
    AX_U32 nStride;
    AX_S32 nInWidth;
    AX_S32 nInHeight;
    AX_S32 nOutWidth;
    AX_S32 nOutHeight;
    AX_S32 nOffsetCropX;
    AX_S32 nOffsetCropY;
    AX_S32 nOffsetCropW;
    AX_S32 nOffsetCropH;
    AX_IMG_FORMAT_E eImgFormat;
    RC_INFO_T stRCInfo;
    AX_S32 nBitrate;
} VIDEO_CONFIG_T;

typedef struct _UVC_VIN_PARAM_T{
    UVC_SYS_CASE_E eSysCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bAiispEnable;
    AX_S32 nDumpFrameNum;
} UVC_VIN_PARAM_T;

static const AX_U8 QTableLuminance[64] = {
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1};

static const AX_U8 QTableChrominance[64] = {
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1};

AX_S32 video_init(struct uvc_device** dev, UVC_SYS_CASE_E sys_case, COMMON_SYS_ARGS_T sys_arg, COMMON_VIN_MODE_E vin_mode,\
                    AX_SNS_HDR_MODE_E hdr_mode, SAMPLE_SNS_TYPE_E sns_type, AX_S32 isp_tuning, AX_S32 aiisp_enable);
AX_VOID video_deinit(AX_S32 isp_tuning);
AX_S32 init_sys_config(UVC_SYS_CASE_E sys_case, COMMON_SYS_ARGS_T* sys_arg, COMMON_SYS_ARGS_T* priv_pool_arg, COMMON_VIN_MODE_E vin_mode,\
                        AX_SNS_HDR_MODE_E hdr_mode, SAMPLE_SNS_TYPE_E sns_type, AX_S32 aiisp_enable);

AX_S32 jenc_chn_attr_init(struct uvc_device* dev, AX_S32 input_width, AX_S32 input_height, AX_IMG_FORMAT_E fomat,\
                            AX_S32 output_width, AX_S32 output_height, AX_U32 enc_chn_id,\
                            AX_U32 bitrate);

AX_S32 set_rc_param(AX_U32 chn, AX_PAYLOAD_TYPE_E type, AX_U32 bitrate);
AX_S32 set_jpeg_param(AX_U32 chn);

AX_S32 venc_init(AX_S32 enc_format);
AX_S32 venc_deinit(AX_VOID);

AX_S32 venc_chn_attr_init(struct uvc_device* dev, AX_S32 input_width, AX_S32 input_height, AX_IMG_FORMAT_E fomat,\
                            AX_S32 output_width, AX_S32 output_height, AX_U32 enc_chn_id,\
                            AX_PAYLOAD_TYPE_E payload_type, AX_U32 bitrate);
AX_S32 venc_chn_deinit(struct uvc_device* dev, AX_S32 enc_format);

AX_VOID *venc_get_stream(AX_VOID *arg);

AX_S32 link_vin_venc_mod(struct uvc_device* dev);
AX_S32 unlink_vin_venc_mod(struct uvc_device* dev);

AX_S32 uncompressed_chn_init(struct uvc_device* dev, AX_S32 input_width, AX_S32 input_height, AX_IMG_FORMAT_E src_format,\
                                AX_IMG_FORMAT_E dst_format, AX_S32 output_width,\
                                AX_S32 output_height, AX_U32 enc_chn_id);
AX_VOID uncompressed_chn_deinit(struct uvc_device* dev);

AX_VOID *get_uncompressed_stream(AX_VOID *arg);

AX_S32 uvc_set_chn_attr(AX_U8 pipe, AX_VIN_CHN_ID_E chn, AX_VIN_CHN_ATTR_T *chn_attr);
AX_S32 uvc_get_chn_attr(AX_U8 pipe, AX_VIN_CHN_ID_E chn, AX_VIN_CHN_ATTR_T *chn_attr);

#ifdef SUPPORT_DSP_CSC
AX_S32 uvc_init_dsp(AX_DSP_ID_E dsp_id, const AX_CHAR* itcm, const AX_CHAR* sram);
AX_S32 uvc_deinit_dsp(AX_DSP_ID_E dsp_id);
#else
AX_S32 uvc_nv12_yuv422(AX_U32 width, AX_U32 height, AX_U8* src_img, AX_U8* dst_img);
#endif

AX_S32 uvc_draw_yuv422_color_stripe(AX_S32 width, AX_S32 height, AX_U8* img);
#endif
