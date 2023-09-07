/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/


#include "sample_cmd_parse.h"

#include <assert.h>
#include <string.h>

#include "sample_venc_log.h"


AX_S32 SampleGetNext(AX_S32 argc, AX_CHAR **argv, SAMPLE_PARAMETER_T *parameter, AX_CHAR **p)
{
    if ((parameter->cnt >= argc) || (parameter->cnt < 0))
        return -1;

    *p = argv[parameter->cnt];
    parameter->cnt++;

    return 0;
}

AX_S32 SampleParse(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *parameter, AX_CHAR **p,
                   AX_U32 lenght)
{
    AX_CHAR *arg;
    AX_U32 arg_len = 0;
    AX_S32 ret = 0;

    parameter->short_opt = option->short_opt;
    parameter->longOpt = option->long_opt;
    arg = *p + lenght;

    /* Argument and option are together */
    arg_len = strlen(arg);
    if (arg_len != 0) {
        /* There should be no argument */
        if (option->enable == 0)
            return -1;

        /* Remove = */
        if (strncmp("=", arg, 1) == 0)
            arg++;

        parameter->enable = 1;
        parameter->argument = arg;
        return 0;
    }

    /* Argument and option are separately */
    ret = SampleGetNext(argc, argv, parameter, p);
    if (ret) {
        /* There is no more parameters */
        if (option->enable == 1)
            return -1;

        return 0;
    }

    /** Parameter is missing if next start with "-" but next time this
     * option is OK so we must fix parameter->cnt */
    ret = strncmp("-", *p, 1);
    if (ret == 0) {
        parameter->cnt--;
        if (option->enable == 1)
            return -1;

        return 0;
    }

    /* There should be no argument */
    if (option->enable == 0) {
        SAMPLE_LOG(" *p:%s", *p);
        return -1;
    }

    parameter->enable = 1;
    parameter->argument = *p;

    return 0;
}

AX_S32 SampleShortOption(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *parameter,
                         AX_CHAR **p)
{
    AX_S32 i = 0;
    AX_CHAR short_opt;

    if (strncmp("-", *p, 1) != 0)
        return 1;

    short_opt = *(*p + 1);
    parameter->short_opt = short_opt;

    while (option[i].long_opt != NULL) {
        if (option[i].short_opt == short_opt)
            goto MATCH;

        i++;
    }

    return 1;

MATCH:
    if (SampleParse(argc, argv, &option[i], parameter, p, 2) != 0)
        return -2;

    return 0;
}

AX_S32 SampleLongOption(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *parameter,
                        AX_CHAR **p)
{
    AX_S32 i = 0;
    AX_U32 lenght;

    if (strncmp("--", *p, 2) != 0)
        return 1;

    while (option[i].long_opt != NULL) {
        lenght = strlen(option[i].long_opt);
        if (strncmp(option[i].long_opt, *p + 2, lenght) == 0)
            goto MATCH;

        i++;
    }

    return 1;

MATCH:

    lenght += 2; /* Because option start -- */
    if (SampleParse(argc, argv, &option[i], parameter, p, lenght) != 0) {
        return -2;
    }

    return 0;
}

AX_S32 SampleGetOption(AX_S32 argc, AX_CHAR **argv, SAMPLE_OPTION_T *option, SAMPLE_PARAMETER_T *parameter)
{
    AX_CHAR *p = NULL;
    AX_S32 ret;

    parameter->argument = "?";
    parameter->short_opt = '?';
    parameter->enable = 0;

    if (SampleGetNext(argc, argv, parameter, &p))
        return -1;

    ret = SampleLongOption(argc, argv, option, parameter, &p);
    if (ret != 1)
        return ret;

    ret = SampleShortOption(argc, argv, option, parameter, &p);
    if (ret != 1)
        return ret;

    /* This is unknow option but option anyway so argument must return */
    parameter->argument = p;

    return 1;
}


AX_S32 SampleParseDelim(AX_CHAR *optArg, AX_CHAR delim)
{
    AX_S32 i;

    for (i = 0; i < (AX_S32)strlen(optArg); i++)
        if (optArg[i] == delim) {
            optArg[i] = 0;
            return i;
        }

    return -1;
}
