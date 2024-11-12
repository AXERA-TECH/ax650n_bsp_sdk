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

#include "BaseSensor.h"

class CUxe : public CBaseSensor {
public:
    CUxe(AX_IMG_FORMAT_E eImgFormat, AX_U32 nW = 3840, AX_U32 nH = 2160, AX_U32 nFps = 60);
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
    //AX_BOOL m_bYUV444{AX_FALSE};
    AX_IMG_FORMAT_E m_eImgFormat;  // AX_FORMAT_BAYER_RAW_8BPP, AX_FORMAT_YUV420_SEMIPLANAR, AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010
    AX_FRAME_INTERRUPT_ATTR_T m_tDevFrmIntAttr;
    AX_U32 m_nW;
    AX_U32 m_nH;
    AX_U32 m_nFps;
};
