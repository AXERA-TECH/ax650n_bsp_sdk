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

#include <string>
#include "ax_global_type.h"
#include "ax_skel_type.h"

#define AX_APP_ALGO_SNS_MAX                           (2)
#define AX_APP_ALGO_PATH_LEN                          (128)
#define AX_APP_ALGO_LPR_LEN                           (16)
#define AX_APP_ALGO_DETECT_ROI_POINT_MAX              (10)
#define AX_APP_ALGO_DETECT_DEFAULT_QP_LEVEL           (75)
#define AX_APP_ALGO_DETECT_DEFAULT_FRAMERATE          (-1)
#define AX_APP_ALGO_IVES_MD_REGION_MAX                (16)
#define AX_APP_ALGO_IVES_MD_DEFAULT_BLOCK_WIDTH       (32)
#define AX_APP_ALGO_IVES_MD_DEFAULT_BLOCK_HEIGHT      (32)
#define AX_APP_ALGO_IVES_MD_DEFAULT_THRESHOLD_Y       (20)
#define AX_APP_ALGO_IVES_MD_DEFAULT_CONFIDENCE        (50)

#define AX_APP_ALGO_IVES_OD_DEFAULT_THRESHOLD_Y       (100)
#define AX_APP_ALGO_IVES_OD_DEFAULT_CONFIDENCE_Y      (80)
#define AX_APP_ALGO_IVES_OD_DEFAULT_LUX_THRESHOLD     (0)
#define AX_APP_ALGO_IVES_OD_DEFAULT_LUX_DIFF          (60)

#define AX_APP_ALGO_IVES_SCD_DEFAULT_THRESHOLD        (60)
#define AX_APP_ALGO_IVES_SCD_DEFAULT_CONFIDENCE       (60)

// algorithm type
typedef enum axAPP_ALGO_TYPE_E {
    AX_APP_ALGO_TYPE_NONE               = (0 << 0),
    AX_APP_ALGO_PERSON_DETECT           = (1 << 0),
    AX_APP_ALGO_VEHICLE_DETECT          = (1 << 1),
    AX_APP_ALGO_CYCLE_DETECT            = (1 << 2),
    AX_APP_ALGO_PLATE_DETECT            = (1 << 3),
    AX_APP_ALGO_LICENSE_PLATE_RECOGNIZE = (1 << 4),
    AX_APP_ALGO_FACE_DETECT             = (1 << 5),
    AX_APP_ALGO_FACE_RECOGNIZE          = (1 << 6),
    AX_APP_ALGO_MOTION_DETECT           = (1 << 7),
    AX_APP_ALGO_OCCLUSION_DETECT        = (1 << 8),
    AX_APP_ALGO_SCENE_CHANGE_DETECT     = (1 << 9),
    AX_APP_ALGO_SOUND_DETECT            = (1 << 10)
} AX_APP_ALGO_TYPE_E;

#define AX_APP_ALGO_TYPE_HVCFP (AX_APP_ALGO_PERSON_DETECT \
                                | AX_APP_ALGO_VEHICLE_DETECT \
                                | AX_APP_ALGO_CYCLE_DETECT \
                                | AX_APP_ALGO_PLATE_DETECT)

#define AX_APP_ALGO_TYPE_HVCP (AX_APP_ALGO_PERSON_DETECT \
                                | AX_APP_ALGO_VEHICLE_DETECT \
                                | AX_APP_ALGO_CYCLE_DETECT \
                                | AX_APP_ALGO_PLATE_DETECT \
                                | AX_APP_ALGO_LICENSE_PLATE_RECOGNIZE)

#define AX_APP_ALGO_TYPE_FH (AX_APP_ALGO_PERSON_DETECT \
                                | AX_APP_ALGO_FACE_DETECT \
                                | AX_APP_ALGO_FACE_RECOGNIZE)

// algorithm hvcfp object type
typedef enum axAPP_ALGO_HVCFP_TYPE_E {
    AX_APP_ALGO_HVCFP_BODY,
    AX_APP_ALGO_HVCFP_VEHICLE,
    AX_APP_ALGO_HVCFP_CYCLE,
    AX_APP_ALGO_HVCFP_FACE,
    AX_APP_ALGO_HVCFP_PLATE,
    AX_APP_ALGO_HVCFP_TYPE_BUTT
} AX_APP_ALGO_HVCFP_TYPE_E;

// algorithm ives type
typedef enum axAPP_ALGO_IVES_TYPE_E {
    AX_APP_ALGO_IVES_MOTION,
    AX_APP_ALGO_IVES_OCCLUSION,
    AX_APP_ALGO_IVES_SCENE_CHANGE,
    AX_APP_ALGO_IVES_TYPE_BUTT
} AX_APP_ALGO_IVES_TYPE_E;

// algorithm face respirator type
typedef enum axAPP_ALGO_FACE_RESPIRATOR_TYPE_E {
    AX_APP_ALGO_FACE_RESPIRATOR_NONE,
    AX_APP_ALGO_FACE_RESPIRATOR_SURGICAL,
    AX_APP_ALGO_FACE_RESPIRATOR_ANTI_PULLTION,
    AX_APP_ALGO_FACE_RESPIRATOR_COMMON,
    AX_APP_ALGO_FACE_RESPIRATOR_KITCHEN_TRANSPARENT,
    AX_APP_ALGO_FACE_RESPIRATOR_UNKOWN,
    AX_APP_ALGO_FACE_RESPIRATOR_TYPE_BUTT
} AX_APP_ALGO_FACE_RESPIRATOR_TYPE_E;

// algorithm plate color type
typedef enum axAPP_ALGO_PLATE_COLOR_TYPE_E {
    AX_APP_ALGO_PLATE_COLOR_BLUE,
    AX_APP_ALGO_PLATE_COLOR_YELLOW,
    AX_APP_ALGO_PLATE_COLOR_BLACK,
    AX_APP_ALGO_PLATE_COLOR_WHITE,
    AX_APP_ALGO_PLATE_COLOR_GREEN,
    AX_APP_ALGO_PLATE_COLOR_NEW_ENERGY,
    AX_APP_ALGO_PLATE_COLOR_UNKOWN,
    AX_APP_ALGO_PLATE_COLOR_TYPE_BUTT
} AX_APP_ALGO_PLATE_COLOR_TYPE_E;

// algorithm track status type
typedef enum axAPP_ALGO_TRACK_STATUS_E {
    AX_APP_ALGO_TRACK_STATUS_NEW,
    AX_APP_ALGO_TRACK_STATUS_UPDATE,
    AX_APP_ALGO_TRACK_STATUS_LOST,
    AX_APP_ALGO_TRACK_STATUS_SELECT,
    AX_APP_ALGO_TRACK_STATUS_BUTT
} AX_APP_ALGO_TRACK_STATUS_E;

// algorithm push mode
typedef enum axAPP_ALGO_PUSH_MODE_E {
    AX_APP_ALGO_PUSH_MODE_FAST     = 1,
    AX_APP_ALGO_PUSH_MODE_INTERVAL = 2,
    AX_APP_ALGO_PUSH_MODE_BEST     = 3,
    AX_APP_ALGO_PUSH_MODE_BUTT
} AX_APP_ALGO_PUSH_MODE_E;

// algorithm face recognize operation type
typedef enum axAPP_ALGO_FACE_RECOGNIZE_OP_E {
    AX_APP_ALGO_FACE_RECOGNIZE_OP_NEW,
    AX_APP_ALGO_FACE_RECOGNIZE_OP_CANCEL,
    AX_APP_ALGO_FACE_RECOGNIZE_OP_UPDATE,
    AX_APP_ALGO_FACE_RECOGNIZE_OP_BUTT
} AX_APP_ALGO_FACE_RECOGNIZE_OP_E;

// algorithm face recognize error code
typedef enum axAPP_ALGO_FACE_RECOGNIZE_ERR_CODE_E {
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_NONE = 0,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_DB_LOADING = 1,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_EXCEED_1_FACE = 2,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_NO_FACE = 3,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_NAME_DUPLICATE = 4,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_WAITING = 5,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_TIMEOUT = 6,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_PARAM = 7,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_NOT_SUPPORT = 8,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_EXCEED_CAPABILITY = 9,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_CANCEL = 10,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_OTHERS = 11,
    AX_APP_ALGO_CATPURE_FACE_RECOGNIZE_ERR_BUTT
} AX_APP_ALGO_FACE_RECOGNIZE_ERR_CODE_E;

// face recognize attribute
typedef struct axAPP_ALGO_FACE_RECOGNIZE_ATTR_T {
    AX_U32 nCapability;
    AX_F32 fCompareScoreThreshold;
    AX_CHAR *strDataBasePath;
    AX_CHAR *strDataBaseName;
} AX_APP_ALGO_FACE_RECOGNIZE_ATTR_T, *AX_APP_ALGO_FACE_RECOGNIZE_ATTR_PTR;

// algorithm attribute
typedef struct axAPP_ALGO_ATTR_T {
    AX_U32 nAlgoType;
    AX_APP_ALGO_FACE_RECOGNIZE_ATTR_T stFaceRecognizeAttr;
} AX_APP_ALGO_ATTR_T, *AX_APP_ALGO_ATTR_PTR;

// point coordinates
typedef struct axAPP_POINT_T {
    AX_F32 fX;
    AX_F32 fY;
} AX_APP_POINT_T, *AX_APP_POINT_PTR;

// polygon coordinates
typedef struct axAPP_POLYGON_T {
    AX_U32 nPointNum;
    AX_APP_POINT_T stPoints[AX_APP_ALGO_DETECT_ROI_POINT_MAX];
} AX_APP_POLYGON_T, *AX_APP_POLYGON_PTR;

// rect coordinates
typedef struct axAPP_RECT_T {
    AX_F32 fX;
    AX_F32 fY;
    AX_F32 fW;
    AX_F32 fH;
} AX_APP_RECT_T, *AX_APP_RECT_PTR;

// algorithm box
typedef struct axAPP_ALGO_BOX_T {
    // normalized coordinates
    AX_F32 fX;
    AX_F32 fY;
    AX_F32 fW;
    AX_F32 fH;

    // image resolution
    AX_U32 nImgWidth;
    AX_U32 nImgHeight;
} AX_APP_ALGO_BOX_T, *AX_APP_ALGO_BOX_PTR;

// algorithm face attribute
typedef struct axAPP_ALGO_FACE_ATTR_T {
    AX_BOOL bExist;
    AX_BOOL bIdentified;
    AX_U8 nAge;
    /* 0: female 1: male */
    AX_U8 nGender;
    AX_APP_ALGO_FACE_RESPIRATOR_TYPE_E eRespirator;
    AX_CHAR *pstrRecognizeName;
} AX_APP_ALGO_FACE_ATTR_T, *AX_APP_ALGO_FACE_ATTR_PTR;

// algorithm body attribute
typedef struct axAPP_ALGO_BODY_ATTR_T {
    AX_BOOL bExist;
} AX_APP_ALGO_BODY_ATTR_T, *AX_APP_ALGO_BODY_ATTR_PTR;

// algorithm plate attribute
typedef struct axAPP_ALGO_PLATE_ATTR_T {
    AX_BOOL bExist;
    AX_BOOL bValid;
    /* AX_CHAR: UTF8*/
    AX_CHAR strPlateCode[AX_APP_ALGO_LPR_LEN];
    AX_APP_ALGO_PLATE_COLOR_TYPE_E ePlateColor;
} AX_APP_ALGO_PLATE_ATTR_T, *AX_APP_ALGO_PLATE_ATTR_PTR;

// algorithm vehicle attribute
typedef struct axAPP_ALGO_VEHICLE_ATTR_T {
    AX_BOOL bExist;
    AX_APP_ALGO_PLATE_ATTR_T stPlateAttr;
} AX_APP_ALGO_VEHICLE_ATTR_T, *AX_APP_ALGO_VEHICLE_ATTR_PTR;

// algorithm cycle attribute
typedef struct axAPP_ALGO_CYCLE_ATTR_T {
    AX_BOOL bExist;
} AX_APP_ALGO_CYCLE_ATTR_T;

// algorithm face pose blur
typedef struct axAPP_ALGO_FACE_POSE_BLUR_T {
    AX_F32 fPitch;
    AX_F32 fYaw;
    AX_F32 fRoll;
    AX_F32 fBlur;
} AX_APP_ALGO_FACE_POSE_BLUR_T, *AX_APP_ALGO_FACE_POSE_BLUR_PTR;

// algorithm hvcfp attribute
typedef struct axAPP_ALGO_HVCFP_ATTR_T {
    union {
        AX_APP_ALGO_BODY_ATTR_T stBodyAttr;
        AX_APP_ALGO_VEHICLE_ATTR_T stVehicleAttr;
        AX_APP_ALGO_CYCLE_ATTR_T stCycleAttr;
        AX_APP_ALGO_FACE_ATTR_T stFaceAttr;
        AX_APP_ALGO_PLATE_ATTR_T stPlateAttr;
    };

    AX_APP_ALGO_FACE_POSE_BLUR_T stFacePoseBlur; // only for face
} AX_APP_ALGO_HVCFP_ATTR_T, *AX_APP_ALGO_HVCFP_ATTR_PTR;

// algorithm image
typedef struct axAPP_ALGO_IMG_T {
    AX_BOOL bExist;
    AX_U8 *pData;
    AX_U32 nDataSize;
    AX_U32 nWidth;
    AX_U32 nHeight;
} AX_APP_ALGO_IMG_T, *AX_APP_ALGO_IMG_PTR;

// algorithm hvcfp item
typedef struct axAPP_ALGO_HVCFP_ITEM_T {
    AX_APP_ALGO_HVCFP_TYPE_E eType;
    AX_U64 u64FrameId;
    AX_U64 u64TrackId;
    AX_F32 fConfidence;
    AX_APP_ALGO_TRACK_STATUS_E eTrackStatus;
    AX_APP_ALGO_BOX_T stBox;
    AX_APP_ALGO_IMG_T stImg;
    AX_APP_ALGO_IMG_T stPanoramaImg;
    AX_APP_ALGO_HVCFP_ATTR_T stAttr;
} AX_APP_ALGO_HVCFP_ITEM_T, *AX_APP_ALGO_HVCFP_ITEM_PTR;

// algorithm hvcfp result
typedef struct axAPP_ALGO_HVCFP_RESULT_T {
    AX_S32 nSnsId;
    AX_U32 u64FrameId;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nBodySize;
    AX_APP_ALGO_HVCFP_ITEM_PTR pstBodys;
    AX_U32 nVehicleSize;
    AX_APP_ALGO_HVCFP_ITEM_PTR pstVehicles;
    AX_U32 nCycleSize;
    AX_APP_ALGO_HVCFP_ITEM_PTR pstCycles;
    AX_U32 nFaceSize;
    AX_APP_ALGO_HVCFP_ITEM_PTR pstFaces;
    AX_U32 nPlateSize;
    AX_APP_ALGO_HVCFP_ITEM_PTR pstPlates;
} AX_APP_ALGO_HVCFP_RESULT_T, *AX_APP_ALGO_HVCFP_RESULT_PTR;

// algorithm ives item
typedef struct axAPP_ALGO_IVES_ITEM_T {
    AX_APP_ALGO_IVES_TYPE_E eType;
    AX_U64 u64FrameId;
    AX_F32 fConfidence;
    AX_APP_ALGO_BOX_T stBox;
    AX_APP_ALGO_IMG_T stPanoramaImg;
} AX_APP_ALGO_IVES_ITEM_T, *AX_APP_ALGO_IVES_ITEM_PTR;

// algorithm ives result
typedef struct axAPP_ALGO_IVES_RESULT_T {
    AX_S32 nSnsId;
    AX_U64 u64FrameId;
    AX_U32 nMdSize;
    AX_APP_ALGO_IVES_ITEM_PTR pstMds;
    AX_U32 nOdSize;
    AX_APP_ALGO_IVES_ITEM_PTR pstOds;
    AX_U32 nScdSize;
    AX_APP_ALGO_IVES_ITEM_PTR pstScds;
} AX_APP_ALGO_IVES_RESULT_T, *AX_APP_ALGO_IVES_RESULT_PTR;

// algorithm result
typedef struct axAPP_ALGO_RESULT_T {
    AX_APP_ALGO_HVCFP_RESULT_T stHvcfpResult;
    AX_APP_ALGO_IVES_RESULT_T stIvesResult;
    AX_VOID *pUserData;
    AX_VOID *pPrivateData;
} AX_APP_ALGO_RESULT_T, *AX_APP_ALGO_RESULT_PTR;

// algorithm roi config
typedef struct axAPP_ALGO_ROI_CONFIG_T {
    AX_BOOL bEnable;
    AX_APP_POLYGON_T stPolygon;
} AX_APP_ALGO_ROI_CONFIG_T, *AX_APP_ALGO_ROI_CONFIG_PTR;

// algorithm object fliter config
typedef struct axAPP_ALGO_HVCFP_FILTER_CONFIG_T {
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_F32 fConfidence;
} AX_APP_ALGO_HVCFP_FILTER_CONFIG_T, *AX_APP_ALGO_HVCFP_FILTER_CONFIG_PTR;

// algorithm track size
typedef struct axAPP_ALGO_TRACK_SIZE_T {
    AX_U8 nTrackHumanSize;
    AX_U8 nTrackVehicleSize;
    AX_U8 nTrackCycleSize;
} AX_APP_ALGO_TRACK_SIZE_T, *AX_APP_ALGO_TRACK_SIZE_PTR;

// algorithm push strategy
typedef struct axAPP_ALGO_PUSH_STRATEGY_T {
    AX_APP_ALGO_PUSH_MODE_E ePushMode;
    AX_U32 nInterval;
    AX_U32 nPushCount;
} AX_APP_ALGO_PUSH_STRATEGY_T, *AX_APP_ALGO_PUSH_STRATEGY_PTR;

// algorithm panorama
typedef struct axAPP_ALGO_PANORAMA_T {
    AX_BOOL bEnable;
} AX_APP_ALGO_PANORAMA_T, *AX_APP_ALGO_PANORAMA_PTR;

// algorithm crop encoder threshold config
typedef struct axAPP_ALGO_CROP_THRESHOLD_CONFIG_T {
    AX_F32 fScaleLeft;
    AX_F32 fScaleRight;
    AX_F32 fScaleTop;
    AX_F32 fScaleBottom;
} AX_APP_ALGO_CROP_THRESHOLD_CONFIG_T, *AX_APP_ALGO_CROP_THRESHOLD_CONFIG_PTR;

// algorithm face push filter config
typedef struct axAPP_ALGO_FACE_PUSH_FILTER_CONFIG_T {
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_APP_ALGO_FACE_POSE_BLUR_T stFacePoseBlur;
} AX_APP_ALGO_FACE_PUSH_FILTER_CONFIG_T, *AX_APP_ALGO_FACE_PUSH_FILTER_CONFIG_PTR;

// algorithm common push filter config
typedef struct axAPP_ALGO_COMMON_PUSH_FILTER_CONFIG_T {
    AX_F32 fQuality;
} AX_APP_ALGO_COMMON_PUSH_FILTER_CONFIG_T, *AX_APP_ALGO_COMMON_PUSH_FILTER_CONFIG_PTR;

// algorithm push filter config
typedef struct {
    union {
        AX_APP_ALGO_FACE_PUSH_FILTER_CONFIG_T stFacePushFilterConfig;
        AX_APP_ALGO_COMMON_PUSH_FILTER_CONFIG_T stCommonPushFilterConfig;
    };
} AX_APP_ALGO_PUSH_FILTER_CONFIG_T, *AX_APP_ALGO_PUSH_FILTER_CONFIG_PTR;

// algorithm hvcfp parameter
typedef struct axAPP_ALGO_HVCFP_PARAM_T {
    AX_S32 nFramerate;
    AX_U8 nCropEncoderQpLevel;
    AX_BOOL bPushToWeb;
    AX_CHAR strDetectModelsPath[AX_APP_ALGO_PATH_LEN];
    AX_APP_ALGO_ROI_CONFIG_T stRoiConfig;
    AX_APP_ALGO_TRACK_SIZE_T stTrackSize;
    AX_APP_ALGO_PUSH_STRATEGY_T stPushStrategy;
    AX_APP_ALGO_PANORAMA_T stPanoramaConfig;
    AX_APP_ALGO_CROP_THRESHOLD_CONFIG_T stCropThresholdConfig[AX_APP_ALGO_HVCFP_TYPE_BUTT];
    AX_APP_ALGO_HVCFP_FILTER_CONFIG_T stObjectFliterConfig[AX_APP_ALGO_HVCFP_TYPE_BUTT];
    AX_APP_ALGO_PUSH_FILTER_CONFIG_T stPushFliterConfig[AX_APP_ALGO_HVCFP_TYPE_BUTT];
} AX_APP_ALGO_HVCFP_PARAM_T, *AX_APP_ALGO_HVCFP_PARAM_PTR;

// algorithm motion region
typedef struct axAPP_ALGO_MOTION_REGION_T {
    AX_BOOL bEnable;
    AX_S32 nChan;
    AX_F32 fThresholdY;
    AX_U32 nMbWidth;
    AX_U32 nMbHeight;
    AX_F32 fConfidence;
    AX_APP_RECT_T stRect;
} AX_APP_ALGO_MOTION_REGION_T, *AX_APP_ALGO_MOTION_REGION_PTR;

// algorithm motion parameter
typedef struct axAPP_ALGO_MOTION_PARAM_T {
    AX_BOOL bEnable;
    AX_BOOL bCapture;
    AX_U8 nRegionSize;
    AX_APP_ALGO_MOTION_REGION_T stRegions[AX_APP_ALGO_IVES_MD_REGION_MAX];
} AX_APP_ALGO_MOTION_PARAM_T, *AX_APP_ALGO_MOTION_PARAM_PTR;

// algorithm occlusion parameter
typedef struct axAPP_ALGO_OCCLUSION_PARAM_T {
    AX_BOOL bEnable;
    AX_F32 fThreshold;
    AX_F32 fConfidence;
    AX_F32 fLuxThreshold;
    AX_F32 fLuxConfidence;
} AX_APP_ALGO_OCCLUSION_PARAM_T, *AX_APP_ALGO_OCCLUSION_PARAM_PTR;

// algorithm scene change parameter
typedef struct axAPP_ALGO_SCENE_CHANGE_PARAM_T {
    AX_BOOL bEnable;
    AX_F32 fThreshold;
    AX_F32 fConfidence;
} AX_APP_ALGO_SCENE_CHANGE_PARAM_T, *AX_APP_ALGO_SCENE_CHANGE_PARAM_PTR;

// algorithm recognize parameter
typedef struct axAPP_ALGO_FACE_RECOGNIZE_PARAM_T {
    AX_U32 nCapability;
    AX_F32 fCompareScoreThreshold;
    AX_CHAR strDataBasePath[AX_APP_ALGO_PATH_LEN];
    AX_CHAR strDataBaseName[AX_APP_ALGO_PATH_LEN];
} AX_APP_ALGO_FACE_RECOGNIZE_PARAM_T, *AX_APP_ALGO_FACE_RECOGNIZE_PARAM_PTR;

// algorithm audio parameter
typedef struct axAPP_ALGO_AUDIO_PARAM_T {
    AX_BOOL bEnable;
    AX_F32 fThreshold;
} AX_APP_ALGO_AUDIO_PARAM_T, *AX_APP_ALGO_AUDIO_PARAM_PTR;

// algorithm parameter
typedef struct axAPP_ALGO_PARAM_T {
    AX_U32 nAlgoType;
    AX_APP_ALGO_HVCFP_PARAM_T stHvcfpParam;
    AX_APP_ALGO_MOTION_PARAM_T stMotionParam;
    AX_APP_ALGO_OCCLUSION_PARAM_T stOcclusionParam;
    AX_APP_ALGO_SCENE_CHANGE_PARAM_T stSceneChangeParam;
    AX_APP_ALGO_FACE_RECOGNIZE_PARAM_T stFaceRecognizeParam;
} AX_APP_ALGO_PARAM_T, *AX_APP_ALGO_PARAM_PTR;

// algorithm capture face recognize result
typedef struct axAPP_ALGO_CAPTURE_FACE_RECOGNIZE_RESULT_T {
    AX_U32 nId;
    AX_BOOL bExisted;
    AX_CHAR *pstrRegcognizeName;
    AX_APP_ALGO_IMG_T stFaceImg;
    AX_VOID *pUserData;
    AX_VOID *pPrivateData;
} AX_APP_ALGO_CAPTURE_FACE_RECOGNIZE_RESULT_T, *AX_APP_ALGO_CAPTURE_FACE_RECOGNIZE_RESULT_PTR;

// algorithm face recognize information
typedef struct axAPP_ALGO_FACE_RECOGNIZE_INFO_T {
    AX_CHAR *pstrRecognizeName;
} AX_APP_ALGO_FACE_RECOGNIZE_INFO_T, *AX_APP_ALGO_FACE_RECOGNIZE_INFO_PTR;

// algorithm face recognize operation
typedef struct axAPP_ALGO_FACE_FEATRUE_OP_T {
    AX_APP_ALGO_FACE_RECOGNIZE_OP_E eOp;
    AX_APP_ALGO_FACE_RECOGNIZE_INFO_T stRegconizeInfo;
} AX_APP_ALGO_FACE_FEATRUE_OP_T, *AX_APP_ALGO_FACE_FEATRUE_OP_PTR;

// algorithm face recognize list
typedef struct axAPP_ALGO_FACE_RECOGNIZE_LIST_T {
    AX_U32 nListSize;
    AX_APP_ALGO_FACE_RECOGNIZE_INFO_PTR pLists;
    AX_VOID *pPrivateData;
} AX_APP_ALGO_FACE_RECOGNIZE_LIST_T, *AX_APP_ALGO_FACE_RECOGNIZE_LIST_PTR;

// video algorithm callback
typedef AX_VOID (*AX_APP_VIDEO_ALGO_CALLBACK)(AX_S32 nSnsId, const AX_APP_ALGO_RESULT_PTR pstResult);

// video algorithm capture face recognize callback
typedef AX_S32 (*AX_APP_VIDEO_ALGO_CAPTUREFACERECOGNIZE_CALLBACK)(AX_S32 nSnsId, const AX_APP_ALGO_CAPTURE_FACE_RECOGNIZE_RESULT_PTR pstResult);

static inline AX_APP_ALGO_HVCFP_TYPE_E AX_APP_ALGO_GET_HVCFP_TYPE(const std::string& strObjectCategory) {
    const AX_CHAR* pstrObjectCategory[AX_APP_ALGO_HVCFP_TYPE_BUTT] = {"body", "vehicle", "cycle", "face", "plate"};

    for (size_t i = 0; i < AX_APP_ALGO_HVCFP_TYPE_BUTT; i ++) {
        if (pstrObjectCategory[i] == strObjectCategory) {
            return (AX_APP_ALGO_HVCFP_TYPE_E)i;
        }
    }

    return AX_APP_ALGO_HVCFP_TYPE_BUTT;
}

static inline std::string AX_APP_ALGO_GET_OBJECT_CATEGORY(const AX_APP_ALGO_HVCFP_TYPE_E& eType) {
    const AX_CHAR* pstrObjectCategory[AX_APP_ALGO_HVCFP_TYPE_BUTT] = {"body", "vehicle", "cycle", "face", "plate"};

    if (eType < AX_APP_ALGO_HVCFP_TYPE_BUTT) {
        return pstrObjectCategory[eType];
    }

    return "";
}

static inline AX_APP_ALGO_PLATE_COLOR_TYPE_E AX_APP_ALGO_GET_PLATE_COLOR_TYPE(const std::string& strPlateColor) {
    if (strPlateColor == "blue") {
        return AX_APP_ALGO_PLATE_COLOR_BLUE;
    }
    else if (strPlateColor == "yellow") {
        return AX_APP_ALGO_PLATE_COLOR_YELLOW;
    }
    else if (strPlateColor == "black") {
        return AX_APP_ALGO_PLATE_COLOR_BLACK;
    }
    else if (strPlateColor == "white") {
        return AX_APP_ALGO_PLATE_COLOR_WHITE;
    }
    else if (strPlateColor == "green") {
        return AX_APP_ALGO_PLATE_COLOR_GREEN;
    }
    else if (strPlateColor == "small_new_energy") {
        return AX_APP_ALGO_PLATE_COLOR_NEW_ENERGY;
    }
    else if (strPlateColor == "large_new_energy") {
        return AX_APP_ALGO_PLATE_COLOR_NEW_ENERGY;
    }
#if 0
    else if (strPlateColor == "absence") {
        return AX_APP_ALGO_PLATE_COLOR_UNKOWN;
    }
    else if (strPlateColor == "unknown") {
        return AX_APP_ALGO_PLATE_COLOR_UNKOWN;
    }
#endif

    return AX_APP_ALGO_PLATE_COLOR_UNKOWN;
}

static inline std::string AX_APP_ALGO_GET_PLATE_COLOR(const AX_APP_ALGO_PLATE_COLOR_TYPE_E& eType) {
    if (eType == AX_APP_ALGO_PLATE_COLOR_BLUE) {
        return "blue";
    }
    else if (eType == AX_APP_ALGO_PLATE_COLOR_YELLOW) {
        return "yellow";
    }
    else if (eType == AX_APP_ALGO_PLATE_COLOR_BLACK) {
        return "black";
    }
    else if (eType == AX_APP_ALGO_PLATE_COLOR_WHITE) {
        return "white";
    }
    else if (eType == AX_APP_ALGO_PLATE_COLOR_GREEN) {
        return "green";
    }
    else if (eType == AX_APP_ALGO_PLATE_COLOR_NEW_ENERGY) {
        return "small_new_energy";
    }

    return "unknown";
}

static inline AX_APP_ALGO_FACE_RESPIRATOR_TYPE_E AX_APP_ALGO_GET_RESPIRATOR_TYPE(const std::string& strRespirator) {
    if (strRespirator == "no_respirator") {
        return AX_APP_ALGO_FACE_RESPIRATOR_NONE;
    }
    else if (strRespirator == "surgical") {
        return AX_APP_ALGO_FACE_RESPIRATOR_SURGICAL;
    }
    else if (strRespirator == "anti-pollution") {
        return AX_APP_ALGO_FACE_RESPIRATOR_ANTI_PULLTION;
    }
    else if (strRespirator == "common") {
        return AX_APP_ALGO_FACE_RESPIRATOR_COMMON;
    }
    else if (strRespirator == "kitchen_transparent") {
        return AX_APP_ALGO_FACE_RESPIRATOR_KITCHEN_TRANSPARENT;
    }
#if 0
    else if (strRespirator == "unknown") {
        return AX_APP_ALGO_FACE_RESPIRATOR_UNKOWN;
    }
#endif

    return AX_APP_ALGO_FACE_RESPIRATOR_UNKOWN;
}

static inline std::string AX_APP_ALGO_GET_RESPIRATOR(const AX_APP_ALGO_FACE_RESPIRATOR_TYPE_E& eType) {
    if (eType == AX_APP_ALGO_FACE_RESPIRATOR_NONE) {
        return "no_respirator";
    }
    else if (eType == AX_APP_ALGO_FACE_RESPIRATOR_SURGICAL) {
        return "surgical";
    }
    else if (eType == AX_APP_ALGO_FACE_RESPIRATOR_ANTI_PULLTION) {
        return "anti-pollution";
    }
    else if (eType == AX_APP_ALGO_FACE_RESPIRATOR_COMMON) {
        return "common";
    }
    else if (eType == AX_APP_ALGO_FACE_RESPIRATOR_KITCHEN_TRANSPARENT) {
        return "kitchen_transparent";
    }
#if 0
    else if (eType == AX_APP_ALGO_FACE_RESPIRATOR_UNKOWN) {
        return "unknown";
    }
#endif

    return "unknown";
}

