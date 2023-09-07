/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#ifndef __OSD_CONFIG_H__
#define __OSD_CONFIG_H__

#include <map>
#include <vector>
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "ax_base_type.h"
#include "string.h"
#define AX_MAX_RGN_NUM (32)
#define AX_MAX_CHN_RGN_NUM (16)
#define AX_CHN_RGN_NUM (5)
#define MAX_OSD_CHN_COUNT (3)
#define OSD_ALIGN_X_OFFSET (2)
#define OSD_ALIGN_Y_OFFSET (2)
#define OSD_ALIGN_WIDTH (2)
#define OSD_ALIGN_HEIGHT (2)
#define IS_VALID_CHANNEL(chn) ((chn) >= 0 && (chn) < MAX_WEB_CHANNEL_NUM)
#define IS_VALID_OSD_NUM(num) ((num) >= 0 && (num) <= AX_MAX_VO_NUM)

#define BASE_FONT_SIZE (16)
#define MAX_OSD_TIME_CHAR_LEN (32)
#define MAX_OSD_STR_CHAR_LEN (128)
#define MAX_OSD_WSTR_CHAR_LEN (256)

#ifndef RANGE_CHECK
#define RANGE_CHECK(v, min, max) (((v) < (min)) ? 0 : ((v) > (max)) ? 0 : 1)
#endif

#define CHECK_POINTER(p)             \
    if (!p) {                        \
        return OSD_ERR_NULL_POINTER; \
    }

#define CHECK_CHANNEL(chn)                    \
    if (!IS_VALID_CHANNEL(chn)) {             \
        return OSD_ERR_INVALID_VIDEO_CHANNEL; \
    }

#define CHECK_OSD_NUMBER(num)         \
    if (!IS_VALID_OSD_NUM(num)) {     \
        return OSD_ERR_INVALID_PARAM; \
    }

#define CHECK_OSD_HANDLE(chn, handle)                     \
    if (!IS_VALID_OSD_NUM(handle)) {                      \
        return OSD_ERR_INVALID_HANDLE;                    \
    }                                                     \
    if (!RANGE_CHECK(handle, 0, m_nOsdCfgNum[chn] - 1)) { \
        return OSD_ERR_INVALID_HANDLE;                    \
    }

/* OSD ERROR */
typedef enum {
    OSD_SUCCESS = 0,
    OSD_ERR_NULL_POINTER,
    OSD_ERR_INVALID_HANDLE,
    OSD_ERR_INVALID_VIDEO_CHANNEL,
    OSD_ERR_INVALID_PARAM,
    OSD_ERR_MAX
} OSD_ERR_CODE_E;

/* OSD Align Type */
typedef enum {
    OSD_ALIGN_TYPE_LEFT_TOP,     /* 左上角对齐 */
    OSD_ALIGN_TYPE_RIGHT_TOP,    /* 右上角对齐 */
    OSD_ALIGN_TYPE_LEFT_BOTTOM,  /* 左下角对齐 */
    OSD_ALIGN_TYPE_RIGHT_BOTTOM, /* 右下角对齐 */
    OSD_ALIGN_TYPE_MAX
} OSD_ALIGN_TYPE_E;

/* OSD Type */
typedef enum {
    OSD_TYPE_TIME,            /* 时间 */
    OSD_TYPE_PICTURE,         /* 图片 */
    OSD_TYPE_STRING_CHANNEL,  /* 字符串 */
    OSD_TYPE_STRING_LOCATION, /* 字符串 */
    OSD_TYPE_PRIVACY,         /* 矩形遮挡 */
    OSD_TYPE_RECT,            /* 矩形框 */
    OSD_TYPE_MAX
} OSD_TYPE_E;

/* 隐私遮挡类型 */
typedef enum {
    OSD_PRIVACY_TYPE_LINE,    /* 直线遮挡 */
    OSD_PRIVACY_TYPE_RECT,    /* 矩形遮挡 */
    OSD_PRIVACY_TYPE_POLYGON, /* 多边形遮挡 */
    OSD_PRIVACY_TYPE_MOSAIC,  /* 马赛克遮挡 */
    OSD_PRIVACY_TYPE_MAX
} OSD_PRIVACY_TYPE_E;

/* 图片类型OSD属性 */
typedef struct _OSD_PIC_ATTR_T {
    AX_S32 nWidth;           /* 图片分辨率 */
    AX_S32 nHeight;
    AX_CHAR szFileName[128]; /* 图片所在位置 /xxx/xxx/xxx.bmp */
} OSD_PIC_ATTR_T;

/* 字符串类型OSD属性 */
typedef struct _OSD_STR_ATTR_T {
    AX_S32 nFontSize;                    /* 字体大小，-1：默认大小 */
    AX_U32 nColor;                       /* 颜色，A,R,G,B各一个字节 */
    AX_CHAR szStr[MAX_OSD_STR_CHAR_LEN]; /* 字符串内容 */

} OSD_STR_ATTR_T;

/* 时间串类型OSD属性 */
typedef struct _OSD_TIME_ATTR_T {
    OSD_DATE_FORMAT_E eFormat; /* 格式 */
    AX_U32 nFontSize;          /* 字体大小，-1：默认大小 */
    AX_U32 nColor;             /* 颜色，A,R,G,B各一个字节 */
} OSD_TIME_ATTR_T;

typedef struct _OSD_POINT_T {
    AX_S16 x;
    AX_S16 y;
} OSD_POINT_T;

/* 隐私遮挡类型OSD属性 */
typedef struct _OSD_PRIVACY_ATTR_T {
    OSD_PRIVACY_TYPE_E eType; /* 遮挡类型 */
    AX_U32 nLineWidth;        /* 线宽 */
    AX_U32 nColor;            /* 填充色或线的颜色 */
    AX_BOOL bSolid;           /* 是否填充颜色 */
    OSD_POINT_T tPt[4];       /* 直线的两个点或者矩形/多边形的4个点,顺时针 */
} OSD_PRIVACY_ATTR_T;

typedef struct {
    AX_U32 nLineWidth;
} OSD_RECT_ATTR_T;

/* OSD Config */
typedef struct _OSD_CFG_T {
    AX_BOOL bEnable{AX_FALSE};                        /* 是否显示 */
    OSD_TYPE_E eType{OSD_TYPE_MAX};                   /* 类型 */
    AX_BOOL bChanged{AX_FALSE};                       /* 是否使用初始值*/
    OSD_ALIGN_TYPE_E eAlign{OSD_ALIGN_TYPE_LEFT_TOP}; /* 对齐方式 */
    AX_S32 nBoundaryX; /* X轴上，OSD显示位置与边界距离，OSD_ALIGN_LEFT_TOP/OSD_ALIGN_LEFT_BOTTOM对齐方式，则是与左边界距离，否则右边界 */
    AX_S32 nBoundaryY; /* Y轴上，OSD显示位置与边界距离，OSD_ALIGN_LEFT_TOP/OSD_ALIGN_RIGHT_TOP对齐方式，则是与上边界距离，否则下边界 */
    AX_S32 nBoundaryW; /* OSD 显示区域的宽 */
    AX_S32 nBoundaryH; /* OSD 显示区域的高 */
    AX_S32 nZIndex;
    union {
        OSD_PIC_ATTR_T tPicAttr;         /* 图片OSD属性 */
        OSD_STR_ATTR_T tStrAttr;         /* 字符串OSD属性 */
        OSD_TIME_ATTR_T tTimeAttr;       /* 时间OSD属性 */
        OSD_PRIVACY_ATTR_T tPrivacyAttr; /* 隐私遮挡OSD属性 */
        OSD_RECT_ATTR_T tRectAttr;       /* 矩形框属性 */
    };
} OSD_CFG_T;

typedef struct _OSD_SENSOR_CONFIG_T {
    AX_BOOL bEnable;
    std::map<AX_U8, std::map<AX_U8, std::vector<OSD_CFG_T>>> mapGrpChnConfig;
} OSD_SENSOR_CONFIG_T;

typedef struct _OSD_FONT_STYLE {
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nTimeFontSize;
    AX_U32 nRectLineWidth;
    AX_U32 nBoundaryX;
} OSD_FONT_STYLE;

class COSDStyle : public CAXSingleton<COSDStyle> {
    friend class CAXSingleton<COSDStyle>;

public:
    virtual AX_BOOL InitOnce() override;
    AX_U32 GetTimeFontSize(AX_U32 nHeight);
    AX_U32 GetRectLineWidth(AX_U32 nHeight);
    AX_U32 GetBoundaryX(AX_U32 nHeight);

private:
    COSDStyle() = default;
    ~COSDStyle() = default;

private:
    const OSD_FONT_STYLE g_arrOsdStyle[10]{{3840, 2160, 128, 4, 56}, {2048, 1536, 64, 4, 48}, {1920, 1080, 48, 2, 24},
                                           {1280, 720, 48, 2, 20},   {1024, 768, 48, 2, 16},  {720, 576, 32, 2, 12},
                                           {640, 480, 32, 2, 8},     {384, 288, 16, 2, 2}};
    AX_U32 GetOsdStyleIndex(AX_U32 nWidth);
};
#endif  // __OSD_CONFIG_H__
