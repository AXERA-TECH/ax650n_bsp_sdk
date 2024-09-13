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

#include <vector>
#include "BaseLinkage.h"
#include "IPPLBuilder.h"
#include "ax_base_type.h"

namespace AX_ITS {

class CLinkage : public CBaseLinkage {
public:
    CLinkage(AX_VOID) = default;
    virtual ~CLinkage(AX_VOID) = default;

public:
    virtual AX_BOOL Setup() override;
    virtual AX_BOOL Release() override;

    AX_BOOL SetLinkMode(PPL_MODULE_TYPE_E eModule, AX_S32 nChn, AX_BOOL bLink);
    IPC_MOD_RELATIONSHIP_T GetRelation(const LINK_MOD_INFO_T& tModLink) const;
    AX_BOOL GetRelationsBySrcMod(const IPC_MOD_INFO_T& tSrcMod, std::vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                 AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetRelationsByDstMod(const IPC_MOD_INFO_T& tDstMod, std::vector<IPC_MOD_RELATIONSHIP_T>& vecOutRelations,
                                 AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetPrecedingMod(const IPC_MOD_INFO_T& tDstMod, IPC_MOD_INFO_T& tPrecedingMod) const;
    AX_BOOL UpdateRelation(PPL_MODULE_TYPE_E eOldModule, AX_S32 nOldChn, PPL_MODULE_TYPE_E eDstModule, AX_S32 nDstGrp, AX_S32 nDstChn, AX_BOOL bLink);

private:
    const AX_BOOL GetCurrRelations(std::vector<IPC_MOD_RELATIONSHIP_T>& vecRelations) const;

private:
    std::vector<IPC_MOD_RELATIONSHIP_T> m_vecRelations;
};

}  // namespace AX_ITS