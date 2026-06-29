/* Vibio OS - MP4 (ISO BMFF) container demuxer / probe. See mp4.h for scope.
 *
 * Design notes:
 *   - Every read goes through rd_u8/rd_be16/rd_be32/rd_be64 which clamp to the
 *     buffer and return 0 past the end, so malformed input is never an OOB read.
 *   - The box walker enforces a recursion-depth cap and rejects any child box
 *     that claims to extend past its parent, so a hostile/garbled file cannot
 *     cause runaway recursion or huge loops.
 *   - No allocation, no I/O. Pure function of the input buffer.
 */
#include "mp4.h"

/* ---- tiny freestanding helpers (no libc) --------------------------------- */

static void mp4_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) {
        b[i] = 0;
    }
}

static void mp4_copy_bytes(void *dst, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static void mp4_copy_track(Mp4Track *dst, const Mp4Track *src)
{
    mp4_copy_bytes(dst, src, sizeof(Mp4Track));
}

static void mp4_copy_str(char *dst, uint32_t dst_max, const char *src)
{
    uint32_t i = 0;
    if (dst_max == 0) {
        return;
    }
    while (src[i] != 0 && i + 1 < dst_max) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static uint8_t rd_u8(const uint8_t *d, uint32_t size, uint32_t off)
{
    return off < size ? d[off] : 0;
}

static uint16_t rd_be16(const uint8_t *d, uint32_t size, uint32_t off)
{
    return (uint16_t)(((uint32_t)rd_u8(d, size, off) << 8) | rd_u8(d, size, off + 1));
}

static uint32_t rd_be32(const uint8_t *d, uint32_t size, uint32_t off)
{
    return ((uint32_t)rd_u8(d, size, off) << 24) |
           ((uint32_t)rd_u8(d, size, off + 1) << 16) |
           ((uint32_t)rd_u8(d, size, off + 2) << 8) |
           ((uint32_t)rd_u8(d, size, off + 3));
}

static uint64_t rd_be64(const uint8_t *d, uint32_t size, uint32_t off)
{
    return ((uint64_t)rd_be32(d, size, off) << 32) | (uint64_t)rd_be32(d, size, off + 4);
}

/* FourCC at offset, compared against a 4-char literal. */
static int fourcc_eq(const uint8_t *d, uint32_t size, uint32_t off, const char *cc)
{
    for (uint32_t i = 0; i < 4; i++) {
        if (rd_u8(d, size, off + i) != (uint8_t)cc[i]) {
            return 0;
        }
    }
    return 1;
}

static void fourcc_copy(char out[5], const uint8_t *d, uint32_t size, uint32_t off)
{
    for (uint32_t i = 0; i < 4; i++) {
        uint8_t c = rd_u8(d, size, off + i);
        /* keep it printable so the console never shows control glyphs */
        out[i] = (c >= 0x20 && c < 0x7F) ? (char)c : '?';
    }
    out[4] = 0;
}

/* ---- codec / status name tables ------------------------------------------ */

const char *mp4_codec_name(uint8_t codec)
{
    switch (codec) {
    case MP4_CODEC_H264:  return "H.264/AVC";
    case MP4_CODEC_HEVC:  return "H.265/HEVC";
    case MP4_CODEC_AV1:   return "AV1";
    case MP4_CODEC_VP9:   return "VP9";
    case MP4_CODEC_MPEG4: return "MPEG-4 Visual";
    case MP4_CODEC_AAC:   return "AAC";
    case MP4_CODEC_MP3:   return "MP3";
    case MP4_CODEC_AC3:   return "AC-3";
    case MP4_CODEC_OPUS:  return "Opus";
    case MP4_CODEC_OTHER: return "unsupported";
    default:              return "none";
    }
}

const char *mp4_status_text(uint32_t status)
{
    switch (status) {
    case MP4_OK:            return "OK";
    case MP4_ERR_TOO_SMALL: return "file too small to be MP4";
    case MP4_ERR_NOT_MP4:   return "not an MP4/ISO-BMFF container";
    case MP4_ERR_NO_MOOV:   return "no moov box in read window";
    case MP4_ERR_TRUNCATED: return "moov truncated / past buffer";
    case MP4_ERR_NO_TRACK:  return "no usable track in moov";
    default:                return "unknown";
    }
}

const char *mp4_h264_profile_name(uint8_t profile_idc, uint8_t compat)
{
    switch (profile_idc) {
    case MP4_H264_PROFILE_BASELINE:
        /* constraint_set1_flag (bit 6 of profile_compatibility) => Constrained
         * Baseline, the subset Vibio targets first. */
        return (compat & 0x40) ? "Constrained Baseline" : "Baseline";
    case MP4_H264_PROFILE_MAIN:     return "Main";
    case MP4_H264_PROFILE_EXTENDED: return "Extended";
    case MP4_H264_PROFILE_HIGH:     return "High";
    default:                        return "Other";
    }
}

/* ---- codec resolution from a sample-entry FourCC ------------------------- */

static uint8_t codec_from_fourcc(const uint8_t *d, uint32_t size, uint32_t off)
{
    if (fourcc_eq(d, size, off, "avc1") || fourcc_eq(d, size, off, "avc3")) return MP4_CODEC_H264;
    if (fourcc_eq(d, size, off, "hvc1") || fourcc_eq(d, size, off, "hev1") ||
        fourcc_eq(d, size, off, "hvc2")) return MP4_CODEC_HEVC;
    if (fourcc_eq(d, size, off, "av01")) return MP4_CODEC_AV1;
    if (fourcc_eq(d, size, off, "vp09") || fourcc_eq(d, size, off, "vp08")) return MP4_CODEC_VP9;
    if (fourcc_eq(d, size, off, "mp4v")) return MP4_CODEC_MPEG4;
    if (fourcc_eq(d, size, off, "mp4a")) return MP4_CODEC_AAC;
    if (fourcc_eq(d, size, off, ".mp3")) return MP4_CODEC_MP3;
    if (fourcc_eq(d, size, off, "ac-3") || fourcc_eq(d, size, off, "ec-3")) return MP4_CODEC_AC3;
    if (fourcc_eq(d, size, off, "Opus") || fourcc_eq(d, size, off, "opus")) return MP4_CODEC_OPUS;
    return MP4_CODEC_OTHER;
}

/* ---- box walker ---------------------------------------------------------- */

#define MP4_MAX_DEPTH 8

typedef struct {
    const uint8_t *data;
    uint32_t size;     /* total buffer length */
    Mp4Info *out;
    Mp4Track *cur;     /* track currently being parsed (inside a trak) */
    uint8_t truncated; /* a box claimed to extend past the buffer */
} Mp4Ctx;

static void parse_boxes(Mp4Ctx *c, uint32_t start, uint32_t end, int depth);

/* Find a child box of a given type within [start,end) and return its payload
 * range via the two out-params (p_off, p_end). Returns 1 if found. Bounds-safe. */
static int find_box(Mp4Ctx *c, uint32_t start, uint32_t end, const char *type,
                    uint32_t *p_off, uint32_t *p_end)
{
    uint32_t pos = start;
    while (pos + 8 <= end) {
        uint32_t box_size = rd_be32(c->data, c->size, pos);
        uint32_t header = 8;
        uint64_t real_size = box_size;
        if (box_size == 1) {
            real_size = rd_be64(c->data, c->size, pos + 8);
            header = 16;
        } else if (box_size == 0) {
            real_size = (uint64_t)end - pos;
        }
        if (real_size < header || pos + real_size > (uint64_t)end) {
            break;
        }
        if (fourcc_eq(c->data, c->size, pos + 4, type)) {
            *p_off = pos + header;
            *p_end = pos + (uint32_t)real_size;
            return 1;
        }
        pos += (uint32_t)real_size;
    }
    return 0;
}

/* mvhd / mdhd share a timescale+duration layout that depends on version. */
static void read_scale_duration(Mp4Ctx *c, uint32_t off, uint32_t end,
                                uint32_t *timescale, uint64_t *duration)
{
    uint8_t version = rd_u8(c->data, c->size, off);
    if (version == 1) {
        /* version(1)+flags(3)+ctime(8)+mtime(8) => timescale at +20, dur 64 at +24 */
        if (off + 32 <= end) {
            *timescale = rd_be32(c->data, c->size, off + 20);
            *duration = rd_be64(c->data, c->size, off + 24);
        }
    } else {
        /* version(1)+flags(3)+ctime(4)+mtime(4) => timescale at +12, dur 32 at +16 */
        if (off + 20 <= end) {
            *timescale = rd_be32(c->data, c->size, off + 12);
            *duration = rd_be32(c->data, c->size, off + 16);
        }
    }
}

static void parse_tkhd(Mp4Ctx *c, uint32_t off, uint32_t end)
{
    if (c->cur == 0) {
        return;
    }
    uint8_t version = rd_u8(c->data, c->size, off);
    /* width/height are 16.16 fixed-point at the tail of tkhd. */
    uint32_t w_off = (version == 1) ? (off + 88) : (off + 76);
    if (w_off + 8 <= end) {
        uint32_t w = rd_be32(c->data, c->size, w_off);
        uint32_t h = rd_be32(c->data, c->size, w_off + 4);
        uint32_t wp = w >> 16;
        uint32_t hp = h >> 16;
        if (wp > 0 && hp > 0 && wp <= 8192 && hp <= 8192) {
            c->cur->width = wp;
            c->cur->height = hp;
        }
    }
}

static void parse_hdlr(Mp4Ctx *c, uint32_t off, uint32_t end)
{
    if (c->cur == 0) {
        return;
    }
    /* version(1)+flags(3)+pre_defined(4) => handler_type at +8 */
    if (off + 12 > end) {
        return;
    }
    if (fourcc_eq(c->data, c->size, off + 8, "vide")) {
        c->cur->is_video = 1;
    } else if (fourcc_eq(c->data, c->size, off + 8, "soun")) {
        c->cur->is_audio = 1;
    }
}

/* Dive into the avc1 sample entry's avcC record for real H.264 profile info. */
static void parse_avcc(Mp4Ctx *c, uint32_t off, uint32_t end)
{
    /* AVCDecoderConfigurationRecord:
     *  [0]=configurationVersion [1]=AVCProfileIndication
     *  [2]=profile_compatibility [3]=AVCLevelIndication ... */
    if (off + 4 > end) {
        return;
    }
    c->cur->has_avcc = 1;
    c->cur->h264_profile = rd_u8(c->data, c->size, off + 1);
    c->cur->h264_compat = rd_u8(c->data, c->size, off + 2);
    c->cur->h264_level = rd_u8(c->data, c->size, off + 3);
    if (off + 5 <= end) {
        c->cur->h264_nal_length_size = (uint8_t)((rd_u8(c->data, c->size, off + 4) & 0x03U) + 1U);
    }
    if (off + 6 <= end) {
        uint32_t pos = off + 6;
        uint32_t sps_count = rd_u8(c->data, c->size, off + 5) & 0x1FU;
        for (uint32_t i = 0; i < sps_count && pos + 2 <= end; i++) {
            uint32_t len = rd_be16(c->data, c->size, pos);
            pos += 2;
            if (pos + len > end) {
                return;
            }
            if (i == 0) {
                c->cur->h264_sps_offset = pos;
                c->cur->h264_sps_size = len;
            }
            pos += len;
        }
        if (pos + 1 <= end) {
            uint32_t pps_count = rd_u8(c->data, c->size, pos++);
            for (uint32_t i = 0; i < pps_count && pos + 2 <= end; i++) {
                uint32_t len = rd_be16(c->data, c->size, pos);
                pos += 2;
                if (pos + len > end) {
                    return;
                }
                if (i == 0) {
                    c->cur->h264_pps_offset = pos;
                    c->cur->h264_pps_size = len;
                }
                pos += len;
            }
        }
    }
}

static void parse_stsd(Mp4Ctx *c, uint32_t off, uint32_t end)
{
    if (c->cur == 0) {
        return;
    }
    c->cur->has_stsd = 1;
    /* version(1)+flags(3)+entry_count(4) => first entry at +8 */
    uint32_t entry = off + 8;
    if (entry + 8 > end) {
        return;
    }
    uint32_t entry_size = rd_be32(c->data, c->size, entry);
    uint32_t entry_end = entry + entry_size;
    if (entry_size < 8 || entry_end > end) {
        entry_end = end; /* be lenient: still read the FourCC + fixed fields */
    }
    uint32_t fmt_off = entry + 4;
    c->cur->codec = codec_from_fourcc(c->data, c->size, fmt_off);
    fourcc_copy(c->cur->fourcc, c->data, c->size, fmt_off);

    /* SampleEntry base is 16 bytes (size+fmt+reserved6+data_ref_index2). */
    uint32_t body = entry + 16;
    if (c->cur->is_video || c->cur->codec == MP4_CODEC_H264 ||
        c->cur->codec == MP4_CODEC_HEVC || c->cur->codec == MP4_CODEC_AV1 ||
        c->cur->codec == MP4_CODEC_VP9 || c->cur->codec == MP4_CODEC_MPEG4) {
        /* VisualSampleEntry: width at body+16, height at body+18 (16-bit). */
        uint32_t w = rd_be16(c->data, c->size, body + 16);
        uint32_t h = rd_be16(c->data, c->size, body + 18);
        if (w > 0 && h > 0 && (c->cur->width == 0 || c->cur->height == 0)) {
            c->cur->width = w;
            c->cur->height = h;
        }
        /* Child boxes (avcC, hvcC, ...) start after the 78-byte fixed part. */
        uint32_t child = body + 70;
        uint32_t avcc_off = 0, avcc_end = 0;
        if (child < entry_end && find_box(c, child, entry_end, "avcC", &avcc_off, &avcc_end)) {
            parse_avcc(c, avcc_off, avcc_end);
        }
    } else if (c->cur->is_audio || c->cur->codec == MP4_CODEC_AAC ||
               c->cur->codec == MP4_CODEC_MP3 || c->cur->codec == MP4_CODEC_AC3 ||
               c->cur->codec == MP4_CODEC_OPUS) {
        /* AudioSampleEntry: channelcount at body+8, samplerate(16.16) at body+16. */
        c->cur->channels = rd_be16(c->data, c->size, body + 8);
        c->cur->sample_rate = rd_be32(c->data, c->size, body + 16) >> 16;
    }
}

static void parse_leaf(Mp4Ctx *c, const char *type, uint32_t off, uint32_t end)
{
    /* Dispatch by the 4-char type literal. */
    if (type[0]=='f'&&type[1]=='t'&&type[2]=='y'&&type[3]=='p') {
        c->out->has_ftyp = 1;
        fourcc_copy(c->out->major_brand, c->data, c->size, off);
    } else if (type[0]=='m'&&type[1]=='v'&&type[2]=='h'&&type[3]=='d') {
        c->out->has_mvhd = 1;
        read_scale_duration(c, off, end, &c->out->movie_timescale, &c->out->movie_duration);
    } else if (type[0]=='t'&&type[1]=='k'&&type[2]=='h'&&type[3]=='d') {
        parse_tkhd(c, off, end);
    } else if (type[0]=='m'&&type[1]=='d'&&type[2]=='h'&&type[3]=='d') {
        if (c->cur) {
            read_scale_duration(c, off, end, &c->cur->timescale, &c->cur->duration);
        }
    } else if (type[0]=='h'&&type[1]=='d'&&type[2]=='l'&&type[3]=='r') {
        parse_hdlr(c, off, end);
    } else if (type[0]=='s'&&type[1]=='t'&&type[2]=='s'&&type[3]=='d') {
        parse_stsd(c, off, end);
    } else if (type[0]=='s'&&type[1]=='t'&&type[2]=='t'&&type[3]=='s') {
        if (c->cur) {
            c->cur->has_stts = 1;
            c->cur->stts_offset = off;
            c->cur->stts_end = end;
            c->cur->stts_entry_count = (off + 8 <= end) ? rd_be32(c->data, c->size, off + 4) : 0;
            if (c->cur->stts_entry_count > 0 && off + 16 <= end) {
                c->cur->first_sample_duration = rd_be32(c->data, c->size, off + 12);
            }
        }
    } else if (type[0]=='s'&&type[1]=='t'&&type[2]=='s'&&type[3]=='c') {
        if (c->cur) {
            c->cur->has_stsc = 1;
            c->cur->stsc_offset = off;
            c->cur->stsc_end = end;
            c->cur->stsc_entry_count = (off + 8 <= end) ? rd_be32(c->data, c->size, off + 4) : 0;
        }
    } else if (type[0]=='s'&&type[1]=='t'&&type[2]=='s'&&type[3]=='s') {
        if (c->cur) {
            c->cur->has_stss = 1;
            c->cur->stss_offset = off;
            c->cur->stss_end = end;
            c->cur->stss_entry_count = (off + 8 <= end) ? rd_be32(c->data, c->size, off + 4) : 0;
        }
    } else if (type[0]=='s'&&type[1]=='t'&&type[2]=='s'&&type[3]=='z') {
        if (c->cur) {
            c->cur->has_stsz = 1;
            c->cur->stsz_offset = off;
            c->cur->stsz_end = end;
            c->cur->stsz_sample_size = (off + 8 <= end) ? rd_be32(c->data, c->size, off + 4) : 0;
            c->cur->sample_count = (off + 12 <= end) ? rd_be32(c->data, c->size, off + 8) : 0;
            c->cur->stsz_entry_count = c->cur->sample_count;
        }
    } else if ((type[0]=='s'&&type[1]=='t'&&type[2]=='c'&&type[3]=='o')) {
        if (c->cur) {
            c->cur->has_stco = 1;
            c->cur->stco_offset = off;
            c->cur->stco_end = end;
            c->cur->stco_entry_count = (off + 8 <= end) ? rd_be32(c->data, c->size, off + 4) : 0;
            c->cur->chunk_count = c->cur->stco_entry_count;
            c->cur->stco_is_64 = 0;
        }
    } else if (type[0]=='c'&&type[1]=='o'&&type[2]=='6'&&type[3]=='4') {
        if (c->cur) {
            c->cur->has_stco = 1;
            c->cur->stco_offset = off;
            c->cur->stco_end = end;
            c->cur->stco_entry_count = (off + 8 <= end) ? rd_be32(c->data, c->size, off + 4) : 0;
            c->cur->chunk_count = c->cur->stco_entry_count;
            c->cur->stco_is_64 = 1;
        }
    } else if (type[0]=='m'&&type[1]=='d'&&type[2]=='a'&&type[3]=='t') {
        c->out->has_mdat = 1;
    }
}

static int is_container(const char *t)
{
    return (t[0]=='m'&&t[1]=='o'&&t[2]=='o'&&t[3]=='v') ||
           (t[0]=='t'&&t[1]=='r'&&t[2]=='a'&&t[3]=='k') ||
           (t[0]=='m'&&t[1]=='d'&&t[2]=='i'&&t[3]=='a') ||
           (t[0]=='m'&&t[1]=='i'&&t[2]=='n'&&t[3]=='f') ||
           (t[0]=='s'&&t[1]=='t'&&t[2]=='b'&&t[3]=='l') ||
           (t[0]=='d'&&t[1]=='i'&&t[2]=='n'&&t[3]=='f') ||
           (t[0]=='e'&&t[1]=='d'&&t[2]=='t'&&t[3]=='s');
}

static void parse_boxes(Mp4Ctx *c, uint32_t start, uint32_t end, int depth)
{
    if (depth > MP4_MAX_DEPTH) {
        return;
    }
    uint32_t pos = start;
    while (pos + 8 <= end) {
        uint32_t box_size = rd_be32(c->data, c->size, pos);
        uint32_t header = 8;
        uint64_t real_size = box_size;
        if (box_size == 1) {
            real_size = rd_be64(c->data, c->size, pos + 8);
            header = 16;
        } else if (box_size == 0) {
            real_size = (uint64_t)end - pos;
        }
        if (real_size < header) {
            break; /* malformed */
        }
        if (pos + real_size > (uint64_t)end) {
            /* Box claims to extend past what we have. Note truncation; if it is
             * moov we cannot trust the metadata. Stop walking this level. */
            c->truncated = 1;
            break;
        }
        char type[4];
        type[0] = (char)rd_u8(c->data, c->size, pos + 4);
        type[1] = (char)rd_u8(c->data, c->size, pos + 5);
        type[2] = (char)rd_u8(c->data, c->size, pos + 6);
        type[3] = (char)rd_u8(c->data, c->size, pos + 7);
        uint32_t payload = pos + header;
        uint32_t payload_end = pos + (uint32_t)real_size;

        if (type[0]=='m'&&type[1]=='o'&&type[2]=='o'&&type[3]=='v') {
            c->out->has_moov = 1;
            parse_boxes(c, payload, payload_end, depth + 1);
        } else if (type[0]=='t'&&type[1]=='r'&&type[2]=='a'&&type[3]=='k') {
            /* New track: parse into a scratch track, then file it by handler. */
            Mp4Track t;
            mp4_zero(&t, sizeof(t));
            t.present = 1;
            Mp4Track *prev = c->cur;
            c->cur = &t;
            c->out->track_count++;
            parse_boxes(c, payload, payload_end, depth + 1);
            c->cur = prev;
            if (t.is_video && !c->out->video.present) {
                mp4_copy_track(&c->out->video, &t);
            } else if (t.is_audio && !c->out->audio.present) {
                mp4_copy_track(&c->out->audio, &t);
            } else if (!c->out->video.present && t.codec != MP4_CODEC_NONE &&
                       (t.codec == MP4_CODEC_H264 || t.codec == MP4_CODEC_HEVC ||
                        t.codec == MP4_CODEC_AV1 || t.codec == MP4_CODEC_VP9 ||
                        t.codec == MP4_CODEC_MPEG4)) {
                mp4_copy_track(&c->out->video, &t);
            } else if (!c->out->audio.present && t.codec != MP4_CODEC_NONE) {
                mp4_copy_track(&c->out->audio, &t);
            }
        } else if (is_container(type)) {
            parse_boxes(c, payload, payload_end, depth + 1);
        } else {
            parse_leaf(c, type, payload, payload_end);
        }

        pos += (uint32_t)real_size;
    }
}

/* ---- sample table resolver ----------------------------------------------- */

static uint32_t table_entry_available(uint32_t base, uint32_t stride, uint32_t count,
                                      uint32_t index, uint32_t end)
{
    uint64_t off = (uint64_t)base + (uint64_t)stride * (uint64_t)index;
    return (index < count && off + stride <= end) ? (uint32_t)off : 0;
}

static uint32_t stsz_sample_size(const uint8_t *data, uint32_t size,
                                 const Mp4Track *t, uint32_t sample_index)
{
    if (t->stsz_sample_size != 0) {
        return t->stsz_sample_size;
    }
    uint32_t ent = table_entry_available(t->stsz_offset + 12, 4, t->stsz_entry_count,
                                         sample_index, t->stsz_end);
    return ent ? rd_be32(data, size, ent) : 0;
}

static uint64_t stco_chunk_offset(const uint8_t *data, uint32_t size,
                                  const Mp4Track *t, uint32_t chunk_index0)
{
    if (t->stco_is_64) {
        uint32_t ent = table_entry_available(t->stco_offset + 8, 8, t->stco_entry_count,
                                             chunk_index0, t->stco_end);
        return ent ? rd_be64(data, size, ent) : 0;
    }
    uint32_t ent = table_entry_available(t->stco_offset + 8, 4, t->stco_entry_count,
                                         chunk_index0, t->stco_end);
    return ent ? (uint64_t)rd_be32(data, size, ent) : 0;
}

static uint32_t stsc_entry_field(const uint8_t *data, uint32_t size,
                                 const Mp4Track *t, uint32_t entry_index,
                                 uint32_t field)
{
    uint32_t ent = table_entry_available(t->stsc_offset + 8, 12, t->stsc_entry_count,
                                         entry_index, t->stsc_end);
    return ent ? rd_be32(data, size, ent + field * 4U) : 0;
}

static void stts_sample_time(const uint8_t *data, uint32_t size,
                             const Mp4Track *t, uint32_t sample_index,
                             uint64_t *out_dts, uint32_t *out_duration)
{
    uint64_t dts = 0;
    uint32_t seen = 0;
    uint32_t duration = 0;
    for (uint32_t i = 0; i < t->stts_entry_count; i++) {
        uint32_t ent = table_entry_available(t->stts_offset + 8, 8, t->stts_entry_count,
                                             i, t->stts_end);
        if (!ent) {
            break;
        }
        uint32_t count = rd_be32(data, size, ent);
        uint32_t delta = rd_be32(data, size, ent + 4);
        if (count == 0) {
            continue;
        }
        if (sample_index < seen + count) {
            dts += (uint64_t)(sample_index - seen) * (uint64_t)delta;
            duration = delta;
            break;
        }
        dts += (uint64_t)count * (uint64_t)delta;
        seen += count;
    }
    if (out_dts) {
        *out_dts = dts;
    }
    if (out_duration) {
        *out_duration = duration;
    }
}

static int track_tables_ready(const Mp4Track *t)
{
    return t != 0 && t->present &&
           t->has_stts && t->has_stsc && t->has_stsz && t->has_stco &&
           t->sample_count > 0 && t->chunk_count > 0 &&
           t->stts_entry_count > 0 && t->stsc_entry_count > 0 &&
           t->stco_entry_count > 0 &&
           (t->stsz_sample_size != 0 || t->stsz_entry_count >= t->sample_count);
}

static int mp4_get_sample_from_track(const uint8_t *data, uint32_t size,
                                     const Mp4Track *t, uint32_t sample_index,
                                     Mp4Sample *out)
{
    if (out) {
        mp4_zero(out, sizeof(*out));
    }
    if (!track_tables_ready(t) || sample_index >= t->sample_count || out == 0) {
        return 0;
    }

    uint32_t remaining = sample_index;
    uint32_t sample_base = 0;
    uint32_t chunk_index0 = 0;
    uint32_t samples_per_chunk = 0;
    for (uint32_t i = 0; i < t->stsc_entry_count; i++) {
        uint32_t first_chunk = stsc_entry_field(data, size, t, i, 0);
        uint32_t spc = stsc_entry_field(data, size, t, i, 1);
        uint32_t next_first = (i + 1 < t->stsc_entry_count) ?
                              stsc_entry_field(data, size, t, i + 1, 0) :
                              (t->chunk_count + 1U);
        if (first_chunk == 0 || spc == 0 || next_first <= first_chunk) {
            return 0;
        }
        uint32_t run_chunks = next_first - first_chunk;
        uint64_t run_samples = (uint64_t)run_chunks * (uint64_t)spc;
        if ((uint64_t)remaining < run_samples) {
            uint32_t in_run_chunk = remaining / spc;
            chunk_index0 = first_chunk - 1U + in_run_chunk;
            samples_per_chunk = spc;
            sample_base = sample_index - (remaining % spc);
            break;
        }
        remaining -= (uint32_t)run_samples;
    }
    if (samples_per_chunk == 0 || chunk_index0 >= t->chunk_count) {
        return 0;
    }

    uint64_t off = stco_chunk_offset(data, size, t, chunk_index0);
    if (off == 0) {
        return 0;
    }
    for (uint32_t si = sample_base; si < sample_index; si++) {
        uint32_t prev_size = stsz_sample_size(data, size, t, si);
        if (prev_size == 0) {
            return 0;
        }
        off += prev_size;
    }
    uint32_t sample_size = stsz_sample_size(data, size, t, sample_index);
    if (sample_size == 0) {
        return 0;
    }

    uint64_t dts = 0;
    uint32_t duration = 0;
    stts_sample_time(data, size, t, sample_index, &dts, &duration);
    if (duration == 0) {
        duration = t->first_sample_duration;
    }

    out->present = 1;
    out->offset = off;
    out->size = sample_size;
    out->dts = dts;
    out->duration = duration;
    return 1;
}

uint32_t mp4_find_sync_sample_before(const uint8_t *data, uint32_t size,
                                     const Mp4Track *track, uint32_t sample_index)
{
    uint32_t best = 0xFFFFFFFFU;
    if (track == 0 || !track->has_stss || track->stss_entry_count == 0) {
        return best;
    }
    for (uint32_t i = 0; i < track->stss_entry_count; i++) {
        uint32_t ent = table_entry_available(track->stss_offset + 8, 4,
                                             track->stss_entry_count, i,
                                             track->stss_end);
        if (!ent) {
            break;
        }
        uint32_t sample_number = rd_be32(data, size, ent);
        if (sample_number == 0) {
            continue;
        }
        uint32_t sample0 = sample_number - 1U;
        if (sample0 > sample_index) {
            break;
        }
        best = sample0;
    }
    return best;
}

static void finalize_track_demux(const uint8_t *data, uint32_t size, Mp4Track *t)
{
    if (!track_tables_ready(t)) {
        return;
    }
    Mp4Sample s;
    if (mp4_get_sample_from_track(data, size, t, 0, &s)) {
        t->sample_demux_ready = 1;
        t->first_sample_offset = s.offset;
        t->first_sample_size = s.size;
        t->first_sample_duration = s.duration;
    }
    if (t->stsz_sample_size != 0) {
        t->max_sample_size = t->stsz_sample_size;
    } else {
        uint32_t max = 0;
        for (uint32_t i = 0; i < t->sample_count; i++) {
            uint32_t sz = stsz_sample_size(data, size, t, i);
            if (sz > max) {
                max = sz;
            }
        }
        t->max_sample_size = max;
    }
}

/* ---- public entry point -------------------------------------------------- */

static void decide_decode_support(Mp4Info *out)
{
    /* Honest: Vibio has a real demuxer but NO in-kernel video/audio decoder yet,
     * so nothing here is currently decode_supported. The reason is specific. */
    out->decode_supported = 0;
    if (out->video.present) {
        if (out->video.codec == MP4_CODEC_H264) {
            mp4_copy_str(out->unsupported_reason, sizeof(out->unsupported_reason),
                         out->video.sample_demux_ready ?
                         "H.264 samples demuxed; no in-kernel decoder yet" :
                         "H.264 video: sample tables incomplete or no decoder yet");
        } else {
            char tmp[80];
            uint32_t p = 0;
            const char *nm = mp4_codec_name(out->video.codec);
            while (nm[p] != 0 && p + 24 < sizeof(tmp)) { tmp[p] = nm[p]; p++; }
            const char *suffix = " video not supported";
            uint32_t k = 0;
            while (suffix[k] != 0 && p + 1 < sizeof(tmp)) { tmp[p++] = suffix[k++]; }
            tmp[p] = 0;
            mp4_copy_str(out->unsupported_reason, sizeof(out->unsupported_reason), tmp);
        }
    } else if (out->audio.present) {
        mp4_copy_str(out->unsupported_reason, sizeof(out->unsupported_reason),
                     "audio-only MP4: no in-kernel audio decoder yet");
    } else {
        mp4_copy_str(out->unsupported_reason, sizeof(out->unsupported_reason),
                     "no decodable track found");
    }
}

int mp4_parse(const uint8_t *data, uint32_t size, Mp4Info *out)
{
    mp4_zero(out, sizeof(*out));

    if (data == 0 || size < 8) {
        out->status = MP4_ERR_TOO_SMALL;
        mp4_copy_str(out->status_detail, sizeof(out->status_detail), mp4_status_text(out->status));
        return out->status;
    }

    /* Sanity-check the very first box looks like ISO BMFF: a plausible size and
     * a printable FourCC. The first top-level box is almost always ftyp. */
    {
        uint32_t first_size = rd_be32(data, size, 0);
        int printable = 1;
        for (uint32_t i = 4; i < 8; i++) {
            uint8_t ch = rd_u8(data, size, i);
            if (ch < 0x20 || ch >= 0x7F) { printable = 0; break; }
        }
        if (!printable || first_size < 8) {
            out->status = MP4_ERR_NOT_MP4;
            mp4_copy_str(out->status_detail, sizeof(out->status_detail), mp4_status_text(out->status));
            return out->status;
        }
    }

    Mp4Ctx ctx;
    ctx.data = data;
    ctx.size = size;
    ctx.out = out;
    ctx.cur = 0;
    ctx.truncated = 0;
    parse_boxes(&ctx, 0, size, 0);

    if (!out->has_ftyp && !out->has_moov) {
        out->status = MP4_ERR_NOT_MP4;
    } else if (!out->has_moov) {
        out->status = ctx.truncated ? MP4_ERR_TRUNCATED : MP4_ERR_NO_MOOV;
    } else if (!out->video.present && !out->audio.present) {
        out->status = ctx.truncated ? MP4_ERR_TRUNCATED : MP4_ERR_NO_TRACK;
    } else {
        out->status = MP4_OK;
    }

    if (out->status == MP4_OK) {
        finalize_track_demux(data, size, &out->video);
        finalize_track_demux(data, size, &out->audio);
        decide_decode_support(out);
        mp4_copy_str(out->status_detail, sizeof(out->status_detail), "OK");
    } else {
        mp4_copy_str(out->status_detail, sizeof(out->status_detail), mp4_status_text(out->status));
    }
    return out->status;
}

int mp4_get_sample(const uint8_t *data, uint32_t size, const Mp4Info *info,
                   uint8_t track_selector, uint32_t sample_index, Mp4Sample *out)
{
    if (info == 0) {
        if (out) {
            mp4_zero(out, sizeof(*out));
        }
        return 0;
    }
    if (track_selector == MP4_TRACK_VIDEO) {
        return mp4_get_sample_from_track(data, size, &info->video, sample_index, out);
    }
    if (track_selector == MP4_TRACK_AUDIO) {
        return mp4_get_sample_from_track(data, size, &info->audio, sample_index, out);
    }
    if (out) {
        mp4_zero(out, sizeof(*out));
    }
    return 0;
}
