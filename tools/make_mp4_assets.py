#!/usr/bin/env python3
"""Generate small, *real* MP4 test assets for Vibio's native MP4 demuxer.

No ffmpeg is required (and none is assumed): these files are hand-built valid
ISO-BMFF containers with genuine box structure, a real H.264
AVCDecoderConfigurationRecord (avcC) carrying a real Constrained Baseline SPS,
and a real mp4a/AAC sample entry. They are honest about one thing: the coded
sample bytes in `mdat` are *stubs* - without an encoder we cannot produce real
decodable H.264 frames here, and Vibio has no H.264 decoder anyway. The point of
these assets is to exercise the demuxer / MP4INFO / Media Player probe path:

  TEST.MP4   faststart H.264 Constrained Baseline 320x180 + AAC-LC 44100 stereo
  UNSUP.MP4  same container shape but an unsupported video codec (hvc1/HEVC)
  BAD.MP4    deliberately corrupt: valid ftyp then a moov that overruns the file

Pure standard library. Writes into the chosen output dir (default build/media)
so tools/make_fat32_image.py packs them onto the VM disk like the other media.
"""
import argparse
import os
import struct


# ---- box helpers -----------------------------------------------------------

def box(box_type, payload):
    assert len(box_type) == 4
    return struct.pack(">I", 8 + len(payload)) + box_type.encode("ascii") + payload


def full_box(box_type, version, flags, payload):
    return box(box_type, struct.pack(">I", (version << 24) | (flags & 0xFFFFFF)) + payload)


# ---- a tiny H.264 bitstream writer (for a genuine SPS/PPS) ------------------

class BitWriter:
    def __init__(self):
        self.bits = []

    def u(self, value, n):
        for i in range(n - 1, -1, -1):
            self.bits.append((value >> i) & 1)

    def ue(self, value):
        # Exp-Golomb unsigned
        value += 1
        n = value.bit_length()
        self.u(0, n - 1)
        self.u(value, n)

    def se(self, value):
        if value <= 0:
            self.ue(-2 * value)
        else:
            self.ue(2 * value - 1)

    def rbsp_trailing(self):
        self.u(1, 1)
        while len(self.bits) % 8 != 0:
            self.u(0, 1)

    def bytes(self):
        out = bytearray()
        for i in range(0, len(self.bits), 8):
            byte = 0
            for b in range(8):
                byte = (byte << 1) | self.bits[i + b]
            out.append(byte)
        return bytes(out)


def emulation_prevent(rbsp):
    """Insert 0x03 escape bytes so 00 00 00/01/02/03 sequences are legal NAL."""
    out = bytearray()
    zeros = 0
    for byte in rbsp:
        if zeros >= 2 and byte <= 3:
            out.append(0x03)
            zeros = 0
        out.append(byte)
        zeros = zeros + 1 if byte == 0 else 0
    return bytes(out)


def make_sps_320x180():
    """Real H.264 SPS: Constrained Baseline (profile 66, constraint_set1), L3.0,
    320x180 (20x12 MBs with a 6-unit bottom crop -> 192 down to 180)."""
    bw = BitWriter()
    bw.u(66, 8)          # profile_idc = Baseline
    bw.u(0, 1)           # constraint_set0_flag
    bw.u(1, 1)           # constraint_set1_flag  => Constrained Baseline
    bw.u(0, 1)           # constraint_set2_flag
    bw.u(0, 1)           # constraint_set3_flag
    bw.u(0, 1)           # constraint_set4_flag
    bw.u(0, 1)           # constraint_set5_flag
    bw.u(0, 2)           # reserved_zero_2bits
    bw.u(30, 8)          # level_idc = 3.0
    bw.ue(0)             # seq_parameter_set_id
    bw.ue(0)             # log2_max_frame_num_minus4
    bw.ue(0)             # pic_order_cnt_type
    bw.ue(0)             # log2_max_pic_order_cnt_lsb_minus4
    bw.ue(1)             # max_num_ref_frames
    bw.u(0, 1)           # gaps_in_frame_num_value_allowed_flag
    bw.ue(19)            # pic_width_in_mbs_minus1     (20*16 = 320)
    bw.ue(11)            # pic_height_in_map_units_minus1 (12*16 = 192)
    bw.u(1, 1)           # frame_mbs_only_flag
    bw.u(1, 1)           # direct_8x8_inference_flag
    bw.u(1, 1)           # frame_cropping_flag
    bw.ue(0)             # frame_crop_left_offset
    bw.ue(0)             # frame_crop_right_offset
    bw.ue(0)             # frame_crop_top_offset
    bw.ue(6)             # frame_crop_bottom_offset  (192 - 2*6 = 180)
    bw.u(0, 1)           # vui_parameters_present_flag
    bw.rbsp_trailing()
    return bytes([0x67]) + emulation_prevent(bw.bytes())   # NAL: SPS, ref_idc 3


def make_pps():
    bw = BitWriter()
    bw.ue(0)             # pic_parameter_set_id
    bw.ue(0)             # seq_parameter_set_id
    bw.u(0, 1)           # entropy_coding_mode_flag = 0 (CAVLC, no CABAC)
    bw.u(0, 1)           # bottom_field_pic_order_in_frame_present_flag
    bw.ue(0)             # num_slice_groups_minus1
    bw.ue(0)             # num_ref_idx_l0_default_active_minus1
    bw.ue(0)             # num_ref_idx_l1_default_active_minus1
    bw.u(0, 1)           # weighted_pred_flag
    bw.u(0, 2)           # weighted_bipred_idc
    bw.se(0)             # pic_init_qp_minus26
    bw.se(0)             # pic_init_qs_minus26
    bw.se(0)             # chroma_qp_index_offset
    bw.u(0, 1)           # deblocking_filter_control_present_flag
    bw.u(0, 1)           # constrained_intra_pred_flag
    bw.u(0, 1)           # redundant_pic_cnt_present_flag
    bw.rbsp_trailing()
    return bytes([0x68]) + emulation_prevent(bw.bytes())   # NAL: PPS, ref_idc 3


def make_avcc(sps, pps):
    body = bytearray()
    body.append(1)            # configurationVersion
    body.append(sps[1])       # AVCProfileIndication      (66)
    body.append(sps[2])       # profile_compatibility     (0x40 => constrained)
    body.append(sps[3])       # AVCLevelIndication        (30)
    body.append(0xFF)         # 6 bits reserved + lengthSizeMinusOne = 3
    body.append(0xE1)         # 3 bits reserved + numOfSPS = 1
    body += struct.pack(">H", len(sps)) + sps
    body.append(1)            # numOfPPS
    body += struct.pack(">H", len(pps)) + pps
    return box("avcC", bytes(body))


def make_esds_aac():
    """A minimal but valid esds for AAC-LC (audioObjectType 2), 44100 stereo."""
    # DecoderSpecificInfo (AudioSpecificConfig): AOT=2, freq idx 4 (44100), 2ch
    # 00010 0100 0010 000 = 0x12 0x10
    asc = bytes([0x12, 0x10])

    def desc(tag, payload):
        # ISO 14496-1 expandable size (single byte is fine for tiny payloads)
        return bytes([tag, len(payload)]) + payload

    dec_specific = desc(0x05, asc)
    # DecoderConfigDescriptor: objectTypeIndication 0x40 (AAC), streamType audio
    dcd_payload = bytes([0x40, 0x15]) + bytes([0, 0, 0]) + \
        struct.pack(">I", 0) + struct.pack(">I", 0) + dec_specific
    dcd = desc(0x04, dcd_payload)
    sl = desc(0x06, bytes([0x02]))
    es_payload = struct.pack(">H", 0) + bytes([0x00]) + dcd + sl
    es = desc(0x03, es_payload)
    return full_box("esds", 0, 0, es)


# ---- sample tables ---------------------------------------------------------

def stbl_video(avcc, sizes, chunk_offset, sample_delta):
    # avc1 VisualSampleEntry, width 320 height 180
    vse = bytearray()
    vse += bytes(6)                       # reserved
    vse += struct.pack(">H", 1)           # data_reference_index
    vse += bytes(2 + 2 + 12)              # pre_defined/reserved/pre_defined
    vse += struct.pack(">H", 320)         # width
    vse += struct.pack(">H", 180)         # height
    vse += struct.pack(">I", 0x00480000)  # horizresolution 72dpi
    vse += struct.pack(">I", 0x00480000)  # vertresolution
    vse += struct.pack(">I", 0)           # reserved
    vse += struct.pack(">H", 1)           # frame_count
    vse += bytes(32)                      # compressorname
    vse += struct.pack(">H", 0x0018)      # depth
    vse += struct.pack(">h", -1)          # pre_defined
    vse += avcc
    avc1 = box("avc1", bytes(vse))
    stsd = full_box("stsd", 0, 0, struct.pack(">I", 1) + avc1)

    stts = full_box("stts", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">II", len(sizes), sample_delta))
    stsc = full_box("stsc", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">III", 1, len(sizes), 1))
    stsz = full_box("stsz", 0, 0, struct.pack(">II", 0, len(sizes)) +
                    b"".join(struct.pack(">I", s) for s in sizes))
    stco = full_box("stco", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">I", chunk_offset))
    return box("stbl", stsd + stts + stsc + stsz + stco)


def stbl_video_unsupported(sizes, chunk_offset, sample_delta):
    # hvc1 (HEVC) sample entry with no decodable config - codec detection only.
    vse = bytearray()
    vse += bytes(6)
    vse += struct.pack(">H", 1)
    vse += bytes(2 + 2 + 12)
    vse += struct.pack(">H", 320)
    vse += struct.pack(">H", 180)
    vse += struct.pack(">I", 0x00480000)
    vse += struct.pack(">I", 0x00480000)
    vse += struct.pack(">I", 0)
    vse += struct.pack(">H", 1)
    vse += bytes(32)
    vse += struct.pack(">H", 0x0018)
    vse += struct.pack(">h", -1)
    hvc1 = box("hvc1", bytes(vse))
    stsd = full_box("stsd", 0, 0, struct.pack(">I", 1) + hvc1)
    stts = full_box("stts", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">II", len(sizes), sample_delta))
    stsc = full_box("stsc", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">III", 1, len(sizes), 1))
    stsz = full_box("stsz", 0, 0, struct.pack(">II", 0, len(sizes)) +
                    b"".join(struct.pack(">I", s) for s in sizes))
    stco = full_box("stco", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">I", chunk_offset))
    return box("stbl", stsd + stts + stsc + stsz + stco)


def stbl_audio(sizes, chunk_offset, sample_delta):
    ase = bytearray()
    ase += bytes(6)                       # reserved
    ase += struct.pack(">H", 1)           # data_reference_index
    ase += bytes(8)                       # reserved (version/revision/vendor)
    ase += struct.pack(">H", 2)           # channelcount
    ase += struct.pack(">H", 16)          # samplesize
    ase += struct.pack(">H", 0)           # pre_defined
    ase += struct.pack(">H", 0)           # reserved
    ase += struct.pack(">I", 44100 << 16) # samplerate 16.16
    ase += make_esds_aac()
    mp4a = box("mp4a", bytes(ase))
    stsd = full_box("stsd", 0, 0, struct.pack(">I", 1) + mp4a)
    stts = full_box("stts", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">II", len(sizes), sample_delta))
    stsc = full_box("stsc", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">III", 1, len(sizes), 1))
    stsz = full_box("stsz", 0, 0, struct.pack(">II", 0, len(sizes)) +
                    b"".join(struct.pack(">I", s) for s in sizes))
    stco = full_box("stco", 0, 0, struct.pack(">I", 1) +
                    struct.pack(">I", chunk_offset))
    return box("stbl", stsd + stts + stsc + stsz + stco)


def dinf():
    dref = full_box("dref", 0, 0, struct.pack(">I", 1) +
                    full_box("url ", 0, 1, b""))
    return box("dinf", dref)


def mdia(timescale, duration, handler, hdlr_name, minf_inner):
    mdhd = full_box("mdhd", 0, 0, struct.pack(">IIII", 0, 0, timescale, duration) +
                    struct.pack(">HH", 0x55C4, 0))  # language 'und', pre_defined
    hdlr = full_box("hdlr", 0, 0, struct.pack(">I", 0) + handler.encode("ascii") +
                    bytes(12) + hdlr_name.encode("ascii") + b"\x00")
    minf = box("minf", minf_inner)
    return box("mdia", mdhd + hdlr + minf)


def trak(track_id, width, height, duration_movie, mdia_box):
    # tkhd v0: flags 0x7 (enabled|in_movie|in_preview)
    payload = struct.pack(">IIIII", 0, 0, track_id, 0, duration_movie)
    payload += bytes(8)                      # reserved
    payload += struct.pack(">HH", 0, 0)      # layer, alternate_group
    payload += struct.pack(">H", 0x0100 if height == 0 else 0)  # volume
    payload += struct.pack(">H", 0)          # reserved
    # unity matrix
    matrix = [0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000]
    payload += b"".join(struct.pack(">I", v) for v in matrix)
    payload += struct.pack(">I", width << 16)
    payload += struct.pack(">I", height << 16)
    tkhd = full_box("tkhd", 0, 0x7, payload)
    return box("trak", tkhd + mdia_box)


def mvhd(timescale, duration, next_track_id):
    payload = struct.pack(">IIII", 0, 0, timescale, duration)
    payload += struct.pack(">I", 0x00010000)   # rate
    payload += struct.pack(">H", 0x0100)        # volume
    payload += struct.pack(">H", 0) + bytes(8)  # reserved
    matrix = [0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000]
    payload += b"".join(struct.pack(">I", v) for v in matrix)
    payload += bytes(24)                        # pre_defined
    payload += struct.pack(">I", next_track_id)
    return full_box("mvhd", 0, 0, payload)


def ftyp():
    return box("ftyp", b"isom" + struct.pack(">I", 0x200) +
               b"isom" + b"iso2" + b"avc1" + b"mp41")


def build_mp4(video_kind):
    """video_kind: 'h264' (avc1+avcC) or 'hevc' (hvc1, unsupported)."""
    movie_timescale = 1000
    vts = 12800          # video media timescale
    ats = 44100          # audio media timescale
    vframes = 3
    aframes = 2
    vdelta = vts // 15   # ~15 fps
    adelta = 1024        # AAC frame size
    duration_movie = movie_timescale * 1  # ~1s

    # Stub coded samples (real container, placeholder coded bytes).
    if video_kind == "h264":
        sps = make_sps_320x180()
        pps = make_pps()
        # Each "sample" = 4-byte length-prefixed stub NAL (type 1, non-IDR slice
        # header byte only). Not a decodable frame; demuxer/probe target only.
        vsamps = []
        for i in range(vframes):
            nal = bytes([0x61, 0x9A, 0x00 ^ (i & 0xFF)])  # ref_idc|type=1 + stub
            vsamps.append(struct.pack(">I", len(nal)) + nal)
    else:
        vsamps = []
        for i in range(vframes):
            nal = bytes([0x26, 0x01, 0xAF, i & 0xFF])     # stub HEVC NAL
            vsamps.append(struct.pack(">I", len(nal)) + nal)

    asamps = [bytes([0xDE, 0xAD, 0xBE, 0xEF, i & 0xFF]) for i in range(aframes)]

    vsizes = [len(s) for s in vsamps]
    asizes = [len(s) for s in asamps]
    video_data = b"".join(vsamps)
    audio_data = b"".join(asamps)

    def assemble(v_chunk_off, a_chunk_off):
        if video_kind == "h264":
            v_stbl = stbl_video(make_avcc(make_sps_320x180(), make_pps()),
                                 vsizes, v_chunk_off, vdelta)
        else:
            v_stbl = stbl_video_unsupported(vsizes, v_chunk_off, vdelta)
        # Inner content of minf; mdia() wraps it in the single minf box.
        vminf = (full_box("vmhd", 0, 1, struct.pack(">HHHH", 0, 0, 0, 0)) +
                 dinf() + v_stbl)
        aminf = (full_box("smhd", 0, 0, struct.pack(">HH", 0, 0)) +
                 dinf() + stbl_audio(asizes, a_chunk_off, adelta))
        v_mdia = mdia(vts, vdelta * vframes, "vide", "VideoHandler", vminf)
        a_mdia = mdia(ats, adelta * aframes, "soun", "SoundHandler", aminf)
        v_trak = trak(1, 320, 180, duration_movie, v_mdia)
        a_trak = trak(2, 0, 0, duration_movie, a_mdia)
        moov = box("moov", mvhd(movie_timescale, duration_movie, 3) + v_trak + a_trak)
        return moov

    head = ftyp()
    # First pass with placeholder offsets to measure moov length (stable, since
    # chunk offsets are fixed 32-bit fields).
    moov0 = assemble(0, 0)
    mdat_data_off = len(head) + len(moov0) + 8
    v_off = mdat_data_off
    a_off = mdat_data_off + len(video_data)
    moov = assemble(v_off, a_off)
    assert len(moov) == len(moov0), "moov length shifted unexpectedly"
    mdat = box("mdat", video_data + audio_data)
    return head + moov + mdat


def build_bad():
    """Valid ftyp, then a moov box claiming a size far past the file end."""
    head = ftyp()
    # moov header says 0x00FFFFFF bytes but only 4 follow -> overrun/truncation.
    bad_moov = struct.pack(">I", 0x00FFFFFF) + b"moov" + b"\x00\x00\x00\x00"
    return head + bad_moov


def main():
    ap = argparse.ArgumentParser(description="Generate Vibio MP4 test assets.")
    ap.add_argument("--out", required=True, help="Output directory (e.g. build/media)")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    assets = {
        "TEST.MP4": build_mp4("h264"),
        "UNSUP.MP4": build_mp4("hevc"),
        "BAD.MP4": build_bad(),
    }
    for name, data in assets.items():
        path = os.path.join(args.out, name)
        with open(path, "wb") as f:
            f.write(data)
        print("make_mp4_assets: {0} ({1} bytes)".format(name, len(data)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
