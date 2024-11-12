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
#include "AiCardMstAppSys.hpp"
#include "AiCardMstConfig.hpp"
#include "AiSwitchConfig.hpp"
#include "Dispatcher.hpp"
#include "IObserver.h"
#include "IStreamHandler.hpp"
#include "VideoDecoder.hpp"
#include "Vo.hpp"
#include "TransferHelper.hpp"
#include "AiSwitchSimulator.hpp"

using namespace aicard_mst;

class CAiCardMstBuilder final {
public:
    CAiCardMstBuilder(AX_VOID) noexcept = default;

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL QueryStreamsAllEof(AX_VOID);

protected:
    AX_BOOL Init(AX_VOID);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL InitStreamer(const STREAM_CONFIG_T& streamConfig);
    AX_BOOL InitDisplay(const DISPVO_CONFIG_T& dispVoConfig);
    AX_BOOL InitDispatcher(const std::string& strFontPath);
    AX_BOOL InitDecoder(const STREAM_CONFIG_T& streamConfig);
    AX_BOOL InitTransHelper();
    AX_BOOL InitAiSwitchSimlator();

protected:
    CAiCardMstAppSys m_sys;
    AX_U32 m_nDecodeGrpCount{0};
    std::unique_ptr<CVideoDecoder> m_vdec;
    std::unique_ptr<CVo> m_disp;
    std::vector<std::unique_ptr<CDispatcher>> m_arrDispatcher;
    IObserverUniquePtr m_dispObserver;
    std::vector<IObserverUniquePtr> m_arrDispatchObserver;
    std::vector<IStreamerHandlerPtr> m_arrStreamer;
    std::unique_ptr<CTransferHelper> m_transHelper;
    std::unique_ptr<CAiSwitchSimulator> m_aiSwitchSimulator;
};