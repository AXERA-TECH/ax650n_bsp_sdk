/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <vector>
#include "BaseLinkage.h"
#include "IPPLBuilder.h"
#include "ax_base_type.h"

namespace AX_PANO {

class CLinkage : public CBaseLinkage {
public:
    CLinkage(AX_VOID) = default;
    virtual ~CLinkage(AX_VOID) = default;

public:
    virtual AX_BOOL Setup() override;
    virtual AX_BOOL Release() override;

    PPL_MOD_RELATIONSHIP_T GetRelation(const LINK_MOD_INFO_T& tModLink) const;
    AX_BOOL GetRelationsBySrcMod(const PPL_MOD_INFO_T& tSrcMod, std::vector<PPL_MOD_RELATIONSHIP_T>& vecOutRelations, AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetRelationsByDstMod(const PPL_MOD_INFO_T& tDstMod, std::vector<PPL_MOD_RELATIONSHIP_T>& vecOutRelations, AX_BOOL bIgnoreChn = AX_FALSE) const;
    AX_BOOL GetPrecedingMod(const PPL_MOD_INFO_T& tDstMod, PPL_MOD_INFO_T& tPrecedingMod) const;

private:
    const AX_BOOL GetCurrRelations(std::vector<PPL_MOD_RELATIONSHIP_T>& vecRelations) const;
};

}  // namespace AX_PANO