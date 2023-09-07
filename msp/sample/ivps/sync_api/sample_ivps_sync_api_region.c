/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ivps_main.h"

AX_S32 SAMPLE_FillCanvas(AX_IVPS_RGN_CANVAS_INFO_T *ptCanvas)
{
    AX_IVPS_GDI_ATTR_T tAttr;

    ALOGI("nStride %d nW:%d nH:%d", ptCanvas->nStride, ptCanvas->nW, ptCanvas->nH);

    tAttr.nColor = GREEN;
    tAttr.nAlpha = 0xFF;
    tAttr.nThick = (rand() % 16);
    tAttr.bSolid = AX_TRUE;

    AX_IVPS_POINT_T tLine[2];
    tLine[0].nX = 200;
    tLine[0].nY = 100;
    tLine[1].nX = 200;
    tLine[1].nY = 100;
    ALOGI("X0:%d Y0:%d", tLine[0].nX, tLine[0].nY);
    ALOGI("X1:%d Y1:%d", tLine[1].nX, tLine[1].nY);
    AX_IVPS_DrawLine(ptCanvas, tAttr, tLine, 2);

    tAttr.nColor = PALETURQUOISE;
    tAttr.nAlpha = 0xFF;
    tAttr.nThick = 4;
    tAttr.bSolid = AX_TRUE;

    AX_IVPS_POINT_T tPolygon[4];
    tPolygon[0].nX = 60;
    tPolygon[0].nY = 50;
    tPolygon[1].nX = 100;
    tPolygon[1].nY = 20;

    tPolygon[2].nX = 200;
    tPolygon[2].nY = 70;

    tPolygon[3].nX = 110;
    tPolygon[3].nY = 380;
    AX_IVPS_DrawPolygon(ptCanvas, tAttr, tPolygon, 4);

    tAttr.nColor = BLUE;
    tAttr.nAlpha = 0xFF;
    tAttr.nThick = 5;
    tAttr.bSolid = AX_FALSE;
    AX_IVPS_RECT_T tRect;

    tRect.nX = 500;
    tRect.nY = 150;
    tRect.nW = 100;
    tRect.nH = 50;
    AX_IVPS_DrawRect(ptCanvas, tAttr, tRect);

    tAttr.nColor = RED;
    tAttr.nAlpha = 0xFF;
    tAttr.nThick = 5;
    tAttr.bSolid = AX_TRUE;
    tRect.nX = 100;
    tRect.nY = 100;
    tRect.nW = 100;
    tRect.nH = 100;
    AX_IVPS_DrawRect(ptCanvas, tAttr, tRect);

    tAttr.nColor = LIGHTCYAN;
    tAttr.nAlpha = 0xFF;
    tAttr.nThick = 5;

    AX_IVPS_POINT_T tFoldLine[3];
    tFoldLine[0].nX = 200;
    tFoldLine[0].nY = 100;
    tFoldLine[1].nX = 500;
    tFoldLine[1].nY = 320;
    tFoldLine[2].nX = 300;
    tFoldLine[2].nY = 250;
    AX_IVPS_DrawLine(ptCanvas, tAttr, tFoldLine, 3);

    tAttr.nColor = DARKGREEN;
    tAttr.nAlpha = 0xFF;
    tAttr.nThick = 1;

    tRect.nX = 400;
    tRect.nY = 300;
    tRect.nW = 100;
    tRect.nH = 60;
    AX_IVPS_POINT_T CornerRect[3];
    CornerRect[1].nX = tRect.nX;
    CornerRect[1].nY = tRect.nY;
    CornerRect[0].nX = CornerRect[1].nX + tRect.nW / 5;
    CornerRect[0].nY = CornerRect[1].nY;
    CornerRect[2].nX = CornerRect[1].nX;
    CornerRect[2].nY = CornerRect[1].nY + tRect.nH / 5;
    AX_IVPS_DrawLine(ptCanvas, tAttr, CornerRect, 3);

    CornerRect[1].nX = tRect.nX + tRect.nW;
    CornerRect[1].nY = tRect.nY;
    CornerRect[0].nX = CornerRect[1].nX - tRect.nW / 5;
    CornerRect[0].nY = CornerRect[1].nY;
    CornerRect[2].nX = CornerRect[1].nX;
    CornerRect[2].nY = CornerRect[1].nY + tRect.nH / 5;
    AX_IVPS_DrawLine(ptCanvas, tAttr, CornerRect, 3);

    CornerRect[1].nX = tRect.nX;
    CornerRect[1].nY = tRect.nY + tRect.nH;
    CornerRect[0].nX = CornerRect[1].nX + tRect.nW / 5;
    CornerRect[0].nY = CornerRect[1].nY;
    CornerRect[2].nX = CornerRect[1].nX;
    CornerRect[2].nY = CornerRect[1].nY - tRect.nH / 5;
    AX_IVPS_DrawLine(ptCanvas, tAttr, CornerRect, 3);

    CornerRect[1].nX = tRect.nX + tRect.nW;
    CornerRect[1].nY = tRect.nY + tRect.nH;
    CornerRect[0].nX = CornerRect[1].nX - tRect.nW / 5;
    CornerRect[0].nY = CornerRect[1].nY;
    CornerRect[2].nX = CornerRect[1].nX;
    CornerRect[2].nY = CornerRect[1].nY - tRect.nH / 5;

    AX_IVPS_DrawLine(ptCanvas, tAttr, CornerRect, 3);
    return 0;
}

AX_S32 SAMPLE_DrawCover(const AX_VIDEO_FRAME_T *pstVFrame, const SAMPLE_IVPS_COVER_T *ptIvpsCover, char *strFilePath)
{
    AX_IVPS_RGN_CANVAS_INFO_T tCanvas;
    AX_IVPS_GDI_ATTR_T tAttr;
    AX_VIDEO_FRAME_T tVFrame;

    printf("AX_IVPS_DrawLine");
    tCanvas.eFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    tCanvas.pVirAddr = (AX_VOID *)((AX_ULONG)pstVFrame->u64VirAddr[0]);
    tCanvas.nStride = pstVFrame->u32PicStride[0];
    tCanvas.nW = pstVFrame->u32Width;
    if (pstVFrame->u64PhyAddr[1])
    {
        tCanvas.nH = (pstVFrame->u64PhyAddr[1] - pstVFrame->u64PhyAddr[0]) / tCanvas.nStride;
        printf("tCanvas.nH:%d", tCanvas.nH);
    }
    else
    {
        tCanvas.nH = pstVFrame->u32Height;
    }
    AX_IVPS_POINT_T tPoint[10];
    tAttr.nThick = 5;
    tAttr.nColor = 0xFF0000;
    tPoint[0].nX = 10;
    tPoint[0].nY = 10;
    tPoint[1].nX = 100;
    tPoint[1].nY = 100;
    tPoint[2].nX = 50;
    tPoint[2].nY = 200;
    tPoint[3].nX = 440;
    tPoint[3].nY = 570;

    AX_IVPS_DrawLine(&tCanvas, tAttr, tPoint, 4);

    tAttr.nThick = 2;
    tAttr.nColor = 0xFF8000;
    tPoint[0].nX = 10 + 500;
    tPoint[0].nY = 10 + 500;
    tPoint[1].nX = 100 + 500;
    tPoint[1].nY = 100 + 600;
    tPoint[2].nX = 50 + 700;
    tPoint[2].nY = 200 + 400;
    tPoint[3].nX = 440;
    tPoint[3].nY = 570;

    AX_IVPS_DrawLine(&tCanvas, tAttr, tPoint, 4);

    /* tAttr.nThick = 2; */
    tAttr.nColor = 0xFF0080;
    tPoint[0].nX = 800;
    tPoint[0].nY = 800;
    tPoint[1].nX = 900;
    tPoint[1].nY = 900;
    tPoint[2].nX = 800;
    tPoint[2].nY = 1000;
    tPoint[3].nX = 700;
    tPoint[3].nY = 900;

    AX_IVPS_DrawPolygon(&tCanvas, tAttr, tPoint, 4);

    AX_IVPS_RECT_T tRect;

    /* tAttr.nThick = 2; */
    tAttr.bSolid = AX_FALSE;
    tAttr.nColor = 0xFFFF80;
    tRect.nX = 1000;
    tRect.nY = 900;
    tRect.nW = 600;
    tRect.nH = 300;

    AX_IVPS_DrawRect(&tCanvas, tAttr, tRect);

    tVFrame = *pstVFrame;
    SaveFile(&tVFrame, 0, 0, strFilePath, "CPUDraw");
    return 0;
}
