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
#include <assert.h>
#include <exception>
#include "AXFrame.hpp"
#include "IObserver.h"
#include "VideoDecoder.hpp"
#include "CommonDef.h"

/**
 * @brief
 *
 */
class CVdecCtrlObserver final : public IObserver {
public:
    CVdecCtrlObserver(CVideoDecoder* pSink, AX_BOOL bEnableReset) noexcept : m_pSink(pSink), m_bEnableReset(bEnableReset){};
    virtual ~CVdecCtrlObserver(AX_VOID) = default;

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (m_bEnableReset && (E_OBS_TARGET_TYPE_AICARD_TRANSFER == eTarget)) {
            std::vector<VDEC_GRP_ATTR_T> vecGrpAttr;
            if (m_pSink->GetAllGrpAttr(vecGrpAttr)) {
                m_pSink->Stop();
                m_pSink->DeInit();
                m_pSink->Init(vecGrpAttr);
                for (AX_U32 i = 0; i < vecGrpAttr.size(); i++) {
                    m_pSink->AttachPool(i, VDEC_CHN1, m_mapChnToPool[i]);
                }

                m_pSink->Start();
            }
        }

        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        return AX_TRUE;
    }

    AX_VOID SetPool(AX_VDEC_GRP vdGrp, AX_VDEC_CHN nChn, AX_POOL pool) {
        m_mapChnToPool[vdGrp] = pool;
    }

private:
    CVideoDecoder* m_pSink;
    AX_BOOL m_bEnableReset{AX_FALSE};
    std::map<AX_VDEC_GRP, AX_POOL> m_mapChnToPool{};
};