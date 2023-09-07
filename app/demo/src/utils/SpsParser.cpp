/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "SpsParser.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "h264.hpp"
#include "hevc.hpp"

/* H264: https://wiki.aixin-chip.com/pages/viewpage.action?pageId=40873951 */
/* HEVC: https://wiki.aixin-chip.com/pages/viewpage.action?pageId=61593700 */

#define EXTENDED_SAR 255
#define FFMIN(a, b) ((a) > (b) ? (b) : (a))
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define HEVC_MAX_SHORT_TERM_REF_PIC_SETS 64
#define HEVC_MAX_DPB_SIZE 16
#define HEVC_MAX_REFS HEVC_MAX_DPB_SIZE

#define SPECIAL_PROFILE_IDC(idc)                                                                                                \
    (idc == 100 || idc == 110 || idc == 122 || idc == 244 || idc == 44 || idc == 83 || idc == 86 || idc == 118 || idc == 128 || \
     idc == 138 || idc == 139 || idc == 134 || idc == 135)

static const AX_U8 default_scaling_list_intra[] = {16, 16, 16, 16, 17, 18, 21, 24, 16, 16, 16, 16, 17, 19, 22, 25, 16, 16, 17, 18, 20, 22,
                                                   25, 29, 16, 16, 18, 21, 24, 27, 31, 36, 17, 17, 20, 24, 30, 35, 41, 47, 18, 19, 22, 27,
                                                   35, 44, 54, 65, 21, 22, 25, 31, 41, 54, 70, 88, 24, 25, 29, 36, 47, 65, 88, 115};

static const AX_U8 default_scaling_list_inter[] = {16, 16, 16, 16, 17, 18, 20, 24, 16, 16, 16, 17, 18, 20, 24, 25, 16, 16, 17, 18, 20, 24,
                                                   25, 28, 16, 17, 18, 20, 24, 25, 28, 33, 17, 18, 20, 24, 25, 28, 33, 41, 18, 20, 24, 25,
                                                   28, 33, 41, 54, 20, 24, 25, 28, 33, 41, 54, 71, 24, 25, 28, 33, 41, 54, 71, 91};

static const AX_U8 ff_hevc_diag_scan4x4_x[16] = {
    0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 1, 2, 3, 2, 3, 3,
};

static const AX_U8 ff_hevc_diag_scan4x4_y[16] = {
    0, 1, 0, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 2, 3,
};

static const AX_U8 ff_hevc_diag_scan8x8_x[64] = {
    0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3,
    4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 6, 7, 3, 4, 5, 6, 7, 4, 5, 6, 7, 5, 6, 7, 6, 7, 7,
};

static const AX_U8 ff_hevc_diag_scan8x8_y[64] = {
    0, 1, 0, 2, 1, 0, 3, 2, 1, 0, 4, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4,
    3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 7, 6, 5, 4, 3, 2, 7, 6, 5, 4, 3, 7, 6, 5, 4, 7, 6, 5, 7, 6, 7,
};

typedef struct {
    const AX_U8 *data;
    AX_U32 index;
    AX_U32 size_in_bits;
} SPS_BIT_STREAM_T;

typedef struct ScalingList {
    /* This is a little wasteful, since sizeID 0 only needs 8 coeffs,
     * and size ID 3 only has 2 arrays, not 6. */
    AX_U8 sl[4][6][64];
    AX_U8 sl_dc[2][6];
} ScalingList;

typedef struct ShortTermRPS {
    unsigned int num_negative_pics;
    int num_delta_pocs;
    int rps_idx_num_delta_pocs;
    AX_S32 delta_poc[32];
    AX_U8 used[32];
} ShortTermRPS;

typedef struct {
    AX_S32 profile_idc;
    AX_S32 constraint_set0_flag;
    AX_S32 constraint_set1_flag;
    AX_S32 constraint_set2_flag;
    AX_S32 constraint_set3_flag;
    AX_S32 constraint_set4_flag;
    AX_S32 constraint_set5_flag;
    AX_S32 level_idc;
    AX_S32 chroma_format_idc;
    AX_S32 seq_parameter_set_id;
    AX_S32 residual_colour_transform_flag;
    AX_S32 bit_depth_luma_minus8;
    AX_S32 bit_depth_chroma_minus8;
    AX_S32 qpprime_y_zero_transform_bypass_flag;
    AX_S32 seq_scaling_matrix_present_flag;
    AX_S32 log2_max_frame_num_minus4;
    AX_S32 log2_max_pic_order_cnt_lsb_minus4;
    AX_S32 pic_order_cnt_type;
    AX_S32 delta_pic_order_always_zero_flag;
    AX_S32 offset_for_non_ref_pic;
    AX_S32 offset_for_top_to_bottom_field;
    AX_S32 num_ref_frames_in_pic_order_cnt_cycle;
    AX_S32 num_ref_frames;
    AX_S32 gaps_in_frame_num_value_allowed_flag;
    AX_S32 pic_width_in_mbs_minus1;
    AX_S32 pic_height_in_map_units_minus1;
    AX_S32 frame_mbs_only_flag;
    AX_S32 mb_adaptive_frame_field_flag;
    AX_S32 direct_8x8_inference_flag;
    AX_S32 frame_cropping_flag;
    AX_S32 frame_crop_left_offset;
    AX_S32 frame_crop_right_offset;
    AX_S32 frame_crop_top_offset;
    AX_S32 frame_crop_bottom_offset;
    AX_S32 vui_parameters_present_flag;
    AX_S32 aspect_ratio_info_present_flag;
    AX_S32 aspect_ratio_idc;
    AX_S32 sar_width;
    AX_S32 sar_height;
    AX_S32 overscan_info_present_flag;
    AX_S32 overscan_appropriate_flag;
    AX_S32 video_signal_type_present_flag;
    AX_S32 video_format;
    AX_S32 video_full_range_flag;
    AX_S32 colour_description_present_flag;
    AX_S32 colour_primaries;
    AX_S32 transfer_characteristics;
    AX_S32 matrix_coefficients;
    AX_S32 chroma_loc_info_present_flag;
    AX_S32 chroma_sample_loc_type_top_field;
    AX_S32 chroma_sample_loc_type_bottom_field;
    AX_S32 timing_info_present_flag;
    AX_S32 num_units_in_tick;
    AX_S32 time_scale;
    AX_S32 fixed_frame_rate_flag;

} H264_SPS_T;

typedef struct {
    AX_S32 sps_video_parameter_set_id;
    AX_S32 sps_max_sub_layers_minus1;
    AX_S32 sps_temporal_id_nesting_flag;
    AX_S32 general_profile_idc;
    AX_S32 general_level_idc;
    AX_S32 sps_seq_parameter_set_id;
    AX_S32 chroma_format_idc;
    AX_S32 separate_colour_plane_flag;
    AX_S32 pic_width_in_luma_samples;
    AX_S32 pic_height_in_luma_samples;
    AX_S32 conformance_window_flag;
    AX_S32 conf_win_left_offset;
    AX_S32 conf_win_right_offset;
    AX_S32 conf_win_top_offset;
    AX_S32 conf_win_bottom_offset;
    AX_S32 bit_depth_luma_minus8;
    AX_S32 bit_depth_chroma_minus8;
    AX_S32 log2_max_pic_order_cnt_lsb_minus4;
    AX_S32 sps_sub_layer_ordering_info_present_flag;
    AX_S32 log2_min_luma_coding_block_size_minus3;
    AX_S32 log2_diff_max_min_luma_coding_block_size;
    AX_S32 log2_min_luma_transform_block_size_minus2;
    AX_S32 log2_diff_max_min_luma_transform_block_size;
    AX_S32 max_transform_hierarchy_depth_inter;
    AX_S32 max_transform_hierarchy_depth_intra;
    AX_S32 scaling_list_enabled_flag;
    AX_S32 amp_enabled_flag;
    AX_S32 sample_adaptive_offset_enabled_flag;
    AX_S32 pcm_enabled_flag;
    AX_U32 nb_st_rps;  // num_short_term_ref_pic_sets;
    AX_S32 long_term_ref_pics_present_flag;
    AX_S32 sps_temporal_mvp_enabled_flag;
    AX_S32 strong_intra_smoothing_enabled_flag;
    AX_S32 vui_parameters_present_flag;
    ScalingList scaling_list;
    ShortTermRPS st_rps[HEVC_MAX_SHORT_TERM_REF_PIC_SETS];

} HEVC_SPS_T;

static std::ostream &operator<<(std::ostream &os, H264_SPS_T &sps) {
    os << "chroma_format_idc = " << sps.chroma_format_idc << std::endl;
    os << "profile_idc = " << sps.profile_idc << std::endl;
    os << "constraint_set0_flag = " << sps.constraint_set0_flag << std::endl;
    os << "constraint_set1_flag = " << sps.constraint_set1_flag << std::endl;
    os << "constraint_set2_flag = " << sps.constraint_set2_flag << std::endl;
    os << "constraint_set3_flag = " << sps.constraint_set3_flag << std::endl;
    os << "constraint_set4_flag = " << sps.constraint_set4_flag << std::endl;
    os << "constraint_set5_flag = " << sps.constraint_set5_flag << std::endl;
    os << "level_idc = " << sps.level_idc << std::endl;
    os << "seq_parameter_set_id = " << sps.seq_parameter_set_id << std::endl;
    if (sps.chroma_format_idc == 3) {
        os << "  residual_colour_transform_flag = " << sps.residual_colour_transform_flag << std::endl;
    }
    if (SPECIAL_PROFILE_IDC(sps.profile_idc)) {
        os << "  bit_depth_luma_minus8 = " << sps.bit_depth_luma_minus8 << std::endl;
        os << "  bit_depth_chroma_minus8 = " << sps.bit_depth_chroma_minus8 << std::endl;
        os << "  qpprime_y_zero_transform_bypass_flag = " << sps.qpprime_y_zero_transform_bypass_flag << std::endl;
        os << "  seq_scaling_matrix_present_flag = " << sps.seq_scaling_matrix_present_flag << std::endl;
    }

    os << "log2_max_frame_num_minus4 = " << sps.log2_max_frame_num_minus4 << std::endl;
    os << "pic_order_cnt_type = " << sps.pic_order_cnt_type << std::endl;
    if (sps.pic_order_cnt_type == 0) {
        os << "  log2_max_pic_order_cnt_lsb_minus4 = " << sps.log2_max_pic_order_cnt_lsb_minus4 << std::endl;
    } else if (sps.pic_order_cnt_type == 1) {
        os << "  delta_pic_order_always_zero_flag = " << sps.delta_pic_order_always_zero_flag << std::endl;
        os << "  offset_for_non_ref_pic = " << sps.offset_for_non_ref_pic << std::endl;
        os << "  offset_for_top_to_bottom_field = " << sps.offset_for_top_to_bottom_field << std::endl;
        os << "  num_ref_frames_in_pic_order_cnt_cycle = " << sps.num_ref_frames_in_pic_order_cnt_cycle << std::endl;
    }

    os << "num_ref_frames = " << sps.num_ref_frames << std::endl;
    os << "gaps_in_frame_num_value_allowed_flag = " << sps.gaps_in_frame_num_value_allowed_flag << std::endl;
    os << "pic_width_in_mbs_minus1 = " << sps.pic_width_in_mbs_minus1 << std::endl;
    os << "pic_height_in_map_units_minus1 = " << sps.pic_height_in_map_units_minus1 << std::endl;
    os << "frame_mbs_only_flag = " << sps.frame_mbs_only_flag << std::endl;
    if (!sps.frame_mbs_only_flag) {
        os << "  mb_adaptive_frame_field_flag = " << sps.mb_adaptive_frame_field_flag << std::endl;
    }
    os << "direct_8x8_inference_flag = " << sps.direct_8x8_inference_flag << std::endl;
    os << "frame_cropping_flag = " << sps.frame_cropping_flag << std::endl;
    if (sps.frame_cropping_flag) {
        os << "  frame_crop_left_offset = " << sps.frame_crop_left_offset << std::endl;
        os << "  frame_crop_right_offset = " << sps.frame_crop_right_offset << std::endl;
        os << "  frame_crop_top_offset = " << sps.frame_crop_top_offset << std::endl;
        os << "  frame_crop_bottom_offset = " << sps.frame_crop_bottom_offset << std::endl;
    }
    os << "vui_parameters_present_flag = " << sps.vui_parameters_present_flag << std::endl;
    if (sps.vui_parameters_present_flag) {
        os << "  aspect_ratio_info_present_flag = " << sps.aspect_ratio_info_present_flag << std::endl;
        if (sps.aspect_ratio_info_present_flag) {
            os << "    aspect_ratio_idc = " << sps.aspect_ratio_idc << std::endl;
            if (sps.aspect_ratio_idc == EXTENDED_SAR) {
                os << "       sar_width = " << sps.sar_width << std::endl;
                os << "       sar_height = " << sps.sar_height << std::endl;
            }
        }
        os << "  overscan_info_present_flag = " << sps.overscan_info_present_flag << std::endl;
        if (sps.overscan_info_present_flag) {
            os << "    overscan_appropriate_flag = " << sps.overscan_appropriate_flag << std::endl;
        }
        os << "  video_signal_type_present_flag = " << sps.video_signal_type_present_flag << std::endl;
        if (sps.video_signal_type_present_flag) {
            os << "    video_format = " << sps.video_format << std::endl;
            os << "    video_full_range_flag = " << sps.video_full_range_flag << std::endl;
            os << "    colour_description_present_flag = " << sps.colour_description_present_flag << std::endl;
            if (sps.colour_description_present_flag) {
                os << "      colour_primaries = " << sps.colour_primaries << std::endl;
                os << "      transfer_characteristics = " << sps.transfer_characteristics << std::endl;
                os << "      matrix_coefficients = " << sps.matrix_coefficients << std::endl;
            }
        }
        os << "  chroma_loc_info_present_flag = " << sps.chroma_loc_info_present_flag << std::endl;
        if (sps.chroma_loc_info_present_flag) {
            os << "    chroma_sample_loc_type_top_field = " << sps.chroma_sample_loc_type_top_field << std::endl;
            os << "    chroma_sample_loc_type_bottom_field = " << sps.chroma_sample_loc_type_bottom_field << std::endl;
        }
        os << "  timing_info_present_flag = " << sps.timing_info_present_flag << std::endl;
        if (sps.timing_info_present_flag) {
            os << "    num_units_in_tick = " << sps.num_units_in_tick << std::endl;
            os << "    time_scale = " << sps.time_scale << std::endl;
            os << "    fixed_frame_rate_flag = " << sps.fixed_frame_rate_flag << std::endl;
        }
    }

    return os;
}

static std::ostream &operator<<(std::ostream &os, HEVC_SPS_T &sps) {
    os << "sps_video_parameter_set_id = " << sps.sps_video_parameter_set_id << std::endl;
    os << "sps_max_sub_layers_minus1 = " << sps.sps_max_sub_layers_minus1 << std::endl;
    os << "sps_temporal_id_nesting_flag = " << sps.sps_temporal_id_nesting_flag << std::endl;
    os << "general_profile_idc = " << sps.general_profile_idc << std::endl;
    os << "general_level_idc = " << sps.general_level_idc << std::endl;
    os << "sps_seq_parameter_set_id = " << sps.sps_seq_parameter_set_id << std::endl;
    os << "chroma_format_idc = " << sps.chroma_format_idc << std::endl;
    os << "pic_width_in_luma_samples = " << sps.pic_width_in_luma_samples << std::endl;
    os << "pic_height_in_luma_samples = " << sps.pic_height_in_luma_samples << std::endl;
    os << "conformance_window_flag = " << sps.conformance_window_flag << std::endl;
    if (sps.conformance_window_flag) {
        os << "  conf_win_left_offset = " << sps.conf_win_left_offset << std::endl;
        os << "  conf_win_right_offset = " << sps.conf_win_right_offset << std::endl;
        os << "  conf_win_top_offset = " << sps.conf_win_top_offset << std::endl;
        os << "  conf_win_bottom_offset = " << sps.conf_win_bottom_offset << std::endl;
    }
    os << "bit_depth_luma_minus8 = " << sps.bit_depth_luma_minus8 << std::endl;
    os << "bit_depth_chroma_minus8 = " << sps.bit_depth_chroma_minus8 << std::endl;
    os << "log2_max_pic_order_cnt_lsb_minus4 = " << sps.log2_max_pic_order_cnt_lsb_minus4 << std::endl;
    os << "sps_sub_layer_ordering_info_present_flag = " << sps.sps_sub_layer_ordering_info_present_flag << std::endl;
    os << "log2_min_luma_coding_block_size_minus3 = " << sps.log2_min_luma_coding_block_size_minus3 << std::endl;
    os << "log2_diff_max_min_luma_coding_block_size = " << sps.log2_diff_max_min_luma_coding_block_size << std::endl;
    os << "log2_min_luma_transform_block_size_minus2 = " << sps.log2_min_luma_transform_block_size_minus2 << std::endl;
    os << "log2_diff_max_min_luma_transform_block_size = " << sps.log2_diff_max_min_luma_transform_block_size << std::endl;
    os << "max_transform_hierarchy_depth_inter = " << sps.max_transform_hierarchy_depth_inter << std::endl;
    os << "max_transform_hierarchy_depth_intra = " << sps.max_transform_hierarchy_depth_intra << std::endl;
    os << "scaling_list_enabled_flag = " << sps.scaling_list_enabled_flag << std::endl;
    os << "amp_enabled_flag = " << sps.amp_enabled_flag << std::endl;
    os << "sample_adaptive_offset_enabled_flag = " << sps.sample_adaptive_offset_enabled_flag << std::endl;
    os << "pcm_enabled_flag = " << sps.pcm_enabled_flag << std::endl;
    os << "num_short_term_ref_pic_sets = " << sps.nb_st_rps << std::endl;
    os << "long_term_ref_pics_present_flag = " << sps.long_term_ref_pics_present_flag << std::endl;
    os << "sps_temporal_mvp_enabled_flag = " << sps.sps_temporal_mvp_enabled_flag << std::endl;
    os << "strong_intra_smoothing_enabled_flag = " << sps.strong_intra_smoothing_enabled_flag << std::endl;
    os << "vui_parameters_present_flag = " << sps.vui_parameters_present_flag << std::endl;

    return os;
}

static AX_VOID sps_bs_init(SPS_BIT_STREAM_T *bs, const AX_U8 *data, AX_U32 size) {
    if (bs) {
        bs->data = data;
        bs->index = 0;
        bs->size_in_bits = size << 3;
    }
}

static inline AX_BOOL eof(SPS_BIT_STREAM_T *bs) {
    return (bs->index >= bs->size_in_bits) ? AX_TRUE : AX_FALSE;
}

static AX_U32 u(SPS_BIT_STREAM_T *bs, AX_U8 bits) {
    AX_U32 val = 0;

    for (AX_U8 i = 0; i < bits; i++) {
        val <<= 1;

        if (eof(bs)) {
            val = 0;
            break;
        } else if (bs->data[bs->index / 8] & (0x80 >> (bs->index % 8))) {
            val |= 1;
        }

        ++bs->index;
    }

    return val;
}

static AX_U32 ue(SPS_BIT_STREAM_T *bs) {
    AX_U32 zeroNum = 0;
    while (u(bs, 1) == 0 && !eof(bs) && zeroNum < 32) {
        zeroNum++;
    }

    return (AX_U32)((1 << zeroNum) - 1 + u(bs, zeroNum));
}

static AX_S32 se(SPS_BIT_STREAM_T *bs) {
    AX_S32 ueVal = (AX_S32)ue(bs);
    double k = ueVal;

    AX_S32 seVal = (AX_S32)ceil(k / 2);
    if (ueVal % 2 == 0) {
        seVal = -seVal;
    }

    return seVal;
}

static AX_VOID del_emulation_prevention(AX_U8 *data, AX_U32 &size) {
    /*
        0x00 0x00 0x03 0x00 -> 0x00 0x00 0x00
        0x00 0x00 0x03 0x01 -> 0x00 0x00 0x01
        0x00 0x00 0x03 0x02 -> 0x00 0x00 0x02
        0x00 0x00 0x03 0x03 -> 0x00 0x00 0x03
    */

    AX_U32 bytes = size;
    for (AX_U32 i = 0; i < (bytes - 2); ++i) {
        AX_S32 val = (data[i] ^ 0x00) + (data[i + 1] ^ 0x00) + (data[i + 2] ^ 0x03);
        if (val == 0) {
            for (AX_U32 j = i + 2; j < bytes - 1; j++) {
                data[j] = data[j + 1];
            }

            --size;
        }
    }
}

AX_BOOL h264_parse_sps(const AX_U8 *data, AX_U32 size, SPS_INFO_T *info) {
    if (!data || 0 == size || !info) {
        fprintf(stderr, "h264_parse_sps: invalid parameter!");
        return AX_FALSE;
    }

    memset(info, 0, sizeof(SPS_INFO_T));

    AX_S32 nalu_type = (data[0] & 0x1F);
    if (H264_NAL_SPS != nalu_type) {
        fprintf(stderr, "h264_parse_sps: not h264 sps nalu!");
        return AX_FALSE;
    }

    AX_U8 *buf = (AX_U8 *)malloc(size);
    if (!buf) {
        fprintf(stderr, "h264_parse_sps: malloc %d fail!", size);
        return AX_FALSE;
    }

    memcpy(buf, data, size);
    del_emulation_prevention(buf, size);

    SPS_BIT_STREAM_T bs;
    sps_bs_init(&bs, buf, size);
    bs.index = 8;

    H264_SPS_T sps;
    memset(&sps, 0, sizeof(sps));

    sps.profile_idc = u(&bs, 8);
    info->profile_idc = sps.profile_idc;

    sps.constraint_set0_flag = u(&bs, 1);
    sps.constraint_set1_flag = u(&bs, 1);
    sps.constraint_set2_flag = u(&bs, 1);
    sps.constraint_set3_flag = u(&bs, 1);
    sps.constraint_set4_flag = u(&bs, 1);
    sps.constraint_set5_flag = u(&bs, 1);
    (AX_VOID) u(&bs, 2);  // reserved_zero_2bits
    sps.level_idc = u(&bs, 8);
    info->level_idc = sps.level_idc;

    sps.seq_parameter_set_id = ue(&bs);
    if (SPECIAL_PROFILE_IDC(sps.profile_idc)) {
        sps.chroma_format_idc = ue(&bs);
        if (sps.chroma_format_idc == 3) {
            sps.residual_colour_transform_flag = u(&bs, 1);
        }
        sps.bit_depth_luma_minus8 = ue(&bs);
        sps.bit_depth_chroma_minus8 = ue(&bs);
        sps.qpprime_y_zero_transform_bypass_flag = u(&bs, 1);
        sps.seq_scaling_matrix_present_flag = u(&bs, 1);
        if (sps.seq_scaling_matrix_present_flag) {
            for (AX_S32 i = 0; i < ((sps.chroma_format_idc != 3) ? 8 : 12); ++i) {
                (AX_VOID) u(&bs, 1);  // seq_scaling_list_present_flag[i]
            }
        }
    } else {
        sps.chroma_format_idc = 1;
    }

    sps.log2_max_frame_num_minus4 = ue(&bs);
    sps.pic_order_cnt_type = ue(&bs);
    if (sps.pic_order_cnt_type == 0) {
        sps.log2_max_pic_order_cnt_lsb_minus4 = ue(&bs);
    } else if (sps.pic_order_cnt_type == 1) {
        sps.delta_pic_order_always_zero_flag = u(&bs, 1);
        sps.offset_for_non_ref_pic = se(&bs);
        sps.offset_for_top_to_bottom_field = se(&bs);
        AX_S32 num_ref_frames_in_pic_order_cnt_cycle = ue(&bs);
        for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
            (AX_VOID) se(&bs);  // offset_for_ref_frame[i]
        }
    }

    sps.num_ref_frames = ue(&bs);
    info->num_ref_frames = sps.num_ref_frames;

    sps.gaps_in_frame_num_value_allowed_flag = u(&bs, 1);
    sps.pic_width_in_mbs_minus1 = ue(&bs);
    sps.pic_height_in_map_units_minus1 = ue(&bs);
    sps.frame_mbs_only_flag = u(&bs, 1);
    if (!sps.frame_mbs_only_flag) {
        sps.mb_adaptive_frame_field_flag = u(&bs, 1);
    }

    sps.direct_8x8_inference_flag = u(&bs, 1);
    sps.frame_cropping_flag = u(&bs, 1);
    if (sps.frame_cropping_flag) {
        sps.frame_crop_left_offset = ue(&bs);
        sps.frame_crop_right_offset = ue(&bs);
        sps.frame_crop_top_offset = ue(&bs);
        sps.frame_crop_bottom_offset = ue(&bs);
    }

    info->width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
    info->height = (2 - sps.frame_mbs_only_flag) * (sps.pic_height_in_map_units_minus1 + 1) * 16;

    if (sps.frame_cropping_flag) {
        AX_U32 crop_unit_x;
        AX_U32 crop_unit_y;

        if (0 == sps.chroma_format_idc) {
            // monochrome
            crop_unit_x = 1;
            crop_unit_y = 2 - sps.frame_mbs_only_flag;
        } else if (1 == sps.chroma_format_idc) {
            // 4:2:0
            crop_unit_x = 2;
            crop_unit_y = 2 * (2 - sps.frame_mbs_only_flag);
        } else if (2 == sps.chroma_format_idc) {
            // 4:2:2
            crop_unit_x = 2;
            crop_unit_y = 2 - sps.frame_mbs_only_flag;
        } else {
            // 4:4:4
            crop_unit_x = 1;
            crop_unit_y = 2 - sps.frame_mbs_only_flag;
        }

        info->width -= crop_unit_x * (sps.frame_crop_left_offset + sps.frame_crop_right_offset);
        info->height -= crop_unit_y * (sps.frame_crop_top_offset + sps.frame_crop_bottom_offset);
    }

    sps.vui_parameters_present_flag = u(&bs, 1);
    if (sps.vui_parameters_present_flag) {
        sps.aspect_ratio_info_present_flag = u(&bs, 1);
        if (sps.aspect_ratio_info_present_flag) {
            sps.aspect_ratio_idc = u(&bs, 8);
            if (sps.aspect_ratio_idc == EXTENDED_SAR) {
                sps.sar_width = u(&bs, 16);
                sps.sar_height = u(&bs, 16);
            }
        }
        sps.overscan_info_present_flag = u(&bs, 1);
        if (sps.overscan_info_present_flag) {
            sps.overscan_appropriate_flag = u(&bs, 1);
        }
        sps.video_signal_type_present_flag = u(&bs, 1);
        if (sps.video_signal_type_present_flag) {
            sps.video_format = u(&bs, 3);
            sps.video_full_range_flag = u(&bs, 1);
            sps.colour_description_present_flag = u(&bs, 1);
            if (sps.colour_description_present_flag) {
                sps.colour_primaries = u(&bs, 8);
                sps.transfer_characteristics = u(&bs, 8);
                sps.matrix_coefficients = u(&bs, 8);
            }
        }

        sps.chroma_loc_info_present_flag = u(&bs, 1);
        if (sps.chroma_loc_info_present_flag) {
            sps.chroma_sample_loc_type_top_field = ue(&bs);
            sps.chroma_sample_loc_type_bottom_field = ue(&bs);
        }
        sps.timing_info_present_flag = u(&bs, 1);
        if (sps.timing_info_present_flag) {
            sps.num_units_in_tick = u(&bs, 32);
            sps.time_scale = u(&bs, 32);
            sps.fixed_frame_rate_flag = u(&bs, 1);

            info->fps = sps.time_scale / sps.num_units_in_tick / 2;
        }
    }

    free(buf);

    AX_BOOL bPrint = AX_FALSE;
    char *env = getenv("PRINT_SPS_INFO");
    if (env) {
        bPrint = (1 == atoi(env)) ? AX_TRUE : AX_FALSE;
    }

    if (bPrint) {
        std::cout << sps;
    }

    return AX_TRUE;
}

static void set_default_scaling_list_data(ScalingList *sl) {
    int matrixId;

    for (matrixId = 0; matrixId < 6; matrixId++) {
        // 4x4 default is 16
        memset(sl->sl[0][matrixId], 16, 16);
        sl->sl_dc[0][matrixId] = 16;  // default for 16x16
        sl->sl_dc[1][matrixId] = 16;  // default for 32x32
    }
    memcpy(sl->sl[1][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[1][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[1][5], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][5], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][5], default_scaling_list_inter, 64);
}

static int scaling_list_data(SPS_BIT_STREAM_T *bs, ScalingList *sl, HEVC_SPS_T &sps) {
    AX_U8 scaling_list_pred_mode_flag;
    AX_U8 scaling_list_dc_coef[2][6];
    int size_id, matrix_id, pos;
    int i;

    for (size_id = 0; size_id < 4; size_id++)
        for (matrix_id = 0; matrix_id < 6; matrix_id += ((size_id == 3) ? 3 : 1)) {
            scaling_list_pred_mode_flag = u(bs, 1);
            if (!scaling_list_pred_mode_flag) {
                unsigned int delta = ue(bs);
                /* Only need to handle non-zero delta. Zero means default,
                 * which should already be in the arrays. */
                if (delta) {
                    // Copy from previous array.
                    delta *= (size_id == 3) ? 3 : 1;
                    if (matrix_id < (int)delta) {
                        fprintf(stderr, "Invalid delta in scaling list data: %d.\n", delta);
                        return -1;
                    }

                    memcpy(sl->sl[size_id][matrix_id], sl->sl[size_id][matrix_id - delta], size_id > 0 ? 64 : 16);
                    if (size_id > 1) sl->sl_dc[size_id - 2][matrix_id] = sl->sl_dc[size_id - 2][matrix_id - delta];
                }
            } else {
                int next_coef, coef_num;
                AX_S32 scaling_list_delta_coef;

                next_coef = 8;
                coef_num = FFMIN(64, 1 << (4 + (size_id << 1)));
                if (size_id > 1) {
                    int scaling_list_coeff_minus8 = se(bs);
                    if (scaling_list_coeff_minus8 < -7 || scaling_list_coeff_minus8 > 247) return -1;
                    scaling_list_dc_coef[size_id - 2][matrix_id] = scaling_list_coeff_minus8 + 8;
                    next_coef = scaling_list_dc_coef[size_id - 2][matrix_id];
                    sl->sl_dc[size_id - 2][matrix_id] = next_coef;
                }
                for (i = 0; i < coef_num; i++) {
                    if (size_id == 0)
                        pos = 4 * ff_hevc_diag_scan4x4_y[i] + ff_hevc_diag_scan4x4_x[i];
                    else
                        pos = 8 * ff_hevc_diag_scan8x8_y[i] + ff_hevc_diag_scan8x8_x[i];

                    scaling_list_delta_coef = se(bs);
                    next_coef = (next_coef + 256U + scaling_list_delta_coef) % 256;
                    sl->sl[size_id][matrix_id][pos] = next_coef;
                }
            }
        }

    if (sps.chroma_format_idc == 3) {
        for (i = 0; i < 64; i++) {
            sl->sl[3][1][i] = sl->sl[2][1][i];
            sl->sl[3][2][i] = sl->sl[2][2][i];
            sl->sl[3][4][i] = sl->sl[2][4][i];
            sl->sl[3][5][i] = sl->sl[2][5][i];
        }
        sl->sl_dc[1][1] = sl->sl_dc[0][1];
        sl->sl_dc[1][2] = sl->sl_dc[0][2];
        sl->sl_dc[1][4] = sl->sl_dc[0][4];
        sl->sl_dc[1][5] = sl->sl_dc[0][5];
    }

    return 0;
}

static int decode_short_term_rps(SPS_BIT_STREAM_T *bs, ShortTermRPS *rps, HEVC_SPS_T *sps, int is_slice_header) {
    uint8_t rps_predict = 0;
    int delta_poc;
    int k0 = 0;
    int k = 0;
    int i;

    if (rps != sps->st_rps && sps->nb_st_rps) rps_predict = u(bs, 1);

    if (rps_predict) {
        const ShortTermRPS *rps_ridx;
        int delta_rps;
        unsigned abs_delta_rps;
        uint8_t use_delta_flag = 0;
        uint8_t delta_rps_sign;

        if (is_slice_header) {
            unsigned int delta_idx = ue(bs) + 1;
            if (delta_idx > sps->nb_st_rps) {
                fprintf(stderr, "Invalid value of delta_idx in slice header RPS: %d > %d.\n", delta_idx, sps->nb_st_rps);
                return -1;
            }
            rps_ridx = &sps->st_rps[sps->nb_st_rps - delta_idx];
            rps->rps_idx_num_delta_pocs = rps_ridx->num_delta_pocs;
        } else
            rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];

        delta_rps_sign = u(bs, 1);
        abs_delta_rps = ue(bs) + 1;
        if (abs_delta_rps < 1 || abs_delta_rps > 32768) {
            fprintf(stderr, "Invalid value of abs_delta_rps: %d\n", abs_delta_rps);
            return -1;
        }
        delta_rps = (1 - (delta_rps_sign << 1)) * abs_delta_rps;
        for (i = 0; i <= rps_ridx->num_delta_pocs; i++) {
            int used = rps->used[k] = u(bs, 1);

            if (!used) use_delta_flag = u(bs, 1);

            if (used || use_delta_flag) {
                if (i < rps_ridx->num_delta_pocs)
                    delta_poc = delta_rps + rps_ridx->delta_poc[i];
                else
                    delta_poc = delta_rps;
                rps->delta_poc[k] = delta_poc;
                if (delta_poc < 0) k0++;
                k++;
            }
        }

        if (k >= (int)(FF_ARRAY_ELEMS(rps->used))) {
            fprintf(stderr, "Invalid num_delta_pocs: %d\n", k);
            return -1;
        }

        rps->num_delta_pocs = k;
        rps->num_negative_pics = k0;
        // sort in increasing order (smallest first)
        if (rps->num_delta_pocs != 0) {
            int used, tmp;
            for (i = 1; i < rps->num_delta_pocs; i++) {
                delta_poc = rps->delta_poc[i];
                used = rps->used[i];
                for (k = i - 1; k >= 0; k--) {
                    tmp = rps->delta_poc[k];
                    if (delta_poc < tmp) {
                        rps->delta_poc[k + 1] = tmp;
                        rps->used[k + 1] = rps->used[k];
                        rps->delta_poc[k] = delta_poc;
                        rps->used[k] = used;
                    }
                }
            }
        }
        if ((rps->num_negative_pics >> 1) != 0) {
            int used;
            k = rps->num_negative_pics - 1;
            // flip the negative values to largest first
            for (i = 0; i < (int)(rps->num_negative_pics >> 1); i++) {
                delta_poc = rps->delta_poc[i];
                used = rps->used[i];
                rps->delta_poc[i] = rps->delta_poc[k];
                rps->used[i] = rps->used[k];
                rps->delta_poc[k] = delta_poc;
                rps->used[k] = used;
                k--;
            }
        }
    } else {
        unsigned int prev, nb_positive_pics;
        rps->num_negative_pics = ue(bs);
        nb_positive_pics = ue(bs);

        if (rps->num_negative_pics >= HEVC_MAX_REFS || nb_positive_pics >= HEVC_MAX_REFS) {
            fprintf(stderr, "Too many refs in a short term RPS.\n");
            return -1;
        }

        rps->num_delta_pocs = rps->num_negative_pics + nb_positive_pics;
        if (rps->num_delta_pocs) {
            prev = 0;
            for (i = 0; i < (int)rps->num_negative_pics; i++) {
                delta_poc = ue(bs) + 1;
                if (delta_poc < 1 || delta_poc > 32768) {
                    fprintf(stderr, "Invalid value of delta_poc: %d\n", delta_poc);
                    return -1;
                }
                prev -= delta_poc;
                rps->delta_poc[i] = prev;
                rps->used[i] = u(bs, 1);
            }
            prev = 0;
            for (i = 0; i < (int)nb_positive_pics; i++) {
                delta_poc = ue(bs) + 1;
                if (delta_poc < 1 || delta_poc > 32768) {
                    fprintf(stderr, "Invalid value of delta_poc: %d\n", delta_poc);
                    return -1;
                }
                prev += delta_poc;
                rps->delta_poc[rps->num_negative_pics + i] = prev;
                rps->used[rps->num_negative_pics + i] = u(bs, 1);
            }
        }
    }
    return 0;
}

static AX_VOID decode_profile_tier_level(SPS_BIT_STREAM_T *bs, AX_S32 *profile) {
    /* https://wiki.aixin-chip.com/display/SW/H.265+Profile-Tier-Level+Syntax */
    /* ffmpeg libavcodec/hevc_ps.c */
    (AX_VOID) u(bs, 2);  // general_profile_space
    (AX_VOID) u(bs, 1);  // general_tier_flag
    AX_S32 general_profile_idc = u(bs, 5);
    AX_S32 general_profile_compatibility_flag[32];
    for (AX_U32 i = 0; i < 32; ++i) {
        general_profile_compatibility_flag[i] = u(bs, 1);
        if (general_profile_idc == 0 && i > 0 && general_profile_compatibility_flag[i]) {
            general_profile_idc = i;
        }
    }

    if (profile) {
        *profile = general_profile_idc;
    }

    (AX_VOID) u(bs, 1);  // general_progressive_source_flag
    (AX_VOID) u(bs, 1);  // general_interlaced_source_flag
    (AX_VOID) u(bs, 1);  // general_non_packed_constraint_flag
    (AX_VOID) u(bs, 1);  // general_frame_only_constraint_flag

#define check_profile_idc(idc) general_profile_idc == idc || general_profile_compatibility_flag[idc]

    if (check_profile_idc(4) || check_profile_idc(5) || check_profile_idc(6) || check_profile_idc(7) || check_profile_idc(8) ||
        check_profile_idc(9) || check_profile_idc(10)) {
        (AX_VOID) u(bs, 1);  // general_max_12bit_constraint_flag
        (AX_VOID) u(bs, 1);  // general_max_10bit_constraint_flag
        (AX_VOID) u(bs, 1);  // general_max_8bit_constraint_flag
        (AX_VOID) u(bs, 1);  // general_max_422chroma_constraint_flag
        (AX_VOID) u(bs, 1);  // general_max_420chroma_constraint_flag
        (AX_VOID) u(bs, 1);  // general_max_monochrome_constraint_flag
        (AX_VOID) u(bs, 1);  // general_intra_constraint_flag
        (AX_VOID) u(bs, 1);  // general_one_picture_only_constraint_flag
        (AX_VOID) u(bs, 1);  // general_lower_bit_rate_constraint_flag

        if (check_profile_idc(5) || check_profile_idc(9) || check_profile_idc(10)) {
            (AX_VOID) u(bs, 1);   // general_max_14bit_constraint_flag
            (AX_VOID) u(bs, 33);  // XXX_reserved_zero_33bits[0..32]
        } else {
            (AX_VOID) u(bs, 34);  // XXX_reserved_zero_33bits[0..33]
        }
    } else if (check_profile_idc(2)) {
        (AX_VOID) u(bs, 7);
        (AX_VOID) u(bs, 1);   // general_one_picture_only_constraint_flag
        (AX_VOID) u(bs, 35);  // XXX_reserved_zero_33bits[0..34]
    } else {
        (AX_VOID) u(bs, 43);  // XXX_reserved_zero_33bits[0..42]
    }

    if (check_profile_idc(1) || check_profile_idc(2) || check_profile_idc(3) || check_profile_idc(4) || check_profile_idc(5) ||
        check_profile_idc(9)) {
        (AX_VOID) u(bs, 1);  // general_inbld_flag
    } else {
        (AX_VOID) u(bs, 1);
    }
#undef check_profile_idc
}

static AX_VOID parse_ptl(SPS_BIT_STREAM_T *bs, HEVC_SPS_T &sps) {
    decode_profile_tier_level(bs, &sps.general_profile_idc);

    AX_S32 i;
    sps.general_level_idc = u(bs, 8);

    AX_S32 max_num_sub_layers = sps.sps_max_sub_layers_minus1 + 1;
    AX_S32 sub_layer_profile_present_flag[7];
    AX_S32 sub_layer_level_present_flag[7];
    for (i = 0; i < max_num_sub_layers - 1; i++) {
        sub_layer_profile_present_flag[i] = u(bs, 1);
        sub_layer_level_present_flag[i] = u(bs, 1);
    }

    if (max_num_sub_layers - 1 > 0)
        for (i = max_num_sub_layers - 1; i < 8; i++) (AX_VOID) u(bs, 2);  // reserved_zero_2bits[i]
    for (i = 0; i < max_num_sub_layers - 1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            decode_profile_tier_level(bs, nullptr);
        }

        if (sub_layer_level_present_flag[i]) {
            (AX_VOID) u(bs, 8);  // sub_layer_level_idc
        }
    }
}

AX_BOOL hevc_parse_sps(const AX_U8 *data, AX_U32 size, SPS_INFO_T *info) {
    if (!data || 0 == size || !info) {
        fprintf(stderr, "%s: invalid parameter!", __FUNCTION__);
        return AX_FALSE;
    }

    memset(info, 0, sizeof(SPS_INFO_T));

    AX_S32 nalu_type = (data[0] & 0x7E) >> 1;
    if (HEVC_NAL_SPS != nalu_type) {
        fprintf(stderr, "%s: not hevc sps nalu!", __FUNCTION__);
        return AX_FALSE;
    }

    AX_U8 *buf = (AX_U8 *)malloc(size);
    if (!buf) {
        fprintf(stderr, "%s: malloc %d fail!", __FUNCTION__, size);
        return AX_FALSE;
    }

    memcpy(buf, data, size);
    del_emulation_prevention(buf, size);

    SPS_BIT_STREAM_T bs;
    sps_bs_init(&bs, buf, size);
    bs.index = 16;

    HEVC_SPS_T sps;
    memset(&sps, 0, sizeof(sps));

    sps.sps_video_parameter_set_id = u(&bs, 4);
    sps.sps_max_sub_layers_minus1 = u(&bs, 3);
    sps.sps_temporal_id_nesting_flag = u(&bs, 1);
    parse_ptl(&bs, sps);
    info->profile_idc = sps.general_profile_idc;
    info->level_idc = sps.general_level_idc;

    sps.sps_seq_parameter_set_id = ue(&bs);
    sps.chroma_format_idc = ue(&bs);

    if (sps.chroma_format_idc == 3) {
        sps.separate_colour_plane_flag = u(&bs, 1);
    }

    if (sps.separate_colour_plane_flag) {
        sps.chroma_format_idc = 0;
    }

    sps.pic_width_in_luma_samples = ue(&bs);
    sps.pic_height_in_luma_samples = ue(&bs);

    info->width = sps.pic_width_in_luma_samples;
    info->height = sps.pic_height_in_luma_samples;

    sps.conformance_window_flag = u(&bs, 1);
    if (sps.conformance_window_flag) {
        sps.conf_win_left_offset = ue(&bs);
        sps.conf_win_right_offset = ue(&bs);
        sps.conf_win_top_offset = ue(&bs);
        sps.conf_win_bottom_offset = ue(&bs);
    }

    AX_S32 sub_width_c = ((1 == sps.chroma_format_idc) || (2 == sps.chroma_format_idc)) && (0 == sps.separate_colour_plane_flag) ? 2 : 1;
    AX_S32 sub_height_c = (1 == sps.chroma_format_idc) && (0 == sps.separate_colour_plane_flag) ? 2 : 1;
    info->width -= (sub_width_c * sps.conf_win_right_offset + sub_width_c * sps.conf_win_left_offset);
    info->height -= (sub_height_c * sps.conf_win_bottom_offset + sub_height_c * sps.conf_win_top_offset);

    sps.bit_depth_luma_minus8 = ue(&bs);
    sps.bit_depth_chroma_minus8 = ue(&bs);
    sps.log2_max_pic_order_cnt_lsb_minus4 = ue(&bs);
    sps.sps_sub_layer_ordering_info_present_flag = u(&bs, 1);

    for (AX_S32 i = (sps.sps_sub_layer_ordering_info_present_flag ? 0 : sps.sps_max_sub_layers_minus1); i <= sps.sps_max_sub_layers_minus1;
         i++) {
        (AX_VOID) ue(&bs);  // sps_max_dec_pic_buffering_minus1[i]
        (AX_VOID) ue(&bs);  // sps_max_num_reorder_pics[i]
        (AX_VOID) ue(&bs);  // sps_max_latency_increase_plus1[i]
    }

    sps.log2_min_luma_coding_block_size_minus3 = ue(&bs);
    sps.log2_diff_max_min_luma_coding_block_size = ue(&bs);
    sps.log2_min_luma_transform_block_size_minus2 = ue(&bs);
    sps.log2_diff_max_min_luma_transform_block_size = ue(&bs);
    sps.max_transform_hierarchy_depth_inter = ue(&bs);
    sps.max_transform_hierarchy_depth_intra = ue(&bs);

    sps.scaling_list_enabled_flag = u(&bs, 1);
    if (sps.scaling_list_enabled_flag) {
        set_default_scaling_list_data(&sps.scaling_list);
        if (u(&bs, 1)) {
            scaling_list_data(&bs, &sps.scaling_list, sps);
        }
    }

    sps.amp_enabled_flag = u(&bs, 1);
    sps.sample_adaptive_offset_enabled_flag = u(&bs, 1);
    sps.pcm_enabled_flag = u(&bs, 1);
    if (sps.pcm_enabled_flag) {
        (AX_VOID) u(&bs, 4);  // pcm_sample_bit_depth_luma_minus1
        (AX_VOID) u(&bs, 4);  // pcm_sample_bit_depth_chroma_minus1
        (AX_VOID) ue(&bs);    // log2_min_pcm_luma_coding_block_size_minus3
        (AX_VOID) ue(&bs);    // log2_diff_max_min_pcm_luma_coding_block_size
        (AX_VOID) u(&bs, 1);  // pcm_loop_filter_disabled_flag
    }

    sps.nb_st_rps = ue(&bs);
    for (AX_U32 i = 0; i < sps.nb_st_rps; i++) {
        decode_short_term_rps(&bs, &sps.st_rps[i], &sps, 0);
    }

    sps.long_term_ref_pics_present_flag = u(&bs, 1);
    if (sps.long_term_ref_pics_present_flag) {
        AX_S32 num_long_term_ref_pics_sps = ue(&bs);
        for (AX_S32 i = 0; i < num_long_term_ref_pics_sps; ++i) {
            (AX_VOID) u(&bs, sps.log2_max_pic_order_cnt_lsb_minus4 + 4);  // lt_ref_pic_poc_lsb_sps
            (AX_VOID) u(&bs, 1);
        }
    }

    sps.sps_temporal_mvp_enabled_flag = u(&bs, 1);
    sps.strong_intra_smoothing_enabled_flag = u(&bs, 1);
    sps.vui_parameters_present_flag = u(&bs, 1);
    if (sps.vui_parameters_present_flag) {
        if (u(&bs, 1)) {
            AX_S32 aspect_ratio_idc = u(&bs, 8);
            if (aspect_ratio_idc == EXTENDED_SAR) {
                (AX_VOID) u(&bs, 16);  // sar_width
                (AX_VOID) u(&bs, 16);  // sar_height
            }
        }

        if (u(&bs, 1)) {  // overscan_info_present_flag
            (AX_VOID) u(&bs, 1);
        }

        if (u(&bs, 1)) {              // video_signal_type_present_flag
            (AX_VOID) u(&bs, 3);      // video_format
            (AX_VOID) u(&bs, 1);      // video_full_range_flag
            if (u(&bs, 1)) {          // colour_description_present_flag
                (AX_VOID) u(&bs, 8);  // colour_primaries
                (AX_VOID) u(&bs, 8);  // transfer_characteristics
                (AX_VOID) u(&bs, 8);  // matrix_coefficients
            }
        }

        if (u(&bs, 1)) {        // chroma_loc_info_present_flag
            (AX_VOID) ue(&bs);  // chroma_sample_loc_type_top_field
            (AX_VOID) ue(&bs);  // chroma_sample_loc_type_bottom_field
        }

        (AX_VOID) u(&bs, 1);  // neutra_chroma_indication_flag
        (AX_VOID) u(&bs, 1);  // field_seq_flag
        (AX_VOID) u(&bs, 1);  // frame_field_info_present_flag

        if (u(&bs, 1)) {        // default_display_window_flag
            (AX_VOID) ue(&bs);  // def_disp_win_left_offset
            (AX_VOID) ue(&bs);  // def_disp_win_right_offset
            (AX_VOID) ue(&bs);  // def_disp_win_top_offset
            (AX_VOID) ue(&bs);  // def_disp_win_bottom_offset
        }

        if (u(&bs, 1)) {  // timing_info_present_flag
            AX_S32 num_units_in_tick = u(&bs, 32);
            AX_S32 time_scale = u(&bs, 32);
            info->fps = time_scale / num_units_in_tick;
        }
    }

    free(buf);

    AX_BOOL bPrint = AX_FALSE;
    char *env = getenv("PRINT_SPS_INFO");
    if (env) {
        bPrint = (1 == atoi(env)) ? AX_TRUE : AX_FALSE;
    }

    if (bPrint) {
        std::cout << sps;
    }

    return AX_TRUE;
}
