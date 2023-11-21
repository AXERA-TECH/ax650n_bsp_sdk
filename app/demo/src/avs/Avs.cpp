/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "Avs.h"
#include "ax_avs_api.h"
#include "AppLogApi.h"
#include <thread>
#include <chrono>

#define TAG "AVS"
#define DEFAULT_AVS_CHN_DEPTH   (2)

extern string g_SDKVersion;

AX_BOOL CAvs::Init(const AX_APP_AVS_ATTR_T& stAvsAttr) {
    memset((AX_VOID *)&m_stAvsGrpAttr, 0, sizeof(AX_AVS_GRP_ATTR_T));
    memset((AX_VOID *)&m_stAvsChnAttr, 0, sizeof(AX_AVS_CHN_ATTR_T));

    m_SDKVersion = g_SDKVersion;
    m_stAvsAttr = stAvsAttr;

    m_stAvsChnAttr.u32Depth = DEFAULT_AVS_CHN_DEPTH;
    m_stAvsChnAttr.stCompressInfo = stAvsAttr.stAvsCompress;
    m_stAvsChnAttr.bBlockEnable = AX_FALSE;

    m_stAvsGrpAttr.u32PipeNum = (AX_U32)m_stAvsAttr.u8PipeNum;
    m_stAvsGrpAttr.bSyncPipe = m_stAvsAttr.bSyncPipe;
    m_stAvsGrpAttr.enMode = m_stAvsAttr.enMode;
    m_stAvsGrpAttr.bDynamicSeam = m_stAvsAttr.bDynamicSeam;
    m_stAvsGrpAttr.enBlendMode = m_stAvsAttr.enBlendMode;

    for (auto& e : m_stAvsMeshAddr) {
        e.pVirAddr = nullptr;
        e.u64PhyAddr = 0x0;
    }

    for (auto& e : m_stAvsMaskAddr) {
        e.pVirAddr = nullptr;
        e.u64PhyAddr = 0x0;
    }

    switch(m_stAvsAttr.enParamType) {
        case E_AVS_PARAM_TYPE_NORMAL:
            m_stAvsGrpAttr.enCalibrationMode = AVS_CALIBRATION_PARAM_CAMERA;
            m_stAvsGrpAttr.stGrpCameraParam.enCameraType = AVS_CAMERA_TYPE_PINHOLE;
            m_stAvsGrpAttr.stOutAttr.enPrjMode = m_stAvsAttr.enProjectionType;
            break;
        case E_AVS_PARAM_TYPE_MESHTABLE:
            m_stAvsGrpAttr.enCalibrationMode = AVS_CALIBRATION_PARAM_TRANSFORM;
            break;
        default:
            LOG_M_E(TAG, "Unsupported avs parameter type 0x%x", m_stAvsAttr.enParamType);
            return AX_FALSE;
    }

    AX_S32 s32Ret = LoadParam();
    if (AX_SUCCESS != s32Ret) {
        if (AX_ERR_AVSCALI_DATA_IMCOMPATIBLE == s32Ret) {
            LOG_M_E(TAG, "Broken avs parameters, need to enable cali, set enCalibrationMode(AVS_CALIBRATION_PARAM_CAMERA) and recalibrate");
        }

        if (!(AX_ERR_AVSCALI_DATA_IMCOMPATIBLE == s32Ret && m_stAvsAttr.u8CaliEnable)) {
            return AX_FALSE;
        }
    }

    m_stAvsChnAttr.stOutAttr.u32Width = m_stAvsGrpAttr.stOutAttr.u32Width;
    m_stAvsChnAttr.stOutAttr.u32Height = m_stAvsGrpAttr.stOutAttr.u32Height;

    return AX_TRUE;
}

AX_BOOL CAvs::DeInit(AX_BOOL bDeInitCali/* = AX_FALSE */) {
    AX_S32 s32Ret { -1 };

    s32Ret = AX_AVS_DestroyGrp(m_nGrp);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_DestroyGrp failed, s32Ret 0x%x", s32Ret);
    }

    for (auto& e : m_stAvsMeshAddr) {
        if (nullptr != e.pVirAddr) {
            s32Ret = AX_SYS_MemFree(e.u64PhyAddr, e.pVirAddr);
            if (AX_SUCCESS != s32Ret)
                LOG_M_E(TAG, "AX_SYS_MemFree failed, s32Ret 0x%x", s32Ret);
        }
    }

    for (auto& e : m_stAvsMaskAddr) {
        if (nullptr != e.pVirAddr)  {
            s32Ret = AX_SYS_MemFree(e.u64PhyAddr, e.pVirAddr);
            if (AX_SUCCESS != s32Ret) {
                LOG_M_E(TAG, "AX_SYS_MemFree failed, s32Ret 0x%x", s32Ret);
            }
        }
    }

    if (m_stAvsAttr.u8CaliEnable && bDeInitCali) {
       s32Ret = AX_AVSCALI_DeInit();
       if (AX_SUCCESS != s32Ret) {
            LOG_M_W(TAG, "AX_AVSCALI_DeInit failed, s32Ret 0x%x", s32Ret);
       }
    }

    return AX_TRUE;
}

AX_BOOL CAvs::Start(AX_VOID) {
    AX_S32 s32Ret {-1};

    s32Ret = AX_AVS_CreateGrp(m_nGrp, &m_stAvsGrpAttr);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_CreateGrp failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    s32Ret = AX_AVS_SetChnAttr(m_nGrp, m_nChn, &m_stAvsChnAttr);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_SetChnAttr failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    s32Ret = AX_AVS_EnableChn(m_nGrp, m_nChn);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_EnableChn failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    s32Ret = AX_AVS_StartGrp(m_nGrp);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_StartGrp failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAvs::Stop(AX_BOOL bStopCali/* = AX_FALSE*/) {
    AX_S32 s32Ret {-1};

    s32Ret = AX_AVS_DisableChn(m_nGrp, m_nChn);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_W(TAG, "AX_AVS_DisableChn failed, s32Ret 0x%x", s32Ret);
    }

    s32Ret = AX_AVS_StopGrp(m_nGrp);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_W(TAG, "AX_AVS_StopGrp failed, s32Ret 0x%x", s32Ret);
    }

    if (m_stAvsAttr.u8CaliEnable && bStopCali) {
        s32Ret = AX_AVSCALI_Stop();
        if (AX_SUCCESS != s32Ret) {
            LOG_M_W(TAG, "AX_AVSCALI_Stop failed, s32Ret 0x%x", s32Ret);
        }
    }

    return AX_TRUE;
}

AX_BOOL CAvs::UpdateParam(AX_VOID) {
    m_stAvsChnAttr.stOutAttr.u32Width = m_stAvsGrpAttr.stOutAttr.u32Width;
    m_stAvsChnAttr.stOutAttr.u32Height = m_stAvsGrpAttr.stOutAttr.u32Height;
    return AX_TRUE;
}

AX_S32 CAvs::LoadParam(AX_VOID) {
    AX_S32 s32Ret { -1 };
    string strCaliDataPath;
    if (m_strCaliDataPath.empty()) {
        strCaliDataPath = m_stAvsAttr.tCaliInitParam.strCaliDataPath;
    } else {
        strCaliDataPath = m_strCaliDataPath;
    }
    s32Ret = AX_AVSCALI_LoadParam(strCaliDataPath.c_str(), &m_stAvsGrpAttr, nullptr, &m_bIsCalibrated);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "Failed to load calibration parameters, s32Ret=0x%x", s32Ret);
        return s32Ret;
    }

    g_SDKVersion = m_SDKVersion + (m_bIsCalibrated ? " Calibrated" : " UnCalibrated");

    if (AVS_CALIBRATION_PARAM_TRANSFORM == m_stAvsGrpAttr.enCalibrationMode) {
        for (AX_U8 i = 0; i < m_stAvsGrpAttr.u32PipeNum; i++) {
            AX_U32 u32MeshFileSize = m_stAvsGrpAttr.stGrpTransformParam.stPipeMesh.s32MeshSize[i];
            s32Ret = AX_SYS_MemAlloc(&m_stAvsMeshAddr[i].u64PhyAddr, &m_stAvsMeshAddr[i].pVirAddr, u32MeshFileSize, 128, (const AX_S8 *)"avs_mesh");
            if (AX_SUCCESS != s32Ret) {
                LOG_M_E(TAG, "AX_SYS_MemAlloc memory for [%d] mesh data failed, s32Ret 0x%x", i, s32Ret);
                return s32Ret;
            }
            m_stAvsGrpAttr.stGrpTransformParam.stPipeMesh.pstVirAddr[i] = m_stAvsMeshAddr[i].pVirAddr;

            if (i < (m_stAvsGrpAttr.u32PipeNum - 1)) {
                AX_U32 u32MaskFileSize = m_stAvsGrpAttr.stGrpTransformParam.stMask.s32MaskSize[i];
                s32Ret = AX_SYS_MemAlloc(&m_stAvsMaskAddr[i].u64PhyAddr, &m_stAvsMaskAddr[i].pVirAddr, u32MaskFileSize, 128, (const AX_S8 *)"avs_mask");
                if (AX_SUCCESS != s32Ret) {
                    LOG_M_E(TAG, "AX_SYS_MemAlloc memory for [%d] mesh data failed, s32Ret 0x%x", i, s32Ret);
                    for (auto& e: m_stAvsMeshAddr) {
                        AX_SYS_MemFree(e.u64PhyAddr, e.pVirAddr);
                    }
                    return s32Ret;
                }
                m_stAvsGrpAttr.stGrpTransformParam.stMask.pstVirAddr[i] = m_stAvsMaskAddr[i].pVirAddr;
            }
        }

        s32Ret = AX_AVSCALI_LoadParam(strCaliDataPath.c_str(), &m_stAvsGrpAttr, nullptr, nullptr);
        if (AX_SUCCESS != s32Ret) {
            LOG_M_E(TAG, "failed to load calibration parameters, s32Ret 0x%x", s32Ret);
            return s32Ret;
        }
    }

    m_stAvsResolution.u32Width  = m_stAvsGrpAttr.stOutAttr.u32Width;
    m_stAvsResolution.u32Height = m_stAvsGrpAttr.stOutAttr.u32Height;

    LOG_M_D(TAG, "tOutFrame size: (%d, %d)", m_stAvsGrpAttr.stOutAttr.u32Width, m_stAvsGrpAttr.stOutAttr.u32Height);

    return s32Ret;
}

AX_BOOL CAvs::StartAVSCalibrate() {
    if (!m_stAvsAttr.u8CaliEnable) {
        return AX_TRUE;
    }

    LOG_M_I(TAG, "Start Calibrate, snsNum: %d, pipe0 id: %d, pipe1 id: %d, master pipe: %d, isp chn: %d, width: %d, height: %d, avs cali param path: %s, ip:%s, port:%d",
            m_stAvsAttr.tCaliInitParam.tSnsInfo.nSnsNum,
            m_stAvsAttr.tCaliInitParam.tSnsInfo.arrPipeId[0], m_stAvsAttr.tCaliInitParam.tSnsInfo.arrPipeId[1],
            m_stAvsAttr.tCaliInitParam.tSnsInfo.nMasterPipeId, m_stAvsAttr.tCaliInitParam.tSnsInfo.nChn,
            m_stAvsAttr.tCaliInitParam.tSnsInfo.nImgWidth, m_stAvsAttr.tCaliInitParam.tSnsInfo.nImgHeight,
            m_stAvsAttr.tCaliInitParam.strCaliDataPath,
            m_stAvsAttr.strCaliServerIP.c_str(), m_stAvsAttr.u16CaliServerPort);

    AX_S32 s32Ret{-1};
    s32Ret = AX_AVSCALI_Init(&m_stAvsAttr.tCaliInitParam);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVSCALI_Init failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    s32Ret = AX_AVSCALI_Start(m_stAvsAttr.strCaliServerIP.c_str(), m_stAvsAttr.u16CaliServerPort);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVSCALI_Start failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_S32 CAvs::Get3ASyncRatio(AX_AVSCALI_3A_SYNC_RATIO_T& t3ASyncRatio) {
    string strCaliDataPath;
    if (m_strCaliDataPath.empty()) {
        strCaliDataPath = m_stAvsAttr.tCaliInitParam.strCaliDataPath;
    } else {
        strCaliDataPath = m_strCaliDataPath;
    }

    AX_S32 nRet = AX_AVSCALI_LoadParam(strCaliDataPath.c_str(), nullptr, &t3ASyncRatio, nullptr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(TAG, "Failed to get 3a sync ratio, nRet: 0x%x", nRet);
    }

    return nRet;
}
