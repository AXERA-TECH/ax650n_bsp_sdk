/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_scd.h"
#include <string.h>
#include "sample_util.h"
#include "trace.h"

AX_S32 SAMPLE_IVES_SCD_ENTRY(const AX_CHAR *pRefImg, const AX_CHAR *pCurImg, AX_U32 nWidth, AX_U32 nHeight) {
    AX_S32 ret = -1;
    AX_U32 i;
    AX_U8 result;
    AX_U64 u64Tick1, u64Tick2;
    const AX_CHAR *IMAGE_FILE_PATH[2] = {pRefImg, pCurImg};
    AX_IVES_IMAGE_T *pstImgs[2] = {NULL};
    AX_SCD_CHN_ATTR_T stChnAttr = {.chn = 1, .stArea = {0, 0, nWidth, nHeight}, .u8Thrd = 60, .u8Confidence = 60};

    for (i = 0; i < 2; ++i) {
        pstImgs[i] = SAMPLE_LOAD_IMAGE(IMAGE_FILE_PATH[i], nWidth, nHeight, AX_FORMAT_YUV420_SEMIPLANAR);
        if (!pstImgs[i]) {
            goto EXIT0;
        }

        pstImgs[i]->u64SeqNum = i + 1;
    }

    ret = AX_IVES_SCD_Init();
    if (0 != ret) {
        ALOGE("AX_IVES_SCD_Init() fail, ret = 0x%x", ret);
        goto EXIT0;
    }

    ret = AX_IVES_SCD_CreateChn(stChnAttr.chn, &stChnAttr);
    if (0 != ret) {
        ALOGE("AX_IVES_SCD_CreateChn(Chn: %d) fail, ret = 0x%x", stChnAttr.chn, ret);
        goto EXIT1;
    }

    for (i = 0; i < 2; ++i) {
        u64Tick1 = SAMPLE_GET_TICK_COUNT();
        ret = AX_IVES_SCD_Process(stChnAttr.chn, pstImgs[i], &result);
        u64Tick2 = SAMPLE_GET_TICK_COUNT();
        printf("SCD (Chn %d, img: %lld, elapsed time: %lld ms\n", stChnAttr.chn, pstImgs[i]->u64SeqNum,
               u64Tick2 - u64Tick1);

        if (0 != ret) {
            ALOGE("AX_IVES_SCD_Process(Chn: %d, img: %lld, ref) fail, ret = 0x%x", stChnAttr.chn, pstImgs[i]->u64SeqNum,
                  ret);
            goto EXIT2;
        }

        if (i > 0) {
            printf("Scene change detection result: %s\n", (0 == result) ? "unchanged" : "changed");
        }
    }

    ret = AX_IVES_SCD_DestoryChn(stChnAttr.chn);
    if (0 != ret) {
        ALOGE("AX_IVES_SCD_DestoryChn(Chn: %d) fail, ret = 0x%x", stChnAttr.chn, ret);
    }

#ifndef UT_AUTO
#define UT_AUTO
    if (result != 0) {
        goto EXIT0;
    }
#endif

    ALOGI("SCD sample success");
    goto EXIT1;

EXIT2:
    AX_IVES_SCD_DestoryChn(stChnAttr.chn);

EXIT1:
    AX_IVES_SCD_DeInit();

EXIT0:
    for (i = 0; i < 2; ++i) {
        if (pstImgs[i]) {
            SAMPLE_FREE_IMAGE(pstImgs[i]);
            pstImgs[i] = NULL;
        }
    }

    return ret;
}
