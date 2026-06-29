#ifndef VIBIO_BOOT_INFO_H
#define VIBIO_BOOT_INFO_H

typedef unsigned char vibio_u8;
typedef unsigned short vibio_u16;
typedef unsigned int vibio_u32;
typedef unsigned long long vibio_u64;

#define VIBIO_BOOT_INFO_MAGIC 0x564942494F424F4FULL
#define VIBIO_KERNEL_LOAD_ADDRESS 0x100000ULL
/* The boot stage loads at most this many bytes of KERNEL.BIN. It must exceed the
 * actual kernel size or the tail (rodata: icon + wallpaper pixel arrays) loads
 * as zeros and renders black. KERNEL.BIN is ~3.7 MB once the imported G:\
 * wallpapers are embedded, so this is sized to 16 MB for comfortable headroom. */
#define VIBIO_KERNEL_MAX_SIZE (16ULL * 1024ULL * 1024ULL)
#define VIBIO_MEMORY_MAP_MAX_SIZE (64ULL * 1024ULL)
#define VIBIO_MEMORY_TYPE_CONVENTIONAL 7
#define VIBIO_BOOT_FILE_MAX 256
#define VIBIO_BOOT_FILE_NAME_MAX 64
#define VIBIO_BOOT_FILE_TYPE_SOUND 1
#define VIBIO_BOOT_FILE_TYPE_HTML 2
#define VIBIO_BOOT_FILE_TYPE_IMAGE 3
#define VIBIO_BOOT_FILE_TYPE_DIRECTORY 4
#define VIBIO_BOOT_FILE_TYPE_VIDEO 5
/* A file that was enumerated on the boot volume but NOT loaded into memory: an
 * unknown/unsupported extension (e.g. .mp4, .txt) or a file too large to
 * preload. It still appears in the Files listing with its real size (address is
 * 0); opening it shows an honest "cannot open" message instead of hiding it. */
#define VIBIO_BOOT_FILE_TYPE_OTHER 6
/* Upper safety limit for a single UEFI-preloaded file. The loader now allocates
 * each file at its exact size (see load_read_only_file), so this no longer costs
 * memory per file - it only caps how large a single preloaded media file may be.
 * Raised to 32 MiB so videos and full-resolution images can preload on machines
 * with no post-boot storage driver (e.g. the real laptop booting from USB). */
#define VIBIO_BOOT_FILE_MAX_SIZE (32ULL * 1024ULL * 1024ULL)

typedef struct {
    vibio_u32 type;
    vibio_u32 padding;
    vibio_u64 physical_start;
    vibio_u64 virtual_start;
    vibio_u64 number_of_pages;
    vibio_u64 attribute;
} VibioMemoryDescriptor;

typedef struct {
    vibio_u64 address;
    vibio_u64 size;
    vibio_u32 type;
    vibio_u32 name_length;
    char name[VIBIO_BOOT_FILE_NAME_MAX];
} VibioBootFile;

typedef struct {
    vibio_u64 magic;
    vibio_u64 framebuffer_base;
    vibio_u64 framebuffer_size;
    vibio_u32 framebuffer_width;
    vibio_u32 framebuffer_height;
    vibio_u32 pixels_per_scan_line;
    vibio_u32 pixel_format;
    vibio_u64 kernel_load_address;
    vibio_u64 kernel_size;
    vibio_u64 memory_map_base;
    vibio_u64 memory_map_size;
    vibio_u64 memory_map_entry_count;
    vibio_u64 memory_map_descriptor_size;
    vibio_u32 memory_map_descriptor_version;
    vibio_u32 boot_services_exited;
    vibio_u64 boot_file_count;
    VibioBootFile boot_files[VIBIO_BOOT_FILE_MAX];
    vibio_u64 acpi_rsdp;
} VibioBootInfo;

#endif
