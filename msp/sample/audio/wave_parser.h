/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _WAVE_PARSER_H__
#define _WAVE_PARSER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ParseWaveHeader(char *filename, FILE *file, uint16_t *num_channels, uint32_t *sample_rate, uint16_t *bits_per_sample);
void LeaveWaveHeader(FILE *file);
void WriteWaveHeader(FILE *file, unsigned int channels, unsigned int rate, uint16_t bits_per_sample, unsigned int frames);

#ifdef __cplusplus
}
#endif

#endif /* _WAVE_PARSER_H__ */
