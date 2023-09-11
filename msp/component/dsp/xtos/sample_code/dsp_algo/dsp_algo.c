/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <xtensa/config/core.h>
#include <xtensa/hal.h>
#include <string.h>

#if defined BOARD
    #include <xtensa/xtbsp.h>
#endif
#include <stdlib.h>
#include <stdio.h>
/* We want to include the local copy of xos.h */
#include <xtensa/xtutil.h>
#include <ax_base_type.h>
#include <ax_dsp_common.h>
#include <ax_base_type.h>
#include <ax650x_api.h>
#include <xtensa/idma.h>
#include <xtensa/hal.h>
#include <ax_cvlib.h>
#include "ax_dsp_def.h"
#include "ax_dsp_tm.h"
#include "ax_dsp_trace.h"

#define u32 unsigned int
#define u64 unsigned long long

extern void _memmap_seg_dram0_0_end;
extern void _memmap_seg_dram0_0_max;
extern void _memmap_seg_dram1_0_end;
extern void _memmap_seg_dram1_0_max;
extern int AX_DSP_AlgoGaussianBlur(AX_DSP_MESSAGE_T *msg);
extern int AX_DSP_AlgoArrayadd(AX_DSP_MESSAGE_T *msg);
extern AX_S32 SAMPLE_VDSPCopy(AX_VOID);
extern int AX_DSP_TestIdmaPerf(int isread);
extern int AX_DSP_TestFBCDC(AX_DSP_MESSAGE_T *msg);

void AX_VDSP_BspTmInit(AX_VOID)
{
    AX_S32 pBuffer[2];
    AX_S32 pBufferSize[2];
    pBuffer[0] = (AX_S32)&_memmap_seg_dram0_0_end;
    pBuffer[1] = (AX_S32)&_memmap_seg_dram1_0_end;
    pBufferSize[0] = &_memmap_seg_dram0_0_max - &_memmap_seg_dram0_0_end;
    pBufferSize[1] = &_memmap_seg_dram1_0_max - &_memmap_seg_dram1_0_end;
    AX_VDSP_TM_Init(pBuffer, pBufferSize);
}
static int AX_DSP_AlgoProcess(AX_DSP_MESSAGE_T *msg)
{
    int ret = 0;
    AX_VDSP_LOG_INFO("0x%x, 0x%x", msg->u32CMD, msg->u32MsgId);
    switch (msg->u32CMD) {
    case AX_DSP_CMD_INIT:
        break;
    case AX_DSP_CMD_EXIT:
        break;
    case AX_DSP_CMD_TEST_COPY:
        ret =  SAMPLE_VDSPCopy();
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_TEST_ADD: {
        ret = AX_DSP_AlgoArrayadd(msg);
        break;
    }
    case AX_DSP_CMD_TEST_FBCDC:
        ret = AX_DSP_TestFBCDC(msg);
        break;
    case AX_DSP_CMD_TEST_DDRPERF:
        AX_DSP_TestIdmaPerf(msg->u32Body[0]); //it's a infinite loop
        break;
    case AX_DSP_CMD_TEST_GAUSSIBLUR:
        ret =  AX_DSP_AlgoGaussianBlur(msg);
        break;
    case AX_DSP_CMD_FBCDC_CHANGE:
        AX_VDSP_Update_Fbcdc_Para();
        break;
    case AX_DSP_CMD_LOGLEVEL:
#define LOGLEVEL_GET 0
#define LOGLEVEL_SET 1
        if (msg->u32Body[0] == LOGLEVEL_GET) {
            msg->u32Body[1] = AX_GET_LOG_LEVEL();
        } else if (msg->u32Body[0] == LOGLEVEL_SET) {
            AX_SET_LOG_LEVEL(msg->u32Body[1]);
        }
        break;
    case AX_DSP_CMD_OPERATOR_RESIZE:
        ret = AX_DSP_Resize(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_CVTCOLOR:
        ret = AX_DSP_CvtColor(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_JOINT_LR:
        ret = AX_DSP_JointLR(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_SAD:
        ret = AX_DSP_SAD2(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_KVM_SPLIT:
        ret = AX_DSP_KVM_SPLIT(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_KVM_COMBINE:
        ret = AX_DSP_KVM_COMBINE(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_MAP:
        ret = AX_DSP_MAP(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_NV12COPY:
        ret = AX_DSP_NV12COPY(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_NV12Blending:
        ret = AX_DSP_NV12Blending(msg);
        msg->u32Body[0] = ret;
        break;
    case AX_DSP_CMD_OPERATOR_COPY:
        ret = AX_DSP_COPY(msg);
        msg->u32Body[0] = ret;
        break;
    default:
        break;
    }
    return ret;
}
static int AX_DSP_Algo(void *arg, int unused)
{
    AX_S32 ret, fromid;
    AX_DSP_MESSAGE_T msg;
    AX_VDSP_LOG_INFO("AX_DSP_Algo!\r\n");
    while (1) {
        fromid = AX_Mailbox_Read((u32 *)&msg, sizeof(AX_DSP_MESSAGE_T));
        if (fromid < 0)continue;
        ret = AX_DSP_AlgoProcess(&msg);
        if (ret != 0) {
            AX_VDSP_LOG_ERROR("AlgoProcess Error\n");
        }

        if (msg.u32CMD == AX_DSP_CMD_SLEEP)continue; //no ack needed

        ret = AX_Mailbox_Write(fromid, (uint32_t *)&msg, sizeof(AX_DSP_MESSAGE_T));
#if 0
        {
            //dump ack msg
            AX_U32 *ptr = &msg;
            int i = 0;
            for (; i < 8; i++)
                AX_VDSP_LOG_INFO("0x%x ", ptr[i]);
            AX_VDSP_LOG_INFO("KEY %d\n", xthal_get_ccount());
        }
#endif
        if (ret != sizeof(AX_DSP_MESSAGE_T)) {
            AX_VDSP_LOG_ERROR("mailbox write, ret = %lx\r\n", ret);
        }
    }
    return 0;
}
static AX_VOID AX_VDSP_BspInit(AX_VOID)
{
    _procid =  xthal_get_prid() & 1; //A0 FOR DSP0;A1 FOR DSP1

    xtbsp_board_init();
    AX_VDSP_Update_Fbcdc_Para();
//    AX_Mailbox_Init();
//  ax_dsp_log_config(1);
    ax_dsp_memlog_config(1);
    ax_dsp_base_ddr_config();
    AX_VDSP_BspTmInit();
    AX_Mailbox_Init();
}
int main()
{
    AX_VDSP_BspInit();
    AX_SET_LOG_LEVEL(4);
    AX_VDSP_LOG_INFO("dsp algo code begin at %d!\r\n", _procid);
    AX_VDSP_LOG_INFO("_memmap_seg_dram0_0_end:%x, _memmap_seg_dram0_0_max:%x\n", &_memmap_seg_dram0_0_end,
                     &_memmap_seg_dram0_0_max);
    AX_VDSP_LOG_INFO("_memmap_seg_dram1_0_end:%x, _memmap_seg_dram1_0_max:%x\n", &_memmap_seg_dram1_0_end,
                     &_memmap_seg_dram1_0_max);
    AX_Mailbox_Start();

    AX_DSP_Algo(0, 0);
    AX_VDSP_LOG_ERROR("main thread code error!\r\n");
    return 0;      // should never get here
}
