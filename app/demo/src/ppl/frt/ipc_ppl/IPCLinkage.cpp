/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "IPCLinkage.h"
#include "CommonUtils.hpp"
#include "PPLOptionHelper.h"
#include "SensorOptionHelper.h"

using namespace AX_IPC;
using namespace std;

#define LINKAGE "LINKAGE"

AX_BOOL CLinkage::Setup() {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        LOG_M_W(LINKAGE, "No linkage configured in ppl.json.");
        return AX_FALSE;
    }

    for (auto relation : vecRelations) {
        if (relation.Valid() && relation.bLink) {
            LINK_MOD_INFO_T tLinkInfo;
            tLinkInfo.tSrcModChn.eModType = relation.tSrcModChn.eModType;
            tLinkInfo.tSrcModChn.nGroup = relation.tSrcModChn.nGroup;
            tLinkInfo.tSrcModChn.nChannel = relation.tSrcModChn.nChannel;
            tLinkInfo.tDstModChn.eModType = relation.tDstModChn.eModType;
            tLinkInfo.tDstModChn.nGroup = relation.tDstModChn.nGroup;
            tLinkInfo.tDstModChn.nChannel = relation.tDstModChn.nChannel;
            Link(tLinkInfo);
        }
    }

    return AX_TRUE;
}

AX_BOOL CLinkage::Release() {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        return AX_FALSE;
    }

    for (auto relation : vecRelations) {
        if (relation.Valid() && relation.bLink) {
            LINK_MOD_INFO_T tLinkInfo;
            tLinkInfo.tSrcModChn.eModType = relation.tSrcModChn.eModType;
            tLinkInfo.tSrcModChn.nGroup = relation.tSrcModChn.nGroup;
            tLinkInfo.tSrcModChn.nChannel = relation.tSrcModChn.nChannel;
            tLinkInfo.tDstModChn.eModType = relation.tDstModChn.eModType;
            tLinkInfo.tDstModChn.nGroup = relation.tDstModChn.nGroup;
            tLinkInfo.tDstModChn.nChannel = relation.tDstModChn.nChannel;
            Unlink(tLinkInfo);
        }
    }

    return AX_TRUE;
}

const AX_BOOL CLinkage::GetCurrRelations(vector<IPC_MOD_RELATIONSHIP_T>& vecRelations) const {
    AX_U8 nScenario = APP_CURR_SCENARIO();
    APP_PPL_MOD_RELATIONS(nScenario, vecRelations);

    return vecRelations.size() > 0 ? AX_TRUE : AX_FALSE;
}

IPC_MOD_RELATIONSHIP_T CLinkage::GetRelation(const LINK_MOD_INFO_T& tModLink) const {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        return {};
    }

    for (auto relation : vecRelations) {
        if (relation.Valid()) {
            if (CCommonUtils::ModuleEqual(relation.tSrcModChn, tModLink.tSrcModChn) &&
                CCommonUtils::ModuleEqual(relation.tDstModChn, tModLink.tDstModChn)) {
                return relation;
            }
        }
    }

    return {};
}

AX_BOOL CLinkage::GetRelationsBySrcMod(const IPC_MOD_INFO_T& tSrcMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                       AX_BOOL bIgnoreChn /*= AX_FALSE*/) const {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        return AX_FALSE;
    }

    for (auto relation : vecRelations) {
        if (relation.Valid()) {
            if (CCommonUtils::ModuleEqual(relation.tSrcModChn, tSrcMod, bIgnoreChn)) {
                vecOutRelations.emplace_back(relation);
            }
        }
    }

    return vecOutRelations.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CLinkage::GetRelationsByDstMod(const IPC_MOD_INFO_T& tDstMod, vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                       AX_BOOL bIgnoreChn /*= AX_FALSE*/) const {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        return AX_FALSE;
    }

    for (auto relation : vecRelations) {
        if (relation.Valid()) {
            if (CCommonUtils::ModuleEqual(relation.tDstModChn, tDstMod, bIgnoreChn)) {
                vecOutRelations.emplace_back(relation);
            }
        }
    }

    return vecOutRelations.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CLinkage::GetPrecedingMod(const IPC_MOD_INFO_T& tDstMod, IPC_MOD_INFO_T& tPrecedingMod) const {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        return AX_FALSE;
    }

    for (auto relation : vecRelations) {
        if (relation.Valid()) {
            if (CCommonUtils::ModuleEqual(relation.tDstModChn, tDstMod, AX_FALSE)) {
                if (relation.tSrcModChn.eModType == tPrecedingMod.eModType) {
                    tPrecedingMod = relation.tSrcModChn;
                    return AX_TRUE;
                } else {
                    return GetPrecedingMod(relation.tSrcModChn, tPrecedingMod);
                }
            }
        }
    }

    return AX_FALSE;
}
