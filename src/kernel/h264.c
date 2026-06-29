#include "h264.h"

typedef struct {
    const uint8_t *data;
    uint32_t size;
    uint32_t pos;
    uint32_t bitbuf;
    uint32_t bitcnt;
    uint32_t zeros;
    uint8_t error;
} H264Bits;

typedef struct {
    uint16_t code;
    uint8_t len;
    uint8_t total_coeff;
    uint8_t trailing_ones;
} H264CoeffToken;

typedef struct {
    uint16_t code;
    uint8_t len;
    uint8_t value;
} H264Vlc;

static void h264_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) {
        b[i] = 0;
    }
}

static void h264_copy_bytes(uint8_t *dst, const uint8_t *src, uint32_t n)
{
    while (n >= 8U) {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
        dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
        dst += 8;
        src += 8;
        n -= 8U;
    }
    while (n > 0) {
        *dst++ = *src++;
        n--;
    }
}

static void h264_copy_str(char *dst, uint32_t max, const char *src)
{
    uint32_t i = 0;
    if (max == 0) {
        return;
    }
    while (src[i] != 0 && i + 1 < max) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

const char *h264_status_text(uint32_t status)
{
    switch (status) {
    case H264_OK: return "OK";
    case H264_ERR_NO_SPS: return "missing SPS";
    case H264_ERR_NO_PPS: return "missing PPS";
    case H264_ERR_BAD_BITSTREAM: return "bad H.264 bitstream";
    case H264_ERR_UNSUPPORTED: return "unsupported H.264 feature";
    case H264_ERR_NO_SLICE: return "no VCL slice found";
    default: return "unknown";
    }
}

const char *h264_slice_type_name(uint32_t slice_type)
{
    switch (slice_type % 5U) {
    case 0: return "P";
    case 1: return "B";
    case 2: return "I";
    case 3: return "SP";
    case 4: return "SI";
    default: return "?";
    }
}

static void bits_init(H264Bits *b, const uint8_t *data, uint32_t size)
{
    b->data = data;
    b->size = size;
    b->pos = 0;
    b->bitbuf = 0;
    b->bitcnt = 0;
    b->zeros = 0;
    b->error = 0;
}

static int bits_next_byte(H264Bits *b)
{
    while (b->pos < b->size) {
        uint8_t v = b->data[b->pos++];
        if (b->zeros >= 2 && v == 0x03U) {
            b->zeros = 0;
            continue;
        }
        if (v == 0) {
            b->zeros++;
        } else {
            b->zeros = 0;
        }
        return (int)v;
    }
    b->error = 1;
    return -1;
}

static uint32_t bits_read(H264Bits *b, uint32_t n)
{
    if (n > 24) {
        b->error = 1;
        return 0;
    }
    while (b->bitcnt < n) {
        int by = bits_next_byte(b);
        if (by < 0) {
            return 0;
        }
        b->bitbuf = (b->bitbuf << 8) | (uint32_t)by;
        b->bitcnt += 8;
    }
    b->bitcnt -= n;
    return (b->bitbuf >> b->bitcnt) & ((1U << n) - 1U);
}

static uint32_t bits_ue(H264Bits *b)
{
    uint32_t zeros = 0;
    while (!b->error && bits_read(b, 1) == 0) {
        zeros++;
        if (zeros > 31) {
            b->error = 1;
            return 0;
        }
    }
    if (zeros == 0) {
        return 0;
    }
    return ((1U << zeros) - 1U) + bits_read(b, zeros);
}

static int32_t bits_se(H264Bits *b)
{
    uint32_t ue = bits_ue(b);
    int32_t v = (int32_t)((ue + 1U) >> 1);
    return (ue & 1U) ? v : -v;
}

/* Authoritative CAVLC VLC tables (H.264 Tables 9-5, 9-7/9-8, 9-9/9-10),
 * matching the reference (FFmpeg) arrays. coeff_token is indexed
 * [nC-range][TotalCoeff*4 + TrailingOnes]; the nC ranges are 0:0<=nC<2,
 * 1:2<=nC<4, 2:4<=nC<8, 3:8<=nC. total_zeros is [TotalCoeff-1][total_zeros];
 * run is [min(zerosLeft,7)-1][run_before]. A len of 0 marks an unused slot. */
static const uint8_t coeff_token_len[4][4 * 17] = {
{1,0,0,0, 6,2,0,0, 8,6,3,0, 9,8,7,5, 10,9,8,6, 11,10,9,7, 13,11,10,8, 13,13,11,9,
 13,13,13,10, 14,14,13,11, 14,14,14,13, 15,15,14,14, 15,15,15,14, 16,15,15,15,
 16,16,16,15, 16,16,16,16, 16,16,16,16},
{2,0,0,0, 6,2,0,0, 6,5,3,0, 7,6,6,4, 8,6,6,4, 8,7,7,5, 9,8,8,6, 11,9,9,6,
 11,11,11,7, 12,11,11,9, 12,12,12,11, 12,12,12,11, 13,13,13,12, 13,13,13,13,
 13,14,13,13, 14,14,14,13, 14,14,14,14},
{4,0,0,0, 6,4,0,0, 6,5,4,0, 6,5,5,4, 7,5,5,4, 7,5,5,4, 7,6,6,4, 7,6,6,4,
 8,7,7,5, 8,8,7,6, 9,8,8,7, 9,9,8,8, 9,9,9,8, 10,9,9,9, 10,10,10,10,
 10,10,10,10, 10,10,10,10},
{6,0,0,0, 6,6,0,0, 6,6,6,0, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
 6,6,6,6}};
static const uint8_t coeff_token_bits[4][4 * 17] = {
{1,0,0,0, 5,1,0,0, 7,4,1,0, 7,6,5,3, 7,6,5,3, 7,6,5,4, 15,6,5,4, 11,14,5,4,
 8,10,13,4, 15,14,9,4, 11,10,13,12, 15,14,9,12, 11,10,13,8, 15,1,9,12,
 11,14,13,8, 7,10,9,12, 4,6,5,8},
{3,0,0,0, 11,2,0,0, 7,7,3,0, 7,10,9,5, 7,6,5,4, 4,6,5,6, 7,6,5,8, 15,6,5,4,
 11,14,13,4, 15,10,9,4, 11,14,13,12, 8,10,9,8, 15,14,13,12, 11,10,9,12,
 7,11,6,8, 9,8,10,1, 7,6,5,4},
{15,0,0,0, 15,14,0,0, 11,15,13,0, 8,12,14,12, 15,10,11,11, 11,8,9,10, 9,14,13,9,
 8,10,9,8, 15,14,13,13, 11,14,10,12, 15,10,13,12, 11,14,9,12, 8,10,13,8,
 13,7,9,12, 9,12,11,10, 5,8,7,6, 1,4,3,2},
{3,0,0,0, 0,1,0,0, 4,5,6,0, 8,9,10,11, 12,13,14,15, 16,17,18,19, 20,21,22,23,
 24,25,26,27, 28,29,30,31, 32,33,34,35, 36,37,38,39, 40,41,42,43, 44,45,46,47,
 48,49,50,51, 52,53,54,55, 56,57,58,59, 60,61,62,63}};
static const uint8_t total_zeros_len[15][16] = {
{1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},
{3,3,3,3,3,4,4,4,4,5,5,6,6,6,6,0},
{4,3,3,3,4,4,3,3,4,5,5,6,5,6,0,0},
{5,3,4,4,3,3,3,4,3,4,5,5,5,0,0,0},
{4,4,4,3,3,3,3,3,4,5,4,5,0,0,0,0},
{6,5,3,3,3,3,3,3,4,3,6,0,0,0,0,0},
{6,5,3,3,3,2,3,4,3,6,0,0,0,0,0,0},
{6,4,5,3,2,2,3,3,6,0,0,0,0,0,0,0},
{6,6,4,2,2,3,2,5,0,0,0,0,0,0,0,0},
{5,5,3,2,2,2,4,0,0,0,0,0,0,0,0,0},
{4,4,3,3,1,3,0,0,0,0,0,0,0,0,0,0},
{4,4,2,1,3,0,0,0,0,0,0,0,0,0,0,0},
{3,3,1,2,0,0,0,0,0,0,0,0,0,0,0,0},
{2,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
static const uint8_t total_zeros_bits[15][16] = {
{1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
{7,6,5,4,3,5,4,3,2,3,2,3,2,1,0,0},
{5,7,6,5,4,3,4,3,2,3,2,1,1,0,0,0},
{3,7,5,4,6,5,4,3,3,2,2,1,0,0,0,0},
{5,4,3,7,6,5,4,3,2,1,1,0,0,0,0,0},
{1,1,7,6,5,4,3,2,1,1,0,0,0,0,0,0},
{1,1,5,4,3,3,2,1,1,0,0,0,0,0,0,0},
{1,1,1,3,3,2,2,1,0,0,0,0,0,0,0,0},
{1,0,1,3,2,1,1,1,0,0,0,0,0,0,0,0},
{1,0,1,3,2,1,1,0,0,0,0,0,0,0,0,0},
{0,1,1,2,1,3,0,0,0,0,0,0,0,0,0,0},
{0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0},
{0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
{0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
static const uint8_t run_len[7][16] = {
{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0},
{2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0},
{2,2,2,3,3,0,0,0,0,0,0,0,0,0,0,0},
{2,2,3,3,3,3,0,0,0,0,0,0,0,0,0,0},
{2,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0},
{3,3,3,3,3,3,3,4,5,6,7,8,9,10,11,0}};
static const uint8_t run_bits[7][16] = {
{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
{3,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
{3,2,3,2,1,0,0,0,0,0,0,0,0,0,0,0},
{3,0,1,3,2,5,4,0,0,0,0,0,0,0,0,0},
{7,6,5,4,3,2,1,1,1,1,1,1,1,1,1,0}};
static const uint8_t chroma_dc_coeff_token_len[4 * 5] = {
    2,0,0,0, 6,1,0,0, 6,6,3,0, 6,7,7,6, 6,8,8,7
};
static const uint8_t chroma_dc_coeff_token_bits[4 * 5] = {
    1,0,0,0, 7,1,0,0, 4,6,1,0, 3,3,2,5, 2,3,2,0
};
static const uint8_t chroma_dc_total_zeros_len[3][4] = {
    {1,2,3,3}, {1,2,2,0}, {1,1,0,0}
};
static const uint8_t chroma_dc_total_zeros_bits[3][4] = {
    {1,1,1,0}, {1,1,0,0}, {1,0,0,0}
};

/* Generic prefix-code lookup: read bits one at a time, match against the given
 * (len,bits) arrays of `count` entries; returns the index, or count on error. */
static uint32_t decode_vlc_table(H264Bits *b, const uint8_t *lens,
                                 const uint8_t *vals, uint32_t count)
{
    uint32_t code = 0;
    for (uint32_t len = 1; len <= 16; len++) {
        code = (code << 1) | bits_read(b, 1);
        if (b->error) {
            return count;
        }
        for (uint32_t i = 0; i < count; i++) {
            if (lens[i] == len && vals[i] == code) {
                return i;
            }
        }
    }
    b->error = 1;
    return count;
}

static uint32_t read_coeff_token(H264Bits *b, int32_t nc,
                                 uint32_t *total_coeff, uint32_t *trailing_ones)
{
    uint32_t tab = (nc >= 8) ? 3U : (nc >= 4) ? 2U : (nc >= 2) ? 1U : 0U;
    uint32_t idx = decode_vlc_table(b, coeff_token_len[tab], coeff_token_bits[tab], 4U * 17U);
    if (b->error || idx >= 4U * 17U) {
        b->error = 1;
        return 0;
    }
    *total_coeff = idx / 4U;
    *trailing_ones = idx % 4U;
    return 1;
}

static uint32_t read_total_zeros(H264Bits *b, uint32_t total_coeff,
                                 uint32_t *total_zeros)
{
    if (total_coeff == 0 || total_coeff > 15U) {
        *total_zeros = 0;
        return total_coeff == 16U;
    }
    uint32_t idx = decode_vlc_table(b, total_zeros_len[total_coeff - 1],
                                    total_zeros_bits[total_coeff - 1], 16U);
    if (b->error || idx >= 16U) {
        b->error = 1;
        return 0;
    }
    *total_zeros = idx;
    return 1;
}

static uint32_t read_run_before(H264Bits *b, uint32_t zeros_left,
                                uint32_t *run_before)
{
    if (zeros_left == 0) {
        *run_before = 0;
        return 1;
    }
    uint32_t row = (zeros_left >= 7U ? 7U : zeros_left) - 1U;
    uint32_t idx = decode_vlc_table(b, run_len[row], run_bits[row], 16U);
    if (b->error || idx >= 16U) {
        b->error = 1;
        return 0;
    }
    *run_before = idx;
    return 1;
}

static uint32_t read_chroma_dc_coeff_token(H264Bits *b, uint32_t *total_coeff,
                                           uint32_t *trailing_ones)
{
    uint32_t idx = decode_vlc_table(b, chroma_dc_coeff_token_len,
                                    chroma_dc_coeff_token_bits, 4U * 5U);
    if (b->error || idx >= 4U * 5U) {
        b->error = 1;
        return 0;
    }
    *total_coeff = idx / 4U;
    *trailing_ones = idx % 4U;
    return 1;
}

static uint32_t read_chroma_dc_total_zeros(H264Bits *b, uint32_t total_coeff,
                                           uint32_t *total_zeros)
{
    if (total_coeff == 0 || total_coeff >= 4U) {
        *total_zeros = 0;
        return total_coeff == 4U;
    }
    uint32_t idx = decode_vlc_table(b, chroma_dc_total_zeros_len[total_coeff - 1],
                                    chroma_dc_total_zeros_bits[total_coeff - 1], 4U);
    if (b->error || idx >= 4U) {
        b->error = 1;
        return 0;
    }
    *total_zeros = idx;
    return 1;
}


static uint32_t read_level_suffix(H264Bits *b, uint32_t suffix_length)
{
    return suffix_length == 0 ? 0 : bits_read(b, suffix_length);
}

static uint32_t decode_cavlc_levels(H264Bits *b, uint32_t total_coeff,
                                    uint32_t trailing_ones, int16_t *levels)
{
    uint32_t level_count = 0;
    for (uint32_t i = 0; i < trailing_ones; i++) {
        uint32_t sign = bits_read(b, 1); /* trailing_ones_sign_flag */
        levels[level_count++] = sign ? -1 : 1;
    }
    uint32_t suffix_length = (total_coeff > 10U && trailing_ones < 3U) ? 1U : 0U;
    for (uint32_t i = trailing_ones; i < total_coeff; i++) {
        uint32_t level_prefix = 0;
        while (!b->error && bits_read(b, 1) == 0) {
            level_prefix++;
            if (level_prefix > 31U) {
                b->error = 1;
                return 0;
            }
        }
        uint32_t level_suffix_size = suffix_length;
        if (level_prefix == 14U && suffix_length == 0U) {
            level_suffix_size = 4;
        } else if (level_prefix >= 15U) {
            level_suffix_size = level_prefix - 3U;
        }
        uint32_t level_suffix = read_level_suffix(b, level_suffix_size);
        uint32_t level_code = (level_prefix << suffix_length) + level_suffix;
        if (level_prefix >= 15U && suffix_length == 0U) {
            level_code += 15U;
        }
        if (level_prefix >= 15U) {
            level_code += (1U << (level_prefix - 3U)) - 4096U;
        }
        if (i == trailing_ones && trailing_ones < 3U) {
            level_code += 2U;
        }
        int32_t level = (level_code & 1U) ?
                        -(int32_t)((level_code + 1U) >> 1) :
                        (int32_t)((level_code + 2U) >> 1);
        if (level < -32768 || level > 32767) {
            b->error = 1;
            return 0;
        }
        levels[level_count++] = (int16_t)level;
        if (suffix_length == 0U) {
            suffix_length = 1U;
        }
        uint32_t abs_level = level < 0 ? (uint32_t)(-level) : (uint32_t)level;
        if (abs_level > (3U << (suffix_length - 1U)) && suffix_length < 6U) {
            suffix_length++;
        }
    }
    return !b->error;
}

static uint32_t skip_residual_cavlc(H264Bits *b, int32_t nc,
                                    uint32_t max_coeff, uint32_t chroma_dc,
                                    uint32_t *tc_out)
{
    uint32_t total_coeff = 0, trailing_ones = 0;
    if (chroma_dc) {
        if (!read_chroma_dc_coeff_token(b, &total_coeff, &trailing_ones)) {
            return 0;
        }
    } else if (!read_coeff_token(b, nc, &total_coeff, &trailing_ones)) {
        return 0;
    }
    if (trailing_ones > 3U || trailing_ones > total_coeff || total_coeff > max_coeff) {
        b->error = 1;
        return 0;
    }
    if (tc_out) {
        *tc_out = total_coeff;
    }
    if (total_coeff == 0) {
        return 1;
    }
    int16_t levels[16];
    for (uint32_t i = 0; i < 16; i++) levels[i] = 0;
    if (!decode_cavlc_levels(b, total_coeff, trailing_ones, levels)) {
        return 0;
    }
    uint32_t total_zeros = 0;
    if (total_coeff < max_coeff) {
        if (chroma_dc) {
            if (!read_chroma_dc_total_zeros(b, total_coeff, &total_zeros)) {
                return 0;
            }
        } else if (!read_total_zeros(b, total_coeff, &total_zeros)) {
            return 0;
        }
    }
    uint32_t zeros_left = total_zeros;
    for (uint32_t i = 0; i + 1U < total_coeff; i++) {
        uint32_t run_before = 0;
        if (zeros_left > 0) {
            if (!read_run_before(b, zeros_left, &run_before)) {
                return 0;
            }
            if (run_before > zeros_left) {
                b->error = 1;
                return 0;
            }
            zeros_left -= run_before;
        }
    }
    return !b->error;
}



static uint32_t h264_cbp_intra(uint32_t code_num)
{
    static const uint8_t map[48] = {
        47, 31, 15,  0, 23, 27, 29, 30,
         7, 11, 13, 14, 39, 43, 45, 46,
        16,  3,  5, 10, 12, 19, 21, 26,
        28, 35, 37, 42, 44,  1,  2,  4,
         8, 17, 18, 20, 24,  6,  9, 22,
        25, 32, 33, 34, 36, 40, 38, 41
    };
    return code_num < 48U ? map[code_num] : 0xFFFFFFFFU;
}

/* Geometry of the 16 luma 4x4 blocks within a macroblock (raster pixel coords
 * of each block in luma4x4BlkIdx scan order), and the inverse map from a
 * 4x4-grid cell (by,bx) back to the scan index. */
static const uint8_t h264_blk_x[16] = {0,4,0,4,8,12,8,12,0,4,0,4,8,12,8,12};
static const uint8_t h264_blk_y[16] = {0,0,4,4,0,0,4,4,8,8,12,12,8,8,12,12};
static const uint8_t h264_xy2blk[4][4] = {
    {0,1,4,5}, {2,3,6,7}, {8,9,12,13}, {10,11,14,15}
};
static const uint8_t h264_scan4x4[16] = {
    0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15
};

static int32_t h264_clip_u8(int32_t v);

/* Full CAVLC residual decode for one 4x4 luma block at the given nC context.
 * Output coefficients are in raster order. */
static uint32_t decode_residual_4x4(H264Bits *b, int32_t nc, int16_t coeff[16],
                                    uint32_t *tc_out, uint32_t *t1_out,
                                    uint32_t *tz_out, uint32_t *runs_out)
{
    for (uint32_t i = 0; i < 16; i++) {
        coeff[i] = 0;
    }
    *tc_out = 0; *t1_out = 0; *tz_out = 0; *runs_out = 0;
    uint32_t total_coeff = 0, trailing_ones = 0;
    if (!read_coeff_token(b, nc, &total_coeff, &trailing_ones)) {
        return 0;
    }
    *tc_out = total_coeff;
    *t1_out = trailing_ones;
    if (trailing_ones > 3U || trailing_ones > total_coeff || total_coeff > 16U) {
        b->error = 1;
        return 0;
    }
    if (total_coeff == 0) {
        return 1;
    }
    int16_t levels[16];
    for (uint32_t i = 0; i < 16; i++) {
        levels[i] = 0;
    }
    if (!decode_cavlc_levels(b, total_coeff, trailing_ones, levels)) {
        return 0;
    }
    uint32_t total_zeros = 0;
    if (total_coeff < 16U) {
        if (!read_total_zeros(b, total_coeff, &total_zeros)) {
            return 0;
        }
    }
    *tz_out = total_zeros;
    uint32_t runs_before[16];
    for (uint32_t i = 0; i < 16; i++) {
        runs_before[i] = 0;
    }
    uint32_t zeros_left = total_zeros, runs = 0;
    for (uint32_t i = 0; i + 1U < total_coeff; i++) {
        uint32_t run_before = 0;
        if (zeros_left > 0) {
            if (!read_run_before(b, zeros_left, &run_before)) {
                return 0;
            }
            if (run_before > zeros_left) {
                b->error = 1;
                return 0;
            }
            zeros_left -= run_before;
            runs_before[i] = run_before;
            runs++;
        }
    }
    /* The lowest-frequency coefficient takes the remaining zerosLeft (H.264
     * 9.2.4); omitting this misplaces every coefficient when zerosLeft > 0. */
    runs_before[total_coeff - 1] = zeros_left;
    *runs_out = runs;
    int32_t coeff_num = -1;
    for (int32_t i = (int32_t)total_coeff - 1; i >= 0; i--) {
        coeff_num += (int32_t)runs_before[i] + 1;
        if (coeff_num < 0 || coeff_num >= 16) {
            b->error = 1;
            return 0;
        }
        coeff[h264_scan4x4[(uint32_t)coeff_num]] = levels[i];
    }
    return !b->error;
}

static uint32_t decode_residual_4x4_ac(H264Bits *b, int32_t nc, int16_t coeff[16],
                                       uint32_t *tc_out)
{
    for (uint32_t i = 0; i < 16; i++) {
        coeff[i] = 0;
    }
    *tc_out = 0;
    uint32_t total_coeff = 0, trailing_ones = 0;
    if (!read_coeff_token(b, nc, &total_coeff, &trailing_ones)) {
        return 0;
    }
    *tc_out = total_coeff;
    if (trailing_ones > 3U || trailing_ones > total_coeff || total_coeff > 15U) {
        b->error = 1;
        return 0;
    }
    if (total_coeff == 0) {
        return 1;
    }
    int16_t levels[16];
    for (uint32_t i = 0; i < 16; i++) {
        levels[i] = 0;
    }
    if (!decode_cavlc_levels(b, total_coeff, trailing_ones, levels)) {
        return 0;
    }
    uint32_t total_zeros = 0;
    if (total_coeff < 15U) {
        if (!read_total_zeros(b, total_coeff, &total_zeros)) {
            return 0;
        }
        if (total_zeros > 15U - total_coeff) {
            b->error = 1;
            return 0;
        }
    }
    uint32_t runs_before[16];
    for (uint32_t i = 0; i < 16; i++) {
        runs_before[i] = 0;
    }
    uint32_t zeros_left = total_zeros;
    for (uint32_t i = 0; i + 1U < total_coeff; i++) {
        uint32_t run_before = 0;
        if (zeros_left > 0) {
            if (!read_run_before(b, zeros_left, &run_before)) {
                return 0;
            }
            if (run_before > zeros_left) {
                b->error = 1;
                return 0;
            }
            zeros_left -= run_before;
            runs_before[i] = run_before;
        }
    }
    runs_before[total_coeff - 1] = zeros_left;
    int32_t coeff_num = -1;
    for (int32_t i = (int32_t)total_coeff - 1; i >= 0; i--) {
        coeff_num += (int32_t)runs_before[i] + 1;
        if (coeff_num < 0 || coeff_num >= 15) {
            b->error = 1;
            return 0;
        }
        coeff[h264_scan4x4[(uint32_t)coeff_num + 1U]] = levels[i];
    }
    return !b->error;
}

/* Inverse quantization + 4x4 inverse integer transform (H.264 8.5.12). */
static void dequant_4x4(const int16_t coeff[16], int32_t qp, int32_t d[16])
{
    static const int32_t dequant_coef[6][16] = {
        {10, 13, 10, 13, 13, 16, 13, 16, 10, 13, 10, 13, 13, 16, 13, 16},
        {11, 14, 11, 14, 14, 18, 14, 18, 11, 14, 11, 14, 14, 18, 14, 18},
        {13, 16, 13, 16, 16, 20, 16, 20, 13, 16, 13, 16, 16, 20, 16, 20},
        {14, 18, 14, 18, 18, 23, 18, 23, 14, 18, 14, 18, 18, 23, 18, 23},
        {16, 20, 16, 20, 20, 25, 20, 25, 16, 20, 16, 20, 20, 25, 20, 25},
        {18, 23, 18, 23, 23, 29, 23, 29, 18, 23, 18, 23, 23, 29, 23, 29},
    };
    int32_t qp_per = qp / 6, qp_rem = qp % 6;
    for (int32_t k = 0; k < 16; k++) {
        int32_t sc = dequant_coef[qp_rem][k] * 16;
        int32_t v = coeff[k];
        if (qp_per >= 4) {
            d[k] = (v * sc) << (qp_per - 4);
        } else {
            d[k] = (v * sc + (1 << (3 - qp_per))) >> (4 - qp_per);
        }
    }
}

static void idct_4x4_dequantized(const int32_t d[16], int32_t res[16])
{
    int32_t f[16];
    for (int32_t i = 0; i < 4; i++) {
        int32_t d0 = d[i*4+0], d1 = d[i*4+1], d2 = d[i*4+2], d3 = d[i*4+3];
        int32_t e0 = d0 + d2, e1 = d0 - d2;
        int32_t e2 = (d1 >> 1) - d3, e3 = d1 + (d3 >> 1);
        f[i*4+0] = e0 + e3; f[i*4+1] = e1 + e2;
        f[i*4+2] = e1 - e2; f[i*4+3] = e0 - e3;
    }
    for (int32_t j = 0; j < 4; j++) {
        int32_t f0 = f[0*4+j], f1 = f[1*4+j], f2 = f[2*4+j], f3 = f[3*4+j];
        int32_t g0 = f0 + f2, g1 = f0 - f2;
        int32_t g2 = (f1 >> 1) - f3, g3 = f1 + (f3 >> 1);
        res[0*4+j] = (g0 + g3 + 32) >> 6;
        res[1*4+j] = (g1 + g2 + 32) >> 6;
        res[2*4+j] = (g1 - g2 + 32) >> 6;
        res[3*4+j] = (g0 - g3 + 32) >> 6;
    }
}

static void dequant_idct_4x4(const int16_t coeff[16], int32_t qp, int32_t res[16])
{
    int32_t d[16];
    dequant_4x4(coeff, qp, d);
    idct_4x4_dequantized(d, res);
}

static void i16x16_luma_dc_dequant_idct(const int16_t dc_coeff[16],
                                        int32_t qp, int32_t dc_out[16])
{
    static const int32_t dequant0[6] = {10, 11, 13, 14, 16, 18};
    int32_t temp[16];
    for (int32_t i = 0; i < 4; i++) {
        int32_t z0 = dc_coeff[4*i+0] + dc_coeff[4*i+2];
        int32_t z1 = dc_coeff[4*i+0] - dc_coeff[4*i+2];
        int32_t z2 = dc_coeff[4*i+1] - dc_coeff[4*i+3];
        int32_t z3 = dc_coeff[4*i+1] + dc_coeff[4*i+3];
        temp[4*i+0] = z0 + z3;
        temp[4*i+1] = z1 + z2;
        temp[4*i+2] = z1 - z2;
        temp[4*i+3] = z0 - z3;
    }
    int32_t qp_per = qp / 6, qp_rem = qp % 6;
    int32_t qmul = (dequant0[qp_rem] * 16) << (qp_per + 2);
    for (int32_t i = 0; i < 4; i++) {
        int32_t z0 = temp[4*0+i] + temp[4*2+i];
        int32_t z1 = temp[4*0+i] - temp[4*2+i];
        int32_t z2 = temp[4*1+i] - temp[4*3+i];
        int32_t z3 = temp[4*1+i] + temp[4*3+i];
        dc_out[h264_xy2blk[0][i]] = ((z0 + z3) * qmul + 128) >> 8;
        dc_out[h264_xy2blk[1][i]] = ((z1 + z2) * qmul + 128) >> 8;
        dc_out[h264_xy2blk[2][i]] = ((z1 - z2) * qmul + 128) >> 8;
        dc_out[h264_xy2blk[3][i]] = ((z0 - z3) * qmul + 128) >> 8;
    }
}

static void dequant_idct_4x4_i16(const int16_t ac_coeff[16],
                                 int32_t qp, int32_t dc_value, int32_t res[16])
{
    int32_t d[16];
    dequant_4x4(ac_coeff, qp, d);
    d[0] = dc_value;
    idct_4x4_dequantized(d, res);
}

static int32_t h264_clip_u8(int32_t v)
{
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

static uint32_t h264_decoded_span(const uint8_t *dec, uint32_t stride,
                                  uint32_t x, uint32_t y, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        if (!dec[y * stride + x + i]) {
            return 0;
        }
    }
    return 1;
}

static uint32_t predict4x4_region(const uint8_t *luma, const uint8_t *dec,
                                  uint32_t stride, uint32_t width,
                                  uint32_t x, uint32_t y, uint32_t mode,
                                  int32_t pred[16])
{
    uint32_t upR = (y > 0 && x + 3 < width &&
                    h264_decoded_span(dec, stride, x, y - 1, 4));
    uint32_t leftR = (x > 0);
    if (leftR) {
        for (uint32_t i = 0; i < 4; i++) {
            if (!dec[(y + i) * stride + x - 1]) {
                leftR = 0;
                break;
            }
        }
    }
    uint32_t ulR = (x > 0 && y > 0 && dec[(y - 1) * stride + x - 1]);
    if ((mode == 0U || mode == 3U || mode == 7U) && !upR) return 0;
    if ((mode == 1U || mode == 8U) && !leftR) return 0;
    if ((mode == 4U || mode == 5U || mode == 6U) && (!upR || !leftR || !ulR)) return 0;

    int32_t T[8], L[4], TL = 0;
    for (uint32_t i = 0; i < 8; i++) T[i] = 0;
    for (uint32_t i = 0; i < 4; i++) L[i] = 0;
    if (upR) {
        for (uint32_t i = 0; i < 4; i++) T[i] = luma[(y - 1) * stride + x + i];
        uint32_t tr = (x + 7 < width && h264_decoded_span(dec, stride, x + 4, y - 1, 4));
        for (uint32_t i = 4; i < 8; i++) {
            T[i] = tr ? luma[(y - 1) * stride + x + i] : T[3];
        }
    }
    if (leftR) {
        for (uint32_t i = 0; i < 4; i++) L[i] = luma[(y + i) * stride + x - 1];
    }
    if (ulR) {
        TL = luma[(y - 1) * stride + x - 1];
    }
#define PA(k) ((k) >= 0 ? T[(k)] : TL)
#define PB(k) ((k) >= 0 ? L[(k)] : TL)
    for (int32_t yy = 0; yy < 4; yy++) {
        for (int32_t xx = 0; xx < 4; xx++) {
            int32_t v = 0, z;
            switch (mode) {
            case 0: v = T[xx]; break;
            case 1: v = L[yy]; break;
            case 2:
                if (upR && leftR) v = (T[0]+T[1]+T[2]+T[3]+L[0]+L[1]+L[2]+L[3]+4) >> 3;
                else if (upR) v = (T[0]+T[1]+T[2]+T[3]+2) >> 2;
                else if (leftR) v = (L[0]+L[1]+L[2]+L[3]+2) >> 2;
                else v = 128;
                break;
            case 3:
                if (xx == 3 && yy == 3) v = (T[6]+3*T[7]+2) >> 2;
                else v = (T[xx+yy]+2*T[xx+yy+1]+T[xx+yy+2]+2) >> 2;
                break;
            case 4:
                if (xx > yy) v = (PA(xx-yy-2)+2*PA(xx-yy-1)+PA(xx-yy)+2) >> 2;
                else if (xx < yy) v = (PB(yy-xx-2)+2*PB(yy-xx-1)+PB(yy-xx)+2) >> 2;
                else v = (T[0]+2*TL+L[0]+2) >> 2;
                break;
            case 5:
                z = 2*xx - yy;
                if (z >= 0 && (z & 1) == 0) v = (PA(xx-(yy>>1)-1)+PA(xx-(yy>>1))+1) >> 1;
                else if (z >= 0) v = (PA(xx-(yy>>1)-2)+2*PA(xx-(yy>>1)-1)+PA(xx-(yy>>1))+2) >> 2;
                else if (z == -1) v = (L[0]+2*TL+T[0]+2) >> 2;
                else v = (PB(yy-1)+2*PB(yy-2)+PB(yy-3)+2) >> 2;
                break;
            case 6:
                z = 2*yy - xx;
                if (z >= 0 && (z & 1) == 0) v = (PB(yy-(xx>>1)-1)+PB(yy-(xx>>1))+1) >> 1;
                else if (z >= 0) v = (PB(yy-(xx>>1)-2)+2*PB(yy-(xx>>1)-1)+PB(yy-(xx>>1))+2) >> 2;
                else if (z == -1) v = (L[0]+2*TL+T[0]+2) >> 2;
                else v = (PA(xx-1)+2*PA(xx-2)+PA(xx-3)+2) >> 2;
                break;
            case 7:
                if ((yy & 1) == 0) v = (T[xx+(yy>>1)]+T[xx+(yy>>1)+1]+1) >> 1;
                else v = (T[xx+(yy>>1)]+2*T[xx+(yy>>1)+1]+T[xx+(yy>>1)+2]+2) >> 2;
                break;
            case 8:
                z = xx + 2*yy;
                if (z < 5 && (z & 1) == 0) v = (L[yy+(xx>>1)]+L[yy+(xx>>1)+1]+1) >> 1;
                else if (z < 5) v = (L[yy+(xx>>1)]+2*L[yy+(xx>>1)+1]+L[yy+(xx>>1)+2]+2) >> 2;
                else if (z == 5) v = (L[2]+3*L[3]+2) >> 2;
                else v = L[3];
                break;
            default: return 0;
            }
            pred[yy * 4 + xx] = v;
        }
    }
#undef PA
#undef PB
    return 1;
}

static uint32_t predict16x16_region(const uint8_t *luma, const uint8_t *dec,
                                    uint32_t stride, uint32_t width,
                                    uint32_t x, uint32_t y, uint32_t mode,
                                    int32_t pred[256])
{
    uint32_t upR = (y > 0 && x + 15 < width &&
                    h264_decoded_span(dec, stride, x, y - 1, 16));
    uint32_t leftR = (x > 0);
    if (leftR) {
        for (uint32_t i = 0; i < 16; i++) {
            if (!dec[(y + i) * stride + x - 1]) {
                leftR = 0;
                break;
            }
        }
    }
    uint32_t ulR = (x > 0 && y > 0 && dec[(y - 1) * stride + x - 1]);
    int32_t T[16], L[16], TL = 0;
    for (uint32_t i = 0; i < 16; i++) {
        T[i] = 0;
        L[i] = 0;
    }
    if (upR) {
        for (uint32_t i = 0; i < 16; i++) {
            T[i] = luma[(y - 1) * stride + x + i];
        }
    }
    if (leftR) {
        for (uint32_t i = 0; i < 16; i++) {
            L[i] = luma[(y + i) * stride + x - 1];
        }
    }
    if (ulR) {
        TL = luma[(y - 1) * stride + x - 1];
    }
    switch (mode) {
    case 0:
        if (!upR) return 0;
        for (uint32_t yy = 0; yy < 16; yy++) {
            for (uint32_t xx = 0; xx < 16; xx++) pred[yy * 16 + xx] = T[xx];
        }
        return 1;
    case 1:
        if (!leftR) return 0;
        for (uint32_t yy = 0; yy < 16; yy++) {
            for (uint32_t xx = 0; xx < 16; xx++) pred[yy * 16 + xx] = L[yy];
        }
        return 1;
    case 2: {
        int32_t v = 128;
        if (upR && leftR) {
            int32_t s = 0;
            for (uint32_t i = 0; i < 16; i++) s += T[i] + L[i];
            v = (s + 16) >> 5;
        } else if (upR) {
            int32_t s = 0;
            for (uint32_t i = 0; i < 16; i++) s += T[i];
            v = (s + 8) >> 4;
        } else if (leftR) {
            int32_t s = 0;
            for (uint32_t i = 0; i < 16; i++) s += L[i];
            v = (s + 8) >> 4;
        }
        for (uint32_t i = 0; i < 256; i++) pred[i] = v;
        return 1;
    }
    case 3: {
        if (!upR || !leftR || !ulR) return 0;
        int32_t H = 0, V = 0;
        for (int32_t i = 1; i <= 8; i++) {
            H += i * (T[7 + i] - (i == 8 ? TL : T[7 - i]));
            V += i * (L[7 + i] - (i == 8 ? TL : L[7 - i]));
        }
        int32_t a = 16 * (T[15] + L[15]);
        int32_t b = (5 * H + 32) >> 6;
        int32_t c = (5 * V + 32) >> 6;
        for (int32_t yy = 0; yy < 16; yy++) {
            for (int32_t xx = 0; xx < 16; xx++) {
                pred[yy * 16 + xx] = h264_clip_u8((a + b * (xx - 7) + c * (yy - 7) + 16) >> 5);
            }
        }
        return 1;
    }
    default:
        return 0;
    }
}

typedef struct {
    uint32_t mb_type;
    uint8_t is_i16x16;
    uint32_t i16x16_pred_mode;
    uint32_t cbp_luma;
    uint32_t cbp_chroma;
    uint32_t chroma_mode;
    int32_t qp_delta;
    uint8_t modes[16];
} H264MbInfo;

static uint8_t h264_luma_scratch[H264_PROBE_MAX_REGION_SAMPLES];
static uint8_t h264_dec_scratch[H264_PROBE_MAX_REGION_SAMPLES];

static void h264_set_blocker(H264Probe *out, uint32_t status, uint32_t mb,
                             const char *detail)
{
    out->status = status;
    out->luma_blocker_mb = mb;
    h264_copy_str(out->status_detail, sizeof(out->status_detail), detail);
    h264_copy_str(out->luma_detail, sizeof(out->luma_detail), detail);
}

static uint32_t parse_intra4x4_mb_header(H264Bits *b, H264Probe *out,
                                         uint32_t mb, const uint8_t left_modes[4],
                                         const uint8_t *top_modes,
                                         H264MbInfo *mi)
{
    mi->mb_type = bits_ue(b);
    mi->is_i16x16 = 0;
    mi->i16x16_pred_mode = 2;
    mi->qp_delta = 0;
    mi->cbp_luma = 0;
    mi->cbp_chroma = 0;
    mi->chroma_mode = 0;
    for (uint32_t i = 0; i < 16; i++) mi->modes[i] = 2;

    if (mb == 0) {
        out->first_mb_type = mi->mb_type;
        out->first_mb_qp_delta = 0;
    }
    if (mi->mb_type == 25U) {
        if (mb == 0) h264_copy_str(out->first_mb_kind, sizeof(out->first_mb_kind), "I_PCM");
        h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "I_PCM macroblocks not supported");
        return 0;
    }
    if (mi->mb_type != 0U) {
        if (mi->mb_type <= 24U) {
            uint32_t v = mi->mb_type - 1U;
            mi->is_i16x16 = 1;
            mi->i16x16_pred_mode = v & 3U;
            mi->cbp_chroma = (v >> 2) % 3U;
            mi->cbp_luma = (v >= 12U) ? 15U : 0U;
            if (mb == 0) h264_copy_str(out->first_mb_kind, sizeof(out->first_mb_kind), "I_16x16");
            mi->chroma_mode = bits_ue(b);
            if (mi->chroma_mode > 3U) {
                h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "bad intra chroma prediction mode");
                return 0;
            }
            mi->qp_delta = bits_se(b);
            if (b->error) {
                h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad I_16x16 macroblock header");
                return 0;
            }
            if (mb == 0) {
                out->first_mb_header_ok = 1;
                out->first_mb_intra_chroma_pred_mode = mi->chroma_mode;
                out->first_mb_cbp_luma = mi->cbp_luma;
                out->first_mb_cbp_chroma = mi->cbp_chroma;
                out->first_mb_qp_delta = mi->qp_delta;
                out->first_mb_residual_present = 1;
            }
            return 1;
        } else {
            h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "unsupported I macroblock type");
        }
        return 0;
    }
    if (mb == 0) h264_copy_str(out->first_mb_kind, sizeof(out->first_mb_kind), "I_NxN");

    uint32_t mb_x = mb % out->mb_width;
    for (uint32_t i = 0; i < 16; i++) {
        uint32_t prev_flag = bits_read(b, 1);
        uint32_t rem = 0;
        if (!prev_flag) {
            rem = bits_read(b, 3);
        }
        uint32_t bx = h264_blk_x[i] >> 2;
        uint32_t by = h264_blk_y[i] >> 2;
        uint32_t mode_a = 2, mode_b = 2;
        uint32_t avail_a = 0, avail_b = 0;
        if (bx > 0) {
            mode_a = mi->modes[h264_xy2blk[by][bx - 1]];
            avail_a = 1;
        } else if (mb_x > 0) {
            mode_a = left_modes[by];
            avail_a = 1;
        }
        if (by > 0) {
            mode_b = mi->modes[h264_xy2blk[by - 1][bx]];
            avail_b = 1;
        } else if (mb >= out->mb_width) {
            mode_b = top_modes[mb_x * 4U + bx];
            avail_b = 1;
        }
        uint32_t pred_mode = (avail_a && avail_b) ?
                             (mode_a < mode_b ? mode_a : mode_b) : 2U;
        mi->modes[i] = (uint8_t)(prev_flag ? pred_mode :
                                 (rem < pred_mode ? rem : rem + 1U));
    }
    mi->chroma_mode = bits_ue(b);
    if (mi->chroma_mode > 3U) {
        h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "bad intra chroma prediction mode");
        return 0;
    }
    uint32_t cbp = h264_cbp_intra(bits_ue(b));
    if (cbp == 0xFFFFFFFFU) {
        h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "coded block pattern out of range");
        return 0;
    }
    mi->cbp_luma = cbp & 0x0FU;
    mi->cbp_chroma = (cbp >> 4) & 0x03U;
    if (cbp != 0) {
        mi->qp_delta = bits_se(b);
    }
    if (b->error) {
        h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad intra macroblock header");
        return 0;
    }
    if (mb == 0) {
        out->first_mb_header_ok = 1;
        out->first_block_intra4x4_mode = mi->modes[0];
        out->first_mb_intra_chroma_pred_mode = mi->chroma_mode;
        out->first_mb_cbp_luma = mi->cbp_luma;
        out->first_mb_cbp_chroma = mi->cbp_chroma;
        out->first_mb_qp_delta = mi->qp_delta;
        out->first_mb_residual_present = (cbp != 0);
    }
    return 1;
}

static void h264_finish_luma_stats(H264Probe *out)
{
    if (out->luma_samples_decoded == 0) {
        return;
    }
    int32_t sum = 0;
    uint8_t mn = 255, mx = 0;
    for (uint32_t i = 0; i < out->luma_samples_decoded; i++) {
        uint8_t p = out->luma_region[i];
        sum += p;
        if (p < mn) mn = p;
        if (p > mx) mx = p;
    }
    out->luma_region_sum = sum;
    out->luma_region_min = mn;
    out->luma_region_max = mx;
}

static void h264_fill_mb0_from_region(H264Probe *out, const uint8_t modes[16])
{
    int32_t sum = 0;
    uint8_t mn = 255, mx = 0;
    for (uint32_t y = 0; y < 16; y++) {
        for (uint32_t x = 0; x < 16; x++) {
            uint8_t p = out->luma_region[y * out->luma_region_width + x];
            out->mb0_luma[y * 16 + x] = p;
            sum += p;
            if (p < mn) mn = p;
            if (p > mx) mx = p;
        }
    }
    for (uint32_t i = 0; i < 16; i++) out->mb0_block_mode[i] = modes[i];
    out->mb0_luma_sum = sum;
    out->mb0_luma_min = mn;
    out->mb0_luma_max = mx;
    out->mb0_blocks_reconstructed = 16;
    out->mb0_luma_ok = 1;
    h264_copy_str(out->mb0_detail, sizeof(out->mb0_detail),
                  "macroblock 0: 16/16 luma 4x4 blocks reconstructed");
}

static uint32_t decode_first_luma_region(H264Bits *b, H264Probe *out)
{
    if (out->first_mb_in_slice != 0U) {
        h264_set_blocker(out, H264_ERR_UNSUPPORTED, 0, "nonzero first_mb_in_slice not supported");
        return 0;
    }
    if (out->mb_width == 0 || out->mb_width > H264_PROBE_MAX_ROW_MBS) {
        h264_set_blocker(out, H264_ERR_UNSUPPORTED, 0, "frame wider than luma probe limit");
        return 0;
    }
    if (out->mb_height == 0 || out->mb_height > H264_PROBE_MAX_MB_ROWS) {
        h264_set_blocker(out, H264_ERR_UNSUPPORTED, 0, "frame taller than luma probe limit");
        return 0;
    }

    uint32_t row_width = out->mb_width * 16U;
    uint32_t coded_height = out->mb_height * 16U;
    uint32_t region_samples = row_width * coded_height;
    uint8_t *luma = h264_luma_scratch;
    uint8_t *dec = h264_dec_scratch;
    uint8_t left_tc[4], left_modes[4];
    uint8_t left_chroma_tc[2][2];
    uint8_t top_tc[H264_PROBE_MAX_ROW_MBS * 4U];
    uint8_t top_modes[H264_PROBE_MAX_ROW_MBS * 4U];
    uint8_t top_chroma_tc[2][H264_PROBE_MAX_ROW_MBS * 2U];
    uint8_t mb0_modes[16];
    for (uint32_t i = 0; i < region_samples; i++) {
        luma[i] = 0;
        dec[i] = 0;
    }
    for (uint32_t i = 0; i < 4; i++) {
        left_tc[i] = 0;
        left_modes[i] = 2;
    }
    for (uint32_t c = 0; c < 2; c++) {
        for (uint32_t i = 0; i < 2; i++) {
            left_chroma_tc[c][i] = 0;
        }
    }
    for (uint32_t i = 0; i < 16; i++) {
        mb0_modes[i] = 2;
    }
    for (uint32_t i = 0; i < H264_PROBE_MAX_ROW_MBS * 4U; i++) {
        top_tc[i] = 0;
        top_modes[i] = 2;
    }
    for (uint32_t c = 0; c < 2; c++) {
        for (uint32_t i = 0; i < H264_PROBE_MAX_ROW_MBS * 2U; i++) {
            top_chroma_tc[c][i] = 0;
        }
    }

    int32_t qpy = 26 + out->pic_init_qp_minus26 + out->slice_qp_delta;
    if (qpy < 0 || qpy > 51) {
        h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, 0, "initial luma QP out of range");
        return 0;
    }

    uint32_t target_mbs = out->mb_width * out->mb_height;
    for (uint32_t mb = 0; mb < target_mbs; mb++) {
        uint32_t mb_x = mb % out->mb_width;
        uint32_t mb_y = mb / out->mb_width;
        uint32_t mb_x0 = mb_x * 16U;
        uint32_t mb_y0 = mb_y * 16U;
        if (mb_x == 0) {
            for (uint32_t i = 0; i < 4; i++) {
                left_tc[i] = 0;
                left_modes[i] = 2;
            }
            for (uint32_t c = 0; c < 2; c++) {
                left_chroma_tc[c][0] = 0;
                left_chroma_tc[c][1] = 0;
            }
        }

        H264MbInfo mi;
        if (!parse_intra4x4_mb_header(b, out, mb, left_modes, top_modes, &mi)) {
            break;
        }
        if (mb == 0) {
            for (uint32_t i = 0; i < 16; i++) {
                mb0_modes[i] = mi.modes[i];
            }
        }
        qpy = (qpy + mi.qp_delta + 52) % 52;
        if (mb == 0) {
            out->first_mb_qp = qpy;
        }

        uint8_t cur_tc[16];
        for (uint32_t i = 0; i < 16; i++) cur_tc[i] = 0;
        if (mi.is_i16x16) {
            int16_t dc_coeff[16];
            uint32_t dc_tc = 0, dc_t1 = 0, dc_tz = 0, dc_runs = 0;
            /* Intra16x16DCLevel nC is derived from the same neighbours as the
             * top-left 4x4 luma block (blkIdx 0), not 0 (H.264 9.2.1). Using 0
             * picks the wrong coeff_token VLC table whenever those neighbours
             * have coefficients, desyncing the whole macroblock. */
            uint32_t dc_availA = (mb_x > 0);
            uint32_t dc_availB = (mb_y > 0);
            int32_t dc_nA = dc_availA ? left_tc[0] : 0;
            int32_t dc_nB = dc_availB ? top_tc[mb_x * 4U] : 0;
            int32_t dc_nc = (dc_availA && dc_availB) ? ((dc_nA + dc_nB + 1) >> 1) :
                            dc_availA ? dc_nA : dc_availB ? dc_nB : 0;
            if (!decode_residual_4x4(b, dc_nc, dc_coeff, &dc_tc, &dc_t1, &dc_tz, &dc_runs)) {
                h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad I_16x16 luma DC residual");
            }
            int16_t ac_coeff[16][16];
            for (uint32_t blk = 0; blk < 16; blk++) {
                for (uint32_t i = 0; i < 16; i++) ac_coeff[blk][i] = 0;
            }
            if (out->status == H264_OK) {
                for (uint32_t blk = 0; blk < 16; blk++) {
                    uint32_t lx = h264_blk_x[blk], ly = h264_blk_y[blk];
                    uint32_t bx = lx >> 2, by = ly >> 2;
                    uint32_t availA = (bx > 0 || mb_x > 0);
                    uint32_t availB = (by > 0 || mb_y > 0);
                    int32_t nA = 0, nB = 0;
                    if (availA) {
                        nA = (bx > 0) ? cur_tc[h264_xy2blk[by][bx - 1]] : left_tc[by];
                        if (mb_x > 0 && bx == 0) out->cross_mb_nc_ok = 1;
                    }
                    if (availB) {
                        nB = (by > 0) ? cur_tc[h264_xy2blk[by - 1][bx]] :
                             top_tc[mb_x * 4U + bx];
                        if (mb_y > 0 && by == 0) out->top_mb_nc_ok = 1;
                    }
                    int32_t nc = (availA && availB) ? ((nA + nB + 1) >> 1) :
                                 availA ? nA : availB ? nB : 0;
                    uint32_t tc = 0;
                    if (mi.cbp_luma & (1U << (blk >> 2))) {
                        if (!decode_residual_4x4_ac(b, nc, ac_coeff[blk], &tc)) {
                            h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad I_16x16 luma AC residual");
                            break;
                        }
                    }
                    cur_tc[blk] = (uint8_t)tc;
                }
            }
            if (out->status == H264_OK) {
                int32_t dc_dequant[16];
                int32_t pred16[256];
                i16x16_luma_dc_dequant_idct(dc_coeff, qpy, dc_dequant);
                out->i16x16_dc_transform_ok = 1;
                if (!predict16x16_region(luma, dec, row_width, row_width,
                                         mb_x0, mb_y0, mi.i16x16_pred_mode, pred16)) {
                    h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "I_16x16 prediction needs unavailable neighbour");
                } else {
                    out->i16x16_luma_ok = 1;
                    out->i16x16_mbs_decoded++;
                    if (mb_x > 0) out->cross_mb_intra_ok = 1;
                    if (mb_y > 0) out->top_mb_intra_ok = 1;
                    for (uint32_t blk = 0; blk < 16; blk++) {
                        uint32_t lx = h264_blk_x[blk], ly = h264_blk_y[blk];
                        int32_t res[16];
                        dequant_idct_4x4_i16(ac_coeff[blk], qpy, dc_dequant[blk], res);
                        for (uint32_t yy = 0; yy < 4; yy++) {
                            for (uint32_t xx = 0; xx < 4; xx++) {
                                uint32_t px = mb_x0 + lx + xx;
                                uint32_t py = mb_y0 + ly + yy;
                                luma[py * row_width + px] =
                                    (uint8_t)h264_clip_u8(pred16[(ly + yy) * 16 + lx + xx] +
                                                          res[yy * 4 + xx]);
                                dec[py * row_width + px] = 1;
                            }
                        }
                    }
                }
            }
        } else {
            for (uint32_t blk = 0; blk < 16; blk++) {
                uint32_t lx = h264_blk_x[blk], ly = h264_blk_y[blk];
                uint32_t bx = lx >> 2, by = ly >> 2;
                uint32_t availA = (bx > 0 || mb_x > 0);
                uint32_t availB = (by > 0 || mb_y > 0);
                int32_t nA = 0, nB = 0;
                if (availA) {
                    nA = (bx > 0) ? cur_tc[h264_xy2blk[by][bx - 1]] : left_tc[by];
                    if (mb_x > 0 && bx == 0) out->cross_mb_nc_ok = 1;
                }
                if (availB) {
                    nB = (by > 0) ? cur_tc[h264_xy2blk[by - 1][bx]] :
                         top_tc[mb_x * 4U + bx];
                    if (mb_y > 0 && by == 0) out->top_mb_nc_ok = 1;
                }
                int32_t nc = (availA && availB) ? ((nA + nB + 1) >> 1) :
                             availA ? nA : availB ? nB : 0;

                int16_t coeff[16];
                for (uint32_t i = 0; i < 16; i++) coeff[i] = 0;
                uint32_t tc = 0, t1 = 0, tz = 0, runs = 0;
                if (mi.cbp_luma & (1U << (blk >> 2))) {
                    if (!decode_residual_4x4(b, nc, coeff, &tc, &t1, &tz, &runs)) {
                        h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad CAVLC luma residual");
                        break;
                    }
                }
                if (out->status != H264_OK) {
                    break;
                }
                cur_tc[blk] = (uint8_t)tc;

                int32_t res[16], pred[16];
                dequant_idct_4x4(coeff, qpy, res);
                if (!predict4x4_region(luma, dec, row_width, row_width,
                                       mb_x0 + lx, mb_y0 + ly, mi.modes[blk], pred)) {
                    h264_set_blocker(out, H264_ERR_UNSUPPORTED, mb, "intra4x4 mode needs unavailable neighbour");
                    break;
                }
                if (out->status != H264_OK) {
                    break;
                }
                if (mb_x > 0 && lx == 0) out->cross_mb_intra_ok = 1;
                if (mb_y > 0 && ly == 0) out->top_mb_intra_ok = 1;
                for (uint32_t yy = 0; yy < 4; yy++) {
                    for (uint32_t xx = 0; xx < 4; xx++) {
                        uint32_t px = mb_x0 + lx + xx;
                        uint32_t py = mb_y0 + ly + yy;
                        luma[py * row_width + px] =
                            (uint8_t)h264_clip_u8(pred[yy * 4 + xx] + res[yy * 4 + xx]);
                        dec[py * row_width + px] = 1;
                    }
                }
                if (mb == 0 && blk == 0) {
                    out->first_mb_first_residual_ok = 1;
                    out->first_mb_first_residual_block = 0;
                    out->first_mb_first_residual_total_coeff = tc;
                    out->first_mb_first_residual_trailing_ones = t1;
                    out->first_mb_first_residual_total_zeros = tz;
                    out->first_mb_first_residual_runs = runs;
                    out->first_mb_first_residual_coeff_sum = 0;
                    out->first_mb_first_residual_nonzero_mask = 0;
                    for (uint32_t i = 0; i < 16; i++) {
                        out->first_mb_first_residual_coeff[i] = coeff[i];
                        out->first_mb_first_residual_coeff_sum += coeff[i];
                        if (coeff[i]) out->first_mb_first_residual_nonzero_mask |= (1U << i);
                        out->first_block_residual[i] = (int16_t)res[i];
                        out->first_block_recon[i] = luma[(i >> 2) * row_width + (i & 3)];
                    }
                    out->first_block_dequant_ok = 1;
                    out->first_block_idct_ok = 1;
                    out->first_block_residual_sum = 0;
                    out->first_block_recon_sum = 0;
                    for (uint32_t i = 0; i < 16; i++) {
                        out->first_block_residual_sum += out->first_block_residual[i];
                        out->first_block_recon_sum += out->first_block_recon[i];
                    }
                    out->first_block_pred_mode_supported = 1;
                    out->first_block_prediction_dc = (mi.modes[0] == 2) ? 128U : 0U;
                    out->first_block_reconstructed_ok = 1;
                    h264_copy_str(out->first_block_detail, sizeof(out->first_block_detail),
                                  "first 4x4 luma block reconstructed");
                }
            }
        }
        if (out->status != H264_OK) {
            break;
        }

        for (uint32_t by = 0; by < 4; by++) {
            left_tc[by] = cur_tc[h264_xy2blk[by][3]];
            left_modes[by] = mi.modes[h264_xy2blk[by][3]];
        }
        for (uint32_t bx = 0; bx < 4; bx++) {
            top_tc[mb_x * 4U + bx] = cur_tc[h264_xy2blk[3][bx]];
            top_modes[mb_x * 4U + bx] = mi.modes[h264_xy2blk[3][bx]];
        }
        out->luma_mbs_decoded++;
        if (mi.cbp_chroma != 0) {
            for (uint32_t c = 0; c < 2; c++) {
                if (!skip_residual_cavlc(b, -1, 4, 1, 0)) {
                    h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad chroma DC residual");
                    break;
                }
            }
            if (out->status != H264_OK) {
                break;
            }
            if (mi.cbp_chroma == 2U) {
                for (uint32_t c = 0; c < 2; c++) {
                    uint8_t cur_chroma_tc[4];
                    for (uint32_t i = 0; i < 4; i++) cur_chroma_tc[i] = 0;
                    for (uint32_t blk = 0; blk < 4; blk++) {
                        uint32_t bx = blk & 1U;
                        uint32_t by = blk >> 1;
                        uint32_t availA = (bx > 0 || mb_x > 0);
                        uint32_t availB = (by > 0 || mb_y > 0);
                        int32_t nA = availA ? (bx > 0 ? cur_chroma_tc[blk - 1] : left_chroma_tc[c][by]) : 0;
                        int32_t nB = availB ? (by > 0 ? cur_chroma_tc[blk - 2] :
                                               top_chroma_tc[c][mb_x * 2U + bx]) : 0;
                        int32_t nc = (availA && availB) ? ((nA + nB + 1) >> 1) :
                                     availA ? nA : availB ? nB : 0;
                        uint32_t tc = 0;
                        if (!skip_residual_cavlc(b, nc, 15, 0, &tc)) {
                            h264_set_blocker(out, H264_ERR_BAD_BITSTREAM, mb, "bad chroma AC residual");
                            break;
                        }
                        cur_chroma_tc[blk] = (uint8_t)tc;
                    }
                    left_chroma_tc[c][0] = cur_chroma_tc[1];
                    left_chroma_tc[c][1] = cur_chroma_tc[3];
                    top_chroma_tc[c][mb_x * 2U] = cur_chroma_tc[2];
                    top_chroma_tc[c][mb_x * 2U + 1U] = cur_chroma_tc[3];
                    if (out->status != H264_OK) {
                        break;
                    }
                }
                if (out->status != H264_OK) {
                    break;
                }
            } else {
                for (uint32_t c = 0; c < 2; c++) {
                    left_chroma_tc[c][0] = 0;
                    left_chroma_tc[c][1] = 0;
                    top_chroma_tc[c][mb_x * 2U] = 0;
                    top_chroma_tc[c][mb_x * 2U + 1U] = 0;
                }
            }
        } else {
            for (uint32_t c = 0; c < 2; c++) {
                left_chroma_tc[c][0] = 0;
                left_chroma_tc[c][1] = 0;
                top_chroma_tc[c][mb_x * 2U] = 0;
                top_chroma_tc[c][mb_x * 2U + 1U] = 0;
            }
        }
    }

    if (b->error && out->luma_mbs_decoded >= out->mb_width &&
        out->luma_blocker_mb == out->luma_mbs_decoded) {
        b->error = 0;
        out->status = H264_OK;
        h264_copy_str(out->status_detail, sizeof(out->status_detail),
                      "first slice ended after decoded luma region");
    }
    out->luma_mb_rows_decoded = out->luma_mbs_decoded / out->mb_width;
    out->luma_rows_decoded = out->luma_mb_rows_decoded * 16U;
    if (out->status == H264_OK && out->luma_mbs_decoded == target_mbs) {
        out->full_frame_luma_ok = 1;
        out->luma_region_width = row_width;
        out->luma_region_height = out->height <= coded_height ? out->height : coded_height;
    } else if (out->luma_mb_rows_decoded > 0) {
        out->luma_region_width = row_width;
        out->luma_region_height = out->luma_rows_decoded;
    } else if (out->luma_mbs_decoded > 0) {
        out->luma_region_width = out->luma_mbs_decoded * 16U;
        out->luma_region_height = 16U;
    } else {
        out->luma_region_width = 0;
        out->luma_region_height = 0;
    }
    out->luma_samples_decoded = out->luma_region_width * out->luma_region_height;
    for (uint32_t y = 0; y < out->luma_region_height; y++) {
        for (uint32_t x = 0; x < out->luma_region_width; x++) {
            out->luma_region[y * out->luma_region_width + x] = luma[y * row_width + x];
        }
    }
    out->luma_region_info.width = out->luma_region_width;
    out->luma_region_info.height = out->luma_region_height;
    out->luma_region_info.macroblocks_decoded = out->luma_mbs_decoded;
    out->luma_region_info.macroblock_rows_decoded = out->luma_mb_rows_decoded;
    out->luma_region_info.samples_decoded = out->luma_samples_decoded;
    out->luma_region_info.full_frame_luma = out->full_frame_luma_ok;
    h264_finish_luma_stats(out);
    if (out->luma_mbs_decoded > 0) {
        h264_fill_mb0_from_region(out, mb0_modes);
    }
    if (out->luma_mbs_decoded >= out->mb_width) {
        out->luma_row_ok = 1;
        if (out->full_frame_luma_ok) {
            h264_copy_str(out->luma_detail, sizeof(out->luma_detail),
                          "first IDR visible-frame luma reconstructed");
        } else if (out->luma_mb_rows_decoded > 1U) {
            h264_copy_str(out->luma_detail, sizeof(out->luma_detail),
                          "multiple macroblock rows luma reconstructed");
        } else {
            h264_copy_str(out->luma_detail, sizeof(out->luma_detail),
                          "first macroblock row luma reconstructed");
        }
    }
    return out->luma_mbs_decoded > 0;
}

static uint32_t parse_sps(const uint8_t *nal, uint32_t size, H264Probe *out)
{
    if (nal == 0 || size < 5 || (nal[0] & 0x1FU) != 7U) {
        out->status = H264_ERR_NO_SPS;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), h264_status_text(out->status));
        return 0;
    }
    H264Bits b;
    bits_init(&b, nal + 1, size - 1);
    out->profile_idc = (uint8_t)bits_read(&b, 8);
    out->constraint_flags = (uint8_t)bits_read(&b, 8);
    out->level_idc = (uint8_t)bits_read(&b, 8);
    (void)bits_ue(&b); /* seq_parameter_set_id */

    uint32_t chroma_format_idc = 1;
    if (out->profile_idc == 100 || out->profile_idc == 110 || out->profile_idc == 122 ||
        out->profile_idc == 244 || out->profile_idc == 44 || out->profile_idc == 83 ||
        out->profile_idc == 86 || out->profile_idc == 118 || out->profile_idc == 128) {
        chroma_format_idc = bits_ue(&b);
        if (chroma_format_idc == 3) {
            (void)bits_read(&b, 1);
        }
        (void)bits_ue(&b); /* bit_depth_luma_minus8 */
        (void)bits_ue(&b); /* bit_depth_chroma_minus8 */
        (void)bits_read(&b, 1); /* qpprime_y_zero_transform_bypass_flag */
        if (bits_read(&b, 1)) {
            out->status = H264_ERR_UNSUPPORTED;
            h264_copy_str(out->status_detail, sizeof(out->status_detail), "SPS scaling matrices not supported");
            return 0;
        }
    }
    out->log2_max_frame_num = bits_ue(&b) + 4U;
    out->pic_order_cnt_type = bits_ue(&b);
    if (out->pic_order_cnt_type == 0) {
        out->log2_max_pic_order_cnt_lsb = bits_ue(&b) + 4U;
    } else if (out->pic_order_cnt_type == 1) {
        (void)bits_read(&b, 1);
        (void)bits_se(&b);
        (void)bits_se(&b);
        uint32_t n = bits_ue(&b);
        for (uint32_t i = 0; i < n && i < 256; i++) {
            (void)bits_se(&b);
        }
    } else if (out->pic_order_cnt_type != 2) {
        out->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "unsupported pic_order_cnt_type");
        return 0;
    }
    out->max_num_ref_frames = bits_ue(&b);
    (void)bits_read(&b, 1); /* gaps_in_frame_num_value_allowed_flag */
    uint32_t pic_width_in_mbs_minus1 = bits_ue(&b);
    uint32_t pic_height_in_map_units_minus1 = bits_ue(&b);
    out->frame_mbs_only = (uint8_t)bits_read(&b, 1);
    if (!out->frame_mbs_only) {
        (void)bits_read(&b, 1); /* mb_adaptive_frame_field_flag */
    }
    (void)bits_read(&b, 1); /* direct_8x8_inference_flag */
    uint32_t crop_left = 0, crop_right = 0, crop_top = 0, crop_bottom = 0;
    if (bits_read(&b, 1)) {
        crop_left = bits_ue(&b);
        crop_right = bits_ue(&b);
        crop_top = bits_ue(&b);
        crop_bottom = bits_ue(&b);
    }
    if (b.error) {
        out->status = H264_ERR_BAD_BITSTREAM;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), h264_status_text(out->status));
        return 0;
    }
    out->mb_width = pic_width_in_mbs_minus1 + 1U;
    out->mb_height = (pic_height_in_map_units_minus1 + 1U) * (out->frame_mbs_only ? 1U : 2U);
    out->width = out->mb_width * 16U;
    out->height = out->mb_height * 16U;
    if (chroma_format_idc == 1) {
        out->width -= (crop_left + crop_right) * 2U;
        out->height -= (crop_top + crop_bottom) * 2U;
    }
    return 1;
}

static uint32_t parse_pps(const uint8_t *nal, uint32_t size, H264Probe *out)
{
    if (nal == 0 || size < 2 || (nal[0] & 0x1FU) != 8U) {
        out->status = H264_ERR_NO_PPS;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), h264_status_text(out->status));
        return 0;
    }
    H264Bits b;
    bits_init(&b, nal + 1, size - 1);
    (void)bits_ue(&b); /* pic_parameter_set_id */
    (void)bits_ue(&b); /* seq_parameter_set_id */
    out->entropy_coding_mode_flag = (uint8_t)bits_read(&b, 1);
    (void)bits_read(&b, 1); /* bottom_field_pic_order_in_frame_present_flag */
    out->num_slice_groups = (uint8_t)(bits_ue(&b) + 1U);
    if (out->num_slice_groups > 1U) {
        out->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "FMO slice groups not supported");
        return 0;
    }
    (void)bits_ue(&b); /* num_ref_idx_l0_default_active_minus1 */
    (void)bits_ue(&b); /* num_ref_idx_l1_default_active_minus1 */
    (void)bits_read(&b, 1); /* weighted_pred_flag */
    (void)bits_read(&b, 2); /* weighted_bipred_idc */
    out->pic_init_qp_minus26 = bits_se(&b);
    (void)bits_se(&b); /* pic_init_qs_minus26 */
    (void)bits_se(&b); /* chroma_qp_index_offset */
    out->deblocking_filter_control_present_flag = (uint8_t)bits_read(&b, 1);
    (void)bits_read(&b, 1); /* constrained_intra_pred_flag */
    out->redundant_pic_cnt_present_flag = (uint8_t)bits_read(&b, 1);
    if (b.error) {
        out->status = H264_ERR_BAD_BITSTREAM;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), h264_status_text(out->status));
        return 0;
    }
    return 1;
}

static uint32_t parse_slice_header(const uint8_t *nal, uint32_t size, H264Probe *out)
{
    if (nal == 0 || size < 2) {
        return 0;
    }
    uint8_t nal_type = nal[0] & 0x1FU;
    if (nal_type != 1U && nal_type != 5U) {
        return 0;
    }
    H264Bits b;
    bits_init(&b, nal + 1, size - 1);
    out->first_slice_nal_type = nal_type;
    out->first_mb_in_slice = bits_ue(&b);
    out->first_slice_type = bits_ue(&b);
    out->first_pps_id = bits_ue(&b);
    out->first_frame_num = bits_read(&b, out->log2_max_frame_num);
    if (nal_type == 5U) {
        out->idr_pic_id = bits_ue(&b);
        out->has_idr = 1;
    }
    if (out->pic_order_cnt_type == 0) {
        out->pic_order_cnt_lsb = bits_read(&b, out->log2_max_pic_order_cnt_lsb);
    }
    if (out->redundant_pic_cnt_present_flag) {
        (void)bits_ue(&b);
    }
    if ((nal[0] & 0x60U) != 0) {
        if (nal_type == 5U) {
            (void)bits_read(&b, 1); /* no_output_of_prior_pics_flag */
            (void)bits_read(&b, 1); /* long_term_reference_flag */
        } else if (bits_read(&b, 1)) {
            for (uint32_t guard = 0; guard < 32; guard++) {
                uint32_t op = bits_ue(&b);
                if (op == 0) {
                    break;
                }
                if (op == 1 || op == 3) {
                    (void)bits_ue(&b);
                    (void)bits_ue(&b);
                } else if (op == 2) {
                    (void)bits_ue(&b);
                } else if (op == 4) {
                    (void)bits_ue(&b);
                } else if (op == 6) {
                    (void)bits_ue(&b);
                } else {
                    break;
                }
            }
        }
    }
    out->slice_qp_delta = bits_se(&b); /* slice_qp_delta */
    if (out->deblocking_filter_control_present_flag) {
        uint32_t disable_deblocking = bits_ue(&b);
        if (disable_deblocking != 1U) {
            (void)bits_se(&b);
            (void)bits_se(&b);
        }
    }
    if ((out->first_slice_type % 5U) == 2U) {
        if (!decode_first_luma_region(&b, out)) {
            return 0;
        }
    }
    if (b.error) {
        out->status = H264_ERR_BAD_BITSTREAM;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "bad slice header");
        return 0;
    }
    if ((out->first_slice_type % 5U) == 0U) {
        out->has_p_slice = 1;
    }
    return 1;
}

static void decide_support(H264Probe *out)
{
    if (out->status != H264_OK) {
        return;
    }
    if (out->profile_idc != 66U || (out->constraint_flags & 0x40U) == 0) {
        out->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "not Constrained Baseline profile");
        return;
    }
    if (!out->frame_mbs_only) {
        out->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "interlaced H.264 not supported");
        return;
    }
    if (out->entropy_coding_mode_flag) {
        out->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "CABAC not supported");
        return;
    }
    if (out->num_slice_groups != 1U) {
        out->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "FMO slice groups not supported");
        return;
    }
    if (!out->has_idr) {
        out->status = H264_ERR_NO_SLICE;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "first sample has no IDR slice");
        return;
    }
    out->constrained_baseline_supported = 1;
    out->status = H264_OK;
    h264_copy_str(out->status_detail, sizeof(out->status_detail),
                  out->full_frame_luma_ok ?
                  "Baseline first IDR visible-frame luma reconstructed; chroma/RGB/playback not implemented" :
                  out->luma_row_ok ?
                  "Baseline macroblock-row luma reconstructed; full playback not implemented" :
                  out->luma_mbs_decoded > 1U ?
                  "Baseline partial first-row luma reconstructed; blocked before full row" :
                  out->mb0_luma_ok ?
                  "Baseline macroblock 0 luma (16/16 4x4 blocks) reconstructed; full-frame decode not implemented" :
                  out->first_block_reconstructed_ok ?
                  "Baseline first 4x4 luma block reconstructed; rest of MB not implemented" :
                  out->first_mb_first_residual_ok ?
                  "Baseline first MB CAVLC coefficients OK; no pixel decode" :
                  out->first_mb_header_ok ?
                  "H.264 Baseline headers and first macroblock OK; pixel decode not implemented" :
                  "H.264 Baseline headers OK; pixel decode not implemented");
}

void h264_probe_avcc_and_sample(const uint8_t *sps, uint32_t sps_size,
                                const uint8_t *pps, uint32_t pps_size,
                                const uint8_t *sample, uint32_t sample_size,
                                uint8_t nal_length_size,
                                H264Probe *out)
{
    h264_zero(out, sizeof(*out));
    if (!parse_sps(sps, sps_size, out) || !parse_pps(pps, pps_size, out)) {
        return;
    }
    if (nal_length_size == 0 || nal_length_size > 4) {
        out->status = H264_ERR_BAD_BITSTREAM;
        h264_copy_str(out->status_detail, sizeof(out->status_detail), "invalid NAL length size");
        return;
    }
    uint32_t pos = 0;
    while (pos + nal_length_size <= sample_size) {
        uint32_t len = 0;
        for (uint32_t i = 0; i < nal_length_size; i++) {
            len = (len << 8) | sample[pos + i];
        }
        pos += nal_length_size;
        if (len == 0 || pos + len > sample_size) {
            out->status = H264_ERR_BAD_BITSTREAM;
            h264_copy_str(out->status_detail, sizeof(out->status_detail), "sample NAL length overruns sample");
            return;
        }
        out->sample_nal_count++;
        if ((sample[pos] & 0x1FU) == 1U || (sample[pos] & 0x1FU) == 5U) {
            if (parse_slice_header(sample + pos, len, out)) {
                decide_support(out);
                return;
            }
            if (out->status != 0) {
                return;
            }
        }
        pos += len;
    }
    out->status = H264_ERR_NO_SLICE;
    h264_copy_str(out->status_detail, sizeof(out->status_detail), h264_status_text(out->status));
}

/* =========================================================================
 * Full-frame H.264 Constrained Baseline decoder.
 *
 * Reuses the verified bit reader, CAVLC residual decoders, inverse
 * transforms and intra-prediction helpers above. Writes whole reconstructed
 * YUV420 frames into caller-provided planes. Video only.
 * ========================================================================= */

/* H.264 Table 8-15: chroma QP derivation for qPi in 30..51 (below 30, QPc==qPi). */
static const uint8_t h264_chroma_qp_tab[22] = {
    29,30,31,32,32,33,34,34,35,35,36,36,37,37,38,38,39,39,39,39,39,39
};
static int32_t h264_chroma_qp(int32_t qpy, int32_t offset)
{
    int32_t qpi = qpy + offset;
    if (qpi < 0) qpi = 0;
    if (qpi > 51) qpi = 51;
    return qpi < 30 ? qpi : (int32_t)h264_chroma_qp_tab[qpi - 30];
}

/* Decode a chroma DC residual (2x2, 4 coefficients, special VLC). Returns 1 on
 * success with coeff[] in raster order and *tc set. */
static uint32_t decode_chroma_dc_block(H264Bits *b, int16_t coeff[4], uint32_t *tc_out)
{
    for (uint32_t i = 0; i < 4; i++) coeff[i] = 0;
    *tc_out = 0;
    uint32_t total_coeff = 0, trailing_ones = 0;
    if (!read_chroma_dc_coeff_token(b, &total_coeff, &trailing_ones)) return 0;
    *tc_out = total_coeff;
    if (trailing_ones > 3U || trailing_ones > total_coeff || total_coeff > 4U) {
        b->error = 1; return 0;
    }
    if (total_coeff == 0) return 1;
    int16_t levels[4];
    for (uint32_t i = 0; i < 4; i++) levels[i] = 0;
    if (!decode_cavlc_levels(b, total_coeff, trailing_ones, levels)) return 0;
    uint32_t total_zeros = 0;
    if (total_coeff < 4U) {
        if (!read_chroma_dc_total_zeros(b, total_coeff, &total_zeros)) return 0;
    }
    uint32_t runs_before[4];
    for (uint32_t i = 0; i < 4; i++) runs_before[i] = 0;
    uint32_t zeros_left = total_zeros;
    for (uint32_t i = 0; i + 1U < total_coeff; i++) {
        uint32_t run_before = 0;
        if (zeros_left > 0) {
            if (!read_run_before(b, zeros_left, &run_before)) return 0;
            if (run_before > zeros_left) { b->error = 1; return 0; }
            zeros_left -= run_before;
            runs_before[i] = run_before;
        }
    }
    runs_before[total_coeff - 1] = zeros_left;
    int32_t coeff_num = -1;
    for (int32_t i = (int32_t)total_coeff - 1; i >= 0; i--) {
        coeff_num += (int32_t)runs_before[i] + 1;
        if (coeff_num < 0 || coeff_num >= 4) { b->error = 1; return 0; }
        coeff[coeff_num] = levels[i];
    }
    return !b->error;
}

/* Chroma DC 2x2 inverse Hadamard + dequant (H.264 8.5.11). Produces the four
 * DC values to seed each chroma 4x4 block, in raster block order (0,1,2,3). */
static void chroma_dc_dequant(const int16_t c[4], int32_t qpc, int32_t out[4])
{
    static const int32_t dequant0[6] = {10, 11, 13, 14, 16, 18};
    int32_t f0 = c[0] + c[1] + c[2] + c[3];
    int32_t f1 = c[0] - c[1] + c[2] - c[3];
    int32_t f2 = c[0] + c[1] - c[2] - c[3];
    int32_t f3 = c[0] - c[1] - c[2] + c[3];
    int32_t qp_per = qpc / 6, qp_rem = qpc % 6;
    int32_t scale = dequant0[qp_rem];
    /* dcCij = ((f * 16*scale) << qp_per) >> 5  ==  ((f*scale)<<qp_per) >> 1 */
    out[0] = ((f0 * scale) << qp_per) >> 1;
    out[1] = ((f1 * scale) << qp_per) >> 1;
    out[2] = ((f2 * scale) << qp_per) >> 1;
    out[3] = ((f3 * scale) << qp_per) >> 1;
}

/* Chroma intra prediction into an 8x8 buffer (mode: 0 DC, 1 Horiz, 2 Vert,
 * 3 Plane). Chroma neighbours come from fully-reconstructed adjacent MBs, so
 * availability is given by macroblock-position flags (no per-pixel mask). */
static uint32_t predict_chroma8x8(const uint8_t *c,
                                  uint32_t cstride, uint32_t x, uint32_t y,
                                  uint32_t mode, uint32_t upR, uint32_t leftR,
                                  uint32_t ulR, int32_t pred[64])
{
    int32_t T[8], L[8], TL = 0;
    for (uint32_t i = 0; i < 8; i++) { T[i] = 0; L[i] = 0; }
    if (upR) for (uint32_t i = 0; i < 8; i++) T[i] = c[(y - 1) * cstride + x + i];
    if (leftR) for (uint32_t i = 0; i < 8; i++) L[i] = c[(y + i) * cstride + x - 1];
    if (ulR) TL = c[(y - 1) * cstride + x - 1];
    switch (mode) {
    case 1: /* Horizontal */
        if (!leftR) return 0;
        for (uint32_t yy = 0; yy < 8; yy++)
            for (uint32_t xx = 0; xx < 8; xx++) pred[yy * 8 + xx] = L[yy];
        return 1;
    case 2: /* Vertical */
        if (!upR) return 0;
        for (uint32_t yy = 0; yy < 8; yy++)
            for (uint32_t xx = 0; xx < 8; xx++) pred[yy * 8 + xx] = T[xx];
        return 1;
    case 3: { /* Plane */
        if (!upR || !leftR || !ulR) return 0;
        int32_t H = 0, V = 0;
        for (int32_t i = 1; i <= 3; i++) {
            H += i * (T[3 + i] - T[3 - i]);
            V += i * (L[3 + i] - L[3 - i]);
        }
        H += 4 * (T[7] - TL);
        V += 4 * (L[7] - TL);
        int32_t a = 16 * (T[7] + L[7]);
        int32_t bb = (17 * H + 16) >> 5;
        int32_t cc = (17 * V + 16) >> 5;
        for (int32_t yy = 0; yy < 8; yy++)
            for (int32_t xx = 0; xx < 8; xx++)
                pred[yy * 8 + xx] = h264_clip_u8((a + bb * (xx - 3) + cc * (yy - 3) + 16) >> 5);
        return 1;
    }
    default: { /* DC, per-4x4-quadrant rule (8.3.4) */
        for (uint32_t qy = 0; qy < 2; qy++) {
            for (uint32_t qx = 0; qx < 2; qx++) {
                int32_t sumT = 0, sumL = 0;
                for (uint32_t i = 0; i < 4; i++) { sumT += T[qx * 4 + i]; sumL += L[qy * 4 + i]; }
                int32_t v;
                uint32_t useT = upR, useL = leftR;
                if ((qx == 0 && qy == 0) || (qx == 1 && qy == 1)) {
                    if (useT && useL) v = (sumT + sumL + 4) >> 3;
                    else if (useT) v = (sumT + 2) >> 2;
                    else if (useL) v = (sumL + 2) >> 2;
                    else v = 128;
                } else if (qx == 1 && qy == 0) { /* top-right: prefer top */
                    if (useT) v = (sumT + 2) >> 2;
                    else if (useL) v = (sumL + 2) >> 2;
                    else v = 128;
                } else { /* bottom-left: prefer left */
                    if (useL) v = (sumL + 2) >> 2;
                    else if (useT) v = (sumT + 2) >> 2;
                    else v = 128;
                }
                for (uint32_t yy = 0; yy < 4; yy++)
                    for (uint32_t xx = 0; xx < 4; xx++)
                        pred[(qy * 4 + yy) * 8 + (qx * 4 + xx)] = v;
            }
        }
        return 1;
    }
    }
}

/* ---- Decoder init: parse SPS/PPS and validate Constrained Baseline -------- */

static uint32_t parse_pps_extra(const uint8_t *nal, uint32_t size,
                                int32_t *chroma_qp_offset, uint32_t *num_ref_l0)
{
    if (nal == 0 || size < 2 || (nal[0] & 0x1FU) != 8U) return 0;
    H264Bits b;
    bits_init(&b, nal + 1, size - 1);
    (void)bits_ue(&b);            /* pic_parameter_set_id */
    (void)bits_ue(&b);            /* seq_parameter_set_id */
    (void)bits_read(&b, 1);       /* entropy_coding_mode_flag */
    (void)bits_read(&b, 1);       /* bottom_field_pic_order_present */
    (void)bits_ue(&b);            /* num_slice_groups_minus1 (==0 here) */
    *num_ref_l0 = bits_ue(&b) + 1U;  /* num_ref_idx_l0_default_active_minus1 */
    (void)bits_ue(&b);            /* num_ref_idx_l1_default_active_minus1 */
    (void)bits_read(&b, 1);       /* weighted_pred_flag */
    (void)bits_read(&b, 2);       /* weighted_bipred_idc */
    (void)bits_se(&b);            /* pic_init_qp_minus26 */
    (void)bits_se(&b);            /* pic_init_qs_minus26 */
    *chroma_qp_offset = bits_se(&b); /* chroma_qp_index_offset */
    return !b.error;
}

uint32_t h264_decoder_init(H264Decoder *dec,
                           const uint8_t *sps, uint32_t sps_size,
                           const uint8_t *pps, uint32_t pps_size)
{
    H264Frame *frames = dec->frames;
    uint32_t pool = dec->frame_pool_size;
    H264Work work = dec->work;
    h264_zero(dec, sizeof(*dec));
    dec->frames = frames;
    dec->frame_pool_size = pool;
    dec->work = work;

    H264Probe p;
    h264_zero(&p, sizeof(p));
    if (!parse_sps(sps, sps_size, &p)) {
        dec->status = p.status ? p.status : H264_ERR_NO_SPS;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), p.status_detail);
        return dec->status;
    }
    if (!parse_pps(pps, pps_size, &p)) {
        dec->status = p.status ? p.status : H264_ERR_NO_PPS;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), p.status_detail);
        return dec->status;
    }
    int32_t cqo = 0; uint32_t nref0 = 1;
    if (!parse_pps_extra(pps, pps_size, &cqo, &nref0)) {
        dec->status = H264_ERR_NO_PPS;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), "bad PPS");
        return dec->status;
    }

    dec->profile_idc = p.profile_idc;
    dec->constraint_flags = p.constraint_flags;
    dec->level_idc = p.level_idc;
    dec->mb_width = p.mb_width;
    dec->mb_height = p.mb_height;
    dec->width = p.mb_width * 16U;
    dec->height = p.mb_height * 16U;
    dec->crop_width = p.width;
    dec->crop_height = p.height;
    dec->log2_max_frame_num = p.log2_max_frame_num;
    dec->pic_order_cnt_type = p.pic_order_cnt_type;
    dec->log2_max_pic_order_cnt_lsb = p.log2_max_pic_order_cnt_lsb;
    dec->max_num_ref_frames = p.max_num_ref_frames;
    dec->frame_mbs_only = p.frame_mbs_only;
    dec->entropy_coding_mode_flag = p.entropy_coding_mode_flag;
    dec->deblocking_filter_control_present_flag = p.deblocking_filter_control_present_flag;
    dec->num_slice_groups = p.num_slice_groups;
    dec->pic_init_qp_minus26 = p.pic_init_qp_minus26;
    dec->chroma_format_idc = 1;
    dec->pic_init_qp_minus26 = p.pic_init_qp_minus26;
    /* stash chroma_qp_index_offset in an unused-by-decode field via last_frame_num? no */
    dec->last_frame_num = -1;
    dec->ref_count = 0;
    dec->frames_decoded = 0;

    if (dec->profile_idc != 66U && !(dec->constraint_flags & 0x40U)) {
        /* Allow Main/High only if they decode as baseline-compatible is false. */
    }
    if (!dec->frame_mbs_only) {
        dec->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), "interlaced not supported");
        return dec->status;
    }
    if (dec->entropy_coding_mode_flag) {
        dec->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), "CABAC not supported");
        return dec->status;
    }
    if (dec->num_slice_groups != 1U) {
        dec->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), "FMO not supported");
        return dec->status;
    }
    if (dec->mb_width == 0 || dec->mb_width > H264_MAX_MB_WIDTH ||
        dec->mb_height == 0 || dec->mb_height > H264_MAX_MB_HEIGHT) {
        dec->status = H264_ERR_UNSUPPORTED;
        h264_copy_str(dec->status_detail, sizeof(dec->status_detail), "frame size beyond decoder limit");
        return dec->status;
    }

    dec->max_refs = dec->max_num_ref_frames;
    if (dec->max_refs < 1) dec->max_refs = 1;
    if (dec->max_refs > H264_MAX_REF_FRAMES) dec->max_refs = H264_MAX_REF_FRAMES;
    if (pool > 0 && dec->max_refs > pool - 1) dec->max_refs = pool - 1;
    (void)nref0;
    dec->chroma_qp_index_offset = cqo;
    dec->ready = 1;
    dec->status = H264_OK;
    h264_copy_str(dec->status_detail, sizeof(dec->status_detail), "decoder ready");
    return H264_OK;
}

/* ---- Per-frame decode context (transient, lives on the driver stack) ----- */
typedef struct {
    H264Decoder *dec;
    uint8_t *y, *cb, *cr;       /* current frame planes */
    uint8_t *decmask;           /* luma reconstructed mask (cw*ch) */
    uint32_t cw, ch;            /* coded luma dims */
    uint32_t ccw, cch;          /* coded chroma dims */
    uint32_t mb_w, mb_h;
    uint32_t gw;                /* luma 4x4-block grid width  (mb_w*4) */
    uint32_t cgw;               /* chroma 2x2-block grid width (mb_w*2) */
    int32_t  qpy;
    uint32_t slice_type;        /* %5: 2==I, 0==P */
    uint32_t num_ref_idx_l0;
} FrameCtx;

static int32_t luma_nc(const FrameCtx *c, uint32_t mb_x, uint32_t mb_y,
                       uint32_t bx, uint32_t by)
{
    uint32_t bxg = mb_x * 4U + bx, byg = mb_y * 4U + by;
    uint32_t availA = (bx > 0 || mb_x > 0);
    uint32_t availB = (by > 0 || mb_y > 0);
    int32_t nA = availA ? c->dec->work.nnz[byg * c->gw + bxg - 1] : 0;
    int32_t nB = availB ? c->dec->work.nnz[(byg - 1) * c->gw + bxg] : 0;
    return (availA && availB) ? ((nA + nB + 1) >> 1) : availA ? nA : availB ? nB : 0;
}

static int32_t chroma_nc(const FrameCtx *c, uint32_t comp, uint32_t mb_x,
                         uint32_t mb_y, uint32_t bx, uint32_t by)
{
    uint32_t bxg = mb_x * 2U + bx, byg = mb_y * 2U + by;
    uint32_t availA = (bx > 0 || mb_x > 0);
    uint32_t availB = (by > 0 || mb_y > 0);
    int32_t nA = availA ? c->dec->work.cnnz[comp][byg * c->cgw + bxg - 1] : 0;
    int32_t nB = availB ? c->dec->work.cnnz[comp][(byg - 1) * c->cgw + bxg] : 0;
    return (availA && availB) ? ((nA + nB + 1) >> 1) : availA ? nA : availB ? nB : 0;
}

/* Reconstruct chroma (both Cb and Cr) for one macroblock. Returns 0 on error. */
static uint32_t recon_chroma_mb(FrameCtx *c, H264Bits *b, uint32_t mb_x, uint32_t mb_y,
                                uint32_t chroma_mode, uint32_t cbp_chroma)
{
    uint32_t cx0 = mb_x * 8U, cy0 = mb_y * 8U;
    uint32_t upR = (mb_y > 0), leftR = (mb_x > 0), ulR = (mb_x > 0 && mb_y > 0);
    int32_t qpc = h264_chroma_qp(c->qpy, c->dec->chroma_qp_index_offset);
    uint8_t *plane[2] = { c->cb, c->cr };

    int16_t dc[2][4];
    int16_t ac[2][4][16];
    for (uint32_t comp = 0; comp < 2; comp++)
        for (uint32_t blk = 0; blk < 4; blk++)
            for (uint32_t i = 0; i < 16; i++) ac[comp][blk][i] = 0;
    for (uint32_t comp = 0; comp < 2; comp++)
        for (uint32_t i = 0; i < 4; i++) dc[comp][i] = 0;

    /* Residual order: Cb DC, Cr DC, then Cb AC(4), Cr AC(4). */
    if (cbp_chroma != 0) {
        for (uint32_t comp = 0; comp < 2; comp++) {
            uint32_t tc = 0;
            if (!decode_chroma_dc_block(b, dc[comp], &tc)) return 0;
        }
    }
    if (cbp_chroma == 2) {
        for (uint32_t comp = 0; comp < 2; comp++) {
            for (uint32_t blk = 0; blk < 4; blk++) {
                uint32_t bx = blk & 1U, by = blk >> 1;
                int32_t nc = chroma_nc(c, comp, mb_x, mb_y, bx, by);
                uint32_t tc = 0;
                if (!decode_residual_4x4_ac(b, nc, ac[comp][blk], &tc)) return 0;
                c->dec->work.cnnz[comp][(mb_y * 2U + by) * c->cgw + (mb_x * 2U + bx)] = (uint8_t)tc;
            }
        }
    } else {
        for (uint32_t comp = 0; comp < 2; comp++)
            for (uint32_t blk = 0; blk < 4; blk++) {
                uint32_t bx = blk & 1U, by = blk >> 1;
                c->dec->work.cnnz[comp][(mb_y * 2U + by) * c->cgw + (mb_x * 2U + bx)] = 0;
            }
    }

    for (uint32_t comp = 0; comp < 2; comp++) {
        int32_t pred[64];
        if (!predict_chroma8x8(plane[comp], c->ccw, cx0, cy0, chroma_mode,
                               upR, leftR, ulR, pred)) {
            return 0;
        }
        int32_t dcq[4];
        if (cbp_chroma != 0) chroma_dc_dequant(dc[comp], qpc, dcq);
        else { dcq[0] = dcq[1] = dcq[2] = dcq[3] = 0; }
        for (uint32_t blk = 0; blk < 4; blk++) {
            uint32_t bx = blk & 1U, by = blk >> 1;
            int32_t res[16];
            dequant_idct_4x4_i16(ac[comp][blk], qpc, dcq[blk], res);
            for (uint32_t yy = 0; yy < 4; yy++)
                for (uint32_t xx = 0; xx < 4; xx++) {
                    uint32_t px = cx0 + bx * 4U + xx, py = cy0 + by * 4U + yy;
                    int32_t v = pred[(by * 4U + yy) * 8 + (bx * 4U + xx)] + res[yy * 4 + xx];
                    plane[comp][py * c->ccw + px] = (uint8_t)h264_clip_u8(v);
                }
        }
    }
    return 1;
}

/* Reconstruct one intra macroblock (luma + chroma) into the frame. */
static uint32_t recon_intra_mb(FrameCtx *c, H264Bits *b, uint32_t mb_x, uint32_t mb_y,
                               uint32_t is_i16, uint32_t i16mode,
                               const uint8_t modes[16], uint32_t cbp_luma,
                               uint32_t cbp_chroma, uint32_t chroma_mode)
{
    uint32_t mb_x0 = mb_x * 16U, mb_y0 = mb_y * 16U;
    uint8_t *Y = c->y; uint8_t *DM = c->decmask; uint32_t S = c->cw;

    if (is_i16) {
        int16_t dc_coeff[16];
        uint32_t t = 0, t1 = 0, tz = 0, rn = 0;
        int32_t dcnc = luma_nc(c, mb_x, mb_y, 0, 0);
        if (!decode_residual_4x4(b, dcnc, dc_coeff, &t, &t1, &tz, &rn)) return 0;
        int16_t accoef[16][16];
        for (uint32_t k = 0; k < 16; k++) for (uint32_t i = 0; i < 16; i++) accoef[k][i] = 0;
        for (uint32_t blk = 0; blk < 16; blk++) {
            uint32_t bx = h264_blk_x[blk] >> 2, by = h264_blk_y[blk] >> 2;
            int32_t nc = luma_nc(c, mb_x, mb_y, bx, by);
            uint32_t tc = 0;
            if (cbp_luma & (1U << (blk >> 2))) {
                if (!decode_residual_4x4_ac(b, nc, accoef[blk], &tc)) return 0;
            }
            c->dec->work.nnz[(mb_y * 4U + by) * c->gw + (mb_x * 4U + bx)] = (uint8_t)tc;
        }
        int32_t dcq[16], pred[256];
        i16x16_luma_dc_dequant_idct(dc_coeff, c->qpy, dcq);
        if (!predict16x16_region(Y, DM, S, S, mb_x0, mb_y0, i16mode, pred)) return 0;
        for (uint32_t blk = 0; blk < 16; blk++) {
            uint32_t lx = h264_blk_x[blk], ly = h264_blk_y[blk];
            int32_t res[16];
            dequant_idct_4x4_i16(accoef[blk], c->qpy, dcq[blk], res);
            for (uint32_t yy = 0; yy < 4; yy++)
                for (uint32_t xx = 0; xx < 4; xx++) {
                    uint32_t px = mb_x0 + lx + xx, py = mb_y0 + ly + yy;
                    Y[py * S + px] = (uint8_t)h264_clip_u8(pred[(ly + yy) * 16 + lx + xx] + res[yy * 4 + xx]);
                    DM[py * S + px] = 1;
                }
        }
    } else {
        for (uint32_t blk = 0; blk < 16; blk++) {
            uint32_t lx = h264_blk_x[blk], ly = h264_blk_y[blk];
            uint32_t bx = lx >> 2, by = ly >> 2;
            int32_t nc = luma_nc(c, mb_x, mb_y, bx, by);
            int16_t coeff[16];
            for (uint32_t i = 0; i < 16; i++) coeff[i] = 0;
            uint32_t tc = 0, t1 = 0, tz = 0, rn = 0;
            if (cbp_luma & (1U << (blk >> 2))) {
                if (!decode_residual_4x4(b, nc, coeff, &tc, &t1, &tz, &rn)) return 0;
            }
            c->dec->work.nnz[(mb_y * 4U + by) * c->gw + (mb_x * 4U + bx)] = (uint8_t)tc;
            int32_t res[16], pred[16];
            dequant_idct_4x4(coeff, c->qpy, res);
            if (!predict4x4_region(Y, DM, S, S, mb_x0 + lx, mb_y0 + ly, modes[blk], pred)) return 0;
            for (uint32_t yy = 0; yy < 4; yy++)
                for (uint32_t xx = 0; xx < 4; xx++) {
                    uint32_t px = mb_x0 + lx + xx, py = mb_y0 + ly + yy;
                    Y[py * S + px] = (uint8_t)h264_clip_u8(pred[yy * 4 + xx] + res[yy * 4 + xx]);
                    DM[py * S + px] = 1;
                }
        }
    }
    if (!recon_chroma_mb(c, b, mb_x, mb_y, chroma_mode, cbp_chroma)) return 0;
    return 1;
}

/* ---- Slice header ------------------------------------------------------- */
typedef struct {
    uint32_t first_mb;
    uint32_t slice_type;     /* raw */
    uint32_t frame_num;
    uint8_t  is_idr;
    uint8_t  nal_ref_idc;
    int32_t  slice_qp_delta;
    uint32_t num_ref_idx_l0;
    uint8_t  disable_deblock;   /* disable_deblocking_filter_idc */
    int32_t  alpha_c0_off;      /* slice_alpha_c0_offset_div2 * 2 */
    int32_t  beta_off;          /* slice_beta_offset_div2 * 2 */
    uint32_t ok;
} SliceHeader;

/* Parse a slice header from a bit reader already positioned just past the NAL
 * header byte. Leaves *b positioned at the first macroblock's data. */
static void parse_slice_hdr(H264Decoder *dec, H264Bits *b, uint8_t nal_type,
                            uint8_t nal_ref_idc, SliceHeader *sh)
{
    h264_zero(sh, sizeof(*sh));
    sh->nal_ref_idc = nal_ref_idc;
    sh->is_idr = (nal_type == 5U);
    sh->first_mb = bits_ue(b);
    sh->slice_type = bits_ue(b);
    (void)bits_ue(b);                 /* pic_parameter_set_id */
    sh->frame_num = bits_read(b, dec->log2_max_frame_num);
    if (sh->is_idr) (void)bits_ue(b); /* idr_pic_id */
    if (dec->pic_order_cnt_type == 0) {
        (void)bits_read(b, dec->log2_max_pic_order_cnt_lsb);
    }
    uint32_t st = sh->slice_type % 5U;
    sh->num_ref_idx_l0 = 1;
    if (st == 0U || st == 3U) { /* P or SP */
        uint32_t ovr = bits_read(b, 1);
        if (ovr) sh->num_ref_idx_l0 = bits_ue(b) + 1U;
        if (bits_read(b, 1)) { /* ref_pic_list_modification_flag_l0 */
            for (uint32_t guard = 0; guard < 64; guard++) {
                uint32_t idc = bits_ue(b);
                if (idc == 3U) break;
                if (idc == 0U || idc == 1U) (void)bits_ue(b);
                else if (idc == 2U) (void)bits_ue(b);
                else break;
            }
        }
    }
    if (sh->nal_ref_idc != 0) {
        if (sh->is_idr) {
            (void)bits_read(b, 1); /* no_output_of_prior_pics_flag */
            (void)bits_read(b, 1); /* long_term_reference_flag */
        } else {
            if (bits_read(b, 1)) { /* adaptive_ref_pic_marking_mode_flag */
                for (uint32_t guard = 0; guard < 64; guard++) {
                    uint32_t op = bits_ue(b);
                    if (op == 0U) break;
                    if (op == 1U || op == 3U) { (void)bits_ue(b); (void)bits_ue(b); }
                    else if (op == 2U) (void)bits_ue(b);
                    else if (op == 4U || op == 6U) (void)bits_ue(b);
                    else if (op == 5U) { /* nothing */ }
                    else break;
                }
            }
        }
    }
    sh->slice_qp_delta = bits_se(b);
    if (dec->deblocking_filter_control_present_flag) {
        sh->disable_deblock = bits_ue(b);
        if (sh->disable_deblock != 1U) {
            sh->alpha_c0_off = bits_se(b) * 2;
            sh->beta_off = bits_se(b) * 2;
        }
    }
    sh->ok = !b->error;
}

/* ---- Intra-4x4 prediction-mode parse using the frame-wide imode grid ----- */
static void parse_i4x4_modes(FrameCtx *c, H264Bits *b, uint32_t mb_x, uint32_t mb_y,
                             uint8_t modes[16])
{
    uint8_t *im = c->dec->work.imode;
    for (uint32_t blk = 0; blk < 16; blk++) {
        uint32_t prev_flag = bits_read(b, 1);
        uint32_t rem = prev_flag ? 0 : bits_read(b, 3);
        uint32_t bx = h264_blk_x[blk] >> 2, by = h264_blk_y[blk] >> 2;
        uint32_t bxg = mb_x * 4U + bx, byg = mb_y * 4U + by;
        uint32_t availA = (bx > 0 || mb_x > 0);
        uint32_t availB = (by > 0 || mb_y > 0);
        uint32_t modeA = availA ? im[byg * c->gw + bxg - 1] : 2U;
        uint32_t modeB = availB ? im[(byg - 1) * c->gw + bxg] : 2U;
        uint32_t pred = (availA && availB) ? (modeA < modeB ? modeA : modeB) : 2U;
        uint32_t mode = prev_flag ? pred : (rem < pred ? rem : rem + 1U);
        modes[blk] = (uint8_t)mode;
        im[byg * c->gw + bxg] = (uint8_t)mode;
    }
}

static void set_mb_imode_default(FrameCtx *c, uint32_t mb_x, uint32_t mb_y)
{
    uint8_t *im = c->dec->work.imode;
    for (uint32_t by = 0; by < 4; by++)
        for (uint32_t bx = 0; bx < 4; bx++)
            im[(mb_y * 4U + by) * c->gw + (mb_x * 4U + bx)] = 2U;
}

/* Forward decl: P macroblock decode (added with inter prediction). */
static uint32_t recon_inter_mb(FrameCtx *c, H264Bits *b, uint32_t mb,
                               uint32_t mb_x, uint32_t mb_y, uint32_t mb_type_p,
                               uint32_t *mb_skip_run);

/* Decode all macroblocks of one slice into the frame. */
static uint32_t decode_slice_mbs(FrameCtx *c, H264Bits *b, const SliceHeader *sh)
{
    uint32_t total = c->mb_w * c->mb_h;
    uint32_t st = sh->slice_type % 5U;
    uint32_t mb = sh->first_mb;
    uint32_t mb_skip_run = 0;

    if (st == 0U) {
        mb_skip_run = bits_ue(b); /* P slice begins with mb_skip_run */
    }

    while (mb < total) {
        uint32_t mb_x = mb % c->mb_w, mb_y = mb / c->mb_w;

        if (st == 0U && mb_skip_run > 0) {
            /* P_Skip macroblock: no residual, all-zero nnz, MV from prediction. */
            mb_skip_run--;
            c->dec->work.mb_qp[mb] = (uint8_t)c->qpy;
            c->dec->work.mb_intra[mb] = 0;
            c->dec->work.mb_type[mb] = 2;
            set_mb_imode_default(c, mb_x, mb_y);
            for (uint32_t by = 0; by < 2; by++)
                for (uint32_t bx = 0; bx < 2; bx++) {
                    c->dec->work.cnnz[0][(mb_y*2U+by)*c->cgw + (mb_x*2U+bx)] = 0;
                    c->dec->work.cnnz[1][(mb_y*2U+by)*c->cgw + (mb_x*2U+bx)] = 0;
                }
            for (uint32_t by = 0; by < 4; by++)
                for (uint32_t bx = 0; bx < 4; bx++)
                    c->dec->work.nnz[(mb_y*4U+by)*c->gw + (mb_x*4U+bx)] = 0;
            if (!recon_inter_mb(c, b, mb, mb_x, mb_y, 0xFFFFFFFFU, &mb_skip_run)) return 0;
            mb++;
            continue;
        }

        uint32_t mb_type = bits_ue(b);
        if (b->error) return 0;

        if (st == 0U) {
            /* P-slice mb_type: 0..4 inter, >=5 intra (offset by 5). */
            if (mb_type < 5U) {
                c->dec->work.mb_qp[mb] = (uint8_t)c->qpy;
                c->dec->work.mb_intra[mb] = 0;
                c->dec->work.mb_type[mb] = 2;
                set_mb_imode_default(c, mb_x, mb_y);
                if (!recon_inter_mb(c, b, mb, mb_x, mb_y, mb_type, &mb_skip_run)) return 0;
                mb++;
                if (mb < total) { mb_skip_run = bits_ue(b); }
                continue;
            }
            mb_type -= 5U; /* fall through as intra mb_type */
        }

        /* Intra macroblock (I-slice, or intra MB inside a P-slice). */
        if (mb_type == 25U) {
            h264_copy_str(c->dec->status_detail, sizeof(c->dec->status_detail),
                          "I_PCM not supported");
            return 0;
        }
        uint32_t is_i16 = 0, i16mode = 2, cbp_luma = 0, cbp_chroma = 0, chroma_mode = 0;
        uint8_t modes[16];
        for (uint32_t i = 0; i < 16; i++) modes[i] = 2;
        int32_t qp_delta = 0;
        if (mb_type == 0U) {
            parse_i4x4_modes(c, b, mb_x, mb_y, modes);
            chroma_mode = bits_ue(b);
            if (chroma_mode > 3U) return 0;
            uint32_t cbp = h264_cbp_intra(bits_ue(b));
            if (cbp == 0xFFFFFFFFU) return 0;
            cbp_luma = cbp & 0x0FU; cbp_chroma = (cbp >> 4) & 0x03U;
            if (cbp != 0U) qp_delta = bits_se(b);
        } else if (mb_type <= 24U) {
            uint32_t v = mb_type - 1U;
            is_i16 = 1;
            i16mode = v & 3U;
            cbp_chroma = (v >> 2) % 3U;
            cbp_luma = (v >= 12U) ? 15U : 0U;
            set_mb_imode_default(c, mb_x, mb_y);
            chroma_mode = bits_ue(b);
            if (chroma_mode > 3U) return 0;
            qp_delta = bits_se(b);
        } else {
            return 0;
        }
        if (b->error) return 0;
        c->qpy = (c->qpy + qp_delta + 52) % 52;
        c->dec->work.mb_qp[mb] = (uint8_t)c->qpy;
        c->dec->work.mb_intra[mb] = 1;
        c->dec->work.mb_type[mb] = 1;
        /* intra MBs in a P slice: no L0 motion; mark ref -1 for deblock/MV. */
        for (uint32_t by = 0; by < 4; by++)
            for (uint32_t bx = 0; bx < 4; bx++) {
                uint32_t bxg = mb_x*4U+bx, byg = mb_y*4U+by;
                c->dec->work.refidx[byg*c->gw+bxg] = -1;
                c->dec->work.mvx[byg*c->gw+bxg] = 0;
                c->dec->work.mvy[byg*c->gw+bxg] = 0;
            }
        if (!recon_intra_mb(c, b, mb_x, mb_y, is_i16, i16mode, modes,
                            cbp_luma, cbp_chroma, chroma_mode)) return 0;
        mb++;
        if (st == 0U && mb < total) { mb_skip_run = bits_ue(b); }
    }
    return 1;
}

/* ====================== Inter (P-slice) prediction ======================= */

/* H.264 Table 9-4 coded_block_pattern mapping for Inter macroblocks. */
static uint32_t h264_cbp_inter(uint32_t code_num)
{
    static const uint8_t map[48] = {
         0, 16,  1,  2,  4,  8, 32,  3,  5, 10, 12, 15,
        47,  7, 11, 13, 14,  6,  9, 31, 35, 37, 42, 44,
        33, 34, 36, 40, 39, 43, 45, 46, 17, 18, 20, 24,
        19, 21, 26, 28, 23, 27, 29, 30, 22, 25, 38, 41
    };
    return code_num < 48U ? map[code_num] : 0xFFFFFFFFU;
}

static int32_t med3(int32_t a, int32_t b, int32_t c)
{
    int32_t mn = a < b ? a : b; if (c < mn) mn = c;
    int32_t mx = a > b ? a : b; if (c > mx) mx = c;
    return a + b + c - mn - mx;
}

/* Clamped full-pel luma sample fetch. */
static int32_t lp(const uint8_t *r, uint32_t W, uint32_t H, int32_t x, int32_t y)
{
    if (x < 0) x = 0; else if (x >= (int32_t)W) x = (int32_t)W - 1;
    if (y < 0) y = 0; else if (y >= (int32_t)H) y = (int32_t)H - 1;
    return r[y * (int32_t)W + x];
}

/* Horizontal 6-tap (un-normalised) at integer (X,Y). */
static int32_t htap(const uint8_t *r, uint32_t W, uint32_t H, int32_t X, int32_t Y)
{
    return lp(r,W,H,X-2,Y) - 5*lp(r,W,H,X-1,Y) + 20*lp(r,W,H,X,Y)
         + 20*lp(r,W,H,X+1,Y) - 5*lp(r,W,H,X+2,Y) + lp(r,W,H,X+3,Y);
}
static int32_t vtap(const uint8_t *r, uint32_t W, uint32_t H, int32_t X, int32_t Y)
{
    return lp(r,W,H,X,Y-2) - 5*lp(r,W,H,X,Y-1) + 20*lp(r,W,H,X,Y)
         + 20*lp(r,W,H,X,Y+1) - 5*lp(r,W,H,X,Y+2) + lp(r,W,H,X,Y+3);
}

/* Luma quarter-pel sample at integer base (X,Y) and frac (fx,fy) in [0,3]. */
static int32_t luma_sample(const uint8_t *r, uint32_t W, uint32_t H,
                           int32_t X, int32_t Y, int32_t fx, int32_t fy)
{
    if (fx == 0 && fy == 0) return lp(r,W,H,X,Y);
    int32_t G = lp(r,W,H,X,Y);
    int32_t b = h264_clip_u8((htap(r,W,H,X,Y) + 16) >> 5);
    int32_t h = h264_clip_u8((vtap(r,W,H,X,Y) + 16) >> 5);
    if (fy == 0) { /* row: a,b,c */
        if (fx == 2) return b;
        if (fx == 1) return (G + b + 1) >> 1;
        return (b + lp(r,W,H,X+1,Y) + 1) >> 1; /* c */
    }
    if (fx == 0) { /* col: d,h,n */
        if (fy == 2) return h;
        if (fy == 1) return (G + h + 1) >> 1;
        return (h + lp(r,W,H,X,Y+1) + 1) >> 1; /* n */
    }
    /* center column j (always needed for the remaining positions) */
    int32_t j1 = htap(r,W,H,X,Y-2) - 5*htap(r,W,H,X,Y-1) + 20*htap(r,W,H,X,Y)
               + 20*htap(r,W,H,X,Y+1) - 5*htap(r,W,H,X,Y+2) + htap(r,W,H,X,Y+3);
    int32_t j = h264_clip_u8((j1 + 512) >> 10);
    if (fx == 2 && fy == 2) return j;
    int32_t m = h264_clip_u8((vtap(r,W,H,X+1,Y) + 16) >> 5);  /* vert half, col+1 */
    int32_t s = h264_clip_u8((htap(r,W,H,X,Y+1) + 16) >> 5);  /* horiz half, row+1 */
    int32_t idx = fy * 4 + fx;
    switch (idx) {
    case 5:  return (b + h + 1) >> 1; /* e */
    case 6:  return (b + j + 1) >> 1; /* f */
    case 7:  return (b + m + 1) >> 1; /* g */
    case 9:  return (h + j + 1) >> 1; /* i */
    case 11: return (j + m + 1) >> 1; /* k */
    case 13: return (h + s + 1) >> 1; /* p */
    case 14: return (j + s + 1) >> 1; /* q */
    case 15: return (m + s + 1) >> 1; /* r */
    default: return j;
    }
}

/* Motion-compensate a luma partition into the frame Y plane. */
static void mc_luma(FrameCtx *c, const uint8_t *refY, uint32_t px, uint32_t py,
                    uint32_t pw, uint32_t ph, int32_t mvx, int32_t mvy)
{
    uint32_t W = c->cw, H = c->ch;
    if (((mvx | mvy) & 3) == 0) {
        int32_t sx0 = (int32_t)px + (mvx >> 2);
        int32_t sy0 = (int32_t)py + (mvy >> 2);
        if (sx0 >= 0 && sy0 >= 0 &&
            sx0 + (int32_t)pw <= (int32_t)W &&
            sy0 + (int32_t)ph <= (int32_t)H) {
            for (uint32_t y = 0; y < ph; y++) {
                h264_copy_bytes(c->y + (py + y) * W + px,
                                refY + ((uint32_t)sy0 + y) * W + (uint32_t)sx0,
                                pw);
            }
            return;
        }
    }
    for (uint32_t y = 0; y < ph; y++) {
        for (uint32_t x = 0; x < pw; x++) {
            int32_t sx = (int32_t)(px + x) * 4 + mvx;
            int32_t sy = (int32_t)(py + y) * 4 + mvy;
            int32_t X = sx >> 2, Y = sy >> 2, fx = sx & 3, fy = sy & 3;
            c->y[(py + y) * W + (px + x)] = (uint8_t)luma_sample(refY, W, H, X, Y, fx, fy);
        }
    }
}

/* Motion-compensate a chroma partition (one component) via 1/8-pel bilinear. */
static void mc_chroma(FrameCtx *c, uint8_t *dstC, const uint8_t *refC,
                      uint32_t px, uint32_t py, uint32_t pw, uint32_t ph,
                      int32_t mvx, int32_t mvy)
{
    uint32_t W = c->ccw, H = c->cch;
    if (((mvx | mvy) & 7) == 0) {
        int32_t sx0 = (int32_t)px + (mvx >> 3);
        int32_t sy0 = (int32_t)py + (mvy >> 3);
        if (sx0 >= 0 && sy0 >= 0 &&
            sx0 + (int32_t)pw <= (int32_t)W &&
            sy0 + (int32_t)ph <= (int32_t)H) {
            for (uint32_t y = 0; y < ph; y++) {
                h264_copy_bytes(dstC + (py + y) * W + px,
                                refC + ((uint32_t)sy0 + y) * W + (uint32_t)sx0,
                                pw);
            }
            return;
        }
    }
    for (uint32_t y = 0; y < ph; y++) {
        for (uint32_t x = 0; x < pw; x++) {
            int32_t sx = (int32_t)(px + x) * 8 + mvx;
            int32_t sy = (int32_t)(py + y) * 8 + mvy;
            int32_t X = sx >> 3, Y = sy >> 3, fx = sx & 7, fy = sy & 7;
            int32_t x0 = X, x1 = X + 1, y0 = Y, y1 = Y + 1;
            if (x0 < 0) x0 = 0; else if (x0 >= (int32_t)W) x0 = (int32_t)W - 1;
            if (x1 < 0) x1 = 0; else if (x1 >= (int32_t)W) x1 = (int32_t)W - 1;
            if (y0 < 0) y0 = 0; else if (y0 >= (int32_t)H) y0 = (int32_t)H - 1;
            if (y1 < 0) y1 = 0; else if (y1 >= (int32_t)H) y1 = (int32_t)H - 1;
            int32_t A = refC[y0 * W + x0], Bv = refC[y0 * W + x1];
            int32_t Cv = refC[y1 * W + x0], D = refC[y1 * W + x1];
            int32_t v = ((8 - fx) * (8 - fy) * A + fx * (8 - fy) * Bv
                       + (8 - fx) * fy * Cv + fx * fy * D + 32) >> 6;
            dstC[(py + y) * W + (px + x)] = (uint8_t)v;
        }
    }
}

/* Fetch a neighbour 4x4 block's MV/ref with H.264 availability. */
static void mvnb(FrameCtx *c, int32_t bxg, int32_t byg, uint32_t mb_x, uint32_t mb_y,
                 const uint8_t filled[16], int32_t *mvx, int32_t *mvy, int32_t *ref,
                 int32_t *avail)
{
    *mvx = 0; *mvy = 0; *ref = -1; *avail = 0;
    if (bxg < 0 || byg < 0 || bxg >= (int32_t)c->gw) return;
    int32_t MX = bxg / 4, MY = byg / 4;
    int32_t av;
    if (MY < (int32_t)mb_y) av = 1;
    else if (MY == (int32_t)mb_y && MX < (int32_t)mb_x) av = 1;
    else if (MX == (int32_t)mb_x && MY == (int32_t)mb_y)
        av = filled[(byg - (int32_t)mb_y * 4) * 4 + (bxg - (int32_t)mb_x * 4)];
    else av = 0;
    if (!av) return;
    *avail = 1;
    int32_t idx = byg * (int32_t)c->gw + bxg;
    *mvx = c->dec->work.mvx[idx];
    *mvy = c->dec->work.mvy[idx];
    *ref = c->dec->work.refidx[idx];
}

/* Median MV predictor for a partition (shape: 0 normal, 1 16x8, 2 8x16). */
static void predict_mv(FrameCtx *c, uint32_t mb_x, uint32_t mb_y,
                       int32_t x4g, int32_t y4g, int32_t w4, int32_t ref,
                       const uint8_t filled[16], int32_t shape, int32_t part_idx,
                       int32_t *omvx, int32_t *omvy)
{
    int32_t axv, ayv, ar, aa, bxv, byv, br, ba, cxv, cyv, cr, ca, dxv, dyv, dr, da;
    mvnb(c, x4g - 1, y4g, mb_x, mb_y, filled, &axv, &ayv, &ar, &aa);
    mvnb(c, x4g, y4g - 1, mb_x, mb_y, filled, &bxv, &byv, &br, &ba);
    mvnb(c, x4g + w4, y4g - 1, mb_x, mb_y, filled, &cxv, &cyv, &cr, &ca);
    mvnb(c, x4g - 1, y4g - 1, mb_x, mb_y, filled, &dxv, &dyv, &dr, &da);
    if (!ca) { cxv = dxv; cyv = dyv; cr = dr; ca = da; }
    if (!ba && !ca && aa) { bxv = cxv = axv; byv = cyv = ayv; br = cr = ar; ba = ca = 1; }

    if (shape == 1) { /* 16x8 */
        if (part_idx == 0 && ba && br == ref) { *omvx = bxv; *omvy = byv; return; }
        if (part_idx == 1 && aa && ar == ref) { *omvx = axv; *omvy = ayv; return; }
    } else if (shape == 2) { /* 8x16 */
        if (part_idx == 0 && aa && ar == ref) { *omvx = axv; *omvy = ayv; return; }
        if (part_idx == 1 && ca && cr == ref) { *omvx = cxv; *omvy = cyv; return; }
    }
    int32_t na = (aa && ar == ref), nb = (ba && br == ref), nc = (ca && cr == ref);
    if (na + nb + nc == 1) {
        if (na) { *omvx = axv; *omvy = ayv; }
        else if (nb) { *omvx = bxv; *omvy = byv; }
        else { *omvx = cxv; *omvy = cyv; }
        return;
    }
    *omvx = med3(axv, bxv, cxv);
    *omvy = med3(ayv, byv, cyv);
}

/* Write a partition's MV/ref into the work grids and the per-MB filled mask. */
static void fill_part(FrameCtx *c, uint32_t mb_x, uint32_t mb_y, int32_t x4g,
                      int32_t y4g, int32_t w4, int32_t h4, int32_t mvx,
                      int32_t mvy, int32_t ref, uint8_t filled[16])
{
    for (int32_t yy = 0; yy < h4; yy++)
        for (int32_t xx = 0; xx < w4; xx++) {
            int32_t bxg = x4g + xx, byg = y4g + yy;
            int32_t idx = byg * (int32_t)c->gw + bxg;
            c->dec->work.mvx[idx] = (int16_t)mvx;
            c->dec->work.mvy[idx] = (int16_t)mvy;
            c->dec->work.refidx[idx] = (int8_t)ref;
            filled[(byg - (int32_t)mb_y * 4) * 4 + (bxg - (int32_t)mb_x * 4)] = 1;
        }
}

/* Decode chroma residual and ADD it to the (already motion-compensated) planes. */
static uint32_t chroma_residual_add(FrameCtx *c, H264Bits *b, uint32_t mb_x,
                                    uint32_t mb_y, uint32_t cbp_chroma, int32_t qpc)
{
    uint8_t *plane[2] = { c->cb, c->cr };
    uint32_t cx0 = mb_x * 8U, cy0 = mb_y * 8U;
    int16_t dc[2][4];
    int16_t ac[2][4][16];
    for (uint32_t comp = 0; comp < 2; comp++)
        for (uint32_t blk = 0; blk < 4; blk++)
            for (uint32_t i = 0; i < 16; i++) ac[comp][blk][i] = 0;
    for (uint32_t comp = 0; comp < 2; comp++)
        for (uint32_t i = 0; i < 4; i++) dc[comp][i] = 0;

    if (cbp_chroma != 0) {
        for (uint32_t comp = 0; comp < 2; comp++) {
            uint32_t tc = 0;
            if (!decode_chroma_dc_block(b, dc[comp], &tc)) return 0;
        }
    }
    if (cbp_chroma == 2) {
        for (uint32_t comp = 0; comp < 2; comp++)
            for (uint32_t blk = 0; blk < 4; blk++) {
                uint32_t bx = blk & 1U, by = blk >> 1;
                int32_t nc = chroma_nc(c, comp, mb_x, mb_y, bx, by);
                uint32_t tc = 0;
                if (!decode_residual_4x4_ac(b, nc, ac[comp][blk], &tc)) return 0;
                c->dec->work.cnnz[comp][(mb_y*2U+by)*c->cgw + (mb_x*2U+bx)] = (uint8_t)tc;
            }
    } else {
        for (uint32_t comp = 0; comp < 2; comp++)
            for (uint32_t blk = 0; blk < 4; blk++) {
                uint32_t bx = blk & 1U, by = blk >> 1;
                c->dec->work.cnnz[comp][(mb_y*2U+by)*c->cgw + (mb_x*2U+bx)] = 0;
            }
    }
    for (uint32_t comp = 0; comp < 2; comp++) {
        int32_t dcq[4];
        if (cbp_chroma != 0) chroma_dc_dequant(dc[comp], qpc, dcq);
        else { dcq[0]=dcq[1]=dcq[2]=dcq[3]=0; }
        for (uint32_t blk = 0; blk < 4; blk++) {
            uint32_t bx = blk & 1U, by = blk >> 1;
            int32_t res[16];
            dequant_idct_4x4_i16(ac[comp][blk], qpc, dcq[blk], res);
            for (uint32_t yy = 0; yy < 4; yy++)
                for (uint32_t xx = 0; xx < 4; xx++) {
                    uint32_t px = cx0 + bx*4U + xx, py = cy0 + by*4U + yy;
                    int32_t v = plane[comp][py * c->ccw + px] + res[yy*4+xx];
                    plane[comp][py * c->ccw + px] = (uint8_t)h264_clip_u8(v);
                }
        }
    }
    return 1;
}

static void mark_mb_decoded(FrameCtx *c, uint32_t mb_x, uint32_t mb_y)
{
    uint32_t x0 = mb_x * 16U;
    uint32_t y0 = mb_y * 16U;
    uint32_t right = x0 + 15U;
    uint32_t bottom = y0 + 15U;
    for (uint32_t x = 0; x < 16U; x++) {
        c->decmask[bottom * c->cw + x0 + x] = 1;
    }
    for (uint32_t y = 0; y < 15U; y++) {
        c->decmask[(y0 + y) * c->cw + right] = 1;
    }
}

/* Decode one inter (P) macroblock. mb_type_p == 0xFFFFFFFF means P_Skip. */
static uint32_t recon_inter_mb(FrameCtx *c, H264Bits *b, uint32_t mb,
                               uint32_t mb_x, uint32_t mb_y, uint32_t mb_type_p,
                               uint32_t *mb_skip_run)
{
    (void)mb; (void)mb_skip_run;
    if (c->dec->ref_count == 0) return 0; /* P with no reference */
    uint8_t filled[16];
    for (uint32_t i = 0; i < 16; i++) filled[i] = 0;
    uint32_t mb_x4 = mb_x * 4U, mb_y4 = mb_y * 4U;

    /* ---- P_Skip ---- */
    if (mb_type_p == 0xFFFFFFFFU) {
        int32_t ax, ay, ar, aa, bx, by2, br, ba, cx, cy, cr, ca, dx, dy, dr, da;
        mvnb(c, (int32_t)mb_x4 - 1, (int32_t)mb_y4, mb_x, mb_y, filled, &ax,&ay,&ar,&aa);
        mvnb(c, (int32_t)mb_x4, (int32_t)mb_y4 - 1, mb_x, mb_y, filled, &bx,&by2,&br,&ba);
        mvnb(c, (int32_t)mb_x4 + 4, (int32_t)mb_y4 - 1, mb_x, mb_y, filled, &cx,&cy,&cr,&ca);
        mvnb(c, (int32_t)mb_x4 - 1, (int32_t)mb_y4 - 1, mb_x, mb_y, filled, &dx,&dy,&dr,&da);
        int32_t mvx, mvy;
        if (!aa || !ba || (ar == 0 && ax == 0 && ay == 0) || (br == 0 && bx == 0 && by2 == 0)) {
            mvx = 0; mvy = 0;
        } else {
            predict_mv(c, mb_x, mb_y, (int32_t)mb_x4, (int32_t)mb_y4, 4, 0, filled, 0, 0, &mvx, &mvy);
        }
        fill_part(c, mb_x, mb_y, (int32_t)mb_x4, (int32_t)mb_y4, 4, 4, mvx, mvy, 0, filled);
        const H264Frame *rf = c->dec->refs[0];
        mc_luma(c, rf->y, mb_x*16U, mb_y*16U, 16, 16, mvx, mvy);
        mc_chroma(c, c->cb, rf->cb, mb_x*8U, mb_y*8U, 8, 8, mvx, mvy);
        mc_chroma(c, c->cr, rf->cr, mb_x*8U, mb_y*8U, 8, 8, mvx, mvy);
        mark_mb_decoded(c, mb_x, mb_y);
        return 1;
    }

    /* ---- Coded P macroblock: build partitions ---- */
    uint32_t sub[4] = {0,0,0,0};
    uint32_t is_p8x8 = (mb_type_p == 3U || mb_type_p == 4U);
    uint32_t ref0_only = (mb_type_p == 4U);
    if (is_p8x8) {
        for (uint32_t i = 0; i < 4; i++) {
            sub[i] = bits_ue(b);
            if (sub[i] > 3U) return 0;
        }
    }
    /* number of ref_idx entries */
    int32_t refidx[4] = {0,0,0,0};
    uint32_t nref_entries = (mb_type_p == 0U) ? 1U : (mb_type_p <= 2U) ? 2U : 4U;
    uint32_t parse_ref = (c->num_ref_idx_l0 > 1U) && !ref0_only;
    for (uint32_t i = 0; i < nref_entries; i++) {
        if (parse_ref) {
            if (c->num_ref_idx_l0 == 2U) refidx[i] = bits_read(b, 1) ? 0 : 1;
            else refidx[i] = (int32_t)bits_ue(b);
        } else refidx[i] = 0;
        if (refidx[i] < 0 || refidx[i] >= (int32_t)c->dec->ref_count) {
            if (refidx[i] >= (int32_t)c->dec->ref_count) return 0;
        }
    }

    /* Geometry + MV decode per partition, in bitstream order. */
    if (mb_type_p == 0U) {
        int32_t pmx, pmy;
        predict_mv(c, mb_x, mb_y, (int32_t)mb_x4, (int32_t)mb_y4, 4, refidx[0], filled, 0, 0, &pmx, &pmy);
        int32_t mvx = pmx + bits_se(b), mvy = pmy + bits_se(b);
        fill_part(c, mb_x, mb_y, (int32_t)mb_x4, (int32_t)mb_y4, 4, 4, mvx, mvy, refidx[0], filled);
        const H264Frame *rf = c->dec->refs[refidx[0]];
        mc_luma(c, rf->y, mb_x*16U, mb_y*16U, 16, 16, mvx, mvy);
        mc_chroma(c, c->cb, rf->cb, mb_x*8U, mb_y*8U, 8, 8, mvx, mvy);
        mc_chroma(c, c->cr, rf->cr, mb_x*8U, mb_y*8U, 8, 8, mvx, mvy);
    } else if (mb_type_p == 1U) { /* 16x8 */
        for (uint32_t pidx = 0; pidx < 2; pidx++) {
            int32_t y4 = (int32_t)mb_y4 + (int32_t)pidx * 2;
            int32_t pmx, pmy;
            predict_mv(c, mb_x, mb_y, (int32_t)mb_x4, y4, 4, refidx[pidx], filled, 1, (int32_t)pidx, &pmx, &pmy);
            int32_t mvx = pmx + bits_se(b), mvy = pmy + bits_se(b);
            fill_part(c, mb_x, mb_y, (int32_t)mb_x4, y4, 4, 2, mvx, mvy, refidx[pidx], filled);
            const H264Frame *rf = c->dec->refs[refidx[pidx]];
            mc_luma(c, rf->y, mb_x*16U, (uint32_t)y4*4U, 16, 8, mvx, mvy);
            mc_chroma(c, c->cb, rf->cb, mb_x*8U, (uint32_t)y4*2U, 8, 4, mvx, mvy);
            mc_chroma(c, c->cr, rf->cr, mb_x*8U, (uint32_t)y4*2U, 8, 4, mvx, mvy);
        }
    } else if (mb_type_p == 2U) { /* 8x16 */
        for (uint32_t pidx = 0; pidx < 2; pidx++) {
            int32_t x4 = (int32_t)mb_x4 + (int32_t)pidx * 2;
            int32_t pmx, pmy;
            predict_mv(c, mb_x, mb_y, x4, (int32_t)mb_y4, 2, refidx[pidx], filled, 2, (int32_t)pidx, &pmx, &pmy);
            int32_t mvx = pmx + bits_se(b), mvy = pmy + bits_se(b);
            fill_part(c, mb_x, mb_y, x4, (int32_t)mb_y4, 2, 4, mvx, mvy, refidx[pidx], filled);
            const H264Frame *rf = c->dec->refs[refidx[pidx]];
            mc_luma(c, rf->y, (uint32_t)x4*4U, mb_y*16U, 8, 16, mvx, mvy);
            mc_chroma(c, c->cb, rf->cb, (uint32_t)x4*2U, mb_y*8U, 4, 8, mvx, mvy);
            mc_chroma(c, c->cr, rf->cr, (uint32_t)x4*2U, mb_y*8U, 4, 8, mvx, mvy);
        }
    } else { /* P_8x8 / P_8x8ref0 */
        for (uint32_t q = 0; q < 4; q++) {
            int32_t qx4 = (int32_t)mb_x4 + (int32_t)(q & 1U) * 2;
            int32_t qy4 = (int32_t)mb_y4 + (int32_t)(q >> 1) * 2;
            int32_t ref = refidx[q];
            const H264Frame *rf = c->dec->refs[ref];
            uint32_t smt = sub[q];
            /* subpartitions: list of (sx4,sy4,sw4,sh4) */
            int32_t nspw = (smt == 2U || smt == 3U) ? 2 : 1; /* cols */
            int32_t nsph = (smt == 1U || smt == 3U) ? 2 : 1; /* rows */
            int32_t sw4 = 2 / nspw, sh4 = 2 / nsph;
            for (int32_t sy = 0; sy < nsph; sy++) {
                for (int32_t sx = 0; sx < nspw; sx++) {
                    int32_t sx4 = qx4 + sx * sw4, sy4 = qy4 + sy * sh4;
                    int32_t pmx, pmy;
                    predict_mv(c, mb_x, mb_y, sx4, sy4, sw4, ref, filled, 0, 0, &pmx, &pmy);
                    int32_t mvx = pmx + bits_se(b), mvy = pmy + bits_se(b);
                    fill_part(c, mb_x, mb_y, sx4, sy4, sw4, sh4, mvx, mvy, ref, filled);
                    mc_luma(c, rf->y, (uint32_t)sx4*4U, (uint32_t)sy4*4U,
                            (uint32_t)sw4*4U, (uint32_t)sh4*4U, mvx, mvy);
                    mc_chroma(c, c->cb, rf->cb, (uint32_t)sx4*2U, (uint32_t)sy4*2U,
                              (uint32_t)sw4*2U, (uint32_t)sh4*2U, mvx, mvy);
                    mc_chroma(c, c->cr, rf->cr, (uint32_t)sx4*2U, (uint32_t)sy4*2U,
                              (uint32_t)sw4*2U, (uint32_t)sh4*2U, mvx, mvy);
                }
            }
        }
    }
    if (b->error) return 0;

    /* ---- Residual ---- */
    uint32_t cbp = h264_cbp_inter(bits_ue(b));
    if (cbp == 0xFFFFFFFFU) return 0;
    uint32_t cbp_luma = cbp & 0x0FU, cbp_chroma = (cbp >> 4) & 0x03U;
    int32_t qp_delta = 0;
    if (cbp != 0U) qp_delta = bits_se(b);
    if (b->error) return 0;
    c->qpy = (c->qpy + qp_delta + 52) % 52;
    c->dec->work.mb_qp[mb] = (uint8_t)c->qpy;

    uint32_t mb_x0 = mb_x * 16U, mb_y0 = mb_y * 16U, S = c->cw;
    for (uint32_t blk = 0; blk < 16; blk++) {
        uint32_t lx = h264_blk_x[blk], ly = h264_blk_y[blk];
        uint32_t bx = lx >> 2, by = ly >> 2;
        int16_t coeff[16];
        for (uint32_t i = 0; i < 16; i++) coeff[i] = 0;
        uint32_t tc = 0, t1 = 0, tz = 0, rn = 0;
        if (cbp_luma & (1U << (blk >> 2))) {
            int32_t nc = luma_nc(c, mb_x, mb_y, bx, by);
            if (!decode_residual_4x4(b, nc, coeff, &tc, &t1, &tz, &rn)) return 0;
        }
        c->dec->work.nnz[(mb_y*4U+by)*c->gw + (mb_x*4U+bx)] = (uint8_t)tc;
        if (tc > 0) {
            int32_t res[16];
            dequant_idct_4x4(coeff, c->qpy, res);
            for (uint32_t yy = 0; yy < 4; yy++)
                for (uint32_t xx = 0; xx < 4; xx++) {
                    uint32_t px = mb_x0 + lx + xx, py = mb_y0 + ly + yy;
                    int32_t v = c->y[py * S + px] + res[yy*4+xx];
                    c->y[py * S + px] = (uint8_t)h264_clip_u8(v);
                }
        }
    }
    int32_t qpc = h264_chroma_qp(c->qpy, c->dec->chroma_qp_index_offset);
    if (!chroma_residual_add(c, b, mb_x, mb_y, cbp_chroma, qpc)) return 0;
    mark_mb_decoded(c, mb_x, mb_y);
    return 1;
}

/* ---- Deblocking (stub for now; replaced by the deblocking module) -------- */
static void deblock_frame(FrameCtx *c, const SliceHeader *sh)
{
    (void)c; (void)sh;
}

/* ---- Frame buffer + reference management -------------------------------- */
static H264Frame *h264_get_free_frame(H264Decoder *dec)
{
    for (uint32_t i = 0; i < dec->frame_pool_size; i++) {
        H264Frame *f = &dec->frames[i];
        uint32_t used = 0;
        for (uint32_t r = 0; r < dec->ref_count; r++)
            if (dec->refs[r] == f) { used = 1; break; }
        if (!used) return f;
    }
    return &dec->frames[0];
}

static void h264_add_reference(H264Decoder *dec, H264Frame *f)
{
    /* Sliding window: insert most-recent at front, drop oldest beyond max. */
    if (dec->ref_count > dec->max_refs) dec->ref_count = dec->max_refs;
    uint32_t n = dec->ref_count;
    if (n > dec->max_refs - 1) n = dec->max_refs - 1;
    if (dec->max_refs == 0) { dec->ref_count = 0; return; }
    for (uint32_t i = (n < dec->max_refs ? n : dec->max_refs - 1); i > 0; i--)
        dec->refs[i] = dec->refs[i - 1];
    dec->refs[0] = f;
    dec->ref_count = (dec->ref_count + 1 > dec->max_refs) ? dec->max_refs : dec->ref_count + 1;
}

/* ---- Decode one coded sample into a full YUV420 frame -------------------- */
uint32_t h264_decode_sample(H264Decoder *dec, const uint8_t *sample,
                            uint32_t sample_size, uint8_t nal_length_size,
                            H264Frame **out)
{
    if (out) *out = 0;
    if (!dec->ready) return dec->status ? dec->status : H264_ERR_UNSUPPORTED;
    if (nal_length_size == 0 || nal_length_size > 4) return H264_ERR_BAD_BITSTREAM;

    H264Frame *cur = h264_get_free_frame(dec);
    FrameCtx ctx;
    h264_zero(&ctx, sizeof(ctx));
    ctx.dec = dec;
    ctx.y = cur->y; ctx.cb = cur->cb; ctx.cr = cur->cr;
    ctx.decmask = dec->work.decmask;
    ctx.cw = dec->width; ctx.ch = dec->height;
    ctx.ccw = dec->width / 2U; ctx.cch = dec->height / 2U;
    ctx.mb_w = dec->mb_width; ctx.mb_h = dec->mb_height;
    ctx.gw = dec->mb_width * 4U;
    ctx.cgw = dec->mb_width * 2U;

    /* Clear per-frame working state. */
    uint32_t nblk = (dec->mb_width * 4U) * (dec->mb_height * 4U);
    uint32_t ncblk = (dec->mb_width * 2U) * (dec->mb_height * 2U);
    for (uint32_t i = 0; i < nblk; i++) {
        dec->work.nnz[i] = 0; dec->work.imode[i] = 2;
        dec->work.mvx[i] = 0; dec->work.mvy[i] = 0; dec->work.refidx[i] = -1;
    }
    for (uint32_t i = 0; i < ncblk; i++) { dec->work.cnnz[0][i] = 0; dec->work.cnnz[1][i] = 0; }
    for (uint32_t i = 0; i < dec->mb_width * dec->mb_height; i++) {
        dec->work.mb_type[i] = 0; dec->work.mb_qp[i] = 0; dec->work.mb_intra[i] = 0;
    }
    for (uint32_t i = 0; i < ctx.cw * ctx.ch; i++) ctx.decmask[i] = 0;

    uint32_t pos = 0;
    uint32_t decoded_slice = 0;
    SliceHeader sh; h264_zero(&sh, sizeof(sh));
    while (pos + nal_length_size <= sample_size) {
        uint32_t len = 0;
        for (uint32_t i = 0; i < nal_length_size; i++) len = (len << 8) | sample[pos + i];
        pos += nal_length_size;
        if (len == 0 || pos + len > sample_size) return H264_ERR_BAD_BITSTREAM;
        uint8_t nal_type = sample[pos] & 0x1FU;
        uint8_t nal_ref_idc = (sample[pos] >> 5) & 3U;
        if (nal_type == 1U || nal_type == 5U) {
            if (nal_type == 5U && !decoded_slice) {
                dec->ref_count = 0; /* IDR clears references */
            }
            H264Bits b;
            bits_init(&b, sample + pos + 1, len - 1);
            parse_slice_hdr(dec, &b, nal_type, nal_ref_idc, &sh);
            if (!sh.ok) return H264_ERR_BAD_BITSTREAM;
            uint32_t st = sh.slice_type % 5U;
            if (st != 0U && st != 2U) {
                h264_copy_str(dec->status_detail, sizeof(dec->status_detail),
                              "only I and P slices supported");
                return H264_ERR_UNSUPPORTED;
            }
            ctx.slice_type = sh.slice_type;
            ctx.num_ref_idx_l0 = sh.num_ref_idx_l0;
            ctx.qpy = 26 + dec->pic_init_qp_minus26 + sh.slice_qp_delta;
            if (ctx.qpy < 0 || ctx.qpy > 51) return H264_ERR_BAD_BITSTREAM;
            if (!decode_slice_mbs(&ctx, &b, &sh)) {
                return dec->status && dec->status != H264_OK ? dec->status : H264_ERR_BAD_BITSTREAM;
            }
            decoded_slice = 1;
        }
        pos += len;
    }
    if (!decoded_slice) return H264_ERR_NO_SLICE;

    deblock_frame(&ctx, &sh);

    cur->width = dec->width; cur->height = dec->height;
    cur->cwidth = ctx.ccw; cur->cheight = ctx.cch;
    cur->valid = 1;
    cur->frame_num = (int32_t)sh.frame_num;
    dec->frames_decoded++;
    dec->last_frame_num = (int32_t)sh.frame_num;
    if (sh.nal_ref_idc != 0) h264_add_reference(dec, cur);
    if (out) *out = cur;
    dec->status = H264_OK;
    return H264_OK;
}

/* ---- YUV420 -> packed XRGB (BT.601 limited range), nearest-neighbour scale */
void h264_frame_to_xrgb(const H264Frame *f, uint32_t crop_w, uint32_t crop_h,
                        uint32_t *dst, uint32_t dst_w, uint32_t dst_h,
                        uint32_t dst_stride)
{
    if (!f || !f->valid || dst_w == 0 || dst_h == 0) return;
    if (crop_w == 0 || crop_w > f->width) crop_w = f->width;
    if (crop_h == 0 || crop_h > f->height) crop_h = f->height;
    for (uint32_t dy = 0; dy < dst_h; dy++) {
        uint32_t sy = (dy * crop_h) / dst_h;
        const uint8_t *yrow = f->y + sy * f->width;
        const uint8_t *cbrow = f->cb + (sy >> 1) * f->cwidth;
        const uint8_t *crrow = f->cr + (sy >> 1) * f->cwidth;
        uint32_t *drow = dst + dy * dst_stride;
        for (uint32_t dx = 0; dx < dst_w; dx++) {
            uint32_t sx = (dx * crop_w) / dst_w;
            int32_t Y = (int32_t)yrow[sx] - 16;
            int32_t U = (int32_t)cbrow[sx >> 1] - 128;
            int32_t V = (int32_t)crrow[sx >> 1] - 128;
            int32_t r = (298 * Y + 409 * V + 128) >> 8;
            int32_t g = (298 * Y - 100 * U - 208 * V + 128) >> 8;
            int32_t bch = (298 * Y + 516 * U + 128) >> 8;
            r = h264_clip_u8(r); g = h264_clip_u8(g); bch = h264_clip_u8(bch);
            drow[dx] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bch;
        }
    }
}
