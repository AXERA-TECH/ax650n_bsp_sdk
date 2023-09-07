/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "PanoAVS.h"
#include "ax_avs_api.h"
#include "AppLogApi.h"
#include <thread>
#include <chrono>

#define TAG "PanoAVS"

AX_BOOL CPanoAVS::Init(const PANO_AVS_ATTR_T& stPanoAVSAttr) {
    m_stAttr = stPanoAVSAttr;

    m_stGrpAttr.u32PipeNum = (AX_U32)m_stAttr.u8PipeNum;
    m_stGrpAttr.bSyncPipe = m_stAttr.bSyncPipe;
    m_stGrpAttr.enMode = m_stAttr.enMode;
    m_stGrpAttr.bDynamicSeam = m_stAttr.bDynamicSeam;
    m_stGrpAttr.enBlendMode = m_stAttr.enBlendMode;

    for (auto& e : m_stMeshAddr) {
        e.pVirAddr = nullptr;
        e.u64PhyAddr = 0x0;
    }

    for (auto& e : m_stMaskAddr) {
        e.pVirAddr = nullptr;
        e.u64PhyAddr = 0x0;
    }

    switch(m_stAttr.enParamType) {
        case E_AVS_PARAM_TYPE_NORMAL:
            m_stGrpAttr.enCalibrationMode = AVS_CALIBRATION_PARAM_CAMERA;
            m_stGrpAttr.stGrpCameraParam.enCameraType = AVS_CAMERA_TYPE_PINHOLE;
            m_stGrpAttr.stOutAttr.enPrjMode = m_stAttr.enProjectionType;
            break;
        case E_AVS_PARAM_TYPE_MESHTABLE:
            m_stGrpAttr.enCalibrationMode = AVS_CALIBRATION_PARAM_TRANSFORM;
            break;
        default:
            LOG_M_E(TAG, "unsupported avs parameter type 0x%x", m_stAttr.enParamType);
            return AX_FALSE;
    }

    AX_S32 s32Ret { -1 };
    s32Ret = LoadParam();
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "load avs parameters failed");
        if (AX_ERR_AVSCALI_DATA_IMCOMPATIBLE == s32Ret) {
            LOG_M_E(TAG, "broken avs parameters, need to set enCalibrationMode(AVS_CALIBRATION_PARAM_CAMERA) and recalibrate");
            return AX_FALSE;
        }
    }

    m_stAVSChnAttr.u32Depth = 4;
    m_stAVSChnAttr.stOutAttr.u32Width = m_stGrpAttr.stOutAttr.u32Width;
    m_stAVSChnAttr.stOutAttr.u32Height = m_stGrpAttr.stOutAttr.u32Height;

    if (m_stAttr.u8CaliEnable) {
        s32Ret = CalibrateSensors(m_stAttr.arrPipeResolution[0][0], m_stAttr.arrPipeResolution[0][1]);
        if (AX_SUCCESS != s32Ret) {
            LOG_M_E(TAG, "CalibrateSns failed, s32Ret 0x%x", s32Ret);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CPanoAVS::DeInit(AX_BOOL bDeInitCali/* = AX_FALSE */) {
    AX_S32 s32Ret { -1 };

    s32Ret = AX_AVS_DestroyGrp(m_nGrp);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_DestroyGrp failed, s32Ret 0x%x", s32Ret);
    }

    for (auto& e : m_stMeshAddr) {
        if (nullptr != e.pVirAddr) {
            s32Ret = AX_SYS_MemFree(e.u64PhyAddr, e.pVirAddr);
            if (AX_SUCCESS != s32Ret)
                LOG_M_E(TAG, "AX_SYS_MemFree failed, s32Ret 0x%x", s32Ret);
        }
    }

    for (auto& e : m_stMaskAddr) {
        if (nullptr != e.pVirAddr)  {
            s32Ret = AX_SYS_MemFree(e.u64PhyAddr, e.pVirAddr);
            if (AX_SUCCESS != s32Ret) {
                LOG_M_E(TAG, "AX_SYS_MemFree failed, s32Ret 0x%x", s32Ret);
            }
        }
    }

    if (m_stAttr.u8CaliEnable && bDeInitCali) {
        AX_AVSCALI_DeInit();
    }

    return AX_TRUE;
}

AX_BOOL CPanoAVS::Start(AX_VOID) {
    AX_S32 s32Ret {-1};

    s32Ret = AX_AVS_CreateGrp(m_nGrp, &m_stGrpAttr);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_CreateGrp failed, s32Ret 0x%x", s32Ret);
        return AX_FALSE;
    }

    s32Ret = AX_AVS_SetChnAttr(m_nGrp, m_nChn, &m_stAVSChnAttr);
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

AX_BOOL CPanoAVS::Stop(AX_BOOL bStopCali/* = AX_FALSE*/) {
    AX_S32 s32Ret {-1};

    s32Ret = AX_AVS_DisableChn(m_nGrp, m_nChn);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_DisableChn failed, s32Ret 0x%x", s32Ret);
    }

    s32Ret = AX_AVS_StopGrp(m_nGrp);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVS_StopGrp failed, s32Ret 0x%x", s32Ret);
    }

    if (m_stAttr.u8CaliEnable && bStopCali) {
        AX_AVSCALI_Stop();
    }

    return AX_TRUE;
}

AX_BOOL CPanoAVS::UpdateParam(AX_VOID) {
    m_stAVSChnAttr.stOutAttr.u32Width = m_stGrpAttr.stOutAttr.u32Width;
    m_stAVSChnAttr.stOutAttr.u32Height = m_stGrpAttr.stOutAttr.u32Height;
    return AX_TRUE;
}

AX_S32 CPanoAVS::LoadParam(AX_VOID) {
    AX_S32 s32Ret { -1 };
    std::string strCaliDataPath;
    if (m_strCaliDataPath.empty()) {
        strCaliDataPath = m_stAttr.strParamFilePath;
    } else {
        strCaliDataPath = m_strCaliDataPath;
    }
    s32Ret = AX_AVSCALI_LoadParam(strCaliDataPath.c_str(), &m_stGrpAttr, nullptr, &m_bIsCalibrated);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "failed to load calibration parameters, s32Ret 0x%x", s32Ret);
        return s32Ret;
    }

    if (AVS_CALIBRATION_PARAM_TRANSFORM == m_stGrpAttr.enCalibrationMode) {
        for (AX_U8 i = 0; i < m_stGrpAttr.u32PipeNum; i++) {
            AX_U32 u32MeshFileSize = m_stGrpAttr.stGrpTransformParam.stPipeMesh.s32MeshSize[i];
            s32Ret = AX_SYS_MemAlloc(&m_stMeshAddr[i].u64PhyAddr, &m_stMeshAddr[i].pVirAddr, u32MeshFileSize, 128, (const AX_S8 *)"pano_avs_mesh");
            if (AX_SUCCESS != s32Ret) {
                LOG_M_E(TAG, "AX_SYS_MemAlloc memory for [%d] mesh data failed, s32Ret 0x%x", i, s32Ret);
                return s32Ret;
            }
            m_stGrpAttr.stGrpTransformParam.stPipeMesh.pstVirAddr[i] = m_stMeshAddr[i].pVirAddr;

            if (i < (m_stGrpAttr.u32PipeNum - 1)) {
                AX_U32 u32MaskFileSize = m_stGrpAttr.stGrpTransformParam.stMask.s32MaskSize[i];
                s32Ret = AX_SYS_MemAlloc(&m_stMaskAddr[i].u64PhyAddr, &m_stMaskAddr[i].pVirAddr, u32MaskFileSize, 128, (const AX_S8 *)"pano_avs_mask");
                if (AX_SUCCESS != s32Ret) {
                    LOG_M_E(TAG, "AX_SYS_MemAlloc memory for [%d] mesh data failed, s32Ret 0x%x", i, s32Ret);
                    for (auto& e: m_stMeshAddr) {
                        AX_SYS_MemFree(e.u64PhyAddr, e.pVirAddr);
                    }
                    return s32Ret;
                }
                m_stGrpAttr.stGrpTransformParam.stMask.pstVirAddr[i] = m_stMaskAddr[i].pVirAddr;
            }
        }
        s32Ret = AX_AVSCALI_LoadParam(strCaliDataPath.c_str(), &m_stGrpAttr, nullptr, nullptr);
        if (AX_SUCCESS != s32Ret) {
            LOG_M_E(TAG, "failed to load calibration parameters, s32Ret 0x%x", s32Ret);
            return s32Ret;
        }
    }

    m_stCaliReso.u32Width = m_stGrpAttr.stOutAttr.u32Width;
    m_stCaliReso.u32Height = m_stGrpAttr.stOutAttr.u32Height;

    LOG_M_D(TAG, "tOutFrame size: (%d, %d)", m_stGrpAttr.stOutAttr.u32Width, m_stGrpAttr.stOutAttr.u32Height);

    return s32Ret;
}

AX_S32 CPanoAVS::CalibrateSensors(AX_U32 width, AX_U32 height) {
    AX_AVSCALI_INIT_PARAM_T stCaliParam = {0};
    stCaliParam.tSnsInfo.nSnsNum = m_stAttr.u8PipeNum;
    for (AX_U32 i = 0; i < m_stAttr.u8PipeNum; i++) {
        stCaliParam.tSnsInfo.arrPipeId[i] = m_stAttr.arrPipeId[i];
    }
    stCaliParam.tSnsInfo.nMasterPipeId = m_stAttr.u8MasterPipeId;
    stCaliParam.tSnsInfo.nImgWidth = width;
    stCaliParam.tSnsInfo.nImgHeight = height;
    stCaliParam.tSnsInfo.nChn = 0;
    stCaliParam.tCallbacks.CaliDoneCb = m_stAttr.pCaliDone;

    AX_U16 u16PathLen = m_stAttr.strParamFilePath.length() > (AX_AVSCALI_MAX_PATH_LEN - 1) ? (AX_AVSCALI_MAX_PATH_LEN - 1) : m_stAttr.strParamFilePath.length();
    memcpy((void *)stCaliParam.strCaliDataPath,  m_stAttr.strParamFilePath.c_str(), u16PathLen);

    LOG_M_D(TAG, "snsNum: %d, pipe0 id: %d, pipe1 id: %d, width: %d, height: %d, avs cali param path: %s, ip:%s, port:%d",\
            stCaliParam.tSnsInfo.nSnsNum, stCaliParam.tSnsInfo.arrPipeId[0], stCaliParam.tSnsInfo.arrPipeId[1],\
            stCaliParam.tSnsInfo.nImgWidth, stCaliParam.tSnsInfo.nImgHeight, stCaliParam.strCaliDataPath,\
            m_stAttr.strCaliServerIP.c_str(), m_stAttr.u16CaliServerPort);

    AX_S32 s32Ret { -1 };
    s32Ret = AX_AVSCALI_Init(&stCaliParam);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVSCALI_Init failed, s32Ret 0x%x", s32Ret);
        return s32Ret;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    s32Ret = AX_AVSCALI_Start(m_stAttr.strCaliServerIP.c_str(), m_stAttr.u16CaliServerPort);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(TAG, "AX_AVSCALI_Start failed, s32Ret 0x%x", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}