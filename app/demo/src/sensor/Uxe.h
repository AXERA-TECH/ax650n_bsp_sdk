/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "BaseSensor.h"

class CUxe : public CBaseSensor {
public:
    CUxe(AX_BOOL bYUV444);
    virtual ~CUxe(AX_VOID);

    AX_BOOL Init(AX_VOID) override;
    AX_BOOL DeInit(AX_VOID) override;

    AX_BOOL Open() override;
    AX_BOOL Close() override;

    const SENSOR_CONFIG_T& GetSensorCfg();

protected:
    virtual AX_VOID InitSnsLibraryInfo(AX_VOID) override;
    virtual AX_VOID InitSnsAttr() override;
    virtual AX_VOID InitSnsClkAttr() override;
    virtual AX_VOID InitDevAttr() override;
    virtual AX_VOID InitPrivAttr() override;
    virtual AX_VOID InitPipeAttr() override;
    virtual AX_VOID InitMipiRxAttr() override;
    virtual AX_VOID InitChnAttr() override;
    virtual AX_VOID InitAbilities() override;
    virtual AX_VOID InitTriggerAttr() override;

private:
    AX_BOOL m_bYUV444{AX_FALSE};
    AX_FRAME_INTERRUPT_ATTR_T m_tDevFrmIntAttr;
};
