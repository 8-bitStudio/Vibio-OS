#!/usr/bin/env python3
"""Build the Vibio OS Stage 29 VM-only FAT32 read/write sandbox disk image.

This is a *disposable* scratch disk for the first real filesystem write test. It
is deliberately unmistakable so the kernel can positively identify it and refuse
to write to anything else:

  * FAT32 volume label: VIBIORW
  * marker file in root: VIBIO_RW.TXT  (8.3 short name; content begins
    "VIBIO_RW_OK" - the long form documented as VIBIO_RW_OK.TXT)
  * preallocated test file in root: RWTEST.TXT (exactly one cluster, 512 bytes)

It contains nothing else. It is never the boot disk, never the USB, never a host
or internal disk. The build/test workflow attaches it as a second VM disk only.

Layout mirrors tools/make_fat32_image.py closely so the kernel's existing
read-only FAT32 sector path parses it without special cases, except this image
uses one sector per cluster so RWTEST.TXT occupies exactly one 512-byte sector.
"""
import argparse
import math
import os
import struct


SECTOR_SIZE = 512
PARTITION_START_LBA = 2048
RESERVED_SECTORS = 32
FAT_COUNT = 2
ROOT_CLUSTER = 2
MEDIA_DESCRIPTOR = 0xF8
END_OF_CHAIN = 0x0FFFFFFF
SECTORS_PER_CLUSTER = 1

# The 8.3 short name the kernel looks for, plus the magic the kernel verifies in
# the file content. The longer human name is documented as VIBIO_RW_OK.TXT.
MARKER_NAME = ("VIBIO_RW", "TXT")
MARKER_CONTENT = (
    b"VIBIO_RW_OK\r\n"
    b"Vibio OS VM-only FAT32 read/write sandbox.\r\n"
    b"Disposable scratch disk. Safe to overwrite. Not a real disk.\r\n"
)

RWTEST_NAME = ("RWTEST", "TXT")
RWTEST_SIZE = 512  # exactly one cluster / one sector
RWTEST_CONTENT = b"VIBIO RW SANDBOX OK SEQ=0\n"


def write_le16(buf, offset, value):
    struct.pack_into("<H", buf, offset, value)


def write_le32(buf, offset, value):
    struct.pack_into("<I", buf, offset, value)


def short_name(name, ext=""):
    return name.encode("ascii").ljust(8, b" ")[:8] + ext.encode("ascii").ljust(3, b" ")[:3]


def dir_entry(name11, attr, cluster, size=0):
    entry = bytearray(32)
    entry[0:11] = name11
    entry[11] = attr
    write_le16(entry, 20, (cluster >> 16) & 0xFFFF)
    write_le16(entry, 26, cluster & 0xFFFF)
    write_le32(entry, 28, size)
    return entry


def cluster_offset(partition_offset, first_data_sector, cluster):
    sector = first_data_sector + (cluster - 2) * SECTORS_PER_CLUSTER
    return partition_offset + sector * SECTOR_SIZE


def write_cluster(image, partition_offset, first_data_sector, cluster, data):
    offset = cluster_offset(partition_offset, first_data_sector, cluster)
    size = SECTORS_PER_CLUSTER * SECTOR_SIZE
    image[offset : offset + size] = data.ljust(size, b"\0")[:size]


def calculate_fat_size(partition_sectors):
    fat_size = 1
    while True:
        data_sectors = partition_sectors - RESERVED_SECTORS - FAT_COUNT * fat_size
        cluster_count = data_sectors // SECTORS_PER_CLUSTER
        needed = math.ceil((cluster_count + 2) * 4 / SECTOR_SIZE)
        if needed == fat_size:
            return fat_size, cluster_count
        fat_size = needed


def build_image(out_path, label, size_mib):
    total_sectors = size_mib * 1024 * 1024 // SECTOR_SIZE
    partition_sectors = total_sectors - PARTITION_START_LBA
    fat_size, cluster_count = calculate_fat_size(partition_sectors)
    first_data_sector = RESERVED_SECTORS + FAT_COUNT * fat_size
    partition_offset = PARTITION_START_LBA * SECTOR_SIZE

    if cluster_count < 65525:
        raise ValueError(
            "FAT32 partition is too small (%d clusters); use a larger --size-mib"
            % cluster_count)

    image = bytearray(total_sectors * SECTOR_SIZE)

    # MBR with one FAT32 (LBA) partition. This is a local VM scratch disk only.
    mbr = bytearray(SECTOR_SIZE)
    mbr[446 + 4] = 0x0C  # partition type: FAT32 (LBA)
    mbr[446 + 1 : 446 + 4] = b"\xFE\xFF\xFF"
    mbr[446 + 5 : 446 + 8] = b"\xFE\xFF\xFF"
    write_le32(mbr, 446 + 8, PARTITION_START_LBA)
    write_le32(mbr, 446 + 12, partition_sectors)
    mbr[510:512] = b"\x55\xAA"
    image[0:SECTOR_SIZE] = mbr

    volume_label = label.upper().encode("ascii", errors="ignore")[:11].ljust(11, b" ")

    boot = bytearray(SECTOR_SIZE)
    boot[0:3] = b"\xEB\x58\x90"
    boot[3:11] = b"VIBIOOS "
    write_le16(boot, 11, SECTOR_SIZE)
    boot[13] = SECTORS_PER_CLUSTER
    write_le16(boot, 14, RESERVED_SECTORS)
    boot[16] = FAT_COUNT
    write_le16(boot, 17, 0)
    write_le16(boot, 19, 0)
    boot[21] = MEDIA_DESCRIPTOR
    write_le16(boot, 22, 0)
    write_le16(boot, 24, 63)
    write_le16(boot, 26, 255)
    write_le32(boot, 28, PARTITION_START_LBA)
    write_le32(boot, 32, partition_sectors)
    write_le32(boot, 36, fat_size)
    write_le16(boot, 40, 0)
    write_le16(boot, 42, 0)
    write_le32(boot, 44, ROOT_CLUSTER)
    write_le16(boot, 48, 1)
    write_le16(boot, 50, 6)
    boot[64] = 0x80
    boot[66] = 0x29
    write_le32(boot, 67, 0x56494257)  # arbitrary volume serial
    boot[71:82] = volume_label
    boot[82:90] = b"FAT32   "
    boot[510:512] = b"\x55\xAA"
    image[partition_offset : partition_offset + SECTOR_SIZE] = boot
    image[partition_offset + 6 * SECTOR_SIZE : partition_offset + 7 * SECTOR_SIZE] = boot

    fsinfo = bytearray(SECTOR_SIZE)
    write_le32(fsinfo, 0, 0x41615252)
    write_le32(fsinfo, 484, 0x61417272)
    write_le32(fsinfo, 488, 0xFFFFFFFF)
    write_le32(fsinfo, 492, 0xFFFFFFFF)
    fsinfo[508:512] = b"\x00\x00\x55\xAA"
    image[partition_offset + SECTOR_SIZE : partition_offset + 2 * SECTOR_SIZE] = fsinfo
    image[partition_offset + 7 * SECTOR_SIZE : partition_offset + 8 * SECTOR_SIZE] = fsinfo

    # Clusters: 2 = root dir, 3 = marker file, 4 = RWTEST.TXT.
    marker_cluster = 3
    rwtest_cluster = 4

    fat_entries = [0] * (cluster_count + 2)
    fat_entries[0] = 0x0FFFFF00 | MEDIA_DESCRIPTOR
    fat_entries[1] = 0xFFFFFFFF
    fat_entries[ROOT_CLUSTER] = END_OF_CHAIN
    fat_entries[marker_cluster] = END_OF_CHAIN
    fat_entries[rwtest_cluster] = END_OF_CHAIN

    fat = bytearray(fat_size * SECTOR_SIZE)
    for idx, value in enumerate(fat_entries):
        write_le32(fat, idx * 4, value)

    fat1_offset = partition_offset + RESERVED_SECTORS * SECTOR_SIZE
    fat2_offset = fat1_offset + fat_size * SECTOR_SIZE
    image[fat1_offset : fat1_offset + len(fat)] = fat
    image[fat2_offset : fat2_offset + len(fat)] = fat

    root = bytearray()
    root += dir_entry(volume_label, 0x08, 0)
    root += dir_entry(short_name(*MARKER_NAME), 0x21, marker_cluster, len(MARKER_CONTENT))
    root += dir_entry(short_name(*RWTEST_NAME), 0x20, rwtest_cluster, RWTEST_SIZE)
    write_cluster(image, partition_offset, first_data_sector, ROOT_CLUSTER, root)

    write_cluster(image, partition_offset, first_data_sector, marker_cluster, MARKER_CONTENT)
    write_cluster(image, partition_offset, first_data_sector, rwtest_cluster,
                  RWTEST_CONTENT.ljust(RWTEST_SIZE, b"\0"))

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(image)

    print("Wrote %s" % out_path)
    print("  label            : %s" % label.upper())
    print("  size             : %d MiB" % size_mib)
    print("  sectors/cluster  : %d" % SECTORS_PER_CLUSTER)
    print("  clusters         : %d" % cluster_count)
    print("  marker file      : %s.%s (content begins VIBIO_RW_OK)" % MARKER_NAME)
    print("  test file        : %s.%s (%d bytes, 1 cluster, preallocated)"
          % (RWTEST_NAME[0], RWTEST_NAME[1], RWTEST_SIZE))


def main():
    parser = argparse.ArgumentParser(
        description="Create the Vibio OS VM-only FAT32 read/write sandbox image.")
    parser.add_argument("--out", required=True, help="Output raw disk image")
    parser.add_argument("--label", default="VIBIORW", help="FAT volume label (must be VIBIORW)")
    parser.add_argument("--size-mib", type=int, default=64, help="Disk size in MiB")
    args = parser.parse_args()

    build_image(args.out, args.label, args.size_mib)


if __name__ == "__main__":
    main()
