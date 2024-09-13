/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "attrParser.hpp"
#include "picojson.h"

#define PICO_OBJECT get<picojson::object>()
#define PICO_OBJECT_SIZE PICO_OBJECT.size()
#define PICO_ARRAY get<picojson::array>()
#define PICO_ARRAY_SIZE PICO_ARRAY.size()
#define PICO_VALUE get<double>()
#define PICO_BOOL get<bool>()
#define PICO_STRING get<std::string>()
#define PICO_ROOT obj.PICO_OBJECT

typedef struct _Face_Attr_t {
    AX_BOOL bExist;
    AX_U8 nAge;
    /* 0: female 1: male */
    AX_U8 nGender;
    /*
    string:
        no_respirator
        surgical
        anti-pollution
        common
        kitchen_transparent
        unknown
    */
    std::string strRespirator;

    _Face_Attr_t() {
        bExist = AX_FALSE;
        nAge = 0;
        nGender = 0;
        strRespirator = "";
    }
} Face_Attr_t;

typedef struct _Plate_Attr_t {
    AX_BOOL bExist;
    AX_BOOL bValid;
    /*
    string:
        blue
        yellow
        black
        white
        green
        small_new_energy
        large_new_energy
        absence
        unknown
    */
    std::string strPlateColor;
    /*
    string:
        one_row
        two_rows
        unknown
    */
    std::string strPlateType;
    /* string: UTF8*/
    std::string strPlateCode;

    _Plate_Attr_t() {
        bExist = AX_FALSE;
        bValid = AX_FALSE;
        strPlateColor = "";
        strPlateType = "";
        strPlateCode = "";
    }
} Plate_Attr_t;

AX_VOID CAttrParser::FaceAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo) {
    picojson::value obj;
    Face_Attr_t face_attr;

    pAttrInfo->eType = AX_APP_ALGO_HVCFP_FACE;

    face_attr.bExist = AX_FALSE;
    face_attr.nAge = 0;
    face_attr.nGender = 0;
    face_attr.strRespirator = "";

    for (size_t i = 0; i < pstObjectItems->nMetaInfoSize; i++) {
        if (!strcmp(pstObjectItems->pstMetaInfo[i].pstrType, "face_attr")) {
            std::string value = pstObjectItems->pstMetaInfo[i].pstrValue;
            std::string strParseRet = picojson::parse(obj, value);
            if (!strParseRet.empty() || !obj.is<picojson::object>()) {
                break;
            }

            face_attr.bExist = AX_TRUE;
            // age
            face_attr.nAge = PICO_ROOT["age"].PICO_VALUE;

            // gender
            std::string strGender = PICO_ROOT["gender"].PICO_OBJECT["name"].PICO_STRING;
            if (strGender == "male") {
                face_attr.nGender = 1;
            }
            else {
                face_attr.nGender = 0;
            }

            // respirator
            face_attr.strRespirator = PICO_ROOT["respirator"].PICO_OBJECT["name"].PICO_STRING;

            pAttrInfo->bExist = AX_TRUE;

            pAttrInfo->stFaceInfo.nAge = face_attr.nAge;
            pAttrInfo->stFaceInfo.nGender = face_attr.nGender;
            pAttrInfo->stFaceInfo.eRespirator = AX_APP_ALGO_GET_RESPIRATOR_TYPE(face_attr.strRespirator);
        }
    }
}

AX_VOID CAttrParser::PlateAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo) {
    picojson::value obj;
    Plate_Attr_t plat_attr;

    pAttrInfo->eType = AX_APP_ALGO_HVCFP_PLATE;

    plat_attr.bExist = AX_FALSE;
    plat_attr.bValid = AX_FALSE;
    plat_attr.strPlateColor = "";
    plat_attr.strPlateType = "";
    plat_attr.strPlateCode = "";

    for (size_t i = 0; i < pstObjectItems->nMetaInfoSize; i++) {
        if (!strcmp(pstObjectItems->pstMetaInfo[i].pstrType, "plate_attr")) {
            std::string value = pstObjectItems->pstMetaInfo[i].pstrValue;
            std::string strParseRet = picojson::parse(obj, value);
            if (!strParseRet.empty() || !obj.is<picojson::object>()) {
                break;
            }

            plat_attr.bExist = AX_TRUE;
            // color
            plat_attr.strPlateColor = "unknown";
            if (PICO_ROOT.end() != PICO_ROOT.find("color")) {
                plat_attr.strPlateColor = PICO_ROOT["color"].PICO_OBJECT["name"].PICO_STRING;
            }

            // style
            plat_attr.strPlateType = "unknown";
            if (PICO_ROOT.end() != PICO_ROOT.find("style")) {
                plat_attr.strPlateType = PICO_ROOT["style"].PICO_OBJECT["name"].PICO_STRING;
            }

            // code
            plat_attr.strPlateCode = PICO_ROOT["code_result"].PICO_STRING;

            if (PICO_ROOT["code_killed"].PICO_BOOL) {
                plat_attr.bValid = AX_FALSE;
            } else {
                plat_attr.bValid = AX_TRUE;
            }

            pAttrInfo->bExist = AX_TRUE;
            pAttrInfo->stPlateInfo.ePlateColor = AX_APP_ALGO_GET_PLATE_COLOR_TYPE(plat_attr.strPlateColor);
            pAttrInfo->stPlateInfo.bValid = plat_attr.bValid;
            strncpy(pAttrInfo->stPlateInfo.szNum, (const AX_CHAR *)plat_attr.strPlateCode.c_str(), sizeof(pAttrInfo->stPlateInfo.szNum) - 1);
        }
    }
}

AX_VOID CAttrParser::VehicleAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo) {
    picojson::value obj;
    Plate_Attr_t plat_attr;

    pAttrInfo->eType = AX_APP_ALGO_HVCFP_VEHICLE;

    plat_attr.bExist = AX_FALSE;
    plat_attr.bValid = AX_FALSE;
    plat_attr.strPlateColor = "";
    plat_attr.strPlateType = "";
    plat_attr.strPlateCode = "";

    for (size_t i = 0; i < pstObjectItems->nMetaInfoSize; i++) {
        if (!strcmp(pstObjectItems->pstMetaInfo[i].pstrType, "plate_attr")) {
            std::string value = pstObjectItems->pstMetaInfo[i].pstrValue;
            std::string strParseRet = picojson::parse(obj, value);
            if (!strParseRet.empty() || !obj.is<picojson::object>()) {
                break;
            }

            plat_attr.bExist = AX_TRUE;
            // color
            plat_attr.strPlateColor = "unknown";
            if (PICO_ROOT.end() != PICO_ROOT.find("color")) {
                plat_attr.strPlateColor = PICO_ROOT["color"].PICO_OBJECT["name"].PICO_STRING;
            }

            // style
            plat_attr.strPlateType = "unknown";
            if (PICO_ROOT.end() != PICO_ROOT.find("style")) {
                plat_attr.strPlateType = PICO_ROOT["style"].PICO_OBJECT["name"].PICO_STRING;
            }

            // code
            plat_attr.strPlateCode = PICO_ROOT["code_result"].PICO_STRING;

            if (PICO_ROOT["code_killed"].PICO_BOOL) {
                plat_attr.bValid = AX_FALSE;
            } else {
                plat_attr.bValid = AX_TRUE;
            }

            pAttrInfo->bExist = AX_TRUE;
            pAttrInfo->stVehicleInfo.stPlateInfo.ePlateColor = AX_APP_ALGO_GET_PLATE_COLOR_TYPE(plat_attr.strPlateColor);
            pAttrInfo->stVehicleInfo.stPlateInfo.bValid = plat_attr.bValid;
            strncpy(pAttrInfo->stVehicleInfo.stPlateInfo.szNum, (const AX_CHAR *)plat_attr.strPlateCode.c_str(), sizeof(pAttrInfo->stVehicleInfo.stPlateInfo.szNum) - 1);
        }
    }
}

AX_VOID CAttrParser::AttrParser(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, ATTR_INFO_T *pAttrInfo) {
    if (!pstObjectItems || !pAttrInfo) {
        return;
    }

    pAttrInfo->bExist = AX_FALSE;
    pAttrInfo->eType = AX_APP_ALGO_HVCFP_TYPE_BUTT;

    std::string strObjectCategory = pstObjectItems->pstrObjectCategory;

    if (strObjectCategory == "face") {
        FaceAttrResult(pstObjectItems, pAttrInfo);
    }
    else if (strObjectCategory == "plate") {
        PlateAttrResult(pstObjectItems, pAttrInfo);
    }
    else if (strObjectCategory == "vehicle") {
        VehicleAttrResult(pstObjectItems, pAttrInfo);
    }
}
