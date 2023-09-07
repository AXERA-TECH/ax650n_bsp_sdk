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

class CYuvHandler final {
public:
    enum YUV_COLOR {
        YUV_GREEN = 0,
        YUV_RED,
        YUV_BLUE,
        YUV_PURPLE,
        YUV_DARK_GREEN,
        YUV_YELLOW,
        YUV_LIGHT_BLUE,
        YUV_LIGHT_PURPLE,
        YUV_DARK_BLACK,
        YUV_GRAY,
        YUV_WHITE,
        YUV_COLOR_MAX,
    };

    CYuvHandler(AX_VOID);
    CYuvHandler(const AX_U8 *pImgData, AX_U16 nWidth, AX_U16 nHeight, AX_IMG_FORMAT_E eType = AX_FORMAT_YUV420_SEMIPLANAR,
                AX_U16 nStride = 0, AX_BOOL bMemCopy = AX_FALSE);
    ~CYuvHandler(AX_VOID);

    /* load and save YUV image data */
    AX_U32 CalcImgSize(AX_U16 nWidth, AX_U16 nHeight, AX_IMG_FORMAT_E eImgType = AX_FORMAT_YUV420_SEMIPLANAR, AX_U16 nStride = 0);

    const AX_U8 *LoadImage(const AX_CHAR *pFilePath, AX_U16 nWidth, AX_U16 nHeight, AX_IMG_FORMAT_E eType = AX_FORMAT_YUV420_SEMIPLANAR,
                           AX_U16 nStride = 0);

    AX_BOOL SaveImage(const AX_CHAR *pFilePath);
    AX_BOOL SaveImage(const AX_CHAR *pFilePath, const AX_U8 *pImgData, AX_U16 nWidth, AX_U16 nHeight,
                      AX_IMG_FORMAT_E eType = AX_FORMAT_YUV420_SEMIPLANAR, AX_S16 nStride = 0);

    const AX_U8 *GetImageData(AX_VOID) const;
    const AX_U32 GetImageSize(AX_VOID) const;

    /* Clip ROI Image, if pClipImage is nullptr, return real ROI size */
    AX_U32 GetClipImage(AX_S16 x0, AX_S16 y0, AX_U16 &w, AX_U16 &h, AX_U8 *pClipImage);

    const AX_U8 *DrawLine(AX_S16 x0, AX_S16 y0, AX_S16 x1, AX_S16 y1, YUV_COLOR eColor = YUV_GREEN);
    const AX_U8 *DrawRect(AX_S16 x0, AX_S16 y0, AX_U16 w, AX_U16 h, YUV_COLOR eColor = YUV_GREEN);

    AX_VOID DrawPoint(AX_S16 x, AX_S16 y, AX_U8 nScale = 1, AX_S16 xOffset = 0, AX_S16 yOffset = 0, YUV_COLOR eColor = YUV_GREEN);

private:
    AX_VOID DrawPoint(AX_U8 *y, AX_U8 *u, AX_U8 *v, AX_U16 x0, AX_U16 y0, YUV_COLOR eColor);
    AX_VOID FreeImage(AX_VOID);

private:
    AX_U8 *m_pImgData;
    AX_BOOL m_bCopy;
    AX_U16 m_nWidth;
    AX_U16 m_nHeight;
    AX_S16 m_nStride;
    AX_U32 m_nImgSize;
    AX_IMG_FORMAT_E m_eType;
};