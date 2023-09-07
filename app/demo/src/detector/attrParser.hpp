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

#include "ax_global_type.h"
#include "ax_sys_api.h"
#include "ax_skel_api.h"
#include "AXSingleton.h"
#include "AXAlgo.hpp"
#include "DetectResult.hpp"

typedef struct _ATTR_FACEINFO_T {
    AX_U8   nGender; /* 0-female, 1-male */
    AX_U8   nAge;
    AX_APP_ALGO_FACE_RESPIRATOR_TYPE_E eRespirator;
} ATTR_FACEINFO_T;

typedef struct _ATTR_PLATEINFO_T {
    AX_BOOL bValid;
    AX_CHAR szNum[AX_APP_ALGO_LPR_LEN];
    AX_APP_ALGO_PLATE_COLOR_TYPE_E ePlateColor;
} ATTR_PLATEINFO_T;

typedef struct _ATTR_VEHICLEINFO_T {
    ATTR_PLATEINFO_T stPlateInfo;
} ATTR_VEHICLEINFO_T;

typedef struct _ATTR_INFO_T {
    AX_APP_ALGO_HVCFP_TYPE_E eType;
    AX_BOOL bExist;

    union {
        ATTR_FACEINFO_T stFaceInfo;
        ATTR_PLATEINFO_T stPlateInfo;
        ATTR_VEHICLEINFO_T stVehicleInfo;
    };
} ATTR_INFO_T;

class CAttrParser : public CAXSingleton<CAttrParser> {
    friend class CAXSingleton<CAttrParser>;

public:
    AX_VOID AttrParser(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo);

private:
    AX_APP_ALGO_PLATE_COLOR_TYPE_E PlateAttrColor(const std::string &strPlateColor);
    AX_APP_ALGO_FACE_RESPIRATOR_TYPE_E FaceAttrRespirator(const std::string &strRespirator);
    AX_VOID FaceAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo);
    AX_VOID PlateAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo);
    AX_VOID VehicleAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo);
};
