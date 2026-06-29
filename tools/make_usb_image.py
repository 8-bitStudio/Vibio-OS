#!/usr/bin/env python3
"""Build a small MBR + FAT32 USB test image for the Stage 37 read-only xHCI USB
mass-storage driver. This is the disk QEMU exposes as the usb-storage stick
(build/qemu-usb.img), so we can prove Vibio reads files from USB *after boot*,
not via the UEFI preload.

It deliberately reuses the FAT32 primitives in make_fat32_image.py (same on-disk
layout the kernel's read-only parser already understands) and lays down a mix of
content: a text doc, a long-named doc, a native VIMG image, a WAV, an optional
VIV video, and a subfolder - so the Files app and viewers can open real files
from USB. Nothing here runs on the target; it only writes build/qemu-usb.img.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_fat32_image as fat  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SPC = 1  # sectors per cluster (1 keeps the minimum FAT32 image small)


def make_vimg_solid(w, h):
    """A tiny self-contained VIMG (Vibio-native RGBA bitmap) so the image-open
    path can be exercised without Pillow: a blue->teal gradient."""
    px = bytearray()
    for y in range(h):
        for x in range(w):
            r = 20 + (x * 60) // max(1, w)
            g = 80 + (y * 120) // max(1, h)
            b = 150
            px += bytes((r, g, b, 255))
    return b"VIMG" + struct.pack("<III", w, h, 1) + bytes(px)


def read_optional(path):
    try:
        with open(path, "rb") as f:
            return f.read()
    except Exception:
        return None


def build(out_path, size_mib, label, files_dir=None):
    SECTOR = fat.SECTOR_SIZE
    have_files = bool(files_dir) and os.path.isdir(files_dir)
    # Grow the stick to fit the whole verbatim folder.
    if have_files:
        needed = fat.tree_total_bytes(files_dir) + 8 * 1024 * 1024
        min_mib = int((needed * 13) // (10 * 1024 * 1024)) + 8
        if size_mib < min_mib:
            size_mib = min_mib
    total_sectors = size_mib * 1024 * 1024 // SECTOR
    partition_sectors = total_sectors - fat.PARTITION_START_LBA
    fat_size, cluster_count = fat.calculate_fat_size(partition_sectors, SPC)
    if cluster_count < 65525:
        raise ValueError("USB FAT32 image too small (need >= 65525 clusters)")
    first_data_sector = fat.RESERVED_SECTORS + fat.FAT_COUNT * fat_size
    partition_offset = fat.PARTITION_START_LBA * SECTOR
    cluster_size = SPC * SECTOR
    image = bytearray(total_sectors * SECTOR)

    # ---- MBR with a single FAT32-LBA (0x0C) primary partition. ----
    mbr = bytearray(SECTOR)
    mbr[446 + 0] = 0x80  # bootable flag (cosmetic)
    mbr[446 + 4] = 0x0C  # type: FAT32 LBA
    mbr[446 + 1 : 446 + 4] = b"\xFE\xFF\xFF"
    mbr[446 + 5 : 446 + 8] = b"\xFE\xFF\xFF"
    fat.write_le32(mbr, 446 + 8, fat.PARTITION_START_LBA)
    fat.write_le32(mbr, 446 + 12, partition_sectors)
    mbr[510:512] = b"\x55\xAA"
    image[0:SECTOR] = mbr

    volume_label = label.upper().encode("ascii", "ignore")[:11].ljust(11, b" ")

    # ---- FAT32 boot sector (BPB). ----
    boot = bytearray(SECTOR)
    boot[0:3] = b"\xEB\x58\x90"
    boot[3:11] = b"VIBIOUSB"
    fat.write_le16(boot, 11, SECTOR)
    boot[13] = SPC
    fat.write_le16(boot, 14, fat.RESERVED_SECTORS)
    boot[16] = fat.FAT_COUNT
    boot[21] = fat.MEDIA_DESCRIPTOR
    fat.write_le16(boot, 24, 63)
    fat.write_le16(boot, 26, 255)
    fat.write_le32(boot, 28, fat.PARTITION_START_LBA)
    fat.write_le32(boot, 32, partition_sectors)
    fat.write_le32(boot, 36, fat_size)
    fat.write_le32(boot, 44, fat.ROOT_CLUSTER)
    fat.write_le16(boot, 48, 1)
    fat.write_le16(boot, 50, 6)
    boot[64] = 0x80
    boot[66] = 0x29
    fat.write_le32(boot, 67, 0x55534200)
    boot[71:82] = volume_label
    boot[82:90] = b"FAT32   "
    boot[510:512] = b"\x55\xAA"
    image[partition_offset : partition_offset + SECTOR] = boot
    image[partition_offset + 6 * SECTOR : partition_offset + 7 * SECTOR] = boot

    fsinfo = bytearray(SECTOR)
    fat.write_le32(fsinfo, 0, 0x41615252)
    fat.write_le32(fsinfo, 484, 0x61417272)
    fat.write_le32(fsinfo, 488, 0xFFFFFFFF)
    fat.write_le32(fsinfo, 492, 0xFFFFFFFF)
    fsinfo[508:512] = b"\x00\x00\x55\xAA"
    image[partition_offset + SECTOR : partition_offset + 2 * SECTOR] = fsinfo
    image[partition_offset + 7 * SECTOR : partition_offset + 8 * SECTOR] = fsinfo

    # ---- FAT. ----
    fat_entries = [0] * (cluster_count + 2)
    fat_entries[0] = 0x0FFFFF00 | fat.MEDIA_DESCRIPTOR
    fat_entries[1] = 0xFFFFFFFF

    # ---- Copy the entire Usb\ folder verbatim onto the stick root (any type,
    #      subfolders included) - exactly like a real USB drive. ----
    next_cluster = [3]  # mutable holder for the recursive packer
    extra_root_entries = b""
    if have_files:
        state = {"next": next_cluster[0]}
        extra_root_entries = fat.pack_tree(
            image, partition_offset, first_data_sector, SPC, cluster_size,
            fat_entries, state, files_dir, fat.ROOT_CLUSTER, fat.ROOT_CLUSTER, True)
        next_cluster[0] = state["next"]

    # ---- Root directory (multi-cluster chain so it can hold many entries). ----
    root = bytearray()
    root += fat.dir_entry(volume_label, 0x08, 0)
    root += extra_root_entries
    import math as _math
    root_cluster_count = max(1, _math.ceil(len(root) / cluster_size))
    root_clusters = [fat.ROOT_CLUSTER] + list(
        range(next_cluster[0], next_cluster[0] + root_cluster_count - 1))
    next_cluster[0] += root_cluster_count - 1
    fat.mark_cluster_chain(fat_entries, root_clusters)

    if next_cluster[0] > cluster_count + 2:
        raise ValueError("USB FAT32 image too small for its contents")

    fat_bytes = bytearray(fat_size * SECTOR)
    for idx, value in enumerate(fat_entries):
        fat.write_le32(fat_bytes, idx * 4, value)
    fat1 = partition_offset + fat.RESERVED_SECTORS * SECTOR
    fat2 = fat1 + fat_size * SECTOR
    image[fat1 : fat1 + len(fat_bytes)] = fat_bytes
    image[fat2 : fat2 + len(fat_bytes)] = fat_bytes

    fat.write_file_clusters(image, partition_offset, first_data_sector, SPC,
                            root_clusters, bytes(root))

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(image)
    print("make_usb_image: wrote %s (%d MiB FAT32, verbatim folder=%s)" %
          (out_path, size_mib, files_dir if have_files else "<none>"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "qemu-usb.img"))
    ap.add_argument("--size-mib", type=int, default=64)
    ap.add_argument("--label", default="VIBIO USB")
    ap.add_argument("--files", default=os.path.join(ROOT, "Usb"),
                    help="Folder whose entire contents are copied verbatim onto the USB stick")
    args = ap.parse_args()
    build(args.out, args.size_mib, args.label, args.files)


if __name__ == "__main__":
    main()
