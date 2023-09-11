/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_CMD_PARSE_H__
#define __SAMPLE_CMD_PARSE_H__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ax_global_type.h"


#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

typedef struct axSAMPLE_OPTION {
    AX_CHAR *long_opt;
    AX_CHAR short_opt;
    AX_S32 enable;
} SAMPLE_OPTION_T;

typedef struct axSAMPLE_PARAMETER {
    AX_S32 cnt;
    AX_CHAR *argument;
    AX_CHAR short_opt;
    AX_CHAR *longOpt;
    AX_S32 enable;
} SAMPLE_PARAMETER_T;

AX_S32 SampleGetNext(AX_S32 argc, AX_CHAR **argv, SAMPLE_PARAMETER_T *parameter, AX_CHAR **p);
AX_S32 SampleParse(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *para, AX_CHAR **p,
                   AX_U32 lenght);
AX_S32 SampleShortOption(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *para, AX_CHAR **p);
AX_S32 SampleLongOption(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *para, AX_CHAR **p);
AX_S32 SampleGetOption(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *parameter);
AX_S32 SampleParseDelim(AX_CHAR *optArg, AX_CHAR delim);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif