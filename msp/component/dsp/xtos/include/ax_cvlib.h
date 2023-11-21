#ifndef AX_CVLIB_H
#define AX_CVLIB_H

#include "ax_base_type.h"
#include "ax_dsp_common.h"

AX_S32 AX_DSP_Resize(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_CvtColor(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_JointLR(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_SAD(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_KVM_SPLIT(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_KVM_COMBINE(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_SAD2(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_MAP(AX_DSP_MESSAGE_T *msg);
AX_S32 AX_DSP_NV12COPY(AX_DSP_MESSAGE_T *msg);

#if 0
    AX_S32 ax_dsp_resize16(AX_DSP_MESSAGE_S *msg);
    AX_S32 ax_dsp_uyvyTonv12(AX_DSP_MESSAGE_S *msg);
    AX_S32 ax_dsp_jointLR(AX_DSP_MESSAGE_S *msg);
    AX_S32 ax_dsp_vyuy2uyvy(AX_DSP_MESSAGE_S *msg); //keda unique
    AX_S32 ax_dsp_vyuyTonv12(AX_DSP_MESSAGE_S *msg); //keda unique
    AX_S32 ax_dsp_rgbaTonv12(AX_DSP_MESSAGE_S *msg);
    AX_S32 ax_dsp_nv12Touyvy(AX_DSP_MESSAGE_S *msg);
#endif

#endif
