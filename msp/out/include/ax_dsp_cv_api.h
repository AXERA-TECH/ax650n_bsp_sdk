/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_DSP_CV_API_H_
#define _AX_DSP_CV_API_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <ax_dsp_api.h>

//These CMDs are follow in AX_DSP_CMD_E
#define AX_DSP_CMD_OPERATOR_CVTCOLOR (AX_DSP_CMD_OPERATOR + 2)
#define AX_DSP_CMD_OPERATOR_JOINT_LR (AX_DSP_CMD_OPERATOR + 3)
#define AX_DSP_CMD_OPERATOR_SAD (AX_DSP_CMD_OPERATOR + 4)
#define AX_DSP_CMD_OPERATOR_KVM_SPLIT (AX_DSP_CMD_OPERATOR + 5)
#define AX_DSP_CMD_OPERATOR_KVM_COMBINE (AX_DSP_CMD_OPERATOR + 6)
#define AX_DSP_CMD_OPERATOR_MAP (AX_DSP_CMD_OPERATOR + 7)
#define AX_DSP_CMD_OPERATOR_NV12COPY (AX_DSP_CMD_OPERATOR + 8)
#define AX_DSP_CMD_OPERATOR_NV12Blending (AX_DSP_CMD_OPERATOR + 9)
#define AX_DSP_CMD_OPERATOR_COPY (AX_DSP_CMD_OPERATOR + 10)

typedef struct {
    AX_U64 u64PhyAddr;  // the physical address of the memory
    AX_U64 u64VirAddr;  // the virtual address of the memory
    AX_U32 u32Size;     // the size of memory
} AX_MEM_INFO_T;

typedef enum {
    U8_TO_U8,
    U8_TO_S16,
    U8_TO_U16,
    U8_TO_S32,
    S16_TO_U8,
    S16_TO_S16,
    S16_TO_U16,
    S16_TO_S32,
    U16_TO_U16,
    U16_TO_S32,
    S32_TO_S32,
} AX_DSP_CV_DTYPE_E;

typedef struct {
    int format;
    int src_height;
    int src_width;
    int src_stride;
    int dst_height;
    int dst_width;
    int dst_stride;
    int inter_type;
} AX_DSP_CV_RESIZE_PARAM_T;

typedef enum {
    AX_DSP_CV_ERR_NODSPID = -1000,
    AX_DSP_CV_ERR_NOTREADY,
    AX_DSP_CV_ERR_NOMEM,
    AX_DSP_CV_ERR_PROC,
    AX_DSP_CV_ERR_QUERY,
    AX_DSP_CV_ERR_MEMOP,
    AX_DSP_CV_ERR_OPER,
    AX_DSP_CV_ERR_UNSUPPORT,
} AX_DSP_CV_ERR_E;

typedef enum {
    AX_DSP_CV_FORMAT_NV12,
    AX_DSP_CV_FORMAT_Y8,
    AX_DSP_CV_FORMAT_UV8,
} AX_DSP_CV_FORMAT_E;

enum {
    AX_DSP_CV_RESIZE_INNER_TYPE_BILINEAR,
    AX_DSP_CV_RESIZE_INNER_TYPE_NEAREST,
    AX_DSP_CV_RESIZE_INNER_TYPE_BUTT,
};

typedef struct {
    int src_height;
    int src_width;
    int src_stride;
    int src_stride_uv;
    int dst_stride;
} AX_DSP_CV_CVTCOLOR_PARAM_T;

typedef enum {
    AX_DSP_CV_CVTCOLOR_NV12_YUYV,
    AX_DSP_CV_CVTCOLOR_YUV444_RGBX,
    AX_DSP_CV_CVTCOLOR_FORMAT_BUTT,
} AX_DSP_CV_CVTCOLOR_FORMAT_E;

typedef struct {
    AX_U64 phySrcL;
    AX_U64 phySrcR;
    AX_U64 phyDst;
    int width_l;
    int width_r;
    int height;
    int src_stride_l;
    int src_stride_r;
    int dst_stride;
} AX_DSP_CV_JOINTLR_PARAM_T;

typedef struct {
    AX_U64 phySrcL;
    AX_U64 phySrcR;
    AX_U64 phyDst;
    int width;
    int height;
    int src_stride_l;
    int src_stride_r;
    int dst_stride;
    int kernel_size;
    int range;
} AX_DSP_CV_SAD_PARAM_T;

typedef struct {
    AX_U64 phySrc;
    AX_U64 phyMap;
    AX_U64 phyDst;
    int width;
    int height;
    int src_stride;
    int dst_stride;
    int para;
} AX_DSP_CV_MAP_PARAM_T;

typedef struct {
    AX_U64 phySrc_y;
    AX_U64 phySrc_uv;
    AX_U64 phyDst_y;
    AX_U64 phyDst_uv;
    int src_height;
    int src_stride;
    int dst_width;
    int dst_height;
    int dst_stride;
    int src_x; //x should be even
    int src_y;
    int win_w;
    int win_h;
    int dst_x; //x should be even
    int dst_y;
    int para;
} AX_DSP_CV_NV12COPY_PARAM_T;

typedef struct {
    AX_U64 phySrc_NV12;
    AX_U64 phySrc_RGBA;
    AX_U64 phyDst;
    int width;
    int height;
    int src_stride_NV12;
    int src_stride_RGBA;
    int dst_stride;
    int src_x; //x should be even
    int src_y; //y should be even
    int win_w; //should be even
    int win_h; //should be even
    int para; //0:rgba
} AX_DSP_CV_NV12Blending_PARAM_T;

typedef struct {
    AX_U64 phySrc;
    AX_U64 phyDst;
    int src_stride;
    int dst_stride;
    int win_w;
    int win_h;
    int para;
} AX_DSP_CV_COPY_PARAM_T;

typedef struct {
    AX_U64 phySrc;  //RGB888|YUV444
    AX_U64 phyDstL; //YV NV12
    AX_U64 phyDstR; //UV NV12
    int width;
    int height;
    int src_stride;
    int dst_stride_l;
    int dst_stride_r;
    int para; //RGB888: 0 for all;1 for upper;2 for bottom //YUV444: use 3 -> 5 respectively
} AX_DSP_CV_SPLIT_YUV444_PARAM_T;
int AX_DSP_CV_SPLIT_YUV444(int dsp_id, AX_DSP_CV_SPLIT_YUV444_PARAM_T *param);
int AX_DSP_CV_COMBINE_YUV444(int dsp_id, AX_DSP_CV_SPLIT_YUV444_PARAM_T *param);
int AX_DSP_CV_COMBINE_YUV444_2(AX_DSP_CV_SPLIT_YUV444_PARAM_T *param);
int AX_DSP_CV_SPLIT_YUV444_2(AX_DSP_CV_SPLIT_YUV444_PARAM_T *param);

int AX_DSP_CV_Init(int dsp_id);
int AX_DSP_CV_ResizeAll(int dsp_id, AX_MEM_INFO_T in_buf[3], AX_MEM_INFO_T out_buf[3], AX_DSP_CV_RESIZE_PARAM_T *param);
int AX_DSP_CV_Resize(int dsp_id, AX_MEM_INFO_T *in_buf, AX_MEM_INFO_T *out_buf, AX_DSP_CV_RESIZE_PARAM_T *param);
int AX_DSP_CV_ResizeUV(int dsp_id, AX_MEM_INFO_T *in_buf, AX_MEM_INFO_T *out_buf, AX_DSP_CV_RESIZE_PARAM_T *param);
int AX_DSP_CV_CvtColor(int dsp_id, int format, AX_MEM_INFO_T in_buf[3], AX_MEM_INFO_T out_buf[3],
                       AX_DSP_CV_CVTCOLOR_PARAM_T *param);
int AX_DSP_CV_JointLR(int dsp_id, AX_DSP_CV_JOINTLR_PARAM_T *param);
int AX_DSP_CV_SAD(int dsp_id, AX_DSP_CV_SAD_PARAM_T *param);
int AX_DSP_CV_MAP(int dsp_id, AX_DSP_CV_MAP_PARAM_T *param);
int AX_DSP_CV_NV12COPY(int dsp_id, AX_DSP_CV_NV12COPY_PARAM_T *param);
int AX_DSP_CV_NV12Blending(int dsp_id, AX_DSP_CV_NV12Blending_PARAM_T *param);
int AX_DSP_CV_COPY(int dsp_id, AX_DSP_CV_COPY_PARAM_T *param);
int AX_DSP_CV_Exit(int dsp_id);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_AX_SVP_DSP_CV_H_*/

