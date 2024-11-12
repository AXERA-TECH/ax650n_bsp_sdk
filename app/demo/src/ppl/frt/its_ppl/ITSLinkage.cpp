/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ITSLinkage.h"
#include "CommonUtils.hpp"
#include "PPLOptionHelper.h"
#include "SensorOptionHelper.h"

using namespace AX_ITS;
using namespace std;

#define LINKAGE "LINKAGE"

AX_BOOL CLinkage::Setup() {
    vector<IPC_MOD_RELATIONSHIP_T> vecRelations;
    if (!GetCurrRelations(vecRelations)) {
        LOG_M_W(LINKAGE, "No linkage configured in ppl.json.");
        return AX_FALSE;
    }

    for (const auto& relation : vecRelations) {
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
    m_vecRelations.swap(vecRelations);
    return AX_TRUE;
}

AX_BOOL CLinkage::Release() {
    for (const auto& relation : m_vecRelations) {
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

AX_BOOL CLinkage::SetLinkMode(PPL_MODULE_TYPE_E eModule, AX_S32 nChn, AX_BOOL bLink) {
    for (auto& relation : m_vecRelations) {
        if (eModule == relation.tDstModChn.eModType && relation.tDstModChn.nChannel == nChn) {
            LINK_MOD_INFO_T tLinkInfo;
            tLinkInfo.tSrcModChn.eModType = relation.tSrcModChn.eModType;
            tLinkInfo.tSrcModChn.nGroup = relation.tSrcModChn.nGroup;
            tLinkInfo.tSrcModChn.nChannel = relation.tSrcModChn.nChannel;
            tLinkInfo.tDstModChn.eModType = relation.tDstModChn.eModType;
            tLinkInfo.tDstModChn.nGroup = relation.tDstModChn.nGroup;
            tLinkInfo.tDstModChn.nChannel = relation.tDstModChn.nChannel;
            LOG_MM_D(LINKAGE, "bLink:%d Src[module:%d %d,%d], Dst[module:%d %d,%d]", bLink, relation.tSrcModChn.eModType,
                     relation.tSrcModChn.nGroup, relation.tSrcModChn.nChannel, relation.tDstModChn.eModType, relation.tDstModChn.nGroup,
                     relation.tDstModChn.nChannel);
            if (AX_FALSE == bLink) {
                if (AX_SUCCESS != Unlink(tLinkInfo)) {
                    LOG_MM_E(LINKAGE, "Unlink(Src[module:%d %d,%d], Dst[module:%d %d,%d]) Failed.", relation.tSrcModChn.eModType,
                             relation.tSrcModChn.nGroup, relation.tSrcModChn.nChannel, relation.tDstModChn.eModType,
                             relation.tDstModChn.nGroup, relation.tDstModChn.nChannel);
                } else {
                    relation.bLink = AX_FALSE;
                }
            } else {
                if (AX_SUCCESS != Link(tLinkInfo)) {
                    LOG_MM_E(LINKAGE, "link(Src[%d,%d], Dst[%d,%d]) Failed.", relation.tSrcModChn.nGroup, relation.tSrcModChn.nChannel,
                             relation.tDstModChn.nGroup, relation.tDstModChn.nChannel);
                } else {
                    relation.bLink = AX_TRUE;
                }
            }
        }
    }
    return AX_TRUE;
}

AX_BOOL CLinkage::UpdateRelation(PPL_MODULE_TYPE_E eOldModule, AX_S32 nOldChn, PPL_MODULE_TYPE_E eDstModule, AX_S32 nDstGrp, AX_S32 nDstChn,
                                 AX_BOOL bLink) {
    for (auto& relation : m_vecRelations) {
        if (eOldModule == relation.tDstModChn.eModType && relation.tDstModChn.nChannel == nOldChn) {
            if (bLink) {
                LINK_MOD_INFO_T tLinkInfo;
                tLinkInfo.tSrcModChn = relation.tSrcModChn;
                tLinkInfo.tDstModChn = relation.tDstModChn;
                if (AX_SUCCESS != Unlink(tLinkInfo)) {
                    return AX_FALSE;
                }
                relation.bLink = AX_FALSE;
                sleep(1);
            }

            {
                LINK_MOD_INFO_T tLinkInfo;
                tLinkInfo.tSrcModChn = relation.tSrcModChn;
                tLinkInfo.tDstModChn.eModType = eDstModule;
                tLinkInfo.tDstModChn.nGroup = nDstGrp;
                tLinkInfo.tDstModChn.nChannel = nDstChn;
                if (bLink && (AX_SUCCESS != Link(tLinkInfo))) {
                    return AX_FALSE;
                }

                relation.tDstModChn.eModType = tLinkInfo.tDstModChn.eModType;
                relation.tDstModChn.nGroup = tLinkInfo.tDstModChn.nGroup;
                relation.tDstModChn.nChannel = tLinkInfo.tDstModChn.nChannel;
            }
            break;
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
    for (const auto relation : m_vecRelations) {
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
    for (const auto& relation : m_vecRelations) {
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
    for (const auto& relation : m_vecRelations) {
        if (relation.Valid()) {
            if (CCommonUtils::ModuleEqual(relation.tDstModChn, tDstMod, bIgnoreChn)) {
                vecOutRelations.emplace_back(relation);
            }
        }
    }

    return vecOutRelations.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CLinkage::GetPrecedingMod(const IPC_MOD_INFO_T& tDstMod, IPC_MOD_INFO_T& tPrecedingMod) const {
    for (const auto& relation : m_vecRelations) {
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
