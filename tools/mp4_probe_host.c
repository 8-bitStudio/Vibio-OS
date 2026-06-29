#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mp4.h"
#include "h264.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: mp4_probe_host <file.mp4>\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror(argv[1]);
        return 2;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 2;
    }
    long len = ftell(f);
    if (len <= 0) {
        fclose(f);
        return 2;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 2;
    }
    uint8_t *buf = (uint8_t *)malloc((size_t)len);
    if (!buf) {
        fclose(f);
        return 2;
    }
    size_t got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (got != (size_t)len) {
        free(buf);
        return 2;
    }

    Mp4Info info;
    int status = mp4_parse(buf, (uint32_t)len, &info);
    printf("status=%s detail=%s\n", mp4_status_text(info.status), info.status_detail);
    printf("brand=%s tracks=%u duration=%llu/%u\n",
           info.major_brand,
           (unsigned)info.track_count,
           (unsigned long long)info.movie_duration,
           (unsigned)info.movie_timescale);
    printf("video=%s %ux%u samples=%u chunks=%u demux=%u first_off=%llu first_size=%u first_dur=%u max=%u nal_len=%u profile=%s L%u.%u\n",
           mp4_codec_name(info.video.codec),
           (unsigned)info.video.width,
           (unsigned)info.video.height,
           (unsigned)info.video.sample_count,
           (unsigned)info.video.chunk_count,
           (unsigned)info.video.sample_demux_ready,
           (unsigned long long)info.video.first_sample_offset,
           (unsigned)info.video.first_sample_size,
           (unsigned)info.video.first_sample_duration,
           (unsigned)info.video.max_sample_size,
           (unsigned)info.video.h264_nal_length_size,
           mp4_h264_profile_name(info.video.h264_profile, info.video.h264_compat),
           (unsigned)(info.video.h264_level / 10),
           (unsigned)(info.video.h264_level % 10));
    printf("audio=%s %uHz %uch samples=%u chunks=%u demux=%u first_off=%llu first_size=%u first_dur=%u max=%u\n",
           mp4_codec_name(info.audio.codec),
           (unsigned)info.audio.sample_rate,
           (unsigned)info.audio.channels,
           (unsigned)info.audio.sample_count,
           (unsigned)info.audio.chunk_count,
           (unsigned)info.audio.sample_demux_ready,
           (unsigned long long)info.audio.first_sample_offset,
           (unsigned)info.audio.first_sample_size,
           (unsigned)info.audio.first_sample_duration,
           (unsigned)info.audio.max_sample_size);

    Mp4Sample s;
    if (mp4_get_sample(buf, (uint32_t)len, &info, MP4_TRACK_VIDEO, 0, &s)) {
        printf("video_sample0 off=%llu size=%u dts=%llu dur=%u\n",
               (unsigned long long)s.offset,
               (unsigned)s.size,
               (unsigned long long)s.dts,
               (unsigned)s.duration);
        if (info.video.codec == MP4_CODEC_H264 &&
            info.video.h264_sps_offset + info.video.h264_sps_size <= (uint32_t)len &&
            info.video.h264_pps_offset + info.video.h264_pps_size <= (uint32_t)len &&
            s.offset + s.size <= (uint64_t)len) {
            H264Probe hp;
            h264_probe_avcc_and_sample(buf + info.video.h264_sps_offset,
                                       info.video.h264_sps_size,
                                       buf + info.video.h264_pps_offset,
                                       info.video.h264_pps_size,
                                       buf + (uint32_t)s.offset,
                                       s.size,
                                       info.video.h264_nal_length_size,
                                       &hp);
            printf("h264_probe status=%s detail=%s %ux%u slice=%s nal_count=%u constrained=%u\n",
                   h264_status_text(hp.status),
                   hp.status_detail,
                   (unsigned)hp.width,
                   (unsigned)hp.height,
                   h264_slice_type_name(hp.first_slice_type),
                   (unsigned)hp.sample_nal_count,
                   (unsigned)hp.constrained_baseline_supported);
            printf("h264_first_mb ok=%u type=%u kind=%s cbpl=%u cbpc=%u qp_delta=%d residual=%u\n",
                   (unsigned)hp.first_mb_header_ok,
                   (unsigned)hp.first_mb_type,
                   hp.first_mb_kind,
                   (unsigned)hp.first_mb_cbp_luma,
                   (unsigned)hp.first_mb_cbp_chroma,
                   (int)hp.first_mb_qp_delta,
                   (unsigned)hp.first_mb_residual_present);
            printf("h264_first_cavlc ok=%u block=%u coeff=%u trailing=%u zeros=%u runs=%u\n",
                   (unsigned)hp.first_mb_first_residual_ok,
                   (unsigned)hp.first_mb_first_residual_block,
                   (unsigned)hp.first_mb_first_residual_total_coeff,
                   (unsigned)hp.first_mb_first_residual_trailing_ones,
                   (unsigned)hp.first_mb_first_residual_total_zeros,
                   (unsigned)hp.first_mb_first_residual_runs);
            printf("h264_first_coeff sum=%d mask=0x%04x values=",
                   (int)hp.first_mb_first_residual_coeff_sum,
                   (unsigned)hp.first_mb_first_residual_nonzero_mask);
            for (unsigned i = 0; i < 16; i++) {
                printf("%s%d", i == 0 ? "" : ",", (int)hp.first_mb_first_residual_coeff[i]);
            }
            printf("\n");
            printf("h264_first_block qp=%d intra4x4_mode=%u dequant_ok=%u idct_ok=%u pred_ok=%u recon_ok=%u detail=%s\n",
                   (int)hp.first_mb_qp,
                   (unsigned)hp.first_block_intra4x4_mode,
                   (unsigned)hp.first_block_dequant_ok,
                   (unsigned)hp.first_block_idct_ok,
                   (unsigned)hp.first_block_pred_mode_supported,
                   (unsigned)hp.first_block_reconstructed_ok,
                   hp.first_block_detail);
            if (hp.first_block_idct_ok) {
                printf("h264_first_block_residual sum=%d values=",
                       (int)hp.first_block_residual_sum);
                for (unsigned i = 0; i < 16; i++) {
                    printf("%s%d", i == 0 ? "" : ",", (int)hp.first_block_residual[i]);
                }
                printf("\n");
            }
            if (hp.first_block_reconstructed_ok) {
                printf("h264_first_block_recon dc=%u sum=%d pixels=",
                       (unsigned)hp.first_block_prediction_dc,
                       (int)hp.first_block_recon_sum);
                for (unsigned i = 0; i < 16; i++) {
                    printf("%s%u", i == 0 ? "" : ",", (unsigned)hp.first_block_recon[i]);
                }
                printf("\n");
            }
            if (hp.mb0_luma_ok) {
                printf("h264_mb0 blocks=%u sum=%d min=%u max=%u detail=%s\n",
                       (unsigned)hp.mb0_blocks_reconstructed,
                       (int)hp.mb0_luma_sum,
                       (unsigned)hp.mb0_luma_min,
                       (unsigned)hp.mb0_luma_max,
                       hp.mb0_detail);
                printf("h264_mb0_modes=");
                for (unsigned i = 0; i < 16; i++) {
                    printf("%s%u", i == 0 ? "" : ",", (unsigned)hp.mb0_block_mode[i]);
                }
                printf("\n");
                printf("h264_mb0_luma (16x16):\n");
                for (unsigned r = 0; r < 16; r++) {
                    for (unsigned c = 0; c < 16; c++) {
                        printf("%s%3u", c == 0 ? "  " : ",", (unsigned)hp.mb0_luma[r*16+c]);
                    }
                    printf("\n");
                }
            }
            if (hp.luma_mbs_decoded > 0) {
                printf("h264_luma_region mbs=%u mb_rows=%u rows=%u width=%u height=%u samples=%u row_ok=%u full_frame=%u cross_nc=%u cross_intra=%u top_nc=%u top_intra=%u i16=%u i16_dc=%u i16_mbs=%u chroma=%u sum=%d min=%u max=%u blocker_mb=%u detail=%s\n",
                       (unsigned)hp.luma_mbs_decoded,
                       (unsigned)hp.luma_mb_rows_decoded,
                       (unsigned)hp.luma_rows_decoded,
                       (unsigned)hp.luma_region_width,
                       (unsigned)hp.luma_region_height,
                       (unsigned)hp.luma_samples_decoded,
                       (unsigned)hp.luma_row_ok,
                       (unsigned)hp.full_frame_luma_ok,
                       (unsigned)hp.cross_mb_nc_ok,
                       (unsigned)hp.cross_mb_intra_ok,
                       (unsigned)hp.top_mb_nc_ok,
                       (unsigned)hp.top_mb_intra_ok,
                       (unsigned)hp.i16x16_luma_ok,
                       (unsigned)hp.i16x16_dc_transform_ok,
                       (unsigned)hp.i16x16_mbs_decoded,
                       (unsigned)hp.chroma_reconstructed_ok,
                       (int)hp.luma_region_sum,
                       (unsigned)hp.luma_region_min,
                       (unsigned)hp.luma_region_max,
                       (unsigned)hp.luma_blocker_mb,
                       hp.luma_detail);
                printf("h264_luma_region_y (width x height):\n");
                for (unsigned r = 0; r < hp.luma_region_height; r++) {
                    for (unsigned c = 0; c < hp.luma_region_width; c++) {
                        printf("%s%3u", c == 0 ? "  " : ",",
                               (unsigned)hp.luma_region[r * hp.luma_region_width + c]);
                    }
                    printf("\n");
                }
            }
        }
    }
    if (info.video.sample_count > 0 &&
        mp4_get_sample(buf, (uint32_t)len, &info, MP4_TRACK_VIDEO, info.video.sample_count - 1, &s)) {
        printf("video_sample_last off=%llu size=%u dts=%llu dur=%u\n",
               (unsigned long long)s.offset,
               (unsigned)s.size,
               (unsigned long long)s.dts,
               (unsigned)s.duration);
    }
    if (mp4_get_sample(buf, (uint32_t)len, &info, MP4_TRACK_AUDIO, 0, &s)) {
        printf("audio_sample0 off=%llu size=%u dts=%llu dur=%u\n",
               (unsigned long long)s.offset,
               (unsigned)s.size,
               (unsigned long long)s.dts,
               (unsigned)s.duration);
    }

    free(buf);
    return status == MP4_OK &&
           info.video.sample_demux_ready &&
           info.audio.sample_demux_ready ? 0 : 1;
}
