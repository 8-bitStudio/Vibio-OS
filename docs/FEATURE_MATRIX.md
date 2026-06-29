# Vibio OS - Feature Matrix

Last updated: 2026-06-27 (Stage 38). Honest status, no faking. "VM" = verified
in VirtualBox and/or QEMU; "real HW" = booted on the actual laptop/PC.

## Boot & core kernel

| Capability | Status | Notes |
|---|---|---|
| UEFI bootloader + kernel handoff | Done (VM + real HW) | Loads KERNEL.BIN by exact size at 0x100000 |
| Paging / IDT / PIC / PIT | Done (VM + real HW) | Low-4 GiB identity map + targeted high-MMIO maps |
| PS/2 keyboard + mouse | Done (VM); partial real HW | Touchpad wheel only if IntelliMouse-capable |
| Cooperative tasks + syscalls | Done (VM) | Not preemptive user processes |
| Kernel panic screen | Done (VM) | Red fault screen + auto-reset |

## Desktop shell

| Capability | Status | Notes |
|---|---|---|
| Wallpaper (gradient) | Done (VM + real HW) | |
| Wallpaper (imported G:\Backgrounds photos) | Done (VM) | Host-converted into KERNEL.BIN; runtime load needs no disk |
| Desktop icons: single-select / double-open | Done (code); VM-built | Mouse-only behaviour not headless-screenshot-verifiable |
| Right-click menu + submenus | Done (code); VM-built | |
| Taskbar (Files+Terminal pinned, tray, previews) | Done (VM) | Single-window-per-app |
| Volume popup | Done (code); VM-built | |
| Window manager (drag/min/max/close/F11) | Done (VM); F11 real HW unverified | |

## Apps

| Capability | Status | Notes |
|---|---|---|
| Terminal + commands | Done (VM) | Hidden by default; opens via icon |
| Files (read-only browser) | Done (VM) | AHCI / UEFI-boot / USB sources; scrollbar; long names |
| Browser (local HTML, simple PNG/VIMG) | Done (VM + real HW) | No network |
| Media Viewer (images) | Done (VM) | VIMG/VIM/BMP/simple-PNG native |
| Media Player (VIV video + WAV audio) | Done (VM) | + native MP4 video-only path for current H.264 Baseline test clip |
| Native MP4 container sample demux | Done (VM/QEMU) | `MP4INFO` + player probe; ftyp/moov/trak/stbl/mdat plus stts/stsc/stsz/stco sample offsets/sizes/timing |
| MP4 codec/profile detection | Done (VM/QEMU) | avc1->H.264 (+avcC profile/level/NAL length), mp4a->AAC, hvc1/av01 etc. |
| Native H.264 Constrained Baseline decode | Partial (VM/QEMU) | `BASE320.MP4` 320x180 video-only: I/P frames, CAVLC, luma/chroma, YUV420 frames, YUV->RGB, basic timing. All 4594 coded frames verify bit-exact Y/Cb/Cr vs ffmpeg with `-apply_cropping 0 -skip_loop_filter all`. Deblocking disabled; target is narrow, not general H.264 |
| Native MP4/AAC decode | Partial | H.264 video-only works for the current Baseline clip. AAC is detected but unsupported; no AAC PCM and no A/V sync |

## Storage (READ-ONLY)

| Capability | Status | Notes |
|---|---|---|
| AHCI SATA sector reads | Done (VM) | `READ DMA EXT`; N/A on NVMe real HW |
| FAT32 read (8.3 + VFAT long names) | Done (VM) | Reused across all backends |
| UEFI boot-file preload | Done (VM + real HW) | Bootloader reads files before ExitBootServices |
| **xHCI controller bring-up** | **Done (VM/QEMU)** | Reset, rings, DCBAA, scratchpads, run |
| **USB device enumeration** | **Done (VM/QEMU)** | Slot/address/descriptors; 1 device, 1 config |
| **USB Mass Storage BOT + SCSI READ(10)** | **Done (VM/QEMU)** | INQUIRY/TUR/READ CAPACITY/READ(10); 512 B sectors |
| **FAT32 mount over USB** | **Done (VM/QEMU)** | MBR or superfloppy; Files "USB MASS STORAGE" source |
| USB on real hardware | Not tested | Laptop boot USB is behind real xHCI |
| USB hubs / multi-LUN / hotplug re-enum | Not implemented | |

## Writes (intentionally minimal)

| Capability | Status | Notes |
|---|---|---|
| Stage 29 VM-only FAT32 write sandbox | Done (VM) | ONLY write path; gated to a labelled disposable scratch disk |
| USB writes / general FAT writes / installer | Not implemented (by design) | |

## Audio / media conversion

| Capability | Status | Notes |
|---|---|---|
| AC'97 PCM playback | Done (VM) | HDA detect-only; real HDA machines get no audio |
| Host ffmpeg -> VIV/WAV conversion | Done (build tool) | Still used for legacy VIV/WAV media; current `BASE320.MP4` video path decodes H.264 in-kernel |
| Native MP4 container demuxer (in-kernel) | Done (VM/QEMU) | Isolated `src/kernel/mp4.c`; sample demux + timing used by Media Player |
| In-kernel MP4/H.264/AAC/MP3 decode | Partial | H.264 Baseline video-only for `BASE320.MP4`; AAC/MP3 audio not decoded |

## Not implemented

Networking, GPU/accelerated compositor, NVMe/GPT, USB writes, multi-window-per-
app, real installer, persistent desktop file creation.
