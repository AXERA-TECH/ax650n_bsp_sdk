/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "venc.hpp"
#include "make_unique.hpp"

#define TAG "VENC"
#define LOG_FUNC_ENTER() LOG_MM_I(TAG, "veChn %d +++", m_veChn)
#define LOG_FUNC_LEAVE() LOG_MM_I(TAG, "veChn %d ---", m_veChn)

// class CAXFrameRelease : public IFrameRelease {
// public:
//     AX_VOID VideoFrameRelease(CAXFrame *pFrame) {
//         if (pFrame) {
//             pFrame->DecRef();
//             delete pFrame;
//         }
//     }
// };

// static CAXFrameRelease theFrameRelease;

AX_BOOL CVENC::Init(CONST VENC_ATTR_T& stAttr) {
    m_veChn = stAttr.nChannel;

    LOG_FUNC_ENTER();

    m_venc = std::make_unique<CVideoEncoderEx>(stAttr);
    if (!m_venc) {
        LOG_MM_E(TAG, "create VENC instance fail");
        return AX_FALSE;
    }

    if (!m_venc->InitParams()) {
        LOG_MM_E(TAG, "InitParams fail");
        return AX_FALSE;
    }

    if (!m_venc->Init()) {
        return AX_FALSE;
    }

    if (stAttr.nGdrNum > 0 && stAttr.nGdrNum <= stAttr.nGOP) {
        if (!m_venc->ActiveGDR(stAttr.nGdrNum)) {
            return AX_FALSE;
        }
    }

    LOG_FUNC_LEAVE();
    return AX_TRUE;
}

AX_BOOL CVENC::DeInit(AX_VOID) {
    LOG_FUNC_ENTER();

    UnRegisterObserver();

    if (m_venc) {
        if (!m_venc->DeInit()) {
            return AX_FALSE;
        }

        m_venc = nullptr;
    }

    LOG_FUNC_LEAVE();
    return AX_TRUE;
}

AX_BOOL CVENC::RegisterObserver(IVencPackObserver *obs) {
    std::lock_guard<std::mutex> lck(m_mtxObs);

    if (m_obs == obs) {
        LOG_MM_W(TAG, "veChn %d obs is registered", m_veChn);
        return AX_TRUE;
    }

    if (m_obs) {
        m_venc->UnregObserver(m_obs);
    }

    m_venc->RegObserver(obs);
    m_obs = obs;

    return AX_TRUE;
}

AX_VOID CVENC::UnRegisterObserver(AX_VOID) {
    std::lock_guard<std::mutex> lck(m_mtxObs);
    if (m_obs) {
        m_venc->UnregObserver(m_obs);
        m_obs = nullptr;
    }
}

AX_BOOL CVENC::Start(AX_VOID) {
    LOG_FUNC_ENTER();

    if (!m_venc->Start()) {
        return AX_FALSE;
    }

    LOG_FUNC_LEAVE();
    return AX_TRUE;
}

AX_BOOL CVENC::Stop(AX_VOID) {
    LOG_FUNC_ENTER();

    if (!m_venc->Stop()) {
        return AX_FALSE;
    }

    LOG_FUNC_LEAVE();
    return AX_TRUE;
}
