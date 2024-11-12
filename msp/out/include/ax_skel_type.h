/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_SKEL_TYPE_H_
#define _AX_SKEL_TYPE_H_

#include "ax_global_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief pipeline type
typedef enum axSKEL_PPL_E {
    AX_SKEL_PPL_BODY = 1,           /* body detection pipeline */
    AX_SKEL_PPL_POSE,               /* pose detection pipeline */
    AX_SKEL_PPL_FH,                 /* facehuman detection pipeline */
    AX_SKEL_PPL_HVCFP,              /* hvcfp detection pipeline */
    AX_SKEL_PPL_FACE_FEATURE,       /* face feature detection pipeline */
    AX_SKEL_PPL_MAX,
} AX_SKEL_PPL_E;

/// @brief npu type
typedef enum axSKEL_NPU_TYPE_E {
    AX_SKEL_NPU_DEFAULT = 0,        /* running under default NPU according to system */
    AX_SKEL_STD_VNPU_1 = (1 << 0),  /* running under STD VNPU1 */
    AX_SKEL_STD_VNPU_2 = (1 << 1),  /* running under STD VNPU2 */
    AX_SKEL_STD_VNPU_3 = (1 << 2),  /* running under STD VNPU3 */
    AX_SKEL_BL_VNPU_1 = (1 << 3),   /* running under BIG-LITTLE VNPU1 */
    AX_SKEL_BL_VNPU_2 = (1 << 4)    /* running under BIG-LITTLE VNPU2 */
} AX_SKEL_NPU_TYPE_E;

/// @brief init parameter struct
typedef struct axSKEL_INIT_PARAM_T {
    const AX_CHAR *pStrModelDeploymentPath;
} AX_SKEL_INIT_PARAM_T;

/// @brief rect struct
typedef struct axSKEL_RECT_T {
    AX_F32 fX;
    AX_F32 fY;
    AX_F32 fW;
    AX_F32 fH;
} AX_SKEL_RECT_T;

/// @brief point struct
typedef struct axSKEL_POINT_T {
    AX_F32 fX;
    AX_F32 fY;
} AX_SKEL_POINT_T;

/// @brief point set struct
typedef struct axSKEL_POINT_SET_S {
    const AX_CHAR *pstrObjectCategory;
    AX_SKEL_POINT_T stPoint;
    AX_F32 fConfidence;
} AX_SKEL_POINT_SET_S;

/// @brief meta info struct
typedef struct axSKEL_META_INFO_T {
    AX_CHAR *pstrType;
    AX_CHAR *pstrValue;
} AX_SKEL_META_INFO_T;

/// @brief config item struct
typedef struct axSKEL_CONFIG_ITEM_T {
    AX_CHAR *pstrType;
    AX_VOID *pstrValue;
    AX_U32 nValueSize;
} AX_SKEL_CONFIG_ITEM_T;

/// @brief config struct
typedef struct axSKEL_CONFIG_T {
    AX_U32 nSize;
    AX_SKEL_CONFIG_ITEM_T *pstItems;
} AX_SKEL_CONFIG_T;

/// @brief handle parameter struct
typedef struct axSKEL_HANDLE_PARAM_T {
    AX_SKEL_PPL_E ePPL;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nFrameDepth;
    AX_U32 nFrameCacheDepth;
    AX_U32 nIoDepth;
    AX_U32 nNpuType;
    AX_SKEL_CONFIG_T stConfig;
} AX_SKEL_HANDLE_PARAM_T;

/// @brief feature item struct
typedef struct axSKEL_FEATURE_ITEM_T {
    AX_CHAR *pstrType;
    AX_VOID *pstrValue;
    AX_U32 nValueSize;
} AX_SKEL_FEATURE_ITEM_T;

/// @brief tract status
typedef enum axSKEL_TRACK_STATUS_E {
    AX_SKEL_TRACK_STATUS_NEW,
    AX_SKEL_TRACK_STATUS_UPDATE,
    AX_SKEL_TRACK_STATUS_DIE,
    AX_SKEL_TRACK_STATUS_SELECT,
    AX_SKEL_TRACK_STATUS_MAX,
} AX_SKEL_TRACK_STATUS_E;

/// @brief crop frame struct
typedef struct axSKEL_CROP_FRAME_T {
    AX_U64 nFrameId;
    AX_U8 *pFrameData;
    AX_U32 nFrameDataSize;
    AX_U32 nFrameWidth;
    AX_U32 nFrameHeight;
} AX_SKEL_CROP_FRAME_T;

/// @brief pose blur config
typedef struct axSKEL_POSE_BLUR_T {
    AX_F32 fPitch;
    AX_F32 fYaw;
    AX_F32 fRoll;
    AX_F32 fBlur; // 0 - 1
} AX_SKEL_POSE_BLUR_T;

/// @brief object item struct
typedef struct axSKEL_OBJECT_ITEM_T {
    const AX_CHAR *pstrObjectCategory;
    AX_SKEL_RECT_T stRect;
    AX_U64 nFrameId;
    AX_U64 nTrackId;
    AX_SKEL_TRACK_STATUS_E eTrackState;
    AX_F32 fConfidence;
    AX_SKEL_POSE_BLUR_T stFacePostBlur;
    AX_BOOL bCropFrame;
    AX_SKEL_CROP_FRAME_T stCropFrame;
    AX_BOOL bPanoraFrame;
    AX_SKEL_CROP_FRAME_T stPanoraFrame;
    AX_U8 nPointSetSize;
    AX_SKEL_POINT_SET_S *pstPointSet;
    AX_U8 nFeatureSize;
    AX_SKEL_FEATURE_ITEM_T *pstFeatureItem;
    AX_U32 nMetaInfoSize;
    AX_SKEL_META_INFO_T *pstMetaInfo;
} AX_SKEL_OBJECT_ITEM_T;

typedef struct axSKEL_FRAME_CACHE_LIST_T {
    AX_U64 nFrameId;
    AX_U32 nStreamId;
} AX_SKEL_FRAME_CACHE_LIST_T;

/// @brief result struct
typedef struct axSKEL_RESULT_T {
    AX_U64 nFrameId;
    AX_U32 nStreamId;
    AX_U32 nOriginalWidth;
    AX_U32 nOriginalHeight;
    AX_U32 nObjectSize;
    AX_SKEL_OBJECT_ITEM_T *pstObjectItems;
    AX_U32 nCacheListSize;
    AX_SKEL_FRAME_CACHE_LIST_T *pstCacheList;
    AX_VOID *pUserData;
    AX_VOID *pPrivate;
} AX_SKEL_RESULT_T;

/// @brief frame struct
typedef struct axSKEL_FRAME_T {
    AX_U64 nFrameId;
    AX_U32 nStreamId;
    AX_VIDEO_FRAME_T stFrame;
    AX_VOID *pUserData;
} AX_SKEL_FRAME_T;

/// @brief ppl config struct
typedef struct axSKEL_PPL_CONFIG_T {
    AX_SKEL_PPL_E ePPL;
    AX_CHAR *pstrPPLConfigKey;
} AX_SKEL_PPL_CONFIG_T;

/// @brief capability struct
typedef struct axSKEL_CAPABILITY_T {
    AX_U32 nPPLConfigSize;
    AX_SKEL_PPL_CONFIG_T *pstPPLConfig;
    AX_U32 nMetaInfoSize;
    AX_SKEL_META_INFO_T *pstMetaInfo;
    AX_VOID *pPrivate;
} AX_SKEL_CAPABILITY_T;

/// @brief version info struct
typedef struct axSKEL_VERSION_INFO_T {
    AX_CHAR *pstrVersion;
    AX_U32 nMetaInfoSize;
    AX_SKEL_META_INFO_T *pstMetaInfo;
    AX_VOID *pPrivate;
} AX_SKEL_VERSION_INFO_T;

/// @brief handle definition
typedef AX_VOID *AX_SKEL_HANDLE;

/// @brief callback definition
typedef AX_VOID (*AX_SKEL_RESULT_CALLBACK_FUNC)(AX_SKEL_HANDLE pHandle, AX_SKEL_RESULT_T *pstResult, AX_VOID *pUserData);

/* begin of config param */
/// @brief Common Threshold Config
typedef struct axSKEL_COMMON_THRESHOLD_CONFIG_T {
    AX_F32 fValue;
} AX_SKEL_COMMON_THRESHOLD_CONFIG_T;
// cmd: "body_max_target_count", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "vehicle_max_target_count", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "cycle_max_target_count", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "body_confidence", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "face_confidence", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "vehicle_confidence", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "cycle_confidence", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "plate_confidence", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "crop_encoder_qpLevel", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "push_bind_enable", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_S *
// cmd: "track_disable", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *
// cmd: "push_disable", value_type: AX_SKEL_COMMON_THRESHOLD_CONFIG_T *

/// @brief object size filter config
typedef struct axSKEL_OBJECT_SIZE_FILTER_CONFIG_T {
    AX_U32 nWidth;
    AX_U32 nHeight;
} AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T;
// cmd: "body_min_size", value_type: AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T *
// cmd: "face_min_size", value_type: AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T *
// cmd: "vehicle_min_size", value_type: AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T *
// cmd: "cycle_min_size", value_type: AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T *
// cmd: "plate_min_size", value_type: AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T *

/// @brief config target item
typedef struct axSKEL_TARGET_ITEMG_T {
    const AX_CHAR *pstrObjectCategory;
} AX_SKEL_TARGET_ITEM_T;

/// @brief config object target
typedef struct axSKEL_TARGET_CONFIG_T {
    AX_U32 nSize;
    AX_SKEL_TARGET_ITEM_T *pstItems;
} AX_SKEL_TARGET_CONFIG_T;
// cmd:"target_config", value_type: AX_SKEL_TARGET_CONFIG_T *

typedef enum axSKEL_ANALYZER_ATTR_E {
    AX_SKEL_ANALYZER_ATTR_NONE = 0,
    AX_SKEL_ANALYZER_ATTR_FACE_FEATURE,
    AX_SKEL_ANALYZER_ATTR_FACE_ATTRIBUTE,
    AX_SKEL_ANALYZER_ATTR_PLATE_ATTRIBUTE,
    AX_SKEL_ANALYZER_ATTR_MAX,
} AX_SKEL_ANALYZER_ATTR_E;

/// @brief config target attribute analyzer
typedef struct axSKEL_ANALYZER_CONFIG_T {
    AX_U32 nSize;
    AX_SKEL_ANALYZER_ATTR_E *peItems;
} AX_SKEL_ANALYZER_CONFIG_T;
// cmd:"analyzer_attr_config", value_type: AX_SKEL_ATTR_FILTER_CONFIG_T *

/// @brief roi config
typedef struct axSKEL_ROI_CONFIG_T {
    AX_BOOL bEnable;
    AX_SKEL_RECT_T stRect;
} AX_SKEL_ROI_CONFIG_T;
// cmd:"detect_roi", value_type: AX_SKEL_ROI_CONFIG_T*

/// @brief push mode
typedef enum axSKEL_PUSH_MODE_E {
    AX_SKEL_PUSH_MODE_FAST = 1,
    AX_SKEL_PUSH_MODE_INTERVAL,
    AX_SKEL_PUSH_MODE_BEST,
    AX_SKEL_PUSH_MODE_MAX
} AX_SKEL_PUSH_MODE_E;

/// @brief push config
typedef struct axSKEL_PUSH_STRATEGY_T {
    AX_SKEL_PUSH_MODE_E ePushMode;
    AX_U32 nIntervalTimes;      // only for AX_SKEL_PUSH_MODE_INTERVAL or AX_SKEL_PUSH_MODE_FAST
    AX_U32 nPushCounts;         // only for AX_SKEL_PUSH_MODE_INTERVAL or AX_SKEL_PUSH_MODE_FAST
    AX_BOOL bPushSameFrame;       // AX_FALSE: push cross frame; AX_TRUE: push same frame
} AX_SKEL_PUSH_STRATEGY_T;
// cmd: "push_strategy", value_type: AX_SKEL_ROI_CONFIG_T *

/// @brief Crop encoder threshold config
typedef struct axSKEL_CROP_ENCODER_THRESHOLD_CONFIG_T {
    AX_F32 fScaleLeft;
    AX_F32 fScaleRight;
    AX_F32 fScaleTop;
    AX_F32 fScaleBottom;
} AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T;
// cmd:"crop_encoder",  value_type: AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T *

/// @brief Reszie panorama config
typedef struct axSKEL_RESIZE_CONFIG_T {
    AX_F32 fW;
    AX_F32 fH;
} AX_SKEL_RESIZE_CONFIG_T;
// cmd:"resize_panorama_encoder_config",  value_type: AX_SKEL_RESIZE_CONFIG *

/// @brief push panorama config
typedef struct axSKEL_PUSH_PANORAMA_CONFIG_T {
    AX_BOOL bEnable;
    AX_U32 nQuality;
} AX_SKEL_PUSH_PANORAMA_CONFIG_T;
// cmd:"push_panorama",  value_type: AX_SKEL_PUSH_PANORAMA_CONFIG_T *

/// @brief face attr filter config
typedef struct axSKEL_FACE_ATTR_FILTER_CONFIG_T {
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_SKEL_POSE_BLUR_T stPoseblur;
} AX_SKEL_FACE_ATTR_FILTER_CONFIG_T;

/// @brief common attr filter config
typedef struct axSKEL_COMMON_ATTR_FILTER_CONFIG_T {
    AX_F32 fQuality; // 0 - 1
} AX_SKEL_COMMON_ATTR_FILTER_CONFIG_T;

/// @brief attr filter config
typedef struct axSKEL_ATTR_FILTER_CONFIG_T {
    union {
        AX_SKEL_FACE_ATTR_FILTER_CONFIG_T stFaceAttrFilterConfig;      // face
        AX_SKEL_COMMON_ATTR_FILTER_CONFIG_T stCommonAttrFilterConfig;  // body,vehicle,cycle,plate
    };
} AX_SKEL_ATTR_FILTER_CONFIG_T;
// cmd:"push_quality_face", value_type: AX_SKEL_ATTR_FILTER_CONFIG_T *
// cmd:"push_quality_body", value_type: AX_SKEL_ATTR_FILTER_CONFIG_T *
// cmd:"push_quality_vehicle", value_type: AX_SKEL_ATTR_FILTER_CONFIG_T *
// cmd:"push_quality_cycle", value_type: AX_SKEL_ATTR_FILTER_CONFIG_T *
// cmd:"push_quality_plate", value_type: AX_SKEL_ATTR_FILTER_CONFIG_T *

/* end of config param */

/* begin of search */

/// @brief search: group feature param
typedef struct axSKEL_SEARCH_FEATURE_PARAM_T {
    AX_U32 nBatchSize;
    AX_U64* pObjectIds;    // 1 dimensional[batch_size] continuous storage
    AX_U8* pFeatures;       // 1 dimensional[batch_size*feature_size] continuous storage
    AX_VOID **ppObjectInfos;    // 1 dimensional[batch_size] continuous storage // 'object_infos' can be null
} AX_SKEL_SEARCH_FEATURE_PARAM_T;

/// @brief search: group insert feature param
typedef struct axSKEL_SEARCH_PARAM_T {
    AX_U32 nBatchSize;
    AX_U32 nTop_k;
    AX_U8 *pFeatures;    // 1 dimensional[batch_size*feature_size] continuous storage
    // If the following types are valid, the filter policy will be activated. If not, the filter
    // policy will not be performed
} AX_SKEL_SEARCH_PARAM_T;

/// @brief search: result of search
typedef struct axSKEL_SEARCH_RESULT_T {
    AX_U32 nBatchSize;      // object number of search
    AX_U32 nTop_k;           // object with k highest scores in object group
    AX_F32 *pfScores;          // scorces of object matching in object group
    AX_U64 *pObjectIds;    // object id of object matching in object group
    AX_VOID **ppObjectInfos;            // information of object matching in object group(pass-through by user)
    AX_VOID *pPrivate;
} AX_SKEL_SEARCH_RESULT_T;

/* end of search */

#ifdef __cplusplus
}
#endif

#endif /* _AX_SKEL_TYPE_H_ */
