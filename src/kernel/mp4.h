/* Vibio OS - MP4 (ISO BMFF) container demuxer / probe.
 *
 * This is a deliberately isolated, self-contained module: it depends only on
 * <stdint.h> (freestanding) and operates purely on a caller-provided byte
 * buffer. It has NO dependency on the rest of the kernel, performs no I/O and
 * allocates nothing - the kernel reads the file into a buffer and hands it here.
 * It is compiled as its own translation unit (mp4.c) and linked into the
 * kernel; this keeps the MP4 code out of the 22k-line main.c and keeps the
 * kernel footprint growth tiny.
 *
 * What it does (honest scope):
 *   - Walks the ISO box tree with strict bounds checking (corrupt/truncated
 *     input can never read out of bounds or recurse without limit).
 *   - Parses ftyp / moov / mvhd / trak / tkhd / mdia / mdhd / hdlr / minf /
 *     stbl / stsd / stts / stsc / stsz / stco / co64 / mdat.
 *   - Identifies the video and audio sample formats (avc1/H.264, hvc1/HEVC,
 *     av01/AV1, vp09/VP9, mp4v, mp4a/AAC, ac-3, .mp3, opus, ...).
 *   - For H.264 reads the real profile / constraint flags / level from the
 *     avcC AVCDecoderConfigurationRecord, so Constrained Baseline vs Main vs
 *     High is reported truthfully.
 *
 * What it does NOT do: it does not decode any video or audio. decode_supported
 * is reported honestly (currently always 0 - there is no in-kernel H.264 or AAC
 * decoder yet) along with the precise reason.
 */
#ifndef VIBIO_MP4_H
#define VIBIO_MP4_H

#include <stdint.h>

/* Codec identifiers (from the stsd sample-entry FourCC). */
#define MP4_CODEC_NONE   0
#define MP4_CODEC_H264   1   /* avc1 / avc3 */
#define MP4_CODEC_HEVC   2   /* hvc1 / hev1 */
#define MP4_CODEC_AV1    3   /* av01 */
#define MP4_CODEC_VP9    4   /* vp09 */
#define MP4_CODEC_MPEG4  5   /* mp4v */
#define MP4_CODEC_AAC    6   /* mp4a */
#define MP4_CODEC_MP3    7   /* .mp3 / mp4a w/ MP3 object type */
#define MP4_CODEC_AC3    8   /* ac-3 / ec-3 */
#define MP4_CODEC_OPUS   9   /* Opus */
#define MP4_CODEC_OTHER  10  /* recognised box, unrecognised codec */

/* Parse status codes. */
#define MP4_OK            0
#define MP4_ERR_TOO_SMALL 1  /* buffer shorter than a single box header */
#define MP4_ERR_NOT_MP4   2  /* no recognisable ISO box / ftyp */
#define MP4_ERR_NO_MOOV   3  /* no moov found in the provided window */
#define MP4_ERR_TRUNCATED 4  /* moov references data past the buffer end */
#define MP4_ERR_NO_TRACK  5  /* moov present but no usable trak */

/* H.264 standard profile_idc values seen in the avcC record. */
#define MP4_H264_PROFILE_BASELINE   66
#define MP4_H264_PROFILE_MAIN       77
#define MP4_H264_PROFILE_EXTENDED   88
#define MP4_H264_PROFILE_HIGH       100

typedef struct {
    uint8_t  present;
    uint8_t  codec;            /* MP4_CODEC_* */
    char     fourcc[5];        /* NUL-terminated sample-entry FourCC */
    uint8_t  is_video;         /* hdlr handler_type == 'vide' */
    uint8_t  is_audio;         /* hdlr handler_type == 'soun' */

    uint32_t width;            /* luma width  (tkhd / stsd), pixels */
    uint32_t height;           /* luma height (tkhd / stsd), pixels */

    uint32_t timescale;        /* mdhd timescale (units per second) */
    uint64_t duration;         /* mdhd duration in timescale units */

    uint32_t sample_count;     /* from stsz */
    uint32_t chunk_count;      /* from stco / co64 */
    uint32_t first_sample_duration; /* from stts, in this track's timescale */
    uint64_t first_sample_offset;   /* resolved byte offset in the MP4 file */
    uint32_t first_sample_size;     /* resolved coded sample byte size */
    uint32_t max_sample_size;       /* max stsz entry seen while probing */
    uint8_t  sample_demux_ready;    /* stts+stsc+stsz+stco resolve samples */

    /* H.264 avcC AVCDecoderConfigurationRecord (only when codec == H264). */
    uint8_t  has_avcc;
    uint8_t  h264_profile;     /* AVCProfileIndication */
    uint8_t  h264_compat;      /* profile_compatibility (constraint_set flags) */
    uint8_t  h264_level;       /* AVCLevelIndication (e.g. 30 == level 3.0) */
    uint8_t  h264_nal_length_size; /* avcC lengthSizeMinusOne + 1 */
    uint32_t h264_sps_offset, h264_sps_size;
    uint32_t h264_pps_offset, h264_pps_size;

    /* Audio extras (AudioSampleEntry). */
    uint32_t sample_rate;      /* Hz */
    uint16_t channels;

    /* stbl sub-table presence (so MP4INFO can report table health). */
    uint8_t  has_stsd;
    uint8_t  has_stts;
    uint8_t  has_stsc;
    uint8_t  has_stsz;
    uint8_t  has_stco;
    uint8_t  has_stss;

    /* Internal table payload ranges. Exposed so MP4INFO can honestly report
     * what the demuxer has, but callers should use mp4_get_sample(). */
    uint32_t stts_offset, stts_end, stts_entry_count;
    uint32_t stsc_offset, stsc_end, stsc_entry_count;
    uint32_t stsz_offset, stsz_end, stsz_sample_size, stsz_entry_count;
    uint32_t stco_offset, stco_end, stco_entry_count;
    uint32_t stss_offset, stss_end, stss_entry_count;
    uint8_t  stco_is_64;
} Mp4Track;

typedef struct {
    uint8_t  present;
    uint64_t offset;       /* byte offset of the coded sample in the file */
    uint32_t size;         /* coded sample byte size */
    uint64_t dts;          /* decode timestamp in the track timescale */
    uint32_t duration;     /* duration in the track timescale */
} Mp4Sample;

#define MP4_TRACK_VIDEO 1
#define MP4_TRACK_AUDIO 2

typedef struct {
    uint32_t status;               /* MP4_OK or MP4_ERR_* */
    char     status_detail[64];

    uint8_t  has_ftyp;
    char     major_brand[5];       /* NUL-terminated */
    uint8_t  has_moov;
    uint8_t  has_mvhd;
    uint8_t  has_mdat;

    uint32_t movie_timescale;      /* mvhd */
    uint64_t movie_duration;       /* mvhd, in movie_timescale units */

    uint32_t track_count;          /* number of trak boxes seen */
    Mp4Track video;
    Mp4Track audio;

    /* Whether Vibio can actually decode+play this file today, and why not. */
    uint8_t  decode_supported;
    char     unsupported_reason[80];
} Mp4Info;

/* Parse `size` bytes at `data` (the start of the file, or as much of it as fits
 * in the caller's buffer). Always fills *out (zeroed first). Returns out->status
 * (MP4_OK on a usable parse). Safe on corrupt/truncated/empty input. */
int mp4_parse(const uint8_t *data, uint32_t size, Mp4Info *out);

/* Resolve a 0-based sample index into file offset/size/timing from the parsed
 * MP4 sample tables. This is still demux only: it returns coded H.264/AAC bytes,
 * not decoded pixels or PCM. It only needs the moov/stbl tables to be inside
 * `data`; the returned sample offset may point beyond the provided read window. */
int mp4_get_sample(const uint8_t *data, uint32_t size, const Mp4Info *info,
                   uint8_t track_selector, uint32_t sample_index, Mp4Sample *out);

/* Return the nearest sync sample at or before sample_index as a 0-based sample
 * index. Returns 0xFFFFFFFF when the track has no sync table or no usable entry
 * before the requested sample. */
uint32_t mp4_find_sync_sample_before(const uint8_t *data, uint32_t size,
                                     const Mp4Track *track, uint32_t sample_index);

/* Human-readable helpers (static strings, never NULL). */
const char *mp4_codec_name(uint8_t codec);
const char *mp4_status_text(uint32_t status);
const char *mp4_h264_profile_name(uint8_t profile_idc, uint8_t compat);

#endif /* VIBIO_MP4_H */
