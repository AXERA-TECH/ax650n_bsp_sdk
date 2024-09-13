/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "OSDHandler.h"

COSDHandler::COSDHandler() {
}

COSDHandler::~COSDHandler() {
    DeinitOSDHandler();
}

AX_VOID *COSDHandler::GenARGB(wchar_t *pStr, AX_U16 *pArgbBuffer, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight, AX_S16 sX, AX_S16 sY,
                              AX_U16 uFontSize, AX_BOOL bIsBrushSide, AX_U32 uFontColor, AX_U32 uBgColor, AX_U32 uSideColor,
                              OSD_ALIGN_TYPE_E enAlign) {
    AX_S16 x = sX;
    AX_S16 y = sY;

    AX_U16 u16FontColor = 0x0;
    AX_U16 u16BgColor = 0x0;
    AX_U16 u16SideColor = 0x0;
    AX_U8 uColor_H = 0x0;
    AX_U8 uColor_L = 0x0;
    AX_U8 dot8 = 0x0;

    AX_U16 uLen = 0;

    wchar_t *pTextStr = pStr;

    AX_U32 u32ArgbBufferSize = u32OSDHeight * u32OSDWidth * 2;

    if (u32OSDWidth > OSD_PIXEL_MAX_WIDTH || u32OSDWidth <= 0) {
        printf("osd width is out of range.(%d > %d)!\r\n", u32OSDWidth, OSD_PIXEL_MAX_WIDTH);
        return nullptr;
    }

    if (u32OSDHeight > OSD_PIXEL_MAX_HEIGHT || u32OSDHeight <= 0) {
        printf("osd height is out of range. (%d > %d)!\r\n", u32OSDHeight, OSD_PIXEL_MAX_HEIGHT);
        return nullptr;
    }

    if (!pTextStr || !pArgbBuffer) {
        return nullptr;
    }

    uLen = wcslen((wchar_t *)pTextStr);

    u16FontColor = ConvertColor2Argb1555(uFontColor);
    u16BgColor = ConvertColor2Argb1555(uBgColor);
    u16SideColor = ConvertColor2Argb1555(uSideColor);

    uColor_H = (u16BgColor >> 8) & 0xFF;
    uColor_L = (u16BgColor & 0xFF);

    if (uColor_L == uColor_H) {
        memset(pArgbBuffer, uColor_L, u32ArgbBufferSize);
    } else {
        for (AX_U32 i = 0; i < u32ArgbBufferSize / 2; i = i + 1) {
            pArgbBuffer[i] = u16BgColor;
        }
    }

    if (uLen == 0) {
        return nullptr;
    }

    if (x < 0) {
        x = 0;
    }

    if (y < 0) {
        y = 0;
    }


#ifdef FONT_USE_FREETYPE
    AX_S32 s32Error = FT_Set_Pixel_Sizes(m_fontFace, uFontSize, uFontSize);
    if (0 != s32Error) {
        printf("FT Set Size error!\r\n");
        return nullptr;
    }
    FT_Bitmap *bitmap;
    AX_S32 sBearingY = 0;

    switch (enAlign) {
        case OSD_ALIGN_TYPE_LEFT_TOP:
        case OSD_ALIGN_TYPE_LEFT_BOTTOM: {
            int x0 = x;
            AX_BOOL bExit = AX_FALSE;
            for (AX_U16 i = 0; i < uLen; i++) {
                if (bExit) {
                    break;
                }
                bitmap = FTGetGlpyhBitMap(pTextStr[i]);
                sBearingY = m_fontSlot->metrics.horiBearingY;
                int y0 = y + uFontSize - (int)(5.0 * uFontSize / 64) - sBearingY / 64;
                for (AX_U16 j = 0; j < bitmap->rows; j++) {
                    for (AX_S16 k = 0; k < bitmap->pitch; k++) {
                        dot8 = bitmap->buffer[k + j * bitmap->pitch];
                        for (AX_U16 dot = 0; dot < 8; dot++) {
                            if ((dot8 & 0x80) == 0x80) {
                                if (bIsBrushSide) {
                                    BrushSide(pArgbBuffer, x0 + k * 8 + dot, y0 + j, u16SideColor, u16BgColor, u32OSDWidth, u32OSDHeight);
                                }
                                if (OE_WIDTH_BACKWARD_RANGE ==
                                    DrawPoint(pArgbBuffer, x0 + k * 8 + dot, y0 + j, u16FontColor, u32OSDWidth, u32OSDHeight)) {
                                    if (!bExit) {
                                        bExit = AX_TRUE;
                                    }
                                }
                            }
                            dot8 = dot8 << 1;
                        }
                    }
                }
                x0 += bitmap->width + (int)(5.0 * uFontSize / 64);
            }
            break;
        }
        case OSD_ALIGN_TYPE_RIGHT_TOP:
        case OSD_ALIGN_TYPE_RIGHT_BOTTOM: {
            int x0 = u32OSDWidth - x;
            if (x0 < 0) {
                return nullptr;
            }
            AX_BOOL bExit = AX_FALSE;
            for (AX_S16 i = uLen - 1; i >= 0; i--) {
                if (bExit) {
                    break;
                }
                bitmap = FTGetGlpyhBitMap(pTextStr[i]);
                sBearingY = m_fontSlot->metrics.horiBearingY;
                int y0 = y + uFontSize - (int)(5.0 * uFontSize / 64) - sBearingY / 64;
                x0 -= bitmap->width + (int)(5.0 * uFontSize / 64);
                for (AX_U16 j = 0; j < bitmap->rows; j++) {
                    for (AX_S16 k = 0; k < bitmap->pitch; k++) {
                        dot8 = bitmap->buffer[k + j * bitmap->pitch];
                        for (AX_U16 dot = 0; dot < 8; dot++) {
                            if ((dot8 & 0x80) == 0x80) {
                                if (bIsBrushSide) {
                                    BrushSide(pArgbBuffer, x0 + k * 8 + dot, y0 + j, u16SideColor, u16BgColor, u32OSDWidth, u32OSDHeight);
                                }
                                if (OE_WIDTH_FORWARD_RANGE ==
                                    DrawPoint(pArgbBuffer, x0 + k * 8 + dot, y0 + j, u16FontColor, u32OSDWidth, u32OSDHeight)) {
                                    if (!bExit) {
                                        bExit = AX_TRUE;
                                    }
                                }
                            }
                            dot8 = dot8 << 1;
                        }
                    }
                }
            }
            break;
        }
        default:
            printf("unsupported align type!\r\n");
            return nullptr;
    }

#else
    FONT_BITMAP_T bitmap;

    AX_U16 nScale = (uFontSize+15)/16;

    switch (enAlign) {
        case OSD_ALIGN_TYPE_LEFT_TOP:
        case OSD_ALIGN_TYPE_LEFT_BOTTOM: {
            int x0 = x;
            int y0 = y;
            AX_BOOL bExit = AX_FALSE;
            for (AX_U16 i = 0; i < uLen; i++) {
                if (bExit) {
                    break;
                }
                GetFontBitmap((AX_U16)(pTextStr[i]), bitmap);
                AX_U16 nWByte = bitmap.nWidth / 8;
                for (AX_U16 j = 0; j < bitmap.nHeight; j++) {
                    for (AX_U16 k = 0; k < nWByte; k++) {
                        dot8 = bitmap.pBuffer[k + j * nWByte];
                        for (AX_U16 dot = 0; dot < 8; dot++) {
                            if ((dot8 & 0x80) == 0x80) {
                                for(AX_U16 hScale = 0; hScale < nScale; hScale++)
                                {
                                    for(AX_U16 wScale = 0; wScale < nScale; wScale++)
                                    {
                                        if (bIsBrushSide) {
                                            BrushSide(pArgbBuffer, x0 + (k * 8 + dot) * nScale + wScale, y0 + j * nScale + hScale, u16SideColor, u16BgColor, u32OSDWidth, u32OSDHeight);
                                        }
                                        if (OE_WIDTH_BACKWARD_RANGE ==
                                            DrawPoint(pArgbBuffer, x0 + (k * 8 + dot) * nScale + wScale, y0 + j * nScale + hScale, u16FontColor, u32OSDWidth, u32OSDHeight)) {
                                            if (!bExit) {
                                                bExit = AX_TRUE;
                                            }
                                        }
                                    }
                                }
                            }
                            dot8 = dot8 << 1;
                        }
                    }
                }
                x0 += bitmap.nWidth * nScale;
            }
            break;
        }
        case OSD_ALIGN_TYPE_RIGHT_TOP:
        case OSD_ALIGN_TYPE_RIGHT_BOTTOM: {
            int x0 = u32OSDWidth - x;
            if (x0 < 0) {
                return nullptr;
            }
            int y0 = y;
            AX_BOOL bExit = AX_FALSE;
            for (AX_S16 i = uLen - 1; i >= 0; i--) {
                if (bExit) {
                    break;
                }
                GetFontBitmap((AX_U16)(pTextStr[i]), bitmap);
                AX_U16 nWByte = bitmap.nWidth / 8;
                x0 -= bitmap.nWidth * nScale;
                for (AX_U16 j = 0; j < bitmap.nHeight; j++) {
                    for (AX_S16 k = 0; k < nWByte; k++) {
                        dot8 = bitmap.pBuffer[k + j * nWByte];
                        for (AX_U16 dot = 0; dot < 8; dot++) {
                            if ((dot8 & 0x80) == 0x80) {
                                for(AX_U16 hScale = 0; hScale < nScale; hScale++)
                                {
                                    for(AX_U16 wScale = 0; wScale < nScale; wScale++)
                                    {
                                        if (bIsBrushSide) {
                                            BrushSide(pArgbBuffer, x0 + (k * 8 + dot) * nScale + wScale, y0 + j * nScale + hScale, u16SideColor, u16BgColor, u32OSDWidth, u32OSDHeight);
                                        }
                                        if (OE_WIDTH_BACKWARD_RANGE ==
                                            DrawPoint(pArgbBuffer, x0 + (k * 8 + dot) * nScale + wScale, y0 + j * nScale + hScale, u16FontColor, u32OSDWidth, u32OSDHeight)) {
                                            if (!bExit) {
                                                bExit = AX_TRUE;
                                            }
                                        }
                                    }
                                }
                            }
                            dot8 = dot8 << 1;
                        }
                    }
                }
            }
            break;
        }
        default:
            printf("unsupported align type!\r\n");
            return nullptr;
    }

#endif
    return pArgbBuffer;
}

AX_BOOL COSDHandler::InitOSDHandler(const AX_CHAR *fontFilePath) {

#ifdef FONT_USE_FREETYPE
    AX_S32 s32Error = 0;

    s32Error = FT_Init_FreeType(&m_fontLibrary);
    if (0 != s32Error) {
        printf("FT_Init_FreeType error!\r\n");
        return AX_FALSE;
    }

    s32Error = FT_New_Face(m_fontLibrary, fontFilePath, 0, &m_fontFace);
    if (FT_Err_Unknown_File_Format == s32Error) {
        printf("FT_New_Face unknown file format!\r\n");
        return AX_FALSE;
    } else if (0 != s32Error) {
        printf("cannot open font file! errorNo:%d\r\n", s32Error);
        return AX_FALSE;
    }

    m_fontSlot = m_fontFace->glyph;
#endif

    return AX_TRUE;
}

AX_BOOL COSDHandler::DeinitOSDHandler(AX_VOID) {

#ifdef FONT_USE_FREETYPE
    if (nullptr != m_fontFace) {
        FT_Done_Face(m_fontFace);
        m_fontFace = nullptr;
    }

    if (nullptr != m_fontLibrary) {
        FT_Done_FreeType(m_fontLibrary);
        m_fontLibrary = nullptr;
    }
#endif

    return AX_TRUE;
}

COSDHandler::OSD_ERROR COSDHandler::DrawPoint(AX_U16 *pDataBuffer, AX_S16 x, AX_S16 y, AX_U16 uFontColor, AX_U32 u32OSDWidth,
                                              AX_U32 u32OSDHeight) {
    if (x >= static_cast<AX_S16>(u32OSDWidth)) {
        return OE_WIDTH_BACKWARD_RANGE;
    }

    if (x < 0) {
        return OE_WIDTH_FORWARD_RANGE;
    }

    if (y >= static_cast<AX_S16>(u32OSDHeight)) {
        return OE_HEIGHT_BACKWARD_RANGE;
    }

    if (y < 0) {
        return OE_HEIGHT_FORWARD_RANGE;
    }

    if ((x + y * u32OSDWidth) < u32OSDWidth * u32OSDHeight) {
        pDataBuffer[(x + y * u32OSDWidth)] = uFontColor;
        return OE_DRAW_OSD_SUCC;
    } else {
        return OE_OUT_OF_BUFFER;
    }
}

COSDHandler::OSD_ERROR COSDHandler::BrushSide(AX_U16 *pDataBuffer, AX_S16 x, AX_S16 y, AX_U16 uSideColor, AX_S16 uBgColor,
                                              AX_U32 u32OSDWidth, AX_U32 u32OSDHeight) {
    if ((x >= static_cast<AX_S16>(u32OSDWidth)) || ((x + 1) >= static_cast<AX_S16>(u32OSDWidth))) {
        return OE_WIDTH_BACKWARD_RANGE;
    }

    if (((x - 1) < 0) || x < 0) {
        return OE_WIDTH_FORWARD_RANGE;
    }

    if ((y >= static_cast<AX_S16>(u32OSDHeight)) || ((y + 1) >= static_cast<AX_S16>(u32OSDHeight))) {
        return OE_HEIGHT_BACKWARD_RANGE;
    }

    if (((y - 1) < 0) || y < 0) {
        return OE_HEIGHT_FORWARD_RANGE;
    }

    if (!(uBgColor ^ pDataBuffer[x + (y - 1) * u32OSDWidth])) {
        pDataBuffer[x + (y - 1) * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[x + (y + 1) * u32OSDWidth])) {
        pDataBuffer[x + (y + 1) * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[(x - 1) + y * u32OSDWidth])) {
        pDataBuffer[(x - 1) + y * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[(x + 1) + y * u32OSDWidth])) {
        pDataBuffer[(x + 1) + y * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[(x - 1) + (y - 1) * u32OSDWidth])) {
        pDataBuffer[(x - 1) + (y - 1) * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[(x + 1) + (y - 1) * u32OSDWidth])) {
        pDataBuffer[(x + 1) + (y - 1) * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[(x - 1) + (y + 1) * u32OSDWidth])) {
        pDataBuffer[(x - 1) + (y + 1) * u32OSDWidth] = uSideColor;
    }

    if (!(uBgColor ^ pDataBuffer[(x + 1) + (y + 1) * u32OSDWidth])) {
        pDataBuffer[(x + 1) + (y + 1) * u32OSDWidth] = uSideColor;
    }

    return OE_DRAW_OSD_SUCC;
}

AX_U16 COSDHandler::ConvertColor2Argb1555(AX_U32 &uColor) {
    AX_U16 uColorResult = 0x0;
    AX_U16 uTmp = 0x0;

    AX_U8 uBgColor_a = ((uColor >> 24) & 0xFF) & 0x01;
    AX_U8 uBgColor_r = ((uColor >> 16) & 0xFF) & 0x1F;
    AX_U8 uBgColor_g = ((uColor >> 8) & 0xFF) & 0x1F;
    AX_U8 uBgColor_b = (uColor & 0xFF) & 0x1F;

    uColorResult = ((uTmp | uBgColor_a) << 0x0F) | ((uTmp | uBgColor_r) << 0x0A) | ((uTmp | uBgColor_g) << 0x05) | (uTmp | uBgColor_b);

    return uColorResult;
}

#ifdef FONT_USE_FREETYPE
FT_Bitmap *COSDHandler::FTGetGlpyhBitMap(AX_U16 u16CharCode) {
    AX_S32 s32Error = 0;

    s32Error = FT_Load_Char(m_fontFace, u16CharCode, FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
    if (0 != s32Error) {
        printf("FT_Load_Glyph failed!\r\n");
        return nullptr;
    }

    return &m_fontSlot->bitmap;
}
#endif
