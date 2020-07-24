
/*
 * h264_sps_parser.h
 *
 * Copyright (c) 2014 Zhou Quan <zhouqicy@gmail.com>
 *
 * This file is part of ijkPlayer.
 *
 * ijkPlayer is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * ijkPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ijkPlayer; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef IJKMediaPlayer_h264_sps_parser_h
#define IJKMediaPlayer_h264_sps_parser_h
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>

static const uint8_t default_scaling4[2][16] = {
    {  6, 13, 20, 28, 13, 20, 28, 32,
        20, 28, 32, 37, 28, 32, 37, 42 },
    { 10, 14, 20, 24, 14, 20, 24, 27,
        20, 24, 27, 30, 24, 27, 30, 34 }
};
static const uint8_t default_scaling8[2][64] = {
    {  6, 10, 13, 16, 18, 23, 25, 27,
        10, 11, 16, 18, 23, 25, 27, 29,
        13, 16, 18, 23, 25, 27, 29, 31,
        16, 18, 23, 25, 27, 29, 31, 33,
        18, 23, 25, 27, 29, 31, 33, 36,
        23, 25, 27, 29, 31, 33, 36, 38,
        25, 27, 29, 31, 33, 36, 38, 40,
        27, 29, 31, 33, 36, 38, 40, 42 },
    {  9, 13, 15, 17, 19, 21, 22, 24,
        13, 13, 17, 19, 21, 22, 24, 25,
        15, 17, 19, 21, 22, 24, 25, 27,
        17, 19, 21, 22, 24, 25, 27, 28,
        19, 21, 22, 24, 25, 27, 28, 30,
        21, 22, 24, 25, 27, 28, 30, 32,
        22, 24, 25, 27, 28, 30, 32, 33,
        24, 25, 27, 28, 30, 32, 33, 35 }
};
static const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};
static const uint8_t ff_zigzag_scan[16+1] = {
    0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
    1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
    1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
    3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};
typedef struct
{
    const uint8_t *data;
    const uint8_t *end;
    int head;
    uint64_t cache;
} nal_bitstream;
static void
nal_bs_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
    bs->data = data;
    bs->end  = data + size;
    bs->head = 0;
    // fill with something other than 0 to detect
    //  emulation prevention bytes
    bs->cache = 0xffffffff;
}
static uint64_t
nal_bs_read(nal_bitstream *bs, int n)
{
    uint64_t res = 0;
    int shift;
    
    if (n == 0)
        return res;
    
    // fill up the cache if we need to
    while (bs->head < n) {
        uint8_t a_byte;
        bool check_three_byte;
        
        check_three_byte = true;
    next_byte:
        if (bs->data >= bs->end) {
            // we're at the end, can't produce more than head number of bits
            n = bs->head;
            break;
        }
        // get the byte, this can be an emulation_prevention_three_byte that we need
        // to ignore.
        a_byte = *bs->data++;
        if (check_three_byte && a_byte == 0x03 && ((bs->cache & 0xffff) == 0)) {
            // next byte goes unconditionally to the cache, even if it's 0x03
            check_three_byte = false;
            goto next_byte;
        }
        // shift bytes in cache, moving the head bits of the cache left
        bs->cache = (bs->cache << 8) | a_byte;
        bs->head += 8;
    }
    
    // bring the required bits down and truncate
    if ((shift = bs->head - n) > 0)
        res = bs->cache >> shift;
    else
        res = bs->cache;
    
    // mask out required bits
    if (n < 32)
        res &= (1 << n) - 1;
    
    bs->head = shift;
    
    return res;
}
static bool
nal_bs_eos(nal_bitstream *bs)
{
    return (bs->data >= bs->end) && (bs->head == 0);
}
// read unsigned Exp-Golomb code
static int64_t
nal_bs_read_ue(nal_bitstream *bs)
{
    int i = 0;
    
    while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 32)
        i++;
    
    return ((1 << i) - 1 + nal_bs_read(bs, i));
}
//解码有符号的指数哥伦布编码
static int64_t
nal_bs_read_se(nal_bitstream *bs)
{
    int64_t ueVal = nal_bs_read_ue(bs);
    double k = ueVal;
    int64_t nValue = ceil(k/2);
    if(ueVal%2 == 0)
    {
        nValue = -nValue;
    }
    return nValue;
}
typedef struct
{
    uint64_t profile_idc;
    uint64_t level_idc;
    uint64_t sps_id;
    
    uint64_t chroma_format_idc;
    uint64_t separate_colour_plane_flag;
    uint64_t bit_depth_luma_minus8;
    uint64_t bit_depth_chroma_minus8;
    uint64_t qpprime_y_zero_transform_bypass_flag;
    uint64_t seq_scaling_matrix_present_flag;
    
    uint64_t log2_max_frame_num_minus4;
    uint64_t pic_order_cnt_type;
    uint64_t log2_max_pic_order_cnt_lsb_minus4;
    
    uint64_t max_num_ref_frames;
    uint64_t gaps_in_frame_num_value_allowed_flag;
    uint64_t pic_width_in_mbs_minus1;
    uint64_t pic_height_in_map_units_minus1;
    
    uint64_t frame_mbs_only_flag;
    uint64_t mb_adaptive_frame_field_flag;
    
    uint64_t direct_8x8_inference_flag;
    
    uint64_t frame_cropping_flag;
    uint64_t frame_crop_left_offset;
    uint64_t frame_crop_right_offset;
    uint64_t frame_crop_top_offset;
    uint64_t frame_crop_bottom_offset;
} sps_info_struct;
static void decode_scaling_list(nal_bitstream *bs, uint8_t *factors, int size,
                                const uint8_t *jvt_list,
                                const uint8_t *fallback_list)
{
    int i, last = 8, next = 8;
    const uint8_t *scan = size == 16 ? ff_zigzag_scan : ff_zigzag_direct;
    if (!nal_bs_read(bs, 1)) /* matrix not written, we use the predicted one */
        memcpy(factors, fallback_list, size * sizeof(uint8_t));
    else
        for (i = 0; i < size; i++) {
            if (next)
                next = (last + nal_bs_read_se(bs)) & 0xff;
            if (!i && !next) { /* matrix not written, we use the preset one */
                memcpy(factors, jvt_list, size * sizeof(uint8_t));
                break;
            }
            last = factors[scan[i]] = next ? next : last;
        }
}
static void decode_scaling_matrices(nal_bitstream *bs, sps_info_struct *sps,
                                    void* pps, int is_sps,
                                    uint8_t(*scaling_matrix4)[16],
                                    uint8_t(*scaling_matrix8)[64])
{
    int fallback_sps = !is_sps && sps->seq_scaling_matrix_present_flag;
    const uint8_t *fallback[4] = {
        fallback_sps ? scaling_matrix4[0] : default_scaling4[0],
        fallback_sps ? scaling_matrix4[3] : default_scaling4[1],
        fallback_sps ? scaling_matrix8[0] : default_scaling8[0],
        fallback_sps ? scaling_matrix8[3] : default_scaling8[1]
    };
    if (nal_bs_read(bs, 1)) {
        sps->seq_scaling_matrix_present_flag |= is_sps;
        decode_scaling_list(bs, scaling_matrix4[0], 16, default_scaling4[0], fallback[0]);        // Intra, Y
        decode_scaling_list(bs, scaling_matrix4[1], 16, default_scaling4[0], scaling_matrix4[0]); // Intra, Cr
        decode_scaling_list(bs, scaling_matrix4[2], 16, default_scaling4[0], scaling_matrix4[1]); // Intra, Cb
        decode_scaling_list(bs, scaling_matrix4[3], 16, default_scaling4[1], fallback[1]);        // Inter, Y
        decode_scaling_list(bs, scaling_matrix4[4], 16, default_scaling4[1], scaling_matrix4[3]); // Inter, Cr
        decode_scaling_list(bs, scaling_matrix4[5], 16, default_scaling4[1], scaling_matrix4[4]); // Inter, Cb
        if (is_sps) {
            decode_scaling_list(bs, scaling_matrix8[0], 64, default_scaling8[0], fallback[2]); // Intra, Y
            decode_scaling_list(bs, scaling_matrix8[3], 64, default_scaling8[1], fallback[3]); // Inter, Y
            if (sps->chroma_format_idc == 3) {
                decode_scaling_list(bs, scaling_matrix8[1], 64, default_scaling8[0], scaling_matrix8[0]); // Intra, Cr
                decode_scaling_list(bs, scaling_matrix8[4], 64, default_scaling8[1], scaling_matrix8[3]); // Inter, Cr
                decode_scaling_list(bs, scaling_matrix8[2], 64, default_scaling8[0], scaling_matrix8[1]); // Intra, Cb
                decode_scaling_list(bs, scaling_matrix8[5], 64, default_scaling8[1], scaling_matrix8[4]); // Inter, Cb
            }
        }
    }
}
static void parseh264_sps(sps_info_struct* sps_info, uint8_t* sps_data, uint32_t sps_size)
{
    if(sps_info == NULL)
        return;
    memset(sps_info, 0, sizeof(sps_info_struct));
    
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[6][64];
    memset(scaling_matrix4, 16, sizeof(scaling_matrix4));
    memset(scaling_matrix8, 16, sizeof(scaling_matrix8));
    
    nal_bitstream bs;
    nal_bs_init(&bs, sps_data, sps_size);
    
    sps_info->profile_idc  = nal_bs_read(&bs, 8);
    nal_bs_read(&bs, 1);  // constraint_set0_flag
    nal_bs_read(&bs, 1);  // constraint_set1_flag
    nal_bs_read(&bs, 1);  // constraint_set2_flag
    nal_bs_read(&bs, 1);  // constraint_set3_flag
    nal_bs_read(&bs, 4);  // reserved
    sps_info->level_idc    = nal_bs_read(&bs, 8);
    sps_info->sps_id      = nal_bs_read_ue(&bs);
    
    if (sps_info->profile_idc == 100 ||
        sps_info->profile_idc == 110 ||
        sps_info->profile_idc == 122 ||
        sps_info->profile_idc == 244 ||
        sps_info->profile_idc == 44  ||
        sps_info->profile_idc == 83  ||
        sps_info->profile_idc == 86  ||
        sps_info->profile_idc == 118 ||
        sps_info->profile_idc == 128 ||
        sps_info->profile_idc == 138 ||
        sps_info->profile_idc == 144)
    {
        sps_info->chroma_format_idc                    = nal_bs_read_ue(&bs);
        if (sps_info->chroma_format_idc == 3)
            sps_info->separate_colour_plane_flag        = nal_bs_read(&bs, 1);
        sps_info->bit_depth_luma_minus8                = nal_bs_read_ue(&bs);
        sps_info->bit_depth_chroma_minus8              = nal_bs_read_ue(&bs);
        sps_info->qpprime_y_zero_transform_bypass_flag = nal_bs_read(&bs, 1);
        
        //        sps_info->seq_scaling_matrix_present_flag = nal_bs_read (&bs, 1);
        decode_scaling_matrices(&bs, sps_info, NULL, 1, scaling_matrix4, scaling_matrix8);
        
    }
    sps_info->log2_max_frame_num_minus4 = nal_bs_read_ue(&bs);
    if (sps_info->log2_max_frame_num_minus4 > 12) {
        // must be between 0 and 12
        // don't early return here - the bits we are using (profile/level/interlaced/ref frames)
        // might still be valid - let the parser go on and pray.
        //return;
    }
    
    sps_info->pic_order_cnt_type = nal_bs_read_ue(&bs);
    if (sps_info->pic_order_cnt_type == 0) {
        sps_info->log2_max_pic_order_cnt_lsb_minus4 = nal_bs_read_ue(&bs);
    }
    else if (sps_info->pic_order_cnt_type == 1) {
        nal_bs_read(&bs, 1); //delta_pic_order_always_zero_flag
        nal_bs_read_se(&bs); //offset_for_non_ref_pic
        nal_bs_read_se(&bs); //offset_for_top_to_bottom_field
        
        int64_t num_ref_frames_in_pic_order_cnt_cycle = nal_bs_read_ue(&bs);
        for(int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            nal_bs_read_se(&bs); //int offset_for_ref_frame =
        }
    }
    
    sps_info->max_num_ref_frames            = nal_bs_read_ue(&bs);
    sps_info->gaps_in_frame_num_value_allowed_flag = nal_bs_read(&bs, 1);
    sps_info->pic_width_in_mbs_minus1        = nal_bs_read_ue(&bs);
    sps_info->pic_height_in_map_units_minus1 = nal_bs_read_ue(&bs);
    
    sps_info->frame_mbs_only_flag            = nal_bs_read(&bs, 1);
    if (!sps_info->frame_mbs_only_flag)
        sps_info->mb_adaptive_frame_field_flag = nal_bs_read(&bs, 1);
    
    sps_info->direct_8x8_inference_flag      = nal_bs_read(&bs, 1);
    
    sps_info->frame_cropping_flag            = nal_bs_read(&bs, 1);
    if (sps_info->frame_cropping_flag) {
        sps_info->frame_crop_left_offset      = nal_bs_read_ue(&bs);
        sps_info->frame_crop_right_offset      = nal_bs_read_ue(&bs);
        sps_info->frame_crop_top_offset        = nal_bs_read_ue(&bs);
        sps_info->frame_crop_bottom_offset    = nal_bs_read_ue(&bs);
    }
}
static void get_resolution_from_sps(uint8_t* sps, uint32_t sps_size, int* width, int* height)
{
    sps_info_struct sps_info = {0};
    
    parseh264_sps(&sps_info, sps, sps_size);
    
    *width = (sps_info.pic_width_in_mbs_minus1 + 1) * 16 - sps_info.frame_crop_left_offset * 2 - sps_info.frame_crop_right_offset * 2;
    *height = ((2 - sps_info.frame_mbs_only_flag) * (sps_info.pic_height_in_map_units_minus1 + 1) * 16) - sps_info.frame_crop_top_offset * 2 - sps_info.frame_crop_bottom_offset * 2;
    
}


////////////////////////////////////////////////

#define HEVC_MAX_SUB_LAYERS 7

typedef struct 
{
    int chroma_format_idc;
    int separate_colour_plane_flag;
    uint64_t pic_width_in_luma_samples;
    uint64_t pic_height_in_luma_samples;
    int conformance_window_flag;
    uint64_t conf_win_left_offset;
    uint64_t conf_win_right_offset;
    uint64_t conf_win_top_offset;
    uint64_t conf_win_bottom_offset;
}h265_sps_info_struct;

static void parseh265_sps(h265_sps_info_struct* sps_info, uint8_t* sps_data, uint32_t sps_size)
{
    if(sps_info == NULL)
        return;
    memset(sps_info, 0, sizeof(h265_sps_info_struct));
    
    nal_bitstream bs;
    nal_bs_init(&bs, sps_data, sps_size);
    
    nal_bs_read(&bs, 4); //sps_video_parameter_set_id
    int sps_max_sub_layers_minus1 = nal_bs_read(&bs, 3); //sps_max_sub_layers_minus1
    nal_bs_read(&bs, 1); //sps_temporal_id_nesting_flag
    

    uint8_t sub_layer_profile_present_flag[HEVC_MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[HEVC_MAX_SUB_LAYERS];
    nal_bs_read(&bs, 2);
    nal_bs_read(&bs, 1);
    nal_bs_read(&bs, 5);
    nal_bs_read(&bs, 32);
    nal_bs_read(&bs, 48);
    nal_bs_read(&bs, 8);
    for(int i=0;i<sps_max_sub_layers_minus1;i++){
        sub_layer_profile_present_flag[i] = nal_bs_read(&bs, 1);
        sub_layer_level_present_flag[i]   = nal_bs_read(&bs, 1);
    }
    if (sps_max_sub_layers_minus1 > 0){
        for (int i = sps_max_sub_layers_minus1; i < 8; i++)
            nal_bs_read(&bs, 2); // reserved_zero_2bits[i]
    }
    for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            nal_bs_read(&bs, 32);
            nal_bs_read(&bs, 32);
            nal_bs_read(&bs, 24);
        }
        if (sub_layer_level_present_flag[i]){
            nal_bs_read(&bs, 8);
        }
    }

    nal_bs_read_ue(&bs); //sps_seq_parameter_set_id
    sps_info->chroma_format_idc = nal_bs_read_ue(&bs);
    if (sps_info->chroma_format_idc == 3){
        sps_info->separate_colour_plane_flag = nal_bs_read(&bs, 1);
    }

    sps_info->pic_width_in_luma_samples = nal_bs_read_ue(&bs);
    sps_info->pic_height_in_luma_samples = nal_bs_read_ue(&bs);
    sps_info->conformance_window_flag = nal_bs_read(&bs, 1);
    if(sps_info->conformance_window_flag){
        sps_info->conf_win_left_offset = nal_bs_read_ue(&bs);
        sps_info->conf_win_right_offset = nal_bs_read_ue(&bs);
        sps_info->conf_win_top_offset = nal_bs_read_ue(&bs);
        sps_info->conf_win_bottom_offset = nal_bs_read_ue(&bs);
    }
}

static void get_resolution_from_h265_sps(uint8_t* sps, uint32_t sps_size, int* width, int* height)
{
    h265_sps_info_struct sps_info = {0};
    
    parseh265_sps(&sps_info, sps, sps_size);
    
    int w = sps_info.pic_width_in_luma_samples;
    int h = sps_info.pic_height_in_luma_samples;
    int sub_width_c = ((1==sps_info.chroma_format_idc)||(2 == sps_info.chroma_format_idc))&&(0 == sps_info.separate_colour_plane_flag)?2:1;
    int sub_height_c = (1==sps_info.chroma_format_idc)&&(0 == sps_info.separate_colour_plane_flag)?2:1;

    w -= (sub_width_c * sps_info.conf_win_right_offset + sub_width_c * sps_info.conf_win_left_offset);
    h -=  (sub_height_c * sps_info.conf_win_bottom_offset + sub_height_c * sps_info.conf_win_top_offset);
    
    *width = w;
    *height = h;
}

#endif