#!/usr/bin/env python3
"""Verify Vibio's in-kernel H.264 luma decode against ffmpeg.

Runs build/mp4_probe_host.exe on a clip, parses its reconstructed 16x16 luma
block for macroblock 0 and, when present, the wider luma region. Both are
compared bit-exactly to ffmpeg's decode of the first frame with the in-loop
deblocking filter disabled (so ffmpeg's output is the pre-deblock
reconstruction, which is exactly what intra prediction uses).

This is the honest regression check for the H.264 Constrained Baseline step.
MB0 still guards the original 16x16 gate; the first-row and full-region checks
guard cross-macroblock and top-neighbour luma work.

Usage:  py tools/h264_verify_mb0.py [clip.mp4]   (default Disk/BASE320.MP4)
Requires: build/mp4_probe_host.exe built, ffmpeg on PATH or at C:\\ffmpeg.
"""
import subprocess, os, sys, shutil

clip = sys.argv[1] if len(sys.argv) > 1 else "Disk/BASE320.MP4"
probe = os.path.join("build", "mp4_probe_host.exe")
ffmpeg = shutil.which("ffmpeg") or r"C:\ffmpeg\ffmpeg.exe"
yuv = os.path.join("build", "_verify_mb0.yuv")

if not os.path.exists(probe):
    sys.exit("missing %s (build it first)" % probe)
if not os.path.exists(ffmpeg):
    sys.exit("ffmpeg not found")

subprocess.run([ffmpeg, "-y", "-skip_loop_filter", "all", "-i", clip,
                "-frames:v", "1", "-pix_fmt", "yuv420p", "-f", "rawvideo", yuv],
               stderr=subprocess.DEVNULL)
yb = open(yuv, "rb").read()
# luma plane stride = frame width; read it from the probe's reported width below.

out = subprocess.run([probe, clip], capture_output=True, text=True).stdout
lines = out.splitlines()
width = None
for l in lines:
    if l.startswith("video="):
        # video=H.264/AVC 320x180 ...
        try:
            width = int(l.split()[1].split("x")[0])
        except Exception:
            pass
if width is None:
    sys.exit("could not parse width from probe output")
mb0_idx = [k for k, l in enumerate(lines) if l.startswith("h264_mb0_luma")]
if not mb0_idx:
    print("probe did not reconstruct MB0 luma for", clip)
    print("\n".join(l for l in lines if l.startswith("h264_")))
    sys.exit(1)
i = mb0_idx[0]
got = [[int(x) for x in lines[i + 1 + r].replace(" ", "").split(",")] for r in range(16)]
ff = [[yb[r * width + c] for c in range(16)] for r in range(16)]
mism = sum(1 for r in range(16) for c in range(16) if got[r][c] != ff[r][c])
print("clip=%s  MB0 luma mismatch = %d / 256  %s" %
      (clip, mism, "OK (bit-exact vs ffmpeg)" if mism == 0 else "FAIL"))

region_line = None
region_i = None
for k, l in enumerate(lines):
    if l.startswith("h264_luma_region "):
        region_line = l
    if l.startswith("h264_luma_region_y"):
        region_i = k
if region_line and region_i is not None:
    parts = {}
    for tok in region_line.split():
        if "=" in tok:
            key, val = tok.split("=", 1)
            parts[key] = val
    rw = int(parts.get("width", "0"))
    rh = int(parts.get("height", "0"))
    if rw > 0 and rh > 0:
        reg = [[int(x) for x in lines[region_i + 1 + r].replace(" ", "").split(",")]
               for r in range(rh)]
        ffreg = [[yb[r * width + c] for c in range(rw)] for r in range(rh)]
        reg_mism = sum(1 for r in range(rh) for c in range(rw)
                       if reg[r][c] != ffreg[r][c])
        total = rw * rh
        if rw >= 320 and rh >= 16:
            first_row_mism = sum(1 for r in range(16) for c in range(320)
                                 if reg[r][c] != yb[r * width + c])
            print("clip=%s  first row luma mismatch = %d / 5120  %s" %
                  (clip, first_row_mism,
                   "OK (bit-exact vs ffmpeg)" if first_row_mism == 0 else "FAIL"))
            mism += first_row_mism
        print("clip=%s  luma region %ux%u mismatch = %d / %d  %s" %
              (clip, rw, rh, reg_mism, total,
               "OK (bit-exact vs ffmpeg)" if reg_mism == 0 else "FAIL"))
        if rw >= 64 and rh >= 16:
            mb3_mism = sum(1 for r in range(16) for c in range(48, 64)
                           if reg[r][c] != yb[r * width + c])
            print("clip=%s  MB3 luma mismatch = %d / 256  %s" %
                  (clip, mb3_mism,
                   "OK (bit-exact vs ffmpeg)" if mb3_mism == 0 else "FAIL"))
            mism += mb3_mism
        mism += reg_mism

sys.exit(0 if mism == 0 else 1)
