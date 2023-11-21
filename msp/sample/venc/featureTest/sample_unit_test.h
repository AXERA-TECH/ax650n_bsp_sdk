/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_UNIT_TEST_H__
#define __SAMPLE_UNIT_TEST_H__

#include "sample_global.h"

/* #define SUPPORT_DYN_RC_MODE */

typedef AX_S32 (*UTestCase)(SAMPLE_VENC_CMD_PARA_T *pst);

typedef enum
{
    UT_CASE_NORMAL = 0,
    UT_CASE_BIT_RATE,
    UT_CASE_RESET_CHN,
    UT_CASE_VENC_ROI = 3,
    UT_CASE_FRAME_RATE,
    UT_CASE_CHN_ATTR,
    UT_CASE_RC_MODE = 6,
    UT_CASE_VUI,
    UT_CASE_JPEG_ENCODE_ONCE,
    UT_CASE_JPEG_PARAM = 9,
    UT_CASE_VIR_INTRA_INTERVAL,
    UT_CASE_INTRA_REFRESH, /* GDR */
    UT_CASE_RESOLUTION = 12,
    UT_CASE_REQUEST_IDR,
    UT_CASE_SELECT_CHN,
    UT_CASE_SET_USR_DATA = 15,
    UT_CASE_RATE_JAM,
    UT_CASE_SUPER_FRAME,
    UT_CASE_SLICE_SPLIT = 18,
    UT_CASE_CREATE_DESTROY,

    SAMPLE_TESTCASE_BUTT = SAMPLE_MAX_TESTCASE_NUM,

} SAMPLE_TESTCASE_ID_E;

typedef struct axSample_Test_Case_T
{
    SAMPLE_TESTCASE_ID_E enTestId;
    UTestCase testCase;
    TestFunction testAttrFunc;
    SAMPLE_VENC_CMD_PARA_T *pCml;
    AX_CHAR testCaseName[MAX_TEST_CASE_NAME_SIZE];
} Sample_Test_Case_T;


AX_VOID SampleTestCaseStart(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_BOOL SampleInvalidEnType(AX_S32 ut, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode);
/* Unit Test case function */
AX_S32 UTestNormal(SAMPLE_VENC_CMD_PARA_T *pHandle);
AX_S32 UTestDynamicAttr(SAMPLE_VENC_CMD_PARA_T *pHandle);

/* Test reset channel */
AX_S32 UTestResetChn(SAMPLE_VENC_CMD_PARA_T *pHandle);
AX_S32 UTestVencRoi(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_S32 UTestJpegStartEncOnce(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_S32 UTestSetChnAttr(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_S32 UTestDynamicJpeg(SAMPLE_VENC_CMD_PARA_T *pCml);
/* select channel to get stream */
AX_S32 UTestSelectChn(SAMPLE_VENC_CMD_PARA_T *pCml);
/* create and destroy channel multi times */
AX_S32 UTestCreateDestroy(SAMPLE_VENC_CMD_PARA_T *pCml);


#endif
