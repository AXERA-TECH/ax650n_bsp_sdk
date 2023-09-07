/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#ifndef _AX_LENS_AF_STRUCT_H_
#define _AX_LENS_AF_STRUCT_H_

#include "ax_base_type.h"
#include "ax_isp_3a_struct.h"

#define ACTUATOR_MAX_NUM 8
#define TRACK_TABLE_MAX_ROW 128
#define TRACK_TABLE_MAX_COL 32

typedef struct _AX_LENS_ACTUATOR_AF_FUNC_T_ {
    /* af focus actuator */
    AX_S32 (*pfn_af_focus_init)(AX_U8 nPipeId, AX_U8 nBusNum, AX_U8 nCs);
    AX_U8  (*pfn_af_focus_rstb_status)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_focus_get_status)(AX_U8 nPipeId);
    AX_S32 (*pfn_af_focus_to_dest_pos)(AX_U8 nPipeId, AX_S32 nPos, AX_U32 nPps);
    AX_S32 (*pfn_af_focus_to_dest_pos_direction)(AX_U8 nPipeId, AX_S32 nPos, AX_S32 nDirection);
    AX_S32 (*pfn_af_focus_exit)(AX_U8 nPipeId);
    /* af zoom actuator */
    AX_S32 (*pfn_af_zoom_init)(AX_U8 nPipeId, AX_U8 nBusNum, AX_U8 nCs);
    AX_U8  (*pfn_af_zoom_rstb_status)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_zoom1_get_status)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_zoom2_get_status)(AX_U8 nPipeId);
    AX_S32 (*pfn_af_zoom1_to_dest_pos)(AX_U8 nPipeId, AX_S32 nPos, AX_U32 nPps);
    AX_S32 (*pfn_af_zoom2_to_dest_pos)(AX_U8 nPipeId, AX_S32 nPos, AX_U32 nPps);
    AX_S32 (*pfn_af_zoom1_to_dest_pos_direction)(AX_U8 nPipeId, AX_S32 nPos, AX_S32 nDirection);
    AX_S32 (*pfn_af_zoom2_to_dest_pos_direction)(AX_U8 nPipeId, AX_S32 nPos, AX_S32 nDirection);
    AX_S32 (*pfn_af_zoom_exit)(AX_U8 nPipeId);

} AX_LENS_ACTUATOR_AF_FUNC_T;

typedef enum AxAfMotorStatus_s
{
    AF_MOTOR_IDLE          = 0,
    AF_MOTOR_MOVE_FINISHED = 1,
    AF_MOTOR_PI_FOUND      = 2,
    AF_MOTOR_PI_FIND_ERR   = 3,
} AxAfMotorStatus_t;


typedef struct{
    AX_U8   curveTableRows;
    AX_U8   curveTableCols;
    AX_S32  curveTable[TRACK_TABLE_MAX_ROW*TRACK_TABLE_MAX_COL];
    AX_S32  distanceTable[TRACK_TABLE_MAX_COL];
}AX_LENS_AF_TRACK_TABLE_T;


typedef struct{

    /* Focus Motor Params */
    AX_S32  focusHwMaxStep;          // Max Step of Focus Motor Hardware
    AX_S32  focusHwMinPos;           // Min Position of Focus Motor Hardware
    AX_S32  focusHwMaxPos;           // Max Position of Focus Motor Hardware
    AX_S32  focusHwWideFarPos;       // Focus Motor Position of Far End in Wide Angle
    AX_U8   focusRangeExtBase;       // Base-Extended-Range of Scan and Search (Unit:Step)
    AX_U8   focusRangeExtRatio;      // How Many focusRangeExtBase are Added to Theoretical Far & Near End

    /* Zoom Motors Params */
    AX_U8   zoomRatioToPosTableRows;                      //
    AX_F32  zoomRatioToPosTable[TRACK_TABLE_MAX_ROW*3];   //
    AX_S32  zoom1HwMaxStep;          // Max Step of Zoom1 Motor Hardware
    AX_S32  zoom1HwMinPos;           // Min Position of Zoom1 Motor Hardware
    AX_S32  zoom1HwMaxPos;           // Max Position of Zoom1 Motor Hardware
    AX_S32  zoom2HwMaxStep;          // Max Step of Zoom2 Motor Hardware
    AX_S32  zoom2HwMinPos;           // Min Position of Zoom2 Motor Hardware
    AX_S32  zoom2HwMaxPos;           // Max Position of Zoom2 Motor Hardware

    /* Object Distance */
    AX_U8   distLimitNear;           // Nearest Object Distance (Unit:cm)

    AX_LENS_AF_TRACK_TABLE_T  tTrackTable;      // Zoom Track Table
}AX_LENS_AF_HARDWARE_PARAMS_T;


typedef struct{
    AX_S32 focusPiOffset;
    AX_S32 zoom1PiOffset;
    AX_S32 zoom2PiOffset;
} AX_LENS_AF_PI_CALIB_PARAMS_T;


typedef struct{
    AX_LENS_AF_PI_CALIB_PARAMS_T  tPiParams;
}AX_LENS_AF_CALIB_PARAMS_T;


typedef struct {
    /* Optical Params, Mechanical Params, AxAF Algo Tuning Params of A Lens. */
    AX_S32 (*pfn_af_lens_get_tuning_params)(AX_U8 nPipeId,
                                            AX_ISP_IQ_CAF_PARAM_T *pAfTuningParams);
    AX_S32 (*pfn_af_lens_get_hardware_params)(AX_U8 nPipeId,
                                              AX_LENS_AF_HARDWARE_PARAMS_T *pAfHwParams);
    AX_S32 (*pfn_af_lens_get_calib_params)(AX_U8 nPipeId,
                                           AX_LENS_AF_CALIB_PARAMS_T *pAfHwParams);
    /* af focus actuator */
    AX_S32 (*pfn_af_focus_init)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_focus_rstb_status)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_focus_get_status)(AX_U8 nPipeId);
    AX_S32 (*pfn_af_focus_to_dest_pos)(AX_U8 nPipeId, AX_S32 nPos, AX_U32 nPps);
    AX_S32 (*pfn_af_focus_move)(AX_U8 nPipeId, AX_S32 nStep, AX_S32 nDirection);
    AX_S32 (*pfn_af_focus_exit)(AX_U8 nPipeId);

    /* af zoom actuator */
    AX_S32 (*pfn_af_zoom_init)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_zoom_rstb_status)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_zoom1_get_status)(AX_U8 nPipeId);
    AX_U8  (*pfn_af_zoom2_get_status)(AX_U8 nPipeId);
    AX_S32 (*pfn_af_zoom1_to_dest_pos)(AX_U8 nPipeId, AX_S32 nPos, AX_U32 nPps);
    AX_S32 (*pfn_af_zoom2_to_dest_pos)(AX_U8 nPipeId, AX_S32 nPos, AX_U32 nPps);
    AX_S32 (*pfn_af_zoom1_move)(AX_U8 nPipeId, AX_S32 nStep, AX_S32 nDirection);
    AX_S32 (*pfn_af_zoom2_move)(AX_U8 nPipeId, AX_S32 nStep, AX_S32 nDirection);
    AX_S32 (*pfn_af_zoom_exit)(AX_U8 nPipeId);

} AX_LENS_AF_FUNCS_T;

#endif

