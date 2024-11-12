/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "AlgoCfgParser.h"
#include "AlgoOptionHelper.h"
#include "CommonUtils.hpp"

AX_BOOL CAlgoOptionHelper::InitOnce() {
    memset(m_stAlgoParam, 0x00, sizeof(m_stAlgoParam));
    memset(&m_stAudioParam, 0x00, sizeof(m_stAudioParam));

    // default
    for (size_t i = 0; i < AX_APP_ALGO_SNS_MAX; i ++) {
        m_stAlgoParam[i].stHvcfpParam.stPushStrategy.ePushMode = AX_APP_ALGO_PUSH_MODE_BEST;
        m_stAlgoParam[i].stHvcfpParam.stPushStrategy.nInterval = 2000;
        m_stAlgoParam[i].stHvcfpParam.stPushStrategy.nPushCount = 3;
        m_stAlgoParam[i].stHvcfpParam.nCropEncoderQpLevel = AX_APP_ALGO_DETECT_DEFAULT_QP_LEVEL;
        m_stAlgoParam[i].stHvcfpParam.nFramerate = AX_APP_ALGO_DETECT_DEFAULT_FRAMERATE;
    }

    return CAlgoCfgParser::GetInstance()->GetConfig(m_stAlgoParam, m_stAudioParam);
}

const AX_APP_ALGO_PARAM_T& CAlgoOptionHelper::GetAlgoParam(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId];
}

const AX_APP_ALGO_HVCFP_PARAM_T& CAlgoOptionHelper::GetHvcfpParam(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stHvcfpParam;
}

const AX_APP_ALGO_MOTION_PARAM_T& CAlgoOptionHelper::GetMDParam(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stMotionParam;
}

const AX_APP_ALGO_OCCLUSION_PARAM_T& CAlgoOptionHelper::GetODParam(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stOcclusionParam;
}

const AX_APP_ALGO_SCENE_CHANGE_PARAM_T& CAlgoOptionHelper::GetSCDParam(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stSceneChangeParam;
}

const AX_APP_ALGO_FACE_RECOGNIZE_PARAM_T& CAlgoOptionHelper::GetFaceRecognizeParam(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stFaceRecognizeParam;
}

const AX_APP_ALGO_AUDIO_PARAM_T& CAlgoOptionHelper::GetAudioParam(AX_VOID) {
    return m_stAudioParam;
}

AX_VOID CAlgoOptionHelper::SetAlgoParam(AX_U32 nSnsId, const AX_APP_ALGO_PARAM_T& stParam) {
    m_stAlgoParam[nSnsId] = stParam;

    if (stParam.nAlgoType & AX_APP_ALGO_MOTION_DETECT) {
        m_stAlgoParam[nSnsId].stMotionParam.bEnable = AX_TRUE;
    }
    else {
        m_stAlgoParam[nSnsId].stMotionParam.bEnable = AX_FALSE;
    }

    if (stParam.nAlgoType & AX_APP_ALGO_OCCLUSION_DETECT) {
        m_stAlgoParam[nSnsId].stOcclusionParam.bEnable = AX_TRUE;
    }
    else {
        m_stAlgoParam[nSnsId].stOcclusionParam.bEnable = AX_FALSE;
    }

    if (stParam.nAlgoType & AX_APP_ALGO_SCENE_CHANGE_DETECT) {
        m_stAlgoParam[nSnsId].stSceneChangeParam.bEnable = AX_TRUE;
    }
    else {
        m_stAlgoParam[nSnsId].stSceneChangeParam.bEnable = AX_FALSE;
    }

    if (stParam.nAlgoType & AX_APP_ALGO_SOUND_DETECT) {
        m_stAudioParam.bEnable = AX_TRUE;
    }
    else {
        m_stAudioParam.bEnable = AX_FALSE;
    }
}

AX_VOID CAlgoOptionHelper::SetHvcfpParam(AX_U32 nSnsId, const AX_APP_ALGO_HVCFP_PARAM_T& stParam) {
    m_stAlgoParam[nSnsId].stHvcfpParam = stParam;
}

AX_VOID CAlgoOptionHelper::SetMDParam(AX_U32 nSnsId, const AX_APP_ALGO_MOTION_PARAM_T& stParam) {
    m_stAlgoParam[nSnsId].stMotionParam = stParam;
}

AX_VOID CAlgoOptionHelper::SetODParam(AX_U32 nSnsId, const AX_APP_ALGO_OCCLUSION_PARAM_T& stParam) {
    m_stAlgoParam[nSnsId].stOcclusionParam = stParam;
}

AX_VOID CAlgoOptionHelper::SetSCDParam(AX_U32 nSnsId, const AX_APP_ALGO_SCENE_CHANGE_PARAM_T& stParam) {
    m_stAlgoParam[nSnsId].stSceneChangeParam = stParam;
}

AX_VOID CAlgoOptionHelper::SetFaceRecognizeParam(AX_U32 nSnsId, const AX_APP_ALGO_FACE_RECOGNIZE_PARAM_T& stParam) {
    m_stAlgoParam[nSnsId].stFaceRecognizeParam = stParam;
}

AX_VOID CAlgoOptionHelper::SetAudioParam(const AX_APP_ALGO_AUDIO_PARAM_T& stParam) {
    m_stAudioParam = stParam;
}

AX_U32 CAlgoOptionHelper::GetDetectAlgoType(AX_U32 nSnsId) {
    AX_S32 nDetectAlgoType = 0;

    if (m_stAlgoParam[nSnsId].nAlgoType & (AX_APP_ALGO_FACE_DETECT | AX_APP_ALGO_FACE_RECOGNIZE)) {
        nDetectAlgoType = AX_SKEL_PPL_FH;
    }
#if 0
    else if (m_stAlgoParam.nAlgoType & AX_APP_ALGO_LICENSE_PLATE_RECOGNIZE) {
        nDetectAlgoType = AX_SKEL_PPL_HVCP;
    }
#endif
    else if (m_stAlgoParam[nSnsId].nAlgoType & AX_APP_ALGO_TYPE_HVCFP) {
        nDetectAlgoType = AX_SKEL_PPL_HVCFP;
    }

    return nDetectAlgoType;
}

AX_BOOL CAlgoOptionHelper::IsEnableMD(AX_U32 nSnsId) {
    if (m_stAlgoParam[nSnsId].stMotionParam.bEnable
        && m_stAlgoParam[nSnsId].stMotionParam.nRegionSize > 0) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CAlgoOptionHelper::IsEnableOD(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stOcclusionParam.bEnable;
}

AX_BOOL CAlgoOptionHelper::IsEnableSCD(AX_U32 nSnsId) {
    return m_stAlgoParam[nSnsId].stSceneChangeParam.bEnable;
}

AX_U32 CAlgoOptionHelper::GetDetectAlgoType(AX_VOID) {
    for (AX_U32 i = 0; i < AX_APP_ALGO_SNS_MAX; i ++) {
        AX_S32 nDetectAlgoType = GetDetectAlgoType(i);

        if (nDetectAlgoType != 0) {
            return nDetectAlgoType;
        }
    }

    return 0;
}

AX_BOOL CAlgoOptionHelper::IsEnableMD(AX_VOID) {
    for (AX_U32 i = 0; i < AX_APP_ALGO_SNS_MAX; i ++) {
        AX_BOOL bEnable = IsEnableMD(i);

        if (bEnable) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_BOOL CAlgoOptionHelper::IsEnableOD(AX_VOID) {
    for (AX_U32 i = 0; i < AX_APP_ALGO_SNS_MAX; i ++) {
        AX_BOOL bEnable = IsEnableOD(i);

        if (bEnable) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_BOOL CAlgoOptionHelper::IsEnableSCD(AX_VOID) {
    for (AX_U32 i = 0; i < AX_APP_ALGO_SNS_MAX; i ++) {
        AX_BOOL bEnable = IsEnableSCD(i);

        if (bEnable) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}
