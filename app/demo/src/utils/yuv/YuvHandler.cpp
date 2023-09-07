/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "YuvHandler.hpp"
#include <stdlib.h>
#include <string.h>
#include <fstream>
using namespace std;

///
static const struct axYUV_COLOR_T {
    AX_U8 Y;
    AX_U8 U;
    AX_U8 V;
} YUV_COLORS[CYuvHandler::YUV_COLOR_MAX] = {
    {0x00, 0x00, 0x00},  // green
    {0x00, 0x00, 0xff},  // red
    {0x00, 0xff, 0x00},  // blue
    {0x00, 0xff, 0xff},  // purple
    {0xff, 0x00, 0x00},  // dark green
    {0xff, 0x00, 0xff},  // yellow
    {0xff, 0xff, 0x00},  // light blue
    {0xff, 0xff, 0xff},  // light purple
    {0x00, 0x80, 0x80},  // dark black
    {0x80, 0x80, 0x80},  // gray
    {0xff, 0x80, 0x80}   // white
};

//////////////////////////////////////////////////////////////////////////
CYuvHandler::CYuvHandler(AX_VOID)
    : m_pImgData(nullptr), m_bCopy(AX_FALSE), m_nWidth(0), m_nHeight(0), m_nStride(0), m_nImgSize(0), m_eType(AX_FORMAT_YUV420_SEMIPLANAR) {
}

CYuvHandler::CYuvHandler(const AX_U8 *pImgData, AX_U16 nWidth, AX_U16 nHeight, AX_IMG_FORMAT_E eType /* = AX_FORMAT_YUV420_SEMIPLANAR */,
                         AX_U16 nStride /* = 0*/, AX_BOOL bMemCopy /* = AX_FALSE */)
    : m_pImgData(const_cast<AX_U8 *>(pImgData)),
      m_bCopy(bMemCopy),
      m_nWidth(nWidth),
      m_nHeight(nHeight),
      m_nStride((0 == nStride) ? nWidth : nStride),
      m_eType(eType) {
    m_nImgSize = CalcImgSize(m_nWidth, nHeight, eType, m_nStride);
    if (bMemCopy) {
        m_pImgData = (AX_U8 *)::malloc(sizeof(AX_U8) * m_nImgSize);
        if (m_pImgData) {
            memcpy(m_pImgData, pImgData, m_nImgSize);
        }
    }
}

CYuvHandler::~CYuvHandler(AX_VOID) {
    FreeImage();
}

AX_VOID CYuvHandler::FreeImage(AX_VOID) {
    if (m_bCopy && m_pImgData) {
        ::free(m_pImgData);
        m_pImgData = nullptr;
    }
}

const AX_U8 *CYuvHandler::GetImageData(AX_VOID) const {
    return m_pImgData;
}

const AX_U32 CYuvHandler::GetImageSize(AX_VOID) const {
    return m_nImgSize;
}

const AX_U8 *CYuvHandler::LoadImage(const AX_CHAR *pImgFilePath, AX_U16 nWidth, AX_U16 nHeight,
                                    AX_IMG_FORMAT_E eType /* = AX_FORMAT_YUV420_SEMIPLANAR */, AX_U16 nStride /* = 0 */) {
    FreeImage();

    m_bCopy = AX_TRUE;
    m_nWidth = nWidth;
    m_nStride = (0 == nStride) ? nWidth : nStride;
    m_nHeight = nHeight;
    m_eType = eType;
    m_nImgSize = CalcImgSize(m_nWidth, nHeight, eType, m_nStride);
    ifstream f(pImgFilePath, ios::in | ios::binary);
    if (f) {
        m_pImgData = (AX_U8 *)malloc(sizeof(AX_U8) * m_nImgSize);
        if (m_pImgData) {
            f.read((char *)m_pImgData, m_nImgSize);
            if (!f) {
                free(m_pImgData);
                m_pImgData = nullptr;
            }
        }
        f.close();
    }

    return m_pImgData;
}

AX_U32 CYuvHandler::CalcImgSize(AX_U16 nWidth, AX_U16 nHeight, AX_IMG_FORMAT_E eImgType /* = AX_FORMAT_YUV420_SEMIPLANAR */,
                                AX_U16 nStride /* = 0 */) {
    AX_U32 nBpp = 0;
    if (nWidth == 0 || nHeight == 0) {
        return 0;
    }

    if (0 == nStride) {
        nStride = nWidth;
    }

    switch (eImgType) {
        case AX_FORMAT_YUV420_PLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR_VU:
            nBpp = 12;
            break;
        case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
        case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
            nBpp = 16;
            break;
        case AX_FORMAT_YUV444_PACKED:
            nBpp = 24;
            break;
        default:
            nBpp = 0;
            break;
    }

    return nStride * nHeight * nBpp / 8;
}

AX_VOID CYuvHandler::DrawPoint(AX_U8 *y, AX_U8 *u, AX_U8 *v, AX_U16 x0, AX_U16 y0, YUV_COLOR eColor) {
    AX_U32 y_offset = 0;
    AX_U32 u_offset = 0;
    AX_U32 v_offset = 0;
    switch (m_eType) {
        case AX_FORMAT_YUV420_PLANAR:  // YUV420 I420
            /* YYYY...UUUU...VVVV */
            y_offset = (AX_U32)(y0 * m_nStride + x0);
            u_offset = (AX_U32)((y0 / 2) * (m_nStride / 2) + x0 / 2);
            v_offset = u_offset;

            y[y_offset] = YUV_COLORS[eColor].Y;
            u[u_offset] = YUV_COLORS[eColor].U;
            v[v_offset] = YUV_COLORS[eColor].V;
            break;

        case AX_FORMAT_YUV420_SEMIPLANAR:  // YUV420SP NV12
            /* YYYY...UVUV */
            y_offset = (AX_U32)(y0 * m_nStride + x0);
            u_offset = (AX_U32)((y0 / 2) * m_nStride + x0 / 2 * 2);
            v_offset = u_offset + 1;

            if (YUV_COLORS[eColor].Y == 0xFF) {
                y[y_offset] = YUV_COLORS[eColor].Y;
            } else {
                u[u_offset] = YUV_COLORS[eColor].U;
                v[v_offset] = YUV_COLORS[eColor].V;
            }
            break;

        case AX_FORMAT_YUV420_SEMIPLANAR_VU:  // YUV420SP NV21
            /* YYYY...VUVU */
            y_offset = (AX_U32)(y0 * m_nStride + x0);
            v_offset = (AX_U32)((y0 / 2) * m_nStride + x0 / 2 * 2);
            u_offset = v_offset + 1;

            y[y_offset] = YUV_COLORS[eColor].Y;
            u[u_offset] = YUV_COLORS[eColor].U;
            v[v_offset] = YUV_COLORS[eColor].V;
            break;

        case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
            /* UYVYUYVY */
            u_offset = (AX_U32)(y0 * m_nStride * 2 + x0 / 2 * 4);
            v_offset = u_offset + 2;
            y_offset = u_offset + 1;
            y[y_offset] = YUV_COLORS[eColor].Y;
            y[y_offset + 2] = YUV_COLORS[eColor].Y;
            y[u_offset] = YUV_COLORS[eColor].U;
            y[v_offset] = YUV_COLORS[eColor].V;
            break;

        case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
            /* YUYVYUYV */
            y_offset = (AX_U32)(y0 * m_nStride * 2 + x0 / 2 * 4);
            u_offset = y_offset + 1;
            v_offset = u_offset + 2;

            y[y_offset] = YUV_COLORS[eColor].Y;
            y[y_offset + 2] = YUV_COLORS[eColor].Y;
            y[u_offset] = YUV_COLORS[eColor].U;
            y[v_offset] = YUV_COLORS[eColor].V;
            break;

        default:
            break;
    }
}

AX_VOID CYuvHandler::DrawPoint(AX_S16 x, AX_S16 y, AX_U8 nScale /* = 1*/, AX_S16 xOffset /* = 0*/, AX_S16 yOffset /* = 0*/,
                               YUV_COLOR eColor /* = YUV_GREEN*/) {
    if (!m_pImgData) {
        return;
    }

    AX_U8 *pY = nullptr;
    AX_U8 *pU = nullptr;
    AX_U8 *pV = nullptr;
    switch (m_eType) {
        case AX_FORMAT_YUV420_PLANAR:
            pY = m_pImgData;
            pU = m_pImgData + m_nStride * m_nHeight;
            pV = pU + m_nStride * m_nHeight / 4;
            break;
        case AX_FORMAT_YUV420_SEMIPLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR_VU:
            pY = m_pImgData;
            pU = m_pImgData + m_nStride * m_nHeight;
            pV = pU;
            break;
        case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
            pY = m_pImgData;
            pU = pY;
            pV = pY;
            break;
        default:
            break;
    }

    AX_S16 nXStart = 0;
    AX_S16 nYStart = 0;
    for (uint32_t hScale = 0; hScale < nScale; hScale++) {
        for (uint32_t wScale = 0; wScale < nScale; wScale++) {
            nXStart = x * nScale + hScale - xOffset;
            nYStart = y * nScale + wScale - yOffset;
            if (nXStart < 0 || nXStart > m_nWidth) {
                break;
            }

            if (nYStart < 0 || nYStart > m_nHeight) {
                break;
            }

            DrawPoint(pY, pU, pV, nXStart, nYStart, eColor);
        }
    }
}

const AX_U8 *CYuvHandler::DrawLine(AX_S16 x0, AX_S16 y0, AX_S16 x1, AX_S16 y1, YUV_COLOR eColor /* = YUV_GREEN*/) {
    if (!m_pImgData) {
        return nullptr;
    }

    x0 = (x0 < 0) ? 0 : x0;
    y0 = (y0 < 0) ? 0 : y0;
    x1 = (x1 < 0) ? 0 : x1;
    y1 = (y1 < 0) ? 0 : y1;

    x0 = (x0 >= m_nWidth) ? m_nWidth - 1 : x0;
    y0 = (y0 >= m_nHeight) ? m_nHeight - 1 : y0;
    x1 = (x1 >= m_nWidth) ? m_nWidth - 1 : x1;
    y1 = (y1 >= m_nHeight) ? m_nHeight - 1 : y1;

    AX_U16 dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
    AX_U16 dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);

    AX_S16 xstep = (x0 < x1) ? 1 : -1;
    AX_S16 ystep = (y0 < y1) ? 1 : -1;
    AX_S16 nstep = 0, eps = 0;

    AX_U8 *pY = nullptr;
    AX_U8 *pU = nullptr;
    AX_U8 *pV = nullptr;
    switch (m_eType) {
        case AX_FORMAT_YUV420_PLANAR:
            pY = m_pImgData;
            pU = m_pImgData + m_nStride * m_nHeight;
            pV = pU + m_nStride * m_nHeight / 4;
            break;
        case AX_FORMAT_YUV420_SEMIPLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR_VU:
            pY = m_pImgData;
            pU = m_pImgData + m_nStride * m_nHeight;
            pV = pU;
            break;
        case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
            pY = m_pImgData;
            pU = pY;
            pV = pY;
            break;
        default:
            break;
    }

    AX_U16 x = x0;
    AX_U16 y = y0;
    if (dx > dy) {
        while (nstep <= dx) {
            DrawPoint(pY, pU, pV, x, y, eColor);
            eps += dy;
            if ((eps << 1) >= dx) {
                y += ystep;
                eps -= dx;
            }
            x += xstep;
            nstep++;
        }
    } else {
        while (nstep <= dy) {
            DrawPoint(pY, pU, pV, x, y, eColor);
            eps += dx;
            if ((eps << 1) >= dy) {
                x += xstep;
                eps -= dy;
            }
            y += ystep;
            nstep++;
        }
    }

    return pY;
}

const AX_U8 *CYuvHandler::DrawRect(AX_S16 x0, AX_S16 y0, AX_U16 w, AX_U16 h, YUV_COLOR eColor /* = YUV_GREEN*/) {
    if (!m_pImgData) {
        return nullptr;
    }

    if (w > 0 && h > 0) {
        DrawLine(x0, y0, x0 + w, y0, eColor);
        DrawLine(x0, y0, x0, y0 + h, eColor);
        DrawLine(x0 + w, y0, x0 + w, y0 + h, eColor);
        DrawLine(x0, y0 + h, x0 + w, y0 + h, eColor);
    }

    return m_pImgData;
}

AX_U32 CYuvHandler::GetClipImage(AX_S16 x0, AX_S16 y0, AX_U16 &w, AX_U16 &h, AX_U8 *pClipImage) {
    AX_U32 clipSize = 0;

    if (!m_pImgData || w == 0 || h == 0) {
        return clipSize;
    }

    x0 = (x0 < 0) ? 0 : x0;
    y0 = (y0 < 0) ? 0 : y0;
    x0 = (x0 + 1) / 2 * 2;
    y0 = (y0 + 1) / 2 * 2;
    /* 8 pixel padding */
    w = ((w % 8) == 0) ? w : ((w & 0xFFF8) + 8);
    h = ((h % 8) == 0) ? h : ((h & 0xFFF8) + 8);

    if (x0 + w > m_nWidth) {
        w = m_nWidth - x0;
        w = (w & 0xFFF8);
    }

    if (y0 + h > m_nHeight) {
        h = m_nHeight - y0;
        h = (h & 0xFFF8);
    }

    clipSize = CalcImgSize(w, h, m_eType);
    if (!pClipImage) {
        return clipSize;
    }

    AX_U8 *pY = nullptr;
    AX_U8 *pV = nullptr;
    AX_U8 *pO = pClipImage;

    switch (m_eType) {
        case AX_FORMAT_YUV420_SEMIPLANAR:
        case AX_FORMAT_YUV420_SEMIPLANAR_VU:
            /* YYYY UVUV or YYYY VUVU */
            /* Copy Y */
            pY = m_pImgData + y0 * m_nStride + x0;
            for (AX_U16 i = 0; i < h; ++i) {
                memcpy(pO, pY, w);
                pY += m_nStride;
                pO += w;
            }

            /* Copy UV */
            pV = m_pImgData + m_nStride * m_nHeight + (y0 * m_nStride / 2 + x0);
            for (AX_U16 i = 0; i < (h / 2); ++i) {
                memcpy(pO, pV, w);
                pV += m_nStride;
                pO += w;
            }
            break;
        default:
            break;
    }

    return clipSize;
}

AX_BOOL CYuvHandler::SaveImage(const AX_CHAR *pFilePath) {
    return SaveImage(pFilePath, m_pImgData, m_nWidth, m_nHeight, m_eType, m_nStride);
}

AX_BOOL CYuvHandler::SaveImage(const AX_CHAR *pFilePath, const AX_U8 *pImgData, AX_U16 nWidth, AX_U16 nHeight,
                               AX_IMG_FORMAT_E eType /* = AX_FORMAT_YUV420_SEMIPLANAR */, AX_S16 nStride /* = 0 */) {
    if (pFilePath && pImgData && nWidth > 0 && nHeight > 0) {
        ofstream f(pFilePath, ios::out | ios::binary);
        if (f) {
            f.write((const char *)pImgData, CalcImgSize(nWidth, nHeight, eType, nStride));
            f.close();
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}
