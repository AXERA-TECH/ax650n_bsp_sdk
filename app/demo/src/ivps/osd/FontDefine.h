#ifndef __FONTDEFINE_H__
#define __FONTDEFINE_H__

#include "ax_base_type.h"

typedef struct _UICODE_INDEX_T
{
    AX_U16 nUnicode;
    AX_U16 nIndex;
} UICODE_INDEX_T;

typedef struct _FONT_BITMAP_T
{
    AX_U16  nWidth;   // pixels, one bit one pixel
    AX_U16  nHeight;  // pixels, one bit one pixel
    AX_U8*  pBuffer;
} FONT_BITMAP_T;

#endif // __FONTDEFINE_H__


