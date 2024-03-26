/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_UTIL_07C3D107_75E3_491C_AF1F_6E36DEDC43F8_H_
#define _SAMPLE_UTIL_07C3D107_75E3_491C_AF1F_6E36DEDC43F8_H_

#include <signal.h>
#include "ax_base_type.h"
#include "ax_dsp_cv_api.h"

//#define VDSP_DEBUG
#ifdef VDSP_DEBUG
//#define DATA_CHECK_ENABLE
#define vdsp_debug(str, arg...) printf("[DBG]" str, ##arg)
#else
#define vdsp_debug(str, arg...)
#endif
#define vdsp_err(str, arg...)   printf("[ERR]" str, ##arg)

#define DEFAULT_DELIM   "@"
#define SAMPLE_NAME     "sample_dsp_algo"
#define SAMPLE_PHY_MEM_ALIGN_SIZE   (128)
#define ARRAY_SIZE(array)       sizeof(array)/sizeof(array[0])
#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align)-1))

typedef struct {
    AX_U64 u64PhyAddr;
    AX_U64 u64VirAddr;
    AX_U32 u32Size;
    AX_MEM_INFO_T BufAddr[3];	//0: Y, 1: UV
    int height;
    int width;
    int stride;
} AX_DSP_IMAGE_T;

AX_BOOL LoadImage(const AX_CHAR *pszImge, AX_U64 *pPhyAddr, AX_VOID **ppVirAddr, AX_U32 *pImgSize);
AX_S32 DSP_LoadPic(const AX_CHAR *pszBinFileName, AX_VOID *vaddr);
AX_S32 DSP_SavePic(const char *pszBinFileName, AX_VOID *vaddr, AX_U32 size);

#endif
