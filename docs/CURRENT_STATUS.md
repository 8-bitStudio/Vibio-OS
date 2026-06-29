# Vibio OS - Current Status

Last updated: 2026-06-27 (Stage 38 native MP4 video-only playback:
H.264 Constrained Baseline 320x180, all 4594 frames bit-exact vs ffmpeg with
deblocking disabled)

Vibio OS is a from-scratch x86_64 UEFI operating system (not Linux/BSD/GRUB). It
boots its own bootloader + kernel, draws a Windows-like desktop shell, and is
developed VM-first (VirtualBox + QEMU) with an honest, no-faking ethos.

## Current stage

**Stage 37 - Read-only xHCI USB mass storage.** Vibio reads files from a USB
drive after boot, by itself, with no UEFI preload: from-scratch xHCI bring-up,
USB enumeration, USB Mass Storage Bulk-Only Transport + SCSI (INQUIRY / TEST
UNIT READY / READ CAPACITY(10) / READ(10) only), FAT32 mounted read-only over
USB, and the Files app browsing it as a live "USB MASS STORAGE" source.

**Stage 38 - Native MP4 container demuxer + video-only H.264 playback** builds
on top: a real ISO-BMFF box parser (separate `src/kernel/mp4.c` module), the
`MP4INFO` command, and a Media Player path that decodes the supplied
`BASE320.MP4` directly in-kernel. The current target is intentionally narrow:
320x180 H.264 Constrained Baseline video-only, no AAC playback, no VIV fallback.

The H.264 decoder remains in `src/kernel/h264.c` / `src/kernel/h264.h`. It now
decodes I and P frames for the current Baseline test clip, including CAVLC,
intra4x4/Intra16x16 prediction, P-skip/inter prediction, motion compensation,
residual add, chroma Cb/Cr reconstruction, and YUV420 output. Media Player owns
bounded frame/work/RGB buffers in `main.c`, calls the decoder API, converts
YUV420 to XRGB, and blits real MP4 frames into the player window with simple MP4
sample timing. `PLAY BASE320.MP4` shows actual decoded video that advances over
time and labels audio honestly as AAC detected/unsupported, playback video-only.

Verification is against ffmpeg with loop filtering disabled because deblocking
is still stubbed/disabled in Vibio. The host harness decoded all 4594 coded
frames of `BASE320.MP4` and matched ffmpeg `-apply_cropping 0 -skip_loop_filter
all` exactly for Y, Cb, and Cr. The earlier frame 116 divergence is fixed; its
root cause was the CAVLC `level_prefix == 14/15 && suffixLength == 0` edge case
in level decoding, not the originally suspected non-IDR I-frame neighbour reset.

`VERSION` still reports `VIBIO OS STAGE 37` (the version string was not bumped
this pass).

## What works (VM-verified)

- UEFI boot -> bootloader -> kernel at 0x100000 (loaded by exact size).
- Paging, IDT, PIC/PIT, PS/2 keyboard + mouse, cooperative tasks, syscalls.
- Desktop shell: wallpaper (gradient + imported G:\Backgrounds photos), movable
  single-click-select / double-click-open icons, right-click menu, taskbar
  (Files + Terminal pinned), tray (power/sound/wifi/clock), volume popup.
- Apps: Terminal, Files (read-only), Browser (local HTML + simple PNG/VIMG),
  Media Viewer (images), Media Player (VIV video + WAV audio + native MP4
  video-only playback for the current Baseline clip).
- Native MP4 container demuxer (Stage 38): opening a real `.mp4` parses the
  ISO-BMFF boxes natively and reports H.264/AAC codec, profile (e.g. Constrained
  Baseline), resolution, duration, sample/chunk tables, and resolved sample
  offsets/sizes/timing via `MP4INFO` and Media Player.
- Native MP4 video-only playback for `BASE320.MP4`: Media Player decodes H.264
  Constrained Baseline frames in-kernel, converts YUV420 to RGB, blits actual
  video frames, and advances over time. AAC is detected but unsupported; no PCM
  is decoded and no audio is played.
- Storage (read-only): AHCI SATA FAT32, UEFI boot-file preload, and now live
  xHCI USB FAT32 - long file names, subfolders, opening files from each.
- AC'97 audio; ACPI shutdown/restart; kernel panic screen.
- 2026-06-27 bug-fix pass (still Stage 37): desktop icon drag no longer smears +
  grid snapping; Files lists up to 256 entries with no size filter on listing;
  draggable Files scrollbar thumb; live taskbar hover previews; bilinear
  (non-blocky) wallpaper scaling; corrected Media Viewer zoom (minus zooms out,
  clean 10% clamp, less lag); AC'97 stops at DMA-halt so a finished sound no
  longer replays a ~0.3 s blip; system volume now keeps an unscaled PCM copy and
  re-renders the DMA buffer on slider changes, so nonzero levels actually change
  sound level even if the AC'97 mixer only honors mute. Mouse/drag/hover + audio
  fixes are source-/boot-verified only (the harness can't drive the guest mouse
  or capture audio).
- Stage 29 VM-only FAT32 write sandbox (the ONLY write-capable path; gated to a
  positively-identified disposable scratch disk).

## What does NOT work / is not implemented

- No USB writes, no FAT/file writes outside the Stage 29 sandbox, no installer.
- USB stack is one controller / one device / one LUN / one config, polled,
  512-byte sectors only; no hubs, multi-LUN, hotplug re-enumeration, or NVMe/GPT.
- Native MP4 playback is limited to the current 320x180 H.264 Constrained
  Baseline video-only target. AAC is detected only; no AAC PCM decode, no A/V
  sync, no MP3-in-MP4 playback, and no H.264 Main/High/HEVC/VP9/AV1 decode.
- H.264 deblocking is still disabled/stubbed. Verification intentionally uses
  ffmpeg with `-skip_loop_filter all`; normal ffmpeg output with deblocking is
  not the current oracle.
- No networking, no GPU/hardware-accelerated compositor, no multi-window-per-app.

## Real-hardware status

The OS also boots on an old laptop from the `G:` USB, but the Stage 37 USB
driver and several Stage 36 mouse-only behaviours are NOT yet verified on real
hardware (no hardware access during development). Treat real-hardware support as
unverified until booted on the target.

## Where things live

- `src/boot/main.c` - UEFI bootloader. `src/kernel/main.c` - the kernel (single
  file). `src/shared/boot_info.h` - the boot handoff ABI.
- `build.ps1` builds everything; `run-vbox.ps1` / `tools/test-vm.ps1` drive the
  VirtualBox VM; `tools/run-qemu.ps1` drives the QEMU xHCI/USB test rig.
- `docs/DEVELOPMENT_LOG.md` - full dated history. `docs/FEATURE_MATRIX.md` -
  capability table. `docs/screenshots/` - per-stage screenshots.
