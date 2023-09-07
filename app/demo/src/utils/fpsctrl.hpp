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
#include "ax_base_type.h"

class CFpsCtrl {
public:
    CFpsCtrl(AX_U32 nFps);

    AX_VOID Reset(AX_VOID);
    AX_VOID Control(AX_U64 nSeqNum, AX_U32 nMargin = 0 /* microseconds */);
    AX_U64 GetCurPTS(AX_VOID);

private:
    /* microseconds */
    AX_S32 m_nIntervl = {0};
    AX_U64 m_nNextPTS = {0};
    AX_U64 m_nCurrPTS = {0};
};

inline AX_VOID CFpsCtrl::Reset(AX_VOID) {
    m_nCurrPTS = 0;
    m_nNextPTS = 0;
}

inline AX_U64 CFpsCtrl::GetCurPTS(AX_VOID) {
    return m_nCurrPTS;
}