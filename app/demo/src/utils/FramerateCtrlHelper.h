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

class CFramerateCtrlHelper {
public:
    CFramerateCtrlHelper(AX_U32 nSrcFramerate, AX_U32 nDstFramerate);
    ~CFramerateCtrlHelper(AX_VOID);

public:
    AX_BOOL FramerateCtrl(AX_BOOL bTry = AX_FALSE);

private:
    AX_U32 m_nSrcFramerate;
    AX_U32 m_nDstFramerate;
    AX_U64 m_nSeq;
};
