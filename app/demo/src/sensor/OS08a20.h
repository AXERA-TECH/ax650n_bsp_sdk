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

class COS08a20 : public CBaseSensor {
public:
    COS08a20(SENSOR_CONFIG_T tSensorConfig);
    virtual ~COS08a20(AX_VOID);

public:
    virtual AX_SNS_HDR_MODE_E GetMaxHdrMode();

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
    // virtual AX_VOID InitEnhance() override;
    // virtual AX_VOID DeInitEnhance() override;
    // static AX_BOOL SnapshotProcess(AX_U8 nCapturePipeId, AX_U8 nChannel, AX_SNS_HDR_MODE_E eHdrMode, const AX_IMG_INFO_T** pImgInfo, AX_BOOL bDummy);
};
