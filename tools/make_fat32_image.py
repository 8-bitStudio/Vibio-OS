#!/usr/bin/env python3
import argparse
import math
import os
import struct

try:
    from PIL import Image
except Exception:
    Image = None


SECTOR_SIZE = 512
PARTITION_START_LBA = 2048
RESERVED_SECTORS = 32
FAT_COUNT = 2
ROOT_CLUSTER = 2
MEDIA_DESCRIPTOR = 0xF8
END_OF_CHAIN = 0x0FFFFFFF
SOUND_ASSETS = [
    ("Boot.wav", "BOOT", "WAV"),
    ("Error.wav", "ERROR", "WAV"),
    ("notify.wav", "NOTIFY", "WAV"),
    ("Usb Insert.wav", "USBINS", "WAV"),
    ("Usb Remove.wav", "USBRM", "WAV"),
]


def write_le16(buf, offset, value):
    struct.pack_into("<H", buf, offset, value)


def write_le32(buf, offset, value):
    struct.pack_into("<I", buf, offset, value)


def write_le64(buf, offset, value):
    struct.pack_into("<Q", buf, offset, value)


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


def lfn_checksum(name11):
    s = 0
    for b in name11:
        s = (((s & 1) << 7) + (s >> 1) + b) & 0xFF
    return s


# VFAT long-name slot byte offsets within a 32-byte LFN entry (5 + 6 + 2 chars).
_LFN_OFFS = (1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30)


def lfn_entries(long_name, name11):
    """Return the LFN directory entries that must precede the 8.3 entry so a
    VFAT reader sees `long_name`. Real OSes (and Vibio's reader) assemble these."""
    chksum = lfn_checksum(name11)
    chars = [ord(c) for c in long_name]
    chars.append(0x0000)  # NUL terminator
    while len(chars) % 13 != 0:
        chars.append(0xFFFF)  # padding
    groups = len(chars) // 13
    entries = []
    for g in range(groups):
        entry = bytearray(32)
        order = (g + 1) | (0x40 if g == groups - 1 else 0)
        entry[0] = order
        entry[11] = 0x0F  # ATTR_LONG_NAME
        entry[13] = chksum
        chunk = chars[g * 13 : (g + 1) * 13]
        for i in range(13):
            ch = chunk[i]
            entry[_LFN_OFFS[i]] = ch & 0xFF
            entry[_LFN_OFFS[i] + 1] = (ch >> 8) & 0xFF
        entries.append(entry)
    entries.reverse()  # highest sequence (0x40) physically first
    out = bytearray()
    for e in entries:
        out += e
    return out


def file_dir_entries(asset, attr):
    """Short 8.3 entry, preceded by LFN entries when the asset kept a long name."""
    name11 = asset["name11"]
    size = len(asset["data"])
    cluster = asset["clusters"][0]
    long_name = asset.get("long_name")
    out = bytearray()
    if long_name:
        out += lfn_entries(long_name, name11)
    out += dir_entry(name11, attr, cluster, size)
    return out


def cluster_offset(partition_offset, first_data_sector, sectors_per_cluster, cluster):
    sector = first_data_sector + (cluster - 2) * sectors_per_cluster
    return partition_offset + sector * SECTOR_SIZE


def write_cluster(image, partition_offset, first_data_sector, sectors_per_cluster, cluster, data):
    offset = cluster_offset(partition_offset, first_data_sector, sectors_per_cluster, cluster)
    size = sectors_per_cluster * SECTOR_SIZE
    image[offset : offset + size] = data.ljust(size, b"\0")[:size]


def allocate_cluster_chain(start_cluster, data_length, cluster_size):
    cluster_count = max(1, math.ceil(data_length / cluster_size))
    return list(range(start_cluster, start_cluster + cluster_count))


def mark_cluster_chain(fat_entries, clusters):
    for idx, cluster in enumerate(clusters):
        fat_entries[cluster] = END_OF_CHAIN if idx == len(clusters) - 1 else clusters[idx + 1]


def write_file_clusters(image, partition_offset, first_data_sector, sectors_per_cluster, clusters, data):
    cluster_size = sectors_per_cluster * SECTOR_SIZE

    for idx, cluster in enumerate(clusters):
        start = idx * cluster_size
        end = start + cluster_size
        write_cluster(image, partition_offset, first_data_sector, sectors_per_cluster, cluster, data[start:end])


IMAGE_EXTS = ("PNG", "JPG", "JPEG", "BMP", "GIF")
VIMG_MAX_DIMENSION = 512


def vimg_name_for_base(base):
    fat_name = "".join(c for c in base.upper() if c.isalnum())[:8]
    if not fat_name:
        return None
    return short_name(fat_name, "VIM")


def make_vimg(path):
    if Image is None:
        return None
    with Image.open(path) as img:
        img.seek(0)
        img = img.convert("RGBA")
        w, h = img.size
        if w <= 0 or h <= 0:
            return None
        scale = min(1.0, VIMG_MAX_DIMENSION / max(w, h))
        if scale < 1.0:
            nw = max(1, int(w * scale))
            nh = max(1, int(h * scale))
            img = img.resize((nw, nh), Image.Resampling.LANCZOS)
            w, h = img.size
        pixels = img.tobytes()
    return b"VIMG" + struct.pack("<III", w, h, 1) + pixels


def load_media_dir(media_dir):
    """Pack converted VIMG/VIM/VIV/WAV/MEDIA.MF assets from make_media_assets.py."""
    assets = []
    if not media_dir or not os.path.isdir(media_dir):
        return assets

    for source_name in sorted(os.listdir(media_dir)):
        path = os.path.join(media_dir, source_name)
        if not os.path.isfile(path):
            continue
        base, ext = os.path.splitext(source_name)
        ext = ext.lstrip(".").upper()
        if ext not in ("VIMG", "VIM", "VIV", "WAV", "MF", "MP4"):
            continue
        fat_ext = ext[:3]
        fat_name = "".join(c for c in base.upper() if c.isalnum())[:8]
        if not fat_name:
            continue
        with open(path, "rb") as f:
            data = f.read()
        assets.append(
            {
                "source": path,
                "name11": short_name(fat_name, fat_ext),
                "data": data,
                "clusters": [],
            }
        )
    return assets


def load_root_assets(html_dir):
    """Top-level browser assets become read-only FAT32 root entries so the
    kernel browser can open them by their 8.3 short name (e.g. START.HTM,
    TEST.PNG)."""
    assets = []
    if not html_dir or not os.path.isdir(html_dir):
        return assets

    for source_name in sorted(os.listdir(html_dir)):
        path = os.path.join(html_dir, source_name)
        if not os.path.isfile(path):
            continue

        base, ext = os.path.splitext(source_name)
        ext = ext.lstrip(".").upper()
        if ext not in ("HTM", "HTML") + IMAGE_EXTS:
            continue

        fat_ext = ext[:3]
        fat_name = "".join(c for c in base.upper() if c.isalnum())[:8]
        if not fat_name:
            continue

        with open(path, "rb") as f:
            data = f.read()

        # Preserve the real filename as a VFAT long name when it does not survive
        # 8.3 (e.g. it has spaces, lowercase, or is longer than 8.3). This lets
        # Vibio browse and open the file by its real name, like a normal OS.
        short_disp = fat_name + ("." + fat_ext if fat_ext else "")
        long_name = source_name if source_name.upper() != short_disp else None

        assets.append(
            {
                "source": path,
                "name11": short_name(fat_name, fat_ext),
                "data": data,
                "clusters": [],
                "long_name": long_name,
            }
        )

        if ext in IMAGE_EXTS:
            vimg_data = make_vimg(path)
            vimg_name11 = vimg_name_for_base(base)
            if vimg_data is not None and vimg_name11 is not None:
                assets.append(
                    {
                        "source": path + " (converted VIMG)",
                        "name11": vimg_name11,
                        "data": vimg_data,
                        "clusters": [],
                    }
                )

    return assets


def load_sound_assets(sound_dir):
    assets = []
    if not sound_dir or not os.path.isdir(sound_dir):
        return assets

    for source_name, fat_name, fat_ext in SOUND_ASSETS:
        path = os.path.join(sound_dir, source_name)
        if not os.path.isfile(path):
            continue

        with open(path, "rb") as f:
            data = f.read()

        assets.append(
            {
                "source": path,
                "name11": short_name(fat_name, fat_ext),
                "data": data,
                "clusters": [],
            }
        )

    return assets


# ---------------------------------------------------------------------------
# Verbatim folder packing: copy an ENTIRE host directory tree onto the image
# unchanged (every file, any type, subfolders included), so whatever the user
# drops in the folder shows up in Vibio's Files app and is readable from the
# real storage device - no extension allow-list, no conversion. The kernel's
# read-only FAT32 reader already lists and reads everything it finds.
# ---------------------------------------------------------------------------

def _fat_clean(s, maxlen):
    """Keep only FAT-safe 8.3 characters, uppercased, up to maxlen."""
    allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-$%'@~`!(){}^#&"
    out = "".join(c for c in s.upper() if c in allowed)
    return out[:maxlen]


def _name11_display(name11):
    base = name11[:8].rstrip(b" ").decode("latin1")
    ext = name11[8:11].rstrip(b" ").decode("latin1")
    return base + ("." + ext if ext else "")


def make_short_name(name, used, is_dir):
    """Return (name11, long_name_or_None) for `name`, unique within `used`.
    Emits a VFAT long name whenever the real name does not survive as a clean
    uppercase 8.3 (lower-case, spaces, too long, or a ~N collision alias)."""
    if is_dir:
        base, ext = name, ""
    else:
        base, dot, ext = name.partition(".")
        if not dot:
            base, ext = name, ""
    cbase = _fat_clean(base, 8) or "_"
    cext = _fat_clean(ext, 3)
    cand = short_name(cbase, cext)
    perfect = (name == _name11_display(cand))  # already a clean uppercase 8.3
    if perfect and cand not in used:
        used.add(cand)
        return cand, None
    n = 1
    while True:
        suffix = "~" + str(n)
        b = (_fat_clean(base, 8 - len(suffix)) or "_") + suffix
        cand = short_name(b, cext)
        if cand not in used:
            used.add(cand)
            return cand, name  # keep the real name via LFN
        n += 1


def _dir_listing(host_dir):
    """List a host directory as [(name, path, is_dir)], dirs first then files,
    each sorted, skipping hidden/system entries that cannot be 8.3-encoded."""
    dirs, files = [], []
    try:
        names = sorted(os.listdir(host_dir))
    except OSError:
        return []
    for name in names:
        if name in (".", ".."):
            continue
        path = os.path.join(host_dir, name)
        if os.path.isdir(path):
            dirs.append((name, path, True))
        elif os.path.isfile(path):
            files.append((name, path, False))
    return dirs + files


def _entry_bytes_len(name11, long_name):
    return (len(lfn_entries(long_name, name11)) if long_name else 0) + 32


def tree_total_bytes(host_dir):
    """Approximate bytes the tree needs (file data only) for image sizing."""
    total = 0
    for _, path, is_dir in _dir_listing(host_dir):
        if is_dir:
            total += tree_total_bytes(path)
        else:
            try:
                total += os.path.getsize(path)
            except OSError:
                pass
    return total


def pack_tree(image, partition_offset, first_data_sector, spc, cluster_size,
              fat_entries, state, host_dir, this_cluster, parent_cluster, is_root):
    """Write the directory `host_dir` (already allocated at `this_cluster`) and
    everything under it into the image, allocating clusters via state['next'].
    Returns the directory's entry bytes (for the root, the caller appends these
    to the existing root entries; the cluster has already been sized by then for
    non-root dirs). `is_root` omits the . and .. entries."""
    listing = _dir_listing(host_dir)
    used = set()
    # Reserve the names the root already uses so verbatim files never collide.
    if is_root:
        for r in ("EFI", "SOUNDS", "KERNEL  BIN", "BOOTX64 EFI"):
            used.add(short_name(*r.split()) if " " in r else short_name(r))

    # Pre-compute each child's 8.3 name (needed both for sizing subdir clusters
    # and for building the entries).
    meta = []
    for name, path, is_dir in listing:
        name11, long_name = make_short_name(name, used, is_dir)
        meta.append((name, path, is_dir, name11, long_name))

    entries = bytearray()
    if not is_root:
        entries += dir_entry(short_name("."), 0x10, this_cluster)
        entries += dir_entry(short_name(".."), 0x10, parent_cluster)

    for name, path, is_dir, name11, long_name in meta:
        if is_dir:
            # Size the subdir's own cluster chain from its child count, allocate
            # it, then recurse to fill it (its '..' points back at this dir).
            sub_listing = _dir_listing(path)
            sub_used = set()
            sub_bytes = 64  # '.' and '..'
            for sname, spath, sis_dir in sub_listing:
                sn11, sln = make_short_name(sname, sub_used, sis_dir)
                sub_bytes += _entry_bytes_len(sn11, sln)
            sub_clusters = max(1, math.ceil(sub_bytes / cluster_size))
            sub_first = state["next"]
            sub_chain = list(range(sub_first, sub_first + sub_clusters))
            state["next"] += sub_clusters
            mark_cluster_chain(fat_entries, sub_chain)
            sub_entry_bytes = pack_tree(image, partition_offset, first_data_sector,
                                        spc, cluster_size, fat_entries, state,
                                        path, sub_first, this_cluster, False)
            write_tree_clusters(image, partition_offset, first_data_sector, spc,
                                sub_chain, sub_entry_bytes)
            if long_name:
                entries += lfn_entries(long_name, name11)
            entries += dir_entry(name11, 0x10, sub_first, 0)
        else:
            try:
                with open(path, "rb") as f:
                    data = f.read()
            except OSError:
                continue
            if len(data) == 0:
                first = 0
                chain = []
            else:
                first = state["next"]
                chain = allocate_cluster_chain(first, len(data), cluster_size)
                state["next"] = chain[-1] + 1
                mark_cluster_chain(fat_entries, chain)
                write_file_clusters(image, partition_offset, first_data_sector,
                                    spc, chain, data)
            if long_name:
                entries += lfn_entries(long_name, name11)
            entries += dir_entry(name11, 0x20, first, len(data))
    return bytes(entries)


def write_tree_clusters(image, partition_offset, first_data_sector, spc, chain, data):
    """Write directory-entry bytes across a (possibly multi-) cluster chain."""
    write_file_clusters(image, partition_offset, first_data_sector, spc, chain, data)


def calculate_fat_size(partition_sectors, sectors_per_cluster):
    fat_size = 1
    while True:
        data_sectors = partition_sectors - RESERVED_SECTORS - FAT_COUNT * fat_size
        cluster_count = data_sectors // sectors_per_cluster
        needed = math.ceil((cluster_count + 2) * 4 / SECTOR_SIZE)
        if needed == fat_size:
            return fat_size, cluster_count
        fat_size = needed


def build_image(efi_path, kernel_path, out_path, label, size_mib, sound_dir=None, html_dir=None, media_dir=None, files_dir=None):
    with open(efi_path, "rb") as f:
        efi_data = f.read()

    with open(kernel_path, "rb") as f:
        kernel_data = f.read()

    sound_assets = load_sound_assets(sound_dir)
    root_assets = load_root_assets(html_dir)
    media_assets = load_media_dir(media_dir)
    root_assets.extend(media_assets)

    have_files = bool(files_dir) and os.path.isdir(files_dir)

    # Auto-grow the image so the WHOLE verbatim folder fits (the user can drop
    # files of any size in there; the disk should just hold all of them).
    if have_files:
        needed = (tree_total_bytes(files_dir) + len(kernel_data) + len(efi_data) +
                  sum(len(a["data"]) for a in root_assets) +
                  sum(len(a["data"]) for a in sound_assets) + 8 * 1024 * 1024)
        min_mib = int((needed * 13) // (10 * 1024 * 1024)) + 16
        if size_mib < min_mib:
            size_mib = min_mib

    total_sectors = size_mib * 1024 * 1024 // SECTOR_SIZE
    partition_sectors = total_sectors - PARTITION_START_LBA
    sectors_per_cluster = 2
    fat_size, cluster_count = calculate_fat_size(partition_sectors, sectors_per_cluster)
    first_data_sector = RESERVED_SECTORS + FAT_COUNT * fat_size
    partition_offset = PARTITION_START_LBA * SECTOR_SIZE

    if cluster_count < 65525:
        raise ValueError("FAT32 partition is too small")

    image = bytearray(total_sectors * SECTOR_SIZE)

    # MBR with one EFI System Partition entry. This is a local VM disk only.
    mbr = bytearray(SECTOR_SIZE)
    mbr[446 + 4] = 0xEF
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
    boot[13] = sectors_per_cluster
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
    write_le32(boot, 67, 0x56494249)
    boot[71:82] = volume_label
    boot[82:90] = b"FAT32   "
    boot[510:512] = b"\x55\xAA"
    image[partition_offset : partition_offset + SECTOR_SIZE] = boot
    image[partition_offset + 6 * SECTOR_SIZE : partition_offset + 7 * SECTOR_SIZE] = boot

    fsinfo = bytearray(SECTOR_SIZE)
    write_le32(fsinfo, 0, 0x41615252)
    write_le32(fsinfo, 484, 0x61417272)
    write_le32(fsinfo, 488, 0xFFFFFFFF)
    write_le32(fsinfo, 492, 8)
    fsinfo[508:512] = b"\x00\x00\x55\xAA"
    image[partition_offset + SECTOR_SIZE : partition_offset + 2 * SECTOR_SIZE] = fsinfo
    image[partition_offset + 7 * SECTOR_SIZE : partition_offset + 8 * SECTOR_SIZE] = fsinfo

    cluster_size = sectors_per_cluster * SECTOR_SIZE

    # The FAT is created up front so the verbatim-folder packer can mark its
    # cluster chains directly as it writes files (it allocates from the same
    # running cluster counter).
    fat_entries = [0] * (cluster_count + 2)
    fat_entries[0] = 0x0FFFFF00 | MEDIA_DESCRIPTOR
    fat_entries[1] = 0xFFFFFFFF

    next_cluster = 3
    efi_dir_cluster = next_cluster
    next_cluster += 1
    boot_dir_cluster = next_cluster
    next_cluster += 1
    sounds_dir_cluster = 0
    if sound_assets:
        sounds_dir_cluster = next_cluster
        next_cluster += 1
    fat_entries[efi_dir_cluster] = END_OF_CHAIN
    fat_entries[boot_dir_cluster] = END_OF_CHAIN
    if sounds_dir_cluster != 0:
        fat_entries[sounds_dir_cluster] = END_OF_CHAIN

    boot_file_clusters = allocate_cluster_chain(next_cluster, len(efi_data), cluster_size)
    next_cluster = boot_file_clusters[-1] + 1
    kernel_file_clusters = allocate_cluster_chain(next_cluster, len(kernel_data), cluster_size)
    next_cluster = kernel_file_clusters[-1] + 1
    mark_cluster_chain(fat_entries, boot_file_clusters)
    mark_cluster_chain(fat_entries, kernel_file_clusters)
    for asset in sound_assets:
        asset["clusters"] = allocate_cluster_chain(next_cluster, len(asset["data"]), cluster_size)
        next_cluster = asset["clusters"][-1] + 1
        mark_cluster_chain(fat_entries, asset["clusters"])
    for asset in root_assets:
        asset["clusters"] = allocate_cluster_chain(next_cluster, len(asset["data"]), cluster_size)
        next_cluster = asset["clusters"][-1] + 1
        mark_cluster_chain(fat_entries, asset["clusters"])

    # Verbatim folder: copy the ENTIRE contents of files_dir onto the disk root
    # (every file, any type, subfolders and all) so the user just drops files in
    # and they all appear in Files - exactly like a real disk. This allocates and
    # writes the file/subdir clusters now and returns the root dir-entry bytes.
    extra_root_entries = b""
    if have_files:
        state = {"next": next_cluster}
        extra_root_entries = pack_tree(
            image, partition_offset, first_data_sector, sectors_per_cluster,
            cluster_size, fat_entries, state, files_dir, ROOT_CLUSTER, ROOT_CLUSTER, True)
        next_cluster = state["next"]

    # Build the root directory entries now that every cluster number is known.
    root = bytearray()
    root += dir_entry(short_name("EFI"), 0x10, efi_dir_cluster)
    root += dir_entry(volume_label, 0x08, 0)
    root += dir_entry(short_name("KERNEL", "BIN"), 0x20, kernel_file_clusters[0], len(kernel_data))
    if sounds_dir_cluster != 0:
        root += dir_entry(short_name("SOUNDS"), 0x10, sounds_dir_cluster)
    for asset in root_assets:
        root += file_dir_entries(asset, 0x20)
    root += extra_root_entries

    # FAT32's root directory is an ordinary cluster chain (not a fixed-size area
    # like FAT12/16), so it can span multiple clusters. The chain must start at
    # ROOT_CLUSTER (the BPB points there); any extra clusters come from the free
    # pool. Without this, more than ~32 root entries silently fell off the end.
    root_cluster_count = max(1, math.ceil(len(root) / cluster_size))
    root_clusters = [ROOT_CLUSTER] + list(range(next_cluster, next_cluster + root_cluster_count - 1))
    next_cluster += root_cluster_count - 1
    mark_cluster_chain(fat_entries, root_clusters)

    if next_cluster > cluster_count + 2:
        raise ValueError("FAT32 image too small for its contents (need a bigger --size-mib)")

    fat = bytearray(fat_size * SECTOR_SIZE)
    for idx, value in enumerate(fat_entries):
        write_le32(fat, idx * 4, value)

    fat1_offset = partition_offset + RESERVED_SECTORS * SECTOR_SIZE
    fat2_offset = fat1_offset + fat_size * SECTOR_SIZE
    image[fat1_offset : fat1_offset + len(fat)] = fat
    image[fat2_offset : fat2_offset + len(fat)] = fat

    write_file_clusters(image, partition_offset, first_data_sector, sectors_per_cluster, root_clusters, bytes(root))

    efi_dir = bytearray()
    efi_dir += dir_entry(short_name("."), 0x10, efi_dir_cluster)
    efi_dir += dir_entry(short_name(".."), 0x10, ROOT_CLUSTER)
    efi_dir += dir_entry(short_name("BOOT"), 0x10, boot_dir_cluster)
    write_cluster(image, partition_offset, first_data_sector, sectors_per_cluster, efi_dir_cluster, efi_dir)

    boot_dir = bytearray()
    boot_dir += dir_entry(short_name("."), 0x10, boot_dir_cluster)
    boot_dir += dir_entry(short_name(".."), 0x10, efi_dir_cluster)
    boot_dir += dir_entry(short_name("BOOTX64", "EFI"), 0x20, boot_file_clusters[0], len(efi_data))
    write_cluster(image, partition_offset, first_data_sector, sectors_per_cluster, boot_dir_cluster, boot_dir)

    if sounds_dir_cluster != 0:
        sounds_dir = bytearray()
        sounds_dir += dir_entry(short_name("."), 0x10, sounds_dir_cluster)
        sounds_dir += dir_entry(short_name(".."), 0x10, ROOT_CLUSTER)
        for asset in sound_assets:
            sounds_dir += dir_entry(asset["name11"], 0x20, asset["clusters"][0], len(asset["data"]))
        write_cluster(image, partition_offset, first_data_sector, sectors_per_cluster, sounds_dir_cluster, sounds_dir)

    write_file_clusters(image, partition_offset, first_data_sector, sectors_per_cluster, boot_file_clusters, efi_data)
    write_file_clusters(image, partition_offset, first_data_sector, sectors_per_cluster, kernel_file_clusters, kernel_data)
    for asset in sound_assets:
        write_file_clusters(
            image,
            partition_offset,
            first_data_sector,
            sectors_per_cluster,
            asset["clusters"],
            asset["data"])
    for asset in root_assets:
        write_file_clusters(
            image,
            partition_offset,
            first_data_sector,
            sectors_per_cluster,
            asset["clusters"],
            asset["data"])

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(image)


def main():
    parser = argparse.ArgumentParser(description="Create a VM-only FAT32 UEFI disk image.")
    parser.add_argument("--efi", required=True, help="Path to BOOTX64.EFI")
    parser.add_argument("--kernel", required=True, help="Path to KERNEL.BIN")
    parser.add_argument("--out", required=True, help="Output raw disk image")
    parser.add_argument("--label", default="VIBIO OS", help="FAT volume label")
    parser.add_argument("--size-mib", type=int, default=128, help="Disk size in MiB")
    parser.add_argument("--sounds", help="Optional directory containing Vibio WAV assets")
    parser.add_argument("--html", help="Optional directory containing top-level browser assets (*.htm/*.html/*.png)")
    parser.add_argument("--media", help="Optional directory from tools/make_media_assets.py (VIMG/VIV/MEDIA.MF)")
    parser.add_argument("--files", help="Optional folder whose ENTIRE contents (any type, subfolders included) are copied verbatim onto the disk root")
    args = parser.parse_args()

    build_image(args.efi, args.kernel, args.out, args.label, args.size_mib, args.sounds, args.html, args.media, args.files)


if __name__ == "__main__":
    main()
