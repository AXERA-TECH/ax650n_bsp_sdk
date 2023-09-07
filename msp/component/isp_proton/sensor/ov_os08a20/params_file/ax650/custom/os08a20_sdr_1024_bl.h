#ifndef __OV08A20_SDR_H__
#define __OV08A20_SDR_H__

/********************************************************************
* Store the default parameters of the sdr mode
* warn: user need to add 'static' when defining global variables.
        Limit the scope of the variable to this sensor
*********************************************************************/

#include "ax_isp_iq_api.h"
#include "ax_isp_api.h"

static AX_ISP_VERSION_T ax_isp_version_param_sdr = {

    /* nIspMajor */
    3,
    /* nIspMinor1 */
    8,
    /* nIspMinor2 */
    1,
    /* szBuildTime */
    "",

    /* szIspVersion */
    "AX650_ISP_V3.8.1",
};


static AX_ISP_IQ_BLC_PARAM_T blc_param_sdr = {
    /* nBlcEnable */
    1,
    /* nAutoMode */
    0,
    /* tManualParam */
    {
        /* nSblRValue[4] */
        {65536, 65536, 65536, 65536, /*0 - 3*/},
        /* nSblGrValue[4] */
        {65536, 65536, 65536, 65536, /*0 - 3*/},
        /* nSblGbValue[4] */
        {65536, 65536, 65536, 65536, /*0 - 3*/},
        /* nSblBValue[4] */
        {65536, 65536, 65536, 65536, /*0 - 3*/},
    },
    /* tAutoParam */
    {
        /* tHcgAutoTable */
        {
            /* nGainGrpNum */
            16,
            /* nExposeTimeGrpNum */
            6,
            /* nGain[16] */
            {1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192, 9216, 10240, 11264, 12288, 13312, 14336, 15360, 15872, /*0 - 15*/},
            /* nExposeTime[10] */
            {1000, 5000, 10000, 20000, 30000, 32939, /*0 - 5*/},
            /* nAutoSblRValue[16][10] */
            {
                {65576, 65558, 65581, 65566, 65568, 65562, /*0 - 5*/},
                {65573, 65586, 65565, 65562, 65579, 65568, /*0 - 5*/},
                {65567, 65571, 65556, 65563, 65557, 65567, /*0 - 5*/},
                {65598, 65551, 65566, 65567, 65571, 65558, /*0 - 5*/},
                {65580, 65556, 65559, 65557, 65551, 65571, /*0 - 5*/},
                {65567, 65565, 65552, 65569, 65579, 65547, /*0 - 5*/},
                {65592, 65544, 65543, 65558, 65580, 65580, /*0 - 5*/},
                {65575, 65582, 65545, 65550, 65555, 65567, /*0 - 5*/},
                {65569, 65558, 65553, 65553, 65537, 65572, /*0 - 5*/},
                {65570, 65567, 65575, 65531, 65561, 65555, /*0 - 5*/},
                {65552, 65575, 65555, 65524, 65567, 65575, /*0 - 5*/},
                {65558, 65548, 65550, 65576, 65552, 65594, /*0 - 5*/},
                {65542, 65533, 65571, 65553, 65552, 65574, /*0 - 5*/},
                {65566, 65563, 65569, 65547, 65598, 65566, /*0 - 5*/},
                {65550, 65525, 65576, 65518, 64350, 65582, /*0 - 5*/},
                {65540, 65548, 65555, 65551, 64350, 64350, /*0 - 5*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {65563, 65539, 65568, 65565, 65568, 65562, /*0 - 5*/},
                {65560, 65593, 65564, 65574, 65584, 65593, /*0 - 5*/},
                {65560, 65583, 65562, 65574, 65581, 65579, /*0 - 5*/},
                {65572, 65569, 65577, 65571, 65576, 65569, /*0 - 5*/},
                {65572, 65569, 65589, 65594, 65561, 65588, /*0 - 5*/},
                {65592, 65601, 65577, 65587, 65603, 65590, /*0 - 5*/},
                {65616, 65595, 65600, 65582, 65603, 65605, /*0 - 5*/},
                {65605, 65598, 65592, 65593, 65586, 65583, /*0 - 5*/},
                {65605, 65614, 65594, 65602, 65600, 65606, /*0 - 5*/},
                {65607, 65583, 65596, 65600, 65590, 65608, /*0 - 5*/},
                {65601, 65603, 65602, 65580, 65609, 65610, /*0 - 5*/},
                {65607, 65577, 65590, 65575, 65611, 65628, /*0 - 5*/},
                {65597, 65573, 65611, 65595, 65599, 65649, /*0 - 5*/},
                {65583, 65579, 65620, 65622, 65666, 65651, /*0 - 5*/},
                {65598, 65586, 65624, 65622, 64411, 65627, /*0 - 5*/},
                {65603, 65603, 65610, 65605, 64411, 64411, /*0 - 5*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {65547, 65542, 65572, 65570, 65579, 65586, /*0 - 5*/},
                {65572, 65568, 65565, 65571, 65574, 65581, /*0 - 5*/},
                {65555, 65568, 65587, 65575, 65566, 65583, /*0 - 5*/},
                {65566, 65588, 65591, 65570, 65563, 65583, /*0 - 5*/},
                {65568, 65573, 65575, 65576, 65565, 65591, /*0 - 5*/},
                {65569, 65579, 65599, 65573, 65578, 65567, /*0 - 5*/},
                {65571, 65574, 65584, 65581, 65567, 65615, /*0 - 5*/},
                {65598, 65601, 65588, 65575, 65563, 65604, /*0 - 5*/},
                {65595, 65603, 65580, 65592, 65601, 65620, /*0 - 5*/},
                {65580, 65573, 65577, 65569, 65554, 65600, /*0 - 5*/},
                {65597, 65628, 65591, 65599, 65599, 65572, /*0 - 5*/},
                {65581, 65576, 65596, 65576, 65616, 65599, /*0 - 5*/},
                {65577, 65592, 65593, 65581, 65588, 65611, /*0 - 5*/},
                {65600, 65596, 65603, 65588, 65577, 65615, /*0 - 5*/},
                {65609, 65589, 65593, 65592, 64360, 65612, /*0 - 5*/},
                {65602, 65618, 65592, 65614, 64360, 64360, /*0 - 5*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {65554, 65556, 65560, 65564, 65580, 65580, /*0 - 5*/},
                {65580, 65575, 65566, 65578, 65586, 65581, /*0 - 5*/},
                {65563, 65575, 65581, 65574, 65578, 65583, /*0 - 5*/},
                {65572, 65583, 65585, 65576, 65557, 65596, /*0 - 5*/},
                {65589, 65573, 65562, 65577, 65579, 65597, /*0 - 5*/},
                {65595, 65585, 65599, 65586, 65584, 65580, /*0 - 5*/},
                {65591, 65562, 65603, 65581, 65594, 65627, /*0 - 5*/},
                {65597, 65596, 65595, 65581, 65566, 65624, /*0 - 5*/},
                {65596, 65603, 65574, 65573, 65594, 65603, /*0 - 5*/},
                {65593, 65592, 65596, 65564, 65579, 65595, /*0 - 5*/},
                {65583, 65603, 65629, 65613, 65608, 65593, /*0 - 5*/},
                {65595, 65593, 65610, 65579, 65617, 65573, /*0 - 5*/},
                {65583, 65604, 65639, 65590, 65619, 65608, /*0 - 5*/},
                {65595, 65613, 65636, 65588, 65615, 65650, /*0 - 5*/},
                {65628, 65615, 65588, 65623, 64393, 65638, /*0 - 5*/},
                {65617, 65612, 65586, 65594, 64393, 64393, /*0 - 5*/},
            },
        },
        /* tLcgAutoTable */
        {
            /* nGainGrpNum */
            16,
            /* nExposeTimeGrpNum */
            6,
            /* nGain[16] */
            {1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192, 9216, 10240, 11264, 12288, 13312, 14336, 15360, 15872, /*0 - 15*/},
            /* nExposeTime[10] */
            {1000, 5000, 10000, 20000, 30000, 32939, /*0 - 5*/},
            /* nAutoSblRValue[16][10] */
            {
                {65561, 65557, 65569, 65565, 65567, 65567, /*0 - 5*/},
                {65567, 65570, 65564, 65583, 65583, 65583, /*0 - 5*/},
                {65586, 65574, 65591, 65564, 65585, 65554, /*0 - 5*/},
                {65564, 65582, 65555, 65576, 65592, 65571, /*0 - 5*/},
                {65583, 65590, 65583, 65577, 65566, 65601, /*0 - 5*/},
                {65588, 65588, 65571, 65577, 65594, 65577, /*0 - 5*/},
                {65582, 65593, 65529, 65593, 65550, 65568, /*0 - 5*/},
                {65582, 65613, 65572, 65580, 65588, 65608, /*0 - 5*/},
                {65600, 65601, 65581, 65591, 65606, 65630, /*0 - 5*/},
                {65617, 65591, 65576, 65597, 65580, 65613, /*0 - 5*/},
                {65565, 65586, 65615, 65601, 65598, 65616, /*0 - 5*/},
                {65572, 65585, 65581, 65567, 65558, 65612, /*0 - 5*/},
                {65613, 65569, 65566, 65598, 65612, 65616, /*0 - 5*/},
                {65585, 65572, 65583, 65577, 65589, 65567, /*0 - 5*/},
                {65589, 65596, 65598, 65567, 65576, 65584, /*0 - 5*/},
                {65632, 65596, 65581, 65603, 65590, 65585, /*0 - 5*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {65562, 65556, 65555, 65558, 65567, 65566, /*0 - 5*/},
                {65573, 65576, 65551, 65589, 65584, 65583, /*0 - 5*/},
                {65587, 65562, 65586, 65559, 65579, 65561, /*0 - 5*/},
                {65564, 65562, 65568, 65569, 65598, 65577, /*0 - 5*/},
                {65557, 65590, 65576, 65571, 65572, 65600, /*0 - 5*/},
                {65587, 65589, 65571, 65576, 65600, 65583, /*0 - 5*/},
                {65588, 65592, 65541, 65593, 65543, 65581, /*0 - 5*/},
                {65569, 65606, 65596, 65593, 65579, 65613, /*0 - 5*/},
                {65605, 65595, 65581, 65597, 65604, 65628, /*0 - 5*/},
                {65617, 65603, 65575, 65596, 65599, 65624, /*0 - 5*/},
                {65558, 65586, 65621, 65588, 65603, 65621, /*0 - 5*/},
                {65574, 65578, 65587, 65572, 65576, 65637, /*0 - 5*/},
                {65612, 65599, 65580, 65602, 65617, 65634, /*0 - 5*/},
                {65578, 65577, 65601, 65602, 65608, 65598, /*0 - 5*/},
                {65583, 65622, 65617, 65581, 65568, 65590, /*0 - 5*/},
                {65617, 65584, 65580, 65615, 65583, 65577, /*0 - 5*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {65554, 65549, 65553, 65549, 65569, 65564, /*0 - 5*/},
                {65562, 65565, 65567, 65574, 65584, 65586, /*0 - 5*/},
                {65556, 65567, 65574, 65564, 65591, 65554, /*0 - 5*/},
                {65574, 65568, 65572, 65613, 65572, 65592, /*0 - 5*/},
                {65585, 65581, 65617, 65580, 65584, 65570, /*0 - 5*/},
                {65543, 65588, 65589, 65563, 65567, 65588, /*0 - 5*/},
                {65586, 65599, 65579, 65555, 65601, 65569, /*0 - 5*/},
                {65589, 65573, 65594, 65586, 65594, 65628, /*0 - 5*/},
                {65553, 65582, 65573, 65585, 65586, 65568, /*0 - 5*/},
                {65577, 65582, 65594, 65594, 65624, 65590, /*0 - 5*/},
                {65580, 65568, 65590, 65620, 65596, 65592, /*0 - 5*/},
                {65583, 65591, 65591, 65600, 65594, 65592, /*0 - 5*/},
                {65555, 65593, 65620, 65593, 65596, 65585, /*0 - 5*/},
                {65597, 65596, 65588, 65642, 65586, 65601, /*0 - 5*/},
                {65588, 65605, 65560, 65611, 65595, 65609, /*0 - 5*/},
                {65559, 65593, 65613, 65588, 65610, 65637, /*0 - 5*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {65560, 65563, 65554, 65556, 65577, 65565, /*0 - 5*/},
                {65569, 65565, 65574, 65588, 65578, 65581, /*0 - 5*/},
                {65557, 65581, 65574, 65551, 65585, 65548, /*0 - 5*/},
                {65587, 65574, 65579, 65608, 65565, 65579, /*0 - 5*/},
                {65592, 65587, 65585, 65573, 65577, 65562, /*0 - 5*/},
                {65556, 65607, 65595, 65557, 65572, 65588, /*0 - 5*/},
                {65593, 65605, 65580, 65555, 65588, 65568, /*0 - 5*/},
                {65596, 65561, 65602, 65593, 65588, 65627, /*0 - 5*/},
                {65559, 65583, 65562, 65566, 65598, 65588, /*0 - 5*/},
                {65591, 65577, 65588, 65594, 65604, 65590, /*0 - 5*/},
                {65586, 65582, 65591, 65619, 65602, 65592, /*0 - 5*/},
                {65571, 65592, 65579, 65606, 65619, 65592, /*0 - 5*/},
                {65569, 65594, 65588, 65604, 65596, 65598, /*0 - 5*/},
                {65598, 65609, 65593, 65635, 65593, 65590, /*0 - 5*/},
                {65592, 65599, 65565, 65616, 65602, 65602, /*0 - 5*/},
                {65577, 65586, 65619, 65580, 65591, 65631, /*0 - 5*/},
            },
        },
    },
};

#endif
