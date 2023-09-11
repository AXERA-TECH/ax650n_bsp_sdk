/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "ax_sys_api.h"
#include "ax_base_type.h"

#define ADC_IOC_MAGIC 'A'
#define SET_ADC_CHAN  _IOW(ADC_IOC_MAGIC, 0x80, AX_U32)
#define GET_ADC_VALUE _IOR(ADC_IOC_MAGIC, 0x81, struct adc_value)

/*
 * This expects the new RTC class driver framework, working with
 * clocks that will often not be clones of what the PC-AT had.
 * Use the command line to specify another RTC if you need one.
 */
static const char default_adc[] = "/dev/axera_adc";

struct adc_value {
    AX_U32 chan0_val;
    AX_U32 chan1_val;
    AX_U32 chan2_val;
    AX_U32 chan3_val;
    AX_U32 chan4_val;
};

int main(int argc, char **argv)
{
    int fd, retval;
    AX_U32 enable_adc_chan;
    struct adc_value get_adc_value;
    const char *adc = default_adc;
    if (argc == 2) {
        fd = open("/dev/axera_adc", O_RDWR);
        if (fd == -1) {
            perror(adc);
            fprintf(stderr,
                "\n\t\t open dev/axera_adc failed. errno :%d\n\n",
                errno);
            exit(errno);
        }
        enable_adc_chan = strtol(argv[1], &argv[1], 16);
        /* set enable the ADC chan */
        retval = ioctl(fd, SET_ADC_CHAN, &enable_adc_chan);
        if (retval == -1) {
            perror("SET_ADC_CHAN ioctl");
        }
        /* Read the ADC date */
        retval = ioctl(fd, GET_ADC_VALUE, &get_adc_value);
        if (retval == -1) {
            perror("GET_ADC_VALUE ioctl");
        }
        if (get_adc_value.chan0_val == 0 || get_adc_value.chan1_val == 0 ||get_adc_value.chan2_val == 0
                || get_adc_value.chan3_val == 0 || get_adc_value.chan4_val == 0 ||
                get_adc_value.chan0_val == 0xff || get_adc_value.chan1_val == 0xff || get_adc_value.chan2_val == 0xff
                || get_adc_value.chan3_val == 0xff || get_adc_value.chan4_val == 0xff)
            fprintf(stderr, "\n\t ADC Driver Test Example Failed.\n");
        else
            fprintf(stderr, "\n\t ADC Driver Test Example Successed.\n");
        fprintf(stderr,
            "\nCurrent ADC date is chan0:0x%x, chan1:0x%x, chan2:0x%x, chan3:0x%x, chan4:0x%x\n",
            get_adc_value.chan0_val, get_adc_value.chan1_val,
            get_adc_value.chan2_val, get_adc_value.chan3_val,
            get_adc_value.chan4_val);
        /* Set the alarm to 5 sec in the future, and check for rollover */
        close(fd);
    } else {
        fprintf(stderr,
            "please input enable channel number\n eg: ./sample_adc 0x1f\n");
    }
    return 0;
}