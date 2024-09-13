#ifndef _SAMPLE_ENGINE_H_
#define _SAMPLE_ENGINE_H_
#include "ax_sys_api.h"
#include "ax_ivps_api.h"

#define SAMPLE_ENGINE_MODEL_FILE "/opt/data/npu/models/yolov5s.axmodel"
#define SAMPLE_ENGINE_OBJ_MAX_COUNT 32
#define INFER_WIDTH     640
#define INFER_HEIGHT    640
#define INFER_FORMAT    AX_FORMAT_RGB888
#define INFER_CHN       3

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        struct
        {
            AX_S32 x, y, width, height;
            AX_S32 class_label;
            AX_F32 prob;

            AX_S32 color;
            AX_CHAR class_name[32];
        } objs[SAMPLE_ENGINE_OBJ_MAX_COUNT];
        AX_S32 obj_count;
    } SAMPLE_ENGINE_Results;

    AX_S32 SAMPLE_ENGINE_Load(AX_CHAR *model_file);
    AX_S32 SAMPLE_ENGINE_Release();
    AX_S32 SAMPLE_ENGINE_Inference(AX_VIDEO_FRAME_T *pFrame, SAMPLE_ENGINE_Results *pResults);
#ifdef __cplusplus
}
#endif

#endif