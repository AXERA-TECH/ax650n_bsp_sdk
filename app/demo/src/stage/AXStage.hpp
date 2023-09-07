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
#include <string>
#include "AXFrame.hpp"
#include "AXLockQ.hpp"
#include "AXThread.hpp"
#include "IModule.h"

typedef struct _STAGE_START_PARAMS {
    /* Indicates whether Stage would start frame processing thread, usually enabled on non-link channels */
    AX_BOOL bStartProcessingThread;
} STAGE_START_PARAM_T, *STAGE_START_PARAM_PTR;

/**
 * @brief
 *
 */
class CAXStage : public IModule {
public:
    CAXStage(const std::string& strName);
    virtual ~CAXStage(AX_VOID);

    const std::string& GetStageName(AX_VOID) const;
    AX_VOID SetCapacity(AX_S32 nCapacity);

    CAXStage* BindNextStage(CAXStage* pNext);
    CAXStage* GetNextStage(AX_VOID);

    virtual AX_BOOL Init(AX_VOID) override;
    virtual AX_BOOL DeInit(AX_VOID) override;

    virtual AX_BOOL Start(AX_VOID) override;
    virtual AX_BOOL Stop(AX_VOID) override;
    virtual AX_BOOL Start(STAGE_START_PARAM_PTR pStartParams);

    virtual AX_BOOL EnqueueFrame(CAXFrame* pFrame);

protected:
    virtual AX_BOOL ProcessFrame(CAXFrame* pFrame);
    virtual AX_VOID StageThreadFunc(AX_VOID* pArg);

protected:
    CAXLockQ<CAXFrame*> m_qFrame;
    std::string m_strStageName;
    CAXStage* m_pNextStage{nullptr};
    CAXThread m_StageThread;
};