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
#include "IObserver.h"
#include "OptionHelper.h"
#include "PrintHelper.h"
#include "OsdOptionHelper.h"
#include "OSDHandler.h"
#include "OsdConfig.h"
#include "DetectResult.hpp"
#include "IVPSGrpStage.h"

#define WEB_OBS "WEB_OBS"

class COsdObserver : public IObserver {
public:
    COsdObserver(vector<CIVPSGrpStage*>* pSink) : m_pSink(pSink){};
    virtual ~COsdObserver(AX_VOID) = default;

public:
    AX_BOOL IsSame(AX_U32 nGrp, AX_U32 nChn) const {
        return (m_nGroup == nGrp && m_nChannel == nChn) ? AX_TRUE : AX_FALSE;
    }

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        if (E_OBS_TARGET_TYPE_DETECT == eTarget) {
            DETECT_RESULT_T* rlt = (DETECT_RESULT_T*)pData;

            AX_U32 nCount[AX_APP_ALGO_HVCFP_TYPE_BUTT]
                = {rlt->nBodyCount, rlt->nVehicleCount, rlt->nCycleCount, rlt->nFaceCount, rlt->nPlateCount};
            AX_APP_ALGO_HVCFP_ITEM_PTR pItems[AX_APP_ALGO_HVCFP_TYPE_BUTT]
                = {rlt->stBodys, rlt->stVehicles, rlt->stCycles, rlt->stFaces, rlt->stPlates};

            std::vector<AX_APP_ALGO_BOX_T> vecBox;
            for (AX_U32 i = 0; i < AX_APP_ALGO_HVCFP_TYPE_BUTT; ++i) {
                for (AX_U32 j = 0; j < nCount[i]; ++j) {
                    if (pItems[i][j].eTrackStatus == AX_APP_ALGO_TRACK_STATUS_NEW
                        || pItems[i][j].eTrackStatus == AX_APP_ALGO_TRACK_STATUS_UPDATE) {
                        vecBox.push_back(pItems[i][j].stBox);
                    }
                }
            }

            vector<CIVPSGrpStage*> vecIvpsInstance = *(vector<CIVPSGrpStage*> *)m_pSink;
            for (size_t i = 0; i < vecIvpsInstance.size(); ++i) {
                if (vecIvpsInstance[i]->GetGrpCfg()->nSnsSrc == nGrp) {
                    if (vecIvpsInstance[i]->GetOsdHelper()) {
                        vecIvpsInstance[i]->GetOsdHelper()->UpdateOSDRect(vecBox);
                    }
                }
            }
        }

        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams) {
            return AX_FALSE;
        }
        m_nGroup = nGrp;
        m_nChannel = nChn;

        if (E_OBS_TARGET_TYPE_DETECT == eTarget) {
            // do nothing
        }

        return AX_TRUE;
    }

private:
    vector<CIVPSGrpStage*>* m_pSink{nullptr};
    AX_U32 m_nGroup{0};
    AX_U32 m_nChannel{0};
};
