#!/usr/bin/env python3
"""Build Vibio media assets: VIMG/VIM fallbacks, VIV test video, MEDIA.MF manifest.

Requires Pillow for images. ffmpeg is optional (used for converting host videos).
The build fails clearly if Pillow is missing when image conversion is needed.
"""
import argparse
import math
import os
import struct
import subprocess
import sys
import wave

try:
    from PIL import Image, ImageDraw
except ImportError:
    Image = None
    ImageDraw = None

VIMG1_MAX_DIMENSION = 2048
VIMG1_VIEWER_MAX = 2048
VIV_MAX_WIDTH = 854
VIV_MAX_HEIGHT = 480
VIV_MAX_FRAMES = 3600
IMAGE_EXTS = (".PNG", ".JPG", ".JPEG", ".BMP", ".GIF", ".WEBP")
VIDEO_EXTS = (".MP4", ".MOV", ".WEBM", ".AVI", ".MKV", ".M4V", ".GIF")


def short_fat_base(name):
    base = os.path.splitext(os.path.basename(name))[0].upper()
    fat = "".join(c for c in base if c.isalnum())[:8]
    return fat or None


def vimg1_checksum(pixels):
    total = 0
    for b in pixels:
        total = (total + b) & 0xFFFFFFFF
    return total


def make_vimg1_bytes(img, max_dim=VIMG1_VIEWER_MAX):
    """VIMG version 1: magic VIMG + version + w + h + flags + checksum + RGBA."""
    img = img.convert("RGBA")
    w, h = img.size
    if w <= 0 or h <= 0:
        raise ValueError("invalid image dimensions")
    scale = min(1.0, max_dim / max(w, h))
    if scale < 1.0:
        nw = max(1, int(w * scale))
        nh = max(1, int(h * scale))
        img = img.resize((nw, nh), Image.Resampling.LANCZOS)
        w, h = img.size
    pixels = img.tobytes()
    flags = 1
    checksum = vimg1_checksum(pixels)
    return (
        b"VIMG"
        + struct.pack("<IIIII", 1, w, h, flags, checksum)
        + pixels
    )


def make_legacy_vim_bytes(img):
    """Legacy Stage 33 VIMG header (no version field) for backward compatibility."""
    img = img.convert("RGBA")
    w, h = img.size
    scale = min(1.0, VIMG1_VIEWER_MAX / max(w, h))
    if scale < 1.0:
        nw = max(1, int(w * scale))
        nh = max(1, int(h * scale))
        img = img.resize((nw, nh), Image.Resampling.LANCZOS)
        w, h = img.size
    pixels = img.tobytes()
    return b"VIMG" + struct.pack("<III", w, h, 1) + pixels


def write_manifest_line(lines, original, fallback, width, height, orig_size, conv_size, fmt):
    lines.append(
        f"{original}|{fallback}|{width}|{height}|{orig_size}|{conv_size}|{fmt}"
    )


def ensure_test_assets(assets_dir):
    os.makedirs(assets_dir, exist_ok=True)
    test_png = os.path.join(assets_dir, "TEST.PNG")
    photo_jpg = os.path.join(assets_dir, "PHOTO.JPG")
    tiny_rgb_png = os.path.join(assets_dir, "TINYRGB.PNG")
    alpha_png = os.path.join(assets_dir, "ALPHA.PNG")
    gray_jpg = os.path.join(assets_dir, "GRAY.JPG")
    progress_jpg = os.path.join(assets_dir, "PROGRESS.JPG")
    if Image is None:
        return
    if not os.path.isfile(test_png):
        img = Image.new("RGBA", (320, 200), (32, 64, 96, 255))
        draw = ImageDraw.Draw(img)
        draw.rectangle((20, 20, 300, 180), fill=(56, 180, 210, 255))
        draw.text((36, 88), "VIBIO TEST PNG", fill=(255, 255, 255, 255))
        img.save(test_png, "PNG")
    if not os.path.isfile(photo_jpg):
        img = Image.new("RGB", (400, 260), (90, 40, 120))
        draw = ImageDraw.Draw(img)
        draw.ellipse((40, 30, 360, 230), fill=(240, 180, 60))
        draw.text((70, 118), "PHOTO JPG FALLBACK", fill=(20, 20, 30))
        img.save(photo_jpg, "JPEG", quality=85)
    if not os.path.isfile(tiny_rgb_png):
        img = Image.new("RGB", (48, 32), (28, 78, 132))
        draw = ImageDraw.Draw(img)
        draw.rectangle((4, 4, 43, 27), outline=(236, 238, 214), width=2)
        draw.line((6, 25, 20, 10, 34, 24, 44, 8), fill=(80, 226, 208), width=3)
        img.save(tiny_rgb_png, "PNG")
    if not os.path.isfile(alpha_png):
        img = Image.new("RGBA", (48, 32), (18, 26, 36, 0))
        draw = ImageDraw.Draw(img)
        draw.rectangle((0, 0, 47, 31), fill=(18, 26, 36, 255))
        draw.ellipse((6, 5, 29, 28), fill=(68, 220, 210, 150))
        draw.rectangle((20, 8, 43, 24), fill=(246, 190, 65, 220))
        draw.line((4, 29, 44, 3), fill=(248, 250, 252, 255), width=2)
        img.save(alpha_png, "PNG")
    if not os.path.isfile(gray_jpg):
        img = Image.new("L", (64, 40), 32)
        draw = ImageDraw.Draw(img)
        for x in range(64):
            shade = 32 + (x * 180) // 63
            draw.line((x, 0, x, 39), fill=shade)
        draw.rectangle((8, 8, 55, 31), outline=230, width=2)
        draw.line((12, 29, 28, 13, 44, 26), fill=245, width=3)
        img.save(gray_jpg, "JPEG", quality=88)
    if not os.path.isfile(progress_jpg):
        img = Image.new("RGB", (64, 40), (44, 52, 88))
        draw = ImageDraw.Draw(img)
        draw.rectangle((6, 6, 57, 33), fill=(82, 122, 164))
        draw.ellipse((37, 8, 55, 26), fill=(245, 188, 65))
        draw.line((9, 30, 24, 16, 38, 30), fill=(85, 232, 216), width=3)
        img.save(progress_jpg, "JPEG", quality=85, progressive=True)


def scan_media_dirs(root, extra_dirs):
    dirs = []
    for rel in ("Assets", "Assets/Media", "DOCS"):
        path = os.path.join(root, rel)
        if os.path.isdir(path):
            dirs.append(path)
    for d in extra_dirs:
        if d and os.path.isdir(d):
            dirs.append(os.path.abspath(d))
    seen = set()
    unique = []
    for d in dirs:
        key = os.path.normcase(os.path.abspath(d))
        if key not in seen:
            seen.add(key)
            unique.append(d)
    return unique


def convert_images(root, out_dir, extra_dirs, manifest_lines, errors):
    if Image is None:
        errors.append("Pillow is required for image conversion (pip install pillow)")
        return 0
    ensure_test_assets(os.path.join(root, "Assets"))
    converted = 0
    seen_sources = set()
    for media_dir in scan_media_dirs(root, extra_dirs):
        for name in sorted(os.listdir(media_dir)):
            path = os.path.join(media_dir, name)
            if not os.path.isfile(path):
                continue
            ext = os.path.splitext(name)[1].upper()
            if ext not in IMAGE_EXTS:
                continue
            key = os.path.normcase(os.path.abspath(path))
            if key in seen_sources:
                continue
            seen_sources.add(key)
            fat = short_fat_base(name)
            if not fat:
                errors.append(f"skip image with no 8.3-safe name: {path}")
                continue
            try:
                with Image.open(path) as img:
                    img.seek(0)
                    vimg = make_vimg1_bytes(img)
                    legacy = make_legacy_vim_bytes(img)
                    w = struct.unpack_from("<I", vimg, 8)[0]
                    h = struct.unpack_from("<I", vimg, 12)[0]
            except Exception as exc:
                errors.append(f"failed to convert {path}: {exc}")
                continue
            orig_size = os.path.getsize(path)
            vimg_name = f"{fat}.VIMG"
            vim_name = f"{fat}.VIM"
            with open(os.path.join(out_dir, vimg_name), "wb") as f:
                f.write(vimg)
            with open(os.path.join(out_dir, vim_name), "wb") as f:
                f.write(legacy)
            write_manifest_line(
                manifest_lines,
                name.upper(),
                vimg_name,
                w,
                h,
                orig_size,
                len(vimg),
                ext.lstrip("."),
            )
            converted += 1
    return converted


def write_tone_wav(path, seconds=2.0, hz=440, rate=44100):
    nframes = int(seconds * rate)
    with wave.open(path, "w") as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(rate)
        for i in range(nframes):
            t = i / rate
            amp = int(12000 * math.sin(2 * math.pi * hz * t))
            sample = max(-32768, min(32767, amp))
            lo = sample & 0xFF
            hi = (sample >> 8) & 0xFF
            wf.writeframes(bytes((lo, hi, lo, hi)))


def make_viv1(frames_rgba, width, height, fps_num=15, fps_den=1):
    if len(frames_rgba) == 0:
        raise ValueError("VIV requires at least one frame")
    if width > VIV_MAX_WIDTH or height > VIV_MAX_HEIGHT:
        raise ValueError("VIV resolution exceeds safe kernel limits")
    if len(frames_rgba) > VIV_MAX_FRAMES:
        raise ValueError("too many VIV frames")
    header_size = 32
    index_size = len(frames_rgba) * 8
    offset = header_size + index_size
    index = []
    body = bytearray()
    # Each frame is stored as a self-contained legacy VIMG bitmap (magic + w + h
    # + flags + RGBA), because the kernel's Media Player decodes every frame with
    # the same media_vimg_parse() it uses for still images. Storing raw RGBA here
    # (as earlier builds did) made every frame fail to parse and render black.
    vimg_header = b"VIMG" + struct.pack("<III", width, height, 1)
    for frame in frames_rgba:
        if len(frame) != width * height * 4:
            raise ValueError("frame size mismatch")
        encoded = vimg_header + frame
        index.append((offset, len(encoded)))
        body.extend(encoded)
        offset += len(encoded)
    out = bytearray()
    out.extend(b"VIV1")
    out.extend(
        struct.pack(
            "<IIIIIIHH",
            width,
            height,
            len(frames_rgba),
            fps_num,
            fps_den,
            0,
            0,
            0,
        )
    )
    for off, size in index:
        out.extend(struct.pack("<II", off, size))
    out.extend(body)
    return bytes(out)


def generate_test_video(out_dir, manifest_lines):
    if Image is None:
        return 0
    width, height = 320, 180
    fps = 15
    seconds = 2
    frame_count = fps * seconds
    colors = [
        (180, 60, 60),
        (60, 160, 90),
        (60, 100, 200),
        (200, 160, 40),
    ]
    frames = []
    for i in range(frame_count):
        r, g, b = colors[(i // 8) % len(colors)]
        img = Image.new("RGBA", (width, height), (r, g, b, 255))
        draw = ImageDraw.Draw(img)
        draw.text((24, height // 2 - 8), f"VIDEOTST {i + 1}/{frame_count}", fill=(255, 255, 255, 255))
        frames.append(img.tobytes())
    viv = make_viv1(frames, width, height, fps, 1)
    viv_path = os.path.join(out_dir, "VIDEOTST.VIV")
    wav_path = os.path.join(out_dir, "VIDEOTST.WAV")
    with open(viv_path, "wb") as f:
        f.write(viv)
    write_tone_wav(wav_path, seconds=seconds)
    write_manifest_line(
        manifest_lines,
        "VIDEOTST.VIV",
        "VIDEOTST.VIV",
        width,
        height,
        len(viv),
        len(viv),
        "VIV",
    )
    return 1


def ffmpeg_available():
    try:
        subprocess.run(
            ["ffmpeg", "-version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        return True
    except OSError:
        return False


def convert_host_videos(root, out_dir, extra_dirs, manifest_lines, errors):
    if not ffmpeg_available():
        return 0
    if Image is None:
        return 0
    converted = 0
    for media_dir in scan_media_dirs(root, extra_dirs):
        for name in sorted(os.listdir(media_dir)):
            ext = os.path.splitext(name)[1].upper()
            if ext not in VIDEO_EXTS:
                continue
            fat = short_fat_base(name)
            if not fat:
                continue
            src = os.path.join(media_dir, name)
            tmp_dir = os.path.join(out_dir, "_tmp_" + fat)
            os.makedirs(tmp_dir, exist_ok=True)
            pattern = os.path.join(tmp_dir, "frame_%05d.png")
            # Preserve the source aspect ratio, scaled DOWN only to fit within the
            # kernel's max frame (no upscaling), with even dimensions. Frames are
            # raw RGBA, so files grow fast - keep fps modest and cap total frames.
            vf = (
                "scale='min({w},iw)':'min({h},ih)':"
                "force_original_aspect_ratio=decrease,"
                "scale=trunc(iw/2)*2:trunc(ih/2)*2"
            ).format(w=VIV_MAX_WIDTH, h=VIV_MAX_HEIGHT)
            fps = 15
            cmd = [
                "ffmpeg", "-y", "-i", src,
                "-vf", vf,
                "-r", str(fps),
                "-frames:v", str(VIV_MAX_FRAMES),
                pattern,
            ]
            try:
                subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except subprocess.CalledProcessError as exc:
                errors.append(f"ffmpeg failed for {src}: {exc}")
                continue
            frame_files = sorted(
                f for f in os.listdir(tmp_dir) if f.lower().endswith(".png")
            )
            if not frame_files:
                errors.append(f"ffmpeg produced no frames for {src}")
                continue
            frames = []
            vw = vh = 0
            for ff in frame_files:
                with Image.open(os.path.join(tmp_dir, ff)) as img:
                    rgba = img.convert("RGBA")
                    vw, vh = rgba.size
                    frames.append(rgba.tobytes())
            viv = make_viv1(frames, vw, vh, fps, 1)
            out_name = f"{fat}.VIV"
            with open(os.path.join(out_dir, out_name), "wb") as f:
                f.write(viv)
            print("make_media_assets: video {0} -> {1} ({2}x{3}, {4} frames, {5:.1f} MB)".format(
                name, out_name, vw, vh, len(frames), len(viv) / (1024.0 * 1024.0)))
            write_manifest_line(
                manifest_lines,
                name.upper(),
                out_name,
                vw,
                vh,
                os.path.getsize(src),
                len(viv),
                ext.lstrip("."),
            )
            converted += 1
            for ff in frame_files:
                try:
                    os.remove(os.path.join(tmp_dir, ff))
                except OSError:
                    pass
            try:
                os.rmdir(tmp_dir)
            except OSError:
                pass
    return converted


def build_media_assets(root, out_dir, extra_dirs=None, require_pillow=True):
    extra_dirs = extra_dirs or []
    os.makedirs(out_dir, exist_ok=True)
    manifest_lines = ["# Vibio MEDIA.MF - original|fallback|w|h|orig_bytes|conv_bytes|fmt"]
    errors = []
    if require_pillow and Image is None:
        raise RuntimeError("Pillow is required. Install with: pip install pillow")
    img_count = convert_images(root, out_dir, extra_dirs, manifest_lines, errors)
    vid_count = generate_test_video(out_dir, manifest_lines)
    vid_count += convert_host_videos(root, out_dir, extra_dirs, manifest_lines, errors)
    manifest_path = os.path.join(out_dir, "MEDIA.MF")
    with open(manifest_path, "w", encoding="ascii") as f:
        f.write("\n".join(manifest_lines) + "\n")
    for msg in errors:
        print(f"make_media_assets: {msg}", file=sys.stderr)
    print(
        f"make_media_assets: {img_count} image fallbacks, {vid_count} video(s), "
        f"manifest -> {manifest_path}"
    )
    return img_count, vid_count, errors


def main():
    parser = argparse.ArgumentParser(description="Build Vibio media fallbacks and test video.")
    parser.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    parser.add_argument("--out", required=True, help="Output directory (e.g. build/media)")
    parser.add_argument("--media-dir", action="append", default=[], help="Extra media scan folder")
    parser.add_argument(
        "--allow-missing-pillow",
        action="store_true",
        help="Do not fail if Pillow is missing (skips conversion)",
    )
    args = parser.parse_args()
    try:
        build_media_assets(
            args.root,
            args.out,
            args.media_dir,
            require_pillow=not args.allow_missing_pillow,
        )
    except RuntimeError as exc:
        print(f"make_media_assets: ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
