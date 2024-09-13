/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_BUFFER_TOOL_H_
#define _AX_BUFFER_TOOL_H_

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_vdec_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

#define AX_COMM_ALIGN(value, n) (((value) + (n) - 1) & ~((n) - 1))

static __inline AX_U32 AX_VIN_GetImgBufferSize(AX_U32 uHeight, AX_U32 uWidth, AX_IMG_FORMAT_E eImageFormat,
        AX_FRAME_COMPRESS_INFO_T *pstCompressInfo, AX_U32 uAlignSize)
{
    AX_U32 uWidthBeat = 0;
    AX_U32 uBufSize = 0;
    AX_U32 uPixBits = 0;
    AX_BOOL bFormatYuv = AX_TRUE;

    switch (eImageFormat) {
    case AX_FORMAT_BAYER_RAW_8BPP:
        uPixBits = 8;
        break;

    case AX_FORMAT_BAYER_RAW_10BPP_PACKED:
        uPixBits = 10;
        break;
    case AX_FORMAT_BAYER_RAW_12BPP_PACKED:
        uPixBits = 12;
        break;
    case AX_FORMAT_BAYER_RAW_14BPP_PACKED:
        uPixBits = 14;
        break;
    case AX_FORMAT_BAYER_RAW_10BPP:
    case AX_FORMAT_BAYER_RAW_12BPP:
    case AX_FORMAT_BAYER_RAW_14BPP:
    case AX_FORMAT_BAYER_RAW_16BPP:
        uPixBits = 16;
        break;
    case AX_FORMAT_YUV400:
    case AX_FORMAT_YUV420_PLANAR:
    case AX_FORMAT_YUV420_PLANAR_VU:
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
    case AX_FORMAT_YUV422_PLANAR:
    case AX_FORMAT_YUV422_PLANAR_VU:
    case AX_FORMAT_YUV422_SEMIPLANAR:
    case AX_FORMAT_YUV422_SEMIPLANAR_VU:
    case AX_FORMAT_YUV422_INTERLEAVED_YUVY:
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
    case AX_FORMAT_YUV422_INTERLEAVED_VYUY:
    case AX_FORMAT_YUV444_PLANAR:
    case AX_FORMAT_YUV444_PLANAR_VU:
    case AX_FORMAT_YUV444_SEMIPLANAR:
    case AX_FORMAT_YUV444_SEMIPLANAR_VU:
    case AX_FORMAT_YUV444_PACKED:
        uPixBits = 8;
        break;
    case AX_FORMAT_YUV400_10BIT:
    case AX_FORMAT_YUV420_PLANAR_10BIT_UV_PACKED_4Y5B:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P101010:
    case AX_FORMAT_YUV444_PACKED_10BIT_P101010:
        uPixBits = 10;
        break;
    case AX_FORMAT_YUV420_PLANAR_10BIT_I010:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P016:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_I016:
    case AX_FORMAT_YUV444_PACKED_10BIT_P010:
        uPixBits = 16;
        break;
    default:
        uPixBits = 8;
        break;
    }

    if (pstCompressInfo->enCompressMode != AX_COMPRESS_MODE_NONE) {
        switch (eImageFormat) {
        case AX_FORMAT_BAYER_RAW_8BPP:
            uPixBits = 8;
            break;
        case AX_FORMAT_BAYER_RAW_10BPP:
        case AX_FORMAT_BAYER_RAW_10BPP_PACKED:
            uPixBits = 10;
            break;
        case AX_FORMAT_BAYER_RAW_12BPP:
        case AX_FORMAT_BAYER_RAW_12BPP_PACKED:
            uPixBits = 12;
            break;

        case AX_FORMAT_BAYER_RAW_14BPP:
        case AX_FORMAT_BAYER_RAW_14BPP_PACKED:
            uPixBits = 14;
            break;

        case AX_FORMAT_BAYER_RAW_16BPP:
            uPixBits = 16;
            break;
        default:
            break;
        }
    }

    if (((uWidth * uPixBits) % 128) != 0) {
        uWidthBeat = ((uWidth * uPixBits) / 128) + 1;
    } else {
        uWidthBeat = (uWidth * uPixBits) / 128 ;
    }

    if (uAlignSize != 0) {
        uWidthBeat = ((uWidthBeat * 16 * uAlignSize) + uAlignSize - 1) / uAlignSize;
        uWidthBeat = uWidthBeat / 16;
    } else {
        uWidthBeat = ((uWidthBeat * 16 * 128) + 127) / 128;
        uWidthBeat = uWidthBeat / 16;
    }

    // calc dma buffer size
    switch (eImageFormat) {
    case AX_FORMAT_BAYER_RAW_8BPP:
    case AX_FORMAT_BAYER_RAW_10BPP:
    case AX_FORMAT_BAYER_RAW_12BPP:
    case AX_FORMAT_BAYER_RAW_14BPP:
    case AX_FORMAT_BAYER_RAW_16BPP:
    case AX_FORMAT_BAYER_RAW_10BPP_PACKED:
    case AX_FORMAT_BAYER_RAW_12BPP_PACKED:
    case AX_FORMAT_BAYER_RAW_14BPP_PACKED:
        uBufSize = uWidthBeat * 16 * uHeight;
        bFormatYuv = AX_FALSE;
        break;
    case AX_FORMAT_YUV400:
    case AX_FORMAT_YUV400_10BIT:
        uBufSize = uWidthBeat * 16 * uHeight;
        break;
    case AX_FORMAT_YUV420_PLANAR:
    case AX_FORMAT_YUV420_PLANAR_VU:
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
    case AX_FORMAT_YUV420_PLANAR_10BIT_UV_PACKED_4Y5B:
    case AX_FORMAT_YUV420_PLANAR_10BIT_I010:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P101010:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P016:
    case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_I016:
        uBufSize = uWidthBeat * 16 * uHeight * 3 / 2;
        break;
    case AX_FORMAT_YUV422_PLANAR:
    case AX_FORMAT_YUV422_PLANAR_VU:
    case AX_FORMAT_YUV422_SEMIPLANAR:
    case AX_FORMAT_YUV422_SEMIPLANAR_VU:
    case AX_FORMAT_YUV422_INTERLEAVED_YUVY:
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
    case AX_FORMAT_YUV422_INTERLEAVED_VYUY:
        uBufSize = uWidthBeat * 16 * uHeight * 2;
        break;
    case AX_FORMAT_YUV444_PLANAR:
    case AX_FORMAT_YUV444_PLANAR_VU:
    case AX_FORMAT_YUV444_SEMIPLANAR:
    case AX_FORMAT_YUV444_SEMIPLANAR_VU:
    case AX_FORMAT_YUV444_PACKED:
    case AX_FORMAT_YUV444_PACKED_10BIT_P010:
    case AX_FORMAT_YUV444_PACKED_10BIT_P101010:
        uBufSize = uWidthBeat * 16 * uHeight * 3;
        break;
    default:
        uBufSize = uWidthBeat * 16 * uHeight * 3 / 2;
        break;
    }

    if (pstCompressInfo != AX_NULL) {
        if (pstCompressInfo->enCompressMode == AX_COMPRESS_MODE_LOSSLESS) {
            if (bFormatYuv == AX_TRUE) {
                uBufSize += uHeight * 96; /* add the header buf for compress data */
            } else {
                uBufSize += uHeight * 32; /* add the header buf for compress data */
            }
        } else if (pstCompressInfo->enCompressMode == AX_COMPRESS_MODE_LOSSY) {
            if (bFormatYuv == AX_TRUE) {
                if ((0 != pstCompressInfo->u32CompressLevel) && (10 >= pstCompressInfo->u32CompressLevel)) {
                    if (8 == uPixBits) {
                        if (pstCompressInfo->u32CompressLevel <= 8) {
                            uBufSize = uBufSize * pstCompressInfo->u32CompressLevel * 0.125;
                        }
                    } else if (10 == uPixBits) {
                        uBufSize = uBufSize * pstCompressInfo->u32CompressLevel * 0.1;
                    } else {
                        //do nothing
                    }
                }
            }
        } else {
            //without fbc
        }
    }

    return uBufSize;
}

/*
 * PT_H264 & PT_H265 width force 256 align
 * PT_JPEG & PT_MJPEG width force 64 align and height force 2 align
 */
static __inline AX_U32 AX_VDEC_GetPicBufferSize(AX_U32 uWidth, AX_U32 uHeight, AX_IMG_FORMAT_E eOutputFormat,
        AX_FRAME_COMPRESS_INFO_T *pstCompressInfo, AX_PAYLOAD_TYPE_E enType)
{
    AX_U32 picSizeInMbs = 0;
    AX_U32 picSize = 0;
    AX_U32 dmvMemSize = 0;
    AX_U32 refBuffSize = 0;
    AX_U32 uPixBits = 8;
    AX_U32 uHeightAlign = 0;
    AX_U32 uWidthAlign = 0;
    AX_U32 uAlignSize = (1 << (8));
    AX_U32 ax_fbc_tile128x2_size[AX_VDEC_FBC_COMPRESS_LEVEL_MAX] =
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288};


    // picSizeInMbs = (AX_COMM_ALIGN(uHeight, 16) >> 4) * (AX_COMM_ALIGN(uWidth, 16) >> 4);
    if ((PT_H264 == enType) || (PT_H265 == enType)) {
        switch (eOutputFormat) {
        case AX_FORMAT_YUV400:
        case AX_FORMAT_YUV420_PLANAR:
        case AX_FORMAT_YUV420_PLANAR_VU:
        case AX_FORMAT_YUV420_SEMIPLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        case AX_FORMAT_YUV422_PLANAR:
        case AX_FORMAT_YUV422_PLANAR_VU:
        case AX_FORMAT_YUV422_SEMIPLANAR:
        case AX_FORMAT_YUV422_SEMIPLANAR_VU:
        case AX_FORMAT_YUV422_INTERLEAVED_YUVY:
        case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
        case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        case AX_FORMAT_YUV422_INTERLEAVED_VYUY:
        case AX_FORMAT_YUV444_PLANAR:
        case AX_FORMAT_YUV444_PLANAR_VU:
        case AX_FORMAT_YUV444_SEMIPLANAR:
        case AX_FORMAT_YUV444_SEMIPLANAR_VU:
        case AX_FORMAT_YUV444_PACKED:
            uPixBits = 8;
            break;
        case AX_FORMAT_YUV400_10BIT:
        case AX_FORMAT_YUV420_PLANAR_10BIT_UV_PACKED_4Y5B:
        case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P101010:
        case AX_FORMAT_YUV444_PACKED_10BIT_P101010:
            uPixBits = 10;
            break;
        case AX_FORMAT_YUV420_PLANAR_10BIT_I010:
        case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010:
        case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P016:
        case AX_FORMAT_YUV420_SEMIPLANAR_10BIT_I016:
        case AX_FORMAT_YUV444_PACKED_10BIT_P010:
            uPixBits = 16;
            break;
        default:
            uPixBits = 8;
            break;
        }

        /* luma */
        uHeightAlign = AX_COMM_ALIGN(uHeight, 2);
        uWidthAlign = AX_COMM_ALIGN(uWidth, uAlignSize);
        if (pstCompressInfo && (pstCompressInfo->enCompressMode == AX_COMPRESS_MODE_LOSSY)) {
            AX_U32 ax_tile128x2_size = 128 * 2 * uPixBits / 8;

            picSize = uWidthAlign * uHeightAlign
                      * ax_fbc_tile128x2_size[pstCompressInfo->u32CompressLevel] / ax_tile128x2_size;
        } else {
            picSize = uHeightAlign * uWidthAlign;
        }

        /* chroma */
        if (eOutputFormat != AX_FORMAT_YUV400) {
            uHeightAlign = uHeightAlign >> 1;
            if (pstCompressInfo && (pstCompressInfo->enCompressMode == AX_COMPRESS_MODE_LOSSY)) {
                AX_U32 ax_tile128x2_size = 128 * 2 * uPixBits / 8;

                picSize += uWidthAlign * uHeightAlign
                           * ax_fbc_tile128x2_size[pstCompressInfo->u32CompressLevel] / ax_tile128x2_size;
            } else {
                picSize += uWidthAlign * uHeightAlign;
            }
        }

        /* buffer size of dpb pic = picSize + dir_mv_size + tbl_size */
        dmvMemSize = picSizeInMbs * 64;
        refBuffSize = picSize  + dmvMemSize + 32;
    } else if ((PT_JPEG == enType) || (PT_MJPEG == enType)) {
        picSize = (AX_COMM_ALIGN(uHeight, 2) * AX_COMM_ALIGN(uWidth, 64) * 3) >> 1;
        refBuffSize = picSize;
    } else {
        refBuffSize = -1;
    }

    return refBuffSize;
}



#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif //_AX_BUFFER_TOOL_H_
