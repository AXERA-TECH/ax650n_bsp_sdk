/**********************************************************************************
 *
 * Copyright (c) 2019-2022 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#include "Scd.h"
#include <string.h>
#include <string>
#include "AppLog.hpp"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "AlgoOptionHelper.h"
#include "IvesResult.hpp"

#define SCD "SCD"

AX_BOOL CSCD::LoadConfig(AX_S32 nSnsId) {
    return AX_TRUE;
}

AX_BOOL CSCD::Startup(AX_U32 nWidth, AX_U32 nHeight, AX_S32 nSnsId) {
    LOG_MM_C(SCD, "+++");
    if (-1 == nSnsId) {
        LOG_M_E(SCD, "Invalid sensorID:%d.", nSnsId);
        return AX_FALSE;
    }

    LoadConfig(nSnsId);

    AX_SCD_CHN_ATTR_T stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.chn = nSnsId;
    stChnAttr.stArea.u32X = 0;
    stChnAttr.stArea.u32Y = 0;
    stChnAttr.stArea.u32W = nWidth;
    stChnAttr.stArea.u32H = nHeight;
    stChnAttr.u8Thrd = (AX_U8)ALGO_SCD_PARAM(nSnsId).fThreshold;
    stChnAttr.u8Confidence = (AX_U8)ALGO_SCD_PARAM(nSnsId).fConfidence;

    AX_S32 ret = AX_IVES_SCD_CreateChn(stChnAttr.chn, &stChnAttr);
    if (0 != ret) {
        LOG_MM_E(SCD, "AX_IVES_SCD_CreateChn(chn: %d) fail, ret = 0x%x", stChnAttr.chn, ret);
        return AX_FALSE;
    }
    m_chn = nSnsId;

    LOG_MM_C(SCD, "---");
    return AX_TRUE;
}

AX_VOID CSCD::Cleanup(AX_VOID) {
    LOG_MM_C(SCD, "+++");

    if (-1 != m_chn) {
        LOG_MM_C(SCD, "AX_IVES_SCD_DestoryChn[%d]", m_chn);
        AX_IVES_SCD_DestoryChn(m_chn);
        m_chn = -1;
    }
    LOG_MM_C(SCD, "---");
}

AX_BOOL CSCD::ProcessFrame(const AX_U32 nSnsId, const CAXFrame *pFrame) {
    if (m_chn < 0) {
        LOG_M_E(SCD, "SCD channel is not created yet.");
        return AX_FALSE;
    }

    const AX_IVES_IMAGE_T *pImage = (const AX_IVES_IMAGE_T *)&pFrame->stFrame.stVFrame;

    AX_U32 nScdCount = 0;
    AX_APP_ALGO_IVES_ITEM_T stScds[MAX_IVES_SCD_RESULT_COUNT];
    memset(stScds, 0x00, sizeof(stScds));

    AX_U8 nChanged;
    AX_S32 ret = AX_IVES_SCD_Process(m_chn, pImage, &nChanged);
    if (0 != ret) {
        LOG_M_E(SCD, "AX_IVES_SCD_Process(frame: %lld) fail, ret = 0x%x", pImage->u64SeqNum, ret);
        return AX_FALSE;
    }
    if (1 == nChanged) {
        stScds[0].eType = AX_APP_ALGO_IVES_SCENE_CHANGE;
        stScds[0].u64FrameId = pFrame->stFrame.stVFrame.stVFrame.u64SeqNum;
        stScds[0].fConfidence = (AX_F32)ALGO_SCD_PARAM(nSnsId).fConfidence;
        stScds[0].stBox.fX = 0.;
        stScds[0].stBox.fY = 0.;
        stScds[0].stBox.fW = 1.;
        stScds[0].stBox.fH = 1.;

        stScds[0].stBox.nImgWidth = pFrame->stFrame.stVFrame.stVFrame.u32Width;
        stScds[0].stBox.nImgHeight = pFrame->stFrame.stVFrame.stVFrame.u32Height;

        stScds[0].stPanoramaImg.bExist = AX_FALSE;

        nScdCount = 1;
    }

    IVES_RESULT_T ives;
    CIvesResult::GetInstance()->Get(nSnsId, ives);

    ives.nScdCount = nScdCount;
    memcpy(ives.stScds, stScds, sizeof(ives.stScds));

    CIvesResult::GetInstance()->Set(nSnsId, ives);

    return AX_TRUE;
}

AX_VOID CSCD::SetThreshold(AX_U32 nSnsId, AX_S32 nThreshold, AX_S32 nConfidence) {
    if (m_chn >= 0) {
        AX_SCD_CHN_ATTR_T stChnAttr;
        AX_S32 ret = AX_IVES_SCD_GetChnAttr(m_chn, &stChnAttr);
        if (0 != ret) {
            LOG_M_E(SCD, "AX_IVES_SCD_GetChnAttr(chn: %d) fail, ret = 0x%x", m_chn, ret);
            return;
        }

        stChnAttr.u8Thrd = (AX_U8)nThreshold;
        stChnAttr.u8Confidence = (AX_U8)nConfidence;
        ret = AX_IVES_SCD_SetChnAttr(m_chn, &stChnAttr);
        if (0 != ret) {
            LOG_M_E(SCD, "AX_IVES_SCD_SetChnAttr(chn: %d) fail, ret = 0x%x", m_chn, ret);
            return;
        }
    }

    // update param
    {
        AX_APP_ALGO_SCENE_CHANGE_PARAM_T stParam = ALGO_SCD_PARAM(nSnsId);
        stParam.fThreshold = (AX_F32)nThreshold;
        stParam.fConfidence = (AX_F32)nConfidence;

        SET_ALGO_SCD_PARAM(nSnsId, stParam);
    }
}

AX_VOID CSCD::GetThreshold(AX_U32 nSnsId, AX_S32 &nThreshold, AX_S32 &nConfidence) const {
    nThreshold = (AX_S32)ALGO_SCD_PARAM(nSnsId).fThreshold;
    nConfidence = (AX_S32)ALGO_SCD_PARAM(nSnsId).fConfidence;
}
