#include "OsdConfig.h"

AX_BOOL COSDStyle::InitOnce() {
    return AX_TRUE;
}

AX_U32 COSDStyle::GetTimeFontSize(AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nHeight);
    return g_arrOsdStyle[nIndex].nTimeFontSize;
}

AX_U32 COSDStyle::GetRectLineWidth(AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nHeight);
    return g_arrOsdStyle[nIndex].nRectLineWidth;
}

AX_U32 COSDStyle::GetBoundaryX(AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nHeight);
    return g_arrOsdStyle[nIndex].nBoundaryX;
}

AX_U32 COSDStyle::GetOsdStyleIndex(AX_U32 nHeight) {
    AX_U32 nIndex = 0;
    if (nHeight > 1536 && nHeight <= 2160)
        nIndex = 0;
    else if (nHeight > 1080 && nHeight <= 1536)
        nIndex = 1;
    else if (nHeight > 768 && nHeight <= 1080)
        nIndex = 2;
    else if (nHeight > 720 && nHeight <= 768)
        nIndex = 3;
    else if (nHeight > 576 && nHeight <= 720)
        nIndex = 4;
    else if (nHeight > 480 && nHeight <= 576)
        nIndex = 5;
    else if (nHeight > 288 && nHeight <= 480)
        nIndex = 6;
    else if (nHeight > 0 && nHeight <= 288)
        nIndex = 7;
    else
        nIndex = 1;
    return nIndex;
}