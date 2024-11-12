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

#include <mutex>
#include "AXFrame.hpp"
#include "IModule.h"
#include "IObserver.h"

#define MAX_COLLECT_GROUP_NUM (4)
#define APP_COLLECT_INVALID (0)

typedef struct COLLECTOR_ATTR_S {
    AX_U32 nWidth{APP_COLLECT_INVALID};
    AX_U32 nHeight{APP_COLLECT_INVALID};
    AX_S8 nSnsSrc{APP_COLLECT_INVALID};
    AX_F32 fFramerate{APP_COLLECT_INVALID};
    AX_BOOL bEnableFBC{AX_TRUE};
} COLLECTOR_ATTR_T;

class CFrameCollector : public IModule {
public:
    CFrameCollector(AX_U8 nGroup);
    virtual ~CFrameCollector(AX_VOID) = default;

    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL Stop() override;

    AX_VOID RegObserver(IObserver* pObserver);
    AX_VOID UnregObserver(IObserver* pObserver);
    std::vector<std::pair<AX_U8, AX_U8>>& GetTargetChannels() {
        return m_vecTargetChannel;
    };
    AX_VOID RegTargetChannel(AX_U8 nGroup, AX_U8 nChannel);

    AX_BOOL RecvFrame(AX_U8 nSrcGroup, AX_U8 nSrcChannel, CAXFrame* pFrame);
    COLLECTOR_ATTR_T* GetCollectorCfg() {
        return &m_stAttr;
    };

    AX_U8 GetGroup() {
        return m_nGroup;
    };

private:
    AX_BOOL IsTarget(AX_U8 nGroup, AX_U8 nChannel);
    AX_VOID NotifyAll(AX_VOID* pFrame);

private:
    AX_U8 m_nGroup;
    std::vector<IObserver*> m_vecObserver;
    std::vector<std::pair<AX_U8, AX_U8>> m_vecTargetChannel;
    std::mutex m_mtxFrame;
    COLLECTOR_ATTR_T m_stAttr;
};
