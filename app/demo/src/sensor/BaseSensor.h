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

#include <map>
#include <string>
#include <thread>
#include "GlobalDef.h"
#include "IModule.h"
#include "IspAlgoWrapper.hpp"

typedef struct axSNS_LIB_INFO_T {
    std::string strLibName;
    std::string strObjName;
} SNS_LIB_INFO_T;

typedef struct _APP_ISP_IMAGE_ATTR_T {
    AX_U8 nAutoMode{0}; /*1:Auto; 0:Manual*/
    AX_U8 nSharpness{0};
    AX_U8 nBrightness{0};
    AX_U8 nContrast{0};
    AX_U8 nSaturation{0};
} APP_ISP_IMAGE_ATTR_T;

typedef AX_BOOL (*SensorAttrUpdCallback)(ISensor* pInstance);
typedef AX_BOOL (*SnapshotProcCallback)(AX_U8 nPipe, AX_U8 nChannel, AX_SNS_HDR_MODE_E eHdrMode, const AX_IMG_INFO_T** pArrImgInfo, AX_BOOL bDummy);

using namespace std;
class CBaseSensor : public ISensor {
public:
    CBaseSensor(SENSOR_CONFIG_T tSensorConfig);
    virtual ~CBaseSensor(AX_VOID) = default;

public:
    virtual AX_BOOL Init(AX_VOID);
    virtual AX_BOOL DeInit(AX_VOID);

    virtual AX_BOOL Open() override;
    virtual AX_BOOL Close() override;

    AX_BOOL OpenAll();
    AX_BOOL CloseAll();

    virtual AX_VOID RegisterIspAlgo(const ISP_ALGO_INFO_T& tAlg) override;

    virtual AX_IMG_FORMAT_E GetMaxImgFmt();
    virtual AX_SNS_HDR_MODE_E GetMaxHdrMode();

    const AX_SNS_ATTR_T& GetSnsAttr(AX_VOID) const override;
    AX_VOID SetSnsAttr(const AX_SNS_ATTR_T& tSnsAttr) override;

    const SNS_CLK_ATTR_T& GetSnsClkAttr(AX_VOID) const override;
    AX_VOID SetSnsClkAttr(const SNS_CLK_ATTR_T& tClkAttr);

    const AX_VIN_DEV_ATTR_T& GetDevAttr(AX_VOID) const override;
    AX_VOID SetDevAttr(const AX_VIN_DEV_ATTR_T& tDevAttr) override;

    const AX_MIPI_RX_DEV_T& GetMipiRxAttr(AX_VOID) const override;
    AX_VOID SetMipiRxAttr(const AX_MIPI_RX_DEV_T& tMipiRxAttr) override;

    const AX_VIN_PIPE_ATTR_T& GetPipeAttr(AX_U8 nPipe) const override;
    AX_VOID SetPipeAttr(AX_U8 nPipe, const AX_VIN_PIPE_ATTR_T& tPipeAttr) override;

    const AX_VIN_CHN_ATTR_T& GetChnAttr(AX_U8 nPipe, AX_U8 nChannel) const override;
    AX_VOID SetChnAttr(AX_U8 nPipe, AX_U8 nChannel, const AX_VIN_CHN_ATTR_T& tChnAttr) override;

    const SNS_ABILITY_T& GetAbilities(AX_VOID) const override;

    const SENSOR_CONFIG_T& GetSnsConfig(AX_VOID) const;

    AX_U32 GetPipeCount();
    AX_VOID RegAttrUpdCallback(SensorAttrUpdCallback callback);

    AX_BOOL StartIspLoopThread();
    AX_BOOL StopIspLoopThread();

    AX_VOID InitSensor(AX_U8 nPipe);
    AX_VOID ExitSensor(AX_U8 nPipe);

    AX_BOOL RestoreIspIQAttr(APP_ISP_IMAGE_ATTR_T& tAttr);
    AX_BOOL GetIspIQAttr(APP_ISP_IMAGE_ATTR_T& tAttr);
    AX_BOOL SetIspIQAttr(const APP_ISP_IMAGE_ATTR_T& tAttr);
    AX_BOOL ChangeDaynightMode(AX_DAYNIGHT_MODE_E eDaynightMode);
    AX_VOID ChangeHdrMode(AX_U32 nSnsMode);
    AX_BOOL UpdateSnsAttr();
    AX_BOOL TriggerFlash();
    AX_BOOL ChangeSnsMirrorFlip(AX_BOOL bMirror, AX_BOOL bFlip);
    AX_U32  EnableMultiCamSync(AX_BOOL bEnable);

    SnapshotProcCallback GetSnapshotFunc() {
        return m_cbSanpshotProc;
    }


protected:
    virtual AX_VOID InitSnsLibraryInfo(AX_VOID) = 0;
    virtual AX_VOID InitSnsAttr() = 0;
    virtual AX_VOID InitSnsClkAttr() = 0;
    virtual AX_VOID InitDevAttr() = 0;
    virtual AX_VOID InitPrivAttr() = 0;
    virtual AX_VOID InitPipeAttr() = 0;
    virtual AX_VOID InitMipiRxAttr() = 0;
    virtual AX_VOID InitChnAttr() = 0;
    virtual AX_VOID InitAbilities() = 0;
    virtual AX_VOID InitTriggerAttr() = 0;
    virtual AX_VOID InitEnhance() {};
    virtual AX_VOID DeInitEnhance() {};

protected:
    virtual AX_BOOL RegisterSensor(AX_U8 nPipe, AX_U8 nDevNode);
    virtual AX_BOOL UnRegisterSensor(AX_U8 nPipe);
    virtual AX_BOOL ResetSensorObj(AX_U8 nDevId, AX_U8 nPipe);
    AX_VOID IspLoopThreadFunc(SENSOR_PIPE_MAPPING_T* pPipeMapping);
    AX_BOOL SetAeToManual(AX_U8 nPipe);

private:
    AX_BOOL InitISP();
    AX_S32 CalcValueToIspIQ(AX_S32 nVal, AX_U8 nIntBits, AX_U8 nFracBits);
    AX_F32 CalcIspIQToValue(AX_S32 nVal, AX_U8 nIntBits, AX_U8 nFracBits);
    AX_S32 COMMON_HW_GetSensorResetGpioNum(AX_U8 nDevId);
    AX_S32 COMMON_HW_GetApdPlateId(AX_CHAR* apd_plate_id);

protected:
    SENSOR_CONFIG_T m_tSnsCfg;
    AX_VOID* m_pSnsLib{nullptr};
    AX_SENSOR_REGISTER_FUNC_T* m_pSnsObj{nullptr};
    AX_SNS_CONNECT_TYPE_E m_eSnsBusType{ISP_SNS_CONNECT_I2C_TYPE};
    SNS_LIB_INFO_T m_tSnsLibInfo;

    AX_SNS_ATTR_T m_tSnsAttr;
    SNS_CLK_ATTR_T m_tSnsClkAttr;
    AX_VIN_DEV_ATTR_T m_tDevAttr;
    AX_VIN_PRIVATE_DATA_ATTR_T m_tPrivAttr;
    std::map<AX_U8, AX_VIN_PIPE_ATTR_T> m_mapPipe2Attr;
    AX_MIPI_RX_DEV_T m_tMipiRxDev;
    std::map<AX_U8, std::map<AX_U8, AX_VIN_CHN_ATTR_T>> m_mapPipe2ChnAttr;
    SNS_ABILITY_T m_tAbilities;
    SNS_SNAP_ATTR_T m_tSnapAttr;

    AX_IMG_FORMAT_E m_eImgFormatSDR;
    AX_IMG_FORMAT_E m_eImgFormatHDR;

    CIspAlgoWrapper m_algWrapper;

    SensorAttrUpdCallback m_cbAttrUpd{nullptr};

    SENSOR_PIPE_MAPPING_T m_arrPipeMapping[MAX_PIPE_PER_DEVICE];
    AX_BOOL m_arrIspThreadRunning[MAX_PIPE_PER_DEVICE]{AX_FALSE, AX_FALSE, AX_FALSE};
    std::thread m_arrIspLoopThread[MAX_PIPE_PER_DEVICE];
    APP_ISP_IMAGE_ATTR_T m_tImageAttr;

    SnapshotProcCallback m_cbSanpshotProc{nullptr};
};
