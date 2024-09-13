/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <map>
#include "statMgr.hpp"

AX_VOID CStatMgr::StatTrackMgr(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, STAT_OBJECT_NUM_T &stObjectNum) {
    if (!pstObjectItems) {
        return;
    }

    std::string strObjectCategory = pstObjectItems->pstrObjectCategory;

    if (pstObjectItems->eTrackState == AX_SKEL_TRACK_STATUS_NEW) {
        if (strObjectCategory == "body") {
            stObjectNum.nBodyNum ++;
        }
        else if (strObjectCategory == "vehicle") {
            stObjectNum.nVehicleNum ++;
        }
        else if (strObjectCategory == "cycle") {
            stObjectNum.nCycleNum ++;
        }
        else if (strObjectCategory == "face") {
            stObjectNum.nFaceNum ++;
        }
        else if (strObjectCategory == "plate") {
            stObjectNum.nPlateNum ++;
        }
    }
}

AX_VOID CStatMgr::StatPushMgr(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, STAT_OBJECT_NUM_T &stObjectNum) {
    if (!pstObjectItems) {
        return;
    }

    std::string strObjectCategory = pstObjectItems->pstrObjectCategory;

    if (pstObjectItems->eTrackState == AX_SKEL_TRACK_STATUS_SELECT) {
        if (strObjectCategory == "body") {
            stObjectNum.nBodyNum ++;
        }
        else if (strObjectCategory == "vehicle") {
            stObjectNum.nVehicleNum ++;
        }
        else if (strObjectCategory == "cycle") {
            stObjectNum.nCycleNum ++;
        }
        else if (strObjectCategory == "face") {
            stObjectNum.nFaceNum ++;
        }
        else if (strObjectCategory == "plate") {
            stObjectNum.nPlateNum ++;
        }
    }
}
