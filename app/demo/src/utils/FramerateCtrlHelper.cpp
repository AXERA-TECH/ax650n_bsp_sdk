/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "FramerateCtrlHelper.h"

CFramerateCtrlHelper::CFramerateCtrlHelper(AX_U32 nSrcFramerate, AX_U32 nDstFramerate)
    : m_nSrcFramerate(nSrcFramerate), m_nDstFramerate(nDstFramerate), m_nSeq(0) {
}

CFramerateCtrlHelper::~CFramerateCtrlHelper(AX_VOID) {
}

AX_BOOL CFramerateCtrlHelper::FramerateCtrl(AX_BOOL bTry /* = AX_FALSE */) {

    if (m_nDstFramerate <= 0) {
        return AX_FALSE;
    }

    if (m_nSrcFramerate <= m_nDstFramerate) {
        return AX_FALSE;
    }

    AX_BOOL bSkip = AX_TRUE;
    do {
        if (0 == m_nSeq) {
            bSkip = AX_FALSE;
            break;
        }

        if ((m_nSeq * m_nDstFramerate / m_nSrcFramerate) > ((m_nSeq - 1) * m_nDstFramerate / m_nSrcFramerate)) {
            bSkip = AX_FALSE;
            break;
        }
    } while (0);

    if (!bTry) {
        m_nSeq++;
    }

    return bSkip;
}