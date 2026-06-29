/* Vibio OS - narrow media codec boundary types.
 *
 * This header is intentionally small: container parsers still own demuxing,
 * codecs report bounded decode artifacts, and playback remains unsupported
 * until video RGB timing and audio output exist.
 */
#ifndef VIBIO_MEDIA_CODEC_H
#define VIBIO_MEDIA_CODEC_H

#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t macroblocks_decoded;
    uint32_t macroblock_rows_decoded;
    uint32_t samples_decoded;
    uint8_t full_frame_luma;
} MediaCodecLumaRegion;

#endif /* VIBIO_MEDIA_CODEC_H */
