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

#include <vector>
#include <memory>

#include "AXFrame.hpp"
#include "AXLockQ.hpp"
#include "OptionHelper.h"
#include "AXResource.hpp"
#include "AXStage.hpp"
#include "AXThread.hpp"
#include "IObserver.h"
#include "Md.h"
#include "Od.h"
#include "Scd.h"

#define MAX_IVES_GROUP_NUM 2

typedef struct _IVES_ATTR_T {
    AX_U32 nGrpCount{0};
    AX_U32 nWidth{0};
    AX_U32 nHeight{0};
    AX_F32 fSrcFramerate{0};
    AX_S8 nSnsSrc{0};
} IVES_ATTR_T;

class CIVESStage final : public CAXStage {
public:
    CIVESStage(AX_VOID);
    ~CIVESStage(AX_VOID) = default;

    AX_BOOL Init(AX_U32 nGrp);
    AX_BOOL DeInit(AX_VOID) override;
    AX_BOOL Start(AX_VOID) override;
    AX_BOOL Stop(AX_VOID) override;
    AX_BOOL ProcessFrame(CAXFrame* pFrame) override;

    AX_BOOL UpdateRotation(AX_U8 nRotation);
    AX_BOOL SendFrame(AX_U32 nSnsID, CAXFrame* pAxFrame);
    IVES_ATTR_T* GetIVESCfg() {
        return &m_stAttr;
    };

    CMD* GetMDInstance(AX_VOID) const {
        return m_spMDInstance.get();
    }
    COD* GetODInstance(AX_VOID) const {
        return m_spODInstance.get();
    }
    CSCD* GetSCDInstance(AX_VOID) const {
        return m_spSCDInstance.get();
    }
    AX_BOOL GetMDCapacity(AX_VOID) const {
        return m_bMDEnable;
    }
    AX_VOID SetMDCapacity(AX_BOOL enable) {
        m_bMDEnable = enable;
    }
    AX_BOOL GetODCapacity(AX_VOID) const {
        return m_bODEnable;
    }
    AX_VOID SetODCapacity(AX_BOOL enable) {
        m_bODEnable = enable;
    }
    AX_BOOL GetSCDCapacity(AX_VOID) const {
        return m_bSCDEnable;
    }
    AX_VOID SetSCDCapacity(AX_BOOL enable) {
        m_bSCDEnable = enable;
    }

    AX_VOID RegObserver(IObserver* pObserver);
    AX_VOID UnregObserver(IObserver* pObserver);

    static AX_BOOL InitModule();
    static AX_BOOL DeinitModule();

private:
    AX_VOID NotifyAll(AX_U32 nGrp, AX_U32 nChn, AX_VOID *pStream);
    AX_BOOL GetResolutionByRotate(AX_U8 nRotation, AX_U32 &nWidth, AX_U32 &nHeight) ;
    AX_VOID FreeQFrames(AX_VOID);

private:
    AX_BOOL m_bMDEnable{AX_FALSE};
    AX_BOOL m_bODEnable{AX_FALSE};
    AX_BOOL m_bSCDEnable{AX_FALSE};

    IVES_ATTR_T m_stAttr;

    std::unique_ptr<CMD> m_spMDInstance;
    std::unique_ptr<COD> m_spODInstance;
    std::unique_ptr<CSCD> m_spSCDInstance;
    AX_U32 m_nGroup{0};
    std::vector<IObserver*> m_vecObserver;

    AX_BOOL m_bReseting{AX_FALSE};
};
