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

#include "OsdConfig.h"
#include "ax_base_type.h"
#include "ax_ivps_api.h"

#ifdef FONT_USE_FREETYPE
#include "freetype.h"
#include "ft2build.h"
#include "ftglyph.h"
#else
#include "FontIndex.h"
#endif

#include <wchar.h>

#include <cmath>
#include <iostream>
#include <mutex>

#define OSD_PIXEL_MAX_WIDTH 3840
#define OSD_PIXEL_MAX_HEIGHT 2160
#define ROTATION_WIDTH_ALIGEMENT (8)

#define RED 0xFF0000
#define PINK 0xFFC0CB
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define PURPLE 0xA020F0
#define ORANGE 0xFFA500
#define YELLOW 0xFFFF00

typedef AX_S32 AX_IVPS_FILTER;

class COSDHandler {
public:
    typedef enum osd_error_en {
        OE_DRAW_OSD_SUCC,
        OE_WIDTH_BACKWARD_RANGE,
        OE_WIDTH_FORWARD_RANGE,
        OE_HEIGHT_BACKWARD_RANGE,
        OE_HEIGHT_FORWARD_RANGE,
        OE_OUT_OF_BUFFER,
        MAX_NUM_OE
    } OSD_ERROR;

    COSDHandler();
    ~COSDHandler();
    /* Init OSDHanlder
       @param - [IN]fontPath: font file path.
       @return - error code

       description:
       1. Init font file path
    */
    AX_BOOL InitOSDHandler(const AX_CHAR *fontfILEPath);

    /* Generate argb data with the input text string
       @param - [IN] pTextStr: text needs to be transformated,support freetype font.
       @param - [IN/OUT]     pArgbBuffer: buffer to store osd argb data
       @param - [IN]     u32OSDWidth: OSD width
       @param - [IN]     u32OSDHeight: OSD height
       @param - [IN]sX:x axis value in osd coordinate
       @param - [IN]sY:y axis value in osd coordinate
       @param - [IN]     uFontSize: font size
       @param - [IN]     bIsBrushSide: flag for brushing character side
       @param - [IN]     uFontColor: font color
       @param - [IN]     uBgColor: background color for osd,two bytes should be same and different from other color
       @param - [IN]     uSideColor: color for brushing side
       @return - argb data buffer pointer, nullptr if error

       description:
       1. Generate argb data with input text string, and set the font color and background color.
     */
    AX_VOID *GenARGB(wchar_t *pTextStr, AX_U16 *pArgbBuffer, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight, AX_S16 sX, AX_S16 sY,
                     AX_U16 uFontSize, AX_BOOL bIsBrushSide = AX_FALSE, AX_U32 uFontColor = 0xFFFFFFFF, AX_U32 uBgColor = 0xFF0000,
                     AX_U32 uSideColor = 0, OSD_ALIGN_TYPE_E enAlign = OSD_ALIGN_TYPE_LEFT_TOP);

private:
    AX_BOOL DeinitOSDHandler(AX_VOID);

    OSD_ERROR DrawPoint(AX_U16 *pDataBuffer, AX_S16 x, AX_S16 y, AX_U16 uFontColor, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight);
    OSD_ERROR BrushSide(AX_U16 *pDataBuffer, AX_S16 x, AX_S16 y, AX_U16 uSideColor, AX_S16 uBgColor, AX_U32 u32OSDWidth,
                        AX_U32 u32OSDHeight);

    AX_U16 ConvertColor2Argb1555(AX_U32 &uColor);

#ifdef FONT_USE_FREETYPE
    FT_Bitmap *FTGetGlpyhBitMap(AX_U16 u16CharCode);

    FT_Library m_fontLibrary;
    FT_Face m_fontFace;
    FT_GlyphSlot m_fontSlot;
#endif
};
