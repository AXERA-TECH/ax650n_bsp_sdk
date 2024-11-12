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
#include <memory>
#include <string>
#include <vector>
#include "AiCardSlvAppSys.hpp"
#include "AiCardSlvConfig.hpp"
#include "TransferHelper.hpp"
#include "Detector.hpp"
#include "DetectObserver.hpp"
#include "IObserver.h"
#include "VideoDecoder.hpp"


using namespace aicard_slv;

class CAiCardSlvBuilder final {
public:
    CAiCardSlvBuilder(AX_VOID) noexcept = default;

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

protected:
    AX_BOOL Init(AX_VOID);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL InitTransHelper();
    AX_BOOL InitDispatcher();
    AX_BOOL InitDecoder(const VDEC_CONFIG_T& tVdecConfig);
    AX_BOOL InitDetector(const DETECT_CONFIG_T &tDetectConfig);

protected:
    CAiCardSlvAppSys m_sys;
    AX_U32 m_nDecodeGrpCount{0};
    AX_BOOL m_bSimulateDetRets{AX_FALSE};
    std::unique_ptr<CTransferHelper> m_transHelper;
    std::unique_ptr<CVideoDecoder> m_vdec;
    std::unique_ptr<CDetector> m_detector;
    IObserverUniquePtr m_vdecCtrlObserver;
    IObserverUniquePtr m_detectorObserver;
    std::vector<IObserverUniquePtr> m_arrDispatchObserver;

};