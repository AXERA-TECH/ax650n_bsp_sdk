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
#include "YuvHandler.hpp"

/**
 *  Load bitmap. Bitmap must be single bit bmp format
 */
class CBmpOSD {
public:
    CBmpOSD(AX_VOID) = default;
    ~CBmpOSD(AX_VOID);

    AX_U8 *LoadBmp(const AX_CHAR *pBmpFile, AX_U16 &u16w, AX_U16 &u16h, AX_U32 &u32Size);
    AX_U8 *GetString(const AX_CHAR *pNumStri, AX_U16 &u16w, AX_U16 &u16h, AX_U32 &u32Size);
    AX_VOID FreeString(AX_U8 *pStr);
    AX_BOOL FillString(const AX_CHAR *pNumStri, AX_S16 x, AX_S16 y, AX_U8 *pBmp, AX_U16 u16w, AX_U16 u16h);
    AX_BOOL FillString(const AX_CHAR *pNumStri, AX_S16 x, AX_S16 y, CYuvHandler *yuvHandler, AX_U16 u16w, AX_U16 u16h, AX_U8 nScale = 1);

private:
    AX_VOID Free(AX_VOID);

private:
    AX_U8 *m_pData{nullptr};
    AX_U8 *m_pFont{nullptr};
    AX_U16 m_uWidth{0};
    AX_U16 m_uHight{0};
};
