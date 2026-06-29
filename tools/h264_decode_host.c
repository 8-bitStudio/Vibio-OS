/* Host harness: drive the full-frame H.264 decoder over an MP4 and dump raw
 * YUV420 planar frames so they can be diffed bit-exactly against ffmpeg. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mp4.h"
#include "h264.h"

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "Disk/BASE320.MP4";
    uint32_t want_frames = argc > 2 ? (uint32_t)atoi(argv[2]) : 1;
    const char *outpath = argc > 3 ? argv[3] : "build/_dec.yuv";
    uint32_t write_coded = (argc > 4 && strcmp(argv[4], "coded") == 0);

    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 2; }
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc((size_t)len);
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) { return 2; }
    fclose(f);

    Mp4Info info;
    mp4_parse(buf, (uint32_t)len, &info);
    if (info.status != MP4_OK || info.video.codec != MP4_CODEC_H264) {
        fprintf(stderr, "not an H.264 MP4: status=%u codec=%u\n", info.status, info.video.codec);
        return 2;
    }
    const Mp4Track *v = &info.video;
    const uint8_t *sps = buf + v->h264_sps_offset;
    const uint8_t *pps = buf + v->h264_pps_offset;

    H264Decoder dec;
    memset(&dec, 0, sizeof(dec));

    /* Pre-init parse to learn dimensions for buffer sizing. We just init twice:
     * give a tiny pool first to read dims, then allocate real buffers. */
    H264Decoder tmp; memset(&tmp, 0, sizeof(tmp));
    tmp.frames = 0; tmp.frame_pool_size = 0;
    /* work arrays must be present even for init? init doesn't touch them. */
    if (h264_decoder_init(&tmp, sps, v->h264_sps_size, pps, v->h264_pps_size) != H264_OK) {
        fprintf(stderr, "init failed: %s\n", tmp.status_detail);
        return 2;
    }
    uint32_t W = tmp.width, H = tmp.height;       /* coded */
    uint32_t CW = W / 2, CH = H / 2;
    uint32_t cropW = tmp.crop_width, cropH = tmp.crop_height;
    uint32_t mbw = tmp.mb_width, mbh = tmp.mb_height;
    uint32_t pool = tmp.max_num_ref_frames + 2; if (pool < 3) pool = 3; if (pool > H264_MAX_REF_FRAMES + 1) pool = H264_MAX_REF_FRAMES + 1;

    H264Frame *frames = calloc(pool, sizeof(H264Frame));
    for (uint32_t i = 0; i < pool; i++) {
        frames[i].y = malloc(W * H);
        frames[i].cb = malloc(CW * CH);
        frames[i].cr = malloc(CW * CH);
    }
    uint32_t nblk = (mbw * 4) * (mbh * 4);
    uint32_t ncblk = (mbw * 2) * (mbh * 2);
    H264Work work; memset(&work, 0, sizeof(work));
    work.nnz = calloc(nblk, 1);
    work.imode = calloc(nblk, 1);
    work.mvx = calloc(nblk, sizeof(int16_t));
    work.mvy = calloc(nblk, sizeof(int16_t));
    work.refidx = calloc(nblk, 1);
    work.mb_type = calloc(mbw * mbh, 1);
    work.mb_qp = calloc(mbw * mbh, 1);
    work.mb_intra = calloc(mbw * mbh, 1);
    work.cnnz[0] = calloc(ncblk, 1);
    work.cnnz[1] = calloc(ncblk, 1);
    work.decmask = calloc(W * H, 1);

    memset(&dec, 0, sizeof(dec));
    dec.frames = frames; dec.frame_pool_size = pool; dec.work = work;
    if (h264_decoder_init(&dec, sps, v->h264_sps_size, pps, v->h264_pps_size) != H264_OK) {
        fprintf(stderr, "init failed: %s\n", dec.status_detail);
        return 2;
    }
    fprintf(stderr, "coded %ux%u crop %ux%u mb %ux%u maxref=%u pool=%u\n",
            W, H, cropW, cropH, mbw, mbh, dec.max_num_ref_frames, pool);

    FILE *o = fopen(outpath, "wb");
    uint32_t decoded = 0, errs = 0;
    for (uint32_t fi = 0; fi < want_frames && fi < v->sample_count; fi++) {
        Mp4Sample s;
        if (!mp4_get_sample(buf, (uint32_t)len, &info, MP4_TRACK_VIDEO, fi, &s) || !s.present) break;
        if (s.offset + s.size > (uint64_t)len) break;
        H264Frame *out = 0;
        uint32_t st = h264_decode_sample(&dec, buf + (uint32_t)s.offset, s.size,
                                         v->h264_nal_length_size, &out);
        if (st != H264_OK || !out) {
            fprintf(stderr, "frame %u decode failed st=%u: %s\n", fi, st, dec.status_detail);
            errs++;
            break;
        }
        if (write_coded) {
            for (uint32_t y = 0; y < out->height; y++) fwrite(out->y + y * out->width, 1, out->width, o);
            for (uint32_t y = 0; y < out->cheight; y++) fwrite(out->cb + y * out->cwidth, 1, out->cwidth, o);
            for (uint32_t y = 0; y < out->cheight; y++) fwrite(out->cr + y * out->cwidth, 1, out->cwidth, o);
        } else {
            for (uint32_t y = 0; y < cropH; y++) fwrite(out->y + y * out->width, 1, cropW, o);
            for (uint32_t y = 0; y < cropH/2; y++) fwrite(out->cb + y * out->cwidth, 1, cropW/2, o);
            for (uint32_t y = 0; y < cropH/2; y++) fwrite(out->cr + y * out->cwidth, 1, cropW/2, o);
        }
        decoded++;
    }
    fclose(o);
    fprintf(stderr, "decoded=%u errs=%u -> %s\n", decoded, errs, outpath);
    printf("DECODED %u %ux%u%s\n", decoded,
           write_coded ? W : cropW, write_coded ? H : cropH,
           write_coded ? " coded" : "");
    return decoded > 0 ? 0 : 1;
}
