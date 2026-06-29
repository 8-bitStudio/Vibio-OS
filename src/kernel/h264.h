/* Vibio OS - minimal H.264 Constrained Baseline stream probe / luma decoder.
 *
 * Scope: parse SPS/PPS and the first IDR slice header and reconstruct the full
 * first IDR frame's luma plane (all macroblocks, bit-exact vs ffmpeg). It still
 * does not reconstruct chroma, produce a full YUV frame, convert to RGB, decode
 * inter (P) frames, or play MP4.
 */
#ifndef VIBIO_H264_H
#define VIBIO_H264_H

#include <stdint.h>

#include "media_codec.h"

#define H264_OK 0
#define H264_ERR_NO_SPS 1
#define H264_ERR_NO_PPS 2
#define H264_ERR_BAD_BITSTREAM 3
#define H264_ERR_UNSUPPORTED 4
#define H264_ERR_NO_SLICE 5

#define H264_PROBE_MAX_ROW_MBS 20
#define H264_PROBE_MAX_ROW_WIDTH (H264_PROBE_MAX_ROW_MBS * 16)
#define H264_PROBE_MAX_MB_ROWS 12
#define H264_PROBE_MAX_REGION_HEIGHT (H264_PROBE_MAX_MB_ROWS * 16)
#define H264_PROBE_MAX_REGION_SAMPLES (H264_PROBE_MAX_ROW_WIDTH * H264_PROBE_MAX_REGION_HEIGHT)
#define H264_PROBE_MAX_ROW_SAMPLES (H264_PROBE_MAX_ROW_WIDTH * 16)

typedef struct {
    uint32_t status;
    char status_detail[80];

    uint8_t profile_idc;
    uint8_t constraint_flags;
    uint8_t level_idc;
    uint32_t width;
    uint32_t height;
    uint32_t mb_width;
    uint32_t mb_height;

    uint32_t log2_max_frame_num;
    uint32_t pic_order_cnt_type;
    uint32_t log2_max_pic_order_cnt_lsb;
    uint32_t max_num_ref_frames;
    uint8_t frame_mbs_only;
    uint8_t entropy_coding_mode_flag;
    uint8_t num_slice_groups;
    uint8_t deblocking_filter_control_present_flag;
    uint8_t redundant_pic_cnt_present_flag;
    int32_t pic_init_qp_minus26;

    uint8_t first_slice_nal_type;
    uint32_t first_mb_in_slice;
    uint32_t first_slice_type;
    uint32_t first_pps_id;
    uint32_t first_frame_num;
    uint32_t idr_pic_id;
    uint32_t pic_order_cnt_lsb;
    uint32_t sample_nal_count;
    uint8_t has_idr;
    uint8_t has_p_slice;

    uint8_t first_mb_header_ok;
    uint32_t first_mb_type;
    char first_mb_kind[16];
    uint32_t first_mb_intra_chroma_pred_mode;
    uint32_t first_mb_cbp_luma;
    uint32_t first_mb_cbp_chroma;
    int32_t first_mb_qp_delta;
    uint8_t first_mb_residual_present;
    uint8_t first_mb_first_residual_ok;
    uint32_t first_mb_first_residual_block;
    uint32_t first_mb_first_residual_total_coeff;
    uint32_t first_mb_first_residual_trailing_ones;
    uint32_t first_mb_first_residual_total_zeros;
    uint32_t first_mb_first_residual_runs;
    int16_t first_mb_first_residual_coeff[16];
    int32_t first_mb_first_residual_coeff_sum;
    uint32_t first_mb_first_residual_nonzero_mask;

    /* Step 2 pixel-pipeline gate: dequant + inverse transform + (DC) intra
     * prediction + reconstruction for the very first 4x4 luma block only.
     * These are real decoded values, not faked; everything is gated on the
     * first macroblock of the frame whose first 4x4 luma block uses DC intra
     * prediction (the only mode needing no neighbour samples). */
    int32_t slice_qp_delta;
    int32_t first_mb_qp;
    uint32_t first_block_intra4x4_mode;

    uint8_t first_block_dequant_ok;
    int16_t first_block_dequant[16];      /* raster 4x4 dequantized coefficients */
    int32_t first_block_dequant_sum;

    uint8_t first_block_idct_ok;
    int16_t first_block_residual[16];     /* raster 4x4 spatial residual samples */
    int32_t first_block_residual_sum;

    uint8_t first_block_pred_mode_supported; /* 1 only for honest DC-with-no-neighbours */
    uint32_t first_block_prediction_dc;      /* DC predictor value (128 when no neighbours) */

    uint8_t first_block_reconstructed_ok;
    uint8_t first_block_recon[16];        /* raster 4x4 final luma samples 0..255 */
    int32_t first_block_recon_sum;
    char first_block_detail[80];

    /* Macroblock 0 full-luma reconstruction (16 4x4 blocks): real decoded
     * samples, validated bit-exact against a reference decoder. */
    uint8_t mb0_luma_ok;
    uint8_t mb0_blocks_reconstructed;   /* how many of the 16 luma blocks done */
    uint8_t mb0_luma[256];              /* 16x16 reconstructed luma, row-major */
    uint8_t mb0_block_mode[16];         /* intra4x4 pred mode per block (scan order) */
    int32_t mb0_luma_sum;
    uint8_t mb0_luma_min;
    uint8_t mb0_luma_max;
    char mb0_detail[80];

    /* Bounded luma reconstruction gate. This stores only reconstructed luma
     * samples for the first IDR frame; chroma, RGB conversion, timing, and
     * playback are deliberately still outside the supported surface. */
    uint8_t luma_row_ok;
    uint8_t cross_mb_nc_ok;
    uint8_t cross_mb_intra_ok;
    uint8_t top_mb_nc_ok;
    uint8_t top_mb_intra_ok;
    uint8_t i16x16_luma_ok;
    uint8_t i16x16_dc_transform_ok;
    uint32_t i16x16_mbs_decoded;
    uint8_t full_frame_luma_ok;
    uint8_t chroma_reconstructed_ok;
    uint32_t luma_mbs_decoded;
    uint32_t luma_mb_rows_decoded;
    uint32_t luma_rows_decoded;
    uint32_t luma_samples_decoded;
    uint32_t luma_region_width;
    uint32_t luma_region_height;
    uint8_t luma_region[H264_PROBE_MAX_REGION_SAMPLES]; /* row-major luma */
    MediaCodecLumaRegion luma_region_info;
    int32_t luma_region_sum;
    uint8_t luma_region_min;
    uint8_t luma_region_max;
    uint32_t luma_blocker_mb;
    char luma_detail[80];

    uint8_t constrained_baseline_supported;
} H264Probe;

void h264_probe_avcc_and_sample(const uint8_t *sps, uint32_t sps_size,
                                const uint8_t *pps, uint32_t pps_size,
                                const uint8_t *sample, uint32_t sample_size,
                                uint8_t nal_length_size,
                                H264Probe *out);

const char *h264_status_text(uint32_t status);
const char *h264_slice_type_name(uint32_t slice_type);

/* ------------------------------------------------------------------------- *
 * Full-frame H.264 Constrained Baseline decoder (video-only).
 *
 * Decodes whole frames (I and P slices, intra + inter, in-loop deblocking)
 * into caller-provided YUV420 planar buffers, with a small reference-frame
 * ring also supplied by the caller. The decoder itself allocates nothing -
 * the kernel hands it page memory, exactly like the MP4 demuxer. This still
 * decodes video only; audio (AAC) is separate and not handled here.
 * ------------------------------------------------------------------------- */

/* Supported coded-frame ceiling for the per-block working arrays the caller
 * must size. 1920x1088 (120x68 macroblocks) covers up to 1080p. */
#define H264_MAX_MB_WIDTH   120u
#define H264_MAX_MB_HEIGHT  68u
#define H264_MAX_REF_FRAMES 4u

/* One decoded YUV420 frame. The three planes are caller-owned; the decoder
 * only writes into them. cstride/cheight describe the (w/2 x h/2) chroma. */
typedef struct {
    uint8_t *y;
    uint8_t *cb;
    uint8_t *cr;
    uint32_t width;     /* luma width in pixels (== mb_width*16, may exceed crop) */
    uint32_t height;    /* luma height in pixels (== mb_height*16) */
    uint32_t cwidth;    /* chroma width  (width/2) */
    uint32_t cheight;   /* chroma height (height/2) */
    uint8_t  valid;     /* 1 once a frame has been reconstructed into it */
    int32_t  frame_num; /* H.264 frame_num of the picture held here */
} H264Frame;

/* Per-4x4-block and per-macroblock working state for the frame in flight.
 * Caller supplies the backing arrays (sized for the actual frame) so the
 * codec stays allocation-free and KERNEL.BIN stays small. */
typedef struct {
    /* Per 4x4 block grid (mb_width*4 wide, mb_height*4 tall), raster order. */
    uint8_t  *nnz;        /* total_coeff per luma 4x4 block (for nC) */
    uint8_t  *imode;      /* intra4x4 pred mode per block (2 == DC default) */
    int16_t  *mvx;        /* L0 motion vector x (1/4 pel) per block */
    int16_t  *mvy;        /* L0 motion vector y (1/4 pel) per block */
    int8_t   *refidx;     /* L0 ref index per block (-1 == intra/unused) */
    /* Per-macroblock (mb_width wide, mb_height tall). */
    uint8_t  *mb_type;    /* 0 unset / 1 intra / 2 inter (for deblock + pred) */
    uint8_t  *mb_qp;      /* QPy used for the macroblock (deblock) */
    uint8_t  *mb_intra;   /* 1 if intra-coded macroblock */
    /* Chroma nnz per 2x2 chroma block grid (mb_width*2 wide, mb_height*2). */
    uint8_t  *cnnz[2];
    /* Luma-plane "reconstructed" mask (width*height) for intra availability. */
    uint8_t  *decmask;
} H264Work;

typedef struct {
    uint8_t  ready;             /* SPS/PPS parsed and supported */
    uint32_t status;            /* H264_OK or H264_ERR_* */
    char     status_detail[80];

    /* Sequence parameters mirrored from the probe parse. */
    uint8_t  profile_idc;
    uint8_t  constraint_flags;
    uint8_t  level_idc;
    uint32_t width, height;     /* coded luma dimensions (mb-aligned) */
    uint32_t crop_width, crop_height; /* displayable dimensions */
    uint32_t mb_width, mb_height;
    uint32_t log2_max_frame_num;
    uint32_t pic_order_cnt_type;
    uint32_t log2_max_pic_order_cnt_lsb;
    uint32_t max_num_ref_frames;
    uint8_t  frame_mbs_only;
    uint8_t  entropy_coding_mode_flag;
    uint8_t  deblocking_filter_control_present_flag;
    uint8_t  num_slice_groups;
    int32_t  pic_init_qp_minus26;
    int32_t  chroma_qp_index_offset;
    uint8_t  chroma_format_idc;

    /* Reference ring + work arrays are caller-provided. */
    H264Frame *frames;          /* pool of (max_refs+1) frame buffers */
    uint32_t   frame_pool_size;
    H264Work   work;

    /* Short-term reference list, most-recent first. */
    H264Frame *refs[H264_MAX_REF_FRAMES];
    uint32_t   ref_count;
    uint32_t   max_refs;

    uint32_t   frames_decoded;
    int32_t    last_frame_num;
} H264Decoder;

/* Parse SPS/PPS (avcC payloads) and prepare the decoder. The caller must have
 * filled dec->frames (a pool of at least max_num_ref_frames+1 H264Frame, each
 * with allocated planes large enough for the coded size) and dec->work arrays.
 * Returns H264_OK or an H264_ERR_* code (also stored in dec->status). */
uint32_t h264_decoder_init(H264Decoder *dec,
                           const uint8_t *sps, uint32_t sps_size,
                           const uint8_t *pps, uint32_t pps_size);

/* Decode one coded sample (avcC length-prefixed NAL units) into a full YUV420
 * frame, using/updating the reference ring. On success *out points at the
 * frame buffer holding the freshly decoded picture (owned by the pool, valid
 * until enough later frames recycle it). Returns H264_OK or an error. */
uint32_t h264_decode_sample(H264Decoder *dec,
                            const uint8_t *sample, uint32_t sample_size,
                            uint8_t nal_length_size,
                            H264Frame **out);

/* Convert a decoded YUV420 frame to packed 32-bit XRGB (0x00RRGGBB), scaling
 * (nearest) from the frame's crop size into dst_w x dst_h. BT.601 limited. */
void h264_frame_to_xrgb(const H264Frame *f, uint32_t crop_w, uint32_t crop_h,
                        uint32_t *dst, uint32_t dst_w, uint32_t dst_h,
                        uint32_t dst_stride);

#endif /* VIBIO_H264_H */
