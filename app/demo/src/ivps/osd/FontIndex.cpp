#include "FontIndex.h"
#include "FontEn16.h"
#include "FontZh16.h"


AX_U16 GetZhGlyphIndex(AX_U16 nUnicode) {
    AX_U16 nCount = sizeof(g_fontZh16Index)/sizeof(g_fontZh16Index[0]);
    for (AX_U16 i = 0; i < nCount; i++) {
        if (g_fontZh16Index[i].nUnicode == nUnicode) {
            return i;
        } else if (g_fontZh16Index[i].nUnicode > nUnicode) {
            break;
        }
    }

    return 0;
}

AX_U16 GetEnGlyphIndex(AX_U16 nUnicode) {
    AX_U16 nCount = sizeof(g_fontEn16Index)/sizeof(g_fontEn16Index[0]);
    for (AX_U16 i = 0; i < nCount; i++) {
        if (g_fontEn16Index[i].nUnicode == nUnicode) {
            return i;
        } else if (g_fontEn16Index[i].nUnicode > nUnicode) {
            break;
        }
    }

    return 0;
}

AX_S32 GetFontBitmap(AX_U16 nUnicode, FONT_BITMAP_T &bmp) {

    if (nUnicode <= 0x7F) {
        AX_U16 nInd = GetEnGlyphIndex(nUnicode);
        bmp.nWidth = 8;
        bmp.nHeight = 16;
        bmp.pBuffer =  g_fontEn16Glyphs + (bmp.nWidth/8 * bmp.nHeight) * nInd;
    }
    else {
        AX_U16 nInd = GetZhGlyphIndex(nUnicode);
        bmp.nWidth = 16;
        bmp.nHeight = 16;
        bmp.pBuffer =  g_fontZh16Glyphs +  (bmp.nWidth/8 * bmp.nHeight) * nInd;
    }

    return 0;
}

