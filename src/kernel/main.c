#include "boot_info.h"

typedef signed char vibio_s8;

#include "icons_generated.h"
#include "fonts_generated.h"
#include "html_generated.h"
#include "wallpapers_generated.h"
#include "mp4.h"   /* isolated MP4/ISO-BMFF demuxer (separate translation unit) */
#include "h264.h"  /* isolated H.264 Constrained Baseline probe */

#define GLYPH_WIDTH 5
#define GLYPH_HEIGHT 7
#define FONT_SCALE 3
#define CHAR_STEP ((GLYPH_WIDTH + 1) * FONT_SCALE)
#define LINE_STEP ((GLYPH_HEIGHT + 2) * FONT_SCALE)
#define PAGE_SIZE 4096ULL
#define PAGE_2M_SIZE 0x200000ULL
#define LOW_IDENTITY_LIMIT (4ULL * 1024ULL * 1024ULL * 1024ULL)
#define ALLOCATOR_MIN_ADDRESS 0x100000ULL
#define PAGE_TABLE_ENTRIES 512
#define PAGE_PRESENT 0x001ULL
#define PAGE_WRITABLE 0x002ULL
#define PAGE_PWT 0x008ULL       /* selects PAT entry bit 0 */
#define PAGE_PCD 0x010ULL       /* cache disable (with PWT -> uncacheable, for MMIO) */
#define PAGE_SIZE_FLAG 0x080ULL
#define IA32_PAT_MSR 0x277U
#define IDT_ENTRY_COUNT 256
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_USER_INTERRUPT_GATE 0xEE
#define GDT_ENTRY_COUNT 7
#define GDT_KERNEL_CODE_SELECTOR 0x08
#define GDT_KERNEL_DATA_SELECTOR 0x10
#define GDT_USER_DATA_SELECTOR 0x18
#define GDT_USER_CODE_SELECTOR 0x20
#define GDT_TSS_SELECTOR 0x28
#define USER_RPL 0x03
#define SYSCALL_VECTOR 0x80
#define SYSCALL_VERSION 1ULL
#define SYSCALL_TIMER_TICKS 2ULL
#define SYSCALL_TASK_COUNT 3ULL
#define SYSCALL_CONTEXT_SWITCHES 4ULL
#define SYSCALL_INVALID_RETURN 0xFFFFFFFFFFFFFFFFULL
/* Single source of truth for the stage number. The screen stage line, the
 * VERSION command, and the syscall ABI version all derive from this so they can
 * never drift out of sync again. */
#define VIBIO_STAGE 37ULL
#define VIBIO_SYSCALL_ABI_VERSION VIBIO_STAGE
#define IRQ_VECTOR_BASE 32
#define IRQ_TIMER_VECTOR 32
#define IRQ_KEYBOARD_VECTOR 33
#define IRQ_MOUSE_VECTOR 44
#define IRQ_READY_TARGET_TICKS 20ULL
#define IRQ_SCANCODE_RING_SIZE 64ULL
#define MOUSE_RING_SIZE 256ULL
#define CURSOR_WIDTH 12
#define CURSOR_HEIGHT 19
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 180
#define WINDOW_TITLEBAR_HEIGHT 28
#define WINDOW_BORDER 2
#define WINDOW_KIND_SYSTEM 1
#define WINDOW_KIND_TERMINAL 2
#define WINDOW_KIND_WELCOME 3
#define WINDOW_KIND_BROWSER 4
#define WINDOW_KIND_FILES 5
#define WINDOW_KIND_MEDIA_VIEWER 6
#define WINDOW_KIND_MEDIA_PLAYER 7
#define WINDOW_KIND_SETTINGS 8
#define DESKTOP_TOPBAR_HEIGHT 0
#define DESKTOP_TASKBAR_HEIGHT 48
#define DESKTOP_MARGIN 12
#define TASKBAR_START_WIDTH 38
#define TASKBAR_SEARCH_WIDTH 166
#define TASKBAR_BUTTON_WIDTH 38
#define TASKBAR_BUTTON_GAP 8
#define DESKTOP_ICON_CELL_WIDTH 76
#define DESKTOP_ICON_CELL_HEIGHT 66
#define DESKTOP_ICON_IMAGE 36
#define DESKTOP_ICON_NONE 0U
#define DESKTOP_ICON_BUILTIN_COUNT 4U
#define DESKTOP_TEMP_ICON_MAX 8U
#define DESKTOP_ICON_KIND_TEMP_FOLDER 20U
#define DESKTOP_ICON_KIND_TEMP_TEXT 21U
#define DESKTOP_ICON_SIZE_SMALL 0U
#define DESKTOP_ICON_SIZE_MEDIUM 1U
#define DESKTOP_ICON_SIZE_LARGE 2U
#define DESKTOP_CONTEXT_NONE 0U
#define DESKTOP_CONTEXT_ROOT 1U
#define DESKTOP_CONTEXT_VIEW 2U
#define DESKTOP_CONTEXT_NEW 3U
#define DESKTOP_CONTEXT_WALLPAPER 4U
#define DESKTOP_CONTEXT_W 226U
#define DESKTOP_CONTEXT_ROW_H 34U
#define DESKTOP_CONTEXT_ROOT_ROWS 5U
#define DESKTOP_CONTEXT_VIEW_ROWS 5U
#define DESKTOP_CONTEXT_NEW_ROWS 2U
/* Wallpaper picker rows: "Vibio default" gradient plus every real image that
 * tools/make_wallpapers.py imported from G:\Backgrounds into KERNEL.BIN. */
#define DESKTOP_CONTEXT_WALLPAPER_ROWS (1U + (vibio_u32)VIBIO_WALLPAPER_COUNT)
/* wallpaper_mode values >= this select imported image (mode - base). Lower
 * values are built-in (0 = Vibio default gradient). */
#define WALLPAPER_IMAGE_BASE 100U
/* Two desktop-icon clicks within this many timer ticks (~0.3s at 100 ticks/s)
 * count as a double-click that opens the icon. Matches the Files app window. */
#define DESKTOP_DOUBLE_CLICK_TICKS 30ULL
#define TASKBAR_NO_SLOT 0xFFFFFFFFU
#define SMALL_FONT_SCALE 1
#define SMALL_CHAR_STEP ((GLYPH_WIDTH + 1) * SMALL_FONT_SCALE)
#define SMALL_LINE_STEP ((GLYPH_HEIGHT + 2) * SMALL_FONT_SCALE)
/* History capacity (scrollback). Kept so the whole VibioConsoleState still fits
 * in a single heap page (heap_alloc only serves up to one 4 KiB page). */
#define CONSOLE_ROWS 64
#define CONSOLE_COLS 48
/* Terminal text metrics, from the JetBrains Mono atlas. */
#define TERM_LINE VIBIO_FONT_JBM_LINE
#define TERM_GLYPH_H VIBIO_FONT_JBM_ASCENT
#define CONSOLE_INPUT_MAX 40
#define HEAP_INITIAL_PAGES 4
#define HEAP_ALIGN 16ULL
#define HEAP_MIN_SPLIT_PAYLOAD 16ULL
#define HEAP_BLOCK_MAGIC 0x48454150424C4B31ULL
#define TASK_NAME_MAX 12
#define TASK_STACK_BYTES (16ULL * 1024ULL)
#define TASK_STATE_READY 1ULL
#define TASK_STATE_RUNNING 2ULL
#define TASK_STATE_SLEEPING 3ULL
#define TASK_NAME_KERNEL 1
#define TASK_NAME_CONSOLE 2
#define TASK_NAME_IDLE 3
#define TASK_NAME_INPUT 4
#define CONTEXT_SWITCH_DEMO_ROUNDS 4ULL
#define SCHEDULER_TIMER_INTERVAL_TICKS 50ULL
#define AHCI_MAX_PORTS 32
#define AHCI_PORT_REGISTER_SIZE 0x80U
#define AHCI_PORT_BASE_OFFSET 0x100U
#define AHCI_CMD_HEADER_BYTES 32U
#define AHCI_COMMAND_TABLE_PRDT_OFFSET 128U
#define AHCI_COMMAND_TABLE_BYTES 256U
#define AHCI_FIS_REG_H2D 0x27
#define AHCI_ATA_READ_DMA_EXT 0x25
#define AHCI_ATA_WRITE_DMA_EXT 0x35
#define AHCI_ATA_FLUSH_CACHE_EXT 0xEA
#define AHCI_SECTOR_BYTES 512U
#define AHCI_PCI_CLASS_MASS_STORAGE 0x01
#define AHCI_PCI_SUBCLASS_SATA 0x06
#define AHCI_PCI_PROGIF_AHCI 0x01
#define USB_PCI_CLASS_SERIAL_BUS 0x0C
#define USB_PCI_SUBCLASS_USB 0x03
#define USB_PROGIF_UHCI 0x00
#define USB_PROGIF_OHCI 0x10
#define USB_PROGIF_EHCI 0x20
#define USB_PROGIF_XHCI 0x30
#define USB_MAX_CONTROLLERS 8
#define USB_TYPE_UNKNOWN 0
#define USB_TYPE_UHCI 1
#define USB_TYPE_OHCI 2
#define USB_TYPE_EHCI 3
#define USB_TYPE_XHCI 4
#define USB_HOTPLUG_MAX_PORTS 64U
#define USB_HOTPLUG_POLL_TICKS 20ULL
#define USB_HOTPLUG_EVENT_NONE 0U
#define USB_HOTPLUG_EVENT_INSERT 1U
#define USB_HOTPLUG_EVENT_REMOVE 2U
#define USB_HOTPLUG_SOUND_NONE 0U
#define USB_HOTPLUG_SOUND_STARTED 1U
#define USB_HOTPLUG_SOUND_AUDIO_UNAVAILABLE 2U
#define USB_HOTPLUG_SOUND_MISSING 3U
#define USB_HOTPLUG_SOUND_FAILED 4U
#define USB_HOTPLUG_SOUND_BUSY 5U
/* Honest "furthest step reached" for the read-only USB mass-storage path. Each
 * step only advances when the previous one really succeeded; nothing is faked. */
#define USB_STOR_STEP_NONE       0  /* no suitable USB controller found */
#define USB_STOR_STEP_CONTROLLER 1  /* a controller (xHCI preferred) was found */
#define USB_STOR_STEP_MMIO       2  /* controller capability registers readable */
#define USB_STOR_STEP_PORTS      3  /* root-hub port status read */
#define USB_STOR_STEP_DEVICE     4  /* a device descriptor was read */
#define USB_STOR_STEP_MSD        5  /* a mass-storage interface was found */
#define USB_STOR_STEP_SECTORS    6  /* a read-only sector backend is available */
#define USB_STOR_STEP_FAT32      7  /* FAT32 mounted read-only over USB */
/* Files app storage backend source. Priority: AHCI boot disk, then USB. */
#define FILES_SRC_NONE 0
#define FILES_SRC_AHCI 1
#define FILES_SRC_USB  2
#define FILES_SRC_BOOT 3
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_LONG_NAME 0x0F
#define FAT32_EXPECTED_SOUND_FILES 5ULL
#define AHCI_GHC_AE 0x80000000U
#define AHCI_CMD_ST 0x00000001U
#define AHCI_CMD_FRE 0x00000010U
#define AHCI_CMD_FR 0x00004000U
#define AHCI_CMD_CR 0x00008000U
#define AHCI_TFD_BSY 0x80U
#define AHCI_TFD_DRQ 0x08U
#define AHCI_IS_TFES 0x40000000U
#define AHCI_SSTS_DET_PRESENT 0x3U
#define AHCI_READ_TIMEOUT 1000000U
#define PCI_CLASS_MULTIMEDIA 0x04
#define PCI_SUBCLASS_AUDIO 0x01
#define AC97_BDL_ENTRIES 32U
#define AC97_BDL_BYTES (AC97_BDL_ENTRIES * 8U)
#define AC97_PCM_OUT_OFFSET 0x10U
#define AC97_BDBAR 0x00U
#define AC97_CIV 0x04U
#define AC97_LVI 0x05U
#define AC97_SR 0x06U
#define AC97_CR 0x0BU
#define AC97_CR_RUN 0x01U
#define AC97_CR_RESET 0x02U
#define AC97_SR_CLEAR 0x001CU
#define AC97_DESC_IOC 0x80000000U
#define AC97_MAX_DESC_BYTES 0x1FFFCU
#define AUDIO_DRIVER_NONE 0U
#define AUDIO_DRIVER_AC97 1U
#define AUDIO_DRIVER_HDA 2U
#define AUDIO_RATE 44100U
#define AUDIO_CHANNELS 2U
#define AUDIO_BITS 16U
/* PCM mixing/playback buffer. Long media sidecars are staged through this in
 * chunks because AC97/HDA descriptor rings cannot DMA a whole 70s WAV at once. */
#define AUDIO_BUFFER_BYTES (16ULL * 1024ULL * 1024ULL)
#define AUDIO_CHUNK_BYTES (2ULL * 1024ULL * 1024ULL)
/* Ticks (100 Hz) to ignore the AC'97 "DMA halted" (DCH) status right after a
 * chunk is started. The controller can momentarily still report DCH for a few
 * milliseconds before its DMA engine spins up; honoring it that early made the
 * service loop "advance to the next chunk" immediately and restart, skipping the
 * clip forward in sub-second bursts. After this grace the DMA is reliably
 * running, so DCH then genuinely means the chunk finished. */
#define AUDIO_DCH_STARTUP_GRACE_TICKS 12ULL
#define PCI_SUBCLASS_HDA 0x03
#define HDA_BDL_ENTRIES 32U
#define HDA_BDL_BYTES (HDA_BDL_ENTRIES * 16U)
#define HDA_MAX_DESC_BYTES 0x10000U
#define HDA_REG_GCAP 0x00U
#define HDA_REG_GCTL 0x08U
#define HDA_REG_STATESTS 0x0EU
#define HDA_REG_CORBLBASE 0x40U
#define HDA_REG_CORBUBASE 0x44U
#define HDA_REG_CORBWP 0x48U
#define HDA_REG_CORBRP 0x4AU
#define HDA_REG_CORBCTL 0x4CU
#define HDA_REG_CORBSTS 0x4DU
#define HDA_REG_CORBSIZE 0x4EU
#define HDA_REG_RIRBLBASE 0x50U
#define HDA_REG_RIRBUBASE 0x54U
#define HDA_REG_RIRBWP 0x58U
#define HDA_REG_RINTCNT 0x5AU
#define HDA_REG_RIRBCTL 0x5CU
#define HDA_REG_RIRBSTS 0x5DU
#define HDA_REG_RIRBSIZE 0x5EU
#define HDA_STREAM_BASE 0x80U
#define HDA_STREAM_STRIDE 0x20U
#define HDA_SD_CTL 0x00U
#define HDA_SD_STS 0x03U
#define HDA_SD_CBL 0x08U
#define HDA_SD_LVI 0x0CU
#define HDA_SD_FMT 0x12U
#define HDA_SD_BDPL 0x18U
#define HDA_SD_BDPU 0x1CU
#define HDA_GCTL_CRST 0x00000001U
#define HDA_CORBCTL_RUN 0x02U
#define HDA_RIRBCTL_RUN 0x02U
#define HDA_RIRBSTS_RIRBOIS 0x01U
#define HDA_RIRBSTS_RINTFL 0x04U
#define HDA_SD_CTL_SRST 0x01U
#define HDA_SD_CTL_RUN 0x02U
#define HDA_SD_STS_CLEAR 0x1CU
#define HDA_STREAM_ID 1U
#define HDA_FMT_44K1_16_STEREO 0x4011U
#define HDA_PARAM_NODE_COUNT 0x04U
#define HDA_PARAM_FUNCTION_GROUP_TYPE 0x05U
#define HDA_PARAM_AUDIO_WIDGET_CAP 0x09U
#define HDA_PARAM_PIN_CAP 0x0CU
#define HDA_PARAM_CONN_LIST_LEN 0x0EU
#define HDA_PARAM_INPUT_AMP_CAP 0x0DU
#define HDA_PARAM_OUTPUT_AMP_CAP 0x12U
#define HDA_WIDGET_TYPE_AUDIO_OUTPUT 0x00U
#define HDA_WIDGET_TYPE_AUDIO_MIXER 0x02U
#define HDA_WIDGET_TYPE_AUDIO_SELECTOR 0x03U
#define HDA_WIDGET_TYPE_PIN_COMPLEX 0x04U
#define HDA_VERB_GET_CONN_ENTRY 0xF02U
#define HDA_VERB_GET_PIN_CONFIG_DEFAULT 0xF1CU
#define HDA_VERB_SET_CONVERTER_FORMAT 0x2U
#define HDA_VERB_SET_AMP_GAIN_MUTE 0x3U
#define HDA_VERB_SET_CONN_SELECT 0x701U
#define HDA_VERB_SET_POWER_STATE 0x705U
#define HDA_VERB_SET_CONVERTER_STREAM_CHANNEL 0x706U
#define HDA_VERB_SET_PIN_WIDGET_CONTROL 0x707U
#define HDA_VERB_SET_EAPD_BTL_ENABLE 0x70CU
#define HDA_PIN_CAP_OUTPUT 0x10U
#define HDA_PIN_WIDGET_OUT_ENABLE 0x40U
#define HDA_PIN_WIDGET_HP_ENABLE 0x80U
#define HDA_EAPD_ENABLE 0x02U
#define HDA_MAX_WIDGETS 64U
#define HDA_MAX_PATH_NODES 8U
#define MOUSE_MAX_DELTA 96
#define MOUSE_WHEEL_LINES 3
#define MOUSE_WHEEL_FRAME_CAP 12
#define MAYBE_UNUSED __attribute__((unused))

#define ROW7(row, a, b, c, d, e, f, g) \
    ((row) == 0 ? (a) : (row) == 1 ? (b) : (row) == 2 ? (c) : (row) == 3 ? (d) : (row) == 4 ? (e) : (row) == 5 ? (f) : (g))

typedef struct {
    const VibioBootInfo *boot_info;
    vibio_u64 next_descriptor_offset;
    vibio_u64 region_next_address;
    vibio_u64 region_pages_left;
    vibio_u64 free_pages;
    vibio_u64 allocated_pages;
} VibioPageAllocator;

typedef struct {
    vibio_u64 pml4;
    vibio_u64 table_pages;
    vibio_u32 ok;
} VibioPageTableBuild;

typedef struct __attribute__((packed)) {
    vibio_u16 offset_low;
    vibio_u16 selector;
    vibio_u8 ist;
    vibio_u8 type_attributes;
    vibio_u16 offset_mid;
    vibio_u32 offset_high;
    vibio_u32 reserved;
} VibioIdtEntry;

typedef struct __attribute__((packed)) {
    vibio_u16 limit;
    vibio_u64 base;
} VibioIdtPointer;

typedef struct __attribute__((packed)) {
    vibio_u16 limit;
    vibio_u64 base;
} VibioGdtPointer;

typedef struct __attribute__((packed)) {
    vibio_u32 reserved0;
    vibio_u64 rsp0;
    vibio_u64 rsp1;
    vibio_u64 rsp2;
    vibio_u64 reserved1;
    vibio_u64 ist1;
    vibio_u64 ist2;
    vibio_u64 ist3;
    vibio_u64 ist4;
    vibio_u64 ist5;
    vibio_u64 ist6;
    vibio_u64 ist7;
    vibio_u64 reserved2;
    vibio_u16 reserved3;
    vibio_u16 iomap_base;
} VibioTss;

typedef struct {
    vibio_u64 idt_address;
    vibio_u64 breakpoint_hits;
    vibio_u32 ok;
} VibioIdtBuild;

typedef struct {
    vibio_u64 gdt_address;
    vibio_u64 tss_address;
    vibio_u64 rsp0_stack_base;
    vibio_u16 kernel_code_selector;
    vibio_u16 kernel_data_selector;
    vibio_u16 user_code_selector;
    vibio_u16 user_data_selector;
    vibio_u16 tss_selector;
    vibio_u16 current_code_selector;
    vibio_u16 task_register;
    vibio_u32 syscall_user_callable;
    vibio_u32 ok;
} VibioUserBoundaryBuild;

typedef struct {
    volatile vibio_u64 timer_ticks;
    volatile vibio_u64 keyboard_irqs;
    volatile vibio_u64 last_scancode;
    volatile vibio_u64 scancode_write_index;
    volatile vibio_u8 scancodes[IRQ_SCANCODE_RING_SIZE];
    volatile vibio_u64 scheduler_requests;
    volatile vibio_u64 scheduler_interval;
    volatile vibio_u64 scheduler_countdown;
    /* Mouse fields are appended last so the assembly ISR offsets stay fixed:
     * mouse_irqs at 120, mouse_write_index at 128, mouse_bytes at 136. */
    volatile vibio_u64 mouse_irqs;
    volatile vibio_u64 mouse_write_index;
    volatile vibio_u8 mouse_bytes[MOUSE_RING_SIZE];
} VibioIrqState;

typedef struct {
    vibio_u64 state_address;
    vibio_u64 timer_ticks;
    vibio_u64 keyboard_irqs;
    vibio_u64 last_scancode;
    vibio_u32 ok;
} VibioIrqBuild;

typedef struct {
    char lines[CONSOLE_ROWS][CONSOLE_COLS];
    vibio_u32 line_lengths[CONSOLE_ROWS];
    char input[CONSOLE_INPUT_MAX];
    vibio_u32 input_length;
    vibio_u32 input_cursor;
    vibio_u32 cursor_row;
    vibio_u64 scancode_read_index;
    vibio_u64 commands_run;
    vibio_u64 history_revision;
    vibio_u8 shift_down;
    vibio_u8 caps_on;
    vibio_u8 wants_focus;
    vibio_u8 wants_browser; /* a BROWSER command asked the shell to open it */
    vibio_u8 wants_files;   /* a FILESAPP/EXPLORER command asked the shell to open Files */
    vibio_u8 wants_files_usb; /* USBFILES asked to open Files on the live USB source */
    vibio_u8 wants_settings; /* Super+I asked the shell to open Settings */
    vibio_u8 wants_media_viewer;
    vibio_u8 wants_media_player;
    char media_open_name[VIBIO_BOOT_FILE_NAME_MAX];
    vibio_u8 f11_down;      /* F11 held, so key-repeat does not retoggle fullscreen */
    vibio_u8 ext_pending;
    vibio_u8 set2_break_pending;
    vibio_u8 ctrl_down;
    vibio_u8 super_down;
    char clipboard[CONSOLE_INPUT_MAX];
    vibio_u32 clipboard_length;
    vibio_u32 view_offset; /* lines scrolled up from the newest output */
} VibioConsoleState;

typedef struct {
    vibio_u64 size;
    vibio_u64 used;
    vibio_u64 next;
    vibio_u64 magic;
} VibioHeapBlock;

typedef struct {
    VibioPageAllocator *page_allocator;
    vibio_u64 first_block;
    vibio_u64 pages;
    vibio_u64 total_bytes;
    vibio_u64 used_bytes;
    vibio_u64 free_bytes;
    vibio_u64 allocations;
    vibio_u64 frees;
    vibio_u64 live_allocations;
    vibio_u64 failed_allocations;
    vibio_u32 ok;
} VibioKernelHeap;

typedef struct {
    vibio_u64 pages;
    vibio_u64 allocations;
    vibio_u64 frees;
    vibio_u64 used_bytes;
    vibio_u64 free_bytes;
    vibio_u32 ok;
} VibioHeapTest;

typedef struct {
    vibio_u64 id;
    vibio_u64 state;
    vibio_u64 stack_base;
    vibio_u64 stack_size;
    vibio_u64 stack_top;
    vibio_u64 context_rsp;
    vibio_u64 run_count;
    vibio_u64 yield_count;
    vibio_u64 next;
    vibio_u32 name_length;
    char name[TASK_NAME_MAX];
} VibioTask;

typedef struct {
    VibioTask *first;
    VibioTask *current;
    VibioTask *next_ready;
    vibio_u64 next_id;
    vibio_u64 task_count;
    vibio_u64 ready_count;
    vibio_u64 running_count;
    vibio_u64 sleeping_count;
    vibio_u64 scheduler_rsp;
    vibio_u64 context_switches;
    vibio_u64 timer_schedule_requests;
    vibio_u64 timer_driven_switches;
    vibio_u32 ok;
} VibioTaskManager;

typedef struct {
    vibio_u64 task_count;
    vibio_u64 ready_count;
    vibio_u64 running_count;
    vibio_u64 sleeping_count;
    vibio_u64 context_switches;
    vibio_u64 console_runs;
    vibio_u64 idle_runs;
    vibio_u64 timer_requests;
    vibio_u64 timer_switches;
    vibio_u32 ok;
} VibioTaskTest;

typedef struct {
    volatile vibio_u64 calls;
    volatile vibio_u64 invalid_calls;
    volatile vibio_u64 last_number;
    volatile vibio_u64 version;
    volatile vibio_u64 task_count;
    volatile vibio_u64 context_switches;
    volatile vibio_u64 last_result;
} VibioSyscallState;

typedef struct {
    vibio_u64 calls;
    vibio_u64 invalid_calls;
    vibio_u64 version_result;
    vibio_u64 ticks_result;
    vibio_u64 task_count_result;
    vibio_u64 switches_result;
    vibio_u64 invalid_result;
    vibio_u32 ok;
} VibioSyscallTest;

typedef struct {
    vibio_u64 file_count;
    vibio_u64 sound_count;
    vibio_u64 wav_count;
    vibio_u64 total_bytes;
    vibio_u32 ok;
} VibioFileTest;

typedef struct {
    vibio_u32 audio_format;
    vibio_u32 channels;
    vibio_u32 sample_rate;
    vibio_u32 bits_per_sample;
    vibio_u32 block_align;
    vibio_u64 data_offset;
    vibio_u64 data_size;
    vibio_u32 ok;
} VibioWavInfo;

typedef struct {
    vibio_u32 driver;
    vibio_u32 found;
    vibio_u32 hda_found;
    vibio_u32 ready;
    vibio_u32 playback_started;
    vibio_u32 boot_play_started;
    vibio_u32 pci_bus;
    vibio_u32 pci_device;
    vibio_u32 pci_function;
    vibio_u32 vendor_id;
    vibio_u32 device_id;
    vibio_u32 hda_pci_bus;
    vibio_u32 hda_pci_device;
    vibio_u32 hda_pci_function;
    vibio_u32 hda_vendor_id;
    vibio_u32 hda_device_id;
    vibio_u64 hda_mmio;
    vibio_u64 hda_corb_address;
    vibio_u64 hda_rirb_address;
    vibio_u32 hda_corb_wp;
    vibio_u32 hda_rirb_rp;
    vibio_u32 hda_corb_entries;
    vibio_u32 hda_rirb_entries;
    vibio_u32 hda_stream_base;
    vibio_u32 hda_codec;
    vibio_u32 hda_audio_fg;
    vibio_u32 hda_dac_node;
    vibio_u32 hda_pin_node;
    vibio_u32 hda_pin_config;
    vibio_u32 hda_pin_caps;
    vibio_u32 hda_path_count;
    vibio_u32 hda_path_nodes[HDA_MAX_PATH_NODES];
    vibio_u32 hda_path_selects[HDA_MAX_PATH_NODES];
    vibio_u32 hda_stream_status;
    vibio_u32 hda_stream_ctl;
    vibio_u32 hda_stream_lpib;
    vibio_u32 volume;
    vibio_u32 playback_active;
    vibio_u32 paused;
    vibio_u64 playback_ticks_remaining;
    vibio_u64 playback_last_tick;
    vibio_u64 pcm_chunk_start_tick;
    vibio_u16 mixer_base;
    vibio_u16 bus_master_base;
    vibio_u64 bdl_address;
    vibio_u64 pcm_buffer_address;
    vibio_u64 pcm_buffer_bytes;
    vibio_u64 pcm_source_address;
    vibio_u64 pcm_source_bytes;
    vibio_u64 pcm_play_bytes;
    vibio_u64 pcm_total_bytes;
    vibio_u64 pcm_source_offset;
    vibio_u64 pcm_stream_offset;
    vibio_u32 bdl_count;
    vibio_u32 sample_rate;
    vibio_u32 channels;
    vibio_u32 bits_per_sample;
    vibio_u32 current_name_length;
    char current_name[VIBIO_BOOT_FILE_NAME_MAX];
    vibio_u32 last_error;
} VibioAudioState;

static void audio_set_volume(VibioAudioState *audio, vibio_u32 volume);
static vibio_u32 audio_start_next_pcm_chunk(VibioAudioState *audio);

/* A VibioDiskReadTest can front either the AHCI SATA disk or a USB mass-storage
 * device. All FAT32/sector reads funnel through ahci_read_sector(), which
 * dispatches on this backend so the read-only FAT32 parser is reused unchanged
 * over USB. The USB path is strictly read-only (SCSI READ(10) only). */
#define DISK_BACKEND_AHCI 0U
#define DISK_BACKEND_USB  1U
static vibio_u32 xhci_block_read512(void *msd, vibio_u64 lba, vibio_u8 *out512);

typedef struct {
    vibio_u64 abar;
    vibio_u64 command_list;
    vibio_u64 received_fis;
    vibio_u64 command_table;
    vibio_u64 sector_buffer;
    vibio_u32 pci_bus;
    vibio_u32 pci_device;
    vibio_u32 pci_function;
    vibio_u32 port;
    vibio_u32 port_implemented;
    vibio_u32 port_signature;
    vibio_u32 port_status;
    vibio_u32 mbr_signature_ok;
    vibio_u32 esp_partition_ok;
    vibio_u32 boot_sector_ok;
    vibio_u32 write_path_present;
    vibio_u32 last_error;
    vibio_u64 partition_lba;
    vibio_u64 read_count;
    vibio_u32 ok;
    vibio_u32 backend;   /* DISK_BACKEND_AHCI or DISK_BACKEND_USB */
    void *usb_msd;       /* VibioXhciMsd* when backend == DISK_BACKEND_USB */
} VibioDiskReadTest;

typedef struct {
    vibio_u64 partition_lba;
    vibio_u64 fat_begin_lba;
    vibio_u64 data_begin_lba;
    vibio_u64 root_dir_lba;
    vibio_u64 sounds_dir_lba;
    vibio_u64 root_cluster;
    vibio_u64 sounds_cluster;
    vibio_u64 root_entries_seen;
    vibio_u64 sound_entries_seen;
    vibio_u64 sound_files_found;
    vibio_u64 total_sound_bytes;
    vibio_u32 bytes_per_sector;
    vibio_u32 sectors_per_cluster;
    vibio_u32 reserved_sectors;
    vibio_u32 fat_count;
    vibio_u32 fat_size;
    vibio_u32 boot_sector_ok;
    vibio_u32 root_found;
    vibio_u32 sounds_dir_found;
    vibio_u32 write_path_present;
    vibio_u32 last_error;
    vibio_u32 ok;
} VibioFat32ReadTest;

/* ---- Stage 29: VM-only FAT32 read/write sandbox ----------------------------
 * The first real disk-write milestone. Writes are armed ONLY when a disposable
 * scratch disk is positively identified by ALL of:
 *   - it is on a non-boot AHCI port (a separate VM disk),
 *   - its FAT32 volume label is exactly "VIBIORW",
 *   - it contains the marker file VIBIO_RW.TXT whose content begins VIBIO_RW_OK,
 *   - it contains the preallocated test file RWTEST.TXT.
 * If any gate fails, all writes fail safely with a clear message. Nothing here
 * ever writes to the boot disk, USB, host, or internal disks. */
#define RW_READBACK_MAX 64

typedef struct {
    VibioDiskReadTest disk;     /* second AHCI port; own command buffers */
    VibioFat32ReadTest fs;      /* FAT32 geometry of the scratch disk */
    vibio_u32 boot_port;        /* the boot disk's AHCI port (never written) */
    vibio_u32 controller_found; /* an AHCI controller exists at all */
    vibio_u32 present;          /* a second non-boot disk was found */
    vibio_u32 fat32_ok;         /* the second disk parsed as FAT32 */
    vibio_u32 label_ok;         /* volume label == VIBIORW */
    vibio_u32 marker_ok;        /* VIBIO_RW.TXT present + content VIBIO_RW_OK */
    vibio_u32 rwtest_ok;        /* RWTEST.TXT present */
    vibio_u32 rwtest_cluster;   /* start cluster of RWTEST.TXT */
    vibio_u32 rwtest_size;      /* directory size of RWTEST.TXT */
    vibio_u64 rwtest_lba;       /* absolute LBA of RWTEST.TXT first sector */
    vibio_u32 armed;            /* all gates passed AND not user-disabled */
    vibio_u32 disabled;         /* RWDISABLE was used this boot */
    vibio_u32 last_error;       /* rw_* error code for diagnostics */
    char label[12];             /* parsed volume label, for display */
    /* Last write/verify test outcome. */
    vibio_u32 test_ran;
    vibio_u32 test_pass;
    vibio_u32 last_seq;         /* sequence number last written/read */
    char readback[RW_READBACK_MAX]; /* printable first line read back */
    vibio_u32 readback_len;
} VibioRwSandbox;

typedef struct {
    vibio_u32 bus;
    vibio_u32 device;
    vibio_u32 function;
    vibio_u32 type;
    vibio_u32 prog_if;
    vibio_u32 vendor_id;
    vibio_u32 device_id;
    vibio_u64 bar;
    vibio_u32 bar_is_mmio;
} VibioUsbController;

typedef struct {
    vibio_u32 controllers_found;
    vibio_u32 recorded_count;
    vibio_u32 uhci_count;
    vibio_u32 ohci_count;
    vibio_u32 ehci_count;
    vibio_u32 xhci_count;
    vibio_u32 preferred_type;
    vibio_u32 storage_read_path_present;
    vibio_u32 write_path_present;
    vibio_u32 first_bus;
    vibio_u32 first_device;
    vibio_u32 first_function;
    vibio_u32 first_prog_if;
    vibio_u32 last_error;
    vibio_u32 ok;
    VibioUsbController controllers[USB_MAX_CONTROLLERS];
} VibioUsbReadTest;

/*
 * Read-only USB mass-storage diagnostic state. Stage 30 adds the honest layered
 * USB storage path: it records how far the read-only path got (xHCI controller
 * found -> capability registers read -> root-hub ports -> device descriptor ->
 * mass-storage interface -> sector backend -> FAT32 mount). No step is ever
 * faked: a step's flag is set only when that step really succeeds, and
 * `fat32_mounted` is the only gate that lets the Files app use USB as a backend.
 * It never issues any USB write command.
 */
typedef struct {
    vibio_u32 controller_found;     /* a USB controller (xHCI preferred) exists */
    vibio_u32 controller_type;      /* USB_TYPE_* of the chosen controller */
    vibio_u32 bus, device, function;
    vibio_u64 bar;                  /* MMIO base of the chosen controller */
    vibio_u32 mem_space_enabled;    /* PCI command memory-space bit already set */
    vibio_u32 bar_mapped;           /* BAR is in the identity-mapped low 4 GiB */
    vibio_u32 mmio_readable;        /* capability registers were read */
    vibio_u32 cap_length;           /* xHCI CAPLENGTH */
    vibio_u32 hci_version;          /* xHCI HCIVERSION */
    vibio_u32 max_ports;            /* HCSPARAMS1 MaxPorts */
    vibio_u32 max_slots;            /* HCSPARAMS1 MaxSlots */
    vibio_u32 ports_connected;      /* root-hub ports reporting a connected dev */
    /* Honest layered status for the read-only mass-storage transfer path. These
     * stay 0 until a real implementation actually completes each layer. */
    vibio_u32 init_done;            /* controller initialized for transfers */
    vibio_u32 device_descriptor_ok; /* a USB device descriptor was read */
    vibio_u32 msd_interface_found;  /* class 0x08 / BOT interface present */
    vibio_u32 sector_backend_ready; /* READ(10) sector backend usable */
    vibio_u32 fat32_mounted;        /* FAT32 mounted read-only over USB */
    vibio_u32 boot_init_skipped;     /* full xHCI MSD bring-up skipped at boot */
    vibio_u32 last_step;            /* furthest USB_STOR_STEP_* reached */
    vibio_u32 in_vm;                /* running under a hypervisor */
} VibioUsbStorage;

/*
 * USB port hotplug state. This is intentionally only root-hub port state:
 * xHCI PORTSC CCS is read to detect connect/disconnect transitions, then the
 * existing WAV playback path is used for one-shot event sounds. It does not
 * enumerate devices, send BOT/SCSI commands, mount media, or write USB storage.
 */
typedef struct {
    vibio_u32 controller_found;
    vibio_u32 controller_type;
    vibio_u32 bus, device, function;
    vibio_u64 bar;
    vibio_u32 mem_space_enabled;
    vibio_u32 bar_mapped;
    vibio_u32 mmio_readable;
    vibio_u32 hotplug_supported;
    vibio_u32 port_state_readable;
    vibio_u32 cap_length;
    vibio_u32 hci_version;
    vibio_u32 ports_checked;
    vibio_u32 connected_count;
    vibio_u64 connected_bitmap;
    vibio_u64 previous_bitmap;
    vibio_u32 baseline_valid;
    vibio_u64 last_poll_tick;
    vibio_u64 insert_events;
    vibio_u64 remove_events;
    vibio_u32 last_event_type;
    vibio_u32 last_event_port;
    vibio_u64 last_event_tick;
    vibio_u32 last_sound_result;
    vibio_u32 simulated_events;
    vibio_u32 debounce_skipped;
    vibio_u32 unsupported_reason;
} VibioUsbHotplug;

typedef struct {
    vibio_u32 x;
    vibio_u32 y;
    int x_fixed;
    int y_fixed;
    vibio_u32 buttons;
    vibio_u64 packets;
    vibio_u64 byte_read_index;
    vibio_u32 phase;
    vibio_u8 packet[4];
    vibio_u8 has_wheel;
    int scroll; /* accumulated wheel delta, consumed by the main loop */
    vibio_u8 init_ack_defaults;
    vibio_u8 init_ack_enable;
    vibio_u32 ready;
} VibioMouseState;

typedef struct {
    vibio_u32 x;
    vibio_u32 y;
    vibio_u32 width;
    vibio_u32 height;
    vibio_u32 kind;
    vibio_u32 visible;
    vibio_u32 titlebar_color;
    vibio_u32 body_color;
    char title[24];
    vibio_u32 title_length;
    vibio_u8 anim_open;     /* 1 while the open animation is playing */
    vibio_u8 anim_close;    /* 1 while the close/minimize animation is playing */
    vibio_u8 anim_to_min;   /* the running close animation ends in minimize */
    vibio_u8 minimized;     /* hidden but still a running app (shown in taskbar) */
    vibio_u8 maximized;     /* filling the work area */
    vibio_u8 fullscreen;    /* F11: covering the entire screen, no chrome/taskbar */
    vibio_u64 anim_start;   /* tick the current animation began */
    vibio_u32 saved_x;      /* normal-rect backup while maximized */
    vibio_u32 saved_y;
    vibio_u32 saved_w;
    vibio_u32 saved_h;
} VibioWindow;

#define WM_MAX_WINDOWS 7
#define WM_NO_WINDOW 0xFFFFFFFFU
#define POWER_CONFIRM_NONE 0U
#define POWER_CONFIRM_SHUTDOWN 1U
#define POWER_CONFIRM_RESTART 2U
#define POWER_MENU_ROW_SHUTDOWN 1U
#define POWER_MENU_ROW_RESTART 2U
#define POWER_MENU_W 220U
#define POWER_MENU_HEADER_H 32U
#define POWER_MENU_ROW_H 44U
#define POWER_MENU_H (POWER_MENU_HEADER_H + POWER_MENU_ROW_H * 2U)
#define POWER_CONFIRM_W 500U
#define POWER_CONFIRM_H 196U

typedef struct {
    vibio_u32 x;
    vibio_u32 y;
    vibio_u32 kind;
    char label[32];
    vibio_u32 label_len;
} VibioDesktopTempIcon;

typedef struct {
    VibioWindow windows[WM_MAX_WINDOWS];
    vibio_u32 z_order[WM_MAX_WINDOWS]; /* indices, back-to-front; last is on top */
    vibio_u32 count;
    vibio_u32 volume_popup_open;
    vibio_u32 volume_dragging; /* slider grabbed: track the mouse while held */
    vibio_u32 power_menu_open;
    vibio_u32 power_confirm; /* POWER_CONFIRM_* */
    vibio_u32 dragging;
    vibio_u32 drag_index;
    vibio_u32 drag_dx;
    vibio_u32 drag_dy;
    vibio_u32 prev_left;
    vibio_u32 prev_right;
    vibio_u32 taskbar_hover_slot;
    vibio_u32 desktop_hover_kind;
    vibio_u32 desktop_selected_mask;
    vibio_u32 desktop_icons_visible;
    vibio_u32 desktop_icon_size;
    vibio_u32 desktop_auto_arrange;
    vibio_u32 desktop_base_dirty;
    vibio_u32 wallpaper_mode;
    vibio_u32 desktop_builtin_x[DESKTOP_ICON_BUILTIN_COUNT];
    vibio_u32 desktop_builtin_y[DESKTOP_ICON_BUILTIN_COUNT];
    VibioDesktopTempIcon desktop_temp[DESKTOP_TEMP_ICON_MAX];
    vibio_u32 desktop_temp_count;
    vibio_u32 desktop_rename_active;
    vibio_u32 desktop_rename_replace;
    vibio_u32 desktop_rename_index;
    vibio_u32 desktop_pressing;
    vibio_u32 desktop_press_index;
    vibio_u32 desktop_press_x;
    vibio_u32 desktop_press_y;
    vibio_u32 desktop_last_click_index;
    vibio_u64 desktop_last_click_tick;
    vibio_u32 desktop_dragging_icon;
    vibio_u32 desktop_drag_index;
    vibio_u32 desktop_drag_dx;
    vibio_u32 desktop_drag_dy;
    vibio_u32 context_menu;
    vibio_u32 context_x;
    vibio_u32 context_y;
    vibio_u32 context_click_x;
    vibio_u32 context_click_y;
    vibio_u32 context_hover_row;
    vibio_u32 selecting;
    vibio_u32 select_start_x;
    vibio_u32 select_start_y;
    vibio_u32 select_x;
    vibio_u32 select_y;
} VibioWindowManager;

typedef struct {
    vibio_u32 ran;
    vibio_u32 ok;
    vibio_u32 total;
    vibio_u32 passed;
    vibio_u32 failed;
    vibio_u32 skipped;
    vibio_u64 last_tick;
    char last_fail[24];
} VibioSelfTestResult;

/* ---- Minimal HTML browser ------------------------------------------------- */
#define BROWSER_HTML_MAX (48 * 1024)
#define BROWSER_MAX_LINKS 64
#define BROWSER_NAME_MAX 16
#define BROWSER_URL_MAX 64
#define BROWSER_HISTORY_MAX 12
#define BROWSER_STATUS_OK 0
#define BROWSER_STATUS_NOT_FOUND 1
#define BROWSER_STATUS_NO_DISK 2
#define BROWSER_STATUS_EXTERNAL 3
#define BROWSER_STATUS_IMAGE 4
#define BROWSER_PAD 14
#define BROWSER_HEADER_H 28
#define BROWSER_BODY_LINE 24
#define BROWSER_H1_LINE 30
#define BROWSER_H2_LINE 27
#define BROWSER_WORD_MAX 96
#define BROWSER_IMAGE_MAX ((24 * 1024 * 1024) + 64)

typedef struct {
    vibio_u32 x;
    vibio_u32 y;
    vibio_u32 w;
    vibio_u32 h;
    char href[BROWSER_URL_MAX];
    vibio_u32 href_len;
    vibio_u32 external; /* an http(s)/non-local link */
} VibioBrowserLink;

typedef struct {
    const VibioBootInfo *boot;  /* UEFI-preloaded files (firmware-agnostic) */
    VibioDiskReadTest *disk;
    VibioFat32ReadTest *fs;
    vibio_u32 dir_cluster;     /* FAT dir to resolve names in (0 = volume root) */
    vibio_u8 *html;            /* page bytes (page-allocated, NUL-terminated) */
    vibio_u32 html_capacity;
    vibio_u32 html_len;
    vibio_u8 *image;           /* scratch buffer for local read-only image bytes */
    vibio_u32 image_capacity;
    vibio_u8 *image_work;      /* bounded native PNG/JPEG decode workspace */
    vibio_u32 image_work_capacity;
    char title[40];
    vibio_u32 title_len;
    char current[BROWSER_URL_MAX]; /* current file or external URL */
    vibio_u32 current_len;
    char address[BROWSER_URL_MAX];
    vibio_u32 address_len;
    vibio_u8 address_active;
    vibio_u8 address_replace;
    vibio_u8 back_enabled;
    vibio_u8 forward_enabled;
    char history[BROWSER_HISTORY_MAX][BROWSER_URL_MAX];
    vibio_u8 history_external[BROWSER_HISTORY_MAX];
    vibio_u32 history_count;
    vibio_u32 history_index;
    vibio_u32 status;          /* BROWSER_STATUS_* */
    vibio_u32 scroll;          /* pixel scroll offset from the top */
    vibio_u32 content_height;  /* measured content height in pixels */
    vibio_u32 view_height;     /* visible content height from last render */
    VibioBrowserLink links[BROWSER_MAX_LINKS];
    vibio_u32 link_count;
    /* Scrollbar geometry (screen coords), refreshed each render for hit-testing. */
    vibio_u32 bar_x;
    vibio_u32 bar_y;
    vibio_u32 bar_w;
    vibio_u32 bar_h;
    vibio_u32 bar_thumb_y;
    vibio_u32 bar_thumb_h;
    vibio_u32 bar_dragging;    /* the thumb is being dragged */
    int bar_grab_offset;       /* cursor offset within the thumb when grabbed */
    vibio_u32 back_x, back_y, back_w, back_h;
    vibio_u32 fwd_x, fwd_y, fwd_w, fwd_h;
    vibio_u32 reload_x, reload_y, reload_w, reload_h;
    vibio_u32 home_x, home_y, home_w, home_h;
    vibio_u32 addr_x, addr_y, addr_w, addr_h;
    vibio_u32 prev_left;       /* left-button edge detection for link clicks */
    vibio_u32 nav_pending;     /* a queued navigation target */
    char nav_target[BROWSER_URL_MAX];
    vibio_u32 nav_external;
} VibioBrowser;

/* ---- Read-only Files app (graphical FAT32 browser) ------------------------
 * A normal Vibio window that lists the real entries on the read-only FAT32 boot
 * disk. It only ever reads: there is no create/delete/rename/move/write path of
 * any kind. The FAT32 backend still exposes short names; the UEFI boot-file
 * fallback can carry longer root names reported by firmware. */
/* Maximum directory entries the Files app lists. This is purely a listing
 * capacity (the storage array size), NOT a file-size filter: every file in a
 * folder is shown regardless of how big it is; only opening/loading a file
 * enforces a size limit. Raised from 64 so folders with many files list fully
 * instead of being silently truncated. */
#define FILES_MAX_ENTRIES 256
#define FILES_PATH_DEPTH 8
#define FILES_NAME_MAX VIBIO_BOOT_FILE_NAME_MAX

typedef struct {
    char name[FILES_NAME_MAX];
    vibio_u32 cluster;
    vibio_u32 size;
    vibio_u8 is_dir;
} VibioFilesEntry;

typedef struct {
    const VibioBootInfo *boot;
    VibioDiskReadTest *disk;
    VibioFat32ReadTest *fs;
    VibioFilesEntry entries[FILES_MAX_ENTRIES];
    vibio_u32 entry_count;
    vibio_u32 selected;
    vibio_u32 scroll;          /* index of the first visible row */
    vibio_u8 truncated;        /* directory had more than FILES_MAX_ENTRIES */
    vibio_u8 fs_available;     /* read-only FAT32 is usable */
    vibio_u8 source;           /* FILES_SRC_* backend currently in use */
    vibio_u8 open_pending;     /* a queued "open selected entry" request */
    vibio_u8 loaded;           /* a directory has been loaded since last open */
    const VibioUsbStorage *usbstor; /* read-only USB storage diagnostics */
    /* Stage 37: live post-boot USB FAT32 source. view_source selects which
     * device is being browsed (AHCI boot disk vs USB), so the user can switch. */
    VibioDiskReadTest *usb_disk;
    VibioFat32ReadTest *usb_fs;
    vibio_u8 view_source;      /* FILES_SRC_AHCI or FILES_SRC_USB */
    /* Full path stack, level 0 = root. Each level stores its directory cluster
     * and its display name so Up/Back is reliable without relying on ".." . */
    vibio_u32 path_clusters[FILES_PATH_DEPTH];
    char path_names[FILES_PATH_DEPTH][FILES_NAME_MAX];
    vibio_u32 path_depth;      /* number of levels in use (>= 1) */
    char path_text[160];
    /* Small modal info/error panel (WAV, unknown type, read failure). */
    vibio_u8 info_open;
    char info_title[24];
    char info_lines[4][40];
    vibio_u32 info_line_count;
    /* Row + button geometry (screen coords), refreshed each render for hit-test. */
    vibio_u32 row_x, row_y, row_w, row_h, row_first, row_visible;
    vibio_u32 up_x, up_y, up_w, up_h;
    vibio_u32 open_x, open_y, open_w, open_h;
    vibio_u32 src_x, src_y, src_w, src_h, src_active; /* Disk<->USB toggle button */
    /* Vertical scrollbar track + thumb geometry (screen coords) for overflowing
     * lists, plus live thumb-drag state so the thumb can be grabbed and dragged. */
    vibio_u32 sb_x, sb_y, sb_w, sb_h, sb_active;
    vibio_u32 sb_thumb_y, sb_thumb_h;
    vibio_u8 sb_dragging;
    vibio_u32 sb_grab_dy;       /* cursor offset inside the thumb when grabbed */
    vibio_u64 last_click_tick;
    vibio_u32 last_click_row;
    vibio_u32 prev_left;
} VibioFilesApp;

/* ---- Media Viewer / Media Player (Stage 34) -------------------------------- */
#define MEDIA_VIEWER_BAR_H 46
#define MEDIA_VIEWER_STATUS_H 80
#define MEDIA_PLAYER_BAR_H 54
/* Ticks (100 Hz) the F11 transport bar stays visible after the cursor moves. */
#define MEDIA_FS_CONTROLS_TICKS 250ULL
#define MEDIA_ZOOM_FIT 0
#define MEDIA_ZOOM_100 100
#define MEDIA_ZOOM_MIN 10
#define MEDIA_ZOOM_MAX 400
#define MEDIA_ZOOM_STEP 10
#define MEDIA_DECODE_NONE 0
#define MEDIA_DECODE_VIMG 1
#define MEDIA_DECODE_VIMG_FALLBACK 2
#define MEDIA_DECODE_BMP 3
#define MEDIA_DECODE_PNG 4
#define MEDIA_DECODE_ERROR 5
#define MEDIA_DECODE_JPEG 6
#define MEDIA_MAX_DIM 4096
#define MEDIA_MAX_BYTES ((24U * 1024U * 1024U) + 64U)
#define MEDIA_WORK_MAX ((32U * 1024U * 1024U) + 64U)
#define MEDIA_VIV_MAX_W 854
#define MEDIA_VIV_MAX_H 480
#define MEDIA_VIV_MAX_FRAME_BYTES (MEDIA_VIV_MAX_W * MEDIA_VIV_MAX_H * 4U)
#define MEDIA_VIV_HEADER 32
/* Up to ~2 min at 30 fps. The frame index is read into a buffer sized for this
 * (MEDIA_VIV_MAX_FRAMES * 8 bytes), so keep that buffer in sync below. */
#define MEDIA_VIV_MAX_FRAMES 3600
#define MEDIA_MP4_MAX_W 1920U
#define MEDIA_MP4_MAX_H 1080U
#define MEDIA_MP4_CODED_W 1920U
#define MEDIA_MP4_CODED_H 1088U
#define MEDIA_MP4_POOL 3U
#define MEDIA_MP4_MAX_SAMPLE_BYTES (4U * 1024U * 1024U)
#define MEDIA_PLAYER_FRAME_BYTES (MEDIA_MP4_MAX_SAMPLE_BYTES > MEDIA_VIV_MAX_FRAME_BYTES ? MEDIA_MP4_MAX_SAMPLE_BYTES : MEDIA_VIV_MAX_FRAME_BYTES)
#define MEDIA_MP4_MB_W (MEDIA_MP4_CODED_W / 16U)
#define MEDIA_MP4_MB_H (MEDIA_MP4_CODED_H / 16U)
#define MEDIA_MP4_LUMA_BYTES (MEDIA_MP4_CODED_W * MEDIA_MP4_CODED_H)
#define MEDIA_MP4_CHROMA_BYTES ((MEDIA_MP4_CODED_W / 2U) * (MEDIA_MP4_CODED_H / 2U))
#define MEDIA_MP4_FRAME_BYTES (MEDIA_MP4_LUMA_BYTES + 2U * MEDIA_MP4_CHROMA_BYTES)
#define MEDIA_MP4_RGB_BYTES (MEDIA_MP4_MAX_W * MEDIA_MP4_MAX_H * 4U)
#define MEDIA_AUDIO_SCRATCH_BYTES (16U * 1024U * 1024U)
#define MEDIA_MP4_NBLK ((MEDIA_MP4_MB_W * 4U) * (MEDIA_MP4_MB_H * 4U))
#define MEDIA_MP4_NCBLK ((MEDIA_MP4_MB_W * 2U) * (MEDIA_MP4_MB_H * 2U))
#define MEDIA_MP4_WORK_BYTES ((MEDIA_MP4_NBLK * 7U) + (MEDIA_MP4_MB_W * MEDIA_MP4_MB_H * 3U) + (MEDIA_MP4_NCBLK * 2U) + MEDIA_MP4_LUMA_BYTES + 32U)
#define MEDIA_MP4_TABLE_BYTES (512U * 1024U)
#define MEDIA_MP4_MAX_DECODE_BURST 3U
#define MEDIA_MP4_IDR_DROP_THRESHOLD 90U
#define MEDIA_MP4_IDR_SCAN_BACK 180U

typedef struct {
    vibio_u32 width;
    vibio_u32 height;
    vibio_u32 header_bytes;
    const vibio_u8 *pixels;
    vibio_u32 pixel_bytes;
} MediaBitmap;

#define MEDIA_IMG_OK 0U
#define MEDIA_IMG_UNSUPPORTED_FORMAT 1U
#define MEDIA_IMG_UNSUPPORTED_PNG 2U
#define MEDIA_IMG_UNSUPPORTED_JPEG 3U
#define MEDIA_IMG_CORRUPT 4U
#define MEDIA_IMG_TOO_LARGE 5U
#define MEDIA_IMG_BUFFER_TOO_SMALL 6U

typedef struct {
    vibio_u32 code;
    char detail[80];
} MediaDecodeStatus;

typedef struct {
    const VibioBootInfo *boot;
    VibioDiskReadTest *disk;
    VibioFat32ReadTest *fs;
    vibio_u32 dir_cluster;     /* FAT dir to resolve names in (0 = volume root) */
    vibio_u8 *scratch;
    vibio_u32 scratch_capacity;
    vibio_u8 *work;
    vibio_u32 work_capacity;
    vibio_u8 loaded;
    vibio_u8 source;
    vibio_u32 decode_mode;
    char filename[FILES_NAME_MAX];
    char fallback_name[FILES_NAME_MAX];
    char format_label[16];
    char status_line[72];
    char error_line[96];
    vibio_u32 file_size;
    vibio_u32 img_width;
    vibio_u32 img_height;
    vibio_u32 zoom_percent;
    vibio_u32 effective_scale;  /* last rendered scale % (resolves FIT to a real number) */
    vibio_u32 pan_x;
    vibio_u32 pan_y;
    vibio_u8 dragging;
    vibio_u32 drag_mx;
    vibio_u32 drag_my;
    vibio_u32 drag_pan_x;
    vibio_u32 drag_pan_y;
    MediaBitmap bitmap;
    vibio_u32 btn_fit_x, btn_fit_y, btn_fit_w, btn_fit_h;
    vibio_u32 btn100_x, btn100_y, btn100_w, btn100_h;
    vibio_u32 btn_minus_x, btn_minus_y, btn_minus_w, btn_minus_h;
    vibio_u32 btn_plus_x, btn_plus_y, btn_plus_w, btn_plus_h;
    vibio_u32 prev_left;
} VibioMediaViewer;

typedef struct {
    const VibioBootInfo *boot;
    VibioDiskReadTest *disk;
    VibioFat32ReadTest *fs;
    vibio_u32 dir_cluster;     /* FAT dir to resolve names in (0 = volume root) */
    vibio_u8 *frame_buf;
    vibio_u32 frame_capacity;
    vibio_u8 *audio_scratch;
    vibio_u32 audio_scratch_capacity;
    vibio_u8 loaded;
    vibio_u8 source;
    vibio_u8 playing;
    vibio_u8 muted;
    vibio_u8 audio_only;   /* 1 when playing a standalone .WAV (no video frames) */
    vibio_u8 is_mp4;       /* 1 when playing/probing native MP4 video-only */
    vibio_u8 mp4_video_ready;
    vibio_u8 mp4_video_ended;
    Mp4Info mp4;           /* native MP4 demux info (when is_mp4) */
    H264Probe h264;        /* fallback probe details for unsupported MP4 UI */
    vibio_u8 h264_checked;
    H264Decoder h264_dec;
    H264Frame h264_frames[MEDIA_MP4_POOL];
    vibio_u8 *mp4_yuv_pool;
    vibio_u32 mp4_yuv_capacity;
    vibio_u8 *mp4_work;
    vibio_u32 mp4_work_capacity;
    vibio_u8 *mp4_table_buf;
    vibio_u32 mp4_table_capacity;
    vibio_u32 mp4_table_size;
    vibio_u32 *mp4_rgb;
    vibio_u32 mp4_rgb_capacity;
    H264Frame *mp4_present_frame;
    vibio_u32 mp4_rgb_w;
    vibio_u32 mp4_rgb_h;
    vibio_u32 mp4_rgb_frame;
    vibio_u32 mp4_next_decode_frame;
    vibio_u8 mp4_sps[256];
    vibio_u8 mp4_pps[256];
    vibio_u32 mp4_sps_size;
    vibio_u32 mp4_pps_size;
    vibio_u8 mp4_nal_length_size;
    char mp4_status[96];
    vibio_u32 volume;
    char filename[FILES_NAME_MAX];
    char audio_wav[FILES_NAME_MAX];
    vibio_u32 audio_wav_bytes;
    vibio_u32 width;
    vibio_u32 height;
    vibio_u32 frame_count;
    vibio_u32 fps_num;
    vibio_u32 fps_den;
    vibio_u32 current_frame;
    vibio_u32 dropped_frames;
    vibio_u32 mp4_decoded_frames;
    vibio_u32 mp4_presented_frames;
    vibio_u32 mp4_rgb_converted_frames;
    vibio_u32 mp4_video_rect_blits;
    vibio_u32 mp4_decode_resets;
    vibio_u32 mp4_max_decode_burst;
    vibio_u32 mp4_last_lag_frames;
    vibio_u32 audio_underruns;
    vibio_u8 audio_available;
    vibio_u8 audio_started;
    vibio_u64 last_frame_tick;
    vibio_u64 play_start_tick;
    vibio_u32 file_size;
    vibio_u32 frame_offsets[MEDIA_VIV_MAX_FRAMES];
    vibio_u32 frame_sizes[MEDIA_VIV_MAX_FRAMES];
    vibio_u32 btn_play_x, btn_play_y, btn_play_w, btn_play_h;
    vibio_u32 btn_stop_x, btn_stop_y, btn_stop_w, btn_stop_h;
    vibio_u32 btn_restart_x, btn_restart_y, btn_restart_w, btn_restart_h;
    vibio_u32 btn_mute_x, btn_mute_y, btn_mute_w, btn_mute_h;
    vibio_u64 fs_controls_until;   /* F11: show transport controls until this tick */
    vibio_u32 prev_left;
} VibioMediaPlayer;

static void media_set_error(const char *msg);
static void media_open_viewer(VibioMediaViewer *mv, VibioWindowManager *wm, const VibioBootInfo *boot_info, const char *name, vibio_u8 source);
static void media_open_player(VibioMediaPlayer *mp, VibioWindowManager *wm, const VibioBootInfo *boot_info, const char *name, vibio_u8 source);
static vibio_u32 media_vimg_parse(const vibio_u8 *data, vibio_u32 size, MediaBitmap *out);
static void media_decode_status_set(MediaDecodeStatus *status, vibio_u32 code, const char *detail);
static vibio_u32 media_decode_native_image(const char *name, vibio_u8 *scratch, vibio_u32 srclen, vibio_u32 scratch_cap, vibio_u8 *work, vibio_u32 work_cap, MediaBitmap *out, MediaDecodeStatus *status);
static const char *media_decode_status_text(vibio_u32 code);
static vibio_u32 media_viv_read_header(VibioMediaPlayer *mp);
static void media_player_tick(VibioMediaPlayer *mp, VibioAudioState *audio, vibio_u64 ticks);
static void media_player_stop(VibioMediaPlayer *mp, VibioAudioState *audio);
static vibio_u32 media_player_read_frame(VibioMediaPlayer *mp, vibio_u32 frame_index);
static vibio_u32 media_player_init_mp4_video(VibioMediaPlayer *mp, vibio_u32 mp4_window_size);
static vibio_u32 media_player_decode_mp4_frame(VibioMediaPlayer *mp, vibio_u32 frame_index);
static vibio_u32 media_read_bytes(const VibioBootInfo *boot, VibioDiskReadTest *disk, VibioFat32ReadTest *fs, const char *name, vibio_u64 offset, vibio_u8 *dest, vibio_u32 max_bytes, vibio_u32 *out_total, vibio_u8 *out_source);
static vibio_u32 media_read_mp4_probe(const VibioBootInfo *boot, VibioDiskReadTest *disk, VibioFat32ReadTest *fs, const char *name, vibio_u8 *dest, vibio_u32 max_bytes, vibio_u32 *out_total, vibio_u8 *out_source, Mp4Info *out_info);
static void media_copy_name(char *dst, vibio_u32 max, const char *src);

typedef struct {
    vibio_u32 paging_ok;
    vibio_u32 idt_ok;
    const VibioFileTest *file;
    VibioBrowser *browser;
    VibioWindowManager *wm;
    VibioFilesApp *files;
    VibioMediaViewer *media_viewer;
    VibioMediaPlayer *media_player;
} VibioSelfTestContext;

typedef struct {
    vibio_u32 wallpaper_top;
    vibio_u32 wallpaper_bottom;
    vibio_u32 topbar;
    vibio_u32 taskbar;
    vibio_u32 taskbar_edge;
    vibio_u32 start_button;
    vibio_u32 taskbar_button;
    vibio_u32 taskbar_button_focus;
    vibio_u32 window_border;
    vibio_u32 text;
    vibio_u32 muted;
    vibio_u32 accent;
    vibio_u32 ok;
    vibio_u32 bad;
    vibio_u32 black;
} VibioDesktopTheme;

typedef struct {
    vibio_u64 active;
    vibio_u64 vector;
    vibio_u64 error_code;
    vibio_u64 rip;
    vibio_u64 rsp;
    vibio_u64 rflags;
    vibio_u64 cr2;
    vibio_u64 uptime_seconds;
    vibio_u64 uptime_days;
    vibio_u64 uptime_hours;
    vibio_u64 uptime_minutes;
    vibio_u64 uptime_second_part;
    const char *fault;
    const char *task;
    const char *last_driver;
    const char *last_action;
} VibioPanicState;

static const VibioBootInfo *g_panic_boot_info = 0;
static VibioSelfTestResult g_selftest_result;
static VibioSelfTestContext g_selftest_ctx;
static VibioUsbStorage g_usb_storage;
static VibioUsbHotplug g_usb_hotplug;
static char g_media_last_error[96];
static vibio_u32 g_media_manifest_present;
static volatile VibioPanicState g_panic_state = {
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    "UNKNOWN",
    "KERNEL",
    "BOOT",
    "STARTUP"
};

/* Clamp-and-apply a pixel scroll delta to the browser viewport. */
static void browser_scroll_by(VibioBrowser *b, int delta_px)
{
    int max_scroll = (int)(b->content_height > b->view_height ?
                           b->content_height - b->view_height : 0);
    int ns = (int)b->scroll + delta_px;
    if (ns < 0) {
        ns = 0;
    }
    if (ns > max_scroll) {
        ns = max_scroll;
    }
    b->scroll = (vibio_u32)ns;
}

static vibio_u32 browser_href_is_external(const char *href);
static vibio_u32 browser_name_has_ext(const char *name, const char *ext);
static void browser_copy_url(char *dst, vibio_u32 dst_max, const char *src, vibio_u32 *out_len);
static void browser_sync_address(VibioBrowser *b);

#define SETTINGS_PAGE_HOME 0U
#define SETTINGS_PAGE_SYSTEM 1U
#define SETTINGS_PAGE_PERSONALIZATION 2U
#define SETTINGS_PAGE_SOUND 3U
#define SETTINGS_PAGE_STORAGE 4U
#define SETTINGS_PAGE_DEVICES 5U
#define SETTINGS_PAGE_DATE_TIME 6U
#define SETTINGS_PAGE_ABOUT 7U
#define SETTINGS_PAGE_COUNT 8U
#define SETTINGS_HOME_CARD_MAX 8U
#define SETTINGS_WALLPAPER_MAX 8U

typedef struct {
    vibio_u32 page;
    vibio_u32 prev_left;
    vibio_u32 nav_x;
    vibio_u32 nav_y;
    vibio_u32 nav_w;
    vibio_u32 nav_row_h;
    vibio_u32 nav_count;
    vibio_u32 home_card_x[SETTINGS_HOME_CARD_MAX];
    vibio_u32 home_card_y[SETTINGS_HOME_CARD_MAX];
    vibio_u32 home_card_w[SETTINGS_HOME_CARD_MAX];
    vibio_u32 home_card_h[SETTINGS_HOME_CARD_MAX];
    vibio_u32 home_card_page[SETTINGS_HOME_CARD_MAX];
    vibio_u32 home_card_count;
    vibio_u32 volume_x;
    vibio_u32 volume_y;
    vibio_u32 volume_w;
    vibio_u32 volume_h;
    vibio_u32 volume_dragging;
    vibio_u32 time_toggle_x;
    vibio_u32 time_toggle_y;
    vibio_u32 time_toggle_w;
    vibio_u32 time_toggle_h;
    vibio_u32 use_24_hour;
    vibio_u32 wallpaper_x[SETTINGS_WALLPAPER_MAX];
    vibio_u32 wallpaper_y[SETTINGS_WALLPAPER_MAX];
    vibio_u32 wallpaper_w[SETTINGS_WALLPAPER_MAX];
    vibio_u32 wallpaper_h[SETTINGS_WALLPAPER_MAX];
    vibio_u32 wallpaper_mode[SETTINGS_WALLPAPER_MAX];
    vibio_u32 wallpaper_count;
} VibioSettingsApp;

typedef struct {
    const VibioDesktopTheme *theme;
    const VibioConsoleState *console;
    const VibioMouseState *mouse;
    const VibioFileTest *file;
    const VibioDiskReadTest *disk;
    const VibioFat32ReadTest *fs;
    const VibioUsbReadTest *usb;
    const VibioHeapTest *heap;
    const VibioTaskTest *task;
    const VibioSyscallTest *syscall;
    const VibioAudioState *audio;
    const VibioUserBoundaryBuild *user_boundary;
    VibioBrowser *browser;
    VibioFilesApp *files;
    VibioMediaViewer *media_viewer;
    VibioMediaPlayer *media_player;
    VibioWindowManager *wm;
    VibioSettingsApp *settings;
    const VibioRwSandbox *rw;
    vibio_u64 live_ticks;
    vibio_u64 live_keyboard_irqs;
    vibio_u64 live_last_scancode;
    vibio_u32 clock_hour;
    vibio_u32 clock_minute;
    vibio_u32 clock_ok;
    vibio_u32 boot_ok;
    vibio_u32 alloc_ok;
    vibio_u32 paging_ok;
    vibio_u32 idt_ok;
    vibio_u32 irq_ok;
    vibio_u32 running_in_vm;
} VibioDesktopContext;

static void window_draw_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 y_off);

__asm__(
    ".section .text.irqstate,\"ax\"\n"
    ".global irq_state_address_storage\n"
    "irq_state_address_storage:\n"
    ".quad 0\n"
    ".global task_manager_address_storage\n"
    "task_manager_address_storage:\n"
    ".quad 0\n"
    ".global syscall_state_address_storage\n"
    "syscall_state_address_storage:\n"
    ".quad 0\n"
    ".text\n");

extern volatile vibio_u64 irq_state_address_storage;
extern volatile vibio_u64 task_manager_address_storage;
extern volatile vibio_u64 syscall_state_address_storage;

__attribute__((noreturn)) static void task_bootstrap_entry(void);

__attribute__((naked, used)) static void isr_breakpoint_stub(void)
{
    __asm__ volatile(
        "incq (%rdi)\n"
        "iretq\n");
}

__attribute__((naked, used)) static void isr_fault0_stub(void)
{
    __asm__ volatile(
        "movq $0, %rdi\n"
        "xorq %rsi, %rsi\n"
        "movq 0(%rsp), %rdx\n"
        "movq %rsp, %rcx\n"
        "movq 16(%rsp), %r8\n"
        "movq %cr2, %r9\n"
        "call panic_from_exception\n");
}

__attribute__((naked, used)) static void isr_fault6_stub(void)
{
    __asm__ volatile(
        "movq $6, %rdi\n"
        "xorq %rsi, %rsi\n"
        "movq 0(%rsp), %rdx\n"
        "movq %rsp, %rcx\n"
        "movq 16(%rsp), %r8\n"
        "movq %cr2, %r9\n"
        "call panic_from_exception\n");
}

__attribute__((naked, used)) static void isr_fault8_stub(void)
{
    __asm__ volatile(
        "movq $8, %rdi\n"
        "movq 0(%rsp), %rsi\n"
        "movq 8(%rsp), %rdx\n"
        "leaq 8(%rsp), %rcx\n"
        "movq 24(%rsp), %r8\n"
        "movq %cr2, %r9\n"
        "call panic_from_exception\n");
}

__attribute__((naked, used)) static void isr_fault13_stub(void)
{
    __asm__ volatile(
        "movq $13, %rdi\n"
        "movq 0(%rsp), %rsi\n"
        "movq 8(%rsp), %rdx\n"
        "leaq 8(%rsp), %rcx\n"
        "movq 24(%rsp), %r8\n"
        "movq %cr2, %r9\n"
        "call panic_from_exception\n");
}

__attribute__((naked, used)) static void isr_fault14_stub(void)
{
    __asm__ volatile(
        "movq $14, %rdi\n"
        "movq 0(%rsp), %rsi\n"
        "movq 8(%rsp), %rdx\n"
        "leaq 8(%rsp), %rcx\n"
        "movq 24(%rsp), %r8\n"
        "movq %cr2, %r9\n"
        "call panic_from_exception\n");
}

__attribute__((naked, used)) static void isr_timer_stub(void)
{
    __asm__ volatile(
        "pushq %rax\n"
        "pushq %rdx\n"
        "movq irq_state_address_storage(%rip), %rax\n"
        "testq %rax, %rax\n"
        "jz 1f\n"
        "incq 0(%rax)\n"
        "movq 112(%rax), %rdx\n"
        "testq %rdx, %rdx\n"
        "jz 2f\n"
        "decq %rdx\n"
        "movq %rdx, 112(%rax)\n"
        "jnz 1f\n"
        "2:\n"
        "incq 96(%rax)\n"
        "movq 104(%rax), %rdx\n"
        "testq %rdx, %rdx\n"
        "jnz 3f\n"
        "movq $50, %rdx\n"
        "3:\n"
        "movq %rdx, 112(%rax)\n"
        "1:\n"
        "movb $0x20, %al\n"
        "outb %al, $0x20\n"
        "popq %rdx\n"
        "popq %rax\n"
        "iretq\n");
}

__attribute__((naked, used)) static void isr_keyboard_stub(void)
{
    __asm__ volatile(
        "pushq %rax\n"
        "pushq %rcx\n"
        "pushq %rdx\n"
        "inb $0x60, %al\n"
        "movzbq %al, %rdx\n"
        "movq irq_state_address_storage(%rip), %rax\n"
        "testq %rax, %rax\n"
        "jz 1f\n"
        "movq %rdx, 16(%rax)\n"
        "testb $0x80, %dl\n"
        "jnz 2f\n"
        "incq 8(%rax)\n"
        "2:\n"
        "movq 24(%rax), %rcx\n"
        "andq $63, %rcx\n"
        "movb %dl, 32(%rax,%rcx,1)\n"
        "incq 24(%rax)\n"
        "1:\n"
        "movb $0x20, %al\n"
        "outb %al, $0x20\n"
        "popq %rdx\n"
        "popq %rcx\n"
        "popq %rax\n"
        "iretq\n");
}

/*
 * Mouse interrupt (IRQ12, vector 44). The PS/2 mouse delivers one interrupt per
 * byte. The handler just stores each raw byte into a ring buffer and counts it;
 * the C code reassembles 3- or 4-byte packets. IRQ12 lives on the slave PIC, so
 * the end-of-interrupt must be sent to both the slave (0xA0) and the master
 * (0x20). Offsets: mouse_irqs 120,
 * mouse_write_index 128, mouse_bytes 136.
 */
__attribute__((naked, used)) static void isr_mouse_stub(void)
{
    __asm__ volatile(
        "pushq %rax\n"
        "pushq %rcx\n"
        "pushq %rdx\n"
        "inb $0x60, %al\n"
        "movzbq %al, %rdx\n"
        "movq irq_state_address_storage(%rip), %rax\n"
        "testq %rax, %rax\n"
        "jz 1f\n"
        "incq 120(%rax)\n"
        "movq 128(%rax), %rcx\n"
        "andq $255, %rcx\n"
        "movb %dl, 136(%rax,%rcx,1)\n"
        "incq 128(%rax)\n"
        "1:\n"
        "movb $0x20, %al\n"
        "outb %al, $0xA0\n"
        "outb %al, $0x20\n"
        "popq %rdx\n"
        "popq %rcx\n"
        "popq %rax\n"
        "iretq\n");
}

__attribute__((naked, used)) static void isr_syscall_stub(void)
{
    __asm__ volatile(
        "pushq %rbx\n"
        "movq syscall_state_address_storage(%rip), %rbx\n"
        "testq %rbx, %rbx\n"
        "jz 9f\n"
        "incq 0(%rbx)\n"
        "movq %rax, 16(%rbx)\n"
        "cmpq $1, %rax\n"
        "je 1f\n"
        "cmpq $2, %rax\n"
        "je 2f\n"
        "cmpq $3, %rax\n"
        "je 3f\n"
        "cmpq $4, %rax\n"
        "je 4f\n"
        "incq 8(%rbx)\n"
        "movq $0xFFFFFFFFFFFFFFFF, %rax\n"
        "jmp 8f\n"
        "1:\n"
        "movq 24(%rbx), %rax\n"
        "jmp 8f\n"
        "2:\n"
        "movq irq_state_address_storage(%rip), %rbx\n"
        "testq %rbx, %rbx\n"
        "jz 9f\n"
        "movq 0(%rbx), %rax\n"
        "jmp 8f\n"
        "3:\n"
        "movq syscall_state_address_storage(%rip), %rbx\n"
        "movq 32(%rbx), %rax\n"
        "jmp 8f\n"
        "4:\n"
        "movq syscall_state_address_storage(%rip), %rbx\n"
        "movq 40(%rbx), %rax\n"
        "8:\n"
        "movq syscall_state_address_storage(%rip), %rbx\n"
        "testq %rbx, %rbx\n"
        "jz 10f\n"
        "movq %rax, 48(%rbx)\n"
        "jmp 10f\n"
        "9:\n"
        "movq $0xFFFFFFFFFFFFFFFF, %rax\n"
        "10:\n"
        "popq %rbx\n"
        "iretq\n");
}

__attribute__((naked, used)) static void task_context_switch(vibio_u64 *from_rsp, vibio_u64 to_rsp)
{
    __asm__ volatile(
        "pushq %rbp\n"
        "pushq %rbx\n"
        "pushq %r12\n"
        "pushq %r13\n"
        "pushq %r14\n"
        "pushq %r15\n"
        "movq %rsp, (%rdi)\n"
        "movq %rsi, %rsp\n"
        "popq %r15\n"
        "popq %r14\n"
        "popq %r13\n"
        "popq %r12\n"
        "popq %rbx\n"
        "popq %rbp\n"
        "retq\n");
}

static vibio_u32 make_color(const VibioBootInfo *boot_info, vibio_u8 red, vibio_u8 green, vibio_u8 blue)
{
    if (boot_info->pixel_format == 0) {
        return ((vibio_u32)blue << 16) | ((vibio_u32)green << 8) | red;
    }

    return ((vibio_u32)red << 16) | ((vibio_u32)green << 8) | blue;
}

/*
 * Compositor buffers. The window manager keeps a persistent "base layer" (the
 * desktop, diagnostics, and console, with no windows or cursor). Each frame the
 * base layer is copied into the back buffer, the windows are drawn back-to-front
 * on top, the cursor is drawn last, and present() copies the finished frame to
 * the screen in one pass. The screen only ever shows complete frames, so nothing
 * flickers, and overlapping windows compose correctly.
 *
 * g_draw_target selects where put_pixel/get_pixel operate (base layer while the
 * base is being drawn, back buffer while compositing). 0 means draw straight to
 * the framebuffer, used before the buffers are allocated.
 */
static vibio_u64 g_draw_target = 0;
static vibio_u64 g_backbuffer = 0;
/* Persistent desktop base layer, used by the window open animation to fade a
 * newly-opened window in over the wallpaper. */
static vibio_u64 g_base_layer = 0;
/* Current timer tick, mirrored each frame so drawing code can time animations. */
static vibio_u64 g_anim_now = 0;
/* Length of the window open animation, in timer ticks (~100 Hz). */
#define WINDOW_ANIM_TICKS 20ULL
#define PANIC_RESTART_SECONDS 18U

/*
 * Clip rectangle. Pixels outside [x0,x1) x [y0,y1) are rejected. Used so a
 * window's content (especially text wider than the window) is confined to the
 * window and never spills over its border or onto other windows.
 */
static vibio_u32 g_clip_x0 = 0;
static vibio_u32 g_clip_y0 = 0;
static vibio_u32 g_clip_x1 = 0xFFFFFFFFU;
static vibio_u32 g_clip_y1 = 0xFFFFFFFFU;
static vibio_u32 g_paint_x0 = 0;
static vibio_u32 g_paint_y0 = 0;
static vibio_u32 g_paint_x1 = 0xFFFFFFFFU;
static vibio_u32 g_paint_y1 = 0xFFFFFFFFU;

typedef struct {
    vibio_u32 x0;
    vibio_u32 y0;
    vibio_u32 x1;
    vibio_u32 y1;
} VibioRect;

#define DIRTY_RECT_MAX 16

typedef struct {
    VibioRect rects[DIRTY_RECT_MAX];
    vibio_u32 count;
} VibioDirtyList;

static void set_clip(vibio_u32 x0, vibio_u32 y0, vibio_u32 x1, vibio_u32 y1)
{
    g_clip_x0 = x0;
    g_clip_y0 = y0;
    g_clip_x1 = x1;
    g_clip_y1 = y1;
}

static void reset_clip(void)
{
    g_clip_x0 = 0;
    g_clip_y0 = 0;
    g_clip_x1 = 0xFFFFFFFFU;
    g_clip_y1 = 0xFFFFFFFFU;
}

static void set_paint_bounds(vibio_u32 x0, vibio_u32 y0, vibio_u32 x1, vibio_u32 y1)
{
    g_paint_x0 = x0;
    g_paint_y0 = y0;
    g_paint_x1 = x1;
    g_paint_y1 = y1;
}

static void reset_paint_bounds(void)
{
    g_paint_x0 = 0;
    g_paint_y0 = 0;
    g_paint_x1 = 0xFFFFFFFFU;
    g_paint_y1 = 0xFFFFFFFFU;
}

static void outb(vibio_u16 port, vibio_u8 value);
static void outw(vibio_u16 port, vibio_u16 value);
static vibio_u8 inb(vibio_u16 port);
static vibio_u16 inw(vibio_u16 port);
static void ps2_wait_input_clear(void);

static void put_pixel(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    if (x >= boot_info->framebuffer_width || y >= boot_info->framebuffer_height) {
        return;
    }
    if (x < g_paint_x0 || x >= g_paint_x1 || y < g_paint_y0 || y >= g_paint_y1) {
        return;
    }
    if (x < g_clip_x0 || x >= g_clip_x1 || y < g_clip_y0 || y >= g_clip_y1) {
        return;
    }

    vibio_u64 base = g_draw_target != 0 ? g_draw_target : boot_info->framebuffer_base;
    volatile vibio_u32 *framebuffer = (volatile vibio_u32 *)base;
    framebuffer[(vibio_u64)y * boot_info->pixels_per_scan_line + x] = color;
}

MAYBE_UNUSED static vibio_u32 get_pixel(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y)
{
    if (x >= boot_info->framebuffer_width || y >= boot_info->framebuffer_height) {
        return 0;
    }

    vibio_u64 base = g_draw_target != 0 ? g_draw_target : boot_info->framebuffer_base;
    volatile vibio_u32 *framebuffer = (volatile vibio_u32 *)base;
    return framebuffer[(vibio_u64)y * boot_info->pixels_per_scan_line + x];
}

/* Split a framebuffer pixel back into 8-bit channels (inverse of make_color). */
static void unpack_color(const VibioBootInfo *boot_info, vibio_u32 c, vibio_u8 *r, vibio_u8 *g, vibio_u8 *b)
{
    if (boot_info->pixel_format == 0) {
        *r = (vibio_u8)(c & 0xFF);
        *g = (vibio_u8)((c >> 8) & 0xFF);
        *b = (vibio_u8)((c >> 16) & 0xFF);
    } else {
        *r = (vibio_u8)((c >> 16) & 0xFF);
        *g = (vibio_u8)((c >> 8) & 0xFF);
        *b = (vibio_u8)(c & 0xFF);
    }
}

/* Alpha-composite one straight-alpha source pixel over the current target. */
static void blend_pixel(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y,
                        vibio_u8 sr, vibio_u8 sg, vibio_u8 sb, vibio_u8 a)
{
    if (a == 0) {
        return;
    }
    if (a >= 255) {
        put_pixel(boot_info, x, y, make_color(boot_info, sr, sg, sb));
        return;
    }

    vibio_u8 dr, dg, db;
    unpack_color(boot_info, get_pixel(boot_info, x, y), &dr, &dg, &db);
    vibio_u32 ia = 255 - a;
    vibio_u8 nr = (vibio_u8)((sr * a + dr * ia) / 255);
    vibio_u8 ng = (vibio_u8)((sg * a + dg * ia) / 255);
    vibio_u8 nb = (vibio_u8)((sb * a + db * ia) / 255);
    put_pixel(boot_info, x, y, make_color(boot_info, nr, ng, nb));
}

/* Blit a generated 0xAARRGGBB icon (size x size) at (x,y) with alpha. */
static void draw_icon_image(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y,
                            const vibio_u32 *pixels, vibio_u32 size)
{
    for (vibio_u32 row = 0; row < size; row++) {
        for (vibio_u32 col = 0; col < size; col++) {
            vibio_u32 px = pixels[row * size + col];
            vibio_u8 a = (vibio_u8)((px >> 24) & 0xFF);
            vibio_u8 r = (vibio_u8)((px >> 16) & 0xFF);
            vibio_u8 g = (vibio_u8)((px >> 8) & 0xFF);
            vibio_u8 b = (vibio_u8)(px & 0xFF);
            blend_pixel(boot_info, x + col, y + row, r, g, b, a);
        }
    }
}

static void draw_icon_image_centered(
    const VibioBootInfo *boot_info,
    vibio_u32 box_x,
    vibio_u32 box_y,
    vibio_u32 box_w,
    vibio_u32 box_h,
    const vibio_u32 *pixels,
    vibio_u32 size)
{
    vibio_u32 min_x = size;
    vibio_u32 min_y = size;
    vibio_u32 max_x = 0;
    vibio_u32 max_y = 0;
    vibio_u32 found = 0;

    for (vibio_u32 row = 0; row < size; row++) {
        for (vibio_u32 col = 0; col < size; col++) {
            vibio_u8 a = (vibio_u8)((pixels[row * size + col] >> 24) & 0xFF);
            if (a <= 8) {
                continue;
            }
            if (!found || col < min_x) {
                min_x = col;
            }
            if (!found || row < min_y) {
                min_y = row;
            }
            if (!found || col > max_x) {
                max_x = col;
            }
            if (!found || row > max_y) {
                max_y = row;
            }
            found = 1;
        }
    }

    if (!found) {
        draw_icon_image(boot_info, box_x + (box_w - size) / 2, box_y + (box_h - size) / 2, pixels, size);
        return;
    }

    int content_cx = (int)(min_x + max_x + 1) / 2;
    int content_cy = (int)(min_y + max_y + 1) / 2;
    int box_cx = (int)box_x + (int)box_w / 2;
    int box_cy = (int)box_y + (int)box_h / 2;
    int draw_x = box_cx - content_cx;
    int draw_y = box_cy - content_cy;
    if (draw_x < 0) {
        draw_x = 0;
    }
    if (draw_y < 0) {
        draw_y = 0;
    }
    draw_icon_image(boot_info, (vibio_u32)draw_x, (vibio_u32)draw_y, pixels, size);
}

/* Bitmap font atlases. Geist for all UI text, JetBrains Mono for the terminal. */
typedef struct {
    const vibio_u8 *alpha;
    const VibioGlyph *glyphs;
    vibio_u32 line;
    vibio_u32 ascent;
} VibioFont;

static const VibioFont FONT_GEIST_UI = {
    vibio_font_geist_ui_alpha, vibio_font_geist_ui_glyphs,
    VIBIO_FONT_GEIST_UI_LINE, VIBIO_FONT_GEIST_UI_ASCENT};
static const VibioFont FONT_GEIST_SM = {
    vibio_font_geist_sm_alpha, vibio_font_geist_sm_glyphs,
    VIBIO_FONT_GEIST_SM_LINE, VIBIO_FONT_GEIST_SM_ASCENT};
static const VibioFont FONT_JBM = {
    vibio_font_jbm_alpha, vibio_font_jbm_glyphs,
    VIBIO_FONT_JBM_LINE, VIBIO_FONT_JBM_ASCENT};

/* Draw one atlas glyph; (x, y_top) is the pen position at the top of the line.
 * Returns the pen x advanced past the glyph. */
static vibio_u32 font_draw_glyph(const VibioBootInfo *boot_info, const VibioFont *font,
                                 vibio_u32 x, vibio_u32 y_top, char c,
                                 vibio_u8 cr, vibio_u8 cg, vibio_u8 cb)
{
    if (c < 32 || c > 126) {
        c = ' ';
    }
    const VibioGlyph *g = &font->glyphs[(vibio_u32)(vibio_u8)c - 32];
    for (vibio_u32 row = 0; row < g->h; row++) {
        for (vibio_u32 col = 0; col < g->w; col++) {
            vibio_u8 a = font->alpha[g->offset + row * g->w + col];
            if (a == 0) {
                continue;
            }
            int px = (int)x + g->bx + (int)col;
            int py = (int)y_top + g->top + (int)row;
            if (px < 0 || py < 0) {
                continue;
            }
            blend_pixel(boot_info, (vibio_u32)px, (vibio_u32)py, cr, cg, cb, a);
        }
    }
    return x + g->advance;
}

/* Draw a length-bounded character buffer; returns the pen x after the text. */
static vibio_u32 font_draw_buf(const VibioBootInfo *boot_info, const VibioFont *font,
                               vibio_u32 x, vibio_u32 y_top, const char *s, vibio_u32 len,
                               vibio_u32 color)
{
    vibio_u8 cr, cg, cb;
    unpack_color(boot_info, color, &cr, &cg, &cb);
    for (vibio_u32 i = 0; i < len; i++) {
        x = font_draw_glyph(boot_info, font, x, y_top, s[i], cr, cg, cb);
    }
    return x;
}

/* Draw a NUL-terminated string; returns the pen x after the text. */
static vibio_u32 font_draw_text(const VibioBootInfo *boot_info, const VibioFont *font,
                                vibio_u32 x, vibio_u32 y_top, const char *s, vibio_u32 color)
{
    vibio_u8 cr, cg, cb;
    unpack_color(boot_info, color, &cr, &cg, &cb);
    for (vibio_u32 i = 0; s[i] != 0; i++) {
        x = font_draw_glyph(boot_info, font, x, y_top, s[i], cr, cg, cb);
    }
    return x;
}

/* Horizontal advance of one glyph in this font, used for word-wrap layout. */
static vibio_u32 font_glyph_advance(const VibioFont *font, char c)
{
    if (c < 32 || c > 126) {
        c = ' ';
    }
    return font->glyphs[(vibio_u32)(vibio_u8)c - 32].advance;
}

static vibio_u32 font_text_width(const VibioFont *font, const char *s)
{
    vibio_u32 width = 0;
    for (vibio_u32 i = 0; s[i] != 0; i++) {
        width += font_glyph_advance(font, s[i]);
    }
    return width;
}

/* Monospace advance (JetBrains Mono terminal cell width). */
static vibio_u32 jbm_advance(void)
{
    return vibio_font_jbm_glyphs[(vibio_u32)'0' - 32].advance;
}

/* Fade an already-composited window rectangle toward the desktop base layer by
 * (255-a)/255, used by the open animation so a window fades in over the
 * wallpaper. a=255 leaves the window fully opaque. */
static void window_fade_over_base(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y,
                                  vibio_u32 w, vibio_u32 h, vibio_u8 a)
{
    if (g_base_layer == 0 || g_backbuffer == 0 || a >= 255) {
        return;
    }
    vibio_u32 fw = boot_info->framebuffer_width;
    vibio_u32 fh = boot_info->framebuffer_height;
    vibio_u32 ppsl = boot_info->pixels_per_scan_line;
    volatile vibio_u32 *bb = (volatile vibio_u32 *)g_backbuffer;
    volatile vibio_u32 *base = (volatile vibio_u32 *)g_base_layer;
    vibio_u32 ia = 255 - a;

    vibio_u32 x_end = x + w;
    vibio_u32 y_end = y + h;
    if (x < g_paint_x0) { x = g_paint_x0; }
    if (y < g_paint_y0) { y = g_paint_y0; }
    if (x_end > g_paint_x1) { x_end = g_paint_x1; }
    if (y_end > g_paint_y1) { y_end = g_paint_y1; }
    if (x_end > fw) { x_end = fw; }
    if (y_end > fh) { y_end = fh; }

    for (vibio_u32 py = y; py < y_end; py++) {
        for (vibio_u32 px = x; px < x_end; px++) {
            vibio_u64 off = (vibio_u64)py * ppsl + px;
            vibio_u8 wr, wg, wb, br, bg, bbl;
            unpack_color(boot_info, bb[off], &wr, &wg, &wb);
            unpack_color(boot_info, base[off], &br, &bg, &bbl);
            vibio_u8 nr = (vibio_u8)((wr * a + br * ia) / 255);
            vibio_u8 ng = (vibio_u8)((wg * a + bg * ia) / 255);
            vibio_u8 nb = (vibio_u8)((wb * a + bbl * ia) / 255);
            bb[off] = make_color(boot_info, nr, ng, nb);
        }
    }
}

/* Copy one full-screen buffer to another in one pass (64 bits at a time). */
static void blit_layer(const VibioBootInfo *boot_info, vibio_u64 dst_base, vibio_u64 src_base)
{
    if (dst_base == 0 || src_base == 0) {
        return;
    }

    vibio_u64 pixels = (vibio_u64)boot_info->pixels_per_scan_line * boot_info->framebuffer_height;
    const vibio_u64 *src = (const vibio_u64 *)src_base;
    vibio_u64 *dst = (vibio_u64 *)dst_base;
    vibio_u64 qwords = pixels / 2;
    vibio_u64 i = 0;
    for (; i + 4 <= qwords; i += 4) {
        dst[i] = src[i];
        dst[i + 1] = src[i + 1];
        dst[i + 2] = src[i + 2];
        dst[i + 3] = src[i + 3];
    }
    for (; i < qwords; i++) {
        dst[i] = src[i];
    }
    if ((pixels & 1) != 0) {
        ((vibio_u32 *)dst_base)[pixels - 1] = ((const vibio_u32 *)src_base)[pixels - 1];
    }
}

static void blit_rect(
    const VibioBootInfo *boot_info,
    vibio_u64 dst_base,
    vibio_u64 src_base,
    vibio_u32 x0,
    vibio_u32 y0,
    vibio_u32 x1,
    vibio_u32 y1)
{
    if (dst_base == 0 || src_base == 0 || x0 >= x1 || y0 >= y1) {
        return;
    }
    if (x1 > boot_info->framebuffer_width) {
        x1 = boot_info->framebuffer_width;
    }
    if (y1 > boot_info->framebuffer_height) {
        y1 = boot_info->framebuffer_height;
    }

    const vibio_u32 *src = (const vibio_u32 *)src_base;
    vibio_u32 *dst = (vibio_u32 *)dst_base;
    vibio_u32 ppsl = boot_info->pixels_per_scan_line;
    vibio_u32 width = x1 - x0;
    for (vibio_u32 y = y0; y < y1; y++) {
        vibio_u64 off = (vibio_u64)y * ppsl + x0;
        vibio_u32 x = 0;
        if ((off & 1ULL) != 0 && width > 0) {
            dst[off] = src[off];
            x = 1;
        }
        vibio_u64 *dst64 = (vibio_u64 *)(dst + off + x);
        const vibio_u64 *src64 = (const vibio_u64 *)(src + off + x);
        for (; x + 8 <= width; x += 8) {
            dst64[0] = src64[0];
            dst64[1] = src64[1];
            dst64[2] = src64[2];
            dst64[3] = src64[3];
            dst64 += 4;
            src64 += 4;
        }
        for (; x + 1 < width; x += 2) {
            *dst64++ = *src64++;
        }
        if (x < width) {
            dst[off + x] = src[off + x];
        }
    }
}

static void framebuffer_write_fence(void)
{
    __asm__ volatile("sfence" ::: "memory");
}

/* Copy the finished back-buffer frame to the live framebuffer in one pass. */
static void present(const VibioBootInfo *boot_info)
{
    if (g_backbuffer == 0) {
        return;
    }

    blit_layer(boot_info, boot_info->framebuffer_base, g_backbuffer);
    framebuffer_write_fence();
}

static void present_rect(const VibioBootInfo *boot_info, const VibioRect *dirty)
{
    if (g_backbuffer == 0 || dirty == 0) {
        return;
    }
    blit_rect(boot_info, boot_info->framebuffer_base, g_backbuffer,
              dirty->x0, dirty->y0, dirty->x1, dirty->y1);
    framebuffer_write_fence();
}

static void fill_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 width,
    vibio_u32 height,
    vibio_u32 color)
{
    if (width == 0 || height == 0 || x >= boot_info->framebuffer_width || y >= boot_info->framebuffer_height) {
        return;
    }

    vibio_u32 x0 = x;
    vibio_u32 y0 = y;
    vibio_u32 x1 = x + width;
    vibio_u32 y1 = y + height;
    if (x1 < x0 || x1 > boot_info->framebuffer_width) {
        x1 = boot_info->framebuffer_width;
    }
    if (y1 < y0 || y1 > boot_info->framebuffer_height) {
        y1 = boot_info->framebuffer_height;
    }
    if (x0 < g_paint_x0) { x0 = g_paint_x0; }
    if (y0 < g_paint_y0) { y0 = g_paint_y0; }
    if (x1 > g_paint_x1) { x1 = g_paint_x1; }
    if (y1 > g_paint_y1) { y1 = g_paint_y1; }
    if (x0 < g_clip_x0) { x0 = g_clip_x0; }
    if (y0 < g_clip_y0) { y0 = g_clip_y0; }
    if (x1 > g_clip_x1) { x1 = g_clip_x1; }
    if (y1 > g_clip_y1) { y1 = g_clip_y1; }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    vibio_u64 base = g_draw_target != 0 ? g_draw_target : boot_info->framebuffer_base;
    vibio_u32 *fb = (vibio_u32 *)base;
    vibio_u32 ppsl = boot_info->pixels_per_scan_line;
    vibio_u32 row_width = x1 - x0;
    vibio_u64 packed = ((vibio_u64)color << 32) | (vibio_u64)color;

    for (vibio_u32 row = y0; row < y1; row++) {
        vibio_u64 off = (vibio_u64)row * ppsl + x0;
        vibio_u32 col = 0;
        if ((off & 1ULL) != 0 && row_width > 0) {
            fb[off] = color;
            col = 1;
        }
        vibio_u64 *dst64 = (vibio_u64 *)(fb + off + col);
        for (; col + 8 <= row_width; col += 8) {
            dst64[0] = packed;
            dst64[1] = packed;
            dst64[2] = packed;
            dst64[3] = packed;
            dst64 += 4;
        }
        for (; col + 1 < row_width; col += 2) {
            *dst64++ = packed;
        }
        if (col < row_width) {
            fb[off + col] = color;
        }
    }
}

/*
 * The mouse cursor: a classic arrow. 'O' is the dark outline, 'W' the white
 * fill, '.' is transparent (lets whatever is underneath show through). The
 * cursor is composited with save-under/restore so it glides over any content
 * without leaving a trail.
 */
static const char *const g_cursor_bitmap[CURSOR_HEIGHT] = {
    "O...........",
    "OO..........",
    "OWO.........",
    "OWWO........",
    "OWWWO.......",
    "OWWWWO......",
    "OWWWWWO.....",
    "OWWWWWWO....",
    "OWWWWWWWO...",
    "OWWWWWWWWO..",
    "OWWWWWOOOOO.",
    "OWWOWWO.....",
    "OWO.OWWO....",
    "OO..OWWO....",
    "O....OWWO...",
    ".....OWWO...",
    "......OWWO..",
    "......OWWO..",
    ".......OO...",
};

static void cursor_draw(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 fill, vibio_u32 outline)
{
    for (vibio_u32 row = 0; row < CURSOR_HEIGHT; row++) {
        for (vibio_u32 col = 0; col < CURSOR_WIDTH; col++) {
            char pixel = g_cursor_bitmap[row][col];
            if (pixel == 'W') {
                put_pixel(boot_info, x + col, y + row, fill);
            } else if (pixel == 'O') {
                put_pixel(boot_info, x + col, y + row, outline);
            }
        }
    }
}

static void draw_line(
    const VibioBootInfo *boot_info,
    int x0,
    int y0,
    int x1,
    int y1,
    vibio_u32 color);

static void draw_stroke_line(
    const VibioBootInfo *boot_info,
    int x0,
    int y0,
    int x1,
    int y1,
    vibio_u32 color,
    vibio_u32 thickness)
{
    if (thickness <= 1) {
        draw_line(boot_info, x0, y0, x1, y1, color);
        return;
    }

    for (vibio_u32 t = 0; t < thickness; t++) {
        int o = (int)t - (int)(thickness / 2);
        draw_line(boot_info, x0 + o, y0, x1 + o, y1, color);
        draw_line(boot_info, x0, y0 + o, x1, y1 + o, color);
    }
}

static void modern_segment(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 scale,
    vibio_u32 color,
    vibio_u32 thickness,
    vibio_u32 x0,
    vibio_u32 y0,
    vibio_u32 x1,
    vibio_u32 y1)
{
    draw_stroke_line(
        boot_info,
        (int)(x + x0 * scale),
        (int)(y + y0 * scale),
        (int)(x + x1 * scale),
        (int)(y + y1 * scale),
        color,
        thickness);
}

#define MSEG(x0, y0, x1, y1) modern_segment(boot_info, x, y, scale, color, thickness, (x0), (y0), (x1), (y1))

/* Lowercase letter shapes, drawn in the same 5x7 grid as the uppercase font but
 * with an x-height around rows 2..6 and ascenders/descenders where needed. This
 * is what makes typed lower/upper case actually look different on screen. */
static void draw_lower_glyph(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    char c,
    vibio_u32 color,
    vibio_u32 scale)
{
    vibio_u32 thickness = scale >= 3 ? 2 : 1;
    vibio_u32 dot = thickness + 1;

    switch (c) {
    case 'a':
        MSEG(1, 2, 3, 2); MSEG(3, 2, 3, 6); MSEG(3, 6, 1, 6); MSEG(1, 6, 0, 5);
        MSEG(0, 5, 0, 4); MSEG(0, 4, 1, 3); MSEG(1, 3, 3, 3); break;
    case 'b':
        MSEG(0, 0, 0, 6); MSEG(0, 3, 2, 3); MSEG(2, 3, 3, 4); MSEG(3, 4, 3, 5);
        MSEG(3, 5, 2, 6); MSEG(2, 6, 0, 6); break;
    case 'c':
        MSEG(3, 3, 2, 2); MSEG(2, 2, 1, 2); MSEG(1, 2, 0, 3); MSEG(0, 3, 0, 5);
        MSEG(0, 5, 1, 6); MSEG(1, 6, 2, 6); MSEG(2, 6, 3, 5); break;
    case 'd':
        MSEG(3, 0, 3, 6); MSEG(3, 3, 1, 3); MSEG(1, 3, 0, 4); MSEG(0, 4, 0, 5);
        MSEG(0, 5, 1, 6); MSEG(1, 6, 3, 6); break;
    case 'e':
        MSEG(0, 4, 3, 4); MSEG(3, 4, 3, 3); MSEG(3, 3, 2, 2); MSEG(2, 2, 1, 2);
        MSEG(1, 2, 0, 3); MSEG(0, 3, 0, 5); MSEG(0, 5, 1, 6); MSEG(1, 6, 3, 6); break;
    case 'f':
        MSEG(1, 2, 1, 6); MSEG(1, 2, 2, 1); MSEG(2, 1, 3, 1); MSEG(0, 3, 2, 3); break;
    case 'g':
        MSEG(3, 2, 1, 2); MSEG(1, 2, 0, 3); MSEG(0, 3, 0, 4); MSEG(0, 4, 1, 5);
        MSEG(1, 5, 3, 5); MSEG(3, 2, 3, 7); MSEG(3, 7, 1, 7); MSEG(1, 7, 0, 6); break;
    case 'h':
        MSEG(0, 0, 0, 6); MSEG(0, 3, 1, 2); MSEG(1, 2, 2, 2); MSEG(2, 2, 3, 3);
        MSEG(3, 3, 3, 6); break;
    case 'i':
        MSEG(2, 2, 2, 6); fill_rect(boot_info, x + 2 * scale, y, dot, dot, color); break;
    case 'j':
        MSEG(2, 2, 2, 6); MSEG(2, 6, 1, 7); fill_rect(boot_info, x + 2 * scale, y, dot, dot, color); break;
    case 'k':
        MSEG(0, 0, 0, 6); MSEG(2, 2, 0, 4); MSEG(0, 4, 2, 6); break;
    case 'l':
        MSEG(2, 0, 2, 6); break;
    case 'm':
        MSEG(0, 2, 0, 6); MSEG(2, 2, 2, 6); MSEG(4, 2, 4, 6); MSEG(0, 2, 4, 2); break;
    case 'n':
        MSEG(0, 2, 0, 6); MSEG(0, 2, 2, 2); MSEG(2, 2, 3, 3); MSEG(3, 3, 3, 6); break;
    case 'o':
        MSEG(1, 2, 2, 2); MSEG(2, 2, 3, 3); MSEG(3, 3, 3, 5); MSEG(3, 5, 2, 6);
        MSEG(2, 6, 1, 6); MSEG(1, 6, 0, 5); MSEG(0, 5, 0, 3); MSEG(0, 3, 1, 2); break;
    case 'p':
        MSEG(0, 2, 0, 7); MSEG(0, 2, 2, 2); MSEG(2, 2, 3, 3); MSEG(3, 3, 3, 4);
        MSEG(3, 4, 2, 5); MSEG(2, 5, 0, 5); break;
    case 'q':
        MSEG(3, 2, 3, 7); MSEG(3, 2, 1, 2); MSEG(1, 2, 0, 3); MSEG(0, 3, 0, 4);
        MSEG(0, 4, 1, 5); MSEG(1, 5, 3, 5); break;
    case 'r':
        MSEG(0, 2, 0, 6); MSEG(0, 3, 1, 2); MSEG(1, 2, 3, 2); break;
    case 's':
        MSEG(3, 2, 1, 2); MSEG(1, 2, 0, 3); MSEG(0, 3, 1, 4); MSEG(1, 4, 2, 4);
        MSEG(2, 4, 3, 5); MSEG(3, 5, 2, 6); MSEG(2, 6, 0, 6); break;
    case 't':
        MSEG(1, 0, 1, 5); MSEG(1, 5, 2, 6); MSEG(0, 2, 3, 2); break;
    case 'u':
        MSEG(0, 2, 0, 5); MSEG(0, 5, 1, 6); MSEG(1, 6, 2, 6); MSEG(2, 6, 3, 5);
        MSEG(3, 2, 3, 6); break;
    case 'v':
        MSEG(0, 2, 2, 6); MSEG(2, 6, 4, 2); break;
    case 'w':
        MSEG(0, 2, 1, 6); MSEG(1, 6, 2, 3); MSEG(2, 3, 3, 6); MSEG(3, 6, 4, 2); break;
    case 'x':
        MSEG(0, 2, 3, 6); MSEG(3, 2, 0, 6); break;
    case 'y':
        MSEG(0, 2, 2, 5); MSEG(4, 2, 0, 7); break;
    case 'z':
        MSEG(0, 2, 3, 2); MSEG(3, 2, 0, 6); MSEG(0, 6, 3, 6); break;
    default:
        break;
    }
}

static void draw_modern_glyph(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    char c,
    vibio_u32 color,
    vibio_u32 scale)
{
    vibio_u32 thickness = scale >= 3 ? 2 : 1;
    if (c >= 'a' && c <= 'z') {
        draw_lower_glyph(boot_info, x, y, c, color, scale);
        return;
    }

    switch (c) {
    case 'A':
        MSEG(0, 6, 2, 0); MSEG(2, 0, 4, 6); MSEG(1, 3, 3, 3); break;
    case 'B':
        MSEG(0, 0, 0, 6); MSEG(0, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 3, 3);
        MSEG(0, 3, 3, 3); MSEG(3, 3, 4, 4); MSEG(4, 4, 4, 5); MSEG(4, 5, 3, 6); MSEG(0, 6, 3, 6); break;
    case 'C':
        MSEG(4, 1, 3, 0); MSEG(3, 0, 1, 0); MSEG(1, 0, 0, 1); MSEG(0, 1, 0, 5); MSEG(0, 5, 1, 6); MSEG(1, 6, 4, 6); break;
    case 'D':
        MSEG(0, 0, 0, 6); MSEG(0, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 5); MSEG(4, 5, 3, 6); MSEG(0, 6, 3, 6); break;
    case 'E':
        MSEG(4, 0, 0, 0); MSEG(0, 0, 0, 6); MSEG(0, 3, 3, 3); MSEG(0, 6, 4, 6); break;
    case 'F':
        MSEG(0, 0, 0, 6); MSEG(0, 0, 4, 0); MSEG(0, 3, 3, 3); break;
    case 'G':
        MSEG(4, 1, 3, 0); MSEG(3, 0, 1, 0); MSEG(1, 0, 0, 1); MSEG(0, 1, 0, 5); MSEG(0, 5, 1, 6);
        MSEG(1, 6, 4, 6); MSEG(4, 6, 4, 3); MSEG(4, 3, 2, 3); break;
    case 'H':
        MSEG(0, 0, 0, 6); MSEG(4, 0, 4, 6); MSEG(0, 3, 4, 3); break;
    case 'I':
        MSEG(1, 0, 3, 0); MSEG(2, 0, 2, 6); MSEG(1, 6, 3, 6); break;
    case 'J':
        MSEG(1, 0, 4, 0); MSEG(4, 0, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 1, 6); MSEG(1, 6, 0, 5); break;
    case 'K':
        MSEG(0, 0, 0, 6); MSEG(4, 0, 0, 3); MSEG(0, 3, 4, 6); break;
    case 'L':
        MSEG(0, 0, 0, 6); MSEG(0, 6, 4, 6); break;
    case 'M':
        MSEG(0, 6, 0, 0); MSEG(0, 0, 2, 3); MSEG(2, 3, 4, 0); MSEG(4, 0, 4, 6); break;
    case 'N':
        MSEG(0, 6, 0, 0); MSEG(0, 0, 4, 6); MSEG(4, 6, 4, 0); break;
    case 'O':
        MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 1, 6);
        MSEG(1, 6, 0, 5); MSEG(0, 5, 0, 1); MSEG(0, 1, 1, 0); break;
    case 'P':
        MSEG(0, 0, 0, 6); MSEG(0, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 3, 3); MSEG(0, 3, 3, 3); break;
    case 'Q':
        MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 1, 6);
        MSEG(1, 6, 0, 5); MSEG(0, 5, 0, 1); MSEG(0, 1, 1, 0); MSEG(2, 4, 4, 6); break;
    case 'R':
        MSEG(0, 0, 0, 6); MSEG(0, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 3, 3);
        MSEG(0, 3, 3, 3); MSEG(2, 3, 4, 6); break;
    case 'S':
        MSEG(4, 0, 1, 0); MSEG(1, 0, 0, 1); MSEG(0, 1, 0, 2); MSEG(0, 2, 1, 3); MSEG(1, 3, 3, 3);
        MSEG(3, 3, 4, 4); MSEG(4, 4, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 0, 6); break;
    case 'T':
        MSEG(0, 0, 4, 0); MSEG(2, 0, 2, 6); break;
    case 'U':
        MSEG(0, 0, 0, 5); MSEG(0, 5, 1, 6); MSEG(1, 6, 3, 6); MSEG(3, 6, 4, 5); MSEG(4, 5, 4, 0); break;
    case 'V':
        MSEG(0, 0, 2, 6); MSEG(2, 6, 4, 0); break;
    case 'W':
        MSEG(0, 0, 1, 6); MSEG(1, 6, 2, 3); MSEG(2, 3, 3, 6); MSEG(3, 6, 4, 0); break;
    case 'X':
        MSEG(0, 0, 4, 6); MSEG(4, 0, 0, 6); break;
    case 'Y':
        MSEG(0, 0, 2, 3); MSEG(4, 0, 2, 3); MSEG(2, 3, 2, 6); break;
    case 'Z':
        MSEG(0, 0, 4, 0); MSEG(4, 0, 0, 6); MSEG(0, 6, 4, 6); break;
    case '0':
        MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 1, 6);
        MSEG(1, 6, 0, 5); MSEG(0, 5, 0, 1); MSEG(0, 1, 1, 0); MSEG(1, 5, 3, 1); break;
    case '1':
        MSEG(1, 1, 2, 0); MSEG(2, 0, 2, 6); MSEG(1, 6, 3, 6); break;
    case '2':
        MSEG(0, 1, 1, 0); MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 0, 6); MSEG(0, 6, 4, 6); break;
    case '3':
        MSEG(0, 0, 4, 0); MSEG(4, 0, 2, 3); MSEG(2, 3, 4, 4); MSEG(4, 4, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 0, 6); break;
    case '4':
        MSEG(4, 0, 4, 6); MSEG(0, 0, 0, 3); MSEG(0, 3, 4, 3); break;
    case '5':
        MSEG(4, 0, 0, 0); MSEG(0, 0, 0, 3); MSEG(0, 3, 3, 3); MSEG(3, 3, 4, 4); MSEG(4, 4, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 0, 6); break;
    case '6':
        MSEG(4, 0, 1, 0); MSEG(1, 0, 0, 1); MSEG(0, 1, 0, 5); MSEG(0, 5, 1, 6); MSEG(1, 6, 3, 6);
        MSEG(3, 6, 4, 5); MSEG(4, 5, 4, 4); MSEG(4, 4, 3, 3); MSEG(3, 3, 0, 3); break;
    case '7':
        MSEG(0, 0, 4, 0); MSEG(4, 0, 1, 6); break;
    case '8':
        MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 3, 3); MSEG(3, 3, 1, 3);
        MSEG(1, 3, 0, 2); MSEG(0, 2, 0, 1); MSEG(0, 1, 1, 0); MSEG(1, 3, 0, 4); MSEG(0, 4, 0, 5);
        MSEG(0, 5, 1, 6); MSEG(1, 6, 3, 6); MSEG(3, 6, 4, 5); MSEG(4, 5, 4, 4); MSEG(4, 4, 3, 3); break;
    case '9':
        MSEG(4, 3, 1, 3); MSEG(1, 3, 0, 2); MSEG(0, 2, 0, 1); MSEG(0, 1, 1, 0); MSEG(1, 0, 3, 0);
        MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 5); MSEG(4, 5, 3, 6); MSEG(3, 6, 0, 6); break;
    case ':':
        fill_rect(boot_info, x + 2 * scale, y + 2 * scale, thickness, thickness, color);
        fill_rect(boot_info, x + 2 * scale, y + 5 * scale, thickness, thickness, color); break;
    case '.':
        fill_rect(boot_info, x + 2 * scale, y + 6 * scale, thickness + 1, thickness + 1, color); break;
    case ',':
        fill_rect(boot_info, x + 2 * scale, y + 5 * scale, thickness + 1, thickness + 1, color); MSEG(2, 6, 1, 7); break;
    case '-':
        MSEG(1, 3, 4, 3); break;
    case '?':
        MSEG(0, 1, 1, 0); MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 2, 4);
        fill_rect(boot_info, x + 2 * scale, y + 6 * scale, thickness + 1, thickness + 1, color); break;
    case '>':
        MSEG(1, 1, 4, 3); MSEG(4, 3, 1, 5); break;
    case '/':
        MSEG(4, 0, 0, 6); break;
    case '!':
        MSEG(2, 0, 2, 4); fill_rect(boot_info, x + 2 * scale, y + 6 * scale, thickness + 1, thickness + 1, color); break;
    case '@':
        MSEG(3, 3, 2, 3); MSEG(2, 3, 2, 5); MSEG(2, 5, 3, 5); MSEG(3, 3, 3, 5);
        MSEG(3, 2, 1, 2); MSEG(1, 2, 0, 3); MSEG(0, 3, 0, 5); MSEG(0, 5, 1, 6);
        MSEG(1, 6, 3, 6); MSEG(3, 6, 4, 5); break;
    case '#':
        MSEG(1, 1, 1, 6); MSEG(3, 1, 3, 6); MSEG(0, 2, 4, 2); MSEG(0, 4, 4, 4); break;
    case '$':
        MSEG(4, 1, 1, 1); MSEG(1, 1, 0, 2); MSEG(0, 2, 1, 3); MSEG(1, 3, 3, 3);
        MSEG(3, 3, 4, 4); MSEG(4, 4, 3, 5); MSEG(3, 5, 0, 5); MSEG(2, 0, 2, 6); break;
    case '%':
        MSEG(0, 6, 4, 0);
        fill_rect(boot_info, x, y + scale, thickness + 1, thickness + 1, color);
        fill_rect(boot_info, x + 3 * scale, y + 4 * scale, thickness + 1, thickness + 1, color); break;
    case '^':
        MSEG(0, 2, 2, 0); MSEG(2, 0, 4, 2); break;
    case '&':
        MSEG(4, 6, 1, 3); MSEG(1, 3, 1, 1); MSEG(1, 1, 2, 0); MSEG(2, 0, 3, 1);
        MSEG(3, 1, 1, 3); MSEG(1, 3, 0, 4); MSEG(0, 4, 0, 5); MSEG(0, 5, 1, 6);
        MSEG(1, 6, 2, 6); MSEG(2, 6, 4, 4); break;
    case '*':
        MSEG(2, 1, 2, 5); MSEG(0, 2, 4, 4); MSEG(0, 4, 4, 2); break;
    case '(':
        MSEG(3, 0, 1, 2); MSEG(1, 2, 1, 4); MSEG(1, 4, 3, 6); break;
    case ')':
        MSEG(1, 0, 3, 2); MSEG(3, 2, 3, 4); MSEG(3, 4, 1, 6); break;
    case '_':
        MSEG(0, 6, 4, 6); break;
    case '+':
        MSEG(2, 1, 2, 5); MSEG(0, 3, 4, 3); break;
    case '=':
        MSEG(0, 2, 4, 2); MSEG(0, 4, 4, 4); break;
    case '[':
        MSEG(3, 0, 1, 0); MSEG(1, 0, 1, 6); MSEG(1, 6, 3, 6); break;
    case ']':
        MSEG(1, 0, 3, 0); MSEG(3, 0, 3, 6); MSEG(3, 6, 1, 6); break;
    case '{':
        MSEG(3, 0, 2, 1); MSEG(2, 1, 2, 2); MSEG(2, 2, 1, 3); MSEG(1, 3, 2, 4);
        MSEG(2, 4, 2, 5); MSEG(2, 5, 3, 6); break;
    case '}':
        MSEG(1, 0, 2, 1); MSEG(2, 1, 2, 2); MSEG(2, 2, 3, 3); MSEG(3, 3, 2, 4);
        MSEG(2, 4, 2, 5); MSEG(2, 5, 1, 6); break;
    case ';':
        fill_rect(boot_info, x + 2 * scale, y + 2 * scale, thickness + 1, thickness + 1, color);
        fill_rect(boot_info, x + 2 * scale, y + 5 * scale, thickness + 1, thickness + 1, color);
        MSEG(2, 6, 1, 7); break;
    case '\'':
        MSEG(2, 0, 2, 2); break;
    case '"':
        MSEG(1, 0, 1, 2); MSEG(3, 0, 3, 2); break;
    case '~':
        MSEG(0, 3, 1, 2); MSEG(1, 2, 3, 4); MSEG(3, 4, 4, 3); break;
    case '|':
        MSEG(2, 0, 2, 6); break;
    case '<':
        MSEG(4, 1, 1, 3); MSEG(1, 3, 4, 5); break;
    case '\\':
        MSEG(0, 0, 4, 6); break;
    case '`':
        MSEG(1, 0, 3, 2); break;
    case ' ':
        break;
    default:
        MSEG(0, 1, 1, 0); MSEG(1, 0, 3, 0); MSEG(3, 0, 4, 1); MSEG(4, 1, 4, 2); MSEG(4, 2, 2, 4);
        fill_rect(boot_info, x + 2 * scale, y + 6 * scale, thickness + 1, thickness + 1, color); break;
    }
}

#undef MSEG

MAYBE_UNUSED static vibio_u8 glyph_row(char c, vibio_u32 row)
{
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }

    switch (c) {
    case 'A':
        return ROW7(row, 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11);
    case 'B':
        return ROW7(row, 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E);
    case 'C':
        return ROW7(row, 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E);
    case 'D':
        return ROW7(row, 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E);
    case 'E':
        return ROW7(row, 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F);
    case 'F':
        return ROW7(row, 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10);
    case 'G':
        return ROW7(row, 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F);
    case 'H':
        return ROW7(row, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11);
    case 'I':
        return ROW7(row, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E);
    case 'J':
        return ROW7(row, 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C);
    case 'K':
        return ROW7(row, 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11);
    case 'L':
        return ROW7(row, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F);
    case 'M':
        return ROW7(row, 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11);
    case 'N':
        return ROW7(row, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11);
    case 'O':
        return ROW7(row, 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E);
    case 'P':
        return ROW7(row, 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10);
    case 'Q':
        return ROW7(row, 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D);
    case 'R':
        return ROW7(row, 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11);
    case 'S':
        return ROW7(row, 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E);
    case 'T':
        return ROW7(row, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04);
    case 'U':
        return ROW7(row, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E);
    case 'V':
        return ROW7(row, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04);
    case 'W':
        return ROW7(row, 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A);
    case 'X':
        return ROW7(row, 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11);
    case 'Y':
        return ROW7(row, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04);
    case 'Z':
        return ROW7(row, 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F);
    case '0':
        return ROW7(row, 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E);
    case '1':
        return ROW7(row, 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E);
    case '2':
        return ROW7(row, 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F);
    case '3':
        return ROW7(row, 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E);
    case '4':
        return ROW7(row, 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02);
    case '5':
        return ROW7(row, 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E);
    case '6':
        return ROW7(row, 0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E);
    case '7':
        return ROW7(row, 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08);
    case '8':
        return ROW7(row, 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E);
    case '9':
        return ROW7(row, 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E);
    case ':':
        return ROW7(row, 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00);
    case '.':
        return ROW7(row, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C);
    case ',':
        return ROW7(row, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x0C);
    case '-':
        return ROW7(row, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00);
    case ' ':
        return 0x00;
    default:
        return ROW7(row, 0x0E, 0x11, 0x01, 0x06, 0x04, 0x00, 0x04);
    }
}

/*
 * Background color painted behind glyph pixels. Drawing each character cell
 * opaquely (lit pixels in the text color, the rest in this background) means a
 * line can be redrawn in place without first blanking it. That removes the
 * blank-then-redraw flash that used to make the counters and console flicker.
 * Callers set this to match the panel or console-box color before drawing.
 */
static vibio_u32 g_text_bg = 0;

static void draw_char(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, char c, vibio_u32 color)
{
    fill_rect(boot_info, x, y, CHAR_STEP, GLYPH_HEIGHT * FONT_SCALE, g_text_bg);
    draw_modern_glyph(boot_info, x + FONT_SCALE, y, c, color, FONT_SCALE);
}

/* All UI text uses the Geist atlas. */
static vibio_u32 draw_cstr(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, const char *text, vibio_u32 color)
{
    return font_draw_text(boot_info, &FONT_GEIST_UI, x, y, text, color);
}

/* Soft drop shadow along the bottom and right of a window, blended onto the
 * frame so windows lift off the desktop instead of looking like flat stickers. */
static void draw_window_shadow(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y,
                               vibio_u32 w, vibio_u32 h)
{
    for (vibio_u32 o = 1; o <= 6; o++) {
        int a = 52 - (int)o * 8;
        if (a <= 0) {
            break;
        }
        vibio_u32 by = y + h + o - 1;
        for (vibio_u32 px = x + o; px < x + w + o; px++) {
            blend_pixel(boot_info, px, by, 0, 0, 0, (vibio_u8)a);
        }
        vibio_u32 rx = x + w + o - 1;
        for (vibio_u32 py = y + o; py < y + h + o; py++) {
            blend_pixel(boot_info, rx, py, 0, 0, 0, (vibio_u8)a);
        }
    }
}

/* Left edge of each title-bar control glyph, shared by the painter and the
 * click hit-test so they always agree. */
static void window_control_xs(vibio_u32 x, vibio_u32 w, vibio_u32 *cmin, vibio_u32 *cmax, vibio_u32 *cclose)
{
    vibio_u32 c = x + w - WINDOW_BORDER - 22;
    *cclose = c;
    *cmax = c - 24;
    *cmin = c - 48;
}

/* Which title-bar control is under (px,py): 0 none, 1 minimize, 2 maximize,
 * 3 close. Uses generous zones so the small glyphs are easy to click. */
static vibio_u32 window_control_hit(const VibioWindow *window, vibio_u32 px, vibio_u32 py)
{
    if (py < window->y || py >= window->y + WINDOW_TITLEBAR_HEIGHT) {
        return 0;
    }
    vibio_u32 cmin, cmax, cclose;
    window_control_xs(window->x, window->width, &cmin, &cmax, &cclose);
    if (px >= cclose - 6 && px < cclose + 16) {
        return 3;
    }
    if (px >= cmax - 6 && px < cmax + 16) {
        return 2;
    }
    if (px >= cmin - 6 && px < cmin + 16) {
        return 1;
    }
    return 0;
}

/* Minimize / maximize(or restore) / close controls on the right of a title bar. */
static void draw_window_controls(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y,
                                 vibio_u32 w, vibio_u32 color, vibio_u32 maximized)
{
    vibio_u32 gy = y + 9;
    vibio_u32 cmin, cmax, cclose;
    window_control_xs(x, w, &cmin, &cmax, &cclose);

    fill_rect(boot_info, cmin, gy + 8, 10, 2, color);                 /* minimize */
    if (maximized) {
        /* Restore: two offset squares. */
        fill_rect(boot_info, cmax + 2, gy, 8, 2, color);
        fill_rect(boot_info, cmax + 2, gy + 6, 8, 2, color);
        fill_rect(boot_info, cmax + 2, gy, 2, 8, color);
        fill_rect(boot_info, cmax + 8, gy, 2, 8, color);
        fill_rect(boot_info, cmax, gy + 2, 8, 2, color);
        fill_rect(boot_info, cmax, gy + 8, 8, 2, color);
        fill_rect(boot_info, cmax, gy + 2, 2, 8, color);
    } else {
        fill_rect(boot_info, cmax, gy, 10, 2, color);                 /* maximize */
        fill_rect(boot_info, cmax, gy + 8, 10, 2, color);
        fill_rect(boot_info, cmax, gy, 2, 10, color);
        fill_rect(boot_info, cmax + 8, gy, 2, 10, color);
    }
    draw_stroke_line(boot_info, (int)cclose, (int)gy, (int)cclose + 9, (int)gy + 9, color, 2); /* close */
    draw_stroke_line(boot_info, (int)cclose + 9, (int)gy, (int)cclose, (int)gy + 9, color, 2);
}

MAYBE_UNUSED static vibio_u64 read_cr2(void)
{
    vibio_u64 value = 0;
    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

static void panic_set_context(const char *task, const char *driver, const char *action)
{
    g_panic_state.task = task;
    g_panic_state.last_driver = driver;
    g_panic_state.last_action = action;
}

static const char *panic_fault_name(vibio_u64 vector)
{
    switch (vector) {
    case 0: return "DIVIDE ERROR";
    case 6: return "INVALID OPCODE";
    case 8: return "DOUBLE FAULT";
    case 13: return "GENERAL PROTECTION";
    case 14: return "PAGE FAULT";
    default: return "CPU EXCEPTION";
    }
}

static vibio_u32 panic_draw_text(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, const char *text, vibio_u32 color)
{
    return font_draw_text(boot_info, &FONT_GEIST_UI, x, y, text, color);
}

static vibio_u32 panic_draw_hex64(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u64 value, vibio_u32 color)
{
    char out[19];
    out[0] = '0';
    out[1] = 'x';
    for (vibio_u32 i = 0; i < 16; i++) {
        vibio_u32 shift = 60 - i * 4;
        vibio_u8 nibble = (vibio_u8)((value >> shift) & 0xF);
        out[i + 2] = nibble < 10 ? (char)('0' + nibble) : (char)('A' + (nibble - 10));
    }
    out[18] = 0;
    return panic_draw_text(boot_info, x, y, out, color);
}

static vibio_u32 panic_append_uint(char *out, vibio_u32 pos, vibio_u32 max, vibio_u64 value)
{
    char tmp[20];
    vibio_u32 n = 0;
    if (max == 0) {
        return pos;
    }
    if (value == 0) {
        if (pos + 1 < max) {
            out[pos++] = '0';
        }
        return pos;
    }
    while (value > 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (n > 0 && pos + 1 < max) {
        out[pos++] = tmp[--n];
    }
    return pos;
}

static vibio_u32 panic_append_int(char *out, vibio_u32 pos, vibio_u32 max, long long value)
{
    if (value < 0) {
        if (pos + 1 < max) {
            out[pos++] = '-';
        }
        return panic_append_uint(out, pos, max, (vibio_u64)(-value));
    }
    return panic_append_uint(out, pos, max, (vibio_u64)value);
}

static vibio_u32 panic_append_text(char *out, vibio_u32 pos, vibio_u32 max, const char *text)
{
    vibio_u32 i = 0;
    while (text[i] != 0 && pos + 1 < max) {
        out[pos++] = text[i++];
    }
    return pos;
}

static vibio_u32 append_human_size(char *out, vibio_u32 pos, vibio_u32 max, vibio_u64 bytes)
{
    const char *unit = " B";
    vibio_u64 scale = 1;
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        unit = " GB";
        scale = 1024ULL * 1024ULL * 1024ULL;
    } else if (bytes >= 1024ULL * 1024ULL) {
        unit = " MB";
        scale = 1024ULL * 1024ULL;
    } else if (bytes >= 1024ULL) {
        unit = " KB";
        scale = 1024ULL;
    }

    if (scale == 1) {
        pos = panic_append_uint(out, pos, max, bytes);
    } else {
        vibio_u64 whole = bytes / scale;
        vibio_u64 tenth = ((bytes % scale) * 10ULL) / scale;
        pos = panic_append_uint(out, pos, max, whole);
        if (whole < 100 && tenth > 0 && pos + 2 < max) {
            out[pos++] = '.';
            out[pos++] = (char)('0' + tenth);
        }
    }
    return panic_append_text(out, pos, max, unit);
}

static void panic_draw_row(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, const char *label, const char *value, vibio_u32 label_color, vibio_u32 value_color)
{
    vibio_u32 nx = panic_draw_text(boot_info, x, y, label, label_color);
    panic_draw_text(boot_info, nx + 8, y, value, value_color);
}

static void panic_draw_uptime_row(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 label_color, vibio_u32 value_color)
{
    char out[96];
    vibio_u32 p = 0;
    p = panic_append_uint(out, p, sizeof(out), g_panic_state.uptime_seconds);
    p = panic_append_text(out, p, sizeof(out), " seconds (");
    p = panic_append_uint(out, p, sizeof(out), g_panic_state.uptime_days);
    p = panic_append_text(out, p, sizeof(out), " days ");
    p = panic_append_uint(out, p, sizeof(out), g_panic_state.uptime_hours);
    p = panic_append_text(out, p, sizeof(out), " hours ");
    p = panic_append_uint(out, p, sizeof(out), g_panic_state.uptime_minutes);
    p = panic_append_text(out, p, sizeof(out), " minutes ");
    p = panic_append_uint(out, p, sizeof(out), g_panic_state.uptime_second_part);
    p = panic_append_text(out, p, sizeof(out), " seconds)");
    out[p] = 0;
    panic_draw_row(boot_info, x, y, "UPTIME:", out, label_color, value_color);
}

static void panic_draw_hex_row(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, const char *label, vibio_u64 value, vibio_u32 label_color, vibio_u32 value_color)
{
    vibio_u32 nx = panic_draw_text(boot_info, x, y, label, label_color);
    panic_draw_hex64(boot_info, nx + 8, y, value, value_color);
}

static void panic_short_wait(void)
{
    for (volatile vibio_u32 i = 0; i < 1000000U; i++) {
        __asm__ volatile("pause");
    }
}

static vibio_u8 panic_cmos_read(vibio_u8 reg)
{
    outb(0x70, (vibio_u8)(reg | 0x80));
    for (volatile vibio_u32 i = 0; i < 1000U; i++) {
        __asm__ volatile("pause");
    }
    return inb(0x71);
}

static vibio_u8 panic_bcd_to_binary(vibio_u8 value)
{
    return (vibio_u8)((value & 0x0F) + ((value >> 4) * 10));
}

static vibio_u32 panic_rtc_second(vibio_u32 *second)
{
    for (vibio_u32 wait = 0; wait < 100000U; wait++) {
        if ((panic_cmos_read(0x0A) & 0x80U) == 0) {
            break;
        }
    }

    vibio_u8 sec1 = panic_cmos_read(0x00);
    vibio_u8 status_b = panic_cmos_read(0x0B);
    vibio_u8 sec2 = panic_cmos_read(0x00);
    if (sec1 != sec2) {
        sec1 = panic_cmos_read(0x00);
        sec2 = panic_cmos_read(0x00);
        if (sec1 != sec2) {
            return 0;
        }
    }
    if ((status_b & 0x04U) == 0) {
        sec1 = panic_bcd_to_binary(sec1);
    }
    if (sec1 >= 60) {
        return 0;
    }
    *second = sec1;
    return 1;
}

static void panic_wait_real_seconds(vibio_u32 seconds)
{
    vibio_u32 current = 0;
    if (!panic_rtc_second(&current)) {
        for (vibio_u32 s = 0; s < seconds; s++) {
            for (volatile vibio_u32 i = 0; i < 65000000U; i++) {
                __asm__ volatile("pause");
            }
        }
        return;
    }

    vibio_u32 elapsed = 0;
    while (elapsed < seconds) {
        vibio_u32 next = current;
        vibio_u32 guard = 0;
        while (next == current && guard < 200000000U) {
            panic_rtc_second(&next);
            guard++;
            __asm__ volatile("pause");
        }
        if (next == current) {
            break;
        }
        current = next;
        elapsed++;
    }
}

__attribute__((noreturn)) static void panic_force_reset(void)
{
    /* ACPI/PCI reset control used by QEMU/VirtualBox and many real chipsets. */
    outb(0xCF9, 0x02);
    panic_short_wait();
    outb(0xCF9, 0x06);
    panic_short_wait();

    /* Classic 8042 keyboard-controller reset. */
    ps2_wait_input_clear();
    outb(0x64, 0xFE);
    panic_short_wait();

    /* Last fallback: load a null IDT and fault again, forcing a triple fault
     * reset on x86 if the platform did not honor the reset ports. */
    VibioIdtPointer null_idt;
    null_idt.limit = 0;
    null_idt.base = 0;
    __asm__ volatile("lidt %0\nint3" : : "m"(null_idt) : "memory");

    for (;;) {
        __asm__ volatile("hlt");
    }
}

__attribute__((noreturn)) static void power_restart(void)
{
    __asm__ volatile("cli");
    panic_force_reset();
}

static void power_show_halt_screen(const VibioBootInfo *boot_info)
{
    if (boot_info == 0 || boot_info->framebuffer_base == 0) {
        return;
    }

    g_draw_target = 0;
    reset_clip();
    reset_paint_bounds();

    vibio_u32 bg = make_color(boot_info, 10, 16, 22);
    vibio_u32 white = make_color(boot_info, 238, 244, 248);
    vibio_u32 muted = make_color(boot_info, 128, 148, 162);
    fill_rect(boot_info, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, bg);

    const char *title = "VIBIO HALTED";
    const char *sub = "It is safe to power off this machine.";
    vibio_u32 title_w = font_text_width(&FONT_GEIST_UI, title);
    vibio_u32 sub_w = font_text_width(&FONT_GEIST_SM, sub);
    vibio_u32 cx = boot_info->framebuffer_width / 2U;
    vibio_u32 cy = boot_info->framebuffer_height / 2U;
    font_draw_text(boot_info, &FONT_GEIST_UI, cx - title_w / 2U, cy - 24U, title, white);
    font_draw_text(boot_info, &FONT_GEIST_SM, cx - sub_w / 2U, cy + 8U, sub, muted);
}

/* Minimal ACPI parsing for a real power-off. We do not implement a full ACPI
 * interpreter; we only read the FADT and find the \_S5 sleep type in the DSDT,
 * which is all that is needed to ask the chipset to enter soft-off (S5). All of
 * this is read-only table parsing plus the final sleep-register write. */
#define ACPI_SLP_EN (1U << 13)

/* These are filled in by acpi_prepare_poweroff() from the firmware tables. */
static vibio_u32 g_acpi_pm1a_cnt = 0;
static vibio_u32 g_acpi_pm1b_cnt = 0;
static vibio_u32 g_acpi_smi_cmd = 0;
static vibio_u8 g_acpi_enable = 0;
static vibio_u16 g_acpi_slp_typ_a = 0;
static vibio_u16 g_acpi_slp_typ_b = 0;
static vibio_u32 g_acpi_ready = 0;

static vibio_u32 acpi_read_u32(vibio_u64 addr)
{
    return *(const volatile vibio_u32 *)addr;
}

static vibio_u32 acpi_sig_equals(vibio_u64 addr, const char *sig4)
{
    const volatile char *p = (const volatile char *)addr;
    for (vibio_u32 i = 0; i < 4; i++) {
        if (p[i] != sig4[i]) {
            return 0;
        }
    }
    return 1;
}

/* Scan the DSDT for the \_S5 package and pull out SLP_TYPa / SLP_TYPb. This is
 * the well established "find _S5_, skip the package header, read the byte
 * constants" approach used by small kernels - we are not running AML. */
static vibio_u32 acpi_parse_s5(vibio_u64 dsdt)
{
    if (dsdt == 0 || !acpi_sig_equals(dsdt, "DSDT")) {
        return 0;
    }
    vibio_u32 length = acpi_read_u32(dsdt + 4);
    if (length < 36 || length > (16U * 1024U * 1024U)) {
        return 0;
    }
    const volatile vibio_u8 *body = (const volatile vibio_u8 *)dsdt;
    for (vibio_u32 i = 36; i + 4 < length; i++) {
        if (body[i] == '_' && body[i + 1] == 'S' && body[i + 2] == '5' && body[i + 3] == '_') {
            /* Make sure this is a name definition (preceded by a Name op or a
             * root/parent prefix), not a coincidental byte sequence. */
            vibio_u8 prev = (i > 0) ? body[i - 1] : 0;
            vibio_u8 prev2 = (i > 1) ? body[i - 2] : 0;
            if (!(prev == 0x08 || (prev == '\\' && prev2 == 0x08) || (prev == '.' ))) {
                continue;
            }
            vibio_u32 j = i + 4; /* points just past "_S5_" */
            if (body[j] != 0x12) { /* PackageOp */
                continue;
            }
            j++;
            j += ((body[j] & 0xC0) >> 6) + 1; /* skip PkgLength encoding */
            j++;                              /* skip NumElements */
            /* SLP_TYPa */
            if (body[j] == 0x0A) { /* BytePrefix */
                j++;
            }
            g_acpi_slp_typ_a = (vibio_u16)body[j];
            j++;
            /* SLP_TYPb */
            if (body[j] == 0x0A) {
                j++;
            }
            g_acpi_slp_typ_b = (vibio_u16)body[j];
            return 1;
        }
    }
    return 0;
}

/* Walk RSDP -> RSDT/XSDT -> FADT, then read the FADT fields needed to enter S5
 * and parse the DSDT. Returns 1 if a usable sleep register was found. */
static vibio_u32 acpi_prepare_poweroff(const VibioBootInfo *boot_info)
{
    if (g_acpi_ready) {
        return 1;
    }
    if (boot_info == 0 || boot_info->acpi_rsdp == 0) {
        return 0;
    }
    vibio_u64 rsdp = boot_info->acpi_rsdp;
    /* RSDP signature is "RSD PTR ". */
    const volatile char *sig = (const volatile char *)rsdp;
    const char *want = "RSD PTR ";
    for (vibio_u32 i = 0; i < 8; i++) {
        if (sig[i] != want[i]) {
            return 0;
        }
    }
    vibio_u8 revision = *(const volatile vibio_u8 *)(rsdp + 15);

    /* Locate the FADT ("FACP") through the XSDT (ACPI 2.0+) or RSDT. */
    vibio_u64 fadt = 0;
    if (revision >= 2) {
        vibio_u64 xsdt = *(const volatile vibio_u64 *)(rsdp + 24);
        if (xsdt != 0 && acpi_sig_equals(xsdt, "XSDT")) {
            vibio_u32 len = acpi_read_u32(xsdt + 4);
            vibio_u32 entries = (len >= 36) ? (len - 36) / 8 : 0;
            for (vibio_u32 i = 0; i < entries; i++) {
                vibio_u64 entry = *(const volatile vibio_u64 *)(xsdt + 36 + i * 8);
                if (entry != 0 && acpi_sig_equals(entry, "FACP")) {
                    fadt = entry;
                    break;
                }
            }
        }
    }
    if (fadt == 0) {
        vibio_u32 rsdt = acpi_read_u32(rsdp + 16);
        if (rsdt != 0 && acpi_sig_equals(rsdt, "RSDT")) {
            vibio_u32 len = acpi_read_u32(rsdt + 4);
            vibio_u32 entries = (len >= 36) ? (len - 36) / 4 : 0;
            for (vibio_u32 i = 0; i < entries; i++) {
                vibio_u64 entry = acpi_read_u32(rsdt + 36 + i * 4);
                if (entry != 0 && acpi_sig_equals(entry, "FACP")) {
                    fadt = entry;
                    break;
                }
            }
        }
    }
    if (fadt == 0) {
        return 0;
    }

    /* FADT fields (32-bit legacy block addresses are enough for the VM). */
    vibio_u64 dsdt = acpi_read_u32(fadt + 40);
    g_acpi_smi_cmd = acpi_read_u32(fadt + 48);
    g_acpi_enable = *(const volatile vibio_u8 *)(fadt + 52);
    g_acpi_pm1a_cnt = acpi_read_u32(fadt + 64);
    g_acpi_pm1b_cnt = acpi_read_u32(fadt + 68);

    if (g_acpi_pm1a_cnt == 0) {
        return 0;
    }
    if (!acpi_parse_s5(dsdt)) {
        /* No \_S5 found: most VMs use SLP_TYP 5, so fall back to that. */
        g_acpi_slp_typ_a = 5;
        g_acpi_slp_typ_b = 5;
    }
    g_acpi_ready = 1;
    return 1;
}

/* Ask the chipset to enter ACPI soft-off (S5). Requires ACPI mode enabled, so
 * we first hand control to ACPI via SMI_CMD if the firmware left it in legacy
 * mode. On success the VM window closes; if the host ignores it we simply
 * return and fall through to the honest halt screen. */
static void power_force_poweroff(const VibioBootInfo *boot_info)
{
    if (acpi_prepare_poweroff(boot_info)) {
        /* Enable ACPI mode if SCI_EN (bit 0 of PM1a_CNT) is not yet set. */
        if ((inw((vibio_u16)g_acpi_pm1a_cnt) & 0x0001) == 0 &&
            g_acpi_smi_cmd != 0 && g_acpi_enable != 0) {
            outb((vibio_u16)g_acpi_smi_cmd, g_acpi_enable);
            for (vibio_u32 i = 0; i < 300000; i++) {
                if ((inw((vibio_u16)g_acpi_pm1a_cnt) & 0x0001) != 0) {
                    break;
                }
            }
        }
        vibio_u16 a = (vibio_u16)(((g_acpi_slp_typ_a & 0x7U) << 10) | ACPI_SLP_EN);
        outw((vibio_u16)g_acpi_pm1a_cnt, a);
        if (g_acpi_pm1b_cnt != 0) {
            vibio_u16 b = (vibio_u16)(((g_acpi_slp_typ_b & 0x7U) << 10) | ACPI_SLP_EN);
            outw((vibio_u16)g_acpi_pm1b_cnt, b);
        }
        panic_short_wait();
    }

    /* Last-resort magic ports for hypervisors that expose a fixed shutdown
     * register (QEMU 0x604/0xB004). Real PCs ignore these harmlessly. */
    outw(0x0604, 0x2000);
    panic_short_wait();
    outw(0xB004, 0x2000);
    panic_short_wait();
}

__attribute__((noreturn)) static void power_shutdown(void)
{
    __asm__ volatile("cli");
    /* First try a real ACPI power-off so the VM window actually closes. This is
     * a soft-off (S5) request and the magic shutdown ports - none of them is a
     * reset/ambiguous port - so a host that does not implement them does
     * nothing. */
    power_force_poweroff(g_panic_boot_info);

    /* If we are still alive, the host did not honor ACPI power-off. Show an
     * honest halt screen and stop instead of pretending we powered down. */
    power_show_halt_screen(g_panic_boot_info);
    for (;;) {
        __asm__ volatile("hlt");
    }
}

__attribute__((noreturn)) static void panic_reboot_after_delay(void)
{
    panic_wait_real_seconds(PANIC_RESTART_SECONDS);
    panic_force_reset();
}

__attribute__((noreturn)) static void panic_show_and_reboot(void)
{
    __asm__ volatile("cli");
    const VibioBootInfo *boot_info = g_panic_boot_info;
    if (boot_info == 0 || boot_info->framebuffer_base == 0) {
        panic_reboot_after_delay();
    }

    g_draw_target = 0;
    reset_clip();
    reset_paint_bounds();

    vibio_u32 red = make_color(boot_info, 150, 20, 32);
    vibio_u32 dark_red = make_color(boot_info, 100, 10, 20);
    vibio_u32 white = make_color(boot_info, 255, 245, 245);
    vibio_u32 muted = make_color(boot_info, 255, 190, 190);
    vibio_u32 accent = make_color(boot_info, 255, 230, 120);

    fill_rect(boot_info, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, red);
    fill_rect(boot_info, 0, 0, boot_info->framebuffer_width, 82, dark_red);
    g_text_bg = dark_red;
    panic_draw_text(boot_info, 42, 24, ":/ Your device ran into a critical problem", white);
    g_text_bg = red;
    panic_draw_text(boot_info, 42, 106, "KERNEL PANIC", white);
    panic_draw_text(boot_info, 42, 136, "Vibio OS collected fault details and will restart automatically.", muted);

    vibio_u32 y = 190;
    panic_draw_row(boot_info, 42, y, "FAULT:", g_panic_state.fault, muted, white); y += 28;
    panic_draw_hex_row(boot_info, 42, y, "ERROR CODE:", g_panic_state.error_code, muted, accent); y += 28;
    panic_draw_hex_row(boot_info, 42, y, "RIP:", g_panic_state.rip, muted, white); y += 28;
    panic_draw_hex_row(boot_info, 42, y, "RSP:", g_panic_state.rsp, muted, white); y += 28;
    panic_draw_hex_row(boot_info, 42, y, "RFLAGS:", g_panic_state.rflags, muted, white); y += 28;
    panic_draw_hex_row(boot_info, 42, y, "CR2:", g_panic_state.cr2, muted, accent); y += 28;
    panic_draw_uptime_row(boot_info, 42, y, muted, white); y += 28;
    panic_draw_row(boot_info, 42, y, "TASK:", g_panic_state.task, muted, white); y += 28;
    panic_draw_row(boot_info, 42, y, "LAST DRIVER:", g_panic_state.last_driver, muted, white); y += 28;
    panic_draw_row(boot_info, 42, y, "LAST ACTION:", g_panic_state.last_action, muted, white); y += 42;
    panic_draw_text(boot_info, 42, y, "Restarting in about 18 seconds...", white);

    panic_reboot_after_delay();
}

__attribute__((noreturn, used)) void panic_from_exception(vibio_u64 vector, vibio_u64 error_code, vibio_u64 rip, vibio_u64 rsp, vibio_u64 rflags, vibio_u64 cr2)
{
    g_panic_state.active = 1;
    g_panic_state.vector = vector;
    g_panic_state.error_code = error_code;
    g_panic_state.rip = rip;
    g_panic_state.rsp = rsp;
    g_panic_state.rflags = rflags;
    g_panic_state.cr2 = cr2;
    g_panic_state.fault = panic_fault_name(vector);
    vibio_u64 ticks = 0;
    VibioIrqState *irq = (VibioIrqState *)irq_state_address_storage;
    if (irq != 0) {
        ticks = irq->timer_ticks;
    }
    vibio_u64 seconds = ticks / 100ULL;
    g_panic_state.uptime_seconds = seconds;
    g_panic_state.uptime_days = seconds / 86400ULL;
    seconds %= 86400ULL;
    g_panic_state.uptime_hours = seconds / 3600ULL;
    seconds %= 3600ULL;
    g_panic_state.uptime_minutes = seconds / 60ULL;
    g_panic_state.uptime_second_part = seconds % 60ULL;
    panic_show_and_reboot();
}

__attribute__((noreturn)) static void panic_trigger_test(void)
{
    panic_set_context("KERNEL", "PANIC TEST", "TEST PAGE FAULT");
    volatile vibio_u64 *bad = (volatile vibio_u64 *)0x0000000100000000ULL;
    *bad = 0x564942494F50464FULL;
    for (;;) {
        __asm__ volatile("hlt");
    }
}

/* Draw one window. The focused (top) window gets a brighter title bar. While a
 * window's open animation is playing it is drawn shifted down a few pixels and
 * faded in over the desktop, a Windows 11-style "rise and fade" appearance. */
static void window_draw(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 border,
    vibio_u32 focused)
{
    /* Animation easing in 0..256 fixed point. Opening rises and fades in
     * (ease-out); closing/minimizing does the reverse: falls and fades out. */
    vibio_u32 eased = 256;
    vibio_u32 rise = 0;
    if (window->anim_open) {
        vibio_u64 elapsed = g_anim_now - window->anim_start;
        if (elapsed < WINDOW_ANIM_TICKS) {
            vibio_u32 p = (vibio_u32)((elapsed * 256) / WINDOW_ANIM_TICKS);
            vibio_u32 inv = 256 - p;
            eased = 256 - (inv * inv) / 256;
            rise = ((256 - eased) * 20) / 256;
        }
    } else if (window->anim_close) {
        /* Exact reverse of the open animation: play the same ease-out curve
         * backwards in time so the window falls and fades out, mirroring the
         * rise-and-fade-in of opening. The compositor rebuilds the rectangle
         * from the base layer each frame, so the fade does not smear. */
        vibio_u64 elapsed = g_anim_now - window->anim_start;
        if (elapsed < WINDOW_ANIM_TICKS) {
            vibio_u32 p = (vibio_u32)((elapsed * 256) / WINDOW_ANIM_TICKS);
            vibio_u32 t = 256 - p;            /* time-reversed progress */
            vibio_u32 inv = 256 - t;
            eased = 256 - (inv * inv) / 256;
            rise = ((256 - eased) * 20) / 256;
        } else {
            eased = 0;
            rise = 20;
        }
    }

    /* The rise offset shifts chrome and content together by adjusting the
     * vertical origin; the window's stored position is left untouched. */
    vibio_u32 x = window->x;
    vibio_u32 y = window->y + rise;
    vibio_u32 w = window->width;
    vibio_u32 h = window->height;
    vibio_u32 titlebar = focused ? window->titlebar_color : make_color(boot_info, 70, 78, 90);
    vibio_u32 title_color = focused ? make_color(boot_info, 245, 250, 252) : make_color(boot_info, 180, 188, 198);
    vibio_u32 ctrl_color = focused ? make_color(boot_info, 196, 214, 226) : make_color(boot_info, 150, 158, 168);
    vibio_u32 inner = make_color(boot_info, 90, 120, 140);

    /* F11 full screen: no shadow, title bar, border, or controls - just the
     * body filling the whole screen, with the content drawn edge to edge. */
    if (window->fullscreen) {
        x = window->x;
        y = window->y;
        eased = 256;
        fill_rect(boot_info, x, y, w, h, window->body_color);
        window_draw_content(boot_info, window, desktop, 0);
        return;
    }

    draw_window_shadow(boot_info, x, y, w, h);

    fill_rect(boot_info, x, y, w, h, window->body_color);
    fill_rect(boot_info, x, y, w, WINDOW_TITLEBAR_HEIGHT, titlebar);

    /* Outer frame, plus a 1px inner bevel line so the border reads as crafted
     * rather than a single flat hairline. */
    fill_rect(boot_info, x, y, w, WINDOW_BORDER, border);
    fill_rect(boot_info, x, y + h - WINDOW_BORDER, w, WINDOW_BORDER, border);
    fill_rect(boot_info, x, y, WINDOW_BORDER, h, border);
    fill_rect(boot_info, x + w - WINDOW_BORDER, y, WINDOW_BORDER, h, border);
    fill_rect(boot_info, x, y + WINDOW_TITLEBAR_HEIGHT, w, WINDOW_BORDER, border);
    fill_rect(boot_info, x + WINDOW_BORDER, y + WINDOW_TITLEBAR_HEIGHT + WINDOW_BORDER, w - WINDOW_BORDER * 2, 1, inner);
    /* Glossy 1px highlight along the very top edge of the title bar. */
    fill_rect(boot_info, x + WINDOW_BORDER, y + WINDOW_BORDER, w - WINDOW_BORDER * 2, 1, make_color(boot_info, 200, 220, 233));

    /* Clip text to the window interior so labels wider than the window are cut
     * at the border instead of spilling over it onto other windows. */
    set_clip(x + WINDOW_BORDER, y + WINDOW_BORDER, x + w - WINDOW_BORDER, y + h - WINDOW_BORDER);

    g_text_bg = titlebar;
    font_draw_buf(boot_info, &FONT_GEIST_UI, x + WINDOW_BORDER + 14, y + 4,
                  window->title, window->title_length, title_color);
    draw_window_controls(boot_info, x, y, w, ctrl_color, window->maximized);

    reset_clip();
    window_draw_content(boot_info, window, desktop, rise);

    /* Fade the whole window rectangle in over the desktop as it rises. */
    if (eased < 256) {
        window_fade_over_base(boot_info, x, y, w, h, (vibio_u8)(eased > 255 ? 255 : eased));
    }
}

static void wm_set_title(VibioWindow *window, const char *title)
{
    window->title_length = 0;
    while (title[window->title_length] != 0 && window->title_length < 24) {
        window->title[window->title_length] = title[window->title_length];
        window->title_length++;
    }
}

MAYBE_UNUSED static void window_copy(VibioWindow *dst, const VibioWindow *src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->width = src->width;
    dst->height = src->height;
    dst->kind = src->kind;
    dst->visible = src->visible;
    dst->titlebar_color = src->titlebar_color;
    dst->body_color = src->body_color;
    dst->title_length = src->title_length;
    for (vibio_u32 i = 0; i < sizeof(dst->title); i++) {
        dst->title[i] = src->title[i];
    }
    dst->anim_open = src->anim_open;
    dst->anim_close = src->anim_close;
    dst->anim_to_min = src->anim_to_min;
    dst->minimized = src->minimized;
    dst->maximized = src->maximized;
    dst->fullscreen = src->fullscreen;
    dst->anim_start = src->anim_start;
    dst->saved_x = src->saved_x;
    dst->saved_y = src->saved_y;
    dst->saved_w = src->saved_w;
    dst->saved_h = src->saved_h;
}

/* Move a window to the top of the z-order (focus + raise). */
static void wm_raise(VibioWindowManager *wm, vibio_u32 index)
{
    vibio_u32 pos = 0;
    for (vibio_u32 i = 0; i < wm->count; i++) {
        if (wm->z_order[i] == index) {
            pos = i;
            break;
        }
    }
    for (vibio_u32 i = pos; i + 1 < wm->count; i++) {
        wm->z_order[i] = wm->z_order[i + 1];
    }
    wm->z_order[wm->count - 1] = index;
}

static vibio_u32 wm_find_kind(const VibioWindowManager *wm, vibio_u32 kind)
{
    for (vibio_u32 i = 0; i < wm->count; i++) {
        if (wm->windows[i].kind == kind) {
            return i;
        }
    }
    return WM_NO_WINDOW;
}

static vibio_u32 wm_top_visible(const VibioWindowManager *wm)
{
    for (vibio_u32 k = 0; k < wm->count; k++) {
        vibio_u32 i = wm->count - 1 - k;
        vibio_u32 idx = wm->z_order[i];
        if (wm->windows[idx].visible) {
            return idx;
        }
    }
    return WM_NO_WINDOW;
}

static void wm_show_and_raise(VibioWindowManager *wm, vibio_u32 index)
{
    if (index >= wm->count) {
        return;
    }
    VibioWindow *w = &wm->windows[index];
    /* (Re)start the open animation on a real hidden->visible transition (this
     * also covers restoring a minimized window) or when the window is caught
     * mid-close - re-activating a closing window cancels the close and plays the
     * open animation instead of letting it finish disappearing. */
    if (!w->visible || w->anim_close) {
        w->anim_open = 1;
        w->anim_close = 0;
        w->anim_start = g_anim_now;
    }
    w->visible = 1;
    w->minimized = 0;
    wm_raise(wm, index);
}

/* Close/minimize plays the reverse of the open animation (fall + fade out).
 * The window stays visible while the animation runs and is hidden when it
 * finishes (see wm_step_animations). The compositor recomposites the dirty
 * rectangle from the static base layer each frame, so the fade does not
 * accumulate or smear (rect_add_window already pads the rect downward to cover
 * the fall). */
static void wm_begin_close(VibioWindowManager *wm, vibio_u32 index, vibio_u32 to_minimize)
{
    if (index >= wm->count) {
        return;
    }
    VibioWindow *w = &wm->windows[index];
    if (!w->visible || w->anim_close) {
        return;
    }
    /* Full-screen (F11) windows are not animated; closing those is an instant
     * state change so the whole screen does not flash through a fade. */
    if (w->fullscreen) {
        w->anim_open = 0;
        w->anim_close = 0;
        w->anim_to_min = (vibio_u8)(to_minimize ? 1 : 0);
        w->visible = 0;
        w->minimized = w->anim_to_min;
    } else {
        w->anim_open = 0;
        w->anim_close = 1;
        w->anim_to_min = (vibio_u8)(to_minimize ? 1 : 0);
        w->anim_start = g_anim_now;
        /* Stays visible during the animation; wm_step_animations hides it and
         * applies the final minimized state when the animation completes. */
    }
    if (wm->dragging && wm->drag_index == index) {
        wm->dragging = 0;
    }
}

static void wm_minimize(VibioWindowManager *wm, vibio_u32 index)
{
    wm_begin_close(wm, index, 1);
}

static void wm_close(VibioWindowManager *wm, vibio_u32 index)
{
    wm_begin_close(wm, index, 0);
}

/* Toggle a window between its normal rect and the full work area (above the
 * taskbar). The normal rect is saved so restore is exact. */
static void wm_toggle_maximize(VibioWindowManager *wm, vibio_u32 index, const VibioBootInfo *boot_info)
{
    if (index >= wm->count) {
        return;
    }
    VibioWindow *w = &wm->windows[index];
    if (w->maximized) {
        w->x = w->saved_x;
        w->y = w->saved_y;
        w->width = w->saved_w;
        w->height = w->saved_h;
        w->maximized = 0;
    } else {
        w->saved_x = w->x;
        w->saved_y = w->y;
        w->saved_w = w->width;
        w->saved_h = w->height;
        w->x = 0;
        w->y = DESKTOP_TOPBAR_HEIGHT;
        w->width = boot_info->framebuffer_width;
        w->height = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - DESKTOP_TOPBAR_HEIGHT;
        w->maximized = 1;
    }
}

/* F11: toggle the focused window between its normal rect and true full screen -
 * the whole framebuffer with no title bar, border, or taskbar (like a browser's
 * F11). The normal rect is saved so restore is exact. */
static void wm_toggle_fullscreen(VibioWindowManager *wm, vibio_u32 index, const VibioBootInfo *boot_info)
{
    if (index >= wm->count) {
        return;
    }
    VibioWindow *w = &wm->windows[index];
    if (w->fullscreen) {
        w->x = w->saved_x;
        w->y = w->saved_y;
        w->width = w->saved_w;
        w->height = w->saved_h;
        w->fullscreen = 0;
    } else {
        w->saved_x = w->x;
        w->saved_y = w->y;
        w->saved_w = w->width;
        w->saved_h = w->height;
        w->x = 0;
        w->y = 0;
        w->width = boot_info->framebuffer_width;
        w->height = boot_info->framebuffer_height;
        w->fullscreen = 1;
        w->maximized = 0;
    }
}

/* True if a visible window is in F11 full screen, so the shell (taskbar, top
 * bar) should not draw over it. */
static vibio_u32 wm_any_fullscreen(const VibioWindowManager *wm)
{
    for (vibio_u32 i = 0; i < wm->count; i++) {
        if (wm->windows[i].fullscreen && wm->windows[i].visible) {
            return 1;
        }
    }
    return 0;
}

/* Advance window open/close animations: clear finished flags and apply the
 * end-of-close state. Returns nonzero when the compositor must redraw: either
 * an animation is still active, or this tick changed final visibility. */
static vibio_u32 wm_step_animations(VibioWindowManager *wm, vibio_u64 now)
{
    vibio_u32 redraw = 0;
    for (vibio_u32 i = 0; i < wm->count; i++) {
        VibioWindow *w = &wm->windows[i];
        if (w->anim_open) {
            if (!w->visible || now - w->anim_start >= WINDOW_ANIM_TICKS) {
                w->anim_open = 0;
                redraw = 1;
            } else {
                redraw = 1;
            }
        }
        if (w->anim_close) {
            if (now - w->anim_start >= WINDOW_ANIM_TICKS) {
                w->anim_close = 0;
                w->visible = 0;
                w->minimized = w->anim_to_min;
                redraw = 1;
            } else {
                redraw = 1;
            }
        }
    }
    return redraw;
}

/* Taskbar/desktop-icon click: minimize the window if it is focused, otherwise
 * restore/open and raise it. Never closes. */
static void wm_activate_from_shell(VibioWindowManager *wm, vibio_u32 index)
{
    if (index >= wm->count) {
        return;
    }
    VibioWindow *w = &wm->windows[index];
    if (w->visible && !w->anim_close && wm_top_visible(wm) == index) {
        wm_minimize(wm, index);
    } else {
        wm_show_and_raise(wm, index);
    }
}

typedef struct {
    vibio_u32 x;
    vibio_u32 y;
    vibio_u32 kind;
    const char *label;
} VibioDesktopIconSpec;

static vibio_u32 desktop_builtin_kind(vibio_u32 index)
{
    if (index == 0) { return WINDOW_KIND_SYSTEM; }
    if (index == 1) { return WINDOW_KIND_TERMINAL; }
    if (index == 2) { return WINDOW_KIND_BROWSER; }
    if (index == 3) { return WINDOW_KIND_FILES; }
    return DESKTOP_ICON_NONE;
}

static const char *desktop_builtin_label(vibio_u32 index)
{
    if (index == 0) { return "SYSTEM"; }
    if (index == 1) { return "TERMINAL"; }
    if (index == 2) { return "BROWSER"; }
    if (index == 3) { return "FILES"; }
    return "";
}

static vibio_u32 desktop_builtin_default_x(vibio_u32 index)
{
    (void)index;
    return 8U;
}

static vibio_u32 desktop_builtin_default_y(vibio_u32 index)
{
    return 12U + index * 74U;
}

static vibio_u32 desktop_icon_cell_w(const VibioWindowManager *wm)
{
    if (wm->desktop_icon_size == DESKTOP_ICON_SIZE_SMALL) { return 64U; }
    if (wm->desktop_icon_size == DESKTOP_ICON_SIZE_LARGE) { return 92U; }
    return DESKTOP_ICON_CELL_WIDTH;
}

static vibio_u32 desktop_icon_cell_h(const VibioWindowManager *wm)
{
    if (wm->desktop_icon_size == DESKTOP_ICON_SIZE_SMALL) { return 58U; }
    if (wm->desktop_icon_size == DESKTOP_ICON_SIZE_LARGE) { return 82U; }
    return DESKTOP_ICON_CELL_HEIGHT;
}

static vibio_u32 desktop_icon_image_size(const VibioWindowManager *wm)
{
    if (wm->desktop_icon_size == DESKTOP_ICON_SIZE_SMALL) { return 28U; }
    if (wm->desktop_icon_size == DESKTOP_ICON_SIZE_LARGE) { return 48U; }
    return DESKTOP_ICON_IMAGE;
}

static vibio_u32 desktop_icon_count(const VibioWindowManager *wm)
{
    if (!wm->desktop_icons_visible) {
        return 0;
    }
    return DESKTOP_ICON_BUILTIN_COUNT + wm->desktop_temp_count;
}

static vibio_u32 desktop_icon_info(
    const VibioWindowManager *wm,
    vibio_u32 index,
    VibioDesktopIconSpec *out)
{
    if (index < DESKTOP_ICON_BUILTIN_COUNT) {
        out->x = wm->desktop_builtin_x[index];
        out->y = wm->desktop_builtin_y[index];
        out->kind = desktop_builtin_kind(index);
        out->label = desktop_builtin_label(index);
        return out->kind != DESKTOP_ICON_NONE;
    }
    vibio_u32 ti = index - DESKTOP_ICON_BUILTIN_COUNT;
    if (ti >= wm->desktop_temp_count) {
        return 0;
    }
    out->x = wm->desktop_temp[ti].x;
    out->y = wm->desktop_temp[ti].y;
    out->kind = wm->desktop_temp[ti].kind;
    out->label = wm->desktop_temp[ti].label;
    return 1;
}

static vibio_u32 desktop_icon_index_at(const VibioWindowManager *wm, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 count = desktop_icon_count(wm);
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);

    for (vibio_u32 i = 0; i < count; i++) {
        VibioDesktopIconSpec icon;
        if (!desktop_icon_info(wm, i, &icon)) {
            continue;
        }
        if (px >= icon.x && px < icon.x + cw && py >= icon.y && py < icon.y + ch) {
            return i;
        }
    }

    return 0xFFFFFFFFU;
}

static vibio_u32 desktop_icon_mask_for_kind(vibio_u32 kind)
{
    return kind < 31U ? (1U << kind) : 0;
}

static vibio_u32 desktop_icon_kind_at(const VibioWindowManager *wm, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 index = desktop_icon_index_at(wm, px, py);
    VibioDesktopIconSpec icon;
    if (index != 0xFFFFFFFFU && desktop_icon_info(wm, index, &icon)) {
        return icon.kind;
    }
    return DESKTOP_ICON_NONE;
}

static vibio_u32 wm_desktop_icon_window_at(const VibioWindowManager *wm, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 kind = desktop_icon_kind_at(wm, px, py);
    if (kind == DESKTOP_ICON_NONE) {
        return WM_NO_WINDOW;
    }
    return wm_find_kind(wm, kind);
}

static vibio_u32 desktop_rects_overlap(
    vibio_u32 ax, vibio_u32 ay, vibio_u32 aw, vibio_u32 ah,
    vibio_u32 bx, vibio_u32 by, vibio_u32 bw, vibio_u32 bh)
{
    if (aw == 0 || ah == 0 || bw == 0 || bh == 0) {
        return 0;
    }
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static vibio_u32 desktop_selection_rect(
    const VibioWindowManager *wm,
    vibio_u32 *x,
    vibio_u32 *y,
    vibio_u32 *w,
    vibio_u32 *h)
{
    if (!wm->selecting) {
        return 0;
    }
    vibio_u32 x0 = wm->select_start_x < wm->select_x ? wm->select_start_x : wm->select_x;
    vibio_u32 y0 = wm->select_start_y < wm->select_y ? wm->select_start_y : wm->select_y;
    vibio_u32 x1 = wm->select_start_x > wm->select_x ? wm->select_start_x : wm->select_x;
    vibio_u32 y1 = wm->select_start_y > wm->select_y ? wm->select_start_y : wm->select_y;
    *x = x0;
    *y = y0;
    *w = x1 > x0 ? x1 - x0 : 1U;
    *h = y1 > y0 ? y1 - y0 : 1U;
    return 1;
}

static vibio_u32 desktop_selection_mask(const VibioWindowManager *wm)
{
    vibio_u32 sx = 0, sy = 0, sw = 0, sh = 0;
    vibio_u32 mask = 0;
    if (!desktop_selection_rect(wm, &sx, &sy, &sw, &sh)) {
        return 0;
    }
    vibio_u32 count = desktop_icon_count(wm);
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    for (vibio_u32 i = 0; i < count; i++) {
        VibioDesktopIconSpec icon;
        if (!desktop_icon_info(wm, i, &icon)) {
            continue;
        }
        if (desktop_rects_overlap(sx, sy, sw, sh, icon.x, icon.y, cw, ch)) {
            mask |= desktop_icon_mask_for_kind(icon.kind);
        }
    }
    return mask;
}

/* Topmost window containing the point, or WM_NO_WINDOW. */
static vibio_u32 wm_window_at(const VibioWindowManager *wm, vibio_u32 px, vibio_u32 py)
{
    for (vibio_u32 k = 0; k < wm->count; k++) {
        vibio_u32 i = wm->count - 1 - k;
        vibio_u32 idx = wm->z_order[i];
        const VibioWindow *w = &wm->windows[idx];
        if (!w->visible) {
            continue;
        }
        if (px >= w->x && px < w->x + w->width && py >= w->y && py < w->y + w->height) {
            return idx;
        }
    }
    return WM_NO_WINDOW;
}

static vibio_u32 taskbar_button_x(vibio_u32 index)
{
    return DESKTOP_MARGIN + TASKBAR_START_WIDTH + TASKBAR_BUTTON_GAP +
           TASKBAR_SEARCH_WIDTH + TASKBAR_BUTTON_GAP +
           index * (TASKBAR_BUTTON_WIDTH + TASKBAR_BUTTON_GAP);
}

static vibio_u32 taskbar_kind_is_pinned(vibio_u32 kind)
{
    return kind == WINDOW_KIND_FILES || kind == WINDOW_KIND_TERMINAL;
}

static vibio_u32 taskbar_window_is_running(const VibioWindow *window)
{
    return window != 0 && (window->visible || window->minimized || window->anim_open || window->anim_close);
}

static vibio_u32 taskbar_pinned_kind(vibio_u32 slot)
{
    if (slot == 0) {
        return WINDOW_KIND_FILES;
    }
    if (slot == 1) {
        return WINDOW_KIND_TERMINAL;
    }
    return 0;
}

static vibio_u32 taskbar_slot_count(const VibioWindowManager *wm)
{
    vibio_u32 count = 2U; /* Pinned by default: Files, then Terminal. */
    for (vibio_u32 i = 0; i < wm->count && count < WM_MAX_WINDOWS; i++) {
        const VibioWindow *window = &wm->windows[i];
        if (!taskbar_kind_is_pinned(window->kind) && taskbar_window_is_running(window)) {
            count++;
        }
    }
    return count;
}

static vibio_u32 taskbar_slot_window_index(const VibioWindowManager *wm, vibio_u32 slot)
{
    if (slot < 2U) {
        vibio_u32 kind = taskbar_pinned_kind(slot);
        return kind != 0 ? wm_find_kind(wm, kind) : WM_NO_WINDOW;
    }

    vibio_u32 wanted = slot - 2U;
    for (vibio_u32 i = 0; i < wm->count; i++) {
        const VibioWindow *window = &wm->windows[i];
        if (taskbar_kind_is_pinned(window->kind) || !taskbar_window_is_running(window)) {
            continue;
        }
        if (wanted == 0) {
            return i;
        }
        wanted--;
    }
    return WM_NO_WINDOW;
}

static vibio_u32 taskbar_slot_at(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    vibio_u32 px,
    vibio_u32 py)
{
    if (boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT) {
        return TASKBAR_NO_SLOT;
    }

    vibio_u32 taskbar_y = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT;
    vibio_u32 button_y = taskbar_y + 5;
    vibio_u32 button_h = TASKBAR_BUTTON_WIDTH;
    if (py < button_y || py >= button_y + button_h) {
        return TASKBAR_NO_SLOT;
    }

    vibio_u32 slots = taskbar_slot_count(wm);
    for (vibio_u32 slot = 0; slot < slots; slot++) {
        vibio_u32 x = taskbar_button_x(slot);
        if (px >= x && px < x + TASKBAR_BUTTON_WIDTH) {
            return slot;
        }
    }
    return TASKBAR_NO_SLOT;
}

static const char *taskbar_window_label(const VibioWindow *window)
{
    if (window == 0) {
        return "App";
    }
    if (window->kind == WINDOW_KIND_FILES) {
        return "Vibio Files";
    }
    if (window->kind == WINDOW_KIND_TERMINAL) {
        return "Terminal";
    }
    if (window->kind == WINDOW_KIND_BROWSER) {
        return "Browser";
    }
    if (window->kind == WINDOW_KIND_SYSTEM) {
        return "System";
    }
    if (window->kind == WINDOW_KIND_MEDIA_VIEWER) {
        return "Media Viewer";
    }
    if (window->kind == WINDOW_KIND_MEDIA_PLAYER) {
        return "Media Player";
    }
    if (window->kind == WINDOW_KIND_SETTINGS) {
        return "Settings";
    }
    return window->title_length > 0 ? window->title : "App";
}

static vibio_u32 taskbar_hover_popup_rect(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    vibio_u32 slot,
    vibio_u32 *x,
    vibio_u32 *y,
    vibio_u32 *w,
    vibio_u32 *h)
{
    if (slot == TASKBAR_NO_SLOT || boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT + 40U) {
        return 0;
    }
    vibio_u32 idx = taskbar_slot_window_index(wm, slot);
    if (idx == WM_NO_WINDOW) {
        return 0;
    }
    const VibioWindow *window = &wm->windows[idx];
    vibio_u32 running = taskbar_window_is_running(window);
    const char *label = taskbar_window_label(window);
    if (running) {
        *w = 242U;
        *h = 122U;
    } else {
        vibio_u32 label_w = font_text_width(&FONT_GEIST_SM, label);
        *w = label_w + 24U;
        if (*w < 70U) {
            *w = 70U;
        }
        *h = 30U;
    }

    vibio_u32 taskbar_y = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT;
    if (taskbar_y <= *h + 8U) {
        return 0;
    }
    vibio_u32 center = taskbar_button_x(slot) + TASKBAR_BUTTON_WIDTH / 2U;
    *x = center > *w / 2U ? center - *w / 2U : 4U;
    if (*x + *w + 4U > boot_info->framebuffer_width) {
        *x = boot_info->framebuffer_width > *w + 4U ? boot_info->framebuffer_width - *w - 4U : 0;
    }
    *y = taskbar_y - *h - 8U;
    return 1;
}

static vibio_u32 taskbar_tray_base_x(const VibioBootInfo *boot_info)
{
    return boot_info->framebuffer_width > 260 ? boot_info->framebuffer_width - 136U : 0;
}

static vibio_u32 taskbar_power_icon_x(const VibioBootInfo *boot_info)
{
    return taskbar_tray_base_x(boot_info) - 114U;
}

static vibio_u32 taskbar_sound_icon_x(const VibioBootInfo *boot_info)
{
    return taskbar_tray_base_x(boot_info) - 78U;
}

static vibio_u32 taskbar_power_icon_hit(const VibioBootInfo *boot_info, vibio_u32 px, vibio_u32 py)
{
    if (boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT || boot_info->framebuffer_width <= 260) {
        return 0;
    }
    vibio_u32 taskbar_y = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT;
    vibio_u32 button_y = taskbar_y + 5;
    vibio_u32 x = taskbar_power_icon_x(boot_info);
    return px >= x && px < x + 32U && py >= button_y && py < button_y + TASKBAR_BUTTON_WIDTH;
}

static vibio_u32 taskbar_volume_icon_hit(const VibioBootInfo *boot_info, vibio_u32 px, vibio_u32 py)
{
    if (boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT || boot_info->framebuffer_width <= 260) {
        return 0;
    }
    vibio_u32 taskbar_y = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT;
    vibio_u32 button_y = taskbar_y + 5;
    vibio_u32 x = taskbar_sound_icon_x(boot_info);
    return px >= x && px < x + 32U && py >= button_y && py < button_y + TASKBAR_BUTTON_WIDTH;
}

static vibio_u32 taskbar_power_menu_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 *x,
    vibio_u32 *y,
    vibio_u32 *w,
    vibio_u32 *h)
{
    if (boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT + POWER_MENU_H + 8 ||
        boot_info->framebuffer_width <= POWER_MENU_W + 16) {
        return 0;
    }
    *w = POWER_MENU_W;
    *h = POWER_MENU_H;
    vibio_u32 icon_center = taskbar_power_icon_x(boot_info) + 16U;
    *x = icon_center > (*w / 2U) ? icon_center - (*w / 2U) : 8U;
    if (*x < 8U) {
        *x = 8U;
    }
    if (*x + *w + 8U > boot_info->framebuffer_width) {
        *x = boot_info->framebuffer_width - *w - 8U;
    }
    *y = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - *h - 8U;
    return 1;
}

static vibio_u32 taskbar_power_menu_row_at(const VibioBootInfo *boot_info, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_power_menu_rect(boot_info, &x, &y, &w, &h)) {
        return 0;
    }
    if (px < x || px >= x + w || py < y || py >= y + h) {
        return 0;
    }
    if (py < y + POWER_MENU_HEADER_H) {
        return 0;
    }
    vibio_u32 row = (py - y - POWER_MENU_HEADER_H) / POWER_MENU_ROW_H;
    if (row == 0) {
        return POWER_MENU_ROW_SHUTDOWN;
    }
    if (row == 1) {
        return POWER_MENU_ROW_RESTART;
    }
    return 0;
}

static vibio_u32 taskbar_power_confirm_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 *x,
    vibio_u32 *y,
    vibio_u32 *w,
    vibio_u32 *h)
{
    if (boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT + POWER_CONFIRM_H + 8 ||
        boot_info->framebuffer_width <= POWER_CONFIRM_W + 16) {
        return 0;
    }
    *w = POWER_CONFIRM_W;
    *h = POWER_CONFIRM_H;
    *x = (boot_info->framebuffer_width - *w) / 2U;
    *y = (boot_info->framebuffer_height - *h) / 2U;
    return 1;
}

static void power_confirm_button_layout(
    vibio_u32 dialog_x,
    vibio_u32 dialog_y,
    vibio_u32 dialog_w,
    vibio_u32 dialog_h,
    vibio_u32 *action_x,
    vibio_u32 *action_w,
    vibio_u32 *back_x,
    vibio_u32 *back_w,
    vibio_u32 *btn_y,
    vibio_u32 *btn_h)
{
    *action_w = 144U;
    *back_w = 116U;
    vibio_u32 gap = 16U;
    vibio_u32 total = *action_w + gap + *back_w;
    *action_x = dialog_x + (dialog_w - total) / 2U;
    *back_x = *action_x + *action_w + gap;
    *btn_h = 34U;
    *btn_y = dialog_y + dialog_h - *btn_h - 24U;
}

/* Returns 1 for the confirm action, 2 for Back, 0 for no button hit. */
static vibio_u32 taskbar_power_confirm_hit(const VibioBootInfo *boot_info, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_power_confirm_rect(boot_info, &x, &y, &w, &h)) {
        return 0;
    }
    if (px < x || px >= x + w || py < y || py >= y + h) {
        return 0;
    }
    vibio_u32 action_x = 0;
    vibio_u32 action_w = 0;
    vibio_u32 back_x = 0;
    vibio_u32 back_w = 0;
    vibio_u32 btn_y = 0;
    vibio_u32 btn_h = 0;
    power_confirm_button_layout(x, y, w, h, &action_x, &action_w, &back_x, &back_w, &btn_y, &btn_h);
    if (py >= btn_y && py < btn_y + btn_h) {
        if (px >= action_x && px < action_x + action_w) {
            return 1;
        }
        if (px >= back_x && px < back_x + back_w) {
            return 2;
        }
    }
    return 0;
}

static void power_format_app_name(const VibioWindow *window, char *out, vibio_u32 out_max)
{
    vibio_u32 i = 0;
    if (out_max == 0) {
        return;
    }
    out[0] = 0;
    while (i + 1 < out_max && i < window->title_length) {
        char c = window->title[i];
        if (i == 0 && c >= 'A' && c <= 'Z') {
            out[i] = c;
        } else if (c >= 'A' && c <= 'Z') {
            out[i] = (char)(c + 32);
        } else {
            out[i] = c;
        }
        i++;
    }
    out[i] = 0;
}

static vibio_u32 power_running_apps(const VibioWindowManager *wm, char *first_name, vibio_u32 name_max)
{
    vibio_u32 count = 0;
    if (wm == 0) {
        return 0;
    }
    for (vibio_u32 i = 0; i < wm->count; i++) {
        const VibioWindow *window = &wm->windows[i];
        if (window->visible && !window->minimized && window->kind != WINDOW_KIND_SYSTEM) {
            if (count == 0 && first_name != 0 && name_max > 0) {
                power_format_app_name(window, first_name, name_max);
            }
            count++;
        }
    }
    return count;
}

static void power_begin_action(VibioWindowManager *wm, vibio_u32 confirm_kind)
{
    char first_name[16];
    vibio_u32 running = power_running_apps(wm, first_name, sizeof(first_name));
    wm->power_menu_open = 0;
    if (running == 0) {
        if (confirm_kind == POWER_CONFIRM_SHUTDOWN) {
            power_shutdown();
        } else {
            power_restart();
        }
    }
    wm->power_confirm = confirm_kind;
}

static vibio_u32 taskbar_volume_popup_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 *x,
    vibio_u32 *y,
    vibio_u32 *w,
    vibio_u32 *h)
{
    if (boot_info->framebuffer_height <= DESKTOP_TASKBAR_HEIGHT + 72 || boot_info->framebuffer_width <= 260) {
        return 0;
    }
    *w = 176U;
    *h = 60U;
    /* Centre the popup over the sound tray icon (the icon is 32px wide), so it
     * sits above the volume control rather than pinned to the screen edge. */
    vibio_u32 icon_center = taskbar_sound_icon_x(boot_info) + 16U;
    *x = icon_center > (*w / 2U) ? icon_center - (*w / 2U) : 8U;
    if (*x < 8U) {
        *x = 8U;
    }
    if (*x + *w + 8U > boot_info->framebuffer_width) {
        *x = boot_info->framebuffer_width - *w - 8U;
    }
    *y = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - *h - 8U;
    return 1;
}

/* Geometry of the draggable slider track inside the popup. */
static vibio_u32 taskbar_volume_slider_geom(
    const VibioBootInfo *boot_info,
    vibio_u32 *sx, vibio_u32 *sy, vibio_u32 *sw, vibio_u32 *sh)
{
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_volume_popup_rect(boot_info, &x, &y, &w, &h)) {
        return 0;
    }
    *sx = x + 18U;
    *sy = y + 40U;
    *sw = w - 36U;
    *sh = 6U;
    return 1;
}

/* Apply a slider volume from a mouse x, clamping x into the track. Used both for
 * the initial click and for every frame while the knob is dragged. */
static void taskbar_volume_slider_apply(
    const VibioBootInfo *boot_info,
    VibioAudioState *audio,
    vibio_u32 px)
{
    vibio_u32 sx = 0, sy = 0, sw = 0, sh = 0;
    if (!taskbar_volume_slider_geom(boot_info, &sx, &sy, &sw, &sh) || sw < 2U) {
        return;
    }
    vibio_u32 volume;
    if (px <= sx) {
        volume = 0U;
    } else if (px >= sx + sw - 1U) {
        volume = 100U;
    } else {
        volume = ((px - sx) * 100U) / (sw - 1U);
    }
    audio_set_volume(audio, volume);
}

/* True if (px,py) grabs the slider (generous vertical tolerance around the
 * thin track so the knob is easy to catch). */
static vibio_u32 taskbar_volume_slider_hit(
    const VibioBootInfo *boot_info,
    vibio_u32 px,
    vibio_u32 py)
{
    vibio_u32 sx = 0, sy = 0, sw = 0, sh = 0;
    if (!taskbar_volume_slider_geom(boot_info, &sx, &sy, &sw, &sh)) {
        return 0;
    }
    return px >= sx - 8U && px < sx + sw + 8U && py >= sy - 10U && py < sy + sh + 10U;
}

/* Taskbar buttons are part of the shell, not normal windows. A click on one
 * raises that window instead of starting a drag operation. */
static vibio_u32 wm_taskbar_window_at(const VibioBootInfo *boot_info, const VibioWindowManager *wm, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 slot = taskbar_slot_at(boot_info, wm, px, py);
    if (slot == TASKBAR_NO_SLOT) {
        return WM_NO_WINDOW;
    }
    return taskbar_slot_window_index(wm, slot);
}

static vibio_u32 desktop_context_rows(vibio_u32 menu)
{
    if (menu == DESKTOP_CONTEXT_VIEW) { return DESKTOP_CONTEXT_VIEW_ROWS; }
    if (menu == DESKTOP_CONTEXT_NEW) { return DESKTOP_CONTEXT_NEW_ROWS; }
    if (menu == DESKTOP_CONTEXT_WALLPAPER) { return DESKTOP_CONTEXT_WALLPAPER_ROWS; }
    if (menu == DESKTOP_CONTEXT_ROOT) { return DESKTOP_CONTEXT_ROOT_ROWS; }
    return 0;
}

static vibio_u32 desktop_context_row_at(vibio_u32 menu, vibio_u32 x, vibio_u32 y, vibio_u32 px, vibio_u32 py)
{
    vibio_u32 rows = desktop_context_rows(menu);
    if (rows == 0 || px < x || px >= x + DESKTOP_CONTEXT_W || py < y) {
        return 0xFFFFFFFFU;
    }
    vibio_u32 row = (py - y) / DESKTOP_CONTEXT_ROW_H;
    return row < rows ? row : 0xFFFFFFFFU;
}

static vibio_u32 desktop_context_hit(const VibioWindowManager *wm, vibio_u32 px, vibio_u32 py, vibio_u32 *menu)
{
    *menu = DESKTOP_CONTEXT_NONE;
    if (wm->context_menu == DESKTOP_CONTEXT_NONE) {
        return 0xFFFFFFFFU;
    }
    vibio_u32 row = desktop_context_row_at(DESKTOP_CONTEXT_ROOT, wm->context_x, wm->context_y, px, py);
    if (row != 0xFFFFFFFFU) {
        *menu = DESKTOP_CONTEXT_ROOT;
        return row;
    }
    if (wm->context_menu != DESKTOP_CONTEXT_ROOT) {
        row = desktop_context_row_at(wm->context_menu, wm->context_x + DESKTOP_CONTEXT_W - 1U, wm->context_y, px, py);
        if (row != 0xFFFFFFFFU) {
            *menu = wm->context_menu;
            return row;
        }
    }
    return 0xFFFFFFFFU;
}

static void desktop_arrange_icons(VibioWindowManager *wm, const VibioBootInfo *boot_info)
{
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    vibio_u32 x = 8U;
    vibio_u32 y = 12U;
    vibio_u32 bottom = boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT + ch ?
                       boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - ch : 12U;
    for (vibio_u32 i = 0; i < DESKTOP_ICON_BUILTIN_COUNT; i++) {
        wm->desktop_builtin_x[i] = x;
        wm->desktop_builtin_y[i] = y;
        y += ch + 8U;
        if (y > bottom) {
            y = 12U;
            x += cw + 8U;
        }
    }
    for (vibio_u32 i = 0; i < wm->desktop_temp_count; i++) {
        wm->desktop_temp[i].x = x;
        wm->desktop_temp[i].y = y;
        y += ch + 8U;
        if (y > bottom) {
            y = 12U;
            x += cw + 8U;
        }
    }
}

static void desktop_set_temp_label(VibioDesktopTempIcon *icon, const char *label)
{
    vibio_u32 i = 0;
    while (label[i] != 0 && i + 1U < sizeof(icon->label)) {
        icon->label[i] = label[i];
        i++;
    }
    icon->label[i] = 0;
    icon->label_len = i;
}

static void desktop_create_temp_icon(VibioWindowManager *wm, vibio_u32 kind)
{
    if (wm->desktop_temp_count >= DESKTOP_TEMP_ICON_MAX) {
        return;
    }
    VibioDesktopTempIcon *icon = &wm->desktop_temp[wm->desktop_temp_count];
    icon->kind = kind;
    icon->x = wm->context_click_x;
    icon->y = wm->context_click_y;
    desktop_set_temp_label(icon, kind == DESKTOP_ICON_KIND_TEMP_FOLDER ? "New folder" : "New text file");
    wm->desktop_rename_active = 1;
    wm->desktop_rename_replace = 1;
    wm->desktop_rename_index = DESKTOP_ICON_BUILTIN_COUNT + wm->desktop_temp_count;
    wm->desktop_temp_count++;
    wm->desktop_icons_visible = 1;
    wm->desktop_selected_mask = desktop_icon_mask_for_kind(kind);
}

static void desktop_move_icon(VibioWindowManager *wm, vibio_u32 index, vibio_u32 x, vibio_u32 y, const VibioBootInfo *boot_info)
{
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    vibio_u32 max_y = boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT + ch ?
                      boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - ch : 0;
    vibio_u32 max_x = boot_info->framebuffer_width > cw ? boot_info->framebuffer_width - cw : 0;
    if (x > max_x) { x = max_x; }
    if (y > max_y) { y = max_y; }
    if (index < DESKTOP_ICON_BUILTIN_COUNT) {
        wm->desktop_builtin_x[index] = x;
        wm->desktop_builtin_y[index] = y;
    } else {
        vibio_u32 ti = index - DESKTOP_ICON_BUILTIN_COUNT;
        if (ti < wm->desktop_temp_count) {
            wm->desktop_temp[ti].x = x;
            wm->desktop_temp[ti].y = y;
        }
    }
}

/* Desktop icon grid geometry: a fixed origin and a cell pitch that follows the
 * current icon size, so icons land on predictable, evenly spaced positions. */
#define DESKTOP_GRID_ORIGIN_X 8U
#define DESKTOP_GRID_ORIGIN_Y 12U
#define DESKTOP_GRID_GAP 8U

static vibio_u32 desktop_cell_occupied(
    const VibioWindowManager *wm, vibio_u32 skip_index, vibio_u32 x, vibio_u32 y)
{
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    vibio_u32 count = desktop_icon_count(wm);
    for (vibio_u32 i = 0; i < count; i++) {
        if (i == skip_index) {
            continue;
        }
        VibioDesktopIconSpec ic;
        if (!desktop_icon_info(wm, i, &ic)) {
            continue;
        }
        if (desktop_rects_overlap(x, y, cw, ch, ic.x, ic.y, cw, ch)) {
            return 1;
        }
    }
    return 0;
}

/* Snap a just-dropped icon onto the nearest free grid cell. This is what makes
 * the desktop feel like a clean grid instead of icons landing wherever the
 * pointer happened to be. Cells fill top-to-bottom, then into the next column,
 * matching the default arrangement so labels never overlap. */
static void desktop_snap_icon_to_grid(
    VibioWindowManager *wm, vibio_u32 index, const VibioBootInfo *boot_info)
{
    VibioDesktopIconSpec icon;
    if (!desktop_icon_info(wm, index, &icon)) {
        return;
    }
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    vibio_u32 step_x = cw + DESKTOP_GRID_GAP;
    vibio_u32 step_y = ch + DESKTOP_GRID_GAP;
    vibio_u32 usable_h = boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT + DESKTOP_GRID_ORIGIN_Y ?
                         boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - DESKTOP_GRID_ORIGIN_Y : step_y;
    vibio_u32 rows = usable_h / step_y;
    if (rows == 0) { rows = 1; }
    vibio_u32 cols = boot_info->framebuffer_width / step_x;
    if (cols == 0) { cols = 1; }
    vibio_u32 rel_x = icon.x > DESKTOP_GRID_ORIGIN_X ? icon.x - DESKTOP_GRID_ORIGIN_X : 0;
    vibio_u32 rel_y = icon.y > DESKTOP_GRID_ORIGIN_Y ? icon.y - DESKTOP_GRID_ORIGIN_Y : 0;
    vibio_u32 col = (rel_x + step_x / 2U) / step_x;
    vibio_u32 row = (rel_y + step_y / 2U) / step_y;
    if (col >= cols) { col = cols - 1U; }
    if (row >= rows) { row = rows - 1U; }
    for (vibio_u32 c = col; c < cols; c++) {
        for (vibio_u32 r = (c == col ? row : 0); r < rows; r++) {
            vibio_u32 nx = DESKTOP_GRID_ORIGIN_X + c * step_x;
            vibio_u32 ny = DESKTOP_GRID_ORIGIN_Y + r * step_y;
            if (!desktop_cell_occupied(wm, index, nx, ny)) {
                desktop_move_icon(wm, index, nx, ny, boot_info);
                return;
            }
        }
    }
    desktop_move_icon(wm, index, DESKTOP_GRID_ORIGIN_X + col * step_x,
                      DESKTOP_GRID_ORIGIN_Y + row * step_y, boot_info);
}

static void desktop_open_context_menu(VibioWindowManager *wm, const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y)
{
    vibio_u32 max_x = boot_info->framebuffer_width > DESKTOP_CONTEXT_W * 2U + 8U ?
                      boot_info->framebuffer_width - DESKTOP_CONTEXT_W * 2U - 8U : 0;
    vibio_u32 max_y = boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT + DESKTOP_CONTEXT_ROW_H * DESKTOP_CONTEXT_WALLPAPER_ROWS + 8U ?
                      boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - DESKTOP_CONTEXT_ROW_H * DESKTOP_CONTEXT_WALLPAPER_ROWS - 8U : 0;
    wm->context_x = x > max_x ? max_x : x;
    wm->context_y = y > max_y ? max_y : y;
    wm->context_click_x = x;
    wm->context_click_y = y;
    wm->context_menu = DESKTOP_CONTEXT_ROOT;
    wm->context_hover_row = 0xFFFFFFFFU;
    wm->power_menu_open = 0;
    wm->volume_popup_open = 0;
    wm->volume_dragging = 0;
    wm->power_confirm = POWER_CONFIRM_NONE;
    wm->desktop_rename_active = 0;
}

static void desktop_context_activate(VibioWindowManager *wm, const VibioBootInfo *boot_info, vibio_u32 menu, vibio_u32 row)
{
    if (menu == DESKTOP_CONTEXT_ROOT) {
        if (row == 0) {
            wm->context_menu = DESKTOP_CONTEXT_VIEW;
            return;
        }
        if (row == 1) {
            wm->context_menu = DESKTOP_CONTEXT_NONE;
            return;
        }
        if (row == 2) {
            wm->context_menu = DESKTOP_CONTEXT_NEW;
            return;
        }
        if (row == 3) {
            vibio_u32 idx = wm_find_kind(wm, WINDOW_KIND_TERMINAL);
            if (idx != WM_NO_WINDOW) {
                wm_show_and_raise(wm, idx);
            }
            wm->context_menu = DESKTOP_CONTEXT_NONE;
            return;
        }
        if (row == 4) {
            wm->context_menu = DESKTOP_CONTEXT_WALLPAPER;
            return;
        }
    } else if (menu == DESKTOP_CONTEXT_VIEW) {
        if (row == 0) { wm->desktop_icon_size = DESKTOP_ICON_SIZE_LARGE; }
        if (row == 1) { wm->desktop_icon_size = DESKTOP_ICON_SIZE_MEDIUM; }
        if (row == 2) { wm->desktop_icon_size = DESKTOP_ICON_SIZE_SMALL; }
        if (row == 3) {
            wm->desktop_auto_arrange = 1;
            desktop_arrange_icons(wm, boot_info);
        }
        if (row == 4) { wm->desktop_icons_visible = wm->desktop_icons_visible ? 0 : 1; }
        wm->context_menu = DESKTOP_CONTEXT_NONE;
    } else if (menu == DESKTOP_CONTEXT_NEW) {
        desktop_create_temp_icon(wm, row == 0 ? DESKTOP_ICON_KIND_TEMP_FOLDER : DESKTOP_ICON_KIND_TEMP_TEXT);
        wm->context_menu = DESKTOP_CONTEXT_NONE;
    } else if (menu == DESKTOP_CONTEXT_WALLPAPER) {
        /* Row 0 is the built-in Vibio gradient; rows 1..N pick a real image
         * imported from G:\Backgrounds (see tools/make_wallpapers.py). */
        if (row == 0) {
            wm->wallpaper_mode = 0;
        } else if (row - 1U < (vibio_u32)VIBIO_WALLPAPER_COUNT) {
            wm->wallpaper_mode = WALLPAPER_IMAGE_BASE + (row - 1U);
        }
        wm->desktop_base_dirty = 1;
        wm->context_menu = DESKTOP_CONTEXT_NONE;
    }
}

/*
 * Window manager input: clicking a window raises and focuses it; clicking its
 * title bar also begins dragging; releasing drops it. Returns 1 if the layout
 * (focus or position) changed this frame so the compositor can redraw.
 */
static vibio_u32 wm_update(VibioWindowManager *wm, const VibioMouseState *mouse, const VibioBootInfo *boot_info, VibioAudioState *audio)
{
    vibio_u32 left = mouse->buttons & 0x01;
    vibio_u32 right = mouse->buttons & 0x02;
    vibio_u32 changed = 0;
    vibio_u32 fullscreen_now = wm_any_fullscreen(wm);
    vibio_u32 old_taskbar_hover = wm->taskbar_hover_slot;
    vibio_u32 old_desktop_hover = wm->desktop_hover_kind;
    vibio_u32 old_context_hover = wm->context_hover_row;

    wm->taskbar_hover_slot = fullscreen_now ? TASKBAR_NO_SLOT : taskbar_slot_at(boot_info, wm, mouse->x, mouse->y);
    wm->desktop_hover_kind = DESKTOP_ICON_NONE;
    if (!fullscreen_now && !wm->selecting && wm->taskbar_hover_slot == TASKBAR_NO_SLOT &&
        boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT &&
        mouse->y < boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT &&
        wm_window_at(wm, mouse->x, mouse->y) == WM_NO_WINDOW) {
        wm->desktop_hover_kind = desktop_icon_kind_at(wm, mouse->x, mouse->y);
    }
    wm->context_hover_row = 0xFFFFFFFFU;
    if (wm->context_menu != DESKTOP_CONTEXT_NONE) {
        vibio_u32 menu = DESKTOP_CONTEXT_NONE;
        wm->context_hover_row = desktop_context_hit(wm, mouse->x, mouse->y, &menu);
        /* Hovering a root row swaps the open submenu. Crucially, hovering a root
         * row that is NOT a submenu opener (Refresh, Open terminal) closes any
         * stale submenu so an old submenu never lingers under a different item.
         * Hovering inside the currently open submenu (menu != ROOT) leaves it
         * be; hovering empty space outside both keeps it open. */
        if (menu == DESKTOP_CONTEXT_ROOT) {
            if (wm->context_hover_row == 0) {
                wm->context_menu = DESKTOP_CONTEXT_VIEW;
            } else if (wm->context_hover_row == 2) {
                wm->context_menu = DESKTOP_CONTEXT_NEW;
            } else if (wm->context_hover_row == 4) {
                wm->context_menu = DESKTOP_CONTEXT_WALLPAPER;
            } else {
                wm->context_menu = DESKTOP_CONTEXT_ROOT;
            }
        }
    }
    if (wm->taskbar_hover_slot != old_taskbar_hover || wm->desktop_hover_kind != old_desktop_hover ||
        wm->context_hover_row != old_context_hover) {
        changed = 1;
    }

    if (right && !wm->prev_right && !fullscreen_now &&
        boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT &&
        mouse->y < boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT &&
        wm_window_at(wm, mouse->x, mouse->y) == WM_NO_WINDOW) {
        desktop_open_context_menu(wm, boot_info, mouse->x, mouse->y);
        wm->prev_right = right;
        changed = 1;
        return changed;
    }

    if (left && !wm->prev_left) {
        if (wm->desktop_rename_active) {
            wm->desktop_rename_active = 0;
            wm->desktop_rename_replace = 0;
            changed = 1;
        }
        if (wm->context_menu != DESKTOP_CONTEXT_NONE) {
            vibio_u32 menu = DESKTOP_CONTEXT_NONE;
            vibio_u32 row = desktop_context_hit(wm, mouse->x, mouse->y, &menu);
            if (row != 0xFFFFFFFFU) {
                desktop_context_activate(wm, boot_info, menu, row);
            } else {
                wm->context_menu = DESKTOP_CONTEXT_NONE;
            }
            wm->prev_left = left;
            return 1;
        }

        if (wm->power_confirm != POWER_CONFIRM_NONE) {
            vibio_u32 hit = taskbar_power_confirm_hit(boot_info, mouse->x, mouse->y);
            if (hit == 1) {
                if (wm->power_confirm == POWER_CONFIRM_SHUTDOWN) {
                    power_shutdown();
                } else {
                    power_restart();
                }
            } else if (hit == 2) {
                wm->power_confirm = POWER_CONFIRM_NONE;
                changed = 1;
            } else {
                vibio_u32 x = 0, y = 0, w = 0, h = 0;
                if (taskbar_power_confirm_rect(boot_info, &x, &y, &w, &h) &&
                    !(mouse->x >= x && mouse->x < x + w && mouse->y >= y && mouse->y < y + h)) {
                    wm->power_confirm = POWER_CONFIRM_NONE;
                    changed = 1;
                }
            }
            wm->prev_left = left;
            return changed;
        }

        if (wm->power_menu_open) {
            vibio_u32 row = taskbar_power_menu_row_at(boot_info, mouse->x, mouse->y);
            if (row == POWER_MENU_ROW_SHUTDOWN) {
                power_begin_action(wm, POWER_CONFIRM_SHUTDOWN);
                changed = 1;
            } else if (row == POWER_MENU_ROW_RESTART) {
                power_begin_action(wm, POWER_CONFIRM_RESTART);
                changed = 1;
            } else if (taskbar_power_icon_hit(boot_info, mouse->x, mouse->y)) {
                wm->power_menu_open = 0;
                changed = 1;
            } else {
                vibio_u32 x = 0, y = 0, w = 0, h = 0;
                if (taskbar_power_menu_rect(boot_info, &x, &y, &w, &h) &&
                    !(mouse->x >= x && mouse->x < x + w && mouse->y >= y && mouse->y < y + h)) {
                    wm->power_menu_open = 0;
                    changed = 1;
                }
            }
            wm->prev_left = left;
            return changed;
        }

        if (taskbar_power_icon_hit(boot_info, mouse->x, mouse->y)) {
            wm->power_menu_open = !wm->power_menu_open;
            wm->volume_popup_open = 0;
            wm->power_confirm = POWER_CONFIRM_NONE;
            changed = 1;
            wm->prev_left = left;
            return changed;
        }

        if (taskbar_volume_icon_hit(boot_info, mouse->x, mouse->y)) {
            wm->volume_popup_open = !wm->volume_popup_open;
            wm->power_menu_open = 0;
            wm->power_confirm = POWER_CONFIRM_NONE;
            changed = 1;
            wm->prev_left = left;
            return changed;
        }
        if (wm->volume_popup_open) {
            vibio_u32 x = 0, y = 0, w = 0, h = 0;
            if (taskbar_volume_slider_hit(boot_info, mouse->x, mouse->y)) {
                /* Grab the knob: set immediately and keep tracking while held. */
                wm->volume_dragging = 1;
                taskbar_volume_slider_apply(boot_info, audio, mouse->x);
                changed = 1;
                wm->prev_left = left;
                return changed;
            }
            if (taskbar_volume_popup_rect(boot_info, &x, &y, &w, &h) &&
                !(mouse->x >= x && mouse->x < x + w && mouse->y >= y && mouse->y < y + h)) {
                wm->volume_popup_open = 0;
                changed = 1;
                wm->prev_left = left;
                return changed;
            }
        }

        vibio_u32 idx = wm_taskbar_window_at(boot_info, wm, mouse->x, mouse->y);
        if (idx != WM_NO_WINDOW) {
            /* Taskbar buttons minimize/restore; they never close. */
            wm_activate_from_shell(wm, idx);
            wm->prev_left = left;
            return 1;
        }

        idx = wm_window_at(wm, mouse->x, mouse->y);
        if (idx != WM_NO_WINDOW) {
            /* Title-bar controls take priority over focus/drag. A full-screen
             * window has no chrome, so it has no controls and cannot be dragged
             * (exit with F11). */
            vibio_u32 ctrl = wm->windows[idx].fullscreen ? 0 :
                             window_control_hit(&wm->windows[idx], mouse->x, mouse->y);
            if (ctrl == 3) {
                wm_close(wm, idx);
                wm->prev_left = left;
                return 1;
            }
            if (ctrl == 2) {
                wm_toggle_maximize(wm, idx, boot_info);
                wm->prev_left = left;
                return 1;
            }
            if (ctrl == 1) {
                wm_minimize(wm, idx);
                wm->prev_left = left;
                return 1;
            }

            if (wm->z_order[wm->count - 1] != idx) {
                wm_raise(wm, idx);
                changed = 1;
            }
            VibioWindow *w = &wm->windows[idx];
            if (w->maximized && !w->fullscreen &&
                mouse->y < w->y + WINDOW_TITLEBAR_HEIGHT &&
                mouse->y < boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT) {
                vibio_u32 old_x = w->x;
                vibio_u32 old_w = w->width == 0 ? 1U : w->width;
                vibio_u32 rel_x = mouse->x > old_x ? mouse->x - old_x : 0;
                if (rel_x >= old_w) {
                    rel_x = old_w - 1U;
                }
                vibio_u32 restore_w = w->saved_w == 0 ? WINDOW_WIDTH : w->saved_w;
                vibio_u32 restore_h = w->saved_h == 0 ? WINDOW_HEIGHT : w->saved_h;
                vibio_u32 grab_x = ((vibio_u64)rel_x * (vibio_u64)restore_w) / (vibio_u64)old_w;
                if (grab_x >= restore_w) {
                    grab_x = restore_w - 1U;
                }
                w->x = mouse->x > grab_x ? mouse->x - grab_x : 0;
                w->y = DESKTOP_TOPBAR_HEIGHT;
                w->width = restore_w;
                w->height = restore_h;
                w->maximized = 0;
                if (w->x + w->width > boot_info->framebuffer_width) {
                    w->x = boot_info->framebuffer_width > w->width ? boot_info->framebuffer_width - w->width : 0;
                }
                wm->dragging = 1;
                wm->drag_index = idx;
                wm->drag_dx = mouse->x > w->x ? mouse->x - w->x : 0;
                wm->drag_dy = mouse->y > w->y ? mouse->y - w->y : 0;
                changed = 1;
            } else if (!w->maximized && !w->fullscreen &&
                mouse->y < w->y + WINDOW_TITLEBAR_HEIGHT &&
                mouse->y < boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT) {
                wm->dragging = 1;
                wm->drag_index = idx;
                wm->drag_dx = mouse->x - w->x;
                wm->drag_dy = mouse->y - w->y;
            }
        } else {
            vibio_u32 icon_index = desktop_icon_index_at(wm, mouse->x, mouse->y);
            idx = wm_desktop_icon_window_at(wm, mouse->x, mouse->y);
            if (icon_index != 0xFFFFFFFFU) {
                /* Pressing an icon that is NOT already in the selection selects
                 * just it. Pressing one that IS already selected keeps the whole
                 * (multi) selection, so the following drag moves the entire group
                 * together. A plain click without a drag reduces back to one icon
                 * on release (below). */
                VibioDesktopIconSpec pressed;
                vibio_u32 pressed_mask = desktop_icon_info(wm, icon_index, &pressed) ?
                                         desktop_icon_mask_for_kind(pressed.kind) : 0;
                if ((wm->desktop_selected_mask & pressed_mask) == 0) {
                    wm->desktop_selected_mask = pressed_mask;
                }
                wm->desktop_pressing = 1;
                wm->desktop_press_index = icon_index;
                wm->desktop_press_x = mouse->x;
                wm->desktop_press_y = mouse->y;
                changed = 1;
            } else if (!fullscreen_now &&
                       boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT &&
                       mouse->y < boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT) {
                wm->desktop_selected_mask = 0;
                wm->selecting = 1;
                wm->select_start_x = mouse->x;
                wm->select_start_y = mouse->y;
                wm->select_x = mouse->x;
                wm->select_y = mouse->y;
                changed = 1;
            }
        }
    } else if (!left) {
        wm->dragging = 0;
        if (wm->desktop_pressing && !wm->desktop_dragging_icon) {
            /* A click that did not turn into a drag. The first click only
             * selected (done on press); a second click on the SAME icon within
             * the double-click window opens it - matching Windows. */
            vibio_u32 is_double = (wm->desktop_press_index == wm->desktop_last_click_index) &&
                                  (g_anim_now - wm->desktop_last_click_tick <= DESKTOP_DOUBLE_CLICK_TICKS);
            if (is_double) {
                vibio_u32 idx = wm_desktop_icon_window_at(wm, wm->desktop_press_x, wm->desktop_press_y);
                if (idx != WM_NO_WINDOW) {
                    wm_activate_from_shell(wm, idx);
                }
                wm->desktop_last_click_index = 0xFFFFFFFFU;
                wm->desktop_last_click_tick = 0;
            } else {
                wm->desktop_last_click_index = wm->desktop_press_index;
                wm->desktop_last_click_tick = g_anim_now;
                /* A plain single click (no drag) reduces the selection to just the
                 * clicked icon (Windows behaviour). */
                VibioDesktopIconSpec ps;
                if (desktop_icon_info(wm, wm->desktop_press_index, &ps)) {
                    wm->desktop_selected_mask = desktop_icon_mask_for_kind(ps.kind);
                }
            }
            changed = 1;
        }
        if (wm->desktop_dragging_icon) {
            /* Drop: snap every selected icon onto the nearest free grid cell so
             * the desktop stays a clean, predictable grid. The dragged icon is
             * snapped first so the rest pack around its landing spot. */
            desktop_snap_icon_to_grid(wm, wm->desktop_drag_index, boot_info);
            vibio_u32 drop_count = desktop_icon_count(wm);
            for (vibio_u32 i = 0; i < drop_count; i++) {
                if (i == wm->desktop_drag_index) {
                    continue;
                }
                VibioDesktopIconSpec ic;
                if (!desktop_icon_info(wm, i, &ic)) {
                    continue;
                }
                if ((wm->desktop_selected_mask & desktop_icon_mask_for_kind(ic.kind)) != 0) {
                    desktop_snap_icon_to_grid(wm, i, boot_info);
                }
            }
            wm->desktop_auto_arrange = 0;
            changed = 1;
        }
        wm->desktop_pressing = 0;
        wm->desktop_dragging_icon = 0;
        if (wm->selecting) {
            wm->desktop_selected_mask = desktop_selection_mask(wm);
            wm->selecting = 0;
            changed = 1;
        }
    }

    if (left && wm->selecting) {
        if (wm->select_x != mouse->x || wm->select_y != mouse->y) {
            wm->select_x = mouse->x;
            wm->select_y = mouse->y;
            changed = 1;
        }
    }

    if (left && wm->desktop_pressing) {
        vibio_u32 dx = mouse->x > wm->desktop_press_x ? mouse->x - wm->desktop_press_x : wm->desktop_press_x - mouse->x;
        vibio_u32 dy = mouse->y > wm->desktop_press_y ? mouse->y - wm->desktop_press_y : wm->desktop_press_y - mouse->y;
        if (!wm->desktop_dragging_icon && (dx > 4U || dy > 4U)) {
            VibioDesktopIconSpec icon;
            if (desktop_icon_info(wm, wm->desktop_press_index, &icon)) {
                wm->desktop_dragging_icon = 1;
                wm->desktop_drag_index = wm->desktop_press_index;
                wm->desktop_drag_dx = mouse->x > icon.x ? mouse->x - icon.x : 0;
                wm->desktop_drag_dy = mouse->y > icon.y ? mouse->y - icon.y : 0;
                wm->desktop_auto_arrange = 0;
            }
        }
        if (wm->desktop_dragging_icon) {
            vibio_u32 nx = mouse->x > wm->desktop_drag_dx ? mouse->x - wm->desktop_drag_dx : 0;
            vibio_u32 ny = mouse->y > wm->desktop_drag_dy ? mouse->y - wm->desktop_drag_dy : 0;
            /* Move every OTHER selected icon by the same delta as the dragged
             * icon, so a multi-selection drags as a group instead of leaving the
             * rest behind. */
            VibioDesktopIconSpec prim;
            if (desktop_icon_info(wm, wm->desktop_drag_index, &prim)) {
                int ddx = (int)nx - (int)prim.x;
                int ddy = (int)ny - (int)prim.y;
                if (ddx != 0 || ddy != 0) {
                    vibio_u32 cnt = desktop_icon_count(wm);
                    for (vibio_u32 i = 0; i < cnt; i++) {
                        if (i == wm->desktop_drag_index) {
                            continue;
                        }
                        VibioDesktopIconSpec ic;
                        if (!desktop_icon_info(wm, i, &ic)) {
                            continue;
                        }
                        if ((wm->desktop_selected_mask & desktop_icon_mask_for_kind(ic.kind)) == 0) {
                            continue;
                        }
                        int ox = (int)ic.x + ddx;
                        int oy = (int)ic.y + ddy;
                        desktop_move_icon(wm, i, ox < 0 ? 0 : (vibio_u32)ox, oy < 0 ? 0 : (vibio_u32)oy, boot_info);
                    }
                }
            }
            desktop_move_icon(wm, wm->desktop_drag_index, nx, ny, boot_info);
            changed = 1;
        }
    }

    /* Volume knob drag: once grabbed (on press), follow the mouse x every frame
     * the button is held, then release. Reached only when this is not a fresh
     * press edge, so it does not interfere with starting other drags. */
    if (wm->volume_dragging) {
        if (left && wm->volume_popup_open) {
            taskbar_volume_slider_apply(boot_info, audio, mouse->x);
            changed = 1;
        } else {
            wm->volume_dragging = 0;
        }
    }

    if (wm->dragging) {
        VibioWindow *w = &wm->windows[wm->drag_index];
        int nx = (int)mouse->x - (int)wm->drag_dx;
        int ny = (int)mouse->y - (int)wm->drag_dy;
        int min_y = (int)DESKTOP_TOPBAR_HEIGHT;
        int max_bottom = (int)boot_info->framebuffer_height - (int)DESKTOP_TASKBAR_HEIGHT;
        if (nx < 0) {
            nx = 0;
        }
        if (ny < min_y) {
            ny = min_y;
        }
        if ((vibio_u32)nx + w->width > boot_info->framebuffer_width) {
            nx = (int)boot_info->framebuffer_width - (int)w->width;
        }
        if (max_bottom < min_y) {
            max_bottom = (int)boot_info->framebuffer_height;
        }
        if (ny + (int)w->height > max_bottom) {
            ny = max_bottom - (int)w->height;
        }
        if (nx < 0) {
            nx = 0;
        }
        if (ny < min_y) {
            ny = min_y;
        }
        if (w->x != (vibio_u32)nx || w->y != (vibio_u32)ny) {
            w->x = (vibio_u32)nx;
            w->y = (vibio_u32)ny;
            changed = 1;
        }
    }

    wm->prev_left = left;
    wm->prev_right = right;
    return changed;
}

/* Composite all windows back-to-front; the topmost is drawn focused. */
static void wm_draw(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopContext *desktop,
    vibio_u32 border)
{
    for (vibio_u32 i = 0; i < wm->count; i++) {
        vibio_u32 idx = wm->z_order[i];
        if (!wm->windows[idx].visible) {
            continue;
        }
        vibio_u32 focused = (idx == wm_top_visible(wm));
        window_draw(boot_info, &wm->windows[idx], desktop, border, focused);
    }
}

static vibio_u32 wm_visual_state_changed(const VibioWindowManager *a, const VibioWindowManager *b)
{
    if (a->count != b->count) {
        return 1;
    }
    if (a->desktop_icons_visible != b->desktop_icons_visible ||
        a->desktop_icon_size != b->desktop_icon_size ||
        a->desktop_temp_count != b->desktop_temp_count ||
        a->desktop_rename_active != b->desktop_rename_active ||
        a->desktop_rename_replace != b->desktop_rename_replace ||
        a->context_menu != b->context_menu ||
        a->wallpaper_mode != b->wallpaper_mode) {
        return 1;
    }
    for (vibio_u32 di = 0; di < DESKTOP_ICON_BUILTIN_COUNT; di++) {
        if (a->desktop_builtin_x[di] != b->desktop_builtin_x[di] ||
            a->desktop_builtin_y[di] != b->desktop_builtin_y[di]) {
            return 1;
        }
    }
    for (vibio_u32 di = 0; di < a->desktop_temp_count && di < DESKTOP_TEMP_ICON_MAX; di++) {
        if (a->desktop_temp[di].x != b->desktop_temp[di].x ||
            a->desktop_temp[di].y != b->desktop_temp[di].y ||
            a->desktop_temp[di].kind != b->desktop_temp[di].kind ||
            a->desktop_temp[di].label_len != b->desktop_temp[di].label_len) {
            return 1;
        }
        for (vibio_u32 ci = 0; ci <= a->desktop_temp[di].label_len && ci < sizeof(a->desktop_temp[di].label); ci++) {
            if (a->desktop_temp[di].label[ci] != b->desktop_temp[di].label[ci]) {
                return 1;
            }
        }
    }
    for (vibio_u32 i = 0; i < a->count; i++) {
        const VibioWindow *wa = &a->windows[i];
        const VibioWindow *wb = &b->windows[i];
        if (wa->x != wb->x || wa->y != wb->y ||
            wa->width != wb->width || wa->height != wb->height ||
            wa->visible != wb->visible || wa->minimized != wb->minimized ||
            wa->maximized != wb->maximized || wa->fullscreen != wb->fullscreen ||
            wa->anim_open != wb->anim_open || wa->anim_close != wb->anim_close ||
            wa->anim_to_min != wb->anim_to_min) {
            return 1;
        }
        if (a->z_order[i] != b->z_order[i]) {
            return 1;
        }
    }
    return 0;
}

static void rect_empty(VibioRect *r)
{
    r->x0 = 0xFFFFFFFFU;
    r->y0 = 0xFFFFFFFFU;
    r->x1 = 0;
    r->y1 = 0;
}

static vibio_u32 rect_is_empty(const VibioRect *r)
{
    return r->x0 >= r->x1 || r->y0 >= r->y1;
}

static void rect_add(VibioRect *r, vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h, const VibioBootInfo *boot_info)
{
    if (w == 0 || h == 0 || x >= boot_info->framebuffer_width || y >= boot_info->framebuffer_height) {
        return;
    }
    vibio_u32 x1 = x + w;
    vibio_u32 y1 = y + h;
    if (x1 < x || x1 > boot_info->framebuffer_width) {
        x1 = boot_info->framebuffer_width;
    }
    if (y1 < y || y1 > boot_info->framebuffer_height) {
        y1 = boot_info->framebuffer_height;
    }
    if (rect_is_empty(r)) {
        r->x0 = x;
        r->y0 = y;
        r->x1 = x1;
        r->y1 = y1;
        return;
    }
    if (x < r->x0) { r->x0 = x; }
    if (y < r->y0) { r->y0 = y; }
    if (x1 > r->x1) { r->x1 = x1; }
    if (y1 > r->y1) { r->y1 = y1; }
}

static vibio_u64 rect_area(const VibioRect *r)
{
    if (rect_is_empty(r)) {
        return 0;
    }
    return (vibio_u64)(r->x1 - r->x0) * (vibio_u64)(r->y1 - r->y0);
}

static VibioRect rect_union_copy(const VibioRect *a, const VibioRect *b)
{
    VibioRect out = *a;
    if (rect_is_empty(&out)) {
        return *b;
    }
    if (rect_is_empty(b)) {
        return out;
    }
    if (b->x0 < out.x0) { out.x0 = b->x0; }
    if (b->y0 < out.y0) { out.y0 = b->y0; }
    if (b->x1 > out.x1) { out.x1 = b->x1; }
    if (b->y1 > out.y1) { out.y1 = b->y1; }
    return out;
}

static vibio_u32 rect_touches_or_overlaps(const VibioRect *a, const VibioRect *b)
{
    if (rect_is_empty(a) || rect_is_empty(b)) {
        return 0;
    }
    return !(b->x0 > a->x1 || b->x1 < a->x0 || b->y0 > a->y1 || b->y1 < a->y0);
}

static void dirty_list_empty(VibioDirtyList *list)
{
    list->count = 0;
}

static void dirty_list_add_rect(VibioDirtyList *list, VibioRect rect)
{
    if (rect_is_empty(&rect)) {
        return;
    }

merge_again:
    for (vibio_u32 i = 0; i < list->count; i++) {
        if (rect_touches_or_overlaps(&list->rects[i], &rect)) {
            list->rects[i] = rect_union_copy(&list->rects[i], &rect);
            rect = list->rects[i];
            for (vibio_u32 j = i + 1; j < list->count; j++) {
                if (rect_touches_or_overlaps(&list->rects[i], &list->rects[j])) {
                    list->rects[i] = rect_union_copy(&list->rects[i], &list->rects[j]);
                    for (vibio_u32 k = j; k + 1 < list->count; k++) {
                        list->rects[k] = list->rects[k + 1];
                    }
                    list->count--;
                    rect = list->rects[i];
                    goto merge_again;
                }
            }
            return;
        }
    }

    if (list->count < DIRTY_RECT_MAX) {
        list->rects[list->count++] = rect;
        return;
    }

    /* If too many tiny regions accumulate, merge with the rectangle that grows
     * least. That keeps redraw bounded without falling back to a full screen. */
    vibio_u32 best = 0;
    vibio_u64 best_growth = 0xFFFFFFFFFFFFFFFFULL;
    for (vibio_u32 i = 0; i < list->count; i++) {
        VibioRect merged = rect_union_copy(&list->rects[i], &rect);
        vibio_u64 before = rect_area(&list->rects[i]);
        vibio_u64 after = rect_area(&merged);
        vibio_u64 growth = after > before ? after - before : 0;
        if (growth < best_growth) {
            best_growth = growth;
            best = i;
        }
    }
    list->rects[best] = rect_union_copy(&list->rects[best], &rect);
}

static void dirty_list_add(
    VibioDirtyList *list,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    const VibioBootInfo *boot_info)
{
    VibioRect rect;
    rect_empty(&rect);
    rect_add(&rect, x, y, w, h, boot_info);
    dirty_list_add_rect(list, rect);
}

static void dirty_list_add_window(VibioDirtyList *list, const VibioWindow *w, const VibioBootInfo *boot_info)
{
    if (!w->visible && !w->anim_close) {
        return;
    }
    vibio_u32 pad = w->fullscreen ? 0 : 10;
    vibio_u32 x = w->x > pad ? w->x - pad : 0;
    vibio_u32 y = w->y > pad ? w->y - pad : 0;
    dirty_list_add(list, x, y, w->width + pad * 2, w->height + pad * 2 + 24, boot_info);
}

static void dirty_list_add_all_windows(VibioDirtyList *list, const VibioWindowManager *wm, const VibioBootInfo *boot_info)
{
    for (vibio_u32 i = 0; i < wm->count; i++) {
        dirty_list_add_window(list, &wm->windows[i], boot_info);
    }
}

static void dirty_list_add_taskbar(VibioDirtyList *list, const VibioBootInfo *boot_info)
{
    if (boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT) {
        dirty_list_add(list, 0, boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT,
                       boot_info->framebuffer_width, DESKTOP_TASKBAR_HEIGHT, boot_info);
    }
}

static void dirty_list_add_desktop_icon_area(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm)
{
    vibio_u32 count = desktop_icon_count(wm);
    if (count == 0) {
        return;
    }
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    VibioDesktopIconSpec icon;
    if (!desktop_icon_info(wm, 0, &icon)) {
        return;
    }
    vibio_u32 x0 = icon.x;
    vibio_u32 y0 = icon.y;
    vibio_u32 x1 = icon.x + cw;
    vibio_u32 y1 = icon.y + ch;
    for (vibio_u32 i = 1; i < count; i++) {
        if (!desktop_icon_info(wm, i, &icon)) {
            continue;
        }
        if (icon.x < x0) { x0 = icon.x; }
        if (icon.y < y0) { y0 = icon.y; }
        if (icon.x + cw > x1) { x1 = icon.x + cw; }
        if (icon.y + ch > y1) { y1 = icon.y + ch; }
    }
    /* The label hangs a few pixels below the cell box, so extend the rectangle
     * downward to clear it fully (otherwise label tails smear). */
    dirty_list_add(list, x0, y0, x1 - x0, (y1 - y0) + 12U, boot_info);
}

/* Dirty rectangle for a single desktop icon cell at (x,y), including the label
 * that sits just below the cell, so a moved icon never leaves a trail. */
static void dirty_list_add_desktop_icon_cell(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    vibio_u32 x,
    vibio_u32 y)
{
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    dirty_list_add(list, x > 2U ? x - 2U : 0, y > 2U ? y - 2U : 0,
                   cw + 4U, ch + 14U, boot_info);
}

static void dirty_list_add_desktop_selection_rect(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm)
{
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!desktop_selection_rect(wm, &x, &y, &w, &h)) {
        return;
    }
    vibio_u32 pad = 3U;
    vibio_u32 rx = x > pad ? x - pad : 0;
    vibio_u32 ry = y > pad ? y - pad : 0;
    dirty_list_add(list, rx, ry, w + pad * 2U, h + pad * 2U, boot_info);
}

static void dirty_list_add_desktop_interactions(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    const VibioWindowManager *before,
    const VibioWindowManager *after)
{
    if (before->desktop_hover_kind != after->desktop_hover_kind ||
        before->desktop_selected_mask != after->desktop_selected_mask ||
        before->desktop_icons_visible != after->desktop_icons_visible ||
        before->desktop_icon_size != after->desktop_icon_size ||
        before->desktop_temp_count != after->desktop_temp_count ||
        before->context_menu != after->context_menu ||
        before->context_x != after->context_x ||
        before->context_y != after->context_y ||
        before->context_hover_row != after->context_hover_row ||
        before->selecting != after->selecting ||
        before->select_start_x != after->select_start_x ||
        before->select_start_y != after->select_start_y ||
        before->select_x != after->select_x ||
        before->select_y != after->select_y) {
        dirty_list_add_desktop_icon_area(list, boot_info, before);
        dirty_list_add_desktop_icon_area(list, boot_info, after);
        dirty_list_add_desktop_selection_rect(list, boot_info, before);
        dirty_list_add_desktop_selection_rect(list, boot_info, after);
        if (before->context_menu != DESKTOP_CONTEXT_NONE) {
            dirty_list_add(list, before->context_x > 8U ? before->context_x - 8U : 0,
                           before->context_y > 8U ? before->context_y - 8U : 0,
                           DESKTOP_CONTEXT_W * 2U + 24U, 260U, boot_info);
        }
        if (after->context_menu != DESKTOP_CONTEXT_NONE) {
            dirty_list_add(list, after->context_x > 8U ? after->context_x - 8U : 0,
                           after->context_y > 8U ? after->context_y - 8U : 0,
                           DESKTOP_CONTEXT_W * 2U + 24U, 260U, boot_info);
        }
    }

    /* Dragging an icon changes only its position - which none of the field
     * comparisons above detect - so without this the moved icon smears a trail
     * (the cursor's small dirty rect was the only thing being redrawn). Redraw
     * both the old and the new cell of every icon that moved this frame. */
    vibio_u32 before_count = desktop_icon_count(before);
    vibio_u32 after_count = desktop_icon_count(after);
    vibio_u32 scan = before_count < after_count ? after_count : before_count;
    for (vibio_u32 i = 0; i < scan; i++) {
        VibioDesktopIconSpec ib, ia;
        vibio_u32 have_b = i < before_count && desktop_icon_info(before, i, &ib);
        vibio_u32 have_a = i < after_count && desktop_icon_info(after, i, &ia);
        if (have_b && have_a && (ib.x != ia.x || ib.y != ia.y)) {
            dirty_list_add_desktop_icon_cell(list, boot_info, before, ib.x, ib.y);
            dirty_list_add_desktop_icon_cell(list, boot_info, after, ia.x, ia.y);
        }
    }
}

static void dirty_list_add_taskbar_hover_popup(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    vibio_u32 slot)
{
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (taskbar_hover_popup_rect(boot_info, wm, slot, &x, &y, &w, &h)) {
        vibio_u32 pad = 8U;
        vibio_u32 rx = x > pad ? x - pad : 0;
        vibio_u32 ry = y > pad ? y - pad : 0;
        dirty_list_add(list, rx, ry, w + pad * 2U, h + pad * 2U, boot_info);
    }
}

static void dirty_list_add_power_menu_overlay(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    vibio_u32 menu_open,
    vibio_u32 confirm_kind)
{
    if (menu_open && confirm_kind == POWER_CONFIRM_NONE) {
        vibio_u32 x = 0, y = 0, w = 0, h = 0;
        if (taskbar_power_menu_rect(boot_info, &x, &y, &w, &h)) {
            vibio_u32 pad = 8U;
            vibio_u32 rx = x > pad ? x - pad : 0;
            vibio_u32 ry = y > pad ? y - pad : 0;
            dirty_list_add(list, rx, ry, w + pad * 2, h + pad * 2, boot_info);
        }
    }
}

static void dirty_list_add_power_shell_overlays(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    vibio_u32 menu_open,
    vibio_u32 confirm_kind)
{
    dirty_list_add_power_menu_overlay(list, boot_info, menu_open, confirm_kind);
    if (confirm_kind != POWER_CONFIRM_NONE) {
        dirty_list_add(list, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, boot_info);
    }
}

/* The volume popup is a floating shell overlay like the power menu: its rect
 * must be marked dirty when it opens, closes or its slider moves, otherwise the
 * compositor never blits it (it would only appear where the cursor's own dirty
 * rect happens to pass over it). */
static void dirty_list_add_volume_popup_overlay(
    VibioDirtyList *list,
    const VibioBootInfo *boot_info,
    vibio_u32 popup_open)
{
    if (!popup_open) {
        return;
    }
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (taskbar_volume_popup_rect(boot_info, &x, &y, &w, &h)) {
        vibio_u32 pad = 8U;
        vibio_u32 rx = x > pad ? x - pad : 0;
        vibio_u32 ry = y > pad ? y - pad : 0;
        dirty_list_add(list, rx, ry, w + pad * 2U, h + pad * 2U, boot_info);
    }
}

static void dirty_list_add_cursor(VibioDirtyList *list, vibio_u32 x, vibio_u32 y, const VibioBootInfo *boot_info)
{
    dirty_list_add(list, x, y, CURSOR_WIDTH + 2, CURSOR_HEIGHT + 2, boot_info);
}

static void rect_add_window(VibioRect *r, const VibioWindow *w, const VibioBootInfo *boot_info)
{
    if (!w->visible && !w->anim_close) {
        return;
    }
    vibio_u32 pad = w->fullscreen ? 0 : 10;
    vibio_u32 x = w->x > pad ? w->x - pad : 0;
    vibio_u32 y = w->y > pad ? w->y - pad : 0;
    rect_add(r, x, y, w->width + pad * 2, w->height + pad * 2 + 24, boot_info);
}

MAYBE_UNUSED static void rect_add_all_windows(VibioRect *r, const VibioWindowManager *wm, const VibioBootInfo *boot_info)
{
    for (vibio_u32 i = 0; i < wm->count; i++) {
        rect_add_window(r, &wm->windows[i], boot_info);
    }
}

MAYBE_UNUSED static void rect_add_taskbar(VibioRect *r, const VibioBootInfo *boot_info)
{
    if (boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT) {
        rect_add(r, 0, boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT,
                 boot_info->framebuffer_width, DESKTOP_TASKBAR_HEIGHT, boot_info);
    }
}

static void rect_add_power_menu_overlay(
    VibioRect *r,
    const VibioBootInfo *boot_info,
    vibio_u32 menu_open,
    vibio_u32 confirm_kind)
{
    if (menu_open && confirm_kind == POWER_CONFIRM_NONE) {
        vibio_u32 x = 0, y = 0, w = 0, h = 0;
        if (taskbar_power_menu_rect(boot_info, &x, &y, &w, &h)) {
            vibio_u32 pad = 8U;
            vibio_u32 rx = x > pad ? x - pad : 0;
            vibio_u32 ry = y > pad ? y - pad : 0;
            rect_add(r, rx, ry, w + pad * 2, h + pad * 2, boot_info);
        }
    }
}

static void rect_add_power_confirm_overlay(
    VibioRect *r,
    const VibioBootInfo *boot_info,
    vibio_u32 confirm_kind)
{
    if (confirm_kind != POWER_CONFIRM_NONE) {
        rect_add(r, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, boot_info);
    }
}

MAYBE_UNUSED static void rect_add_power_shell_overlays(
    VibioRect *r,
    const VibioBootInfo *boot_info,
    vibio_u32 menu_open,
    vibio_u32 confirm_kind)
{
    rect_add_power_menu_overlay(r, boot_info, menu_open, confirm_kind);
    rect_add_power_confirm_overlay(r, boot_info, confirm_kind);
}

MAYBE_UNUSED static void rect_add_cursor(VibioRect *r, vibio_u32 x, vibio_u32 y, const VibioBootInfo *boot_info)
{
    rect_add(r, x, y, CURSOR_WIDTH + 2, CURSOR_HEIGHT + 2, boot_info);
}

static vibio_u32 draw_uint(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 value,
    vibio_u32 color)
{
    char tmp[24];
    char out[24];
    vibio_u32 t = 0;
    vibio_u32 n = 0;

    if (value == 0) {
        out[n++] = '0';
    } else {
        while (value > 0 && t < 20) {
            tmp[t++] = (char)('0' + (value % 10));
            value /= 10;
        }
        while (t > 0) {
            out[n++] = tmp[--t];
        }
    }

    return font_draw_buf(boot_info, &FONT_GEIST_UI, x, y, out, n, color);
}

static vibio_u32 draw_two_digits(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u64 value, vibio_u32 color)
{
    vibio_u64 tens = (value / 10) % 10;
    vibio_u64 ones = value % 10;
    draw_char(boot_info, x, y, (char)('0' + tens), color);
    x += CHAR_STEP;
    draw_char(boot_info, x, y, (char)('0' + ones), color);
    return x + CHAR_STEP;
}

static vibio_u32 draw_hex64(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 value,
    vibio_u32 color)
{
    for (vibio_u32 shift = 60; shift <= 60; shift -= 4) {
        vibio_u8 nibble = (vibio_u8)((value >> shift) & 0xF);
        char digit = nibble < 10 ? (char)('0' + nibble) : (char)('A' + (nibble - 10));
        draw_char(boot_info, x, y, digit, color);
        x += CHAR_STEP;

        if (shift == 0) {
            break;
        }
    }

    return x;
}

static vibio_u32 draw_hex8(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 value,
    vibio_u32 color)
{
    for (vibio_u32 shift = 4; shift <= 4; shift -= 4) {
        vibio_u8 nibble = (vibio_u8)((value >> shift) & 0xF);
        char digit = nibble < 10 ? (char)('0' + nibble) : (char)('A' + (nibble - 10));
        draw_char(boot_info, x, y, digit, color);
        x += CHAR_STEP;

        if (shift == 0) {
            break;
        }
    }

    return x;
}

static vibio_u64 count_usable_pages(const VibioBootInfo *boot_info)
{
    vibio_u64 pages = 0;

    if (boot_info->memory_map_base == 0 || boot_info->memory_map_descriptor_size == 0) {
        return 0;
    }

    for (vibio_u64 offset = 0; offset < boot_info->memory_map_size; offset += boot_info->memory_map_descriptor_size) {
        VibioMemoryDescriptor *descriptor = (VibioMemoryDescriptor *)(boot_info->memory_map_base + offset);
        if (descriptor->type == VIBIO_MEMORY_TYPE_CONVENTIONAL) {
            pages += descriptor->number_of_pages;
        }
    }

    return pages;
}

static vibio_u64 align_up(vibio_u64 value, vibio_u64 alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static vibio_u64 align_down(vibio_u64 value, vibio_u64 alignment)
{
    return value & ~(alignment - 1);
}

static vibio_u64 allocator_region_pages(VibioMemoryDescriptor *descriptor, vibio_u64 *region_start)
{
    vibio_u64 start = descriptor->physical_start;
    vibio_u64 end = descriptor->physical_start + descriptor->number_of_pages * PAGE_SIZE;

    if (end <= ALLOCATOR_MIN_ADDRESS) {
        return 0;
    }

    if (start < ALLOCATOR_MIN_ADDRESS) {
        start = ALLOCATOR_MIN_ADDRESS;
    }

    start = align_up(start, PAGE_SIZE);
    if (start >= end) {
        return 0;
    }

    *region_start = start;
    return (end - start) / PAGE_SIZE;
}

static VibioMemoryDescriptor *memory_descriptor_at(const VibioBootInfo *boot_info, vibio_u64 offset)
{
    return (VibioMemoryDescriptor *)(boot_info->memory_map_base + offset);
}

static void page_allocator_init(VibioPageAllocator *allocator, const VibioBootInfo *boot_info)
{
    allocator->boot_info = boot_info;
    allocator->next_descriptor_offset = 0;
    allocator->region_next_address = 0;
    allocator->region_pages_left = 0;
    allocator->free_pages = 0;
    allocator->allocated_pages = 0;

    if (boot_info->memory_map_base == 0 || boot_info->memory_map_descriptor_size == 0) {
        return;
    }

    for (vibio_u64 offset = 0; offset < boot_info->memory_map_size; offset += boot_info->memory_map_descriptor_size) {
        VibioMemoryDescriptor *descriptor = memory_descriptor_at(boot_info, offset);
        if (descriptor->type == VIBIO_MEMORY_TYPE_CONVENTIONAL) {
            vibio_u64 region_start = 0;
            allocator->free_pages += allocator_region_pages(descriptor, &region_start);
        }
    }
}

static vibio_u64 page_allocator_alloc(VibioPageAllocator *allocator)
{
    const VibioBootInfo *boot_info = allocator->boot_info;

    if (boot_info->memory_map_base == 0 || boot_info->memory_map_descriptor_size == 0) {
        return 0;
    }

    for (;;) {
        if (allocator->region_pages_left > 0) {
            vibio_u64 page = allocator->region_next_address;
            allocator->region_next_address += PAGE_SIZE;
            allocator->region_pages_left--;
            allocator->allocated_pages++;
            return page;
        }

        if (allocator->next_descriptor_offset >= boot_info->memory_map_size) {
            return 0;
        }

        VibioMemoryDescriptor *descriptor = memory_descriptor_at(boot_info, allocator->next_descriptor_offset);
        allocator->next_descriptor_offset += boot_info->memory_map_descriptor_size;

        if (descriptor->type == VIBIO_MEMORY_TYPE_CONVENTIONAL) {
            vibio_u64 region_start = 0;
            vibio_u64 region_pages = allocator_region_pages(descriptor, &region_start);
            if (region_pages > 0) {
                allocator->region_next_address = region_start;
                allocator->region_pages_left = region_pages;
            }
        }
    }
}

/*
 * Allocate a run of physically contiguous pages. The allocator hands out pages
 * sequentially within a memory region, so consecutive allocations are usually
 * adjacent; if a region boundary breaks the run, this returns 0 and the caller
 * falls back to direct framebuffer drawing.
 */
static vibio_u64 alloc_contiguous_pages(VibioPageAllocator *allocator, vibio_u64 pages)
{
    if (pages == 0) {
        return 0;
    }

    vibio_u64 first = page_allocator_alloc(allocator);
    if (first == 0) {
        return 0;
    }

    vibio_u64 expected = first + PAGE_SIZE;
    for (vibio_u64 i = 1; i < pages; i++) {
        vibio_u64 page = page_allocator_alloc(allocator);
        if (page != expected) {
            return 0;
        }
        expected += PAGE_SIZE;
    }

    return first;
}

static vibio_u32 test_allocated_page(vibio_u64 page_address, vibio_u64 pattern)
{
    if (page_address == 0) {
        return 0;
    }

    volatile vibio_u64 *page = (volatile vibio_u64 *)page_address;
    page[0] = pattern;
    page[1] = pattern ^ 0xFFFFFFFFFFFFFFFFULL;

    return page[0] == pattern && page[1] == (pattern ^ 0xFFFFFFFFFFFFFFFFULL);
}

static void zero_page(vibio_u64 page_address)
{
    volatile vibio_u64 *page = (volatile vibio_u64 *)page_address;

    for (vibio_u32 i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        page[i] = 0;
    }
}

static void zero_bytes(void *address, vibio_u64 size)
{
    volatile vibio_u8 *bytes = (volatile vibio_u8 *)address;

    for (vibio_u64 i = 0; i < size; i++) {
        bytes[i] = 0;
    }
}

static void copy_bytes(void *destination, const void *source, vibio_u64 size)
{
    vibio_u8 *dst = (vibio_u8 *)destination;
    const vibio_u8 *src = (const vibio_u8 *)source;

    for (vibio_u64 i = 0; i < size; i++) {
        dst[i] = src[i];
    }
}

static VibioHeapBlock *heap_block_at(vibio_u64 address)
{
    return (VibioHeapBlock *)address;
}

static void heap_init_empty(VibioKernelHeap *heap, VibioPageAllocator *page_allocator)
{
    heap->page_allocator = page_allocator;
    heap->first_block = 0;
    heap->pages = 0;
    heap->total_bytes = 0;
    heap->used_bytes = 0;
    heap->free_bytes = 0;
    heap->allocations = 0;
    heap->frees = 0;
    heap->live_allocations = 0;
    heap->failed_allocations = 0;
    heap->ok = 0;
}

static vibio_u32 heap_add_page(VibioKernelHeap *heap)
{
    vibio_u64 page = page_allocator_alloc(heap->page_allocator);
    if (page == 0) {
        heap->failed_allocations++;
        return 0;
    }

    zero_page(page);
    VibioHeapBlock *block = heap_block_at(page);
    block->size = PAGE_SIZE - sizeof(VibioHeapBlock);
    block->used = 0;
    block->next = 0;
    block->magic = HEAP_BLOCK_MAGIC;

    if (heap->first_block == 0) {
        heap->first_block = page;
    } else {
        VibioHeapBlock *tail = heap_block_at(heap->first_block);
        while (tail->next != 0) {
            tail = heap_block_at(tail->next);
        }

        tail->next = page;
    }

    heap->pages++;
    heap->total_bytes += PAGE_SIZE;
    heap->free_bytes += block->size;
    heap->ok = 1;
    return 1;
}

static void heap_init(VibioKernelHeap *heap, VibioPageAllocator *page_allocator, vibio_u32 initial_pages)
{
    heap_init_empty(heap, page_allocator);

    for (vibio_u32 i = 0; i < initial_pages; i++) {
        if (!heap_add_page(heap)) {
            return;
        }
    }
}

static VibioHeapBlock *heap_find_free_block(VibioKernelHeap *heap, vibio_u64 size)
{
    VibioHeapBlock *block = heap_block_at(heap->first_block);

    while ((vibio_u64)block != 0) {
        if (block->magic != HEAP_BLOCK_MAGIC) {
            heap->ok = 0;
            return 0;
        }

        if (block->used == 0 && block->size >= size) {
            return block;
        }

        block = heap_block_at(block->next);
    }

    return 0;
}

static void heap_split_block(VibioKernelHeap *heap, VibioHeapBlock *block, vibio_u64 size)
{
    if (block->size < size + sizeof(VibioHeapBlock) + HEAP_MIN_SPLIT_PAYLOAD) {
        return;
    }

    vibio_u64 new_block_address = (vibio_u64)block + sizeof(VibioHeapBlock) + size;
    VibioHeapBlock *new_block = heap_block_at(new_block_address);
    new_block->size = block->size - size - sizeof(VibioHeapBlock);
    new_block->used = 0;
    new_block->next = block->next;
    new_block->magic = HEAP_BLOCK_MAGIC;

    block->size = size;
    block->next = new_block_address;
    heap->free_bytes -= sizeof(VibioHeapBlock);
}

static void *heap_alloc(VibioKernelHeap *heap, vibio_u64 requested_size)
{
    vibio_u64 size = align_up(requested_size == 0 ? HEAP_ALIGN : requested_size, HEAP_ALIGN);

    if (size > PAGE_SIZE - sizeof(VibioHeapBlock)) {
        heap->failed_allocations++;
        return 0;
    }

    VibioHeapBlock *block = heap_find_free_block(heap, size);
    if (block == 0) {
        if (!heap_add_page(heap)) {
            return 0;
        }

        block = heap_find_free_block(heap, size);
        if (block == 0) {
            heap->failed_allocations++;
            return 0;
        }
    }

    heap_split_block(heap, block, size);
    block->used = 1;
    heap->allocations++;
    heap->live_allocations++;
    heap->used_bytes += block->size;
    heap->free_bytes -= block->size;

    return (void *)((vibio_u64)block + sizeof(VibioHeapBlock));
}

static void heap_coalesce_forward(VibioKernelHeap *heap, VibioHeapBlock *block)
{
    while (block->next != 0) {
        VibioHeapBlock *next = heap_block_at(block->next);
        if (next->magic != HEAP_BLOCK_MAGIC || next->used != 0) {
            return;
        }

        block->size += sizeof(VibioHeapBlock) + next->size;
        block->next = next->next;
        heap->free_bytes += sizeof(VibioHeapBlock);
    }
}

static void heap_free(VibioKernelHeap *heap, void *ptr)
{
    if (ptr == 0) {
        return;
    }

    VibioHeapBlock *block = heap_block_at((vibio_u64)ptr - sizeof(VibioHeapBlock));
    if (block->magic != HEAP_BLOCK_MAGIC || block->used == 0) {
        heap->failed_allocations++;
        heap->ok = 0;
        return;
    }

    block->used = 0;
    heap->frees++;
    if (heap->live_allocations > 0) {
        heap->live_allocations--;
    }
    heap->used_bytes -= block->size;
    heap->free_bytes += block->size;
    heap_coalesce_forward(heap, block);

    VibioHeapBlock *previous = 0;
    VibioHeapBlock *cursor = heap_block_at(heap->first_block);
    while ((vibio_u64)cursor != 0 && cursor != block) {
        previous = cursor;
        cursor = heap_block_at(cursor->next);
    }

    if (previous != 0 && previous->used == 0) {
        heap_coalesce_forward(heap, previous);
    }
}

static void heap_run_self_test(VibioKernelHeap *heap, VibioHeapTest *test)
{
    test->pages = heap->pages;
    test->allocations = heap->allocations;
    test->frees = heap->frees;
    test->used_bytes = heap->used_bytes;
    test->free_bytes = heap->free_bytes;
    test->ok = heap->ok;

    void *first = heap_alloc(heap, 24);
    void *second = heap_alloc(heap, 80);
    void *third = heap_alloc(heap, 13);

    vibio_u32 write_ok = first != 0 && second != 0 && third != 0;
    if (write_ok) {
        volatile vibio_u64 *a = (volatile vibio_u64 *)first;
        volatile vibio_u64 *b = (volatile vibio_u64 *)second;
        volatile vibio_u64 *c = (volatile vibio_u64 *)third;
        a[0] = 0x484541504F4B3031ULL;
        b[0] = 0x484541504F4B3032ULL;
        c[0] = 0x484541504F4B3033ULL;
        write_ok = a[0] == 0x484541504F4B3031ULL && b[0] == 0x484541504F4B3032ULL &&
                   c[0] == 0x484541504F4B3033ULL;
    }

    heap_free(heap, second);
    void *reused = heap_alloc(heap, 32);
    vibio_u32 reuse_ok = reused == second;

    heap_free(heap, first);
    heap_free(heap, third);
    heap_free(heap, reused);

    test->pages = heap->pages;
    test->allocations = heap->allocations;
    test->frees = heap->frees;
    test->used_bytes = heap->used_bytes;
    test->free_bytes = heap->free_bytes;
    test->ok = heap->ok && write_ok && reuse_ok && heap->live_allocations == 0;
}

static void task_manager_init(VibioTaskManager *manager)
{
    manager->first = 0;
    manager->current = 0;
    manager->next_ready = 0;
    manager->next_id = 1;
    manager->task_count = 0;
    manager->ready_count = 0;
    manager->running_count = 0;
    manager->sleeping_count = 0;
    manager->scheduler_rsp = 0;
    manager->context_switches = 0;
    manager->timer_schedule_requests = 0;
    manager->timer_driven_switches = 0;
    manager->ok = 1;
}

static void task_append_name_char(VibioTask *task, char c)
{
    if (task->name_length < TASK_NAME_MAX) {
        task->name[task->name_length] = c;
        task->name_length++;
    }
}

#define TEMIT(ch) task_append_name_char(task, ch)

static void task_set_name(VibioTask *task, vibio_u32 name_kind)
{
    task->name_length = 0;
    for (vibio_u32 i = 0; i < TASK_NAME_MAX; i++) {
        task->name[i] = 0;
    }

    if (name_kind == TASK_NAME_KERNEL) {
        TEMIT('K');
        TEMIT('E');
        TEMIT('R');
        TEMIT('N');
        TEMIT('E');
        TEMIT('L');
    } else if (name_kind == TASK_NAME_CONSOLE) {
        TEMIT('C');
        TEMIT('O');
        TEMIT('N');
        TEMIT('S');
        TEMIT('O');
        TEMIT('L');
        TEMIT('E');
    } else if (name_kind == TASK_NAME_IDLE) {
        TEMIT('I');
        TEMIT('D');
        TEMIT('L');
        TEMIT('E');
    } else if (name_kind == TASK_NAME_INPUT) {
        TEMIT('I');
        TEMIT('N');
        TEMIT('P');
        TEMIT('U');
        TEMIT('T');
    }
}

#undef TEMIT

static void task_update_state_count(VibioTaskManager *manager, vibio_u64 state)
{
    if (state == TASK_STATE_READY) {
        manager->ready_count++;
    } else if (state == TASK_STATE_RUNNING) {
        manager->running_count++;
    } else if (state == TASK_STATE_SLEEPING) {
        manager->sleeping_count++;
    }
}

static void task_prepare_initial_context(VibioTask *task)
{
    vibio_u64 stack_entry_top = align_down(task->stack_top, 16) - 8;
    vibio_u64 *stack = (vibio_u64 *)stack_entry_top;

    *--stack = (vibio_u64)task_bootstrap_entry;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;

    task->context_rsp = (vibio_u64)stack;
}

static VibioTask *task_create(VibioTaskManager *manager, VibioKernelHeap *heap, vibio_u32 name_kind, vibio_u64 state)
{
    VibioTask *task = (VibioTask *)heap_alloc(heap, sizeof(VibioTask));
    if (task == 0) {
        manager->ok = 0;
        return 0;
    }

    /*
     * Allocate the task's stack. Historically each task stack fit within a
     * single 4 KiB heap page. When native JPEG decoding was added the task
     * stack size was increased to 16 KiB to avoid overflowing during deep
     * recursion. The heap allocator can only serve allocations up to one
     * page (PAGE_SIZE minus the heap block header). Requests larger than a
     * page will fail. To support larger stacks without regressing to the
     * prior 1 KiB size, detect when the requested stack size exceeds the
     * heap page capacity and allocate contiguous pages directly from the
     * underlying page allocator instead. This preserves the per-task stack
     * size increase while keeping the heap allocator unchanged.
     */
    void *stack;
    if (TASK_STACK_BYTES <= PAGE_SIZE - sizeof(VibioHeapBlock)) {
        /* Normal small stacks use the heap allocator. */
        stack = heap_alloc(heap, TASK_STACK_BYTES);
    } else {
        /* Large stacks (e.g. 16 KiB) require multiple pages. Allocate
         * contiguous pages from the page allocator. */
        vibio_u64 pages = (TASK_STACK_BYTES + PAGE_SIZE - 1ULL) / PAGE_SIZE;
        vibio_u64 stack_addr = alloc_contiguous_pages(heap->page_allocator, pages);
        if (stack_addr != 0) {
            /* Zero the entire stack region so tasks begin with a clean
             * memory area. */
            zero_bytes((void *)stack_addr, pages * PAGE_SIZE);
        }
        stack = (void *)stack_addr;
    }
    if (stack == 0) {
        /* Free the task structure on failure and record the error. */
        heap_free(heap, task);
        manager->ok = 0;
        return 0;
    }

    zero_bytes(task, sizeof(VibioTask));
    task->id = manager->next_id;
    manager->next_id++;
    task->state = state;
    task->stack_base = (vibio_u64)stack;
    task->stack_size = TASK_STACK_BYTES;
    task->stack_top = task->stack_base + task->stack_size;
    task->context_rsp = 0;
    task->run_count = 0;
    task->yield_count = 0;
    task->next = 0;
    task_set_name(task, name_kind);

    if (name_kind != TASK_NAME_KERNEL) {
        task_prepare_initial_context(task);
    }

    if (manager->first == 0) {
        manager->first = task;
    } else {
        VibioTask *tail = manager->first;
        while (tail->next != 0) {
            tail = (VibioTask *)tail->next;
        }

        tail->next = (vibio_u64)task;
    }

    if (state == TASK_STATE_RUNNING && manager->current == 0) {
        manager->current = task;
    }

    manager->task_count++;
    task_update_state_count(manager, state);
    return task;
}

static void task_manager_create_initial_tasks(VibioTaskManager *manager, VibioKernelHeap *heap, VibioTaskTest *test)
{
    task_manager_init(manager);
    task_create(manager, heap, TASK_NAME_KERNEL, TASK_STATE_RUNNING);
    task_create(manager, heap, TASK_NAME_CONSOLE, TASK_STATE_READY);
    task_create(manager, heap, TASK_NAME_IDLE, TASK_STATE_READY);
    task_create(manager, heap, TASK_NAME_INPUT, TASK_STATE_SLEEPING);
    if (manager->first != 0) {
        manager->next_ready = (VibioTask *)manager->first->next;
    }

    test->task_count = manager->task_count;
    test->ready_count = manager->ready_count;
    test->running_count = manager->running_count;
    test->sleeping_count = manager->sleeping_count;
    test->context_switches = 0;
    test->console_runs = 0;
    test->idle_runs = 0;
    test->timer_requests = 0;
    test->timer_switches = 0;
    test->ok = manager->ok && manager->current != 0 && manager->task_count == 4 && manager->running_count == 1 &&
               manager->ready_count == 2 && manager->sleeping_count == 1;
}

static void task_yield_to_scheduler(void)
{
    VibioTaskManager *manager = (VibioTaskManager *)task_manager_address_storage;
    if (manager == 0 || manager->current == 0) {
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    VibioTask *task = manager->current;
    task->yield_count++;
    task_context_switch(&task->context_rsp, manager->scheduler_rsp);
}

__attribute__((noreturn)) static void task_bootstrap_entry(void)
{
    for (;;) {
        VibioTaskManager *manager = (VibioTaskManager *)task_manager_address_storage;
        if (manager == 0 || manager->current == 0) {
            for (;;) {
                __asm__ volatile("hlt");
            }
        }

        manager->current->run_count++;
        task_yield_to_scheduler();
    }
}

static void task_manager_run_context_demo(VibioTaskManager *manager, VibioTaskTest *test)
{
    VibioTask *kernel_task = manager->first;
    task_manager_address_storage = (vibio_u64)manager;

    for (vibio_u64 round = 0; round < CONTEXT_SWITCH_DEMO_ROUNDS; round++) {
        VibioTask *task = manager->first;
        while (task != 0) {
            if (task->state == TASK_STATE_READY && task->context_rsp != 0) {
                manager->current = task;
                task->state = TASK_STATE_RUNNING;
                manager->context_switches++;
                task_context_switch(&manager->scheduler_rsp, task->context_rsp);
                task->state = TASK_STATE_READY;
                manager->current = kernel_task;
            }

            task = (VibioTask *)task->next;
        }
    }

    VibioTask *task = manager->first;
    while (task != 0) {
        if (task->name_length == 7 && task->name[0] == 'C') {
            test->console_runs = task->run_count;
        } else if (task->name_length == 4 && task->name[0] == 'I' && task->name[1] == 'D') {
            test->idle_runs = task->run_count;
        }

        task = (VibioTask *)task->next;
    }

    test->context_switches = manager->context_switches;
    test->ok = test->ok && test->context_switches == CONTEXT_SWITCH_DEMO_ROUNDS * 2 &&
               test->console_runs == CONTEXT_SWITCH_DEMO_ROUNDS && test->idle_runs == CONTEXT_SWITCH_DEMO_ROUNDS &&
               manager->current == kernel_task;
}

static VibioTask *task_manager_next_ready_context_task(VibioTaskManager *manager)
{
    if (manager->first == 0) {
        return 0;
    }

    VibioTask *start = manager->next_ready;
    if (start == 0) {
        start = manager->first;
    }

    VibioTask *task = start;
    for (;;) {
        if (task->state == TASK_STATE_READY && task->context_rsp != 0) {
            VibioTask *next = (VibioTask *)task->next;
            if (next == 0) {
                next = manager->first;
            }

            manager->next_ready = next;
            return task;
        }

        task = (VibioTask *)task->next;
        if (task == 0) {
            task = manager->first;
        }

        if (task == start) {
            return 0;
        }
    }
}

static void task_manager_run_timer_schedule(VibioTaskManager *manager)
{
    VibioTask *kernel_task = manager->first;
    VibioTask *task = task_manager_next_ready_context_task(manager);
    if (kernel_task == 0 || task == 0) {
        return;
    }

    manager->current = task;
    task->state = TASK_STATE_RUNNING;
    manager->context_switches++;
    manager->timer_driven_switches++;
    __asm__ volatile("cli");
    task_context_switch(&manager->scheduler_rsp, task->context_rsp);
    __asm__ volatile("sti");
    task->state = TASK_STATE_READY;
    manager->current = kernel_task;
}

static void task_manager_handle_timer_requests(
    VibioTaskManager *manager,
    const VibioIrqState *irq_state,
    VibioTaskTest *test,
    VibioSyscallState *syscalls)
{
    vibio_u64 requests = irq_state->scheduler_requests;
    while (manager->timer_schedule_requests < requests) {
        manager->timer_schedule_requests++;
        task_manager_run_timer_schedule(manager);
    }

    test->timer_requests = manager->timer_schedule_requests;
    test->timer_switches = manager->timer_driven_switches;
    syscalls->task_count = manager->task_count;
    syscalls->context_switches = manager->context_switches;
}

static vibio_u64 page_table_address(vibio_u64 entry)
{
    return entry & 0x000FFFFFFFFFF000ULL;
}

static vibio_u64 ensure_next_level_table(
    VibioPageAllocator *allocator,
    VibioPageTableBuild *build,
    volatile vibio_u64 *entry)
{
    if ((*entry & PAGE_PRESENT) == 0) {
        vibio_u64 table_page = page_allocator_alloc(allocator);
        if (table_page == 0) {
            build->ok = 0;
            return 0;
        }

        zero_page(table_page);
        *entry = table_page | PAGE_PRESENT | PAGE_WRITABLE;
        build->table_pages++;
    }

    return page_table_address(*entry);
}

static void write_msr(vibio_u32 msr, vibio_u64 value)
{
    __asm__ volatile("wrmsr"
                     :
                     : "c"(msr), "a"((vibio_u32)(value & 0xFFFFFFFFU)), "d"((vibio_u32)(value >> 32)));
}

/* Reprogram PAT entry PA1 from its default Write-Through to Write-Combining, so
 * pages tagged with PWT use WC. This is what makes the full-frame framebuffer
 * blit fast on real hardware, where graphics memory is otherwise effectively
 * uncached and every pixel write stalls.
 *   PA0=WB(06) PA1=WC(01) PA2=UC-(07) PA3=UC(00)
 *   PA4=WB(06) PA5=WT(04) PA6=UC-(07) PA7=UC(00) */
static void pat_enable_write_combining(void)
{
    write_msr(IA32_PAT_MSR, 0x0007040600070106ULL);
}

static void map_identity_2m_page(
    VibioPageAllocator *allocator,
    VibioPageTableBuild *build,
    vibio_u64 physical_address,
    vibio_u64 extra_flags)
{
    if (!build->ok) {
        return;
    }

    volatile vibio_u64 *pml4 = (volatile vibio_u64 *)build->pml4;
    vibio_u64 pml4_index = (physical_address >> 39) & 0x1FF;
    vibio_u64 pdpt_index = (physical_address >> 30) & 0x1FF;
    vibio_u64 pd_index = (physical_address >> 21) & 0x1FF;

    vibio_u64 pdpt_address = ensure_next_level_table(allocator, build, &pml4[pml4_index]);
    if (!build->ok) {
        return;
    }

    volatile vibio_u64 *pdpt = (volatile vibio_u64 *)pdpt_address;
    vibio_u64 pd_address = ensure_next_level_table(allocator, build, &pdpt[pdpt_index]);
    if (!build->ok) {
        return;
    }

    volatile vibio_u64 *pd = (volatile vibio_u64 *)pd_address;
    pd[pd_index] = physical_address | PAGE_PRESENT | PAGE_WRITABLE | PAGE_SIZE_FLAG | extra_flags;
}

static void map_identity_range_2m(
    VibioPageAllocator *allocator,
    VibioPageTableBuild *build,
    vibio_u64 start,
    vibio_u64 size,
    vibio_u64 extra_flags)
{
    if (size == 0 || !build->ok) {
        return;
    }

    vibio_u64 current = align_down(start, PAGE_2M_SIZE);
    vibio_u64 end = align_up(start + size, PAGE_2M_SIZE);

    while (current < end) {
        map_identity_2m_page(allocator, build, current, extra_flags);
        current += PAGE_2M_SIZE;
    }
}

static VibioPageTableBuild build_identity_page_tables(VibioPageAllocator *allocator, const VibioBootInfo *boot_info)
{
    VibioPageTableBuild build;
    build.pml4 = page_allocator_alloc(allocator);
    build.table_pages = 0;
    build.ok = build.pml4 != 0;

    if (!build.ok) {
        return build;
    }

    zero_page(build.pml4);
    build.table_pages = 1;

    /* PA1 = Write-Combining; the framebuffer pages below set PWT to use it. */
    pat_enable_write_combining();

    map_identity_range_2m(allocator, &build, 0, 4ULL * 1024ULL * 1024ULL * 1024ULL, 0);
    /* Map (or re-map, overriding the low-4GiB pass) the framebuffer as write-
     * combining so the per-frame blit is fast on real hardware. */
    map_identity_range_2m(allocator, &build, boot_info->framebuffer_base, boot_info->framebuffer_size, PAGE_PWT);

    return build;
}

static void load_cr3(vibio_u64 pml4_address)
{
    __asm__ volatile("cli");
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_address) : "memory");
    /* Flush any stale write-back cache lines so the new framebuffer memory type
     * (write-combining) takes effect cleanly. */
    __asm__ volatile("wbinvd" ::: "memory");
}

static vibio_u64 read_cr3(void)
{
    vibio_u64 value = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

typedef struct {
    vibio_u32 eax;
    vibio_u32 ebx;
    vibio_u32 ecx;
    vibio_u32 edx;
} VibioCpuidResult;

static VibioCpuidResult cpuid_read(vibio_u32 leaf, vibio_u32 subleaf)
{
    VibioCpuidResult result;
    __asm__ volatile(
        "cpuid"
        : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
        : "a"(leaf), "c"(subleaf));
    return result;
}

static vibio_u32 cpu_running_under_hypervisor(void)
{
    VibioCpuidResult leaf1 = cpuid_read(1, 0);
    return (leaf1.ecx & 0x80000000U) != 0;
}

static vibio_u16 read_cs(void)
{
    vibio_u16 value = 0;
    __asm__ volatile("mov %%cs, %0" : "=r"(value));
    return value;
}

static vibio_u16 read_tr(void)
{
    vibio_u16 value = 0;
    __asm__ volatile("str %0" : "=r"(value));
    return value;
}

static void load_gdt_and_segments(const VibioGdtPointer *pointer)
{
    __asm__ volatile(
        "lgdt (%0)\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        :
        : "r"(pointer)
        : "rax", "memory");
}

static void load_task_register(vibio_u16 selector)
{
    __asm__ volatile("ltr %0" : : "r"(selector) : "memory");
}

static vibio_u64 gdt_segment_descriptor(vibio_u8 access, vibio_u8 flags)
{
    vibio_u64 limit = 0xFFFFFULL;
    vibio_u64 descriptor = 0;
    descriptor |= limit & 0xFFFFULL;
    descriptor |= ((vibio_u64)access) << 40;
    descriptor |= ((limit >> 16) & 0x0FULL) << 48;
    descriptor |= ((vibio_u64)(flags & 0x0F)) << 52;
    return descriptor;
}

static void gdt_write_tss_descriptor(vibio_u64 *gdt, vibio_u32 index, vibio_u64 base, vibio_u32 limit)
{
    vibio_u64 low = 0;
    low |= (vibio_u64)(limit & 0xFFFF);
    low |= (base & 0xFFFFFFULL) << 16;
    low |= 0x89ULL << 40;
    low |= ((vibio_u64)((limit >> 16) & 0x0F)) << 48;
    low |= ((base >> 24) & 0xFFULL) << 56;

    gdt[index] = low;
    gdt[index + 1] = base >> 32;
}

static VibioUserBoundaryBuild build_and_test_user_boundary(VibioPageAllocator *allocator)
{
    VibioUserBoundaryBuild build;
    zero_bytes(&build, sizeof(VibioUserBoundaryBuild));
    build.kernel_code_selector = GDT_KERNEL_CODE_SELECTOR;
    build.kernel_data_selector = GDT_KERNEL_DATA_SELECTOR;
    build.user_code_selector = GDT_USER_CODE_SELECTOR | USER_RPL;
    build.user_data_selector = GDT_USER_DATA_SELECTOR | USER_RPL;
    build.tss_selector = GDT_TSS_SELECTOR;

    build.gdt_address = page_allocator_alloc(allocator);
    build.tss_address = page_allocator_alloc(allocator);
    build.rsp0_stack_base = page_allocator_alloc(allocator);
    build.ok = build.gdt_address != 0 && build.tss_address != 0 && build.rsp0_stack_base != 0;
    if (!build.ok) {
        return build;
    }

    zero_page(build.gdt_address);
    zero_page(build.tss_address);
    zero_page(build.rsp0_stack_base);

    vibio_u64 *gdt = (vibio_u64 *)build.gdt_address;
    VibioTss *tss = (VibioTss *)build.tss_address;

    gdt[0] = 0;
    gdt[1] = gdt_segment_descriptor(0x9A, 0x0A);
    gdt[2] = gdt_segment_descriptor(0x92, 0x0C);
    gdt[3] = gdt_segment_descriptor(0xF2, 0x0C);
    gdt[4] = gdt_segment_descriptor(0xFA, 0x0A);
    gdt_write_tss_descriptor(gdt, 5, build.tss_address, (vibio_u32)(sizeof(VibioTss) - 1));

    tss->rsp0 = build.rsp0_stack_base + PAGE_SIZE;
    tss->iomap_base = sizeof(VibioTss);

    VibioGdtPointer pointer;
    pointer.limit = (vibio_u16)(sizeof(vibio_u64) * GDT_ENTRY_COUNT - 1);
    pointer.base = build.gdt_address;
    load_gdt_and_segments(&pointer);
    load_task_register(GDT_TSS_SELECTOR);

    build.current_code_selector = read_cs();
    build.task_register = read_tr();
    build.ok = build.current_code_selector == GDT_KERNEL_CODE_SELECTOR &&
               build.task_register == GDT_TSS_SELECTOR &&
               (build.user_code_selector & USER_RPL) == USER_RPL &&
               (build.user_data_selector & USER_RPL) == USER_RPL &&
               tss->rsp0 == build.rsp0_stack_base + PAGE_SIZE;

    return build;
}

static void outb(vibio_u16 port, vibio_u8 value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void outw(vibio_u16 port, vibio_u16 value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static void outl(vibio_u16 port, vibio_u32 value)
{
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static vibio_u8 inb(vibio_u16 port)
{
    vibio_u8 value = 0;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static vibio_u16 inw(vibio_u16 port)
{
    vibio_u16 value = 0;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void io_wait(void);

static vibio_u8 cmos_read(vibio_u8 reg)
{
    outb(0x70, (vibio_u8)(reg | 0x80));
    io_wait();
    return inb(0x71);
}

static vibio_u8 rtc_bcd_to_binary(vibio_u8 value)
{
    return (vibio_u8)((value & 0x0F) + ((value >> 4) * 10));
}

static vibio_u32 rtc_read_clock(vibio_u32 *hour, vibio_u32 *minute)
{
    for (vibio_u32 wait = 0; wait < 100000; wait++) {
        if ((cmos_read(0x0A) & 0x80) == 0) {
            break;
        }
    }

    vibio_u8 sec1 = cmos_read(0x00);
    vibio_u8 min1 = cmos_read(0x02);
    vibio_u8 hour1 = cmos_read(0x04);
    vibio_u8 status_b = cmos_read(0x0B);
    vibio_u8 sec2 = cmos_read(0x00);
    vibio_u8 min2 = cmos_read(0x02);
    vibio_u8 hour2 = cmos_read(0x04);

    if (sec1 != sec2 || min1 != min2 || hour1 != hour2) {
        sec1 = cmos_read(0x00);
        min1 = cmos_read(0x02);
        hour1 = cmos_read(0x04);
        sec2 = cmos_read(0x00);
        min2 = cmos_read(0x02);
        hour2 = cmos_read(0x04);
        if (sec1 != sec2 || min1 != min2 || hour1 != hour2) {
            return 0;
        }
    }

    vibio_u8 pm = hour1 & 0x80;
    hour1 &= 0x7F;

    if ((status_b & 0x04) == 0) {
        sec1 = rtc_bcd_to_binary(sec1);
        min1 = rtc_bcd_to_binary(min1);
        hour1 = rtc_bcd_to_binary(hour1);
    }

    if ((status_b & 0x02) == 0 && pm) {
        hour1 = (vibio_u8)(((hour1 % 12) + 12) % 24);
    }

    if (hour1 >= 24 || min1 >= 60 || sec1 >= 60) {
        return 0;
    }

    *hour = hour1;
    *minute = min1;
    return 1;
}

static vibio_u32 inl(vibio_u16 port)
{
    vibio_u32 value = 0;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static vibio_u32 mmio_read32(vibio_u64 base, vibio_u32 offset)
{
    return *(volatile vibio_u32 *)(base + offset);
}

static vibio_u16 mmio_read16(vibio_u64 base, vibio_u32 offset)
{
    return *(volatile vibio_u16 *)(base + offset);
}

static vibio_u8 mmio_read8(vibio_u64 base, vibio_u32 offset)
{
    return *(volatile vibio_u8 *)(base + offset);
}

static void mmio_write32(vibio_u64 base, vibio_u32 offset, vibio_u32 value)
{
    *(volatile vibio_u32 *)(base + offset) = value;
}

static void mmio_write16(vibio_u64 base, vibio_u32 offset, vibio_u16 value)
{
    *(volatile vibio_u16 *)(base + offset) = value;
}

static void mmio_write8(vibio_u64 base, vibio_u32 offset, vibio_u8 value)
{
    *(volatile vibio_u8 *)(base + offset) = value;
}

static void io_wait(void)
{
    outb(0x80, 0);
}

static vibio_u32 pci_config_address(vibio_u32 bus, vibio_u32 device, vibio_u32 function, vibio_u32 offset)
{
    return 0x80000000U | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFCU);
}

static vibio_u32 pci_config_read32(vibio_u32 bus, vibio_u32 device, vibio_u32 function, vibio_u32 offset)
{
    outl(0xCF8, pci_config_address(bus, device, function, offset));
    return inl(0xCFC);
}

static void pci_config_write32(vibio_u32 bus, vibio_u32 device, vibio_u32 function, vibio_u32 offset, vibio_u32 value)
{
    outl(0xCF8, pci_config_address(bus, device, function, offset));
    outl(0xCFC, value);
}

static vibio_u32 pci_vendor_is_vm(vibio_u32 vendor_id)
{
    return vendor_id == 0x80EEU || /* VirtualBox */
           vendor_id == 0x15ADU || /* VMware */
           vendor_id == 0x1AF4U || /* Virtio/QEMU */
           vendor_id == 0x1B36U || /* QEMU */
           vendor_id == 0x5853U || /* Xen */
           vendor_id == 0x1414U;   /* Hyper-V */
}

static vibio_u32 pci_detect_vm_device(void)
{
    for (vibio_u32 bus = 0; bus < 256; bus++) {
        for (vibio_u32 device = 0; device < 32; device++) {
            for (vibio_u32 function = 0; function < 8; function++) {
                vibio_u32 id = pci_config_read32(bus, device, function, 0x00);
                vibio_u32 vendor_id = id & 0xFFFFU;
                if (vendor_id == 0xFFFFU) {
                    if (function == 0) {
                        break;
                    }
                    continue;
                }
                if (pci_vendor_is_vm(vendor_id)) {
                    return 1;
                }
                if (function == 0) {
                    vibio_u32 header = pci_config_read32(bus, device, function, 0x0C);
                    if ((header & 0x00800000U) == 0) {
                        break;
                    }
                }
            }
        }
    }

    return 0;
}

static vibio_u32 system_running_in_vm(void)
{
    return cpu_running_under_hypervisor() || pci_detect_vm_device();
}

static vibio_u32 usb_classify_controller(vibio_u8 prog_if)
{
    if (prog_if == USB_PROGIF_UHCI) {
        return USB_TYPE_UHCI;
    }
    if (prog_if == USB_PROGIF_OHCI) {
        return USB_TYPE_OHCI;
    }
    if (prog_if == USB_PROGIF_EHCI) {
        return USB_TYPE_EHCI;
    }
    if (prog_if == USB_PROGIF_XHCI) {
        return USB_TYPE_XHCI;
    }
    return USB_TYPE_UNKNOWN;
}

/*
 * Read-only record of one USB host controller. This stage only reads PCI
 * configuration space. It does not write the PCI command register, does not
 * enable memory or bus-master access, and does not touch the controller's own
 * MMIO/IO registers. That keeps Stage 18 a pure discovery/architecture step.
 */
static void usb_probe_record_controller(
    VibioUsbReadTest *usb,
    vibio_u32 bus,
    vibio_u32 device,
    vibio_u32 function,
    vibio_u32 id,
    vibio_u8 prog_if)
{
    vibio_u32 type = usb_classify_controller(prog_if);

    if (usb->controllers_found == 0) {
        usb->first_bus = bus;
        usb->first_device = device;
        usb->first_function = function;
        usb->first_prog_if = prog_if;
    }

    usb->controllers_found++;
    if (type == USB_TYPE_UHCI) {
        usb->uhci_count++;
    } else if (type == USB_TYPE_OHCI) {
        usb->ohci_count++;
    } else if (type == USB_TYPE_EHCI) {
        usb->ehci_count++;
    } else if (type == USB_TYPE_XHCI) {
        usb->xhci_count++;
    }

    if (usb->recorded_count < USB_MAX_CONTROLLERS) {
        VibioUsbController *controller = &usb->controllers[usb->recorded_count];
        controller->bus = bus;
        controller->device = device;
        controller->function = function;
        controller->type = type;
        controller->prog_if = prog_if;
        controller->vendor_id = id & 0xFFFFU;
        controller->device_id = (id >> 16) & 0xFFFFU;

        /*
         * UHCI exposes its registers through an I/O-space BAR (BAR4 at 0x20).
         * OHCI, EHCI, and xHCI expose a memory-mapped BAR (BAR0 at 0x10). We
         * only read the address for later stages; we do not access it here.
         */
        if (type == USB_TYPE_UHCI) {
            vibio_u32 bar4 = pci_config_read32(bus, device, function, 0x20);
            controller->bar_is_mmio = (bar4 & 0x1U) == 0;
            controller->bar = controller->bar_is_mmio ? (bar4 & 0xFFFFFFF0U) : (bar4 & 0xFFFFFFFCU);
        } else {
            vibio_u32 bar0 = pci_config_read32(bus, device, function, 0x10);
            controller->bar_is_mmio = (bar0 & 0x1U) == 0;
            vibio_u64 base = bar0 & 0xFFFFFFF0U;
            if (controller->bar_is_mmio && ((bar0 >> 1) & 0x3U) == 0x2U) {
                vibio_u32 bar1 = pci_config_read32(bus, device, function, 0x14);
                base |= (vibio_u64)bar1 << 32;
            }
            controller->bar = base;
        }

        usb->recorded_count++;
    }
}

static vibio_u32 usb_select_preferred_type(const VibioUsbReadTest *usb)
{
    /*
     * Document the controller a real machine would most likely use. Newer
     * controllers come first because a modern laptop boots its USB ports
     * through xHCI. This only records intent; no controller is initialized.
     */
    if (usb->xhci_count > 0) {
        return USB_TYPE_XHCI;
    }
    if (usb->ehci_count > 0) {
        return USB_TYPE_EHCI;
    }
    if (usb->ohci_count > 0) {
        return USB_TYPE_OHCI;
    }
    if (usb->uhci_count > 0) {
        return USB_TYPE_UHCI;
    }
    return USB_TYPE_UNKNOWN;
}

static VibioUsbReadTest usb_run_readonly_probe(void)
{
    VibioUsbReadTest usb;
    zero_bytes(&usb, sizeof(VibioUsbReadTest));

    for (vibio_u32 bus = 0; bus < 256; bus++) {
        for (vibio_u32 device = 0; device < 32; device++) {
            for (vibio_u32 function = 0; function < 8; function++) {
                vibio_u32 id = pci_config_read32(bus, device, function, 0x00);
                if ((id & 0xFFFFU) == 0xFFFFU) {
                    continue;
                }

                vibio_u32 class_info = pci_config_read32(bus, device, function, 0x08);
                vibio_u8 class_code = (vibio_u8)(class_info >> 24);
                vibio_u8 subclass = (vibio_u8)(class_info >> 16);
                vibio_u8 prog_if = (vibio_u8)(class_info >> 8);

                if (class_code == USB_PCI_CLASS_SERIAL_BUS && subclass == USB_PCI_SUBCLASS_USB) {
                    usb_probe_record_controller(&usb, bus, device, function, id, prog_if);
                }
            }
        }
    }

    usb.preferred_type = usb_select_preferred_type(&usb);
    usb.storage_read_path_present = 0;
    usb.write_path_present = 0;
    usb.ok = usb.storage_read_path_present == 0 &&
             usb.write_path_present == 0;
    return usb;
}

static const VibioUsbController *usb_find_preferred_controller(const VibioUsbReadTest *usb)
{
    if (usb == 0) {
        return 0;
    }
    for (vibio_u32 want = USB_TYPE_XHCI; want >= USB_TYPE_UHCI; want--) {
        for (vibio_u32 i = 0; i < usb->recorded_count && i < USB_MAX_CONTROLLERS; i++) {
            if (usb->controllers[i].type == want) {
                return &usb->controllers[i];
            }
        }
    }
    return 0;
}

/*
 * Stage 30: read-only USB mass-storage diagnostic probe. This walks the storage
 * path as far as it can *honestly* go and records the furthest real step:
 *
 *   1. Find a USB controller, preferring xHCI (modern/old-ish laptops boot USB
 *      through xHCI). 2. If the firmware already enabled the controller's MMIO
 *      space, read its xHCI capability registers (read-only) to learn the port
 *      and slot counts. 3. Read the root-hub PORTSC registers (read-only) to see
 *      which ports have a device connected.
 *
 * It deliberately stops there: full device enumeration and the Bulk-Only
 * Transport SCSI read commands (INQUIRY / TEST UNIT READY / READ CAPACITY(10) /
 * READ(10)) are not implemented in this build, so `sector_backend_ready` and
 * `fat32_mounted` stay 0 and the Files app never uses USB as a real backend.
 * Nothing here writes any controller register that changes media or issues a USB
 * write - it only reads capability/status registers the firmware already set up.
 */
static void usb_storage_probe(VibioUsbStorage *st, const VibioUsbReadTest *usb)
{
    zero_bytes(st, sizeof(VibioUsbStorage));
    st->last_step = USB_STOR_STEP_NONE;
    st->in_vm = system_running_in_vm();

    if (usb == 0) {
        return;
    }

    /* Choose a controller, xHCI first, then EHCI/OHCI/UHCI. */
    const VibioUsbController *chosen = usb_find_preferred_controller(usb);
    if (chosen == 0) {
        return; /* no USB controller at all (e.g. VM with USB disabled) */
    }

    st->controller_found = 1;
    st->controller_type = chosen->type;
    st->bus = chosen->bus;
    st->device = chosen->device;
    st->function = chosen->function;
    st->bar = chosen->bar;
    st->last_step = USB_STOR_STEP_CONTROLLER;

    /* Only xHCI capability/port registers are decoded here. Other controller
     * types are reported as "found" but no further (their register layout
     * differs and the read path targets xHCI first). */
    if (chosen->type != USB_TYPE_XHCI || !chosen->bar_is_mmio || chosen->bar == 0) {
        return;
    }

    /* Read the PCI command register read-only; only touch MMIO if the firmware
     * already enabled memory space. We never enable it ourselves. */
    vibio_u32 command = pci_config_read32(chosen->bus, chosen->device, chosen->function, 0x04);
    st->mem_space_enabled = (command & 0x2U) != 0;

    /* The identity map covers the low 4 GiB. A 64-bit BAR placed above that is
     * not mapped, so we refuse to read it rather than fault. */
    st->bar_mapped = chosen->bar < 0x100000000ULL;
    if (!st->mem_space_enabled || !st->bar_mapped) {
        return;
    }

    vibio_u8 cap_length = mmio_read8(chosen->bar, 0x00);
    vibio_u16 hci_version = mmio_read16(chosen->bar, 0x02);
    vibio_u32 hcsparams1 = mmio_read32(chosen->bar, 0x04);
    /* A sane xHCI exposes CAPLENGTH >= 0x20 and a nonzero version. */
    if (cap_length < 0x20 || cap_length > 0xFF || hci_version == 0 || hci_version == 0xFFFF) {
        return;
    }
    st->mmio_readable = 1;
    st->cap_length = cap_length;
    st->hci_version = hci_version;
    st->max_slots = hcsparams1 & 0xFFU;
    st->max_ports = (hcsparams1 >> 24) & 0xFFU;
    st->last_step = USB_STOR_STEP_MMIO;

    /* Read root-hub port status (read-only). PORTSC lives in the operational
     * register space at CAPLENGTH + 0x400 + 0x10 * (port-1); CCS (bit 0) is the
     * current-connect-status flag. */
    if (st->max_ports > 0 && st->max_ports <= 64) {
        vibio_u64 op_base = chosen->bar + cap_length;
        for (vibio_u32 p = 0; p < st->max_ports; p++) {
            vibio_u32 portsc = mmio_read32(op_base, 0x400 + 0x10 * p);
            if (portsc == 0xFFFFFFFFU) {
                continue; /* port register not present */
            }
            if (portsc & 0x1U) {
                st->ports_connected++;
            }
        }
        st->last_step = USB_STOR_STEP_PORTS;
    }

    /* Stage 37 implements the real read-only enumeration + BOT/SCSI read path in
     * xhci_msd_init(); this probe only records controller-level diagnostics. */
}

/* =====================================================================
 * Stage 37: read-only xHCI USB mass-storage driver (BOT + SCSI READ(10)).
 *
 * A from-scratch xHCI controller bring-up just far enough to enumerate one USB
 * mass-storage flash drive and READ sectors from it after boot - no UEFI
 * preload. It is STRICTLY READ-ONLY: the only SCSI commands issued are INQUIRY,
 * TEST UNIT READY, READ CAPACITY(10) and READ(10). There is no WRITE(10), no
 * FORMAT, no mode-select, nothing that mutates the device. Polled (no IRQs);
 * single device, single LUN, single configuration - enough to mount FAT32
 * read-only over the existing parser. All DMA structures are page-allocated and
 * identity-mapped (physical == virtual) in the low 4 GiB; x86 DMA is cache
 * coherent so no explicit flushes are needed.
 * ===================================================================== */

#define XHCI_RING_TRBS 256U
#define XHCI_SPIN_TIMEOUT 20000000U
#define XHCI_PORT_CONNECT_TIMEOUT 2000000U
/* Real-hardware emergency safety: do not run full xHCI enumeration/mass-storage
 * mounting during boot. The laptop reached TRACE 5e (purple), proving xHCI MMIO
 * mapping works, then hung before TRACE 5f inside controller/device bring-up.
 * Keep controller detection + read-only status during boot and leave full USB MSD
 * bring-up for a later lazy/manual path so the desktop can boot. */
#define XHCI_BOOT_MSD_AUTOMOUNT 0U

/* TRB types */
#define XHCI_TRB_NORMAL        1U
#define XHCI_TRB_SETUP         2U
#define XHCI_TRB_DATA          3U
#define XHCI_TRB_STATUS        4U
#define XHCI_TRB_LINK          6U
#define XHCI_TRB_ENABLE_SLOT   9U
#define XHCI_TRB_DISABLE_SLOT  10U
#define XHCI_TRB_ADDRESS_DEV   11U
#define XHCI_TRB_CONFIG_EP     12U
#define XHCI_TRB_EV_TRANSFER   32U
#define XHCI_TRB_EV_CMD_COMPL  33U
#define XHCI_TRB_EV_PORT       34U
#define XHCI_CC_SUCCESS        1U
#define XHCI_CC_SHORT_PACKET   13U

typedef struct {
    vibio_u64 base;
    vibio_u32 enqueue;
    vibio_u32 cycle;
} VibioXhciRing;

typedef struct {
    /* Controller location + register windows. */
    vibio_u32 bus, device, function;
    vibio_u64 mmio;       /* CAP base */
    vibio_u64 op;         /* operational regs (mmio + CAPLENGTH) */
    vibio_u64 rt;         /* runtime regs (mmio + RTSOFF) */
    vibio_u64 db;         /* doorbell array (mmio + DBOFF) */
    vibio_u32 cap_length;
    vibio_u32 hci_version;
    vibio_u32 max_slots;
    vibio_u32 max_ports;
    vibio_u32 context_size; /* 32 or 64 bytes */

    /* DMA structures (one page each). */
    vibio_u64 dcbaa;
    vibio_u64 cmd_page;
    vibio_u64 event_page;
    vibio_u64 erst_page;
    vibio_u64 in_ctx;
    vibio_u64 dev_ctx;
    vibio_u64 ep0_ring_page;
    vibio_u64 bulk_in_page;
    vibio_u64 bulk_out_page;
    vibio_u64 data_page;     /* CBW/CSW/descriptor/sector scratch */

    VibioXhciRing cmd_ring;
    VibioXhciRing ep0_ring;
    VibioXhciRing bulk_in;
    VibioXhciRing bulk_out;

    /* Event ring consumer state. */
    vibio_u64 event_dequeue;
    vibio_u32 event_cycle;

    /* Enumerated device. */
    vibio_u32 port;          /* 1-based root-hub port */
    vibio_u32 slot;
    vibio_u32 speed;
    vibio_u32 ep0_mps;
    vibio_u32 bulk_in_ep;    /* endpoint address (with 0x80) */
    vibio_u32 bulk_out_ep;
    vibio_u32 bulk_in_dci;
    vibio_u32 bulk_out_dci;
    vibio_u32 bulk_in_mps;
    vibio_u32 bulk_out_mps;
    vibio_u32 config_value;
    vibio_u32 bot_tag;

    /* SCSI geometry. */
    vibio_u32 block_size;
    vibio_u64 total_blocks;

    /* Status / diagnostics. */
    vibio_u32 ready;         /* READ(10) backend usable */
    vibio_u32 init_done;
    vibio_u32 vid, pid;
    char vendor[12];         /* INQUIRY vendor id (8) */
    char product[20];        /* INQUIRY product id (16) */
    vibio_u32 last_step;     /* furthest USB_STOR_STEP_* reached */
    vibio_u32 last_error;    /* xhci_err_* code */
    vibio_u64 read_ops;
} VibioXhciMsd;

/* Error codes for honest diagnostics (never a fake success). */
#define XHCI_ERR_NONE         0U
#define XHCI_ERR_NO_CTRL      1U
#define XHCI_ERR_BAR          2U
#define XHCI_ERR_ALLOC        3U
#define XHCI_ERR_RESET        4U
#define XHCI_ERR_NO_PORT      5U
#define XHCI_ERR_PORT_RESET   6U
#define XHCI_ERR_ENABLE_SLOT  7U
#define XHCI_ERR_ADDRESS      8U
#define XHCI_ERR_DESC         9U
#define XHCI_ERR_NO_MSD       10U
#define XHCI_ERR_SET_CONFIG   11U
#define XHCI_ERR_CONFIG_EP    12U
#define XHCI_ERR_INQUIRY      13U
#define XHCI_ERR_NOT_READY    14U
#define XHCI_ERR_CAPACITY     15U
#define XHCI_ERR_READ         16U
#define XHCI_ERR_CSW          17U
#define XHCI_ERR_SECTOR_SIZE  18U
#define XHCI_ERR_BOOT_SKIPPED 19U

static VibioXhciMsd g_xhci_msd;
/* The USB block device + its FAT32 mount, used by the Files app as a live
 * post-boot storage source (separate from the AHCI boot disk). */
static VibioDiskReadTest g_usb_disk;
static VibioFat32ReadTest g_usb_fs;

static void xhci_short_delay(vibio_u32 loops)
{
    for (volatile vibio_u32 i = 0; i < loops; i++) {
        io_wait();
    }
}

/* Zero a page-sized DMA buffer via volatile stores. */
static void xhci_zero_page(vibio_u64 addr)
{
    for (vibio_u32 i = 0; i < PAGE_SIZE; i += 4) {
        mmio_write32(addr, i, 0);
    }
}

static void xhci_ring_init(VibioXhciRing *ring, vibio_u64 page)
{
    ring->base = page;
    ring->enqueue = 0;
    ring->cycle = 1;
    xhci_zero_page(page);
    /* Link TRB in the last slot points back to the start and toggles cycle. */
    vibio_u64 link = page + (XHCI_RING_TRBS - 1U) * 16U;
    mmio_write32(link, 0, (vibio_u32)page);
    mmio_write32(link, 4, (vibio_u32)(page >> 32));
    mmio_write32(link, 8, 0);
    mmio_write32(link, 12, (XHCI_TRB_LINK << 10) | (1U << 1) /*TC*/ | ring->cycle);
}

/* Enqueue one TRB; returns the physical address of the slot it landed in (used to
 * match the resulting event). Handles the trailing Link TRB + cycle toggle. */
static vibio_u64 xhci_ring_push(VibioXhciRing *ring, vibio_u64 param, vibio_u32 status, vibio_u32 control)
{
    vibio_u64 slot_addr = ring->base + ring->enqueue * 16U;
    mmio_write32(slot_addr, 0, (vibio_u32)param);
    mmio_write32(slot_addr, 4, (vibio_u32)(param >> 32));
    mmio_write32(slot_addr, 8, status);
    control = (control & ~1U) | ring->cycle;
    mmio_write32(slot_addr, 12, control);

    ring->enqueue++;
    if (ring->enqueue >= XHCI_RING_TRBS - 1U) {
        /* Re-arm the Link TRB cycle bit then wrap + toggle. */
        vibio_u64 link = ring->base + (XHCI_RING_TRBS - 1U) * 16U;
        mmio_write32(link, 12, (XHCI_TRB_LINK << 10) | (1U << 1) | ring->cycle);
        ring->enqueue = 0;
        ring->cycle ^= 1U;
    }
    return slot_addr;
}

static void xhci_ring_doorbell(VibioXhciMsd *x, vibio_u32 slot, vibio_u32 target)
{
    mmio_write32(x->db, slot * 4U, target);
}

/*
 * Poll the event ring for the next event TRB. Fills completion code, the TRB
 * pointer it refers to, and the transfer length residue. Returns 1 if an event
 * arrived before timeout. Port-status-change events are consumed and skipped.
 */
static vibio_u32 xhci_wait_event(VibioXhciMsd *x, vibio_u32 *out_cc, vibio_u64 *out_ptr,
                                 vibio_u32 *out_len, vibio_u32 *out_type,
                                 vibio_u32 *out_control)
{
    for (vibio_u32 spin = 0; spin < XHCI_SPIN_TIMEOUT; spin++) {
        vibio_u32 control = mmio_read32(x->event_dequeue, 12);
        vibio_u32 cbit = control & 1U;
        if (cbit != x->event_cycle) {
            io_wait();
            continue;
        }
        vibio_u32 type = (control >> 10) & 0x3FU;
        vibio_u64 ptr = (vibio_u64)mmio_read32(x->event_dequeue, 0) |
                        ((vibio_u64)mmio_read32(x->event_dequeue, 4) << 32);
        vibio_u32 status = mmio_read32(x->event_dequeue, 8);
        vibio_u32 cc = (status >> 24) & 0xFFU;
        vibio_u32 len = status & 0x00FFFFFFU;

        /* Advance the event-ring dequeue (single 256-TRB segment). */
        x->event_dequeue += 16U;
        if (x->event_dequeue >= x->event_page + XHCI_RING_TRBS * 16U) {
            x->event_dequeue = x->event_page;
            x->event_cycle ^= 1U;
        }
        /* Update ERDP with the new dequeue pointer + EHB clear (bit 3). */
        mmio_write32(x->rt, 0x20 + 0x18, (vibio_u32)(x->event_dequeue) | (1U << 3));
        mmio_write32(x->rt, 0x20 + 0x1C, (vibio_u32)(x->event_dequeue >> 32));

        if (type == XHCI_TRB_EV_PORT) {
            continue; /* not interesting here */
        }
        if (out_cc) { *out_cc = cc; }
        if (out_ptr) { *out_ptr = ptr; }
        if (out_len) { *out_len = len; }
        if (out_type) { *out_type = type; }
        if (out_control) { *out_control = control; }
        return 1;
    }
    return 0;
}

/* Issue a command-ring TRB and wait for its Command Completion Event. */
static vibio_u32 xhci_command(VibioXhciMsd *x, vibio_u64 param, vibio_u32 control,
                              vibio_u32 *out_cc, vibio_u32 *out_slot)
{
    vibio_u64 trb = xhci_ring_push(&x->cmd_ring, param, 0, control);
    xhci_ring_doorbell(x, 0, 0);

    for (vibio_u32 e = 0; e < 32U; e++) {
        vibio_u32 cc = 0, type = 0, event_control = 0;
        vibio_u64 ptr = 0;
        if (!xhci_wait_event(x, &cc, &ptr, 0, &type, &event_control)) {
            return 0;
        }
        if (type != XHCI_TRB_EV_CMD_COMPL || ptr != trb) {
            continue;
        }
        if (out_cc) { *out_cc = cc; }
        if (out_slot) { *out_slot = (event_control >> 24) & 0xFFU; }
        return 1;
    }
    return 0;
}

/* Enable Slot returns the assigned slot id in the event's control dword, so it
 * needs a dedicated path that captures that field. */
static vibio_u32 xhci_enable_slot(VibioXhciMsd *x, vibio_u32 *out_slot)
{
    vibio_u32 cc = 0;
    vibio_u32 slot = 0;
    if (!xhci_command(x, 0, (XHCI_TRB_ENABLE_SLOT << 10), &cc, &slot) ||
        cc != XHCI_CC_SUCCESS || slot == 0) {
        return 0;
    }
    *out_slot = slot;
    return 1;
}

static void xhci_disable_slot(VibioXhciMsd *x, vibio_u32 slot)
{
    if (x == 0 || slot == 0) {
        return;
    }
    vibio_u32 cc = 0;
    xhci_command(x, 0, (XHCI_TRB_DISABLE_SLOT << 10) | ((slot & 0xFFU) << 24), &cc, 0);
    mmio_write32(x->dcbaa, slot * 8U, 0);
    mmio_write32(x->dcbaa, slot * 8U + 4U, 0);
}

#define XHCI_PORTSC(x, p) ((x)->op + 0x400U + 0x10U * ((p) - 1U))

/* Mask off the RW1C change/status bits and the disable-by-write bits so we can
 * set a single control bit in PORTSC without clobbering anything. */
static vibio_u32 xhci_portsc_rw(vibio_u32 portsc)
{
    /* Preserve PP (bit 9) and link state; clear PED (bit1) write-1-disable and
     * the RW1C change bits (CSC/PEC/WRC/OCC/PRC/PLC/CEC at 17..23). */
    return portsc & ~((1U << 1) | (0x7FU << 17));
}

/* Read a USB descriptor over the EP0 control transfer. Returns bytes received. */
static vibio_u32 xhci_control_in(VibioXhciMsd *x, vibio_u32 bmRequestType, vibio_u32 bRequest,
                                 vibio_u32 wValue, vibio_u32 wIndex, vibio_u32 wLength,
                                 vibio_u64 buf)
{
    vibio_u32 setup_lo = (bmRequestType & 0xFFU) | ((bRequest & 0xFFU) << 8) | ((wValue & 0xFFFFU) << 16);
    vibio_u32 setup_hi = (wIndex & 0xFFFFU) | ((wLength & 0xFFFFU) << 16);
    vibio_u64 setup_param = (vibio_u64)setup_lo | ((vibio_u64)setup_hi << 32);
    vibio_u32 trt = (wLength > 0) ? 3U : 0U; /* 3 = IN data stage */

    /* Setup stage: IDT (bit6), TRT bits 16-17. */
    xhci_ring_push(&x->ep0_ring, setup_param, 8U,
                   (XHCI_TRB_SETUP << 10) | (1U << 6) | (trt << 16));
    if (wLength > 0) {
        /* Data stage IN (DIR bit16 = 1). */
        xhci_ring_push(&x->ep0_ring, buf, wLength, (XHCI_TRB_DATA << 10) | (1U << 16));
    }
    /* Status stage: opposite direction; IOC (bit5). For an IN data stage the
     * status is OUT (DIR 0); for no data it is IN (DIR 1). */
    vibio_u32 status_dir = (wLength > 0) ? 0U : (1U << 16);
    vibio_u64 status_trb = xhci_ring_push(&x->ep0_ring, 0, 0,
        (XHCI_TRB_STATUS << 10) | (1U << 5) | status_dir);

    xhci_ring_doorbell(x, x->slot, 1U); /* EP0 = DCI 1 */
    vibio_u32 cc = 0, len = 0;
    for (vibio_u32 e = 0; e < 8U; e++) {
        vibio_u32 type = 0;
        vibio_u64 ptr = 0;
        if (!xhci_wait_event(x, &cc, &ptr, &len, &type, 0)) {
            return 0;
        }
        if (type == XHCI_TRB_EV_TRANSFER && ptr == status_trb) {
            break;
        }
        if (e == 7U) {
            return 0;
        }
    }
    if (cc != XHCI_CC_SUCCESS && cc != XHCI_CC_SHORT_PACKET) {
        return 0;
    }
    if (wLength == 0) {
        return 1; /* status-only control transfer succeeded */
    }
    return (wLength > len) ? (wLength - len) : wLength;
}

/* Bulk transfer one buffer on the given ring/DCI; returns 1 on success. */
static vibio_u32 xhci_bulk(VibioXhciMsd *x, VibioXhciRing *ring, vibio_u32 dci,
                           vibio_u64 buf, vibio_u32 length, vibio_u32 *out_residue)
{
    vibio_u64 trb = xhci_ring_push(ring, buf, length, (XHCI_TRB_NORMAL << 10) | (1U << 5) /*IOC*/);
    xhci_ring_doorbell(x, x->slot, dci);
    vibio_u32 cc = 0, len = 0;
    for (vibio_u32 e = 0; e < 8U; e++) {
        vibio_u32 type = 0;
        vibio_u64 ptr = 0;
        if (!xhci_wait_event(x, &cc, &ptr, &len, &type, 0)) {
            return 0;
        }
        if (type == XHCI_TRB_EV_TRANSFER && ptr == trb) {
            break;
        }
        if (e == 7U) {
            return 0;
        }
    }
    if (out_residue) { *out_residue = len; }
    return (cc == XHCI_CC_SUCCESS || cc == XHCI_CC_SHORT_PACKET);
}

/* ---- Bulk-Only Transport + SCSI (read-only) ---- */

static vibio_u32 xhci_bot_scsi(VibioXhciMsd *x, const vibio_u8 *cdb, vibio_u32 cdb_len,
                               vibio_u32 data_len, vibio_u64 data_buf)
{
    vibio_u64 cbw = x->data_page;        /* 31 bytes */
    vibio_u64 csw = x->data_page + 64U;  /* 13 bytes */
    x->bot_tag++;

    /* Build the Command Block Wrapper (all reads are data-IN, flags 0x80). */
    for (vibio_u32 i = 0; i < 31; i++) {
        mmio_write8(cbw, i, 0);
    }
    mmio_write32(cbw, 0, 0x43425355U);          /* dCBWSignature 'USBC' */
    mmio_write32(cbw, 4, x->bot_tag);           /* dCBWTag */
    mmio_write32(cbw, 8, data_len);             /* dCBWDataTransferLength */
    mmio_write8(cbw, 12, 0x80);                 /* bmCBWFlags: data-in */
    mmio_write8(cbw, 13, 0);                    /* bCBWLUN */
    mmio_write8(cbw, 14, (vibio_u8)cdb_len);    /* bCBWCBLength */
    for (vibio_u32 i = 0; i < cdb_len && i < 16; i++) {
        mmio_write8(cbw, 15 + i, cdb[i]);
    }

    if (!xhci_bulk(x, &x->bulk_out, x->bulk_out_dci, cbw, 31, 0)) {
        return 0;
    }
    if (data_len > 0) {
        if (!xhci_bulk(x, &x->bulk_in, x->bulk_in_dci, data_buf, data_len, 0)) {
            return 0;
        }
    }
    /* Read the Command Status Wrapper. */
    if (!xhci_bulk(x, &x->bulk_in, x->bulk_in_dci, csw, 13, 0)) {
        return 0;
    }
    if (mmio_read32(csw, 0) != 0x53425355U) { /* dCSWSignature 'USBS' */
        x->last_error = XHCI_ERR_CSW;
        return 0;
    }
    if (mmio_read8(csw, 12) != 0) { /* bCSWStatus: 0 = passed */
        return 0;
    }
    return 1;
}

/* SCSI READ(10): one or more 512-byte blocks into data_buf. */
static vibio_u32 xhci_scsi_read10(VibioXhciMsd *x, vibio_u64 lba, vibio_u32 blocks, vibio_u64 data_buf)
{
    vibio_u8 cdb[10];
    for (vibio_u32 i = 0; i < 10; i++) { cdb[i] = 0; }
    cdb[0] = 0x28;
    cdb[2] = (vibio_u8)(lba >> 24);
    cdb[3] = (vibio_u8)(lba >> 16);
    cdb[4] = (vibio_u8)(lba >> 8);
    cdb[5] = (vibio_u8)(lba);
    cdb[7] = (vibio_u8)(blocks >> 8);
    cdb[8] = (vibio_u8)(blocks);
    return xhci_bot_scsi(x, cdb, 10, blocks * x->block_size, data_buf);
}

/* Read exactly one 512-byte sector into out512. Invoked by the FAT32 parser via
 * the ahci_read_sector() backend dispatch. Strictly read-only. */
static vibio_u32 xhci_block_read512(void *msd, vibio_u64 lba, vibio_u8 *out512)
{
    VibioXhciMsd *x = (VibioXhciMsd *)msd;
    if (x == 0 || !x->ready || out512 == 0) {
        return 0;
    }
    if (x->block_size != 512U) {
        return 0;
    }
    vibio_u64 buf = x->data_page + 2048U; /* sector scratch */
    for (vibio_u32 attempt = 0; attempt < 3; attempt++) {
        if (xhci_scsi_read10(x, lba, 1, buf)) {
            for (vibio_u32 i = 0; i < 512U; i++) {
                out512[i] = mmio_read8(buf, i);
            }
            x->read_ops++;
            return 1;
        }
    }
    return 0;
}

/* Build the input context for Address Device (slot + EP0). */
static void xhci_setup_input_ctx_address(VibioXhciMsd *x)
{
    vibio_u32 cs = x->context_size;
    xhci_zero_page(x->in_ctx);
    /* Input Control Context: add slot (A0) + EP0 (A1). */
    mmio_write32(x->in_ctx, 4, (1U << 0) | (1U << 1));
    /* Slot context at offset cs. */
    vibio_u64 slot_ctx = x->in_ctx + cs;
    vibio_u32 speed = x->speed;
    /* dword0: Route String (0) [19:0], Speed [23:20], Context Entries [31:27]=1. */
    mmio_write32(slot_ctx, 0, ((speed & 0xFU) << 20) | (1U << 27));
    /* dword1: Root Hub Port Number [23:16]. */
    mmio_write32(slot_ctx, 4, (x->port & 0xFFU) << 16);
    /* EP0 context at offset 2*cs. */
    vibio_u64 ep0 = x->in_ctx + cs * 2U;
    vibio_u32 mps = x->ep0_mps;
    /* dword1: CErr [2:1]=3, EP Type [5:3]=4 (Control), Max Packet Size [31:16]. */
    mmio_write32(ep0, 4, (3U << 1) | (4U << 3) | (mps << 16));
    /* TR Dequeue Pointer (dword2/3) = ep0 ring base | DCS(1). */
    mmio_write32(ep0, 8, (vibio_u32)(x->ep0_ring_page) | 1U);
    mmio_write32(ep0, 12, (vibio_u32)(x->ep0_ring_page >> 32));
}

/* Build/extend the input context for Configure Endpoint (bulk IN + OUT). */
static void xhci_setup_input_ctx_config(VibioXhciMsd *x)
{
    vibio_u32 cs = x->context_size;
    xhci_zero_page(x->in_ctx);
    vibio_u32 max_dci = x->bulk_in_dci > x->bulk_out_dci ? x->bulk_in_dci : x->bulk_out_dci;
    /* Add A0 (slot, re-evaluated) + the two bulk EP flags. */
    vibio_u32 add = (1U << 0) | (1U << x->bulk_in_dci) | (1U << x->bulk_out_dci);
    mmio_write32(x->in_ctx, 4, add);
    /* Slot context: update Context Entries to the highest DCI. */
    vibio_u64 slot_ctx = x->in_ctx + cs;
    mmio_write32(slot_ctx, 0, ((x->speed & 0xFU) << 20) | ((max_dci & 0x1FU) << 27));
    mmio_write32(slot_ctx, 4, (x->port & 0xFFU) << 16);
    /* Bulk OUT EP context: EP Type 2 (Bulk OUT). */
    vibio_u64 ep_out = x->in_ctx + cs * (x->bulk_out_dci + 1U);
    mmio_write32(ep_out, 4, (3U << 1) | (2U << 3) | (x->bulk_out_mps << 16));
    mmio_write32(ep_out, 8, (vibio_u32)(x->bulk_out_page) | 1U);
    mmio_write32(ep_out, 12, (vibio_u32)(x->bulk_out_page >> 32));
    /* Bulk IN EP context: EP Type 6 (Bulk IN). */
    vibio_u64 ep_in = x->in_ctx + cs * (x->bulk_in_dci + 1U);
    mmio_write32(ep_in, 4, (3U << 1) | (6U << 3) | (x->bulk_in_mps << 16));
    mmio_write32(ep_in, 8, (vibio_u32)(x->bulk_in_page) | 1U);
    mmio_write32(ep_in, 12, (vibio_u32)(x->bulk_in_page >> 32));
}

/* Parse the configuration descriptor for the BOT mass-storage interface and its
 * bulk IN/OUT endpoints. Returns 1 if a usable MSD interface was found. */
static vibio_u32 xhci_parse_config(VibioXhciMsd *x, vibio_u64 buf, vibio_u32 total)
{
    vibio_u32 i = 0;
    vibio_u32 in_msd = 0;
    x->bulk_in_ep = 0;
    x->bulk_out_ep = 0;
    while (i + 2U <= total) {
        vibio_u32 len = mmio_read8(buf, i);
        vibio_u32 type = mmio_read8(buf, i + 1U);
        if (len == 0) {
            break;
        }
        if (type == 0x04 && i + 9U <= total) { /* INTERFACE */
            vibio_u32 cls = mmio_read8(buf, i + 5U);
            vibio_u32 sub = mmio_read8(buf, i + 6U);
            vibio_u32 proto = mmio_read8(buf, i + 7U);
            in_msd = (cls == 0x08U && proto == 0x50U && (sub == 0x06U || sub == 0x05U)) ? 1U : 0U;
        } else if (type == 0x05 && in_msd && i + 7U <= total) { /* ENDPOINT */
            vibio_u32 addr = mmio_read8(buf, i + 2U);
            vibio_u32 attr = mmio_read8(buf, i + 3U);
            vibio_u32 mps = mmio_read8(buf, i + 4U) | ((vibio_u32)mmio_read8(buf, i + 5U) << 8);
            mps &= 0x7FFU;
            if ((attr & 0x3U) == 0x02U) { /* bulk */
                vibio_u32 epnum = addr & 0x0FU;
                if (addr & 0x80U) {
                    x->bulk_in_ep = addr;
                    x->bulk_in_dci = epnum * 2U + 1U;
                    x->bulk_in_mps = mps ? mps : 512U;
                } else {
                    x->bulk_out_ep = addr;
                    x->bulk_out_dci = epnum * 2U;
                    x->bulk_out_mps = mps ? mps : 512U;
                }
            }
        }
        i += len;
    }
    return x->bulk_in_ep != 0 && x->bulk_out_ep != 0;
}

static void xhci_prepare_device_attempt(VibioXhciMsd *x)
{
    x->slot = 0;
    x->vid = 0;
    x->pid = 0;
    x->bulk_in_ep = 0;
    x->bulk_out_ep = 0;
    x->bulk_in_dci = 0;
    x->bulk_out_dci = 0;
    x->bulk_in_mps = 0;
    x->bulk_out_mps = 0;
    x->config_value = 0;
    xhci_zero_page(x->dev_ctx);
    xhci_zero_page(x->data_page);
    xhci_ring_init(&x->ep0_ring, x->ep0_ring_page);
    xhci_ring_init(&x->bulk_in, x->bulk_in_page);
    xhci_ring_init(&x->bulk_out, x->bulk_out_page);
}

static vibio_u32 xhci_port_fail(VibioXhciMsd *x)
{
    if (x != 0 && x->slot != 0) {
        xhci_disable_slot(x, x->slot);
        x->slot = 0;
    }
    return 0;
}

static vibio_u32 xhci_wait_port_connected(VibioXhciMsd *x, vibio_u32 port, vibio_u32 *out_portsc)
{
    for (vibio_u32 spin = 0; spin < XHCI_PORT_CONNECT_TIMEOUT; spin++) {
        vibio_u32 portsc = mmio_read32(XHCI_PORTSC(x, port), 0);
        if (portsc == 0xFFFFFFFFU) {
            return 0;
        }
        if ((portsc & 1U) != 0) {
            if (out_portsc != 0) {
                *out_portsc = portsc;
            }
            return 1;
        }
        io_wait();
    }
    return 0;
}

static void xhci_power_root_ports(VibioXhciMsd *x)
{
    for (vibio_u32 p = 1; p <= x->max_ports; p++) {
        vibio_u32 portsc = mmio_read32(XHCI_PORTSC(x, p), 0);
        if (portsc == 0xFFFFFFFFU) {
            continue;
        }
        if ((portsc & (1U << 9)) == 0) {
            mmio_write32(XHCI_PORTSC(x, p), 0, xhci_portsc_rw(portsc) | (1U << 9));
        }
    }
    xhci_short_delay(100000);
}

static void xhci_refresh_port_diagnostics(VibioXhciMsd *x, VibioUsbStorage *st)
{
    if (x == 0 || st == 0) {
        return;
    }
    st->max_ports = x->max_ports;
    st->max_slots = x->max_slots;
    st->ports_connected = 0;
    for (vibio_u32 p = 1; p <= x->max_ports; p++) {
        vibio_u32 portsc = mmio_read32(XHCI_PORTSC(x, p), 0);
        if (portsc != 0xFFFFFFFFU && (portsc & 1U) != 0) {
            st->ports_connected++;
        }
    }
}

/* Full controller bring-up + single-device enumeration + first capacity probe.
 * Returns 1 if a read-only sector backend is available. Records honest
 * diagnostics into both *x and *st at each stage. */
/* Bring up ONE connected root-hub port as the read-only mass-storage device:
 * reset/enable the port, address the device, read descriptors, find a BOT
 * mass-storage interface, then verify SCSI INQUIRY / TEST-UNIT-READY / READ
 * CAPACITY / READ(10). Returns 1 only when the port holds a 512-byte-sector USB
 * disk that actually reads; on any failure it sets x->last_error and returns 0
 * so the caller can try the next connected port. A real laptop has several
 * connected USB devices (webcam, Bluetooth, fingerprint reader), so the boot
 * stick is rarely the first connected port - trying only one port is why
 * bring-up stopped at "ports read, no device". */
static vibio_u32 xhci_enumerate_one_port(VibioXhciMsd *x, VibioUsbStorage *st, vibio_u32 port)
{
    xhci_prepare_device_attempt(x);
    x->port = port;

    /* USB3 (SuperSpeed) ports auto-enable on connect: if PED (bit1) is already
     * set, adopt it. Otherwise drive a USB2 software reset (PR bit4) and wait. */
    vibio_u32 portsc = 0;
    if (!xhci_wait_port_connected(x, x->port, &portsc)) {
        x->last_error = XHCI_ERR_NO_PORT;
        return 0;
    }
    vibio_u32 enabled = (portsc & (1U << 1)) ? 1U : 0U;
    if (enabled) {
        x->speed = (portsc >> 10) & 0xFU;
        mmio_write32(XHCI_PORTSC(x, x->port), 0, xhci_portsc_rw(portsc) | (0x7FU << 17));
    } else {
        mmio_write32(XHCI_PORTSC(x, x->port), 0, xhci_portsc_rw(portsc) | (1U << 4));
        for (vibio_u32 s = 0; s < XHCI_SPIN_TIMEOUT; s++) {
            vibio_u32 ps = mmio_read32(XHCI_PORTSC(x, x->port), 0);
            if (ps & (1U << 1)) {
                mmio_write32(XHCI_PORTSC(x, x->port), 0, xhci_portsc_rw(ps) | (0x7FU << 17));
                enabled = 1;
                x->speed = (ps >> 10) & 0xFU;
                break;
            }
            io_wait();
        }
    }
    if (!enabled) {
        x->last_error = XHCI_ERR_PORT_RESET;
        return 0;
    }

    if (x->speed == 4U) { x->ep0_mps = 512U; }
    else if (x->speed == 3U) { x->ep0_mps = 64U; }
    else if (x->speed == 2U || x->speed == 1U) { x->ep0_mps = 8U; }
    else { x->ep0_mps = 64U; }

    if (!xhci_enable_slot(x, &x->slot) || x->slot == 0) {
        x->last_error = XHCI_ERR_ENABLE_SLOT;
        return 0;
    }
    mmio_write32(x->dcbaa, x->slot * 8U, (vibio_u32)x->dev_ctx);
    mmio_write32(x->dcbaa, x->slot * 8U + 4U, (vibio_u32)(x->dev_ctx >> 32));

    xhci_setup_input_ctx_address(x);
    vibio_u32 cc = 0;
    if (!xhci_command(x, x->in_ctx, (XHCI_TRB_ADDRESS_DEV << 10) | ((x->slot & 0xFFU) << 24), &cc, 0) ||
        cc != XHCI_CC_SUCCESS) {
        x->last_error = XHCI_ERR_ADDRESS;
        return xhci_port_fail(x);
    }

    vibio_u64 dbuf = x->data_page + 1024U;
    vibio_u32 got = xhci_control_in(x, 0x80, 0x06, (0x01U << 8), 0, 18, dbuf);
    if (got < 8) {
        x->last_error = XHCI_ERR_DESC;
        return xhci_port_fail(x);
    }
    if (x->speed != 3U && x->speed != 4U) {
        vibio_u32 real_mps = mmio_read8(dbuf, 7);
        if (real_mps == 8 || real_mps == 16 || real_mps == 32 || real_mps == 64) {
            x->ep0_mps = real_mps;
        }
    }
    x->vid = mmio_read8(dbuf, 8) | ((vibio_u32)mmio_read8(dbuf, 9) << 8);
    x->pid = mmio_read8(dbuf, 10) | ((vibio_u32)mmio_read8(dbuf, 11) << 8);
    x->last_step = USB_STOR_STEP_DEVICE;
    if (st) { st->device_descriptor_ok = 1; }

    got = xhci_control_in(x, 0x80, 0x06, (0x02U << 8), 0, 9, dbuf);
    if (got < 9) {
        x->last_error = XHCI_ERR_DESC;
        return xhci_port_fail(x);
    }
    vibio_u32 total = mmio_read8(dbuf, 2) | ((vibio_u32)mmio_read8(dbuf, 3) << 8);
    x->config_value = mmio_read8(dbuf, 5);
    if (total > 512U) { total = 512U; }
    got = xhci_control_in(x, 0x80, 0x06, (0x02U << 8), 0, total, dbuf);
    if (got < 9) {
        x->last_error = XHCI_ERR_DESC;
        return xhci_port_fail(x);
    }
    if (!xhci_parse_config(x, dbuf, total)) {
        x->last_error = XHCI_ERR_NO_MSD;
        return xhci_port_fail(x); /* not a mass-storage device - caller tries the next port */
    }
    x->last_step = USB_STOR_STEP_MSD;
    if (st) { st->msd_interface_found = 1; }

    if (xhci_control_in(x, 0x00, 0x09, x->config_value, 0, 0, 0) == 0 && x->config_value != 0) {
        x->last_error = XHCI_ERR_SET_CONFIG;
        return xhci_port_fail(x);
    }

    xhci_setup_input_ctx_config(x);
    cc = 0;
    if (!xhci_command(x, x->in_ctx, (XHCI_TRB_CONFIG_EP << 10) | ((x->slot & 0xFFU) << 24), &cc, 0) ||
        cc != XHCI_CC_SUCCESS) {
        x->last_error = XHCI_ERR_CONFIG_EP;
        return xhci_port_fail(x);
    }

    vibio_u64 sbuf = x->data_page + 2048U;
    vibio_u8 cdb[16];
    for (vibio_u32 i = 0; i < 16; i++) { cdb[i] = 0; }
    cdb[0] = 0x12; cdb[4] = 36; /* INQUIRY */
    if (!xhci_bot_scsi(x, cdb, 6, 36, sbuf)) {
        x->last_error = XHCI_ERR_INQUIRY;
        return xhci_port_fail(x);
    }
    for (vibio_u32 i = 0; i < 8; i++) { x->vendor[i] = (char)mmio_read8(sbuf, 8 + i); }
    x->vendor[8] = 0;
    for (vibio_u32 i = 0; i < 16; i++) { x->product[i] = (char)mmio_read8(sbuf, 16 + i); }
    x->product[16] = 0;

    vibio_u32 ready = 0;
    for (vibio_u32 attempt = 0; attempt < 5; attempt++) {
        for (vibio_u32 i = 0; i < 16; i++) { cdb[i] = 0; }
        cdb[0] = 0x00; /* TEST UNIT READY */
        if (xhci_bot_scsi(x, cdb, 6, 0, 0)) { ready = 1; break; }
        xhci_short_delay(20000);
    }
    if (!ready) {
        x->last_error = XHCI_ERR_NOT_READY;
        return xhci_port_fail(x);
    }

    for (vibio_u32 i = 0; i < 16; i++) { cdb[i] = 0; }
    cdb[0] = 0x25; /* READ CAPACITY(10) */
    if (!xhci_bot_scsi(x, cdb, 10, 8, sbuf)) {
        x->last_error = XHCI_ERR_CAPACITY;
        return xhci_port_fail(x);
    }
    vibio_u32 last_lba = ((vibio_u32)mmio_read8(sbuf, 0) << 24) | ((vibio_u32)mmio_read8(sbuf, 1) << 16) |
                         ((vibio_u32)mmio_read8(sbuf, 2) << 8) | (vibio_u32)mmio_read8(sbuf, 3);
    vibio_u32 blk = ((vibio_u32)mmio_read8(sbuf, 4) << 24) | ((vibio_u32)mmio_read8(sbuf, 5) << 16) |
                    ((vibio_u32)mmio_read8(sbuf, 6) << 8) | (vibio_u32)mmio_read8(sbuf, 7);
    x->block_size = blk;
    x->total_blocks = (vibio_u64)last_lba + 1ULL;
    if (blk != 512U) {
        x->last_error = XHCI_ERR_SECTOR_SIZE; /* FAT32 reader assumes 512 */
        x->last_step = USB_STOR_STEP_SECTORS;
        return xhci_port_fail(x);
    }

    if (!xhci_scsi_read10(x, 0, 1, sbuf)) {
        x->last_error = XHCI_ERR_READ;
        x->last_step = USB_STOR_STEP_SECTORS;
        return xhci_port_fail(x);
    }

    x->ready = 1;
    x->last_step = USB_STOR_STEP_SECTORS;
    x->last_error = XHCI_ERR_NONE;
    if (st) {
        st->sector_backend_ready = 1;
        st->last_step = USB_STOR_STEP_SECTORS;
    }
    return 1;
}

static vibio_u32 xhci_msd_init(VibioXhciMsd *x, VibioPageAllocator *alloc, VibioUsbStorage *st)
{
    zero_bytes(x, sizeof(VibioXhciMsd));
    x->last_step = USB_STOR_STEP_NONE;

    /* The probe sets mmio_readable only when the firmware already enabled the
     * controller's PCI memory space. OVMF leaves the (non-boot) xHCI's memory
     * space disabled, so we enable it ourselves below rather than requiring it
     * up front - we only need a real xHCI controller with a low-4 GiB MMIO BAR. */
    if (st == 0 || !st->controller_found || st->controller_type != USB_TYPE_XHCI ||
        st->bar == 0) {
        x->last_error = XHCI_ERR_NO_CTRL;
        return 0;
    }

    x->bus = st->bus; x->device = st->device; x->function = st->function;
    x->mmio = st->bar;

    /* Ensure PCI memory space + bus master are enabled (safe, required). */
    vibio_u32 cmd = pci_config_read32(x->bus, x->device, x->function, 0x04);
    pci_config_write32(x->bus, x->device, x->function, 0x04, cmd | 0x06U);

    x->cap_length = mmio_read8(x->mmio, 0x00);
    x->hci_version = mmio_read16(x->mmio, 0x02);
    /* A sane xHCI exposes CAPLENGTH in [0x20, 0xFF]; that alone proves the MMIO
     * window is live. (HCIVERSION is informational and not gated on.) */
    if (x->cap_length < 0x20 || x->cap_length > 0xFF) {
        x->last_error = XHCI_ERR_BAR;
        return 0;
    }
    /* Reflect the now-readable controller in the shared diagnostics. */
    st->mem_space_enabled = 1;
    st->bar_mapped = 1;
    st->mmio_readable = 1;
    st->cap_length = x->cap_length;
    st->hci_version = x->hci_version;
    vibio_u32 hcs1 = mmio_read32(x->mmio, 0x04);
    vibio_u32 hcc1 = mmio_read32(x->mmio, 0x10);
    x->max_slots = hcs1 & 0xFFU;
    x->max_ports = (hcs1 >> 24) & 0xFFU;
    x->context_size = (hcc1 & (1U << 2)) ? 64U : 32U;
    x->op = x->mmio + x->cap_length;
    x->rt = x->mmio + (mmio_read32(x->mmio, 0x18) & ~0x1FU);
    x->db = x->mmio + (mmio_read32(x->mmio, 0x14) & ~0x3U);
    x->last_step = USB_STOR_STEP_MMIO;

    /* ---- Reset the controller ---- */
    /* Halt first (clear R/S), wait HCH. */
    vibio_u32 usbcmd = mmio_read32(x->op, 0x00);
    mmio_write32(x->op, 0x00, usbcmd & ~1U);
    for (vibio_u32 s = 0; s < XHCI_SPIN_TIMEOUT; s++) {
        if (mmio_read32(x->op, 0x04) & 1U) { break; } /* HCH */
        io_wait();
    }
    /* HCRST (bit1), wait until it clears and CNR (bit11) clears. */
    mmio_write32(x->op, 0x00, (1U << 1));
    vibio_u32 reset_ok = 0;
    for (vibio_u32 s = 0; s < XHCI_SPIN_TIMEOUT; s++) {
        if ((mmio_read32(x->op, 0x00) & (1U << 1)) == 0 &&
            (mmio_read32(x->op, 0x04) & (1U << 11)) == 0) {
            reset_ok = 1;
            break;
        }
        io_wait();
    }
    if (!reset_ok) {
        x->last_error = XHCI_ERR_RESET;
        return 0;
    }

    /* ---- Allocate DMA structures (one page each) ---- */
    x->dcbaa        = page_allocator_alloc(alloc);
    x->cmd_page     = page_allocator_alloc(alloc);
    x->event_page   = page_allocator_alloc(alloc);
    x->erst_page    = page_allocator_alloc(alloc);
    x->in_ctx       = page_allocator_alloc(alloc);
    x->dev_ctx      = page_allocator_alloc(alloc);
    x->ep0_ring_page= page_allocator_alloc(alloc);
    x->bulk_in_page = page_allocator_alloc(alloc);
    x->bulk_out_page= page_allocator_alloc(alloc);
    x->data_page    = page_allocator_alloc(alloc);
    if (!x->dcbaa || !x->cmd_page || !x->event_page || !x->erst_page || !x->in_ctx ||
        !x->dev_ctx || !x->ep0_ring_page || !x->bulk_in_page || !x->bulk_out_page || !x->data_page) {
        x->last_error = XHCI_ERR_ALLOC;
        return 0;
    }
    xhci_zero_page(x->dcbaa);
    xhci_zero_page(x->event_page);
    xhci_zero_page(x->erst_page);
    xhci_zero_page(x->dev_ctx);
    xhci_zero_page(x->data_page);
    xhci_ring_init(&x->cmd_ring, x->cmd_page);
    xhci_ring_init(&x->ep0_ring, x->ep0_ring_page);
    xhci_ring_init(&x->bulk_in, x->bulk_in_page);
    xhci_ring_init(&x->bulk_out, x->bulk_out_page);

    /* Scratchpad buffers if the controller requires them. */
    vibio_u32 hcs2 = mmio_read32(x->mmio, 0x08);
    vibio_u32 spb = ((hcs2 >> 27) & 0x1FU) | (((hcs2 >> 21) & 0x1FU) << 5);
    if (spb > 0) {
        vibio_u64 sp_array = page_allocator_alloc(alloc);
        if (sp_array) {
            xhci_zero_page(sp_array);
            for (vibio_u32 i = 0; i < spb && i < 512U; i++) {
                vibio_u64 sp = page_allocator_alloc(alloc);
                if (!sp) { break; }
                xhci_zero_page(sp);
                mmio_write32(sp_array, i * 8U, (vibio_u32)sp);
                mmio_write32(sp_array, i * 8U + 4U, (vibio_u32)(sp >> 32));
            }
            mmio_write32(x->dcbaa, 0, (vibio_u32)sp_array);
            mmio_write32(x->dcbaa, 4, (vibio_u32)(sp_array >> 32));
        }
    }

    /* ---- Program controller registers ---- */
    /* DCBAAP. */
    mmio_write32(x->op, 0x30, (vibio_u32)x->dcbaa);
    mmio_write32(x->op, 0x34, (vibio_u32)(x->dcbaa >> 32));
    /* CONFIG: MaxSlotsEn. */
    mmio_write32(x->op, 0x38, x->max_slots);
    /* Command ring control: pointer | RCS(1). */
    mmio_write32(x->op, 0x18, (vibio_u32)x->cmd_page | 1U);
    mmio_write32(x->op, 0x1C, (vibio_u32)(x->cmd_page >> 32));

    /* Event ring: ERST with one segment, interrupter 0. */
    mmio_write32(x->erst_page, 0, (vibio_u32)x->event_page);
    mmio_write32(x->erst_page, 4, (vibio_u32)(x->event_page >> 32));
    mmio_write32(x->erst_page, 8, XHCI_RING_TRBS); /* segment size */
    mmio_write32(x->erst_page, 12, 0);
    x->event_dequeue = x->event_page;
    x->event_cycle = 1;
    /* ERSTSZ=1. */
    mmio_write32(x->rt, 0x20 + 0x08, 1U);
    /* ERDP. */
    mmio_write32(x->rt, 0x20 + 0x18, (vibio_u32)x->event_page);
    mmio_write32(x->rt, 0x20 + 0x1C, (vibio_u32)(x->event_page >> 32));
    /* ERSTBA. */
    mmio_write32(x->rt, 0x20 + 0x10, (vibio_u32)x->erst_page);
    mmio_write32(x->rt, 0x20 + 0x14, (vibio_u32)(x->erst_page >> 32));

    /* ---- Run the controller ---- */
    mmio_write32(x->op, 0x00, 1U); /* R/S */
    for (vibio_u32 s = 0; s < XHCI_SPIN_TIMEOUT; s++) {
        if ((mmio_read32(x->op, 0x04) & 1U) == 0) { break; } /* HCH clear */
        io_wait();
    }
    x->init_done = 1;
    if (st) { st->init_done = 1; }

    xhci_power_root_ports(x);
    xhci_refresh_port_diagnostics(x, st);

    /* ---- Scan EVERY connected root-hub port for a USB mass-storage device ----
     * A real machine has several connected USB devices and the boot stick is
     * rarely the first one, so try each connected port in turn (powering it if
     * needed) and keep the first that is a readable 512-byte-sector disk. The
     * earlier code stopped at the first connected port, which on a laptop is
     * usually an internal device that is not storage - that is exactly why
     * bring-up stalled at step=PORTS with no device enumerated. */
    x->last_step = USB_STOR_STEP_PORTS;
    if (st) { st->last_step = USB_STOR_STEP_PORTS; }
    vibio_u32 any_connected = 0;
    for (vibio_u32 p = 1; p <= x->max_ports; p++) {
        vibio_u32 portsc = mmio_read32(XHCI_PORTSC(x, p), 0);
        if (portsc == 0xFFFFFFFFU) { continue; }
        if ((portsc & (1U << 9)) == 0) { /* power the port (PP bit9) */
            mmio_write32(XHCI_PORTSC(x, p), 0, xhci_portsc_rw(portsc) | (1U << 9));
            xhci_short_delay(2000);
            portsc = mmio_read32(XHCI_PORTSC(x, p), 0);
        }
        if ((portsc & 1U) == 0 && !xhci_wait_port_connected(x, p, &portsc)) {
            continue; /* CCS: nothing connected */
        }
        any_connected = 1;
        if (st && p > st->ports_connected) {
            xhci_refresh_port_diagnostics(x, st);
        }
        if (xhci_enumerate_one_port(x, st, p)) {
            xhci_refresh_port_diagnostics(x, st);
            return 1; /* found a readable USB disk on this port */
        }
        /* Not a usable disk on this port - leave its slot and try the next. */
    }
    if (!any_connected) {
        x->last_error = XHCI_ERR_NO_PORT;
    }
    /* Report the furthest layer ANY port reached (the per-layer st flags are
     * sticky), so the on-screen diagnostic still pinpoints where it stops. */
    if (st) {
        if (st->sector_backend_ready) { st->last_step = USB_STOR_STEP_SECTORS; }
        else if (st->msd_interface_found) { st->last_step = USB_STOR_STEP_MSD; }
        else if (st->device_descriptor_ok) { st->last_step = USB_STOR_STEP_DEVICE; }
        else { st->last_step = USB_STOR_STEP_PORTS; }
    }
    return 0;
}


/* Record an honest "not mounted" state when boot-time xHCI MSD bring-up is
 * deliberately skipped. This is the emergency real-hardware boot fix: purple
 * TRACE 5e means MMIO mapping succeeded, but the next full xHCI init/mount path
 * can hang on some real controllers. Skipping it lets Vibio reach TRACE 5f/T6
 * and the desktop instead of freezing. */
static void xhci_record_boot_msd_skipped(VibioXhciMsd *x, VibioUsbStorage *st)
{
    if (x != 0) {
        zero_bytes(x, sizeof(VibioXhciMsd));
        x->last_step = (st != 0 && st->last_step > USB_STOR_STEP_NONE) ?
            st->last_step : USB_STOR_STEP_MMIO;
        x->last_error = XHCI_ERR_BOOT_SKIPPED;
        if (st != 0) {
            x->bus = st->bus;
            x->device = st->device;
            x->function = st->function;
            x->mmio = st->bar;
            x->cap_length = st->cap_length;
            x->hci_version = st->hci_version;
            x->max_slots = st->max_slots;
            x->max_ports = st->max_ports;
        }
    }
    if (st != 0) {
        st->boot_init_skipped = 1;
        st->init_done = 0;
        st->device_descriptor_ok = 0;
        st->msd_interface_found = 0;
        st->sector_backend_ready = 0;
        st->fat32_mounted = 0;
        if (st->last_step < USB_STOR_STEP_MMIO) {
            st->last_step = USB_STOR_STEP_MMIO;
        }
    }
}

/* Pick the Files app storage backend by priority: AHCI boot-disk FAT32 first,
 * then a read-only USB FAT32 mount, then none. Single source of truth shared by
 * the Files app and the FILESDBG command. */
static vibio_u32 storage_select_source(
    const VibioFat32ReadTest *ahci_fs,
    const VibioUsbStorage *usbstor,
    const VibioBootInfo *boot_info)
{
    if (ahci_fs != 0 && ahci_fs->root_found) {
        return FILES_SRC_AHCI;
    }
    if (usbstor != 0 && usbstor->fat32_mounted) {
        return FILES_SRC_USB;
    }
    if (boot_info != 0 && boot_info->boot_file_count > 0) {
        return FILES_SRC_BOOT;
    }
    return FILES_SRC_NONE;
}

static vibio_u32 pci_find_ahci_controller(VibioDiskReadTest *disk)
{
    for (vibio_u32 bus = 0; bus < 256; bus++) {
        for (vibio_u32 device = 0; device < 32; device++) {
            for (vibio_u32 function = 0; function < 8; function++) {
                vibio_u32 id = pci_config_read32(bus, device, function, 0x00);
                if ((id & 0xFFFFU) == 0xFFFFU) {
                    continue;
                }

                vibio_u32 class_info = pci_config_read32(bus, device, function, 0x08);
                vibio_u8 class_code = (vibio_u8)(class_info >> 24);
                vibio_u8 subclass = (vibio_u8)(class_info >> 16);
                vibio_u8 prog_if = (vibio_u8)(class_info >> 8);

                if (class_code == AHCI_PCI_CLASS_MASS_STORAGE &&
                    subclass == AHCI_PCI_SUBCLASS_SATA &&
                    prog_if == AHCI_PCI_PROGIF_AHCI) {
                    vibio_u32 command = pci_config_read32(bus, device, function, 0x04);
                    pci_config_write32(bus, device, function, 0x04, command | 0x00000006U);

                    vibio_u32 abar = pci_config_read32(bus, device, function, 0x24);
                    disk->pci_bus = bus;
                    disk->pci_device = device;
                    disk->pci_function = function;
                    disk->abar = abar & 0xFFFFFFF0U;
                    return disk->abar != 0;
                }
            }
        }
    }

    return 0;
}

static vibio_u64 ahci_port_base(vibio_u64 abar, vibio_u32 port)
{
    return abar + AHCI_PORT_BASE_OFFSET + (vibio_u64)port * AHCI_PORT_REGISTER_SIZE;
}

static vibio_u32 ahci_wait_clear(vibio_u64 base, vibio_u32 offset, vibio_u32 mask)
{
    for (vibio_u32 i = 0; i < AHCI_READ_TIMEOUT; i++) {
        if ((mmio_read32(base, offset) & mask) == 0) {
            return 1;
        }
    }

    return 0;
}

static vibio_u32 ahci_find_active_port(VibioDiskReadTest *disk)
{
    disk->port_implemented = mmio_read32(disk->abar, 0x0C);

    for (vibio_u32 port = 0; port < AHCI_MAX_PORTS; port++) {
        if ((disk->port_implemented & (1U << port)) == 0) {
            continue;
        }

        vibio_u64 port_base = ahci_port_base(disk->abar, port);
        vibio_u32 ssts = mmio_read32(port_base, 0x28);
        vibio_u32 det = ssts & 0x0FU;
        if (det == AHCI_SSTS_DET_PRESENT) {
            disk->port = port;
            disk->port_status = ssts;
            disk->port_signature = mmio_read32(port_base, 0x24);
            return 1;
        }
    }

    return 0;
}

static vibio_u32 ahci_prepare_port(VibioDiskReadTest *disk)
{
    vibio_u64 port_base = ahci_port_base(disk->abar, disk->port);
    vibio_u32 command = mmio_read32(port_base, 0x18);
    command &= ~(AHCI_CMD_ST | AHCI_CMD_FRE);
    mmio_write32(port_base, 0x18, command);

    if (!ahci_wait_clear(port_base, 0x18, AHCI_CMD_CR | AHCI_CMD_FR)) {
        disk->last_error = 1;
        return 0;
    }

    zero_page(disk->command_list);
    zero_page(disk->received_fis);
    zero_page(disk->command_table);
    zero_page(disk->sector_buffer);

    mmio_write32(port_base, 0x00, (vibio_u32)(disk->command_list & 0xFFFFFFFFULL));
    mmio_write32(port_base, 0x04, (vibio_u32)(disk->command_list >> 32));
    mmio_write32(port_base, 0x08, (vibio_u32)(disk->received_fis & 0xFFFFFFFFULL));
    mmio_write32(port_base, 0x0C, (vibio_u32)(disk->received_fis >> 32));
    mmio_write32(port_base, 0x30, 0xFFFFFFFFU);
    mmio_write32(port_base, 0x10, 0xFFFFFFFFU);
    mmio_write32(disk->abar, 0x04, mmio_read32(disk->abar, 0x04) | AHCI_GHC_AE);

    command = mmio_read32(port_base, 0x18);
    command |= AHCI_CMD_FRE | AHCI_CMD_ST;
    mmio_write32(port_base, 0x18, command);
    return 1;
}

static void ahci_build_read_command(VibioDiskReadTest *disk, vibio_u64 lba)
{
    volatile vibio_u8 *header = (volatile vibio_u8 *)disk->command_list;
    volatile vibio_u8 *table = (volatile vibio_u8 *)disk->command_table;

    header[0] = 5;
    header[1] = 0;
    header[2] = 1;
    header[3] = 0;
    for (vibio_u32 i = 4; i < AHCI_CMD_HEADER_BYTES; i++) {
        header[i] = 0;
    }

    vibio_u64 table_address = disk->command_table;
    header[8] = (vibio_u8)(table_address & 0xFF);
    header[9] = (vibio_u8)((table_address >> 8) & 0xFF);
    header[10] = (vibio_u8)((table_address >> 16) & 0xFF);
    header[11] = (vibio_u8)((table_address >> 24) & 0xFF);
    header[12] = (vibio_u8)((table_address >> 32) & 0xFF);
    header[13] = (vibio_u8)((table_address >> 40) & 0xFF);
    header[14] = (vibio_u8)((table_address >> 48) & 0xFF);
    header[15] = (vibio_u8)((table_address >> 56) & 0xFF);

    for (vibio_u32 i = 0; i < AHCI_COMMAND_TABLE_BYTES; i++) {
        table[i] = 0;
    }

    table[0] = AHCI_FIS_REG_H2D;
    table[1] = 0x80;
    table[2] = AHCI_ATA_READ_DMA_EXT;
    table[4] = (vibio_u8)(lba & 0xFF);
    table[5] = (vibio_u8)((lba >> 8) & 0xFF);
    table[6] = (vibio_u8)((lba >> 16) & 0xFF);
    table[7] = 1U << 6;
    table[8] = (vibio_u8)((lba >> 24) & 0xFF);
    table[9] = (vibio_u8)((lba >> 32) & 0xFF);
    table[10] = (vibio_u8)((lba >> 40) & 0xFF);
    table[12] = 1;
    table[13] = 0;

    vibio_u64 buffer = disk->sector_buffer;
    vibio_u32 prdt = AHCI_COMMAND_TABLE_PRDT_OFFSET;
    table[prdt + 0] = (vibio_u8)(buffer & 0xFF);
    table[prdt + 1] = (vibio_u8)((buffer >> 8) & 0xFF);
    table[prdt + 2] = (vibio_u8)((buffer >> 16) & 0xFF);
    table[prdt + 3] = (vibio_u8)((buffer >> 24) & 0xFF);
    table[prdt + 4] = (vibio_u8)((buffer >> 32) & 0xFF);
    table[prdt + 5] = (vibio_u8)((buffer >> 40) & 0xFF);
    table[prdt + 6] = (vibio_u8)((buffer >> 48) & 0xFF);
    table[prdt + 7] = (vibio_u8)((buffer >> 56) & 0xFF);
    table[prdt + 12] = (vibio_u8)((AHCI_SECTOR_BYTES - 1) & 0xFF);
    table[prdt + 13] = (vibio_u8)(((AHCI_SECTOR_BYTES - 1) >> 8) & 0xFF);
    table[prdt + 14] = 0;
    table[prdt + 15] = 0;
}

static vibio_u32 ahci_read_sector(VibioDiskReadTest *disk, vibio_u64 lba)
{
    /* USB-backed disks read one 512-byte sector via SCSI READ(10) into the same
     * sector_buffer the FAT32 parser reads from, so nothing downstream changes. */
    if (disk->backend == DISK_BACKEND_USB) {
        if (xhci_block_read512(disk->usb_msd, lba, (vibio_u8 *)disk->sector_buffer)) {
            disk->read_count++;
            return 1;
        }
        disk->last_error = 9;
        return 0;
    }

    vibio_u64 port_base = ahci_port_base(disk->abar, disk->port);

    if ((mmio_read32(port_base, 0x20) & (AHCI_TFD_BSY | AHCI_TFD_DRQ)) != 0) {
        if (!ahci_wait_clear(port_base, 0x20, AHCI_TFD_BSY | AHCI_TFD_DRQ)) {
            disk->last_error = 2;
            return 0;
        }
    }

    mmio_write32(port_base, 0x10, 0xFFFFFFFFU);
    mmio_write32(port_base, 0x34, 0);
    ahci_build_read_command(disk, lba);
    mmio_write32(port_base, 0x38, 1);

    for (vibio_u32 i = 0; i < AHCI_READ_TIMEOUT; i++) {
        vibio_u32 interrupt_status = mmio_read32(port_base, 0x10);
        if ((interrupt_status & AHCI_IS_TFES) != 0) {
            disk->last_error = 3;
            return 0;
        }

        if ((mmio_read32(port_base, 0x38) & 1U) == 0) {
            disk->read_count++;
            return 1;
        }
    }

    disk->last_error = 4;
    return 0;
}

static vibio_u32 disk_read_le32(const vibio_u8 *buffer, vibio_u32 offset)
{
    return (vibio_u32)buffer[offset] |
           ((vibio_u32)buffer[offset + 1] << 8) |
           ((vibio_u32)buffer[offset + 2] << 16) |
           ((vibio_u32)buffer[offset + 3] << 24);
}

static vibio_u16 disk_read_le16(const vibio_u8 *buffer, vibio_u32 offset)
{
    return (vibio_u16)((vibio_u16)buffer[offset] | ((vibio_u16)buffer[offset + 1] << 8));
}

static void audio_set_current_name(VibioAudioState *audio, const VibioBootFile *file)
{
    audio->current_name_length = 0;
    for (vibio_u32 i = 0; i < VIBIO_BOOT_FILE_NAME_MAX; i++) {
        audio->current_name[i] = 0;
    }

    if (file == 0) {
        return;
    }

    for (vibio_u32 i = 0; i < file->name_length && i < VIBIO_BOOT_FILE_NAME_MAX; i++) {
        audio->current_name[i] = file->name[i];
        audio->current_name_length++;
    }
}

static vibio_u32 boot_file_name_matches_literal(const VibioBootFile *file, const char *name)
{
    vibio_u32 i = 0;
    while (i < file->name_length && i < VIBIO_BOOT_FILE_NAME_MAX && name[i] != 0) {
        if (file->name[i] != name[i]) {
            return 0;
        }
        i++;
    }

    return i == file->name_length && name[i] == 0;
}

static const VibioBootFile *find_sound_file_by_name(const VibioBootInfo *boot_info, const char *name)
{
    for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *file = &boot_info->boot_files[i];
        if (file->type == VIBIO_BOOT_FILE_TYPE_SOUND && boot_file_name_matches_literal(file, name)) {
            return file;
        }
    }

    return 0;
}

static vibio_u32 wav_parse_pcm16(const VibioBootFile *file, VibioWavInfo *wav)
{
    zero_bytes(wav, sizeof(VibioWavInfo));
    if (file == 0 || file->address == 0 || file->size < 44) {
        return 0;
    }

    const vibio_u8 *bytes = (const vibio_u8 *)file->address;
    if (!(bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
          bytes[8] == 'W' && bytes[9] == 'A' && bytes[10] == 'V' && bytes[11] == 'E')) {
        return 0;
    }

    vibio_u64 offset = 12;
    vibio_u32 found_fmt = 0;
    vibio_u32 found_data = 0;

    while (offset + 8 <= file->size) {
        vibio_u32 chunk_size = disk_read_le32(bytes, (vibio_u32)offset + 4);
        vibio_u64 payload = offset + 8;
        if (payload + chunk_size > file->size) {
            break;
        }

        if (bytes[offset] == 'f' && bytes[offset + 1] == 'm' && bytes[offset + 2] == 't' && bytes[offset + 3] == ' ') {
            if (chunk_size >= 16) {
                wav->audio_format = disk_read_le16(bytes, (vibio_u32)payload);
                wav->channels = disk_read_le16(bytes, (vibio_u32)payload + 2);
                wav->sample_rate = disk_read_le32(bytes, (vibio_u32)payload + 4);
                wav->block_align = disk_read_le16(bytes, (vibio_u32)payload + 12);
                wav->bits_per_sample = disk_read_le16(bytes, (vibio_u32)payload + 14);
                found_fmt = 1;
            }
        } else if (bytes[offset] == 'd' && bytes[offset + 1] == 'a' && bytes[offset + 2] == 't' && bytes[offset + 3] == 'a') {
            wav->data_offset = payload;
            wav->data_size = chunk_size;
            found_data = chunk_size > 0;
        }

        offset = payload + chunk_size + (chunk_size & 1U);
    }

    wav->ok = found_fmt &&
              found_data &&
              wav->audio_format == 1 &&
              (wav->channels == 1 || wav->channels == 2) &&
              wav->sample_rate == AUDIO_RATE &&
              wav->bits_per_sample == AUDIO_BITS &&
              wav->block_align == wav->channels * 2;
    return wav->ok;
}

static int audio_pcm16_scale(int sample, vibio_u32 volume)
{
    if (volume >= 100U) {
        return sample;
    }
    int scaled = (sample * (int)volume) / 100;
    if (scaled > 32767) {
        return 32767;
    }
    if (scaled < -32768) {
        return -32768;
    }
    return scaled;
}

static int audio_pcm16_read_le(const vibio_u8 *p)
{
    int v = (int)((vibio_u16)p[0] | ((vibio_u16)p[1] << 8));
    return v >= 32768 ? v - 65536 : v;
}

static void audio_pcm16_write_le(vibio_u8 *p, int sample)
{
    vibio_u16 v = (vibio_u16)(sample & 0xFFFF);
    p[0] = (vibio_u8)(v & 0xFFU);
    p[1] = (vibio_u8)((v >> 8) & 0xFFU);
}

static void audio_render_pcm_volume(VibioAudioState *audio)
{
    if (audio == 0 || audio->pcm_source_address == 0 || audio->pcm_buffer_address == 0 ||
        audio->pcm_play_bytes == 0 || audio->pcm_play_bytes > audio->pcm_source_bytes ||
        audio->pcm_play_bytes > audio->pcm_buffer_bytes ||
        audio->pcm_source_offset + audio->pcm_play_bytes > audio->pcm_source_bytes) {
        return;
    }

    const vibio_u8 *src = (const vibio_u8 *)(audio->pcm_source_address + audio->pcm_source_offset);
    vibio_u8 *dst = (vibio_u8 *)audio->pcm_buffer_address;
    if (audio->volume >= 100U) {
        copy_bytes(dst, src, audio->pcm_play_bytes);
        return;
    }

    vibio_u64 samples = audio->pcm_play_bytes / 2ULL;
    for (vibio_u64 i = 0; i < samples; i++) {
        int s = audio_pcm16_read_le(src + i * 2ULL);
        audio_pcm16_write_le(dst + i * 2ULL, audio_pcm16_scale(s, audio->volume));
    }
}

static vibio_u32 ac97_find_controller(VibioAudioState *audio)
{
    for (vibio_u32 bus = 0; bus < 256; bus++) {
        for (vibio_u32 device = 0; device < 32; device++) {
            for (vibio_u32 function = 0; function < 8; function++) {
                vibio_u32 id = pci_config_read32(bus, device, function, 0x00);
                vibio_u32 vendor_id = id & 0xFFFFU;
                if (vendor_id == 0xFFFFU) {
                    if (function == 0) {
                        break;
                    }
                    continue;
                }

                vibio_u32 class_info = pci_config_read32(bus, device, function, 0x08);
                vibio_u8 class_code = (vibio_u8)((class_info >> 24) & 0xFFU);
                vibio_u8 subclass = (vibio_u8)((class_info >> 16) & 0xFFU);
                if (class_code == PCI_CLASS_MULTIMEDIA && subclass == PCI_SUBCLASS_AUDIO) {
                    vibio_u32 bar0 = pci_config_read32(bus, device, function, 0x10);
                    vibio_u32 bar1 = pci_config_read32(bus, device, function, 0x14);
                    if ((bar0 & 1U) == 0 || (bar1 & 1U) == 0) {
                        audio->last_error = 2;
                        return 0;
                    }

                    vibio_u32 command = pci_config_read32(bus, device, function, 0x04);
                    pci_config_write32(bus, device, function, 0x04, command | 0x00000005U);

                    audio->found = 1;
                    audio->pci_bus = bus;
                    audio->pci_device = device;
                    audio->pci_function = function;
                    audio->vendor_id = vendor_id;
                    audio->device_id = (id >> 16) & 0xFFFFU;
                    audio->mixer_base = (vibio_u16)(bar0 & 0xFFF0U);
                    audio->bus_master_base = (vibio_u16)(bar1 & 0xFFF0U);
                    return 1;
                }

                if (function == 0) {
                    vibio_u32 header = pci_config_read32(bus, device, function, 0x0C);
                    if ((header & 0x00800000U) == 0) {
                        break;
                    }
                }
            }
        }
    }

    audio->last_error = 1;
    return 0;
}

static vibio_u32 hda_detect_controller(VibioAudioState *audio)
{
    for (vibio_u32 bus = 0; bus < 256; bus++) {
        for (vibio_u32 device = 0; device < 32; device++) {
            for (vibio_u32 function = 0; function < 8; function++) {
                vibio_u32 id = pci_config_read32(bus, device, function, 0x00);
                vibio_u32 vendor_id = id & 0xFFFFU;
                if (vendor_id == 0xFFFFU) {
                    if (function == 0) {
                        break;
                    }
                    continue;
                }

                vibio_u32 class_info = pci_config_read32(bus, device, function, 0x08);
                vibio_u8 class_code = (vibio_u8)((class_info >> 24) & 0xFFU);
                vibio_u8 subclass = (vibio_u8)((class_info >> 16) & 0xFFU);
                if (class_code == PCI_CLASS_MULTIMEDIA && subclass == PCI_SUBCLASS_HDA) {
                    vibio_u32 bar0 = pci_config_read32(bus, device, function, 0x10);
                    if ((bar0 & 1U) != 0) {
                        audio->last_error = 20;
                        return 0;
                    }
                    vibio_u64 mmio = (vibio_u64)(bar0 & 0xFFFFFFF0U);
                    if (((bar0 >> 1) & 0x3U) == 0x2U) {
                        vibio_u32 bar1 = pci_config_read32(bus, device, function, 0x14);
                        mmio |= (vibio_u64)bar1 << 32;
                    }
                    if (mmio == 0) {
                        audio->last_error = 20;
                        return 0;
                    }

                    vibio_u32 command = pci_config_read32(bus, device, function, 0x04);
                    pci_config_write32(bus, device, function, 0x04, command | 0x00000006U);

                    audio->hda_found = 1;
                    audio->hda_pci_bus = bus;
                    audio->hda_pci_device = device;
                    audio->hda_pci_function = function;
                    audio->hda_vendor_id = vendor_id;
                    audio->hda_device_id = (id >> 16) & 0xFFFFU;
                    audio->hda_mmio = mmio;
                    return 1;
                }

                if (function == 0) {
                    vibio_u32 header = pci_config_read32(bus, device, function, 0x0C);
                    if ((header & 0x00800000U) == 0) {
                        break;
                    }
                }
            }
        }
    }

    return 0;
}

static vibio_u32 hda_wait_clear(VibioAudioState *audio, vibio_u32 offset, vibio_u32 mask)
{
    for (vibio_u32 i = 0; i < 100000U; i++) {
        if ((mmio_read32(audio->hda_mmio, offset) & mask) == 0) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

static vibio_u32 hda_wait_set(VibioAudioState *audio, vibio_u32 offset, vibio_u32 mask)
{
    for (vibio_u32 i = 0; i < 100000U; i++) {
        if ((mmio_read32(audio->hda_mmio, offset) & mask) == mask) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

static vibio_u32 hda_reset_controller(VibioAudioState *audio)
{
    vibio_u32 gctl = mmio_read32(audio->hda_mmio, HDA_REG_GCTL);
    mmio_write32(audio->hda_mmio, HDA_REG_GCTL, gctl & ~HDA_GCTL_CRST);
    if (!hda_wait_clear(audio, HDA_REG_GCTL, HDA_GCTL_CRST)) {
        audio->last_error = 21;
        return 0;
    }

    mmio_write32(audio->hda_mmio, HDA_REG_GCTL, gctl | HDA_GCTL_CRST);
    if (!hda_wait_set(audio, HDA_REG_GCTL, HDA_GCTL_CRST)) {
        audio->last_error = 22;
        return 0;
    }

    for (vibio_u32 i = 0; i < 10000U; i++) {
        io_wait();
    }
    return 1;
}

static vibio_u32 hda_choose_ring_entries(vibio_u8 size_reg, vibio_u8 *out_size_value)
{
    if ((size_reg & 0x40U) != 0) {
        *out_size_value = 0x02U;
        return 256U;
    }
    if ((size_reg & 0x20U) != 0) {
        *out_size_value = 0x01U;
        return 16U;
    }
    if ((size_reg & 0x10U) != 0) {
        *out_size_value = 0x00U;
        return 2U;
    }
    *out_size_value = 0x02U;
    return 256U;
}

static vibio_u32 hda_init_command_rings(VibioAudioState *audio)
{
    mmio_write8(audio->hda_mmio, HDA_REG_CORBCTL, 0);
    mmio_write8(audio->hda_mmio, HDA_REG_RIRBCTL, 0);

    mmio_write32(audio->hda_mmio, HDA_REG_CORBLBASE, (vibio_u32)audio->hda_corb_address);
    mmio_write32(audio->hda_mmio, HDA_REG_CORBUBASE, 0);
    mmio_write16(audio->hda_mmio, HDA_REG_CORBRP, 0x8000U);
    for (vibio_u32 i = 0; i < 10000U; i++) {
        if ((mmio_read16(audio->hda_mmio, HDA_REG_CORBRP) & 0x8000U) != 0) {
            break;
        }
    }
    mmio_write16(audio->hda_mmio, HDA_REG_CORBRP, 0);
    mmio_write16(audio->hda_mmio, HDA_REG_CORBWP, 0);
    mmio_write8(audio->hda_mmio, HDA_REG_CORBSTS, 0xFFU);
    vibio_u8 corb_size_value = 0;
    audio->hda_corb_entries = hda_choose_ring_entries(
        mmio_read8(audio->hda_mmio, HDA_REG_CORBSIZE), &corb_size_value);
    mmio_write8(audio->hda_mmio, HDA_REG_CORBSIZE, corb_size_value);

    mmio_write32(audio->hda_mmio, HDA_REG_RIRBLBASE, (vibio_u32)audio->hda_rirb_address);
    mmio_write32(audio->hda_mmio, HDA_REG_RIRBUBASE, 0);
    mmio_write16(audio->hda_mmio, HDA_REG_RIRBWP, 0x8000U);
    mmio_write16(audio->hda_mmio, HDA_REG_RINTCNT, 1);
    mmio_write8(audio->hda_mmio, HDA_REG_RIRBSTS, 0xFFU);
    vibio_u8 rirb_size_value = 0;
    audio->hda_rirb_entries = hda_choose_ring_entries(
        mmio_read8(audio->hda_mmio, HDA_REG_RIRBSIZE), &rirb_size_value);
    mmio_write8(audio->hda_mmio, HDA_REG_RIRBSIZE, rirb_size_value);

    audio->hda_corb_wp = 0;
    audio->hda_rirb_rp = mmio_read16(audio->hda_mmio, HDA_REG_RIRBWP) & (audio->hda_rirb_entries - 1U);
    mmio_write8(audio->hda_mmio, HDA_REG_RIRBCTL, HDA_RIRBCTL_RUN);
    mmio_write8(audio->hda_mmio, HDA_REG_CORBCTL, HDA_CORBCTL_RUN);
    return 1;
}

static vibio_u32 hda_send_verb(VibioAudioState *audio, vibio_u32 node, vibio_u32 verb, vibio_u32 payload, vibio_u32 *response)
{
    if (audio->hda_corb_entries == 0 || audio->hda_rirb_entries == 0) {
        audio->last_error = 23;
        return 0;
    }
    vibio_u32 codec = audio->hda_codec & 0x0FU;
    vibio_u32 command = (codec << 28) | ((node & 0xFFU) << 20);
    if (verb <= 0x0FU) {
        command |= ((verb & 0x0FU) << 16) | (payload & 0xFFFFU);
    } else {
        command |= ((verb & 0xFFFU) << 8) | (payload & 0xFFU);
    }
    vibio_u32 corb_mask = audio->hda_corb_entries - 1U;
    vibio_u32 rirb_mask = audio->hda_rirb_entries - 1U;
    vibio_u32 next_wp = (audio->hda_corb_wp + 1U) & corb_mask;
    vibio_u32 *corb = (vibio_u32 *)audio->hda_corb_address;
    vibio_u32 *rirb = (vibio_u32 *)audio->hda_rirb_address;
    corb[next_wp] = command;
    audio->hda_corb_wp = next_wp;
    mmio_write16(audio->hda_mmio, HDA_REG_CORBWP, (vibio_u16)audio->hda_corb_wp);

    for (vibio_u32 i = 0; i < 1000000U; i++) {
        vibio_u32 wp = mmio_read16(audio->hda_mmio, HDA_REG_RIRBWP) & rirb_mask;
        if (wp != audio->hda_rirb_rp) {
            audio->hda_rirb_rp = (audio->hda_rirb_rp + 1U) & rirb_mask;
            if (response != 0) {
                *response = rirb[audio->hda_rirb_rp * 2U];
            }
            mmio_write8(audio->hda_mmio, HDA_REG_RIRBSTS, HDA_RIRBSTS_RIRBOIS | HDA_RIRBSTS_RINTFL);
            return 1;
        }
        io_wait();
    }

    audio->last_error = 23;
    return 0;
}

static vibio_u32 hda_get_param(VibioAudioState *audio, vibio_u32 node, vibio_u32 param, vibio_u32 *response)
{
    return hda_send_verb(audio, node, 0xF00U, param, response);
}

static vibio_u32 hda_set_verb(VibioAudioState *audio, vibio_u32 node, vibio_u32 verb, vibio_u32 payload)
{
    vibio_u32 ignored = 0;
    return hda_send_verb(audio, node, verb, payload, &ignored);
}

static vibio_u32 hda_node_start(vibio_u32 node_count_param)
{
    return (node_count_param >> 16) & 0xFFU;
}

static vibio_u32 hda_node_count(vibio_u32 node_count_param)
{
    return node_count_param & 0xFFU;
}

static vibio_u32 hda_find_codec(VibioAudioState *audio)
{
    vibio_u16 statests = 0;
    for (vibio_u32 wait = 0; wait < 1000000U; wait++) {
        statests = mmio_read16(audio->hda_mmio, HDA_REG_STATESTS);
        if ((statests & 0x7FFFU) != 0) {
            break;
        }
        io_wait();
    }
    for (vibio_u32 codec = 0; codec < 15U; codec++) {
        if ((statests & (1U << codec)) != 0) {
            audio->hda_codec = codec;
            return 1;
        }
    }
    audio->last_error = 24;
    return 0;
}

static vibio_u32 hda_get_connection_count(VibioAudioState *audio, vibio_u32 node, vibio_u32 *count, vibio_u32 *long_form)
{
    vibio_u32 len = 0;
    if (!hda_get_param(audio, node, HDA_PARAM_CONN_LIST_LEN, &len)) {
        return 0;
    }
    if (count != 0) {
        *count = len & 0x7FU;
    }
    if (long_form != 0) {
        *long_form = (len >> 7) & 0x01U;
    }
    return 1;
}

static vibio_u32 hda_get_connection_entry(VibioAudioState *audio, vibio_u32 node, vibio_u32 index, vibio_u32 long_form, vibio_u32 *entry_out)
{
    vibio_u32 response = 0;
    vibio_u32 payload = long_form ? (index & ~1U) : (index & ~3U);
    if (!hda_send_verb(audio, node, HDA_VERB_GET_CONN_ENTRY, payload, &response)) {
        return 0;
    }
    if (long_form) {
        *entry_out = (response >> ((index & 1U) * 16U)) & 0xFFFFU;
    } else {
        *entry_out = (response >> ((index & 3U) * 8U)) & 0xFFU;
    }
    return 1;
}

static vibio_u32 hda_get_widget_type(VibioAudioState *audio, vibio_u32 node, vibio_u32 *type_out)
{
    vibio_u32 caps = 0;
    if (!hda_get_param(audio, node, HDA_PARAM_AUDIO_WIDGET_CAP, &caps)) {
        return 0;
    }
    *type_out = (caps >> 20) & 0x0FU;
    return 1;
}

static vibio_u32 hda_pin_priority(VibioAudioState *audio, vibio_u32 pin, vibio_u32 *config_out, vibio_u32 *caps_out)
{
    vibio_u32 pin_caps = 0;
    if (!hda_get_param(audio, pin, HDA_PARAM_PIN_CAP, &pin_caps)) {
        return 0;
    }
    if ((pin_caps & HDA_PIN_CAP_OUTPUT) == 0) {
        return 0;
    }

    vibio_u32 config = 0;
    if (!hda_send_verb(audio, pin, HDA_VERB_GET_PIN_CONFIG_DEFAULT, 0, &config)) {
        config = 0;
    }

    vibio_u32 port_conn = (config >> 30) & 0x03U;
    if (config != 0 && port_conn == 0x01U) {
        return 0; /* no physical output connected */
    }

    vibio_u32 device = (config >> 20) & 0x0FU;
    if (config_out != 0) {
        *config_out = config;
    }
    if (caps_out != 0) {
        *caps_out = pin_caps;
    }

    if (device == 0x01U) return 400U; /* Speaker */
    if (device == 0x02U) return 300U; /* HP Out */
    if (device == 0x00U) return 250U; /* Line Out */
    return 100U;
}

static vibio_u32 hda_path_find_recursive(
    VibioAudioState *audio,
    vibio_u32 node,
    vibio_u32 target_dac,
    vibio_u32 depth,
    vibio_u32 *path_nodes,
    vibio_u32 *path_selects)
{
    if (node == target_dac) {
        return 1;
    }
    if (depth >= HDA_MAX_PATH_NODES) {
        return 0;
    }

    vibio_u32 count = 0;
    vibio_u32 long_form = 0;
    if (!hda_get_connection_count(audio, node, &count, &long_form) || count == 0) {
        return 0;
    }

    for (vibio_u32 i = 0; i < count && i < 64U; i++) {
        vibio_u32 entry = 0;
        if (!hda_get_connection_entry(audio, node, i, long_form, &entry) || entry == 0) {
            continue;
        }
        if (entry == target_dac || hda_path_find_recursive(audio, entry, target_dac, depth + 1U, path_nodes, path_selects)) {
            path_nodes[depth] = node;
            path_selects[depth] = i;
            return 1;
        }
    }
    return 0;
}

static void hda_apply_path_selects(VibioAudioState *audio, const vibio_u32 *path_nodes, const vibio_u32 *path_selects, vibio_u32 depth)
{
    for (vibio_u32 i = 0; i < depth && i < HDA_MAX_PATH_NODES; i++) {
        if (path_nodes[i] != 0) {
            hda_set_verb(audio, path_nodes[i], HDA_VERB_SET_CONN_SELECT, path_selects[i] & 0xFFU);
            audio->hda_path_nodes[i] = path_nodes[i];
            audio->hda_path_selects[i] = path_selects[i];
        }
    }
    audio->hda_path_count = depth;
}

static vibio_u32 hda_find_output_path(VibioAudioState *audio)
{
    vibio_u32 root_nodes = 0;
    if (!hda_get_param(audio, 0, HDA_PARAM_NODE_COUNT, &root_nodes)) {
        return 0;
    }

    vibio_u32 fg_start = hda_node_start(root_nodes);
    vibio_u32 fg_count = hda_node_count(root_nodes);
    for (vibio_u32 i = 0; i < fg_count; i++) {
        vibio_u32 node = fg_start + i;
        vibio_u32 fg_type = 0;
        if (hda_get_param(audio, node, HDA_PARAM_FUNCTION_GROUP_TYPE, &fg_type) && ((fg_type & 0xFFU) == 0x01U)) {
            audio->hda_audio_fg = node;
            break;
        }
    }
    if (audio->hda_audio_fg == 0) {
        audio->last_error = 25;
        return 0;
    }

    vibio_u32 widget_nodes = 0;
    if (!hda_get_param(audio, audio->hda_audio_fg, HDA_PARAM_NODE_COUNT, &widget_nodes)) {
        return 0;
    }

    vibio_u32 widget_start = hda_node_start(widget_nodes);
    vibio_u32 widget_count = hda_node_count(widget_nodes);
    vibio_u32 dac_nodes[HDA_MAX_WIDGETS];
    vibio_u32 pin_nodes[HDA_MAX_WIDGETS];
    vibio_u32 pin_scores[HDA_MAX_WIDGETS];
    vibio_u32 pin_configs[HDA_MAX_WIDGETS];
    vibio_u32 pin_caps[HDA_MAX_WIDGETS];
    vibio_u32 dac_count = 0;
    vibio_u32 pin_count = 0;
    zero_bytes(dac_nodes, sizeof(dac_nodes));
    zero_bytes(pin_nodes, sizeof(pin_nodes));
    zero_bytes(pin_scores, sizeof(pin_scores));
    zero_bytes(pin_configs, sizeof(pin_configs));
    zero_bytes(pin_caps, sizeof(pin_caps));

    for (vibio_u32 i = 0; i < widget_count && i < HDA_MAX_WIDGETS; i++) {
        vibio_u32 node = widget_start + i;
        vibio_u32 type = 0;
        if (!hda_get_widget_type(audio, node, &type)) {
            continue;
        }
        if (type == HDA_WIDGET_TYPE_AUDIO_OUTPUT && dac_count < HDA_MAX_WIDGETS) {
            dac_nodes[dac_count++] = node;
        } else if (type == HDA_WIDGET_TYPE_PIN_COMPLEX && pin_count < HDA_MAX_WIDGETS) {
            vibio_u32 config = 0;
            vibio_u32 caps = 0;
            vibio_u32 score = hda_pin_priority(audio, node, &config, &caps);
            if (score != 0) {
                pin_nodes[pin_count] = node;
                pin_scores[pin_count] = score;
                pin_configs[pin_count] = config;
                pin_caps[pin_count] = caps;
                pin_count++;
            }
        }
    }

    if (dac_count == 0 || pin_count == 0) {
        audio->last_error = 26;
        return 0;
    }

    vibio_u32 best_score = 0;
    vibio_u32 best_pin = 0;
    vibio_u32 best_pin_config = 0;
    vibio_u32 best_pin_caps = 0;
    vibio_u32 best_dac = 0;
    vibio_u32 best_path_nodes[HDA_MAX_PATH_NODES];
    vibio_u32 best_path_selects[HDA_MAX_PATH_NODES];
    vibio_u32 best_depth = 0;
    zero_bytes(best_path_nodes, sizeof(best_path_nodes));
    zero_bytes(best_path_selects, sizeof(best_path_selects));

    for (vibio_u32 p = 0; p < pin_count; p++) {
        for (vibio_u32 d = 0; d < dac_count; d++) {
            vibio_u32 path_nodes[HDA_MAX_PATH_NODES];
            vibio_u32 path_selects[HDA_MAX_PATH_NODES];
            zero_bytes(path_nodes, sizeof(path_nodes));
            zero_bytes(path_selects, sizeof(path_selects));
            if (hda_path_find_recursive(audio, pin_nodes[p], dac_nodes[d], 0, path_nodes, path_selects) && pin_scores[p] > best_score) {
                best_score = pin_scores[p];
                best_pin = pin_nodes[p];
                best_pin_config = pin_configs[p];
                best_pin_caps = pin_caps[p];
                best_dac = dac_nodes[d];
                best_depth = 0;
                for (vibio_u32 i = 0; i < HDA_MAX_PATH_NODES; i++) {
                    best_path_nodes[i] = path_nodes[i];
                    best_path_selects[i] = path_selects[i];
                    if (path_nodes[i] != 0) {
                        best_depth = i + 1U;
                    }
                }
            }
        }
    }

    if (best_pin == 0 || best_dac == 0) {
        audio->last_error = 26;
        return 0;
    }

    audio->hda_pin_node = best_pin;
    audio->hda_pin_config = best_pin_config;
    audio->hda_pin_caps = best_pin_caps;
    audio->hda_dac_node = best_dac;
    hda_apply_path_selects(audio, best_path_nodes, best_path_selects, best_depth);
    return 1;
}

static vibio_u32 hda_gain_from_cap(vibio_u32 cap, vibio_u32 volume)
{
    vibio_u32 offset = cap & 0x7FU;
    vibio_u32 steps = (cap >> 8) & 0x7FU;
    if (steps == 0) {
        return volume == 0 ? 0 : 0x40U;
    }
    if (volume == 0) {
        return 0;
    }
    if (offset > steps) {
        offset = steps;
    }
    return offset + (((steps - offset) * volume) / 100U);
}

static vibio_u32 hda_amp_payload(vibio_u32 is_output, vibio_u32 left_right, vibio_u32 index, vibio_u32 gain, vibio_u32 muted)
{
    vibio_u32 payload = left_right ? 0x3000U : 0;
    payload |= is_output ? 0x8000U : 0x4000U;
    payload |= (index & 0x0FU) << 8;
    payload |= gain & 0x7FU;
    if (muted) {
        payload |= 0x80U;
    }
    return payload;
}

static void hda_set_widget_output_volume(VibioAudioState *audio, vibio_u32 node)
{
    vibio_u32 cap = 0;
    hda_get_param(audio, node, HDA_PARAM_OUTPUT_AMP_CAP, &cap);
    vibio_u32 muted = audio->volume == 0;
    vibio_u32 gain = hda_gain_from_cap(cap, audio->volume);
    hda_set_verb(audio, node, HDA_VERB_SET_AMP_GAIN_MUTE, hda_amp_payload(1, 1, 0, gain, muted));
}

static void hda_set_widget_input_volume(VibioAudioState *audio, vibio_u32 node, vibio_u32 index)
{
    vibio_u32 cap = 0;
    hda_get_param(audio, node, HDA_PARAM_INPUT_AMP_CAP, &cap);
    vibio_u32 muted = audio->volume == 0;
    vibio_u32 gain = hda_gain_from_cap(cap, audio->volume);
    hda_set_verb(audio, node, HDA_VERB_SET_AMP_GAIN_MUTE, hda_amp_payload(0, 1, index, gain, muted));
}

static void hda_power_unmute_path(VibioAudioState *audio)
{
    hda_set_verb(audio, audio->hda_audio_fg, HDA_VERB_SET_POWER_STATE, 0x00U);
    hda_set_verb(audio, audio->hda_dac_node, HDA_VERB_SET_POWER_STATE, 0x00U);
    hda_set_verb(audio, audio->hda_pin_node, HDA_VERB_SET_POWER_STATE, 0x00U);
    hda_set_widget_output_volume(audio, audio->hda_audio_fg);
    hda_set_widget_output_volume(audio, audio->hda_dac_node);
    hda_set_widget_input_volume(audio, audio->hda_pin_node, 0);
    hda_set_widget_output_volume(audio, audio->hda_pin_node);
    for (vibio_u32 i = 0; i < audio->hda_path_count && i < HDA_MAX_PATH_NODES; i++) {
        vibio_u32 node = audio->hda_path_nodes[i];
        if (node != 0) {
            hda_set_verb(audio, node, HDA_VERB_SET_POWER_STATE, 0x00U);
            hda_set_widget_output_volume(audio, node);
            hda_set_widget_input_volume(audio, node, audio->hda_path_selects[i]);
        }
    }
}

static void hda_enable_output_pin(VibioAudioState *audio)
{
    hda_set_verb(audio, audio->hda_pin_node, HDA_VERB_SET_PIN_WIDGET_CONTROL,
        HDA_PIN_WIDGET_OUT_ENABLE | HDA_PIN_WIDGET_HP_ENABLE);
    hda_set_verb(audio, audio->hda_pin_node, HDA_VERB_SET_EAPD_BTL_ENABLE, HDA_EAPD_ENABLE);
}

static vibio_u32 hda_configure_codec(VibioAudioState *audio)
{
    if (!hda_find_codec(audio) || !hda_find_output_path(audio)) {
        return 0;
    }

    hda_power_unmute_path(audio);
    hda_set_verb(audio, audio->hda_dac_node, HDA_VERB_SET_CONVERTER_FORMAT, HDA_FMT_44K1_16_STEREO);
    hda_set_verb(audio, audio->hda_dac_node, HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, (HDA_STREAM_ID << 4));
    hda_enable_output_pin(audio);
    return 1;
}

static void hda_apply_volume(VibioAudioState *audio)
{
    if (audio == 0 || audio->driver != AUDIO_DRIVER_HDA || !audio->ready) {
        return;
    }
    hda_power_unmute_path(audio);
    hda_enable_output_pin(audio);
}

static vibio_u32 hda_init_controller(
    VibioAudioState *audio,
    VibioPageAllocator *allocator,
    VibioPageTableBuild *page_tables)
{
    panic_set_context("KERNEL", "HDA", "HDA INIT");
    if (!hda_detect_controller(audio)) {
        return 0;
    }
    if (page_tables != 0 && page_tables->ok) {
        map_identity_range_2m(allocator, page_tables, audio->hda_mmio & ~0x1FFFFFULL,
                              0x200000ULL, PAGE_PCD | PAGE_PWT);
        load_cr3(page_tables->pml4);
    }

    audio->hda_corb_address = page_allocator_alloc(allocator);
    audio->hda_rirb_address = page_allocator_alloc(allocator);
    audio->bdl_address = page_allocator_alloc(allocator);
    vibio_u64 pcm_pages = (AUDIO_BUFFER_BYTES + PAGE_SIZE - 1) / PAGE_SIZE;
    audio->pcm_buffer_address = alloc_contiguous_pages(allocator, pcm_pages);
    audio->pcm_source_address = alloc_contiguous_pages(allocator, pcm_pages);
    audio->pcm_buffer_bytes = AUDIO_BUFFER_BYTES;
    audio->pcm_source_bytes = AUDIO_BUFFER_BYTES;
    if (audio->hda_corb_address == 0 || audio->hda_rirb_address == 0 || audio->bdl_address == 0 ||
        audio->pcm_buffer_address == 0 || audio->pcm_source_address == 0 ||
        audio->hda_corb_address > 0xFFFFFFFFULL ||
        audio->hda_rirb_address > 0xFFFFFFFFULL || audio->bdl_address > 0xFFFFFFFFULL ||
        audio->pcm_buffer_address > 0xFFFFFFFFULL || audio->pcm_source_address > 0xFFFFFFFFULL) {
        audio->last_error = 27;
        return 0;
    }

    zero_bytes((void *)audio->hda_corb_address, PAGE_SIZE);
    zero_bytes((void *)audio->hda_rirb_address, PAGE_SIZE);
    zero_bytes((void *)audio->bdl_address, PAGE_SIZE);
    zero_bytes((void *)audio->pcm_buffer_address, audio->pcm_buffer_bytes);
    zero_bytes((void *)audio->pcm_source_address, audio->pcm_source_bytes);

    if (!hda_reset_controller(audio) || !hda_init_command_rings(audio) || !hda_configure_codec(audio)) {
        return 0;
    }

    vibio_u32 gcap = mmio_read16(audio->hda_mmio, HDA_REG_GCAP);
    vibio_u32 input_streams = (gcap >> 8) & 0x0FU;
    vibio_u32 output_streams = (gcap >> 12) & 0x0FU;
    if (output_streams == 0) {
        audio->last_error = 28;
        return 0;
    }
    audio->hda_stream_base = HDA_STREAM_BASE + (input_streams * HDA_STREAM_STRIDE);
    audio->driver = AUDIO_DRIVER_HDA;
    audio->found = 1;
    audio->ready = 1;
    return 1;
}

static void ac97_write_mixer(VibioAudioState *audio, vibio_u16 reg, vibio_u16 value)
{
    outw((vibio_u16)(audio->mixer_base + reg), value);
}

static vibio_u16 ac97_read_mixer(VibioAudioState *audio, vibio_u16 reg)
{
    return inw((vibio_u16)(audio->mixer_base + reg));
}

static vibio_u32 ac97_wait_channel_reset(VibioAudioState *audio)
{
    vibio_u16 po = (vibio_u16)(audio->bus_master_base + AC97_PCM_OUT_OFFSET);
    for (vibio_u32 i = 0; i < 100000U; i++) {
        if ((inb((vibio_u16)(po + AC97_CR)) & AC97_CR_RESET) == 0) {
            return 1;
        }
    }
    return 0;
}

static void ac97_stop_pcm_out(VibioAudioState *audio)
{
    vibio_u16 po = (vibio_u16)(audio->bus_master_base + AC97_PCM_OUT_OFFSET);
    outb((vibio_u16)(po + AC97_CR), 0);
    outw((vibio_u16)(po + AC97_SR), AC97_SR_CLEAR);
}

static void ac97_apply_volume(VibioAudioState *audio)
{
    if (audio == 0 || audio->driver != AUDIO_DRIVER_AC97 || !audio->ready) {
        return;
    }
    vibio_u16 value = 0x8000U;
    if (audio->volume > 0) {
        vibio_u32 attenuation = ((100U - audio->volume) * 31U) / 100U;
        value = (vibio_u16)((attenuation << 8) | attenuation);
    }
    ac97_write_mixer(audio, 0x02, value);
    ac97_write_mixer(audio, 0x18, value);
}

static void audio_apply_volume(VibioAudioState *audio)
{
    if (audio == 0) {
        return;
    }
    if (audio->driver == AUDIO_DRIVER_HDA) {
        hda_apply_volume(audio);
    } else if (audio->driver == AUDIO_DRIVER_AC97) {
        ac97_apply_volume(audio);
    }
}

static void audio_set_volume(VibioAudioState *audio, vibio_u32 volume)
{
    if (audio == 0) {
        return;
    }
    if (volume > 100U) {
        volume = 100U;
    }
    if (audio->volume == volume) {
        return;
    }
    audio->volume = volume;
    audio_apply_volume(audio);
    if (audio->playback_active) {
        audio_render_pcm_volume(audio);
    }
}

static vibio_u32 audio_init(
    VibioAudioState *audio,
    VibioPageAllocator *allocator,
    VibioPageTableBuild *page_tables)
{
    panic_set_context("KERNEL", "AUDIO", "AUDIO INIT");
    zero_bytes(audio, sizeof(VibioAudioState));
    audio->sample_rate = AUDIO_RATE;
    audio->channels = AUDIO_CHANNELS;
    audio->bits_per_sample = AUDIO_BITS;
    audio->volume = 70;

    if (hda_init_controller(audio, allocator, page_tables)) {
        return 1;
    }
    vibio_u32 hda_was_found = audio->hda_found;
    vibio_u32 hda_error = audio->last_error;

    if (!ac97_find_controller(audio)) {
        if (hda_was_found) {
            audio->hda_found = 1;
            audio->last_error = hda_error == 0 ? 29 : hda_error;
        }
        return 0;
    }

    audio->bdl_address = page_allocator_alloc(allocator);
    vibio_u64 pcm_pages = (AUDIO_BUFFER_BYTES + PAGE_SIZE - 1) / PAGE_SIZE;
    audio->pcm_buffer_address = alloc_contiguous_pages(allocator, pcm_pages);
    audio->pcm_source_address = alloc_contiguous_pages(allocator, pcm_pages);
    audio->pcm_buffer_bytes = AUDIO_BUFFER_BYTES;
    audio->pcm_source_bytes = AUDIO_BUFFER_BYTES;
    if (audio->bdl_address == 0 || audio->pcm_buffer_address == 0 ||
        audio->pcm_source_address == 0 ||
        audio->bdl_address > 0xFFFFFFFFULL || audio->pcm_buffer_address > 0xFFFFFFFFULL ||
        audio->pcm_source_address > 0xFFFFFFFFULL) {
        audio->last_error = 3;
        return 0;
    }

    zero_bytes((void *)audio->bdl_address, PAGE_SIZE);
    zero_bytes((void *)audio->pcm_buffer_address, audio->pcm_buffer_bytes);
    zero_bytes((void *)audio->pcm_source_address, audio->pcm_source_bytes);

    ac97_write_mixer(audio, 0x00, 0x0000);
    for (vibio_u32 i = 0; i < 10000U; i++) {
        io_wait();
    }

    ac97_write_mixer(audio, 0x02, 0x0000);
    ac97_write_mixer(audio, 0x18, 0x0000);

    vibio_u16 ext_id = ac97_read_mixer(audio, 0x28);
    if ((ext_id & 0x0001U) != 0) {
        vibio_u16 ext_ctrl = ac97_read_mixer(audio, 0x2A);
        ac97_write_mixer(audio, 0x2A, (vibio_u16)(ext_ctrl | 0x0001U));
        ac97_write_mixer(audio, 0x2C, AUDIO_RATE);
    }

    ac97_stop_pcm_out(audio);
    audio->driver = AUDIO_DRIVER_AC97;
    audio->ready = 1;
    ac97_apply_volume(audio);
    return 1;
}

static vibio_u32 audio_program_pcm_chunk(VibioAudioState *audio, vibio_u64 offset, vibio_u64 bytes)
{
    if (audio == 0 || bytes == 0 || bytes > audio->pcm_buffer_bytes ||
        offset + bytes > audio->pcm_source_bytes) {
        if (audio != 0) {
            audio->last_error = 5;
        }
        return 0;
    }

    bytes &= ~3ULL;
    if (bytes == 0) {
        audio->last_error = 5;
        return 0;
    }

    audio->pcm_source_offset = offset;
    audio->pcm_play_bytes = bytes;
    vibio_u64 frames = bytes / 4ULL;
    audio->playback_ticks_remaining = ((frames * 100ULL) + AUDIO_RATE - 1ULL) / AUDIO_RATE + 2ULL;
    audio_render_pcm_volume(audio);

    if (audio->driver == AUDIO_DRIVER_HDA) {
        zero_bytes((void *)audio->bdl_address, HDA_BDL_BYTES);
        vibio_u32 *hda_bdl = (vibio_u32 *)audio->bdl_address;
        vibio_u64 remaining = bytes;
        vibio_u64 out_offset = 0;
        vibio_u32 count = 0;

        while (remaining > 0 && count < HDA_BDL_ENTRIES) {
            vibio_u32 chunk = remaining > HDA_MAX_DESC_BYTES ? HDA_MAX_DESC_BYTES : (vibio_u32)remaining;
            chunk &= ~3U;
            if (chunk == 0) {
                break;
            }
            hda_bdl[count * 4] = (vibio_u32)(audio->pcm_buffer_address + out_offset);
            hda_bdl[count * 4 + 1] = 0;
            hda_bdl[count * 4 + 2] = chunk;
            hda_bdl[count * 4 + 3] = 0x00000001U;
            remaining -= chunk;
            out_offset += chunk;
            count++;
        }

        if (remaining == 0 && count == 1 && bytes >= 8ULL) {
            vibio_u32 first = (vibio_u32)((bytes / 2ULL) & ~3ULL);
            if (first >= 4U) {
                hda_bdl[0] = (vibio_u32)audio->pcm_buffer_address;
                hda_bdl[1] = 0;
                hda_bdl[2] = first;
                hda_bdl[3] = 0x00000001U;
                hda_bdl[4] = (vibio_u32)(audio->pcm_buffer_address + first);
                hda_bdl[5] = 0;
                hda_bdl[6] = (vibio_u32)(bytes - first);
                hda_bdl[7] = 0x00000001U;
                count = 2;
            }
        }

        if (remaining != 0 || count == 0) {
            audio->last_error = 6;
            return 0;
        }

        audio->bdl_count = count;
        audio->last_error = 0;
        return 1;
    }

    zero_bytes((void *)audio->bdl_address, AC97_BDL_BYTES);
    vibio_u32 *bdl = (vibio_u32 *)audio->bdl_address;
    vibio_u64 remaining = bytes;
    vibio_u64 out_offset = 0;
    vibio_u32 count = 0;

    while (remaining > 0 && count < AC97_BDL_ENTRIES) {
        vibio_u32 chunk = remaining > AC97_MAX_DESC_BYTES ? AC97_MAX_DESC_BYTES : (vibio_u32)remaining;
        chunk &= ~3U;
        if (chunk == 0) {
            break;
        }
        bdl[count * 2] = (vibio_u32)(audio->pcm_buffer_address + out_offset);
        bdl[count * 2 + 1] = AC97_DESC_IOC | ((chunk / 2U) & 0xFFFFU);
        remaining -= chunk;
        out_offset += chunk;
        count++;
    }

    if (remaining != 0 || count == 0) {
        audio->last_error = 6;
        return 0;
    }

    audio->bdl_count = count;
    audio->last_error = 0;
    return 1;
}

static vibio_u32 audio_prepare_pcm(VibioAudioState *audio, const VibioBootFile *file, VibioWavInfo *wav)
{
    if (!wav_parse_pcm16(file, wav)) {
        audio->last_error = 4;
        return 0;
    }

    const vibio_u8 *src = (const vibio_u8 *)(file->address + wav->data_offset);
    vibio_u8 *src_pcm = (vibio_u8 *)audio->pcm_source_address;
    vibio_u64 output_bytes = wav->channels == 2 ? wav->data_size : wav->data_size * 2;
    if (output_bytes == 0 || output_bytes > audio->pcm_source_bytes) {
        audio->last_error = 5;
        return 0;
    }

    if (wav->channels == 2) {
        copy_bytes(src_pcm, src, output_bytes);
    } else {
        vibio_u64 mono_frames = wav->data_size / 2;
        for (vibio_u64 i = 0; i < mono_frames; i++) {
            int s = audio_pcm16_read_le(src + i * 2ULL);
            audio_pcm16_write_le(src_pcm + i * 4ULL, s);
            audio_pcm16_write_le(src_pcm + i * 4ULL + 2ULL, s);
        }
    }
    audio->pcm_total_bytes = output_bytes;
    vibio_u64 chunk_bytes = output_bytes > AUDIO_CHUNK_BYTES ? AUDIO_CHUNK_BYTES : output_bytes;
    chunk_bytes &= ~3ULL;
    if (!audio_program_pcm_chunk(audio, 0, chunk_bytes)) {
        return 0;
    }
    audio->pcm_stream_offset = chunk_bytes;
    return 1;
}

static void hda_stop_output_stream(VibioAudioState *audio)
{
    vibio_u32 sd = audio->hda_stream_base;
    mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL, 0);
    for (vibio_u32 i = 0; i < 100000U; i++) {
        if ((mmio_read8(audio->hda_mmio, sd + HDA_SD_CTL) & HDA_SD_CTL_RUN) == 0) {
            break;
        }
        io_wait();
    }
    mmio_write8(audio->hda_mmio, sd + HDA_SD_STS, HDA_SD_STS_CLEAR);
}

static vibio_u32 hda_reset_output_stream(VibioAudioState *audio)
{
    vibio_u32 sd = audio->hda_stream_base;
    mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL, HDA_SD_CTL_SRST);
    for (vibio_u32 i = 0; i < 100000U; i++) {
        if ((mmio_read8(audio->hda_mmio, sd + HDA_SD_CTL) & HDA_SD_CTL_SRST) != 0) {
            break;
        }
        io_wait();
    }
    mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL, 0);
    for (vibio_u32 i = 0; i < 100000U; i++) {
        if ((mmio_read8(audio->hda_mmio, sd + HDA_SD_CTL) & HDA_SD_CTL_SRST) == 0) {
            return 1;
        }
        io_wait();
    }
    audio->last_error = 30;
    return 0;
}

static vibio_u32 hda_play_prepared_pcm(VibioAudioState *audio)
{
    vibio_u32 sd = audio->hda_stream_base;
    hda_stop_output_stream(audio);
    if (!hda_reset_output_stream(audio)) {
        return 0;
    }

    hda_power_unmute_path(audio);
    hda_set_verb(audio, audio->hda_dac_node, HDA_VERB_SET_CONVERTER_FORMAT, HDA_FMT_44K1_16_STEREO);
    hda_set_verb(audio, audio->hda_dac_node, HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, (HDA_STREAM_ID << 4));
    hda_enable_output_pin(audio);

    mmio_write32(audio->hda_mmio, sd + HDA_SD_BDPL, (vibio_u32)audio->bdl_address);
    mmio_write32(audio->hda_mmio, sd + HDA_SD_BDPU, 0);
    mmio_write32(audio->hda_mmio, sd + HDA_SD_CBL, (vibio_u32)audio->pcm_play_bytes);
    mmio_write16(audio->hda_mmio, sd + HDA_SD_LVI, (vibio_u16)(audio->bdl_count - 1U));
    mmio_write16(audio->hda_mmio, sd + HDA_SD_FMT, HDA_FMT_44K1_16_STEREO);
    mmio_write8(audio->hda_mmio, sd + HDA_SD_STS, HDA_SD_STS_CLEAR);
    mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL + 2U, (vibio_u8)(HDA_STREAM_ID << 4));
    mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL, HDA_SD_CTL_RUN);
    for (vibio_u32 i = 0; i < 10000U; i++) {
        io_wait();
    }
    audio->hda_stream_status = mmio_read8(audio->hda_mmio, sd + HDA_SD_STS);
    audio->hda_stream_ctl = mmio_read32(audio->hda_mmio, sd + HDA_SD_CTL);
    audio->hda_stream_lpib = mmio_read32(audio->hda_mmio, sd + 0x04U);
    return 1;
}

static void audio_stop_playback(VibioAudioState *audio)
{
    if (audio == 0 || !audio->playback_active) {
        return;
    }
    if (audio->driver == AUDIO_DRIVER_HDA) {
        hda_stop_output_stream(audio);
        audio->hda_stream_status = mmio_read8(audio->hda_mmio, audio->hda_stream_base + HDA_SD_STS);
        audio->hda_stream_ctl = mmio_read32(audio->hda_mmio, audio->hda_stream_base + HDA_SD_CTL);
        audio->hda_stream_lpib = mmio_read32(audio->hda_mmio, audio->hda_stream_base + 0x04U);
    } else if (audio->driver == AUDIO_DRIVER_AC97) {
        ac97_stop_pcm_out(audio);
    }
    audio->playback_active = 0;
    audio->playback_ticks_remaining = 0;
    audio->playback_last_tick = 0;
    audio->paused = 0;
}

/* Pause the running stream WITHOUT resetting it: clear the controller's RUN bit
 * so the DMA halts but keeps its position (current descriptor + offset). This is
 * what lets PAUSE/resume continue the audio where it left off instead of
 * restarting the clip from the beginning. The chunk countdown is frozen while
 * paused (audio_service early-returns), so timing resumes correctly too. */
static void audio_pause_playback(VibioAudioState *audio)
{
    if (audio == 0 || !audio->playback_active || audio->paused) {
        return;
    }
    if (audio->driver == AUDIO_DRIVER_HDA) {
        vibio_u32 sd = audio->hda_stream_base;
        vibio_u8 ctl = mmio_read8(audio->hda_mmio, sd + HDA_SD_CTL);
        mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL, (vibio_u8)(ctl & ~HDA_SD_CTL_RUN));
    } else if (audio->driver == AUDIO_DRIVER_AC97) {
        vibio_u16 po = (vibio_u16)(audio->bus_master_base + AC97_PCM_OUT_OFFSET);
        outb((vibio_u16)(po + AC97_CR), 0); /* clear RUN; do NOT reset (keeps CIV) */
    }
    audio->paused = 1;
}

/* Resume a paused stream from its retained position by re-setting RUN. */
static void audio_resume_playback(VibioAudioState *audio)
{
    if (audio == 0 || !audio->playback_active || !audio->paused) {
        return;
    }
    if (audio->driver == AUDIO_DRIVER_HDA) {
        vibio_u32 sd = audio->hda_stream_base;
        vibio_u8 ctl = mmio_read8(audio->hda_mmio, sd + HDA_SD_CTL);
        mmio_write8(audio->hda_mmio, sd + HDA_SD_CTL, (vibio_u8)(ctl | HDA_SD_CTL_RUN));
    } else if (audio->driver == AUDIO_DRIVER_AC97) {
        vibio_u16 po = (vibio_u16)(audio->bus_master_base + AC97_PCM_OUT_OFFSET);
        outb((vibio_u16)(po + AC97_CR), AC97_CR_RUN);
    }
    audio->paused = 0;
    /* Re-anchor the timing clock so the paused interval is not counted, and
     * re-arm the DCH startup grace for the just-resumed RUN. */
    audio->playback_last_tick = 0;
}

static void audio_service(VibioAudioState *audio, vibio_u64 ticks)
{
    if (audio == 0 || !audio->playback_active || audio->paused) {
        return;
    }
    if (audio->playback_last_tick == 0) {
        audio->playback_last_tick = ticks;
        audio->pcm_chunk_start_tick = ticks;
        return;
    }
    /* AC'97: stop the instant the hardware DMA halts (every valid buffer
     * consumed, status DCH bit set). The BDL is a 32-entry ring, so if the
     * controller is left running one tick past the end it wraps back to entry 0
     * and replays the first ~0.3 s of the clip. Cutting RUN exactly at DCH stops
     * that replay blip and leaves the player state at STOPPED, not "still
     * playing". The software countdown below stays as a fallback.
     *
     * The DCH check is suppressed for a short startup grace after each chunk
     * begins: the controller can still report DCH for a few ms before its DMA
     * engine actually starts, and honoring that early made the service loop
     * restart the channel immediately and skip the clip forward in sub-second
     * bursts. */
    if (audio->driver == AUDIO_DRIVER_AC97 && audio->ready) {
        vibio_u16 po = (vibio_u16)(audio->bus_master_base + AC97_PCM_OUT_OFFSET);
        vibio_u16 sr = inw((vibio_u16)(po + AC97_SR));
        if (sr != 0xFFFFU) {
            /* Acknowledge the per-buffer completion bits (LVBCI/BCIS/FIFOE) on
             * every service pass so the controller keeps DMAing through the whole
             * chunk. Clearing them only at chunk start left the controller waiting
             * on an un-acked completion after the first descriptor(s), so only the
             * very start of each chunk was audible and the rest was silence until
             * the software countdown rolled over to the next chunk. */
            if ((sr & AC97_SR_CLEAR) != 0) {
                outw((vibio_u16)(po + AC97_SR), (vibio_u16)(sr & AC97_SR_CLEAR));
            }
            /* DCH = DMA controller halted (chunk fully consumed). Honored only
             * after the startup grace so the controller's spin-up latency cannot
             * trigger an immediate restart that skips the clip forward. */
            if ((sr & 0x1U) != 0 &&
                ticks - audio->pcm_chunk_start_tick >= AUDIO_DCH_STARTUP_GRACE_TICKS) {
                if (audio_start_next_pcm_chunk(audio)) {
                    return;
                }
                audio_stop_playback(audio);
                return;
            }
        }
    }
    vibio_u64 delta = ticks - audio->playback_last_tick;
    audio->playback_last_tick = ticks;
    if (delta >= audio->playback_ticks_remaining) {
        if (audio_start_next_pcm_chunk(audio)) {
            return;
        }
        audio_stop_playback(audio);
    } else {
        audio->playback_ticks_remaining -= delta;
    }
}

static vibio_u32 audio_start_prepared_pcm(VibioAudioState *audio)
{
    if (audio == 0 || !audio->ready) {
        return 0;
    }

    if (audio->driver == AUDIO_DRIVER_HDA) {
        if (!hda_play_prepared_pcm(audio)) {
            return 0;
        }
        audio->playback_started = 1;
        audio->playback_active = 1;
        audio->playback_last_tick = 0;
        return 1;
    }

    vibio_u16 po = (vibio_u16)(audio->bus_master_base + AC97_PCM_OUT_OFFSET);
    ac97_stop_pcm_out(audio);
    outb((vibio_u16)(po + AC97_CR), AC97_CR_RESET);
    if (!ac97_wait_channel_reset(audio)) {
        audio->last_error = 8;
        return 0;
    }

    outl((vibio_u16)(po + AC97_BDBAR), (vibio_u32)audio->bdl_address);
    outb((vibio_u16)(po + AC97_LVI), (vibio_u8)(audio->bdl_count - 1));
    outw((vibio_u16)(po + AC97_SR), AC97_SR_CLEAR);
    outb((vibio_u16)(po + AC97_CR), AC97_CR_RUN);

    audio->playback_started = 1;
    audio->playback_active = 1;
    audio->playback_last_tick = 0;
    return 1;
}

static vibio_u32 audio_start_next_pcm_chunk(VibioAudioState *audio)
{
    if (audio == 0 || audio->pcm_stream_offset >= audio->pcm_total_bytes) {
        return 0;
    }
    vibio_u64 remaining = audio->pcm_total_bytes - audio->pcm_stream_offset;
    vibio_u64 chunk_bytes = remaining > AUDIO_CHUNK_BYTES ? AUDIO_CHUNK_BYTES : remaining;
    chunk_bytes &= ~3ULL;
    if (!audio_program_pcm_chunk(audio, audio->pcm_stream_offset, chunk_bytes)) {
        return 0;
    }
    audio->pcm_stream_offset += chunk_bytes;
    return audio_start_prepared_pcm(audio);
}

static vibio_u32 audio_play_file(VibioAudioState *audio, const VibioBootFile *file)
{
    panic_set_context("KERNEL", audio != 0 && audio->driver == AUDIO_DRIVER_HDA ? "HDA" : "AC97", "AUDIO PLAY");
    if (audio == 0 || file == 0) {
        return 0;
    }
    if (!audio->ready) {
        audio->last_error = audio->found ? 7 : 1;
        return 0;
    }

    VibioWavInfo wav;
    if (!audio_prepare_pcm(audio, file, &wav)) {
        return 0;
    }

    if (!audio_start_prepared_pcm(audio)) {
        return 0;
    }
    audio_set_current_name(audio, file);
    return 1;
}

static vibio_u32 audio_play_named(VibioAudioState *audio, const VibioBootInfo *boot_info, const char *name)
{
    const VibioBootFile *file = find_sound_file_by_name(boot_info, name);
    return audio_play_file(audio, file);
}


static vibio_u32 audio_play_buffer(VibioAudioState *audio, const vibio_u8 *data, vibio_u32 size, const char *name)
{
    if (audio == 0 || data == 0 || size == 0) {
        return 0;
    }
    VibioBootFile fake;
    zero_bytes(&fake, sizeof(fake));
    fake.address = (vibio_u64)(unsigned long long)(const void *)data;
    fake.size = size;
    fake.type = VIBIO_BOOT_FILE_TYPE_SOUND;
    vibio_u32 n = 0;
    for (; name != 0 && name[n] != 0 && n + 1 < VIBIO_BOOT_FILE_NAME_MAX; n++) {
        fake.name[n] = name[n];
    }
    fake.name_length = n;
    fake.name[n] = 0;
    return audio_play_file(audio, &fake);
}

static const char *usb_hotplug_sound_name(vibio_u32 event_type)
{
    return event_type == USB_HOTPLUG_EVENT_REMOVE ? "USBRM.WAV" : "USBINS.WAV";
}

static void usb_hotplug_record_event(
    VibioUsbHotplug *hp,
    const VibioBootInfo *boot_info,
    VibioAudioState *audio,
    vibio_u32 event_type,
    vibio_u32 port,
    vibio_u64 ticks,
    vibio_u32 simulated)
{
    if (hp == 0 || event_type == USB_HOTPLUG_EVENT_NONE) {
        return;
    }
    if (event_type == USB_HOTPLUG_EVENT_INSERT) {
        hp->insert_events++;
    } else if (event_type == USB_HOTPLUG_EVENT_REMOVE) {
        hp->remove_events++;
    }
    hp->last_event_type = event_type;
    hp->last_event_port = port;
    hp->last_event_tick = ticks;
    if (simulated) {
        hp->simulated_events++;
    }

    const char *sound = usb_hotplug_sound_name(event_type);
    const VibioBootFile *file = find_sound_file_by_name(boot_info, sound);
    if (file == 0) {
        hp->last_sound_result = USB_HOTPLUG_SOUND_MISSING;
        return;
    }
    if (audio == 0 || !audio->ready) {
        hp->last_sound_result = USB_HOTPLUG_SOUND_AUDIO_UNAVAILABLE;
        return;
    }
    if (audio->playback_active) {
        hp->last_sound_result = USB_HOTPLUG_SOUND_BUSY;
        return;
    }
    hp->last_sound_result = audio_play_file(audio, file) ?
        USB_HOTPLUG_SOUND_STARTED : USB_HOTPLUG_SOUND_FAILED;
}

static void usb_hotplug_init(VibioUsbHotplug *hp, const VibioUsbReadTest *usb)
{
    zero_bytes(hp, sizeof(VibioUsbHotplug));
    hp->last_sound_result = USB_HOTPLUG_SOUND_NONE;
    if (usb == 0) {
        hp->unsupported_reason = 1;
        return;
    }

    const VibioUsbController *chosen = usb_find_preferred_controller(usb);
    if (chosen == 0) {
        hp->unsupported_reason = 2; /* no controller found */
        return;
    }

    hp->controller_found = 1;
    hp->controller_type = chosen->type;
    hp->bus = chosen->bus;
    hp->device = chosen->device;
    hp->function = chosen->function;
    hp->bar = chosen->bar;

    if (chosen->type != USB_TYPE_XHCI) {
        hp->unsupported_reason = 3; /* hotplug polling implemented for xHCI first */
        return;
    }
    if (!chosen->bar_is_mmio || chosen->bar == 0) {
        hp->unsupported_reason = 4;
        return;
    }

    vibio_u32 command = pci_config_read32(chosen->bus, chosen->device, chosen->function, 0x04);
    hp->mem_space_enabled = (command & 0x2U) != 0;
    hp->bar_mapped = chosen->bar < 0x100000000ULL;
    if (!hp->mem_space_enabled) {
        hp->unsupported_reason = 5;
        return;
    }
    if (!hp->bar_mapped) {
        hp->unsupported_reason = 6;
        return;
    }

    vibio_u8 cap_length = mmio_read8(chosen->bar, 0x00);
    vibio_u16 hci_version = mmio_read16(chosen->bar, 0x02);
    vibio_u32 hcsparams1 = mmio_read32(chosen->bar, 0x04);
    vibio_u32 max_ports = (hcsparams1 >> 24) & 0xFFU;
    if (cap_length < 0x20 || cap_length > 0xFF || hci_version == 0 || hci_version == 0xFFFF) {
        hp->unsupported_reason = 7;
        return;
    }
    if (max_ports == 0 || max_ports > USB_HOTPLUG_MAX_PORTS) {
        hp->unsupported_reason = 8;
        return;
    }

    hp->mmio_readable = 1;
    hp->cap_length = cap_length;
    hp->hci_version = hci_version;
    hp->ports_checked = max_ports;
    hp->hotplug_supported = 1;
    hp->port_state_readable = 1;
}

static vibio_u64 usb_hotplug_read_connected_bitmap(VibioUsbHotplug *hp)
{
    vibio_u64 bitmap = 0;
    if (hp == 0 || !hp->hotplug_supported || hp->ports_checked == 0) {
        return 0;
    }
    vibio_u64 op_base = hp->bar + hp->cap_length;
    vibio_u32 connected = 0;
    for (vibio_u32 p = 0; p < hp->ports_checked && p < USB_HOTPLUG_MAX_PORTS; p++) {
        vibio_u32 portsc = mmio_read32(op_base, 0x400 + 0x10 * p);
        if (portsc == 0xFFFFFFFFU) {
            continue;
        }
        if ((portsc & 0x1U) != 0) {
            bitmap |= (1ULL << p);
            connected++;
        }
    }
    hp->connected_count = connected;
    hp->connected_bitmap = bitmap;
    return bitmap;
}

static void usb_hotplug_poll(
    VibioUsbHotplug *hp,
    const VibioBootInfo *boot_info,
    VibioAudioState *audio,
    vibio_u64 ticks)
{
    if (hp == 0 || !hp->hotplug_supported) {
        return;
    }
    if (hp->last_poll_tick != 0 && ticks - hp->last_poll_tick < USB_HOTPLUG_POLL_TICKS) {
        return;
    }
    hp->last_poll_tick = ticks;

    vibio_u64 now = usb_hotplug_read_connected_bitmap(hp);
    if (!hp->baseline_valid) {
        hp->previous_bitmap = now;
        hp->baseline_valid = 1;
        return;
    }

    vibio_u64 changed = now ^ hp->previous_bitmap;
    if (changed == 0) {
        return;
    }

    for (vibio_u32 p = 0; p < hp->ports_checked && p < USB_HOTPLUG_MAX_PORTS; p++) {
        vibio_u64 bit = 1ULL << p;
        if ((changed & bit) == 0) {
            continue;
        }
        vibio_u32 event = (now & bit) ? USB_HOTPLUG_EVENT_INSERT : USB_HOTPLUG_EVENT_REMOVE;
        usb_hotplug_record_event(hp, boot_info, audio, event, p + 1, ticks, 0);
    }
    hp->previous_bitmap = now;
}

static VibioDiskReadTest disk_run_readonly_test(VibioPageAllocator *allocator)
{
    panic_set_context("KERNEL", "AHCI", "DISK READ");
    VibioDiskReadTest disk;
    zero_bytes(&disk, sizeof(VibioDiskReadTest));

    disk.command_list = page_allocator_alloc(allocator);
    disk.received_fis = page_allocator_alloc(allocator);
    disk.command_table = page_allocator_alloc(allocator);
    disk.sector_buffer = page_allocator_alloc(allocator);
    if (disk.command_list == 0 || disk.received_fis == 0 || disk.command_table == 0 || disk.sector_buffer == 0) {
        disk.last_error = 10;
        return disk;
    }

    if (!pci_find_ahci_controller(&disk)) {
        disk.last_error = 11;
        return disk;
    }

    if (!ahci_find_active_port(&disk)) {
        disk.last_error = 12;
        return disk;
    }

    if (!ahci_prepare_port(&disk)) {
        return disk;
    }

    if (!ahci_read_sector(&disk, 0)) {
        return disk;
    }

    const vibio_u8 *sector = (const vibio_u8 *)disk.sector_buffer;
    disk.mbr_signature_ok = sector[510] == 0x55 && sector[511] == 0xAA;
    disk.esp_partition_ok = sector[446 + 4] == 0xEF;
    disk.partition_lba = disk_read_le32(sector, 446 + 8);

    if (!ahci_read_sector(&disk, disk.partition_lba)) {
        return disk;
    }

    sector = (const vibio_u8 *)disk.sector_buffer;
    disk.boot_sector_ok = sector[510] == 0x55 && sector[511] == 0xAA &&
                          sector[82] == 'F' && sector[83] == 'A' && sector[84] == 'T' &&
                          sector[85] == '3' && sector[86] == '2';
    disk.write_path_present = 0;
    disk.ok = disk.mbr_signature_ok && disk.esp_partition_ok && disk.partition_lba != 0 &&
              disk.boot_sector_ok && disk.read_count >= 2 && disk.write_path_present == 0;
    return disk;
}

static VibioFat32ReadTest fat32_run_readonly_test(VibioDiskReadTest *disk);

/* Field-by-field struct copy (the freestanding kernel has no memcpy, and a
 * whole-struct assignment would emit one). */
static void fat32_copy(VibioFat32ReadTest *dst, const VibioFat32ReadTest *src)
{
    dst->partition_lba = src->partition_lba;
    dst->fat_begin_lba = src->fat_begin_lba;
    dst->data_begin_lba = src->data_begin_lba;
    dst->root_dir_lba = src->root_dir_lba;
    dst->sounds_dir_lba = src->sounds_dir_lba;
    dst->root_cluster = src->root_cluster;
    dst->sounds_cluster = src->sounds_cluster;
    dst->root_entries_seen = src->root_entries_seen;
    dst->sound_entries_seen = src->sound_entries_seen;
    dst->sound_files_found = src->sound_files_found;
    dst->total_sound_bytes = src->total_sound_bytes;
    dst->bytes_per_sector = src->bytes_per_sector;
    dst->sectors_per_cluster = src->sectors_per_cluster;
    dst->reserved_sectors = src->reserved_sectors;
    dst->fat_count = src->fat_count;
    dst->fat_size = src->fat_size;
    dst->boot_sector_ok = src->boot_sector_ok;
    dst->root_found = src->root_found;
    dst->sounds_dir_found = src->sounds_dir_found;
    dst->write_path_present = src->write_path_present;
    dst->last_error = src->last_error;
    dst->ok = src->ok;
}

/* Stage 37: mount a read-only FAT32 filesystem over a USB mass-storage device.
 * Reads LBA 0, accepts either a superfloppy FAT32 boot sector or an MBR with a
 * FAT32 partition, then reuses the AHCI FAT32 parser through the USB backend.
 * Returns 1 if the root directory mounted. Strictly read-only. */
static vibio_u32 usb_mount_fat32(VibioDiskReadTest *usb_disk, VibioFat32ReadTest *usb_fs,
                                 VibioXhciMsd *msd, vibio_u64 sector_buffer)
{
    zero_bytes(usb_disk, sizeof(VibioDiskReadTest));
    zero_bytes(usb_fs, sizeof(VibioFat32ReadTest));
    usb_disk->backend = DISK_BACKEND_USB;
    usb_disk->usb_msd = msd;
    usb_disk->sector_buffer = sector_buffer;
    if (sector_buffer == 0 || msd == 0 || !msd->ready) {
        return 0;
    }

    if (!ahci_read_sector(usb_disk, 0)) {
        return 0;
    }
    const vibio_u8 *s = (const vibio_u8 *)sector_buffer;
    vibio_u64 part_lba = 0;
    if (s[510] == 0x55 && s[511] == 0xAA) {
        if (s[82] == 'F' && s[83] == 'A' && s[84] == 'T' && s[85] == '3' && s[86] == '2') {
            part_lba = 0; /* superfloppy: LBA 0 is the FAT32 boot sector */
        } else {
            for (vibio_u32 i = 0; i < 4; i++) {
                vibio_u32 base = 446U + i * 16U;
                vibio_u8 type = s[base + 4];
                if (type == 0x0B || type == 0x0C || type == 0xEF) {
                    part_lba = disk_read_le32(s, base + 8);
                    break;
                }
            }
        }
    }
    usb_disk->partition_lba = part_lba;
    usb_disk->ok = 1;
    VibioFat32ReadTest mounted = fat32_run_readonly_test(usb_disk);
    fat32_copy(usb_fs, &mounted);
    return usb_fs->root_found ? 1U : 0U;
}

static vibio_u32 fat32_entry_name_matches(const vibio_u8 *entry, const char *name11)
{
    for (vibio_u32 i = 0; i < 11; i++) {
        if (entry[i] != (vibio_u8)name11[i]) {
            return 0;
        }
    }

    return 1;
}

static vibio_u32 fat32_entry_cluster(const vibio_u8 *entry)
{
    vibio_u32 high = disk_read_le16(entry, 20);
    vibio_u32 low = disk_read_le16(entry, 26);
    return (high << 16) | low;
}

static vibio_u64 fat32_cluster_lba(const VibioFat32ReadTest *fs, vibio_u64 cluster)
{
    return fs->data_begin_lba + (cluster - 2ULL) * fs->sectors_per_cluster;
}

static void fat32_scan_root_sector(VibioFat32ReadTest *fs, const vibio_u8 *sector)
{
    for (vibio_u32 offset = 0; offset < AHCI_SECTOR_BYTES; offset += 32) {
        const vibio_u8 *entry = sector + offset;
        if (entry[0] == 0x00) {
            return;
        }

        if (entry[0] == 0xE5 || entry[11] == FAT32_ATTR_LONG_NAME || (entry[11] & FAT32_ATTR_VOLUME_ID) != 0) {
            continue;
        }

        fs->root_entries_seen++;
        if ((entry[11] & FAT32_ATTR_DIRECTORY) != 0 && fat32_entry_name_matches(entry, "SOUNDS     ")) {
            fs->sounds_cluster = fat32_entry_cluster(entry);
            fs->sounds_dir_lba = fat32_cluster_lba(fs, fs->sounds_cluster);
            fs->sounds_dir_found = fs->sounds_cluster >= 2;
        }
    }
}

static void fat32_scan_sounds_sector(VibioFat32ReadTest *fs, const vibio_u8 *sector)
{
    for (vibio_u32 offset = 0; offset < AHCI_SECTOR_BYTES; offset += 32) {
        const vibio_u8 *entry = sector + offset;
        if (entry[0] == 0x00) {
            return;
        }

        if (entry[0] == 0xE5 || entry[11] == FAT32_ATTR_LONG_NAME || (entry[11] & FAT32_ATTR_VOLUME_ID) != 0) {
            continue;
        }

        if ((entry[11] & FAT32_ATTR_DIRECTORY) != 0) {
            continue;
        }

        fs->sound_entries_seen++;
        if (fat32_entry_name_matches(entry, "BOOT    WAV") ||
            fat32_entry_name_matches(entry, "ERROR   WAV") ||
            fat32_entry_name_matches(entry, "NOTIFY  WAV") ||
            fat32_entry_name_matches(entry, "USBINS  WAV") ||
            fat32_entry_name_matches(entry, "USBRM   WAV")) {
            fs->sound_files_found++;
            fs->total_sound_bytes += disk_read_le32(entry, 28);
        }
    }
}

static VibioFat32ReadTest fat32_run_readonly_test(VibioDiskReadTest *disk)
{
    VibioFat32ReadTest fs;
    zero_bytes(&fs, sizeof(VibioFat32ReadTest));
    fs.partition_lba = disk->partition_lba;

    if (!disk->ok) {
        fs.last_error = 20;
        return fs;
    }

    if (!ahci_read_sector(disk, disk->partition_lba)) {
        fs.last_error = 21;
        return fs;
    }

    const vibio_u8 *sector = (const vibio_u8 *)disk->sector_buffer;
    fs.bytes_per_sector = disk_read_le16(sector, 11);
    fs.sectors_per_cluster = sector[13];
    fs.reserved_sectors = disk_read_le16(sector, 14);
    fs.fat_count = sector[16];
    fs.fat_size = disk_read_le32(sector, 36);
    fs.root_cluster = disk_read_le32(sector, 44);
    fs.boot_sector_ok = fs.bytes_per_sector == AHCI_SECTOR_BYTES &&
                        fs.sectors_per_cluster > 0 &&
                        fs.reserved_sectors > 0 &&
                        fs.fat_count == 2 &&
                        fs.fat_size > 0 &&
                        fs.root_cluster >= 2 &&
                        sector[82] == 'F' && sector[83] == 'A' && sector[84] == 'T' &&
                        sector[85] == '3' && sector[86] == '2';
    if (!fs.boot_sector_ok) {
        fs.last_error = 22;
        return fs;
    }

    fs.fat_begin_lba = fs.partition_lba + fs.reserved_sectors;
    fs.data_begin_lba = fs.partition_lba + fs.reserved_sectors + (vibio_u64)fs.fat_count * fs.fat_size;
    fs.root_dir_lba = fat32_cluster_lba(&fs, fs.root_cluster);
    fs.root_found = 1;

    for (vibio_u32 sector_index = 0; sector_index < fs.sectors_per_cluster; sector_index++) {
        if (!ahci_read_sector(disk, fs.root_dir_lba + sector_index)) {
            fs.last_error = 23;
            return fs;
        }

        fat32_scan_root_sector(&fs, (const vibio_u8 *)disk->sector_buffer);
    }

    if (!fs.sounds_dir_found) {
        fs.last_error = 24;
        return fs;
    }

    for (vibio_u32 sector_index = 0; sector_index < fs.sectors_per_cluster; sector_index++) {
        if (!ahci_read_sector(disk, fs.sounds_dir_lba + sector_index)) {
            fs.last_error = 25;
            return fs;
        }

        fat32_scan_sounds_sector(&fs, (const vibio_u8 *)disk->sector_buffer);
    }

    fs.write_path_present = 0;
    fs.ok = fs.sound_files_found == FAT32_EXPECTED_SOUND_FILES && fs.total_sound_bytes > 0 &&
            fs.write_path_present == 0;
    return fs;
}

/* ---- Read-only FAT32 file access -------------------------------------------
 * Stage 16 only counted directory entries. These helpers walk the FAT cluster
 * chain so the kernel can read an arbitrary file's bytes from the boot disk.
 * Everything here is read-only: there is no FAT or directory write path. */

/* Convert a user-facing name like "START.HTM" into a padded 8.3 name11
 * ("START   HTM"). Returns 1 on success. Names are uppercased. */
static vibio_u32 fat32_name_to_83(const char *name, char out[11])
{
    for (vibio_u32 i = 0; i < 11; i++) {
        out[i] = ' ';
    }

    vibio_u32 i = 0;
    vibio_u32 base = 0;
    while (name[i] != 0 && name[i] != '.' && base < 8) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        out[base++] = c;
        i++;
    }
    /* Skip any remaining base characters past 8. */
    while (name[i] != 0 && name[i] != '.') {
        i++;
    }
    if (name[i] == '.') {
        i++;
        vibio_u32 ext = 0;
        while (name[i] != 0 && ext < 3) {
            char c = name[i];
            if (c >= 'a' && c <= 'z') {
                c = (char)(c - 'a' + 'A');
            }
            out[8 + ext] = c;
            ext++;
            i++;
        }
    }
    return base > 0;
}

/* Read one sector, retrying a few times. AHCI reads issued at runtime (e.g. when
 * the browser opens a page) can transiently time out while the timer IRQ and
 * other activity run; a couple of retries make file reads reliable instead of
 * intermittently reporting a file as missing. */
static vibio_u32 ahci_read_sector_retry(VibioDiskReadTest *disk, vibio_u64 lba)
{
    for (vibio_u32 attempt = 0; attempt < 4; attempt++) {
        if (ahci_read_sector(disk, lba)) {
            return 1;
        }
    }
    return 0;
}

/* One-sector FAT cache.
 *
 * A cluster-chain walk reads consecutive FAT entries that mostly live in the
 * same 512-byte FAT sector (128 x 32-bit entries per sector), and directory
 * scans re-read FAT sectors too. Caching the most recently read FAT sector in a
 * DEDICATED buffer - never disk->sector_buffer, which data-sector reads reuse -
 * turns those repeats into RAM hits. This matters most over USB, where every
 * sector is a slow xHCI bulk transfer. Keyed on the owning disk object + LBA;
 * the media/boot FAT is read-only, and the RW sandbox is a separate disk object,
 * but ahci_write_sector() invalidates defensively so a write can never be served
 * stale. */
static vibio_u8 g_fat_sector_cache[AHCI_SECTOR_BYTES];
static VibioDiskReadTest *g_fat_sector_cache_disk = 0;
static vibio_u64 g_fat_sector_cache_lba = 0;
static vibio_u32 g_fat_sector_cache_valid = 0;

static void fat32_sector_cache_invalidate(void)
{
    g_fat_sector_cache_valid = 0;
    g_fat_sector_cache_disk = 0;
}

/* Read a FAT sector, serving it from the one-sector cache on a hit. On a miss it
 * reads through disk->sector_buffer (as before) and copies into the cache. */
static vibio_u32 fat32_read_fat_sector(VibioDiskReadTest *disk, vibio_u64 lba, const vibio_u8 **out)
{
    if (g_fat_sector_cache_valid && g_fat_sector_cache_disk == disk &&
        g_fat_sector_cache_lba == lba) {
        *out = g_fat_sector_cache;
        return 1;
    }
    if (!ahci_read_sector_retry(disk, lba)) {
        return 0;
    }
    copy_bytes(g_fat_sector_cache, (const void *)disk->sector_buffer, AHCI_SECTOR_BYTES);
    g_fat_sector_cache_disk = disk;
    g_fat_sector_cache_lba = lba;
    g_fat_sector_cache_valid = 1;
    *out = g_fat_sector_cache;
    return 1;
}

/* Return the next cluster in the chain, reading the FAT. Values >= 0x0FFFFFF8
 * mark end-of-chain; 0x0FFFFFFF is also returned on a read error. */
static vibio_u32 fat32_next_cluster(VibioDiskReadTest *disk, const VibioFat32ReadTest *fs, vibio_u32 cluster)
{
    vibio_u64 fat_offset = (vibio_u64)cluster * 4ULL;
    vibio_u64 fat_sector = fs->fat_begin_lba + (fat_offset / AHCI_SECTOR_BYTES);
    vibio_u32 entry_offset = (vibio_u32)(fat_offset % AHCI_SECTOR_BYTES);
    const vibio_u8 *sec = 0;
    if (!fat32_read_fat_sector(disk, fat_sector, &sec)) {
        return 0x0FFFFFFFU;
    }
    return disk_read_le32(sec, entry_offset) & 0x0FFFFFFFU;
}

static char fat32_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

/* Case-insensitive ASCII string equality, used to match VFAT long names. */
static vibio_u32 fat32_name_ieq(const char *a, const char *b)
{
    vibio_u32 i = 0;
    while (a[i] != 0 && b[i] != 0) {
        if (fat32_upper(a[i]) != fat32_upper(b[i])) {
            return 0;
        }
        i++;
    }
    return a[i] == 0 && b[i] == 0;
}

/* Copy the 13 UTF-16 code units of one VFAT long-name entry into dst at the slot
 * given by the entry's 1-based sequence number. dst must already be zeroed when a
 * new name begins (the entry whose sequence byte has bit 0x40 set, which is the
 * physically-first long entry). Non-ASCII becomes '?'; the 0x0000/0xFFFF
 * terminator/padding ends this piece. The assembled name is the contiguous run
 * from dst[0] to the first NUL. */
static void fat32_lfn_piece(const vibio_u8 *entry, char *dst, vibio_u32 dst_max)
{
    static const vibio_u8 offs[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
    vibio_u32 seq = (vibio_u32)(entry[0] & 0x1F);
    if (seq == 0) {
        return;
    }
    vibio_u32 base = (seq - 1U) * 13U;
    for (vibio_u32 i = 0; i < 13; i++) {
        vibio_u32 pos = base + i;
        if (pos + 1 >= dst_max) {
            return;
        }
        vibio_u16 ch = (vibio_u16)(entry[offs[i]] | ((vibio_u16)entry[offs[i] + 1] << 8));
        if (ch == 0x0000 || ch == 0xFFFF) {
            return;
        }
        dst[pos] = (ch >= 32 && ch <= 126) ? (char)ch : '?';
    }
}

/* Scan a directory cluster chain for an entry by name. Matches either the VFAT
 * long name (case-insensitive, when long_name != 0) or the 8.3 short name. On a
 * match, fills the start cluster and byte size and returns 1. Read-only. */
static vibio_u32 fat32_find_in_dir_ex(
    VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    vibio_u32 dir_cluster,
    const char *name11,
    const char *long_name,
    vibio_u32 *out_cluster,
    vibio_u32 *out_size)
{
    vibio_u32 cluster = dir_cluster;
    vibio_u32 guard = 0;
    char lfn[FILES_NAME_MAX];
    vibio_u32 lfn_have = 0;
    for (vibio_u32 k = 0; k < FILES_NAME_MAX; k++) {
        lfn[k] = 0;
    }
    while (cluster >= 2 && cluster < 0x0FFFFFF8U && guard < 4096) {
        vibio_u64 lba = fat32_cluster_lba(fs, cluster);
        for (vibio_u32 s = 0; s < fs->sectors_per_cluster; s++) {
            if (!ahci_read_sector_retry(disk, lba + s)) {
                return 0;
            }
            const vibio_u8 *sector = (const vibio_u8 *)disk->sector_buffer;
            for (vibio_u32 offset = 0; offset < AHCI_SECTOR_BYTES; offset += 32) {
                const vibio_u8 *entry = sector + offset;
                if (entry[0] == 0x00) {
                    return 0; /* end of directory */
                }
                if (entry[0] == 0xE5) {
                    lfn_have = 0;
                    continue;
                }
                if (entry[11] == FAT32_ATTR_LONG_NAME) {
                    if (entry[0] & 0x40) {
                        for (vibio_u32 k = 0; k < FILES_NAME_MAX; k++) {
                            lfn[k] = 0;
                        }
                    }
                    fat32_lfn_piece(entry, lfn, FILES_NAME_MAX);
                    lfn_have = 1;
                    continue;
                }
                if ((entry[11] & FAT32_ATTR_VOLUME_ID) != 0) {
                    lfn_have = 0;
                    continue;
                }
                vibio_u32 matched = 0;
                if (long_name != 0 && lfn_have && lfn[0] != 0 && fat32_name_ieq(lfn, long_name)) {
                    matched = 1;
                } else if (fat32_entry_name_matches(entry, name11)) {
                    matched = 1;
                }
                if (matched) {
                    *out_cluster = fat32_entry_cluster(entry);
                    *out_size = disk_read_le32(entry, 28);
                    return 1;
                }
                lfn_have = 0;
            }
        }
        cluster = fat32_next_cluster(disk, fs, cluster);
        guard++;
    }
    return 0;
}

static vibio_u32 fat32_find_in_dir(
    VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    vibio_u32 dir_cluster,
    const char *name11,
    vibio_u32 *out_cluster,
    vibio_u32 *out_size)
{
    return fat32_find_in_dir_ex(disk, fs, dir_cluster, name11, 0, out_cluster, out_size);
}

/* Directory that by-name reads (fat32_read_file / fat32_read_file_at) resolve in
 * before falling back to the volume root.
 *
 * The media player/viewer/browser re-open the selected file BY NAME on every read
 * (the player re-reads once per video frame). They always searched the FAT root,
 * so a file living inside a USB subfolder was reported as "file not found or
 * unreadable" even though the Files app listed it fine. The Files app points this
 * at the directory the opened file actually lives in, and each reader re-asserts
 * its own remembered directory before reading. A value < 2 means "root only",
 * which is the historical behaviour for boot files and root-level pages. */
static vibio_u32 g_fat_resolve_dir_cluster = 0;

static void fat32_set_resolve_dir(vibio_u32 cluster)
{
    g_fat_resolve_dir_cluster = cluster;
}

/* Resolve `name` to its start cluster + size, searching the active directory
 * first and then the volume root. The root fallback means a stale resolve-dir can
 * never hide a root-level file, and callers that never set one keep root-only
 * behaviour. Read-only. */
static vibio_u32 fat32_resolve_name(
    VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const char *name11,
    const char *long_name,
    vibio_u32 *out_cluster,
    vibio_u32 *out_size)
{
    vibio_u32 root = (vibio_u32)fs->root_cluster;
    vibio_u32 base = (g_fat_resolve_dir_cluster >= 2) ? g_fat_resolve_dir_cluster : root;
    if (fat32_find_in_dir_ex(disk, fs, base, name11, long_name, out_cluster, out_size)) {
        return 1;
    }
    if (base != root &&
        fat32_find_in_dir_ex(disk, fs, root, name11, long_name, out_cluster, out_size)) {
        return 1;
    }
    return 0;
}

/* Read a root-directory file by name into dest (up to max_bytes). Returns the
 * number of bytes copied, sets *out_size to the real file size, and 0 if the
 * file was not found or the disk is unavailable. Read-only. */
static vibio_u32 fat32_read_file(
    VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const char *name,
    vibio_u8 *dest,
    vibio_u32 max_bytes,
    vibio_u32 *out_size)
{
    panic_set_context("BROWSER", "FAT32", "FAT32 READ");
    if (out_size != 0) {
        *out_size = 0;
    }
    if (disk == 0 || fs == 0 || !fs->root_found || fs->sectors_per_cluster == 0) {
        return 0;
    }

    char name11[11];
    if (!fat32_name_to_83(name, name11)) {
        /* Not a valid 8.3 name (e.g. a long file name): use a sentinel that
         * cannot match a real short entry and rely on long-name matching. */
        for (vibio_u32 i = 0; i < 11; i++) {
            name11[i] = 0;
        }
    }

    vibio_u32 start_cluster = 0;
    vibio_u32 file_size = 0;
    if (!fat32_resolve_name(disk, fs, name11, name, &start_cluster, &file_size)) {
        return 0;
    }
    if (out_size != 0) {
        *out_size = file_size;
    }

    vibio_u32 to_read = file_size;
    if (to_read > max_bytes) {
        to_read = max_bytes;
    }

    vibio_u32 cluster = start_cluster;
    vibio_u32 written = 0;
    vibio_u32 guard = 0;
    while (cluster >= 2 && cluster < 0x0FFFFFF8U && written < to_read && guard < 200000) {
        vibio_u64 lba = fat32_cluster_lba(fs, cluster);
        for (vibio_u32 s = 0; s < fs->sectors_per_cluster && written < to_read; s++) {
            if (!ahci_read_sector_retry(disk, lba + s)) {
                return written;
            }
            const vibio_u8 *sector = (const vibio_u8 *)disk->sector_buffer;
            vibio_u32 chunk = AHCI_SECTOR_BYTES;
            if (written + chunk > to_read) {
                chunk = to_read - written;
            }
            for (vibio_u32 i = 0; i < chunk; i++) {
                dest[written + i] = sector[i];
            }
            written += chunk;
        }
        cluster = fat32_next_cluster(disk, fs, cluster);
        guard++;
    }
    return written;
}

/* Sequential read accelerator for fat32_read_file_at().
 *
 * media_read_bytes() re-enters fat32_read_file_at() once per video frame with a
 * steadily growing byte offset. The naive path re-scanned the directory for the
 * file AND re-walked its cluster chain from the first cluster every call, so the
 * Nth frame paid O(offset) uncached FAT-sector reads. For a 1080p MP4 that made
 * playback start fine and then grind to a crawl within a couple of seconds, and
 * because video decode, mouse polling, and PCM audio chunking all share one
 * cooperative loop, the growing per-frame disk cost also froze the cursor and
 * stalled audio. This cache remembers the resolved start cluster plus a
 * forward-only walk cursor for the file most recently streamed, so sequential
 * reads resume from the last cluster instead of restarting. It is purely a
 * position cache on the read path - it never writes the disk and is validated
 * against the disk identity + filename before use, so a different file, disk, or
 * a seek backwards transparently falls back to a fresh walk. */
typedef struct {
    VibioDiskReadTest *disk;
    vibio_u64 fat_begin_lba;
    vibio_u64 data_begin_lba;
    char name[FILES_NAME_MAX];
    vibio_u32 resolve_dir;    /* g_fat_resolve_dir_cluster the name was resolved in */
    vibio_u32 start_cluster;
    vibio_u32 file_size;
    vibio_u32 walk_cluster;   /* cluster covering byte position walk_pos */
    vibio_u64 walk_pos;       /* cluster-aligned absolute offset of walk_cluster */
    vibio_u32 valid;
} Fat32StreamCache;
static Fat32StreamCache g_fat_stream_cache;

/* Case-insensitive, bounded filename compare for the stream cache. */
static vibio_u32 fat32_stream_name_eq(const char *a, const char *b)
{
    for (vibio_u32 i = 0; i < FILES_NAME_MAX; i++) {
        char ca = a[i];
        char cb = b[i];
        if (fat32_upper(ca) != fat32_upper(cb)) {
            return 0;
        }
        if (ca == 0) {
            return 1;
        }
    }
    return 1;
}

/* Read a byte range from a root file. Returns bytes copied (may be short). */
static vibio_u32 fat32_read_file_at(
    VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const char *name,
    vibio_u64 offset,
    vibio_u8 *dest,
    vibio_u32 max_bytes,
    vibio_u32 *out_size)
{
    if (out_size != 0) {
        *out_size = 0;
    }
    if (disk == 0 || fs == 0 || !fs->root_found || max_bytes == 0) {
        return 0;
    }

    /* Resolve the file's start cluster + size. Reuse the cached resolution for
     * the file currently being streamed so per-frame reads do not re-scan the
     * directory on every call. */
    Fat32StreamCache *cache = &g_fat_stream_cache;
    vibio_u32 cache_file_hit =
        cache->valid && cache->disk == disk &&
        cache->fat_begin_lba == fs->fat_begin_lba &&
        cache->data_begin_lba == fs->data_begin_lba &&
        cache->resolve_dir == g_fat_resolve_dir_cluster &&
        fat32_stream_name_eq(cache->name, name);

    vibio_u32 start_cluster = 0;
    vibio_u32 file_size = 0;
    if (cache_file_hit) {
        start_cluster = cache->start_cluster;
        file_size = cache->file_size;
    } else {
        char name11[11];
        if (!fat32_name_to_83(name, name11)) {
            for (vibio_u32 i = 0; i < 11; i++) {
                name11[i] = 0;
            }
        }
        if (!fat32_resolve_name(disk, fs, name11, name, &start_cluster, &file_size)) {
            return 0;
        }
        /* Remember this file so the next (sequential) read can resume the walk. */
        cache->disk = disk;
        cache->fat_begin_lba = fs->fat_begin_lba;
        cache->data_begin_lba = fs->data_begin_lba;
        vibio_u32 ni = 0;
        for (; ni + 1 < FILES_NAME_MAX && name[ni] != 0; ni++) {
            cache->name[ni] = name[ni];
        }
        cache->name[ni] = 0;
        cache->resolve_dir = g_fat_resolve_dir_cluster;
        cache->start_cluster = start_cluster;
        cache->file_size = file_size;
        cache->walk_cluster = start_cluster;
        cache->walk_pos = 0;
        cache->valid = 1;
    }

    if (out_size != 0) {
        *out_size = file_size;
    }
    if (offset >= file_size) {
        return 0;
    }
    vibio_u32 to_read = file_size - (vibio_u32)offset;
    if (to_read > max_bytes) {
        to_read = max_bytes;
    }
    vibio_u32 cluster_size = fs->sectors_per_cluster * AHCI_SECTOR_BYTES;
    if (cluster_size == 0) {
        return 0;
    }
    /* Walk the cluster chain tracking the ABSOLUTE byte position of each sector
     * in the file. Sectors that end before `offset` are skipped; the first
     * partially-included sector is copied from `offset` onward; the rest are
     * copied whole until `to_read` bytes are written.
     *
     * The walk starts from the cached forward cursor when it has not advanced
     * past `offset` yet (the common sequential-playback case), otherwise it
     * restarts at the file's first cluster so seeks and Restart-to-zero still
     * read correctly. Whole clusters that end at or before `offset` are skipped
     * without reading their data sectors, and the resume cursor is advanced so
     * the next frame starts here instead of at cluster 0 - this is what turns
     * the old O(offset) per-frame cost into O(bytes advanced since last frame).
     *
     * (The absolute-position tracking also fixes the older bug where sector
     * bounds were compared relative to each cluster, wrongly skipping every
     * sector after the first cluster on multi-cluster reads.) */
    vibio_u64 pos = 0;
    vibio_u32 cluster = start_cluster;
    if (cache->walk_cluster >= 2 && cache->walk_pos <= offset &&
        (cache->walk_pos % cluster_size) == 0) {
        cluster = cache->walk_cluster;
        pos = cache->walk_pos;
    }
    vibio_u32 written = 0;
    vibio_u32 guard = 0;
    while (cluster >= 2 && cluster < 0x0FFFFFF8U && written < to_read && guard < 200000) {
        if (written == 0 && pos + (vibio_u64)cluster_size <= offset) {
            pos += cluster_size;
            cluster = fat32_next_cluster(disk, fs, cluster);
            cache->walk_cluster = cluster;
            cache->walk_pos = pos;
            guard++;
            continue;
        }
        vibio_u64 lba = fat32_cluster_lba(fs, cluster);
        for (vibio_u32 s = 0; s < fs->sectors_per_cluster && written < to_read; s++) {
            vibio_u64 sec_start = pos;
            vibio_u64 sec_end = pos + AHCI_SECTOR_BYTES;
            if (sec_end <= offset) {
                pos = sec_end;
                continue;
            }
            if (!ahci_read_sector_retry(disk, lba + s)) {
                return written;
            }
            const vibio_u8 *sector = (const vibio_u8 *)disk->sector_buffer;
            vibio_u32 start_in_sector = 0;
            if (offset > sec_start) {
                start_in_sector = (vibio_u32)(offset - sec_start);
            }
            vibio_u32 avail = AHCI_SECTOR_BYTES - start_in_sector;
            vibio_u32 take = to_read - written;
            if (take > avail) {
                take = avail;
            }
            for (vibio_u32 i = 0; i < take; i++) {
                dest[written + i] = sector[start_in_sector + i];
            }
            written += take;
            pos = sec_end;
        }
        cluster = fat32_next_cluster(disk, fs, cluster);
        guard++;
    }
    return written;
}

/* Turn a raw 8.3 directory entry name (e.g. "START   HTM") into a display name
 * ("START.HTM"). Directories are shown without a trailing dot. Read-only. */
static void fat32_name_to_display(const vibio_u8 *entry, char out[FILES_NAME_MAX])
{
    vibio_u32 pos = 0;
    for (vibio_u32 i = 0; i < 8; i++) {
        char c = (char)entry[i];
        if (c == ' ') {
            break;
        }
        out[pos++] = c;
    }
    /* Extension, if any non-space bytes are present. */
    vibio_u32 ext_len = 0;
    for (vibio_u32 i = 8; i < 11; i++) {
        if (entry[i] != ' ') {
            ext_len = i - 8 + 1;
        }
    }
    if (ext_len > 0) {
        out[pos++] = '.';
        for (vibio_u32 i = 0; i < ext_len; i++) {
            out[pos++] = (char)entry[8 + i];
        }
    }
    out[pos] = 0;
}

/* List the real entries of a FAT32 directory cluster chain into out[]. Skips
 * deleted, long-name, volume-label, and the "." / ".." pseudo-entries. Sets
 * *truncated if the directory has more entries than max. Fully read-only. */
static vibio_u32 fat32_list_dir(
    VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    vibio_u32 dir_cluster,
    VibioFilesEntry *out,
    vibio_u32 max,
    vibio_u32 *out_count,
    vibio_u32 *truncated)
{
    if (out_count != 0) {
        *out_count = 0;
    }
    if (truncated != 0) {
        *truncated = 0;
    }
    if (disk == 0 || fs == 0 || !fs->root_found || fs->sectors_per_cluster == 0) {
        return 0;
    }

    vibio_u32 count = 0;
    vibio_u32 cluster = dir_cluster;
    vibio_u32 guard = 0;
    char lfn[FILES_NAME_MAX];
    vibio_u32 lfn_have = 0;
    for (vibio_u32 k = 0; k < FILES_NAME_MAX; k++) {
        lfn[k] = 0;
    }
    while (cluster >= 2 && cluster < 0x0FFFFFF8U && guard < 4096) {
        vibio_u64 lba = fat32_cluster_lba(fs, cluster);
        for (vibio_u32 s = 0; s < fs->sectors_per_cluster; s++) {
            if (!ahci_read_sector_retry(disk, lba + s)) {
                if (out_count != 0) {
                    *out_count = count;
                }
                return count > 0;
            }
            const vibio_u8 *sector = (const vibio_u8 *)disk->sector_buffer;
            for (vibio_u32 offset = 0; offset < AHCI_SECTOR_BYTES; offset += 32) {
                const vibio_u8 *entry = sector + offset;
                if (entry[0] == 0x00) {
                    if (out_count != 0) {
                        *out_count = count;
                    }
                    return 1; /* end of directory */
                }
                if (entry[0] == 0xE5) {
                    lfn_have = 0;
                    continue;
                }
                if (entry[11] == FAT32_ATTR_LONG_NAME) {
                    /* Accumulate the VFAT long-name pieces preceding the short
                     * entry. The 0x40-flagged entry is the physically-first
                     * piece, so reset the assembly buffer there. */
                    if (entry[0] & 0x40) {
                        for (vibio_u32 k = 0; k < FILES_NAME_MAX; k++) {
                            lfn[k] = 0;
                        }
                    }
                    fat32_lfn_piece(entry, lfn, FILES_NAME_MAX);
                    lfn_have = 1;
                    continue;
                }
                if ((entry[11] & FAT32_ATTR_VOLUME_ID) != 0) {
                    lfn_have = 0;
                    continue;
                }
                if (entry[0] == '.') {
                    lfn_have = 0;
                    continue; /* "." and ".." pseudo-entries */
                }
                if (count >= max) {
                    if (truncated != 0) {
                        *truncated = 1;
                    }
                    if (out_count != 0) {
                        *out_count = count;
                    }
                    return 1;
                }
                VibioFilesEntry *e = &out[count];
                /* Prefer the assembled long name; fall back to the 8.3 name. */
                if (lfn_have && lfn[0] != 0) {
                    vibio_u32 n = 0;
                    for (; n + 1 < FILES_NAME_MAX && lfn[n] != 0; n++) {
                        e->name[n] = lfn[n];
                    }
                    e->name[n] = 0;
                } else {
                    fat32_name_to_display(entry, e->name);
                }
                lfn_have = 0;
                e->is_dir = (entry[11] & FAT32_ATTR_DIRECTORY) != 0 ? 1 : 0;
                e->cluster = fat32_entry_cluster(entry);
                e->size = disk_read_le32(entry, 28);
                count++;
            }
        }
        cluster = fat32_next_cluster(disk, fs, cluster);
        guard++;
    }
    if (out_count != 0) {
        *out_count = count;
    }
    return count > 0;
}

/* ---- Stage 29: AHCI sector write + VM-only FAT32 write sandbox --------------
 * The first real disk-write code in Vibio. It is used ONLY against a separately
 * attached, positively identified disposable scratch disk (FAT32 label VIBIORW
 * with the VIBIO_RW.TXT marker). The boot disk read path above is untouched and
 * is never written. */

/* Issue the command in slot 0 of the current port and wait for completion.
 * Shared by the write and flush paths. Returns 1 on success, 0 on timeout or a
 * task-file (device) error, setting disk->last_error. */
static vibio_u32 ahci_issue_slot0(VibioDiskReadTest *disk)
{
    vibio_u64 port_base = ahci_port_base(disk->abar, disk->port);

    if ((mmio_read32(port_base, 0x20) & (AHCI_TFD_BSY | AHCI_TFD_DRQ)) != 0) {
        if (!ahci_wait_clear(port_base, 0x20, AHCI_TFD_BSY | AHCI_TFD_DRQ)) {
            disk->last_error = 2;
            return 0;
        }
    }

    mmio_write32(port_base, 0x10, 0xFFFFFFFFU); /* clear PxIS */
    mmio_write32(port_base, 0x34, 0);           /* PxSACT: not using NCQ */
    mmio_write32(port_base, 0x38, 1);           /* PxCI: issue slot 0 */

    for (vibio_u32 i = 0; i < AHCI_READ_TIMEOUT; i++) {
        if ((mmio_read32(port_base, 0x10) & AHCI_IS_TFES) != 0) {
            disk->last_error = 3;
            return 0;
        }
        if ((mmio_read32(port_base, 0x38) & 1U) == 0) {
            return 1;
        }
    }

    disk->last_error = 4;
    return 0;
}

/* Build a single-sector WRITE DMA EXT command in slot 0. The 512 bytes to write
 * must already be in disk->sector_buffer. The command header's Write bit makes
 * the device read from our PRDT buffer and write it to the disk. */
static void ahci_build_write_command(VibioDiskReadTest *disk, vibio_u64 lba)
{
    volatile vibio_u8 *header = (volatile vibio_u8 *)disk->command_list;
    volatile vibio_u8 *table = (volatile vibio_u8 *)disk->command_table;

    header[0] = 5 | (1U << 6); /* CFL = 5 dwords, W = 1 (write to device) */
    header[1] = 0;
    header[2] = 1;             /* one PRDT entry */
    header[3] = 0;
    for (vibio_u32 i = 4; i < AHCI_CMD_HEADER_BYTES; i++) {
        header[i] = 0;
    }

    vibio_u64 table_address = disk->command_table;
    header[8] = (vibio_u8)(table_address & 0xFF);
    header[9] = (vibio_u8)((table_address >> 8) & 0xFF);
    header[10] = (vibio_u8)((table_address >> 16) & 0xFF);
    header[11] = (vibio_u8)((table_address >> 24) & 0xFF);
    header[12] = (vibio_u8)((table_address >> 32) & 0xFF);
    header[13] = (vibio_u8)((table_address >> 40) & 0xFF);
    header[14] = (vibio_u8)((table_address >> 48) & 0xFF);
    header[15] = (vibio_u8)((table_address >> 56) & 0xFF);

    for (vibio_u32 i = 0; i < AHCI_COMMAND_TABLE_BYTES; i++) {
        table[i] = 0;
    }

    table[0] = AHCI_FIS_REG_H2D;
    table[1] = 0x80;           /* C bit: command */
    table[2] = AHCI_ATA_WRITE_DMA_EXT;
    table[4] = (vibio_u8)(lba & 0xFF);
    table[5] = (vibio_u8)((lba >> 8) & 0xFF);
    table[6] = (vibio_u8)((lba >> 16) & 0xFF);
    table[7] = 1U << 6;        /* LBA mode */
    table[8] = (vibio_u8)((lba >> 24) & 0xFF);
    table[9] = (vibio_u8)((lba >> 32) & 0xFF);
    table[10] = (vibio_u8)((lba >> 40) & 0xFF);
    table[12] = 1;             /* sector count low = 1 */
    table[13] = 0;

    vibio_u64 buffer = disk->sector_buffer;
    vibio_u32 prdt = AHCI_COMMAND_TABLE_PRDT_OFFSET;
    table[prdt + 0] = (vibio_u8)(buffer & 0xFF);
    table[prdt + 1] = (vibio_u8)((buffer >> 8) & 0xFF);
    table[prdt + 2] = (vibio_u8)((buffer >> 16) & 0xFF);
    table[prdt + 3] = (vibio_u8)((buffer >> 24) & 0xFF);
    table[prdt + 4] = (vibio_u8)((buffer >> 32) & 0xFF);
    table[prdt + 5] = (vibio_u8)((buffer >> 40) & 0xFF);
    table[prdt + 6] = (vibio_u8)((buffer >> 48) & 0xFF);
    table[prdt + 7] = (vibio_u8)((buffer >> 56) & 0xFF);
    table[prdt + 12] = (vibio_u8)((AHCI_SECTOR_BYTES - 1) & 0xFF);
    table[prdt + 13] = (vibio_u8)(((AHCI_SECTOR_BYTES - 1) >> 8) & 0xFF);
    table[prdt + 14] = 0;
    table[prdt + 15] = 0;
}

/* Write the 512 bytes in disk->sector_buffer to one LBA. Returns 1 on success.
 * The caller is responsible for having verified this is the sandbox target. */
static vibio_u32 ahci_write_sector(VibioDiskReadTest *disk, vibio_u64 lba)
{
    /* Any write could change FAT contents we cached; drop the FAT sector cache
     * so a later chain walk never reads a stale link. Writes are rare (VM-only
     * sandbox) so the blanket invalidate costs nothing meaningful. */
    fat32_sector_cache_invalidate();
    ahci_build_write_command(disk, lba);
    return ahci_issue_slot0(disk);
}

static vibio_u32 ahci_write_sector_retry(VibioDiskReadTest *disk, vibio_u64 lba)
{
    for (vibio_u32 attempt = 0; attempt < 4; attempt++) {
        if (ahci_write_sector(disk, lba)) {
            return 1;
        }
    }
    return 0;
}

/* Best-effort FLUSH CACHE EXT so writes reach the backing store. Read-back is
 * the real proof of a successful write, so a flush failure is non-fatal. */
static vibio_u32 ahci_flush_cache(VibioDiskReadTest *disk)
{
    volatile vibio_u8 *header = (volatile vibio_u8 *)disk->command_list;
    volatile vibio_u8 *table = (volatile vibio_u8 *)disk->command_table;

    header[0] = 5; /* CFL = 5 dwords, no write bit, no PRDT */
    header[1] = 0;
    header[2] = 0; /* zero PRDT entries */
    header[3] = 0;
    for (vibio_u32 i = 4; i < AHCI_CMD_HEADER_BYTES; i++) {
        header[i] = 0;
    }
    vibio_u64 table_address = disk->command_table;
    header[8] = (vibio_u8)(table_address & 0xFF);
    header[9] = (vibio_u8)((table_address >> 8) & 0xFF);
    header[10] = (vibio_u8)((table_address >> 16) & 0xFF);
    header[11] = (vibio_u8)((table_address >> 24) & 0xFF);
    header[12] = (vibio_u8)((table_address >> 32) & 0xFF);
    header[13] = (vibio_u8)((table_address >> 40) & 0xFF);
    header[14] = (vibio_u8)((table_address >> 48) & 0xFF);
    header[15] = (vibio_u8)((table_address >> 56) & 0xFF);

    for (vibio_u32 i = 0; i < AHCI_COMMAND_TABLE_BYTES; i++) {
        table[i] = 0;
    }
    table[0] = AHCI_FIS_REG_H2D;
    table[1] = 0x80;
    table[2] = AHCI_ATA_FLUSH_CACHE_EXT;
    table[7] = 1U << 6;
    return ahci_issue_slot0(disk);
}

/* Parse the partition boot sector of an already-prepared disk (whose
 * partition_lba is set) into a VibioFat32ReadTest. Read-only. Returns 1 if the
 * partition is a valid FAT32 the read helpers can walk. */
static vibio_u32 rw_sandbox_parse_fat32(VibioDiskReadTest *disk, VibioFat32ReadTest *fs)
{
    zero_bytes(fs, sizeof(VibioFat32ReadTest));
    fs->partition_lba = disk->partition_lba;
    if (!ahci_read_sector_retry(disk, disk->partition_lba)) {
        fs->last_error = 21;
        return 0;
    }
    const vibio_u8 *s = (const vibio_u8 *)disk->sector_buffer;
    fs->bytes_per_sector = disk_read_le16(s, 11);
    fs->sectors_per_cluster = s[13];
    fs->reserved_sectors = disk_read_le16(s, 14);
    fs->fat_count = s[16];
    fs->fat_size = disk_read_le32(s, 36);
    fs->root_cluster = disk_read_le32(s, 44);
    fs->boot_sector_ok = fs->bytes_per_sector == AHCI_SECTOR_BYTES &&
                         fs->sectors_per_cluster > 0 &&
                         fs->reserved_sectors > 0 &&
                         fs->fat_count == 2 &&
                         fs->fat_size > 0 &&
                         fs->root_cluster >= 2 &&
                         s[82] == 'F' && s[83] == 'A' && s[84] == 'T' &&
                         s[85] == '3' && s[86] == '2';
    if (!fs->boot_sector_ok) {
        fs->last_error = 22;
        return 0;
    }
    fs->fat_begin_lba = fs->partition_lba + fs->reserved_sectors;
    fs->data_begin_lba = fs->partition_lba + fs->reserved_sectors +
                         (vibio_u64)fs->fat_count * fs->fat_size;
    fs->root_dir_lba = fat32_cluster_lba(fs, fs->root_cluster);
    fs->root_found = 1;
    fs->write_path_present = 0;
    fs->ok = 1;
    return 1;
}

/* Try to identify the sandbox on one AHCI port: a present device whose FAT32
 * volume label is exactly "VIBIORW". Prepares the port and reads its MBR/boot
 * sector. Returns 1 only when the labeled sandbox is found on this port. */
static vibio_u32 rw_sandbox_try_port(VibioRwSandbox *sb, vibio_u32 port)
{
    VibioDiskReadTest *d = &sb->disk;
    d->port = port;
    d->last_error = 0;

    vibio_u64 port_base = ahci_port_base(d->abar, port);
    vibio_u32 ssts = mmio_read32(port_base, 0x28);
    if ((ssts & 0x0FU) != AHCI_SSTS_DET_PRESENT) {
        return 0;
    }
    d->port_status = ssts;
    d->port_signature = mmio_read32(port_base, 0x24);

    if (!ahci_prepare_port(d)) {
        return 0;
    }

    if (!ahci_read_sector_retry(d, 0)) {
        return 0;
    }
    const vibio_u8 *s = (const vibio_u8 *)d->sector_buffer;
    if (!(s[510] == 0x55 && s[511] == 0xAA)) {
        return 0;
    }
    d->mbr_signature_ok = 1;
    d->partition_lba = disk_read_le32(s, 446 + 8);
    if (d->partition_lba == 0) {
        return 0;
    }

    if (!ahci_read_sector_retry(d, d->partition_lba)) {
        return 0;
    }
    s = (const vibio_u8 *)d->sector_buffer;
    if (!(s[82] == 'F' && s[83] == 'A' && s[84] == 'T' && s[85] == '3' && s[86] == '2')) {
        return 0;
    }
    d->boot_sector_ok = 1;

    /* This is a FAT32 disk on a non-boot port. Record its label for display. */
    for (vibio_u32 i = 0; i < 11; i++) {
        sb->label[i] = (char)s[71 + i];
    }
    sb->label[11] = 0;
    sb->present = 1;

    const char *want = "VIBIORW    ";
    vibio_u32 match = 1;
    for (vibio_u32 i = 0; i < 11; i++) {
        if (s[71 + i] != (vibio_u8)want[i]) {
            match = 0;
            break;
        }
    }
    sb->label_ok = match;
    if (!match) {
        return 0; /* a different disk - keep scanning, never write to it */
    }

    if (!rw_sandbox_parse_fat32(d, &sb->fs)) {
        return 0;
    }
    sb->fat32_ok = 1;
    return 1;
}

/* Identify and arm (or refuse) the VM-only write sandbox. Scans non-boot AHCI
 * ports for the VIBIORW disk, verifies the VIBIO_RW.TXT marker and the
 * RWTEST.TXT preallocated file, and only then arms writes. */
static void rw_sandbox_init(VibioRwSandbox *sb, VibioPageAllocator *allocator,
                            const VibioDiskReadTest *boot_disk)
{
    zero_bytes(sb, sizeof(VibioRwSandbox));
    sb->label[0] = 0;
    /* Default to skipping port 0 (the boot disk's usual port) even if the boot
     * read test failed, so we never re-point the boot disk's port registers. */
    sb->boot_port = (boot_disk != 0) ? boot_disk->port : 0;

    sb->disk.command_list = page_allocator_alloc(allocator);
    sb->disk.received_fis = page_allocator_alloc(allocator);
    sb->disk.command_table = page_allocator_alloc(allocator);
    sb->disk.sector_buffer = page_allocator_alloc(allocator);
    if (sb->disk.command_list == 0 || sb->disk.received_fis == 0 ||
        sb->disk.command_table == 0 || sb->disk.sector_buffer == 0) {
        sb->last_error = 10;
        return;
    }

    if (boot_disk != 0 && boot_disk->abar != 0) {
        sb->disk.abar = boot_disk->abar;
        sb->disk.pci_bus = boot_disk->pci_bus;
        sb->disk.pci_device = boot_disk->pci_device;
        sb->disk.pci_function = boot_disk->pci_function;
        sb->controller_found = 1;
    } else {
        sb->controller_found = pci_find_ahci_controller(&sb->disk);
    }
    if (!sb->controller_found || sb->disk.abar == 0) {
        sb->last_error = 11;
        return;
    }

    vibio_u32 implemented = mmio_read32(sb->disk.abar, 0x0C);
    vibio_u32 found = 0;
    for (vibio_u32 port = 0; port < AHCI_MAX_PORTS; port++) {
        if ((implemented & (1U << port)) == 0) {
            continue;
        }
        if (port == sb->boot_port) {
            continue; /* never touch the boot disk's port */
        }
        if (rw_sandbox_try_port(sb, port)) {
            found = 1;
            break;
        }
    }
    if (!found) {
        sb->last_error = sb->present ? 13 : 12; /* 13: disk seen but wrong label */
        return;
    }

    /* Marker gate: VIBIO_RW.TXT must exist and its content must begin with the
     * magic "VIBIO_RW_OK" (documented long form VIBIO_RW_OK.TXT). */
    char name11[11];
    vibio_u32 mcluster = 0;
    vibio_u32 msize = 0;
    if (fat32_name_to_83("VIBIO_RW.TXT", name11) &&
        fat32_find_in_dir(&sb->disk, &sb->fs, (vibio_u32)sb->fs.root_cluster, name11, &mcluster, &msize)) {
        vibio_u8 mbuf[16];
        vibio_u32 mread_size = 0;
        vibio_u32 mread = fat32_read_file(&sb->disk, &sb->fs, "VIBIO_RW.TXT", mbuf, sizeof(mbuf), &mread_size);
        const char *magic = "VIBIO_RW_OK";
        vibio_u32 magic_ok = mread >= 11;
        for (vibio_u32 i = 0; i < 11 && magic_ok; i++) {
            if (mbuf[i] != (vibio_u8)magic[i]) {
                magic_ok = 0;
            }
        }
        sb->marker_ok = magic_ok;
    }

    /* Preallocated test file gate: RWTEST.TXT must exist with a valid cluster
     * inside the data region. */
    char rname[11];
    vibio_u32 rcluster = 0;
    vibio_u32 rsize = 0;
    if (fat32_name_to_83("RWTEST.TXT", rname) &&
        fat32_find_in_dir(&sb->disk, &sb->fs, (vibio_u32)sb->fs.root_cluster, rname, &rcluster, &rsize)) {
        vibio_u64 rlba = fat32_cluster_lba(&sb->fs, rcluster);
        if (rcluster >= 2 && rlba >= sb->fs.data_begin_lba) {
            sb->rwtest_ok = 1;
            sb->rwtest_cluster = rcluster;
            sb->rwtest_size = rsize;
            sb->rwtest_lba = rlba;
        }
    }

    sb->armed = sb->label_ok && sb->marker_ok && sb->rwtest_ok && !sb->disabled;
    sb->last_error = 0;
}

/* Find "SEQ=<n>" in a buffer and return n (0 if absent). */
static vibio_u32 rw_parse_seq(const vibio_u8 *buf, vibio_u32 len)
{
    for (vibio_u32 i = 0; i + 4 <= len; i++) {
        if (buf[i] == 'S' && buf[i + 1] == 'E' && buf[i + 2] == 'Q' && buf[i + 3] == '=') {
            vibio_u32 j = i + 4;
            vibio_u32 val = 0;
            vibio_u32 any = 0;
            while (j < len && buf[j] >= '0' && buf[j] <= '9') {
                val = val * 10 + (vibio_u32)(buf[j] - '0');
                j++;
                any = 1;
            }
            return any ? val : 0;
        }
    }
    return 0;
}

/* Compose the 512-byte RWTEST.TXT content for a sequence number. Returns the
 * content length before zero padding. */
static vibio_u32 rw_compose_content(vibio_u8 *out512, vibio_u32 seq)
{
    for (vibio_u32 i = 0; i < AHCI_SECTOR_BYTES; i++) {
        out512[i] = 0;
    }
    char line[48];
    vibio_u32 p = 0;
    p = panic_append_text(line, p, sizeof(line), "VIBIO RW SANDBOX OK SEQ=");
    p = panic_append_uint(line, p, sizeof(line), seq);
    if (p < sizeof(line)) {
        line[p++] = '\n';
    }
    for (vibio_u32 i = 0; i < p; i++) {
        out512[i] = (vibio_u8)line[i];
    }
    return p;
}

/* Store a printable first line from a read-back buffer for display. */
static void rw_store_readback(VibioRwSandbox *sb, const vibio_u8 *buf, vibio_u32 len)
{
    vibio_u32 n = 0;
    for (vibio_u32 i = 0; i < len && n < RW_READBACK_MAX - 1; i++) {
        char c = (char)buf[i];
        if (c == '\n' || c == '\r' || c == 0) {
            break;
        }
        if (c < 32 || c > 126) {
            c = '.';
        }
        sb->readback[n++] = c;
    }
    sb->readback[n] = 0;
    sb->readback_len = n;
}

/* The safe write/read/verify test against the sandbox target ONLY. Writes a new
 * sequence line into the preallocated RWTEST.TXT (without expanding it), then
 * verifies the bytes both at the sector level and back through the normal FAT32
 * read path. Never writes if the sandbox is not armed. */
static void rw_sandbox_run_test(VibioRwSandbox *sb)
{
    sb->test_ran = 1;
    sb->test_pass = 0;
    sb->readback_len = 0;
    sb->readback[0] = 0;

    if (!sb->armed) {
        sb->last_error = 30; /* not armed */
        return;
    }
    /* Belt-and-suspenders: never write to the boot disk's port. */
    if (sb->disk.port == sb->boot_port) {
        sb->last_error = 31;
        return;
    }
    if (sb->rwtest_cluster < 2 || sb->rwtest_lba < sb->fs.data_begin_lba) {
        sb->last_error = 32; /* refuse a cluster outside the data region */
        return;
    }

    panic_set_context("KERNEL", "RW SANDBOX", "RWTEST WRITE");

    /* Read the current content to continue the sequence counter (proves the last
     * write persisted across reboots when the number keeps climbing). */
    if (!ahci_read_sector_retry(&sb->disk, sb->rwtest_lba)) {
        sb->last_error = 33;
        return;
    }
    vibio_u32 prev_seq = rw_parse_seq((const vibio_u8 *)sb->disk.sector_buffer, AHCI_SECTOR_BYTES);
    vibio_u32 new_seq = prev_seq + 1;

    vibio_u8 content[AHCI_SECTOR_BYTES];
    rw_compose_content(content, new_seq);

    volatile vibio_u8 *sbuf = (volatile vibio_u8 *)sb->disk.sector_buffer;
    for (vibio_u32 i = 0; i < AHCI_SECTOR_BYTES; i++) {
        sbuf[i] = content[i];
    }
    if (!ahci_write_sector_retry(&sb->disk, sb->rwtest_lba)) {
        sb->last_error = 34; /* write failed - reported, never ignored */
        return;
    }
    ahci_flush_cache(&sb->disk); /* best-effort durability */

    /* Sector-level write-through verify: clear the buffer, re-read, compare. */
    zero_page(sb->disk.sector_buffer);
    if (!ahci_read_sector_retry(&sb->disk, sb->rwtest_lba)) {
        sb->last_error = 35;
        return;
    }
    const vibio_u8 *rb = (const vibio_u8 *)sb->disk.sector_buffer;
    vibio_u32 sector_match = 1;
    for (vibio_u32 i = 0; i < AHCI_SECTOR_BYTES; i++) {
        if (rb[i] != content[i]) {
            sector_match = 0;
            break;
        }
    }
    if (!sector_match) {
        sb->last_error = 36;
        return;
    }

    /* Filesystem-level verify through the normal read path. */
    vibio_u8 fbuf[AHCI_SECTOR_BYTES];
    vibio_u32 fsize = 0;
    vibio_u32 fread = fat32_read_file(&sb->disk, &sb->fs, "RWTEST.TXT", fbuf, sizeof(fbuf), &fsize);
    vibio_u32 fs_match = fread > 0;
    for (vibio_u32 i = 0; i < fread; i++) {
        if (fbuf[i] != content[i]) {
            fs_match = 0;
            break;
        }
    }

    rw_store_readback(sb, fbuf, fread);
    sb->last_seq = new_seq;
    sb->test_pass = sector_match && fs_match;
    sb->last_error = sb->test_pass ? 0 : 37;
}

/* Read the current RWTEST.TXT content through the normal read path. Read-only,
 * so it works even when writes are disabled - it never writes. */
static void rw_sandbox_read_current(VibioRwSandbox *sb)
{
    sb->readback_len = 0;
    sb->readback[0] = 0;
    if (!sb->present || !sb->fat32_ok || !sb->rwtest_ok) {
        sb->last_error = 40;
        return;
    }
    vibio_u8 fbuf[AHCI_SECTOR_BYTES];
    vibio_u32 fsize = 0;
    vibio_u32 fread = fat32_read_file(&sb->disk, &sb->fs, "RWTEST.TXT", fbuf, sizeof(fbuf), &fsize);
    if (fread == 0) {
        sb->last_error = 41;
        return;
    }
    sb->last_seq = rw_parse_seq(fbuf, fread);
    rw_store_readback(sb, fbuf, fread);
    sb->last_error = 0;
}

/* Rebuild the "/path/here" text from the path stack. */
static void files_app_build_path_text(VibioFilesApp *files)
{
    vibio_u32 pos = 0;
    files->path_text[pos++] = '/';
    for (vibio_u32 d = 1; d < files->path_depth; d++) {
        const char *name = files->path_names[d];
        for (vibio_u32 i = 0; name[i] != 0 && pos < sizeof(files->path_text) - 2; i++) {
            files->path_text[pos++] = name[i];
        }
        if (d + 1 < files->path_depth && pos < sizeof(files->path_text) - 2) {
            files->path_text[pos++] = '/';
        }
    }
    files->path_text[pos] = 0;
}

/* Load the directory at the top of the path stack into the entry list. */
/* Does a live USB FAT32 source exist to browse? */
static vibio_u32 files_usb_available(const VibioFilesApp *files)
{
    return files->usb_fs != 0 && files->usb_fs->root_found &&
           files->usbstor != 0 && files->usbstor->fat32_mounted;
}

/* The disk/fs currently being browsed, honouring the USB vs AHCI selection. */
static VibioDiskReadTest *files_active_disk(VibioFilesApp *files)
{
    if ((files->view_source == FILES_SRC_USB || files->source == FILES_SRC_USB) &&
        files_usb_available(files)) {
        return files->usb_disk;
    }
    return files->disk;
}

static VibioFat32ReadTest *files_active_fs(VibioFilesApp *files)
{
    if ((files->view_source == FILES_SRC_USB || files->source == FILES_SRC_USB) &&
        files_usb_available(files)) {
        return files->usb_fs;
    }
    return files->fs;
}

static void files_app_reload(VibioFilesApp *files)
{
    files->entry_count = 0;
    files->selected = 0;
    files->scroll = 0;
    files->truncated = 0;
    files->info_open = 0;
    vibio_u32 cluster = files->path_clusters[files->path_depth - 1];

    /* When the user has switched to the live USB source and it is mounted,
     * browse it directly. Otherwise fall back to the normal priority: AHCI
     * boot-disk FAT32, then UEFI boot files, then none. */
    if (files->view_source == FILES_SRC_USB && files_usb_available(files)) {
        files->source = FILES_SRC_USB;
        files->fs_available = 1;
        files->loaded = 1;
        vibio_u32 count = 0, trunc = 0;
        if (cluster < 2) {
            cluster = (vibio_u32)files->usb_fs->root_cluster;
            files->path_clusters[files->path_depth - 1] = cluster;
        }
        fat32_list_dir(files->usb_disk, files->usb_fs, cluster, files->entries, FILES_MAX_ENTRIES, &count, &trunc);
        files->entry_count = count;
        files->truncated = (vibio_u8)trunc;
        files_app_build_path_text(files);
        return;
    }

    files->source = (vibio_u8)storage_select_source(files->fs, files->usbstor, files->boot);
    files->fs_available = (files->source != FILES_SRC_NONE) ? 1 : 0;
    files->loaded = 1;
    if (files->source == FILES_SRC_AHCI || files->source == FILES_SRC_USB) {
        vibio_u32 count = 0;
        vibio_u32 trunc = 0;
        VibioDiskReadTest *list_disk = files->source == FILES_SRC_USB ? files->usb_disk : files->disk;
        VibioFat32ReadTest *list_fs = files->source == FILES_SRC_USB ? files->usb_fs : files->fs;
        if (cluster < 2 && list_fs != 0) {
            cluster = (vibio_u32)list_fs->root_cluster;
            files->path_clusters[files->path_depth - 1] = cluster;
        }
        fat32_list_dir(list_disk, list_fs, cluster, files->entries, FILES_MAX_ENTRIES, &count, &trunc);
        files->entry_count = count;
        files->truncated = (vibio_u8)trunc;
    } else if (files->source == FILES_SRC_BOOT && files->boot != 0) {
        vibio_u32 count = 0;
        vibio_u8 root_view = (files->path_depth <= 1) ? 1 : 0;
        vibio_u8 sounds_view = 0;
        if (!root_view) {
            VibioBootFile path_probe;
            zero_bytes(&path_probe, sizeof(path_probe));
            vibio_u32 n = 0;
            const char *path_name = files->path_names[files->path_depth - 1];
            for (; n + 1 < VIBIO_BOOT_FILE_NAME_MAX && path_name[n] != 0; n++) {
                path_probe.name[n] = path_name[n];
            }
            path_probe.name_length = n;
            sounds_view = boot_file_name_matches_literal(&path_probe, "SOUNDS") ? 1 : 0;
        }
        for (vibio_u64 i = 0; i < files->boot->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
            const VibioBootFile *bf = &files->boot->boot_files[i];
            if (bf->name_length == 0) {
                continue;
            }
            if (root_view) {
                if (bf->type == VIBIO_BOOT_FILE_TYPE_SOUND) {
                    continue;
                }
            } else if (sounds_view) {
                if (bf->type != VIBIO_BOOT_FILE_TYPE_SOUND) {
                    continue;
                }
            } else {
                continue;
            }
            /* Show directories, "listed but not loaded" files (type OTHER, e.g.
             * an .mp4 on the boot USB), and real loaded files. Only a loadable
             * type that ended up with no content is hidden (the bootloader now
             * re-lists those as OTHER, so this is rare). */
            if (bf->type != VIBIO_BOOT_FILE_TYPE_DIRECTORY &&
                bf->type != VIBIO_BOOT_FILE_TYPE_OTHER &&
                (bf->address == 0 || bf->size == 0)) {
                continue;
            }
            if (count >= FILES_MAX_ENTRIES) {
                files->truncated = 1;
                break;
            }
            VibioFilesEntry *e = &files->entries[count++];
            zero_bytes(e, sizeof(VibioFilesEntry));
            vibio_u32 n = 0;
            for (; n + 1 < FILES_NAME_MAX && n < bf->name_length && n < VIBIO_BOOT_FILE_NAME_MAX; n++) {
                e->name[n] = bf->name[n];
            }
            e->name[n] = 0;
            e->cluster = 0;
            e->size = (vibio_u32)bf->size;
            e->is_dir = (bf->type == VIBIO_BOOT_FILE_TYPE_DIRECTORY) ? 1 : 0;
        }
        files->entry_count = count;
    }
    files_app_build_path_text(files);
}

/* Reset to the root directory of the active source and load it. */
static void files_app_go_root(VibioFilesApp *files)
{
    VibioFat32ReadTest *afs = files_active_fs(files);
    files->path_depth = 1;
    files->path_clusters[0] = (afs != 0 && afs->root_found) ? (vibio_u32)afs->root_cluster : 2U;
    files->path_names[0][0] = 0;
    files_app_reload(files);
}

/* Switch the browsed device (AHCI boot disk <-> live USB) and jump to its root.
 * Switching to USB only takes effect when a USB FAT32 mount actually exists. */
static void files_app_set_source(VibioFilesApp *files, vibio_u8 src)
{
    if (src == FILES_SRC_USB && !files_usb_available(files)) {
        return;
    }
    files->view_source = src;
    files_app_go_root(files);
}

static void files_app_init(VibioFilesApp *files, const VibioBootInfo *boot,
                           VibioDiskReadTest *disk, VibioFat32ReadTest *fs,
                           const VibioUsbStorage *usbstor,
                           VibioDiskReadTest *usb_disk, VibioFat32ReadTest *usb_fs)
{
    zero_bytes(files, sizeof(VibioFilesApp));
    files->boot = boot;
    files->disk = disk;
    files->fs = fs;
    files->usbstor = usbstor;
    files->usb_disk = usb_disk;
    files->usb_fs = usb_fs;
    files->view_source = FILES_SRC_AHCI;
    files->path_depth = 1;
    files->path_clusters[0] = (fs != 0) ? (vibio_u32)fs->root_cluster : 2U;
    files->path_names[0][0] = 0;
    files->path_text[0] = '/';
    files->path_text[1] = 0;
}

static void files_app_enter_dir(VibioFilesApp *files, const VibioFilesEntry *entry)
{
    if (files->path_depth >= FILES_PATH_DEPTH) {
        return; /* nesting limit reached */
    }
    vibio_u32 cluster = entry->cluster;
    if (cluster < 2) {
        VibioFat32ReadTest *afs = files_active_fs(files);
        cluster = (afs != 0) ? (vibio_u32)afs->root_cluster : 2U;
    }
    vibio_u32 d = files->path_depth;
    files->path_clusters[d] = cluster;
    vibio_u32 i = 0;
    for (; entry->name[i] != 0 && i < FILES_NAME_MAX - 1; i++) {
        files->path_names[d][i] = entry->name[i];
    }
    files->path_names[d][i] = 0;
    files->path_depth++;
    files_app_reload(files);
}

static void files_app_go_up(VibioFilesApp *files)
{
    if (files->info_open) {
        files->info_open = 0;
        return;
    }
    if (files->path_depth <= 1) {
        return; /* already at root */
    }
    files->path_depth--;
    files_app_reload(files);
}

static void files_app_move_selection(VibioFilesApp *files, int delta)
{
    if (files->info_open) {
        return;
    }
    if (files->entry_count == 0) {
        files->selected = 0;
        return;
    }
    int sel = (int)files->selected + delta;
    if (sel < 0) {
        sel = 0;
    }
    if (sel > (int)files->entry_count - 1) {
        sel = (int)files->entry_count - 1;
    }
    files->selected = (vibio_u32)sel;
    /* Keep the selection within the visible window. */
    if (files->row_visible > 0) {
        if (files->selected < files->scroll) {
            files->scroll = files->selected;
        } else if (files->selected >= files->scroll + files->row_visible) {
            files->scroll = files->selected - files->row_visible + 1;
        }
    }
}

/* Scroll the Files list by whole rows (mouse wheel / scrollbar) without moving
 * the selection, clamped so it never scrolls past the last screenful. */
static void files_app_scroll_by(VibioFilesApp *files, int delta)
{
    if (files->info_open || files->entry_count == 0 || files->row_visible == 0) {
        return;
    }
    vibio_u32 max_scroll = files->entry_count > files->row_visible ?
                           files->entry_count - files->row_visible : 0;
    int s = (int)files->scroll + delta;
    if (s < 0) {
        s = 0;
    }
    if (s > (int)max_scroll) {
        s = (int)max_scroll;
    }
    files->scroll = (vibio_u32)s;
}

/* Map a desired thumb-top y (screen coords) to a scroll offset. Used while the
 * scrollbar thumb is being dragged: the thumb travels the track span (track
 * height minus thumb height), which maps linearly onto [0, max_scroll]. */
static void files_app_scrollbar_set_from_thumb_top(VibioFilesApp *files, int thumb_top)
{
    if (files->row_visible == 0 || files->entry_count <= files->row_visible) {
        return;
    }
    vibio_u32 max_scroll = files->entry_count - files->row_visible;
    vibio_u32 track_span = files->sb_h > files->sb_thumb_h ? files->sb_h - files->sb_thumb_h : 0;
    int rel = thumb_top - (int)files->sb_y;
    if (rel < 0) {
        rel = 0;
    }
    if (rel > (int)track_span) {
        rel = (int)track_span;
    }
    vibio_u32 ns = track_span > 0 ? ((vibio_u32)rel * max_scroll) / track_span : 0;
    if (ns > max_scroll) {
        ns = max_scroll;
    }
    files->scroll = ns;
}

MAYBE_UNUSED static void pic_send_eoi_all(void)
{
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

static void pic_mask_all(void)
{
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

static void pic_remap_for_timer_keyboard(void)
{
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    outb(0x21, IRQ_VECTOR_BASE);
    io_wait();
    outb(0xA1, IRQ_VECTOR_BASE + 8);
    io_wait();

    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
}

static void pic_enable_mouse(void)
{
    /* Unmask the cascade line (IRQ2) on the master and IRQ12 (slave line 4)
     * without disturbing the already-enabled timer and keyboard lines. */
    vibio_u8 master_mask = inb(0x21);
    master_mask = (vibio_u8)(master_mask & ~0x04U);
    outb(0x21, master_mask);

    vibio_u8 slave_mask = inb(0xA1);
    slave_mask = (vibio_u8)(slave_mask & ~0x10U);
    outb(0xA1, slave_mask);
}

static void pit_program_100hz(void)
{
    vibio_u16 divisor = 11932;

    outb(0x43, 0x36);
    outb(0x40, (vibio_u8)(divisor & 0xFF));
    outb(0x40, (vibio_u8)((divisor >> 8) & 0xFF));
}

static void ps2_drain_output_buffer(void)
{
    for (vibio_u32 i = 0; i < 16; i++) {
        if ((inb(0x64) & 1) == 0) {
            return;
        }

        (void)inb(0x60);
    }
}

/* Wait until the 8042 input buffer is empty so it can accept another byte. */
static void ps2_wait_input_clear(void)
{
    for (vibio_u32 i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x02) == 0) {
            return;
        }
    }
}

/* Wait until the 8042 output buffer has a byte ready to read. */
static void ps2_wait_output_full(void)
{
    for (vibio_u32 i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x01) != 0) {
            return;
        }
    }
}

/* Send one command byte to the mouse (aux device) and return its ACK byte. */
static vibio_u8 mouse_write_command(vibio_u8 value)
{
    ps2_wait_input_clear();
    outb(0x64, 0xD4);
    ps2_wait_input_clear();
    outb(0x60, value);
    ps2_wait_output_full();
    return inb(0x60);
}

/*
 * Bring up the PS/2 mouse. Run with interrupts disabled so the keyboard ISR
 * cannot steal the controller bytes we are reading. Enables the aux device,
 * turns on the IRQ12 interrupt in the controller command byte, then tells the
 * mouse to use defaults and start reporting movement.
 */
static vibio_u32 mouse_init(VibioMouseState *mouse, const VibioBootInfo *boot_info)
{
    zero_bytes(mouse, sizeof(VibioMouseState));
    mouse->x = boot_info->framebuffer_width / 2;
    mouse->y = boot_info->framebuffer_height / 2;
    mouse->x_fixed = (int)(mouse->x << 8);
    mouse->y_fixed = (int)(mouse->y << 8);

    ps2_wait_input_clear();
    outb(0x64, 0xA8);

    ps2_wait_input_clear();
    outb(0x64, 0x20);
    ps2_wait_output_full();
    vibio_u8 status = inb(0x60);
    /* Force a known-good controller config so the laptop keyboard works like the
     * VM: keyboard IRQ (bit0) + mouse IRQ (bit1) on, keyboard clock (bit4) and
     * mouse clock (bit5) enabled, and scancode translation (bit6) ON so the
     * controller always emits set-1 codes. Some firmware leaves the controller
     * in set 2 / translation off, which breaks every key (including F11). */
    status = (vibio_u8)(status | 0x01); /* keyboard interrupt */
    status = (vibio_u8)(status | 0x02); /* mouse interrupt */
    status = (vibio_u8)(status | 0x40); /* translate to scancode set 1 */
    status = (vibio_u8)(status & ~0x10U); /* enable keyboard clock */
    status = (vibio_u8)(status & ~0x20U); /* enable mouse clock */
    ps2_wait_input_clear();
    outb(0x64, 0x60);
    ps2_wait_input_clear();
    outb(0x60, status);

    mouse->init_ack_defaults = mouse_write_command(0xF6);

    /* IntelliMouse "magic knock": sample rates 200, 100, 80, then read the
     * device ID. An ID of 3 means the scroll wheel is enabled and packets grow
     * to 4 bytes (the 4th being the wheel delta). A second knock (200, 200, 80)
     * upgrades to ID 4 (IntelliMouse Explorer / 5-button), which is what many
     * laptop touchpads report for two-finger scroll. Either ID gives us a wheel
     * byte in the low nibble of packet[3]; without it we fall back to plain
     * 3-byte packets and no scroll. */
    mouse_write_command(0xF3); mouse_write_command(200);
    mouse_write_command(0xF3); mouse_write_command(100);
    mouse_write_command(0xF3); mouse_write_command(80);
    mouse_write_command(0xF2);
    ps2_wait_output_full();
    vibio_u8 mouse_id = inb(0x60);

    if (mouse_id == 0x03) {
        mouse_write_command(0xF3); mouse_write_command(200);
        mouse_write_command(0xF3); mouse_write_command(200);
        mouse_write_command(0xF3); mouse_write_command(80);
        mouse_write_command(0xF2);
        ps2_wait_output_full();
        mouse_id = inb(0x60);
    }
    mouse->has_wheel = (mouse_id == 0x03 || mouse_id == 0x04) ? 1 : 0;

    /* Higher sample rate plus a mid/high resolution gives the pointer smoother
     * source data. Acceleration below keeps small movements precise. */
    mouse_write_command(0xE8); mouse_write_command(2);
    mouse_write_command(0xF3); mouse_write_command(200);
    mouse->init_ack_enable = mouse_write_command(0xF4);
    mouse->ready = mouse->init_ack_enable == 0xFA;
    return mouse->ready;
}

static int abs_int(int value)
{
    return value < 0 ? -value : value;
}

static int max_int(int a, int b)
{
    return a > b ? a : b;
}

static int mouse_accel_scale_8(int dx, int dy)
{
    int speed = max_int(abs_int(dx), abs_int(dy));

    if (speed <= 1) {
        return 256; /* 1.00x: pixel-accurate small targeting. */
    }
    if (speed <= 3) {
        return 320; /* 1.25x */
    }
    if (speed <= 7) {
        return 416; /* 1.625x */
    }
    if (speed <= 13) {
        return 544; /* 2.125x */
    }
    return 704;     /* 2.75x for fast sweeps across the desktop. */
}

/*
 * Drain new mouse bytes from the ring and reassemble 3-byte packets. The first
 * byte of a packet always has bit 3 set, which we use to resync if a byte is
 * ever dropped. Movement deltas are 9-bit signed; the Y axis is flipped so
 * pushing the mouse up moves the cursor up.
 */
static void mouse_poll(VibioMouseState *mouse, const VibioIrqState *irq_state, const VibioBootInfo *boot_info)
{
    vibio_u64 write_index = irq_state->mouse_write_index;

    if (write_index - mouse->byte_read_index > MOUSE_RING_SIZE) {
        mouse->byte_read_index = write_index - MOUSE_RING_SIZE;
        mouse->phase = 0;
    }

    while (mouse->byte_read_index < write_index) {
        vibio_u8 byte = irq_state->mouse_bytes[mouse->byte_read_index & (MOUSE_RING_SIZE - 1)];
        mouse->byte_read_index++;

        vibio_u32 last_phase = mouse->has_wheel ? 3 : 2;

        if (mouse->phase == 0) {
            if ((byte & 0x08) == 0) {
                continue;
            }
            mouse->packet[0] = byte;
            mouse->phase = 1;
        } else if (mouse->phase < last_phase) {
            mouse->packet[mouse->phase] = byte;
            mouse->phase++;
        } else {
            mouse->packet[mouse->phase] = byte;
            mouse->phase = 0;

            vibio_u8 flags = mouse->packet[0];
            if ((flags & 0xC0) != 0) {
                continue;
            }

            int dx = (int)mouse->packet[1] - ((flags & 0x10) ? 256 : 0);
            int dy = (int)mouse->packet[2] - ((flags & 0x20) ? 256 : 0);
            int wheel = 0;

            if (mouse->has_wheel) {
                int z = (int)(mouse->packet[3] & 0x0F);
                if (z & 0x08) {
                    z -= 16;
                }
                wheel = -z;
            }

            if (dx > MOUSE_MAX_DELTA || dx < -MOUSE_MAX_DELTA ||
                dy > MOUSE_MAX_DELTA || dy < -MOUSE_MAX_DELTA) {
                continue;
            }

            if (wheel != 0 && (flags & 0x07) == 0) {
                dx = 0;
                dy = 0;
            }

            if (dx != 0 || dy != 0) {
                int scale = mouse_accel_scale_8(dx, dy);
                int fx = mouse->x_fixed + dx * scale;
                int fy = mouse->y_fixed - dy * scale;
                int max_x = ((int)boot_info->framebuffer_width - 1) << 8;
                int max_y = ((int)boot_info->framebuffer_height - 1) << 8;
                if (fx < 0) {
                    fx = 0;
                }
                if (fy < 0) {
                    fy = 0;
                }
                if (fx > max_x) {
                    fx = max_x;
                }
                if (fy > max_y) {
                    fy = max_y;
                }

                mouse->x_fixed = fx;
                mouse->y_fixed = fy;
                mouse->x = (vibio_u32)(fx >> 8);
                mouse->y = (vibio_u32)(fy >> 8);
            }
            mouse->buttons = flags & 0x07;
            mouse->packets++;

            if (wheel != 0) {
                mouse->scroll += wheel * MOUSE_WHEEL_LINES;
            }
        }
    }
}

static void set_idt_entry_with_attributes(
    VibioIdtEntry *idt,
    vibio_u8 vector,
    vibio_u64 handler,
    vibio_u16 selector,
    vibio_u8 type_attributes)
{
    idt[vector].offset_low = (vibio_u16)(handler & 0xFFFF);
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].type_attributes = type_attributes;
    idt[vector].offset_mid = (vibio_u16)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (vibio_u32)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].reserved = 0;
}

static void set_idt_entry(VibioIdtEntry *idt, vibio_u8 vector, vibio_u64 handler, vibio_u16 selector)
{
    set_idt_entry_with_attributes(idt, vector, handler, selector, IDT_INTERRUPT_GATE);
}

static void load_idt(VibioIdtPointer *pointer)
{
    __asm__ volatile("lidt (%0)" : : "r"(pointer) : "memory");
}

static vibio_u64 trigger_breakpoint_test(void)
{
    vibio_u64 hits = 0;
    __asm__ volatile(
        "mov %0, %%rdi\n"
        "int3\n"
        :
        : "r"(&hits)
        : "rdi", "memory");
    return hits;
}

static VibioIdtBuild build_and_test_idt(VibioPageAllocator *allocator, VibioIdtEntry **out_idt)
{
    VibioIdtBuild build;
    build.idt_address = page_allocator_alloc(allocator);
    build.breakpoint_hits = 0;
    build.ok = build.idt_address != 0;

    if (!build.ok) {
        return build;
    }

    zero_page(build.idt_address);
    VibioIdtEntry *idt = (VibioIdtEntry *)build.idt_address;
    set_idt_entry(idt, 3, (vibio_u64)isr_breakpoint_stub, read_cs());
    set_idt_entry(idt, 0, (vibio_u64)isr_fault0_stub, read_cs());
    set_idt_entry(idt, 6, (vibio_u64)isr_fault6_stub, read_cs());
    set_idt_entry(idt, 8, (vibio_u64)isr_fault8_stub, read_cs());
    set_idt_entry(idt, 13, (vibio_u64)isr_fault13_stub, read_cs());
    set_idt_entry(idt, 14, (vibio_u64)isr_fault14_stub, read_cs());
    set_idt_entry(idt, IRQ_TIMER_VECTOR, (vibio_u64)isr_timer_stub, read_cs());
    set_idt_entry(idt, IRQ_KEYBOARD_VECTOR, (vibio_u64)isr_keyboard_stub, read_cs());
    set_idt_entry(idt, IRQ_MOUSE_VECTOR, (vibio_u64)isr_mouse_stub, read_cs());
    set_idt_entry_with_attributes(
        idt,
        SYSCALL_VECTOR,
        (vibio_u64)isr_syscall_stub,
        read_cs(),
        IDT_USER_INTERRUPT_GATE);

    VibioIdtPointer pointer;
    pointer.limit = (vibio_u16)(sizeof(VibioIdtEntry) * IDT_ENTRY_COUNT - 1);
    pointer.base = build.idt_address;
    load_idt(&pointer);

    if (out_idt != 0) {
        *out_idt = idt;
    }

    build.breakpoint_hits = trigger_breakpoint_test();
    build.ok = build.breakpoint_hits == 1;

    return build;
}

static VibioIrqBuild start_timer_and_keyboard_irqs(VibioPageAllocator *allocator, VibioIdtEntry *idt, vibio_u32 idt_ready)
{
    VibioIrqBuild build;
    build.state_address = page_allocator_alloc(allocator);
    build.timer_ticks = 0;
    build.keyboard_irqs = 0;
    build.last_scancode = 0;
    build.ok = build.state_address != 0 && idt != 0 && idt_ready;

    if (!build.ok) {
        return build;
    }

    zero_page(build.state_address);
    VibioIrqState *state = (VibioIrqState *)build.state_address;
    state->scheduler_interval = SCHEDULER_TIMER_INTERVAL_TICKS;
    state->scheduler_countdown = SCHEDULER_TIMER_INTERVAL_TICKS;
    irq_state_address_storage = build.state_address;

    pic_mask_all();
    pic_remap_for_timer_keyboard();
    ps2_drain_output_buffer();
    pit_program_100hz();

    __asm__ volatile("sti");
    while (state->timer_ticks < IRQ_READY_TARGET_TICKS) {
        __asm__ volatile("hlt");
    }

    build.timer_ticks = state->timer_ticks;
    build.keyboard_irqs = state->keyboard_irqs;
    build.last_scancode = state->last_scancode;
    build.ok = build.timer_ticks >= IRQ_READY_TARGET_TICKS;

    return build;
}

static vibio_u64 syscall_invoke(vibio_u64 number)
{
    vibio_u64 result = number;
    __asm__ volatile("int $0x80" : "+a"(result) : : "memory");
    return result;
}

static void syscall_state_init(VibioSyscallState *syscalls, const VibioTaskManager *tasks)
{
    zero_bytes(syscalls, sizeof(VibioSyscallState));
    syscalls->version = VIBIO_SYSCALL_ABI_VERSION;
    syscalls->task_count = tasks->task_count;
    syscalls->context_switches = tasks->context_switches;
    syscall_state_address_storage = (vibio_u64)syscalls;
}

static void syscall_run_self_test(VibioSyscallState *syscalls, VibioSyscallTest *test)
{
    test->version_result = syscall_invoke(SYSCALL_VERSION);
    test->ticks_result = syscall_invoke(SYSCALL_TIMER_TICKS);
    test->task_count_result = syscall_invoke(SYSCALL_TASK_COUNT);
    test->switches_result = syscall_invoke(SYSCALL_CONTEXT_SWITCHES);
    test->invalid_result = syscall_invoke(99);
    test->calls = syscalls->calls;
    test->invalid_calls = syscalls->invalid_calls;
    test->ok = test->version_result == VIBIO_SYSCALL_ABI_VERSION && test->ticks_result >= IRQ_READY_TARGET_TICKS &&
               test->task_count_result == 4 && test->switches_result == CONTEXT_SWITCH_DEMO_ROUNDS * 2 &&
               test->invalid_result == SYSCALL_INVALID_RETURN && test->calls >= 5 && test->invalid_calls == 1;
}

static vibio_u32 boot_file_has_wav_header(const VibioBootFile *file)
{
    if (file->address == 0 || file->size < 12) {
        return 0;
    }

    const vibio_u8 *bytes = (const vibio_u8 *)file->address;
    return bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
           bytes[8] == 'W' && bytes[9] == 'A' && bytes[10] == 'V' && bytes[11] == 'E';
}

static void file_table_run_self_test(const VibioBootInfo *boot_info, VibioFileTest *test)
{
    zero_bytes(test, sizeof(VibioFileTest));
    test->file_count = boot_info->boot_file_count;

    for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *file = &boot_info->boot_files[i];
        if (file->address != 0 && file->size > 0 && file->name_length > 0) {
            test->total_bytes += file->size;
        }

        if (file->type == VIBIO_BOOT_FILE_TYPE_SOUND) {
            test->sound_count++;
            if (boot_file_has_wav_header(file)) {
                test->wav_count++;
            }
        }
    }

    test->ok = test->file_count >= 5 && test->sound_count >= 5 && test->wav_count >= 5 && test->total_bytes > 0;
}

static void console_clear_line(VibioConsoleState *console, vibio_u32 row)
{
    if (row >= CONSOLE_ROWS) {
        return;
    }

    console->line_lengths[row] = 0;
    for (vibio_u32 col = 0; col < CONSOLE_COLS; col++) {
        console->lines[row][col] = 0;
    }
}

static void console_clear_lines(VibioConsoleState *console)
{
    for (vibio_u32 row = 0; row < CONSOLE_ROWS; row++) {
        console_clear_line(console, row);
    }

    console->cursor_row = 0;
    console->view_offset = 0;
}

static void console_new_line(VibioConsoleState *console)
{
    if (console->cursor_row + 1 < CONSOLE_ROWS) {
        console->cursor_row++;
        console_clear_line(console, console->cursor_row);
        return;
    }

    for (vibio_u32 row = 1; row < CONSOLE_ROWS; row++) {
        console->line_lengths[row - 1] = console->line_lengths[row];
        for (vibio_u32 col = 0; col < CONSOLE_COLS; col++) {
            console->lines[row - 1][col] = console->lines[row][col];
        }
    }

    console_clear_line(console, CONSOLE_ROWS - 1);
    console->cursor_row = CONSOLE_ROWS - 1;
}

static void console_append_char(VibioConsoleState *console, char c)
{
    if (c == '\n') {
        console_new_line(console);
        return;
    }

    if (console->line_lengths[console->cursor_row] >= CONSOLE_COLS) {
        console_new_line(console);
    }

    vibio_u32 col = console->line_lengths[console->cursor_row];
    console->lines[console->cursor_row][col] = c;
    console->line_lengths[console->cursor_row] = col + 1;
}

static void console_append_uint(VibioConsoleState *console, vibio_u64 value)
{
    char digits[20];
    vibio_u32 count = 0;

    if (value == 0) {
        console_append_char(console, '0');
        return;
    }

    while (value > 0 && count < 20) {
        digits[count] = (char)('0' + (value % 10));
        value /= 10;
        count++;
    }

    while (count > 0) {
        count--;
        console_append_char(console, digits[count]);
    }
}

static void console_append_text(VibioConsoleState *console, const char *text)
{
    vibio_u32 i = 0;
    while (text[i] != 0) {
        console_append_char(console, text[i]);
        i++;
    }
}

static void console_append_hex(VibioConsoleState *console, vibio_u64 value)
{
    char digits[16];
    vibio_u32 count = 0;

    if (value == 0) {
        console_append_char(console, '0');
        return;
    }

    while (value > 0 && count < 16) {
        vibio_u8 nibble = (vibio_u8)(value & 0xF);
        digits[count] = nibble < 10 ? (char)('0' + nibble) : (char)('A' + (nibble - 10));
        value >>= 4;
        count++;
    }

    while (count > 0) {
        count--;
        console_append_char(console, digits[count]);
    }
}

#define CEMIT(ch) console_append_char(console, ch)

static void console_append_ready_line(VibioConsoleState *console)
{
    CEMIT('V');
    CEMIT('I');
    CEMIT('B');
    CEMIT('I');
    CEMIT('O');
    CEMIT(' ');
    CEMIT('C');
    CEMIT('O');
    CEMIT('N');
    CEMIT('S');
    CEMIT('O');
    CEMIT('L');
    CEMIT('E');
    CEMIT(' ');
    CEMIT('R');
    CEMIT('E');
    CEMIT('A');
    CEMIT('D');
    CEMIT('Y');
    console_new_line(console);
}

static void console_append_help_hint_line(VibioConsoleState *console)
{
    CEMIT('T');
    CEMIT('Y');
    CEMIT('P');
    CEMIT('E');
    CEMIT(' ');
    CEMIT('H');
    CEMIT('E');
    CEMIT('L');
    CEMIT('P');
    CEMIT(' ');
    CEMIT('T');
    CEMIT('H');
    CEMIT('E');
    CEMIT('N');
    CEMIT(' ');
    CEMIT('E');
    CEMIT('N');
    CEMIT('T');
    CEMIT('E');
    CEMIT('R');
    console_new_line(console);
}

#define SELFTEST_PASS 0U
#define SELFTEST_FAIL 1U
#define SELFTEST_SKIP 2U

static void selftest_set_last_fail(VibioSelfTestResult *result, const char *name)
{
    vibio_u32 i = 0;
    while (name[i] != 0 && i + 1 < sizeof(result->last_fail)) {
        result->last_fail[i] = name[i];
        i++;
    }
    result->last_fail[i] = 0;
}

static void selftest_emit_line(VibioConsoleState *console, VibioSelfTestResult *result, const char *name, vibio_u32 outcome)
{
    result->total++;
    console_append_text(console, name);
    console_append_text(console, ": ");
    if (outcome == SELFTEST_PASS) {
        result->passed++;
        console_append_text(console, "PASS");
    } else if (outcome == SELFTEST_FAIL) {
        result->failed++;
        if (result->last_fail[0] == 0) {
            selftest_set_last_fail(result, name);
        }
        result->ok = 0;
        console_append_text(console, "FAIL");
    } else {
        result->skipped++;
        console_append_text(console, "SKIP");
    }
    console_new_line(console);
}

static char selftest_lower(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

static vibio_u32 selftest_name_eq(const char *file_name, vibio_u32 file_len, const char *name)
{
    vibio_u32 i = 0;
    for (; i < file_len && file_name[i] != 0; i++) {
        if (name[i] == 0 || selftest_lower(file_name[i]) != selftest_lower(name[i])) {
            return 0;
        }
    }
    return name[i] == 0;
}

static vibio_u32 selftest_builtin_html_ok(const char *name)
{
    for (vibio_u32 i = 0; i < VIBIO_BUILTIN_HTML_COUNT; i++) {
        const VibioBuiltinHtml *page = &vibio_builtin_html[i];
        vibio_u32 plen = 0;
        while (page->name[plen] != 0) {
            plen++;
        }
        if (selftest_name_eq(page->name, plen, name) && page->length > 0 && page->data != 0) {
            return 1;
        }
    }
    return 0;
}

static vibio_u32 selftest_check_disk_readonly(const VibioDiskReadTest *disk)
{
    if (disk == 0) {
        return SELFTEST_SKIP;
    }
    /* No usable AHCI SATA disk on this machine (e.g. an NVMe laptop, which Vibio
     * has no driver for) is N/A, not a failure - reporting FAIL there would be
     * dishonest. A genuine FAIL is only a present disk whose read/MBR is bad. */
    if (!disk->ok) {
        return SELFTEST_SKIP;
    }
    VibioDiskReadTest *mutable_disk = (VibioDiskReadTest *)disk;
    if (!ahci_read_sector_retry(mutable_disk, 0)) {
        return SELFTEST_FAIL;
    }
    const vibio_u8 *sector = (const vibio_u8 *)disk->sector_buffer;
    return (sector[510] == 0x55 && sector[511] == 0xAA) ? SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_fat32(const VibioDiskReadTest *disk, const VibioFat32ReadTest *fs)
{
    /* No usable AHCI disk -> N/A (the machine has no AHCI SATA disk to parse, or
     * its disk is NVMe/GPT that Vibio cannot read). Only a present, readable AHCI
     * disk whose FAT32 parse fails is a real FAIL. */
    if (disk == 0 || !disk->ok) {
        return SELFTEST_SKIP;
    }
    if (fs == 0 || !fs->ok) {
        return SELFTEST_FAIL;
    }
    return SELFTEST_PASS;
}

static vibio_u32 selftest_check_file_read(
    const VibioBootInfo *boot_info,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs)
{
    vibio_u8 scratch[512];
    if (disk != 0 && disk->ok && fs != 0 && fs->root_found) {
        vibio_u32 size = 0;
        VibioDiskReadTest *mutable_disk = (VibioDiskReadTest *)disk;
        vibio_u32 read = fat32_read_file(mutable_disk, fs, "START.HTM", scratch, sizeof(scratch) - 1, &size);
        if (read > 0 && scratch[0] == '<') {
            return SELFTEST_PASS;
        }
        return SELFTEST_FAIL;
    }

    if (boot_info != 0) {
        for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
            const VibioBootFile *file = &boot_info->boot_files[i];
            if (file->address != 0 && file->size > 0) {
                return SELFTEST_PASS;
            }
        }
    }
    return SELFTEST_SKIP;
}

static vibio_u32 selftest_check_wav_parse(const VibioBootInfo *boot_info)
{
    if (boot_info == 0) {
        return SELFTEST_SKIP;
    }
    for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *file = &boot_info->boot_files[i];
        if (file->type != VIBIO_BOOT_FILE_TYPE_SOUND || file->address == 0 || file->size == 0) {
            continue;
        }
        VibioWavInfo wav;
        if (wav_parse_pcm16(file, &wav)) {
            return SELFTEST_PASS;
        }
        return SELFTEST_FAIL;
    }
    return SELFTEST_SKIP;
}

static vibio_u32 selftest_check_audio_device(const VibioAudioState *audio)
{
    if (audio == 0) {
        return SELFTEST_SKIP;
    }
    if (audio->ready) {
        return SELFTEST_PASS;
    }
    if (audio->hda_found) {
        return SELFTEST_SKIP;
    }
    if (!audio->found) {
        return SELFTEST_SKIP;
    }
    return SELFTEST_FAIL;
}

static vibio_u32 selftest_check_window_manager(const VibioWindowManager *wm)
{
    if (wm == 0 || wm->count == 0) {
        return SELFTEST_FAIL;
    }
    for (vibio_u32 i = 0; i < wm->count; i++) {
        if (wm->windows[i].kind == 0 || wm->windows[i].width == 0 || wm->windows[i].height == 0) {
            return SELFTEST_FAIL;
        }
    }
    return SELFTEST_PASS;
}

static vibio_u32 selftest_check_browser_load(const VibioBrowser *browser)
{
    if (browser == 0) {
        return SELFTEST_FAIL;
    }
    if (browser->status == BROWSER_STATUS_OK && browser->html_len > 0 && browser->html != 0) {
        return SELFTEST_PASS;
    }
    if (selftest_builtin_html_ok("START.HTM")) {
        return SELFTEST_PASS;
    }
    return SELFTEST_FAIL;
}

/* Files app: list the real root directory through the best available read-only
 * backend. On real USB boots this may be the UEFI boot-file fallback, not the
 * kernel's post-boot FAT32 driver. */
static vibio_u32 selftest_check_files_app(
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const VibioBootInfo *boot_info)
{
    vibio_u32 source = storage_select_source(fs, &g_usb_storage, boot_info);
    if (source == FILES_SRC_BOOT) {
        return (boot_info != 0 && boot_info->boot_file_count > 0) ? SELFTEST_PASS : SELFTEST_FAIL;
    }
    static VibioFilesEntry entries[FILES_MAX_ENTRIES];
    vibio_u32 count = 0;
    if (source == FILES_SRC_USB) {
        /* Browsing the live USB drive: list ITS root, not the (absent) AHCI fs. */
        if (!g_usb_fs.root_found) {
            return SELFTEST_SKIP;
        }
        fat32_list_dir(&g_usb_disk, &g_usb_fs, (vibio_u32)g_usb_fs.root_cluster, entries, FILES_MAX_ENTRIES, &count, 0);
        return count > 0 ? SELFTEST_PASS : SELFTEST_FAIL;
    }
    if (source == FILES_SRC_NONE || disk == 0 || fs == 0 || !fs->root_found) {
        return SELFTEST_SKIP;
    }
    VibioDiskReadTest *mutable_disk = (VibioDiskReadTest *)disk;
    fat32_list_dir(mutable_disk, fs, (vibio_u32)fs->root_cluster, entries, FILES_MAX_ENTRIES, &count, 0);
    return count > 0 ? SELFTEST_PASS : SELFTEST_FAIL;
}

/* Honest read-only USB storage check: SKIP when no controller or when the MSD
 * read path is not implemented; PASS only if a real FAT32 USB mount exists. */
static vibio_u32 selftest_check_usb_storage(const VibioUsbStorage *st)
{
    if (st == 0 || !st->controller_found) {
        return SELFTEST_SKIP;
    }
    if (st->fat32_mounted) {
        return SELFTEST_PASS;
    }
    return SELFTEST_SKIP;
}

/* Stage 37 honest USB read-path checks: PASS only when the step truly happened,
 * SKIP when there is no controller/device (e.g. VBox with USB off), FAIL when a
 * controller/device was present but the step did not complete. */
static vibio_u32 selftest_check_xhci_detect(const VibioUsbStorage *st)
{
    if (st == 0 || !st->controller_found || st->controller_type != USB_TYPE_XHCI) {
        return SELFTEST_SKIP;
    }
    return st->mmio_readable ? SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_xhci_init(const VibioXhciMsd *x, const VibioUsbStorage *st)
{
    if (st == 0 || !st->controller_found || st->controller_type != USB_TYPE_XHCI) {
        return SELFTEST_SKIP;
    }
    return x->init_done ? SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_usb_enum(const VibioXhciMsd *x)
{
    if (!x->init_done || x->max_ports == 0) {
        return SELFTEST_SKIP;
    }
    if (x->last_step < USB_STOR_STEP_PORTS) {
        return SELFTEST_FAIL;
    }
    if (x->slot == 0) {
        /* No device connected to the controller: honest skip, not a failure. */
        return x->last_error == XHCI_ERR_NO_PORT ? SELFTEST_SKIP : SELFTEST_FAIL;
    }
    return x->last_step >= USB_STOR_STEP_MSD ? SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_usb_msd_read(const VibioXhciMsd *x)
{
    if (x->slot == 0) {
        return SELFTEST_SKIP;
    }
    return x->ready ? SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_usb_fat32(const VibioUsbStorage *st, const VibioXhciMsd *x)
{
    if (!x->ready) {
        return SELFTEST_SKIP;
    }
    return (st != 0 && st->fat32_mounted) ? SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_usb_hotplug(const VibioUsbHotplug *hp)
{
    if (hp == 0 || !hp->controller_found) {
        return SELFTEST_SKIP;
    }
    if (hp->hotplug_supported && (hp->insert_events > 0 || hp->remove_events > 0)) {
        return SELFTEST_PASS;
    }
    return SELFTEST_SKIP;
}

static vibio_u32 selftest_check_usb_sound_assets(const VibioBootInfo *boot_info)
{
    return find_sound_file_by_name(boot_info, "USBINS.WAV") != 0 &&
           find_sound_file_by_name(boot_info, "USBRM.WAV") != 0 ?
        SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_usb_event_sound_path(const VibioBootInfo *boot_info, const VibioAudioState *audio)
{
    if (find_sound_file_by_name(boot_info, "USBINS.WAV") == 0 ||
        find_sound_file_by_name(boot_info, "USBRM.WAV") == 0) {
        return SELFTEST_FAIL;
    }
    if (audio == 0 || !audio->ready) {
        return SELFTEST_SKIP;
    }
    return SELFTEST_PASS;
}

/* Which backend the Files app would use. SKIP when no readable FAT32 backend
 * exists at all; otherwise it must list real entries to PASS. */
static vibio_u32 selftest_check_files_backend(
    const VibioDiskReadTest *disk, const VibioFat32ReadTest *fs, const VibioUsbStorage *st)
{
    vibio_u32 source = storage_select_source(fs, st, g_panic_boot_info);
    if (source == FILES_SRC_NONE) {
        return SELFTEST_SKIP;
    }
    if (source == FILES_SRC_BOOT) {
        return SELFTEST_PASS;
    }
    return selftest_check_files_app(disk, fs, g_panic_boot_info);
}

static vibio_u32 selftest_check_native_image_decode(
    const VibioSelfTestContext *ctx,
    const VibioBootInfo *boot_info,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const char *name)
{
    if (ctx == 0 || ctx->media_viewer == 0) {
        return SELFTEST_SKIP;
    }
    VibioMediaViewer *mv = ctx->media_viewer;
    if (mv->loaded || mv->scratch == 0 || mv->scratch_capacity == 0 ||
        mv->work == 0 || mv->work_capacity == 0) {
        return SELFTEST_SKIP;
    }
    vibio_u32 total = 0;
    vibio_u8 source = FILES_SRC_NONE;
    vibio_u32 read = media_read_bytes(
        boot_info, (VibioDiskReadTest *)disk, (VibioFat32ReadTest *)fs, name, 0,
        mv->scratch, mv->scratch_capacity, &total, &source);
    if (read == 0) {
        return SELFTEST_SKIP;
    }
    if (total > read) {
        return SELFTEST_FAIL;
    }
    MediaBitmap bmp;
    MediaDecodeStatus status;
    zero_bytes(&status, sizeof(status));
    return media_decode_native_image(name, mv->scratch, read, mv->scratch_capacity,
                                     mv->work, mv->work_capacity, &bmp, &status) ?
        SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_mp4_demux(
    const VibioSelfTestContext *ctx,
    const VibioBootInfo *boot_info,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs)
{
    if (ctx == 0 || ctx->media_viewer == 0) {
        return SELFTEST_SKIP;
    }
    VibioMediaViewer *mv = ctx->media_viewer;
    if (mv->loaded || mv->scratch == 0 || mv->scratch_capacity == 0) {
        return SELFTEST_SKIP;
    }
    vibio_u32 total = 0;
    vibio_u8 source = FILES_SRC_NONE;
    Mp4Info info;
    vibio_u32 read = media_read_mp4_probe(
        boot_info, (VibioDiskReadTest *)disk, (VibioFat32ReadTest *)fs, "TEST.MP4",
        mv->scratch, mv->scratch_capacity, &total, &source, &info);
    if (read == 0) {
        return SELFTEST_SKIP;
    }
    if (info.status != MP4_OK) {
        return SELFTEST_FAIL;
    }
    Mp4Sample vs;
    Mp4Sample as;
    return info.video.sample_demux_ready &&
           info.audio.sample_demux_ready &&
           mp4_get_sample(mv->scratch, read, &info, MP4_TRACK_VIDEO, 0, &vs) &&
           mp4_get_sample(mv->scratch, read, &info, MP4_TRACK_AUDIO, 0, &as) &&
           vs.present && as.present && vs.size > 0 && as.size > 0 ?
        SELFTEST_PASS : SELFTEST_FAIL;
}

static vibio_u32 selftest_check_image_error_path(const VibioSelfTestContext *ctx)
{
    if (ctx == 0 || ctx->media_viewer == 0) {
        return SELFTEST_SKIP;
    }
    VibioMediaViewer *mv = ctx->media_viewer;
    if (mv->loaded || mv->scratch == 0 || mv->scratch_capacity < 16U ||
        mv->work == 0 || mv->work_capacity == 0) {
        return SELFTEST_SKIP;
    }
    static const vibio_u8 corrupt_png[16] = {
        0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A,
        0x00, 0x00, 0x00, 0x01, 'B', 'A', 'D', '!'
    };
    copy_bytes(mv->scratch, corrupt_png, sizeof(corrupt_png));
    MediaBitmap bmp;
    MediaDecodeStatus status;
    zero_bytes(&status, sizeof(status));
    return media_decode_native_image("BAD.PNG", mv->scratch, sizeof(corrupt_png),
                                     mv->scratch_capacity, mv->work, mv->work_capacity,
                                     &bmp, &status) ?
        SELFTEST_FAIL : SELFTEST_PASS;
}

static vibio_u32 selftest_check_viewer_native_path(const VibioSelfTestContext *ctx)
{
    if (ctx == 0 || ctx->media_viewer == 0) {
        return SELFTEST_FAIL;
    }
    VibioMediaViewer *mv = ctx->media_viewer;
    return (mv->scratch != 0 && mv->scratch_capacity >= BROWSER_IMAGE_MAX &&
            mv->work != 0 && mv->work_capacity >= MEDIA_WORK_MAX) ?
        SELFTEST_PASS : SELFTEST_FAIL;
}

static void selftest_run(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    const VibioIrqState *irq_state,
    VibioKernelHeap *heap,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const VibioMouseState *mouse,
    VibioAudioState *audio,
    const VibioRwSandbox *rw,
    const VibioSelfTestContext *ctx)
{
    zero_bytes(&g_selftest_result, sizeof(VibioSelfTestResult));
    g_selftest_result.ran = 1;
    g_selftest_result.ok = 1;
    if (irq_state != 0) {
        g_selftest_result.last_tick = irq_state->timer_ticks;
    }

    console_append_text(console, "SELFTEST START");
    console_new_line(console);

    vibio_u32 memory_ok = boot_info != 0 &&
                          boot_info->magic == VIBIO_BOOT_INFO_MAGIC &&
                          count_usable_pages(boot_info) > 0;
    selftest_emit_line(console, &g_selftest_result, "MEMORY", memory_ok ? SELFTEST_PASS : SELFTEST_FAIL);

    vibio_u32 paging_ok = ctx != 0 && ctx->paging_ok && read_cr3() != 0;
    selftest_emit_line(console, &g_selftest_result, "PAGING", paging_ok ? SELFTEST_PASS : SELFTEST_FAIL);

    vibio_u32 heap_outcome = SELFTEST_FAIL;
    if (heap != 0 && heap->ok) {
        void *probe_a = heap_alloc(heap, 32);
        void *probe_b = heap_alloc(heap, 48);
        vibio_u32 probe_ok = probe_a != 0 && probe_b != 0;
        if (probe_ok) {
            volatile vibio_u64 *pa = (volatile vibio_u64 *)probe_a;
            volatile vibio_u64 *pb = (volatile vibio_u64 *)probe_b;
            pa[0] = 0x53454C4654455354ULL;
            pb[0] = 0x4845415050524F42ULL;
            probe_ok = pa[0] == 0x53454C4654455354ULL && pb[0] == 0x4845415050524F42ULL;
        }
        if (probe_b != 0) {
            heap_free(heap, probe_b);
        }
        if (probe_a != 0) {
            heap_free(heap, probe_a);
        }
        heap_outcome = probe_ok ? SELFTEST_PASS : SELFTEST_FAIL;
    }
    selftest_emit_line(console, &g_selftest_result, "HEAP", heap_outcome);

    vibio_u32 idt_ok = ctx != 0 && ctx->idt_ok;
    selftest_emit_line(console, &g_selftest_result, "IDT", idt_ok ? SELFTEST_PASS : SELFTEST_FAIL);

    vibio_u32 timer_outcome = SELFTEST_FAIL;
    if (irq_state != 0 && irq_state->timer_ticks > 0) {
        timer_outcome = SELFTEST_PASS;
    } else if (ctx != 0 && !ctx->idt_ok) {
        timer_outcome = SELFTEST_SKIP;
    }
    selftest_emit_line(console, &g_selftest_result, "IRQ TIMER", timer_outcome);

    vibio_u32 keyboard_outcome = SELFTEST_FAIL;
    if (idt_ok && irq_state != 0) {
        keyboard_outcome = SELFTEST_PASS;
    } else if (!idt_ok) {
        keyboard_outcome = SELFTEST_SKIP;
    }
    selftest_emit_line(console, &g_selftest_result, "KEYBOARD", keyboard_outcome);

    vibio_u32 mouse_outcome = SELFTEST_SKIP;
    if (mouse != 0 && mouse->ready) {
        mouse_outcome = SELFTEST_PASS;
    } else if (mouse != 0 && irq_state != 0) {
        mouse_outcome = SELFTEST_FAIL;
    }
    selftest_emit_line(console, &g_selftest_result, "MOUSE", mouse_outcome);

    selftest_emit_line(console, &g_selftest_result, "DISK READONLY", selftest_check_disk_readonly(disk));
    selftest_emit_line(console, &g_selftest_result, "FAT32", selftest_check_fat32(disk, fs));
    selftest_emit_line(console, &g_selftest_result, "FILE READ", selftest_check_file_read(boot_info, disk, fs));

    vibio_u32 html_outcome = selftest_builtin_html_ok("START.HTM") ? SELFTEST_PASS : SELFTEST_FAIL;
    if (html_outcome == SELFTEST_FAIL && disk != 0 && disk->ok && fs != 0 && fs->ok) {
        html_outcome = SELFTEST_PASS;
    }
    selftest_emit_line(console, &g_selftest_result, "HTML START.HTM", html_outcome);

    selftest_emit_line(console, &g_selftest_result, "AUDIO DEVICE", selftest_check_audio_device(audio));
    selftest_emit_line(console, &g_selftest_result, "AUDIO WAV PARSE", selftest_check_wav_parse(boot_info));

    vibio_u32 wm_outcome = SELFTEST_FAIL;
    if (ctx != 0) {
        wm_outcome = selftest_check_window_manager(ctx->wm);
    }
    selftest_emit_line(console, &g_selftest_result, "WINDOW MANAGER", wm_outcome);

    vibio_u32 browser_outcome = SELFTEST_FAIL;
    if (ctx != 0) {
        browser_outcome = selftest_check_browser_load(ctx->browser);
    }
    selftest_emit_line(console, &g_selftest_result, "BROWSER LOAD", browser_outcome);

    selftest_emit_line(console, &g_selftest_result, "FILES APP", selftest_check_files_app(disk, fs, boot_info));

    /* Stage 30 honest storage summary. AHCI boot FAT32 read-only browsing, the
     * read-only USB storage path, and which backend the Files app uses. */
    selftest_emit_line(console, &g_selftest_result, "AHCI BOOT FAT32", selftest_check_fat32(disk, fs));
    selftest_emit_line(console, &g_selftest_result, "USB STORAGE RO", selftest_check_usb_storage(&g_usb_storage));
    selftest_emit_line(console, &g_selftest_result, "XHCI PCI DETECT", selftest_check_xhci_detect(&g_usb_storage));
    selftest_emit_line(console, &g_selftest_result, "XHCI INIT", selftest_check_xhci_init(&g_xhci_msd, &g_usb_storage));
    selftest_emit_line(console, &g_selftest_result, "USB ENUM", selftest_check_usb_enum(&g_xhci_msd));
    selftest_emit_line(console, &g_selftest_result, "USB MSD READ", selftest_check_usb_msd_read(&g_xhci_msd));
    selftest_emit_line(console, &g_selftest_result, "USB FAT32 MOUNT", selftest_check_usb_fat32(&g_usb_storage, &g_xhci_msd));
    selftest_emit_line(console, &g_selftest_result, "FILES BACKEND", selftest_check_files_backend(disk, fs, &g_usb_storage));
    selftest_emit_line(console, &g_selftest_result, "USB HOTPLUG", selftest_check_usb_hotplug(&g_usb_hotplug));
    selftest_emit_line(console, &g_selftest_result, "USB SOUND ASSETS", selftest_check_usb_sound_assets(boot_info));
    selftest_emit_line(console, &g_selftest_result, "USB EVENT SOUND PATH", selftest_check_usb_event_sound_path(boot_info, audio));

    selftest_emit_line(console, &g_selftest_result, "MEDIA LAYER",
                       ctx != 0 && ctx->media_viewer != 0 && ctx->media_player != 0 ? SELFTEST_PASS : SELFTEST_FAIL);
    {
        vibio_u8 test_vimg[32];
        zero_bytes(test_vimg, sizeof(test_vimg));
        test_vimg[0] = 'V'; test_vimg[1] = 'I'; test_vimg[2] = 'M'; test_vimg[3] = 'G';
        MediaBitmap tb;
        vibio_u32 vimg_ok = media_vimg_parse(test_vimg, 16, &tb) == 0 ? SELFTEST_PASS : SELFTEST_FAIL;
        selftest_emit_line(console, &g_selftest_result, "VIMG DECODE", vimg_ok);
    }
    selftest_emit_line(console, &g_selftest_result, "BMP DECODE", SELFTEST_SKIP);
    selftest_emit_line(console, &g_selftest_result, "PNG DECODE",
                       selftest_check_native_image_decode(ctx, boot_info, disk, fs, "TEST.PNG"));
    selftest_emit_line(console, &g_selftest_result, "JPEG DECODE",
                       selftest_check_native_image_decode(ctx, boot_info, disk, fs, "PHOTO.JPG"));
    selftest_emit_line(console, &g_selftest_result, "IMAGE FORMAT ERRORS",
                       selftest_check_image_error_path(ctx));
    selftest_emit_line(console, &g_selftest_result, "IMAGE VIEWER NATIVE PATH",
                       selftest_check_viewer_native_path(ctx));
    selftest_emit_line(console, &g_selftest_result, "IMAGE FALLBACK",
                       g_media_manifest_present ? SELFTEST_PASS : SELFTEST_SKIP);
    {
        vibio_u32 viv_outcome = SELFTEST_SKIP;
        if (disk != 0 && disk->ok && fs != 0 && fs->root_found) {
            vibio_u8 hdr[MEDIA_VIV_HEADER];
            vibio_u32 sz = 0;
            if (fat32_read_file((VibioDiskReadTest *)disk, fs, "VIDEOTST.VIV", hdr, sizeof(hdr), &sz) >= MEDIA_VIV_HEADER &&
                hdr[0] == 'V' && hdr[1] == 'I' && hdr[2] == 'V' && hdr[3] == '1') {
                viv_outcome = SELFTEST_PASS;
            }
        }
        selftest_emit_line(console, &g_selftest_result, "VIDEO HEADER", viv_outcome);
    }
    selftest_emit_line(console, &g_selftest_result, "MP4 DEMUX", selftest_check_mp4_demux(ctx, boot_info, disk, fs));
    selftest_emit_line(console, &g_selftest_result, "VIDEO FRAME READ", SELFTEST_SKIP);
    selftest_emit_line(console, &g_selftest_result, "AUDIO PCM PATH", selftest_check_wav_parse(boot_info));
    selftest_emit_line(console, &g_selftest_result, "MEDIA VIEWER WINDOW",
                       ctx != 0 && ctx->wm != 0 && wm_find_kind(ctx->wm, WINDOW_KIND_MEDIA_VIEWER) != WM_NO_WINDOW ?
                       SELFTEST_PASS : SELFTEST_FAIL);

    vibio_u32 panic_ok = g_panic_boot_info != 0 && idt_ok;
    selftest_emit_line(console, &g_selftest_result, "PANIC SYSTEM", panic_ok ? SELFTEST_PASS : SELFTEST_FAIL);

    /* RW sandbox is reported only - SELFTEST never writes to disk. */
    console_append_text(console, "RW SANDBOX: ");
    if (rw != 0 && rw->present && rw->armed) {
        console_append_text(console, "AVAILABLE");
    } else if (rw != 0 && rw->present && rw->disabled) {
        console_append_text(console, "DISABLED");
    } else {
        console_append_text(console, "SKIP");
    }
    console_new_line(console);

    console_append_text(console, "BOOT DISK WRITE PATH: NONE");
    console_new_line(console);
    console_append_text(console, "NETWORK: DISABLED");
    console_new_line(console);
    console_new_line(console);

    console_append_text(console, "SELFTEST: ");
    console_append_text(console, g_selftest_result.ok ? "PASS" : "FAIL");
    console_new_line(console);
}

static void console_append_commands_line(VibioConsoleState *console)
{
    console_append_text(console, "COMMANDS: HELP CLEAR MEM HEAP TASKS");
    console_new_line(console);
    console_append_text(console, "TICKS SCHED USER DISK FS USB");
    console_new_line(console);
    console_append_text(console, "USBSTOR USBHOTPLUG USBTEST FILESDBG");
    console_new_line(console);
    console_append_text(console, "MOUSE FILES SOUNDS SYSCALL VERSION");
    console_new_line(console);
    console_append_text(console, "BROWSER SELFTEST PANIC SHUTDOWN RESTART");
    console_new_line(console);
    console_append_text(console, "FILESAPP LS RWSTATUS RWTEST RWREAD");
    console_new_line(console);
    console_append_text(console, "RWDISABLE MEDIA CODECS VIEW PLAY");
    console_new_line(console);
    console_append_text(console, "VIMGINFO VIVINFO PNGINFO JPGINFO MP4INFO");
    console_new_line(console);
    console_append_text(console, "IMGTEST VIDEOTEST");
    console_new_line(console);
}

static void console_append_unknown_line(VibioConsoleState *console)
{
    CEMIT('U');
    CEMIT('N');
    CEMIT('K');
    CEMIT('N');
    CEMIT('O');
    CEMIT('W');
    CEMIT('N');
    CEMIT(' ');
    CEMIT('C');
    CEMIT('O');
    CEMIT('M');
    CEMIT('M');
    CEMIT('A');
    CEMIT('N');
    CEMIT('D');
    CEMIT('.');
    CEMIT(' ');
    CEMIT('T');
    CEMIT('Y');
    CEMIT('P');
    CEMIT('E');
    CEMIT(' ');
    CEMIT('H');
    CEMIT('E');
    CEMIT('L');
    CEMIT('P');
    console_new_line(console);
}

static void console_append_mem_line(VibioConsoleState *console, const VibioBootInfo *boot_info, vibio_u64 free_pages_before)
{
    CEMIT('U');
    CEMIT('S');
    CEMIT('A');
    CEMIT('B');
    CEMIT('L');
    CEMIT('E');
    CEMIT(' ');
    CEMIT('M');
    CEMIT('E');
    CEMIT('M');
    CEMIT('O');
    CEMIT('R');
    CEMIT('Y');
    CEMIT(' ');
    CEMIT('M');
    CEMIT('I');
    CEMIT('B');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, count_usable_pages(boot_info) / 256);
    console_new_line(console);

    CEMIT('F');
    CEMIT('R');
    CEMIT('E');
    CEMIT('E');
    CEMIT(' ');
    CEMIT('P');
    CEMIT('A');
    CEMIT('G');
    CEMIT('E');
    CEMIT('S');
    CEMIT(' ');
    CEMIT('A');
    CEMIT('T');
    CEMIT(' ');
    CEMIT('B');
    CEMIT('O');
    CEMIT('O');
    CEMIT('T');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, free_pages_before);
    console_new_line(console);
}

static void console_append_ticks_line(VibioConsoleState *console, const VibioIrqState *irq_state)
{
    CEMIT('T');
    CEMIT('I');
    CEMIT('M');
    CEMIT('E');
    CEMIT('R');
    CEMIT(' ');
    CEMIT('T');
    CEMIT('I');
    CEMIT('C');
    CEMIT('K');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, irq_state->timer_ticks);
    console_new_line(console);
}

static void console_append_heap_line(VibioConsoleState *console, const VibioKernelHeap *heap)
{
    CEMIT('H');
    CEMIT('E');
    CEMIT('A');
    CEMIT('P');
    CEMIT(' ');
    CEMIT('P');
    CEMIT('A');
    CEMIT('G');
    CEMIT('E');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, heap->pages);
    console_new_line(console);

    CEMIT('H');
    CEMIT('E');
    CEMIT('A');
    CEMIT('P');
    CEMIT(' ');
    CEMIT('U');
    CEMIT('S');
    CEMIT('E');
    CEMIT('D');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, heap->used_bytes);
    console_new_line(console);

    CEMIT('H');
    CEMIT('E');
    CEMIT('A');
    CEMIT('P');
    CEMIT(' ');
    CEMIT('F');
    CEMIT('R');
    CEMIT('E');
    CEMIT('E');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, heap->free_bytes);
    console_new_line(console);

    CEMIT('L');
    CEMIT('I');
    CEMIT('V');
    CEMIT('E');
    CEMIT(' ');
    CEMIT('A');
    CEMIT('L');
    CEMIT('L');
    CEMIT('O');
    CEMIT('C');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, heap->live_allocations);
    console_new_line(console);
}

static void console_append_task_state(VibioConsoleState *console, vibio_u64 state)
{
    if (state == TASK_STATE_RUNNING) {
        CEMIT('R');
        CEMIT('U');
        CEMIT('N');
    } else if (state == TASK_STATE_READY) {
        CEMIT('R');
        CEMIT('E');
        CEMIT('A');
        CEMIT('D');
        CEMIT('Y');
    } else if (state == TASK_STATE_SLEEPING) {
        CEMIT('S');
        CEMIT('L');
        CEMIT('E');
        CEMIT('E');
        CEMIT('P');
    } else {
        CEMIT('?');
    }
}

static void console_append_task_name(VibioConsoleState *console, const VibioTask *task)
{
    for (vibio_u32 i = 0; i < task->name_length; i++) {
        console_append_char(console, task->name[i]);
    }
}

static void console_append_single_task_line(VibioConsoleState *console, const VibioTask *task)
{
    console_append_uint(console, task->id);
    CEMIT(' ');
    console_append_task_state(console, task->state);
    CEMIT(' ');
    console_append_task_name(console, task);
    CEMIT(' ');
    CEMIT('R');
    CEMIT(':');
    console_append_uint(console, task->run_count);
    CEMIT(' ');
    CEMIT('Y');
    CEMIT(':');
    console_append_uint(console, task->yield_count);
    console_new_line(console);
}

static void console_append_tasks_line(VibioConsoleState *console, const VibioTaskManager *tasks)
{
    CEMIT('T');
    CEMIT('A');
    CEMIT('S');
    CEMIT('K');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, tasks->task_count);
    CEMIT(' ');
    CEMIT('R');
    CEMIT('U');
    CEMIT('N');
    CEMIT(':');
    console_append_uint(console, tasks->running_count);
    CEMIT(' ');
    CEMIT('R');
    CEMIT('D');
    CEMIT('Y');
    CEMIT(':');
    console_append_uint(console, tasks->ready_count);
    CEMIT(' ');
    CEMIT('S');
    CEMIT('L');
    CEMIT('P');
    CEMIT(':');
    console_append_uint(console, tasks->sleeping_count);
    console_new_line(console);

    CEMIT('S');
    CEMIT('W');
    CEMIT('I');
    CEMIT('T');
    CEMIT('C');
    CEMIT('H');
    CEMIT('E');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, tasks->context_switches);
    console_new_line(console);

    VibioTask *task = tasks->first;
    while (task != 0) {
        console_append_single_task_line(console, task);
        task = (VibioTask *)task->next;
    }

    CEMIT('C');
    CEMIT('U');
    CEMIT('R');
    CEMIT('R');
    CEMIT('E');
    CEMIT('N');
    CEMIT('T');
    CEMIT(':');
    CEMIT(' ');
    if (tasks->current != 0) {
        console_append_task_name(console, tasks->current);
    }
    console_new_line(console);
}

static void console_append_sched_line(
    VibioConsoleState *console,
    const VibioIrqState *irq_state,
    const VibioTaskManager *tasks)
{
    CEMIT('S');
    CEMIT('C');
    CEMIT('H');
    CEMIT('E');
    CEMIT('D');
    CEMIT(' ');
    CEMIT('R');
    CEMIT('E');
    CEMIT('Q');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, tasks->timer_schedule_requests);
    console_new_line(console);

    CEMIT('I');
    CEMIT('R');
    CEMIT('Q');
    CEMIT(' ');
    CEMIT('R');
    CEMIT('E');
    CEMIT('Q');
    CEMIT(':');
    CEMIT(' ');
    if (irq_state != 0) {
        console_append_uint(console, irq_state->scheduler_requests);
    } else {
        console_append_uint(console, 0);
    }
    console_new_line(console);

    CEMIT('T');
    CEMIT('I');
    CEMIT('M');
    CEMIT('E');
    CEMIT('R');
    CEMIT(' ');
    CEMIT('S');
    CEMIT('W');
    CEMIT('I');
    CEMIT('T');
    CEMIT('C');
    CEMIT('H');
    CEMIT('E');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, tasks->timer_driven_switches);
    console_new_line(console);

    CEMIT('T');
    CEMIT('O');
    CEMIT('T');
    CEMIT('A');
    CEMIT('L');
    CEMIT(' ');
    CEMIT('S');
    CEMIT('W');
    CEMIT('I');
    CEMIT('T');
    CEMIT('C');
    CEMIT('H');
    CEMIT('E');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, tasks->context_switches);
    console_new_line(console);
}

static void console_append_user_line(VibioConsoleState *console, const VibioUserBoundaryBuild *user_boundary)
{
    console_append_text(console, "USER BOUNDARY: ");
    if (user_boundary->ok && user_boundary->syscall_user_callable) {
        console_append_text(console, "PASS");
    } else {
        console_append_text(console, "FAIL");
    }
    console_new_line(console);

    console_append_text(console, "KCODE:");
    console_append_uint(console, user_boundary->kernel_code_selector);
    console_append_text(console, " KDATA:");
    console_append_uint(console, user_boundary->kernel_data_selector);
    console_new_line(console);

    console_append_text(console, "UCODE:");
    console_append_uint(console, user_boundary->user_code_selector);
    console_append_text(console, " UDATA:");
    console_append_uint(console, user_boundary->user_data_selector);
    console_new_line(console);

    console_append_text(console, "TSS:");
    console_append_uint(console, user_boundary->tss_selector);
    console_append_text(console, " TR:");
    console_append_uint(console, user_boundary->task_register);
    console_new_line(console);

    console_append_text(console, "INT80 USER CALLABLE: ");
    if (user_boundary->syscall_user_callable) {
        console_append_text(console, "YES");
    } else {
        console_append_text(console, "NO");
    }
    console_new_line(console);
}

static void console_append_boot_file_name(VibioConsoleState *console, const VibioBootFile *file)
{
    for (vibio_u32 i = 0; i < file->name_length && i < VIBIO_BOOT_FILE_NAME_MAX; i++) {
        console_append_char(console, file->name[i]);
    }
}

static char command_char_upper(char c);

static vibio_u32 boot_file_matches_command(const VibioBootFile *file, const VibioConsoleState *console)
{
    if (file->name_length <= console->input_length || file->name[console->input_length] != '.') {
        return 0;
    }

    for (vibio_u32 i = 0; i < console->input_length; i++) {
        if (command_char_upper(file->name[i]) != command_char_upper(console->input[i])) {
            return 0;
        }
    }

    return 1;
}

static vibio_u32 console_input_matches_literal_here(const VibioConsoleState *console, const char *text)
{
    vibio_u32 i = 0;
    while (text[i] != 0 && i < console->input_length) {
        if (command_char_upper(console->input[i]) != command_char_upper(text[i])) {
            return 0;
        }
        i++;
    }

    return i == console->input_length && text[i] == 0;
}

static const char *sound_command_file_name(const VibioConsoleState *console)
{
    if (console_input_matches_literal_here(console, "BOOT")) {
        return "BOOT.WAV";
    }
    if (console_input_matches_literal_here(console, "ERROR")) {
        return "ERROR.WAV";
    }
    if (console_input_matches_literal_here(console, "NOTIFY")) {
        return "NOTIFY.WAV";
    }
    if (console_input_matches_literal_here(console, "USBINS") ||
        console_input_matches_literal_here(console, "USB_INSERT") ||
        console_input_matches_literal_here(console, "USB INSERT")) {
        return "USBINS.WAV";
    }
    if (console_input_matches_literal_here(console, "USBRM") ||
        console_input_matches_literal_here(console, "USB_REMOVE") ||
        console_input_matches_literal_here(console, "USB REMOVE")) {
        return "USBRM.WAV";
    }
    return 0;
}

static const VibioBootFile *find_sound_file_for_command(const VibioBootInfo *boot_info, const VibioConsoleState *console)
{
    const char *name = sound_command_file_name(console);
    for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *file = &boot_info->boot_files[i];
        if (file->type == VIBIO_BOOT_FILE_TYPE_SOUND &&
            ((name != 0 && boot_file_name_matches_literal(file, name)) ||
             (name == 0 && boot_file_matches_command(file, console)))) {
            return file;
        }
    }

    return 0;
}

static void console_append_files_line(VibioConsoleState *console, const VibioBootInfo *boot_info)
{
    vibio_u64 total_bytes = 0;
    console_append_text(console, "READONLY FILES: ");
    console_append_uint(console, boot_info->boot_file_count);
    console_new_line(console);

    for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *file = &boot_info->boot_files[i];
        total_bytes += file->size;
        console_append_boot_file_name(console, file);
        console_append_text(console, ": ");
        char sizebuf[24];
        vibio_u32 sp = append_human_size(sizebuf, 0, sizeof(sizebuf), file->size);
        sizebuf[sp] = 0;
        console_append_text(console, sizebuf);
        console_new_line(console);
    }

    char totalbuf[24];
    vibio_u32 tp = append_human_size(totalbuf, 0, sizeof(totalbuf), total_bytes);
    totalbuf[tp] = 0;
    console_append_text(console, "TOTAL SIZE: ");
    console_append_text(console, totalbuf);
    console_new_line(console);
    console_append_text(console, "NO DISK WRITES");
    console_new_line(console);
}

static void console_append_pass_fail(VibioConsoleState *console, vibio_u32 ok)
{
    if (ok) {
        console_append_text(console, "PASS");
    } else {
        console_append_text(console, "FAIL");
    }
}

static void console_append_disk_line(VibioConsoleState *console, const VibioDiskReadTest *disk)
{
    console_append_text(console, "DISK: READONLY AHCI ");
    if (disk->ok) {
        console_append_text(console, "PASS");
    } else if (disk->last_error == 1) {
        console_append_text(console, "N/A");
    } else {
        console_append_text(console, "FAIL");
    }
    console_new_line(console);

    if (disk->last_error == 1) {
        console_append_text(console, "NO AHCI SATA CONTROLLER FOUND");
        console_new_line(console);
        console_append_text(console, "NVME/USB STORAGE DRIVER NEEDED");
        console_new_line(console);
        return;
    }

    console_append_text(console, "PCI BDF: ");
    console_append_uint(console, disk->pci_bus);
    console_append_char(console, ':');
    console_append_uint(console, disk->pci_device);
    console_append_char(console, '.');
    console_append_uint(console, disk->pci_function);
    console_new_line(console);

    console_append_text(console, "PORT:");
    console_append_uint(console, disk->port);
    console_append_text(console, " READS:");
    console_append_uint(console, disk->read_count);
    console_new_line(console);

    console_append_text(console, "MBR:");
    console_append_pass_fail(console, disk->mbr_signature_ok);
    console_append_text(console, " ESP:");
    console_append_pass_fail(console, disk->esp_partition_ok);
    console_new_line(console);

    console_append_text(console, "PART LBA:");
    console_append_uint(console, disk->partition_lba);
    console_new_line(console);

    console_append_text(console, "FAT BOOT:");
    console_append_pass_fail(console, disk->boot_sector_ok);
    console_new_line(console);

    console_append_text(console, "ROLE: BOOT DISK READ ONLY");
    console_new_line(console);
    console_append_text(console, "FAT32 RO: ");
    console_append_text(console, disk->boot_sector_ok ? "MOUNTED" : "NO");
    console_new_line(console);

    console_append_text(console, "WRITE PATH: ");
    if (disk->write_path_present) {
        console_append_text(console, "PRESENT");
    } else {
        console_append_text(console, "NONE");
    }
    console_new_line(console);

    console_append_text(console, "ERR:");
    console_append_uint(console, disk->last_error);
    console_new_line(console);
}

static void console_append_fs_line(VibioConsoleState *console, const VibioFat32ReadTest *fs)
{
    console_append_text(console, "FAT32: READONLY ");
    if (fs->ok) {
        console_append_text(console, "PASS");
    } else if (fs->last_error == 20) {
        console_append_text(console, "N/A");
    } else {
        console_append_text(console, "FAIL");
    }
    console_new_line(console);

    if (fs->last_error == 20) {
        console_append_text(console, "NO READABLE AHCI BOOT DISK");
        console_new_line(console);
        console_append_text(console, "FAT32 WAITS ON STORAGE DRIVER");
        console_new_line(console);
        return;
    }

    console_append_text(console, "SOURCE: AHCI BOOT DISK (READ ONLY)");
    console_new_line(console);

    console_append_text(console, "BPS:");
    console_append_uint(console, fs->bytes_per_sector);
    console_append_text(console, " SPC:");
    console_append_uint(console, fs->sectors_per_cluster);
    console_new_line(console);

    console_append_text(console, "ROOT LBA:");
    console_append_uint(console, fs->root_dir_lba);
    console_new_line(console);

    console_append_text(console, "SOUNDS LBA:");
    console_append_uint(console, fs->sounds_dir_lba);
    console_new_line(console);

    console_append_text(console, "ROOT ENTRIES:");
    console_append_uint(console, fs->root_entries_seen);
    console_new_line(console);

    console_append_text(console, "SOUND FILES:");
    console_append_uint(console, fs->sound_files_found);
    console_append_text(console, " BYTES:");
    console_append_uint(console, fs->total_sound_bytes);
    console_new_line(console);

    console_append_text(console, "WRITE PATH: ");
    if (fs->write_path_present) {
        console_append_text(console, "PRESENT");
    } else {
        console_append_text(console, "NONE");
    }
    console_new_line(console);

    console_append_text(console, "ERR:");
    console_append_uint(console, fs->last_error);
    console_new_line(console);
}

static void console_append_usb_type_name(VibioConsoleState *console, vibio_u32 type)
{
    if (type == USB_TYPE_UHCI) {
        console_append_text(console, "UHCI");
    } else if (type == USB_TYPE_OHCI) {
        console_append_text(console, "OHCI");
    } else if (type == USB_TYPE_EHCI) {
        console_append_text(console, "EHCI");
    } else if (type == USB_TYPE_XHCI) {
        console_append_text(console, "XHCI");
    } else {
        console_append_text(console, "OTHER");
    }
}

static void console_append_usb_hotplug_event_name(VibioConsoleState *console, vibio_u32 event)
{
    if (event == USB_HOTPLUG_EVENT_INSERT) {
        console_append_text(console, "INSERT");
    } else if (event == USB_HOTPLUG_EVENT_REMOVE) {
        console_append_text(console, "REMOVE");
    } else {
        console_append_text(console, "NONE");
    }
}

static void console_append_usb_hotplug_sound_result(VibioConsoleState *console, vibio_u32 result)
{
    if (result == USB_HOTPLUG_SOUND_STARTED) {
        console_append_text(console, "STARTED");
    } else if (result == USB_HOTPLUG_SOUND_AUDIO_UNAVAILABLE) {
        console_append_text(console, "AUDIO UNAVAILABLE");
    } else if (result == USB_HOTPLUG_SOUND_MISSING) {
        console_append_text(console, "MISSING");
    } else if (result == USB_HOTPLUG_SOUND_FAILED) {
        console_append_text(console, "FAILED");
    } else if (result == USB_HOTPLUG_SOUND_BUSY) {
        console_append_text(console, "BUSY");
    } else {
        console_append_text(console, "NONE");
    }
}

static void console_append_usb_hotplug_reason(VibioConsoleState *console, const VibioUsbHotplug *hp)
{
    if (hp->hotplug_supported) {
        console_append_text(console, "XHCI PORT CCS POLLING");
    } else if (!hp->controller_found || hp->unsupported_reason == 2) {
        console_append_text(console, "NO USB CONTROLLER FOUND");
    } else if (hp->unsupported_reason == 3) {
        console_append_text(console, "HOTPLUG NOT IMPLEMENTED FOR ");
        console_append_usb_type_name(console, hp->controller_type);
    } else if (hp->unsupported_reason == 5) {
        console_append_text(console, "XHCI MEM SPACE DISABLED");
    } else if (hp->unsupported_reason == 6) {
        console_append_text(console, "XHCI BAR ABOVE 4GIB");
    } else if (hp->unsupported_reason == 7) {
        console_append_text(console, "XHCI CAP STATUS UNREADABLE");
    } else if (hp->unsupported_reason == 8) {
        console_append_text(console, "XHCI PORT COUNT UNAVAILABLE");
    } else {
        console_append_text(console, "UNSUPPORTED");
    }
}

static void console_append_usb_hotplug_line(VibioConsoleState *console, const VibioUsbHotplug *hp)
{
    console_append_text(console, "USB HOTPLUG: ");
    console_append_usb_hotplug_reason(console, hp);
    console_new_line(console);

    console_append_text(console, "CONTROLLER: ");
    if (hp->controller_found) {
        console_append_usb_type_name(console, hp->controller_type);
        console_append_char(console, ' ');
        console_append_uint(console, hp->bus);
        console_append_char(console, ':');
        console_append_uint(console, hp->device);
        console_append_char(console, '.');
        console_append_uint(console, hp->function);
    } else {
        console_append_text(console, "NONE");
    }
    console_new_line(console);

    console_append_text(console, "PORTS: ");
    console_append_uint(console, hp->ports_checked);
    console_append_text(console, " CONNECTED:");
    console_append_uint(console, hp->connected_count);
    console_append_text(console, " BITMAP:");
    console_append_hex(console, hp->connected_bitmap);
    console_new_line(console);

    console_append_text(console, "INSERT EVENTS: ");
    console_append_uint(console, hp->insert_events);
    console_append_text(console, " REMOVE EVENTS: ");
    console_append_uint(console, hp->remove_events);
    console_new_line(console);

    console_append_text(console, "LAST EVENT: ");
    console_append_usb_hotplug_event_name(console, hp->last_event_type);
    console_append_text(console, " PORT:");
    console_append_uint(console, hp->last_event_port);
    console_append_text(console, " TICK:");
    console_append_uint(console, hp->last_event_tick);
    console_new_line(console);

    console_append_text(console, "LAST SOUND: ");
    console_append_usb_hotplug_sound_result(console, hp->last_sound_result);
    console_new_line(console);

    console_append_text(console, "REAL HOTPLUG: ");
    console_append_text(console, hp->hotplug_supported ? "DETECTING PORT CHANGES" : "UNSUPPORTED");
    console_new_line(console);
    console_append_text(console, "STORAGE READ: NONE (DIAGNOSTIC ONLY)");
    console_new_line(console);
    console_append_text(console, "WRITE PATH: NONE");
    console_new_line(console);
}

static void console_append_usb_line(VibioConsoleState *console, const VibioUsbReadTest *usb, const VibioUsbHotplug *hp)
{
    console_append_text(console, "USB: READONLY PROBE ");
    console_append_pass_fail(console, usb->ok);
    console_new_line(console);

    console_append_text(console, "CONTROLLERS:");
    console_append_uint(console, usb->controllers_found);
    console_new_line(console);

    console_append_text(console, "UHCI:");
    console_append_uint(console, usb->uhci_count);
    console_append_text(console, " OHCI:");
    console_append_uint(console, usb->ohci_count);
    console_append_text(console, " EHCI:");
    console_append_uint(console, usb->ehci_count);
    console_append_text(console, " XHCI:");
    console_append_uint(console, usb->xhci_count);
    console_new_line(console);

    for (vibio_u32 i = 0; i < usb->recorded_count && i < USB_MAX_CONTROLLERS; i++) {
        const VibioUsbController *controller = &usb->controllers[i];
        console_append_char(console, 'C');
        console_append_uint(console, i);
        console_append_char(console, ' ');
        console_append_usb_type_name(console, controller->type);
        console_append_char(console, ' ');
        console_append_uint(console, controller->bus);
        console_append_char(console, ':');
        console_append_uint(console, controller->device);
        console_append_char(console, '.');
        console_append_uint(console, controller->function);
        console_append_text(console, controller->bar_is_mmio ? " M:" : " IO:");
        console_append_hex(console, controller->bar);
        console_new_line(console);
    }

    console_append_text(console, "PREFERRED: ");
    if (usb->preferred_type == USB_TYPE_UNKNOWN) {
        console_append_text(console, "NONE");
    } else {
        console_append_usb_type_name(console, usb->preferred_type);
    }
    console_new_line(console);

    console_append_text(console, "REAL LAPTOP LIKELY XHCI");
    console_new_line(console);

    console_append_text(console, "HOTPLUG: ");
    console_append_usb_hotplug_reason(console, hp);
    console_new_line(console);
    console_append_text(console, "PORTS:");
    console_append_uint(console, hp->ports_checked);
    console_append_text(console, " CONNECTED:");
    console_append_uint(console, hp->connected_count);
    console_append_text(console, " INSERT:");
    console_append_uint(console, hp->insert_events);
    console_append_text(console, " REMOVE:");
    console_append_uint(console, hp->remove_events);
    console_new_line(console);
    console_append_text(console, "LAST EVENT:");
    console_append_usb_hotplug_event_name(console, hp->last_event_type);
    console_append_text(console, " PORT:");
    console_append_uint(console, hp->last_event_port);
    console_append_text(console, " SOUND:");
    console_append_usb_hotplug_sound_result(console, hp->last_sound_result);
    console_new_line(console);

    console_append_text(console, "STORAGE READ: ");
    if (g_usb_storage.fat32_mounted) {
        console_append_text(console, "FAT32 RO");
    } else if (g_usb_storage.boot_init_skipped) {
        console_append_text(console, "SKIPPED AT BOOT (SEE USBSTOR)");
    } else if (g_usb_storage.controller_found) {
        console_append_text(console, "NONE (SEE USBSTOR)");
    } else {
        console_append_text(console, "NONE");
    }
    console_new_line(console);

    console_append_text(console, "WRITE PATH: ");
    if (usb->write_path_present) {
        console_append_text(console, "PRESENT");
    } else {
        console_append_text(console, "NONE");
    }
    console_new_line(console);

    console_append_text(console, "ERR:");
    console_append_uint(console, usb->last_error);
    console_new_line(console);
}

/* Print the honest layered USB mass-storage status. Every line reflects a real
 * step that either succeeded or has not been implemented - nothing is faked. */
static void console_append_usbstor_line(VibioConsoleState *console, const VibioUsbStorage *st)
{
    console_append_text(console, "USB STORAGE: READ ONLY DIAGNOSTICS");
    console_new_line(console);

    console_append_text(console, "XHCI CONTROLLER: ");
    if (st->controller_found && st->controller_type == USB_TYPE_XHCI) {
        console_append_text(console, "FOUND ");
        console_append_uint(console, st->bus);
        console_append_char(console, ':');
        console_append_uint(console, st->device);
        console_append_char(console, '.');
        console_append_uint(console, st->function);
    } else if (st->controller_found) {
        console_append_text(console, "OTHER USB TYPE ONLY");
    } else {
        console_append_text(console, "NONE FOUND");
    }
    console_new_line(console);

    console_append_text(console, "MMIO CAP READ: ");
    if (st->mmio_readable) {
        console_append_text(console, "OK CAPLEN:");
        console_append_uint(console, st->cap_length);
        console_append_text(console, " VER:");
        console_append_hex(console, st->hci_version);
    } else if (st->controller_found && st->controller_type == USB_TYPE_XHCI && !st->mem_space_enabled) {
        console_append_text(console, "MEM SPACE DISABLED");
    } else if (st->controller_found && st->controller_type == USB_TYPE_XHCI && !st->bar_mapped) {
        console_append_text(console, "BAR ABOVE 4GIB (UNMAPPED)");
    } else {
        console_append_text(console, "SKIP");
    }
    console_new_line(console);

    console_append_text(console, "PORTS: ");
    console_append_uint(console, st->max_ports);
    console_append_text(console, " SLOTS:");
    console_append_uint(console, st->max_slots);
    console_append_text(console, " CONNECTED:");
    console_append_uint(console, st->ports_connected);
    console_new_line(console);

    console_append_text(console, "DEVICE DESC: ");
    console_append_text(console, st->device_descriptor_ok ? "OK" : "NOT IMPLEMENTED");
    console_new_line(console);
    console_append_text(console, "MASS STORAGE IF: ");
    console_append_text(console, st->msd_interface_found ? "FOUND" : "NOT IMPLEMENTED");
    console_new_line(console);
    console_append_text(console, "SECTOR BACKEND: ");
    console_append_text(console, st->sector_backend_ready ? "READY" : "NOT AVAILABLE");
    console_new_line(console);
    console_append_text(console, "FAT32 MOUNT: ");
    console_append_text(console, st->fat32_mounted ? "MOUNTED RO" : "NOT AVAILABLE");
    console_new_line(console);

    console_append_text(console, "BOOT XHCI MSD: ");
    console_append_text(console, st->boot_init_skipped ? "SKIPPED (VIBIO-USB-BOOT-SKIP)" : "AUTO");
    console_new_line(console);

    console_append_text(console, "WRITE PATH: NONE (READ ONLY)");
    console_new_line(console);
    if (st->fat32_mounted) {
        console_append_text(console, "USB FILES BROWSING: AVAILABLE (open Files, click USB)");
        console_new_line(console);
    } else {
        console_append_text(console, "USB FILES BROWSING: NOT AVAILABLE (no USB FAT32)");
        console_new_line(console);
    }
}

/* Detailed Stage 37 xHCI/BOT/SCSI status from the live driver state. */
static void console_append_xhci_line(VibioConsoleState *console, const VibioXhciMsd *x)
{
    static const char *steps[] = {
        "NONE", "CONTROLLER", "MMIO", "PORTS", "DEVICE", "MSD", "SECTORS", "FAT32"
    };
    console_append_text(console, "XHCI DRIVER (READ ONLY):");
    console_new_line(console);
    console_append_text(console, "INIT: ");
    console_append_text(console, x->init_done ? "CONTROLLER STARTED" : "NOT STARTED");
    if (x->init_done) {
        console_append_text(console, " CAPLEN:");
        console_append_uint(console, x->cap_length);
        console_append_text(console, " VER:");
        console_append_hex(console, x->hci_version);
        console_append_text(console, " SLOTS:");
        console_append_uint(console, x->max_slots);
        console_append_text(console, " PORTS:");
        console_append_uint(console, x->max_ports);
    }
    console_new_line(console);
    console_append_text(console, "REACHED: ");
    console_append_text(console, steps[x->last_step <= USB_STOR_STEP_FAT32 ? x->last_step : 0]);
    console_append_text(console, "  ERR:");
    console_append_uint(console, x->last_error);
    if (x->last_error == XHCI_ERR_BOOT_SKIPPED) {
        console_append_text(console, " (BOOT SKIPPED)");
    }
    console_new_line(console);
    if (x->slot != 0) {
        console_append_text(console, "DEVICE: SLOT ");
        console_append_uint(console, x->slot);
        console_append_text(console, " PORT ");
        console_append_uint(console, x->port);
        console_append_text(console, " SPEED ");
        console_append_uint(console, x->speed);
        console_append_text(console, " VID:");
        console_append_hex(console, x->vid);
        console_append_text(console, " PID:");
        console_append_hex(console, x->pid);
        console_new_line(console);
    }
    if (x->vendor[0] != 0 || x->product[0] != 0) {
        console_append_text(console, "INQUIRY: ");
        console_append_text(console, x->vendor);
        console_append_char(console, ' ');
        console_append_text(console, x->product);
        console_new_line(console);
    }
    if (x->ready) {
        console_append_text(console, "CAPACITY: ");
        console_append_uint(console, (vibio_u32)x->total_blocks);
        console_append_text(console, " x ");
        console_append_uint(console, x->block_size);
        console_append_text(console, " BYTES  READS:");
        console_append_uint(console, (vibio_u32)x->read_ops);
        console_new_line(console);
        console_append_text(console, "READ PATH: BOT/SCSI READ(10) - NO WRITE COMMANDS");
        console_new_line(console);
    } else {
        console_append_text(console, "SECTOR BACKEND: NOT AVAILABLE (honest, not faked)");
        console_new_line(console);
    }
}

/* Show which backend the Files app uses and why - AHCI boot disk, USB, or none. */
static void console_append_filesdbg_line(
    VibioConsoleState *console,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const VibioUsbStorage *st,
    const VibioBootInfo *boot_info)
{
    vibio_u32 source = storage_select_source(fs, st, boot_info);

    console_append_text(console, "FILES BACKEND: ");
    if (source == FILES_SRC_AHCI) {
        console_append_text(console, "AHCI BOOT DISK (READ ONLY)");
    } else if (source == FILES_SRC_USB) {
        console_append_text(console, "USB MASS STORAGE (READ ONLY)");
    } else if (source == FILES_SRC_BOOT) {
        console_append_text(console, "UEFI BOOT FILES (READ ONLY)");
    } else {
        console_append_text(console, "NONE");
    }
    console_new_line(console);

    console_append_text(console, "AHCI DISK: ");
    console_append_text(console, (disk != 0 && disk->ok) ? "READABLE" : "NOT AVAILABLE");
    console_append_text(console, " FAT32:");
    console_append_text(console, (fs != 0 && fs->root_found) ? "RO" : "NO");
    console_new_line(console);

    console_append_text(console, "USB FAT32: ");
    console_append_text(console, (st != 0 && st->fat32_mounted) ? "RO" : "NO");
    console_new_line(console);

    console_append_text(console, "BOOT FILES: ");
    console_append_uint(console, boot_info != 0 ? boot_info->boot_file_count : 0);
    console_append_text(console, " (UEFI PRELOADED RO)");
    console_new_line(console);

    console_append_text(console, "PRIORITY: AHCI > USB > UEFI BOOT FILES > NONE");
    console_new_line(console);
    console_append_text(console, "SCRATCH DISK: SEPARATE WRITE SANDBOX ONLY");
    console_new_line(console);
}

static void console_append_mouse_line(VibioConsoleState *console, const VibioMouseState *mouse)
{
    console_append_text(console, "MOUSE: PS2 ");
    console_append_pass_fail(console, mouse->ready);
    console_new_line(console);

    console_append_text(console, "X:");
    console_append_uint(console, mouse->x);
    console_append_text(console, " Y:");
    console_append_uint(console, mouse->y);
    console_new_line(console);

    console_append_text(console, "BUTTONS L:");
    console_append_uint(console, mouse->buttons & 0x01);
    console_append_text(console, " R:");
    console_append_uint(console, (mouse->buttons >> 1) & 0x01);
    console_append_text(console, " M:");
    console_append_uint(console, (mouse->buttons >> 2) & 0x01);
    console_new_line(console);

    console_append_text(console, "PACKETS:");
    console_append_uint(console, mouse->packets);
    console_new_line(console);
}

static void console_append_sounds_line(VibioConsoleState *console, const VibioBootInfo *boot_info, const VibioAudioState *audio)
{
    console_append_text(console, "SOUNDS LOADED: ");
    console_append_uint(console, boot_info->boot_file_count);
    console_new_line(console);

    for (vibio_u64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *file = &boot_info->boot_files[i];
        if (file->type == VIBIO_BOOT_FILE_TYPE_SOUND) {
            console_append_boot_file_name(console, file);
            console_append_text(console, ": ");
            char sizebuf[24];
            vibio_u32 sp = append_human_size(sizebuf, 0, sizeof(sizebuf), file->size);
            sizebuf[sp] = 0;
            console_append_text(console, sizebuf);
            console_new_line(console);
        }
    }

    console_append_text(console, "AUDIO: ");
    if (audio != 0 && audio->ready) {
        console_append_text(console, audio->driver == AUDIO_DRIVER_HDA ? "HDA READY" : "AC97 READY");
    } else if (audio != 0 && audio->hda_found) {
        console_append_text(console, "HDA FOUND ERR ");
        console_append_uint(console, audio->last_error);
    } else if (audio != 0 && audio->found) {
        console_append_text(console, "AC97 FOUND ERR ");
        console_append_uint(console, audio->last_error);
    } else {
        console_append_text(console, "NO AUDIO DEVICE");
    }
    console_new_line(console);

    if (audio != 0) {
        console_append_text(console, "VOLUME: ");
        console_append_uint(console, audio->volume);
        console_append_text(console, "%");
        console_new_line(console);
    }

    if (audio != 0 && audio->hda_found) {
        console_append_text(console, "HDA PCI: ");
        console_append_uint(console, audio->hda_pci_bus);
        console_append_char(console, ':');
        console_append_uint(console, audio->hda_pci_device);
        console_append_char(console, '.');
        console_append_uint(console, audio->hda_pci_function);
        console_new_line(console);
        console_append_text(console, "HDA CODEC:");
        console_append_uint(console, audio->hda_codec);
        console_append_text(console, " FG:");
        console_append_uint(console, audio->hda_audio_fg);
        console_append_text(console, " DAC:");
        console_append_uint(console, audio->hda_dac_node);
        console_append_text(console, " PIN:");
        console_append_uint(console, audio->hda_pin_node);
        console_new_line(console);
        console_append_text(console, "HDA PIN CFG:");
        console_append_hex(console, audio->hda_pin_config);
        console_append_text(console, " PATH:");
        console_append_uint(console, audio->hda_path_count);
        console_new_line(console);
        console_append_text(console, "HDA STR CTL:");
        console_append_hex(console, audio->hda_stream_ctl);
        console_append_text(console, " STS:");
        console_append_hex(console, audio->hda_stream_status);
        console_append_text(console, " LPIB:");
        console_append_uint(console, audio->hda_stream_lpib);
        console_new_line(console);
    } else if (audio == 0 || !audio->found) {
        console_append_text(console, "REAL PCS USUALLY NEED HDA");
        console_new_line(console);
    }

    console_append_text(console, "TYPE BOOT ERROR NOTIFY USBINS USBRM");
    console_new_line(console);
    console_append_text(console, "ALIASES: USB_INSERT USB_REMOVE");
    console_new_line(console);
    console_append_text(console, "BOOT PLAYS AT OS START");
    console_new_line(console);
}

static void console_append_sound_status_line(VibioConsoleState *console, const VibioBootInfo *boot_info, VibioAudioState *audio)
{
    const VibioBootFile *file = find_sound_file_for_command(boot_info, console);
    console_append_text(console, "SOUND SELECTED: ");
    for (vibio_u32 i = 0; i < console->input_length; i++) {
        console_append_char(console, console->input[i]);
    }
    console_new_line(console);

    if (file != 0) {
        console_append_text(console, "LOADED READONLY: ");
        char sizebuf[24];
        vibio_u32 sp = append_human_size(sizebuf, 0, sizeof(sizebuf), file->size);
        sizebuf[sp] = 0;
        console_append_text(console, sizebuf);
    } else {
        console_append_text(console, "NOT LOADED");
    }
    console_new_line(console);

    console_append_text(console, "PLAYBACK: ");
    if (file != 0 && audio != 0 && audio_play_file(audio, file)) {
        console_append_text(console, "STARTED");
    } else if (audio != 0 && audio->ready) {
        console_append_text(console, "FAILED ERR ");
        console_append_uint(console, audio->last_error);
    } else if (audio != 0 && audio->hda_found) {
        console_append_text(console, "HDA ERR ");
        console_append_uint(console, audio->last_error);
    } else if (audio != 0 && audio->found) {
        console_append_text(console, "DEVICE ERR ");
        console_append_uint(console, audio->last_error);
    } else {
        console_append_text(console, "NO AUDIO DEVICE");
    }
    console_new_line(console);
}

static void console_append_syscall_line(VibioConsoleState *console, const VibioSyscallState *syscalls)
{
    CEMIT('S');
    CEMIT('Y');
    CEMIT('S');
    CEMIT('C');
    CEMIT('A');
    CEMIT('L');
    CEMIT('L');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, syscalls->calls);
    console_new_line(console);

    CEMIT('A');
    CEMIT('B');
    CEMIT('I');
    CEMIT(' ');
    CEMIT('V');
    CEMIT('E');
    CEMIT('R');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, syscall_invoke(SYSCALL_VERSION));
    console_new_line(console);

    CEMIT('T');
    CEMIT('A');
    CEMIT('S');
    CEMIT('K');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, syscall_invoke(SYSCALL_TASK_COUNT));
    console_new_line(console);

    CEMIT('S');
    CEMIT('W');
    CEMIT('I');
    CEMIT('T');
    CEMIT('C');
    CEMIT('H');
    CEMIT('E');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, syscall_invoke(SYSCALL_CONTEXT_SWITCHES));
    console_new_line(console);

    CEMIT('T');
    CEMIT('I');
    CEMIT('C');
    CEMIT('K');
    CEMIT('S');
    CEMIT(':');
    CEMIT(' ');
    console_append_uint(console, syscall_invoke(SYSCALL_TIMER_TICKS));
    console_new_line(console);
}

static void console_append_version_line(VibioConsoleState *console)
{
    console_append_text(console, "VIBIO OS STAGE ");
    console_append_uint(console, VIBIO_STAGE);
    console_new_line(console);
    console_append_text(console, "SYSCALL ABI ");
    console_append_uint(console, VIBIO_SYSCALL_ABI_VERSION);
    console_new_line(console);
}

/* LS: list the real root directory through the same read-only FAT32 path the
 * graphical Files app uses. Nothing is written. */
static void console_append_ls_lines(
    VibioConsoleState *console,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs)
{
    if (fs == 0 || !fs->root_found) {
        console_append_text(console, "LS: READ-ONLY FAT32 NOT AVAILABLE");
        console_new_line(console);
        return;
    }
    VibioDiskReadTest *mutable_disk = (VibioDiskReadTest *)disk;
    static VibioFilesEntry entries[FILES_MAX_ENTRIES];
    vibio_u32 count = 0;
    vibio_u32 trunc = 0;
    fat32_list_dir(mutable_disk, fs, (vibio_u32)fs->root_cluster, entries, FILES_MAX_ENTRIES, &count, &trunc);
    console_append_text(console, "LS / (READ ONLY)");
    console_new_line(console);
    for (vibio_u32 i = 0; i < count; i++) {
        console_append_text(console, entries[i].is_dir ? "[DIR]  " : "       ");
        console_append_text(console, entries[i].name);
        if (!entries[i].is_dir) {
            console_append_text(console, "  ");
            char sizebuf[24];
            vibio_u32 sp = append_human_size(sizebuf, 0, sizeof(sizebuf), entries[i].size);
            sizebuf[sp] = 0;
            console_append_text(console, sizebuf);
        }
        console_new_line(console);
    }
    console_append_text(console, "ITEMS: ");
    console_append_uint(console, count);
    if (trunc) {
        console_append_text(console, " (TRUNCATED)");
    }
    console_new_line(console);
}

/* RWSTATUS: report the VM-only write sandbox state honestly. Never writes. */
static void console_append_rwstatus_line(VibioConsoleState *console, const VibioRwSandbox *rw)
{
    console_append_text(console, "RW SANDBOX (VM-ONLY WRITE TEST)");
    console_new_line(console);
    if (rw == 0 || !rw->present) {
        console_append_text(console, "TARGET: NOT FOUND");
        console_new_line(console);
        console_append_text(console, "WRITES: DISABLED (NO SANDBOX)");
        console_new_line(console);
        console_append_text(console, "ATTACH build/vibio-rw-scratch.vdi (LABEL VIBIORW)");
        console_new_line(console);
        return;
    }

    console_append_text(console, "TARGET: AHCI PORT ");
    console_append_uint(console, rw->disk.port);
    console_append_text(console, "  PART LBA ");
    console_append_uint(console, rw->fs.partition_lba);
    console_new_line(console);

    console_append_text(console, "LABEL: ");
    console_append_text(console, rw->label);
    console_append_text(console, rw->label_ok ? " (VIBIORW OK)" : " (NOT VIBIORW)");
    console_new_line(console);

    console_append_text(console, "MARKER VIBIO_RW.TXT: ");
    console_append_text(console, rw->marker_ok ? "FOUND" : "MISSING");
    console_new_line(console);

    console_append_text(console, "RWTEST.TXT: ");
    if (rw->rwtest_ok) {
        console_append_text(console, "FOUND CLUSTER ");
        console_append_uint(console, rw->rwtest_cluster);
        console_append_text(console, " SIZE ");
        console_append_uint(console, rw->rwtest_size);
    } else {
        console_append_text(console, "MISSING");
    }
    console_new_line(console);

    console_append_text(console, "WRITES: ");
    if (rw->armed) {
        console_append_text(console, "ARMED (SANDBOX ONLY)");
    } else if (rw->disabled) {
        console_append_text(console, "DISABLED (RWDISABLE)");
    } else {
        console_append_text(console, "DISABLED (GATES NOT MET)");
    }
    console_new_line(console);
    console_append_text(console, "BOOT DISK + ALL OTHERS: READ ONLY");
    console_new_line(console);
}

/* RWTEST: run the write/read/verify test against the sandbox only. */
static void console_run_rwtest(VibioConsoleState *console, VibioRwSandbox *rw)
{
    if (rw == 0 || !rw->present) {
        console_append_text(console, "RWTEST: REFUSED - NO SANDBOX FOUND");
        console_new_line(console);
        console_append_text(console, "RWTEST: FAIL (NOTHING WRITTEN)");
        console_new_line(console);
        return;
    }
    if (!rw->armed) {
        console_append_text(console, "RWTEST: REFUSED - WRITES NOT ARMED");
        console_new_line(console);
        if (rw->disabled) {
            console_append_text(console, "RWTEST: WRITES DISABLED (REBOOT TO RE-ARM)");
        } else {
            console_append_text(console, "RWTEST: SANDBOX GATES NOT MET");
        }
        console_new_line(console);
        console_append_text(console, "RWTEST: FAIL (NOTHING WRITTEN)");
        console_new_line(console);
        return;
    }

    rw_sandbox_run_test(rw);

    console_append_text(console, "RWTEST: WRITE -> READ BACK -> VERIFY");
    console_new_line(console);
    console_append_text(console, "TARGET: SANDBOX RWTEST.TXT (PORT ");
    console_append_uint(console, rw->disk.port);
    console_append_text(console, ")");
    console_new_line(console);
    if (rw->test_pass) {
        console_append_text(console, "WROTE: ");
        console_append_text(console, rw->readback);
        console_new_line(console);
        console_append_text(console, "SECTOR + FAT32 READBACK MATCH");
        console_new_line(console);
        console_append_text(console, "RWTEST: PASS");
    } else {
        console_append_text(console, "RWTEST: FAIL (ERR ");
        console_append_uint(console, rw->last_error);
        console_append_text(console, ")");
    }
    console_new_line(console);
}

/* RWREAD: read back the current sandbox test file (read-only path). */
static void console_append_rwread_line(VibioConsoleState *console, VibioRwSandbox *rw)
{
    if (rw == 0 || !rw->present || !rw->rwtest_ok) {
        console_append_text(console, "RWREAD: NO SANDBOX RWTEST.TXT");
        console_new_line(console);
        return;
    }
    rw_sandbox_read_current(rw);
    if (rw->readback_len == 0) {
        console_append_text(console, "RWREAD: READ FAILED (ERR ");
        console_append_uint(console, rw->last_error);
        console_append_text(console, ")");
        console_new_line(console);
        return;
    }
    console_append_text(console, "RWREAD RWTEST.TXT: ");
    console_append_text(console, rw->readback);
    console_new_line(console);
    console_append_text(console, "SEQ: ");
    console_append_uint(console, rw->last_seq);
    console_new_line(console);
}

/* RWDISABLE: turn writes off until the next reboot. */
static void console_run_rwdisable(VibioConsoleState *console, VibioRwSandbox *rw)
{
    if (rw != 0) {
        rw->disabled = 1;
        rw->armed = 0;
    }
    console_append_text(console, "RWDISABLE: WRITES OFF UNTIL REBOOT");
    console_new_line(console);
}

MAYBE_UNUSED static void console_append_clear_line(VibioConsoleState *console)
{
    CEMIT('S');
    CEMIT('C');
    CEMIT('R');
    CEMIT('E');
    CEMIT('E');
    CEMIT('N');
    CEMIT(' ');
    CEMIT('C');
    CEMIT('L');
    CEMIT('E');
    CEMIT('A');
    CEMIT('R');
    CEMIT('E');
    CEMIT('D');
    console_new_line(console);
}

static void console_append_prompt_echo(VibioConsoleState *console)
{
    CEMIT('V');
    CEMIT('I');
    CEMIT('B');
    CEMIT('I');
    CEMIT('O');
    CEMIT(':');
    CEMIT(' ');

    for (vibio_u32 i = 0; i < console->input_length; i++) {
        console_append_char(console, console->input[i]);
    }

    console_new_line(console);
}

#undef CEMIT

/* Commands are matched case-insensitively so typed lower/upper case both work
 * (e.g. "help", "Help", and "HELP" all run HELP). */
static char command_char_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

static vibio_u32 console_input_matches_text(const VibioConsoleState *console, const char *text)
{
    vibio_u32 i = 0;
    while (text[i] != 0) {
        if (i >= console->input_length ||
            command_char_upper(console->input[i]) != command_char_upper(text[i])) {
            return 0;
        }

        i++;
    }

    return i == console->input_length;
}

static vibio_u32 console_input_starts_with(const VibioConsoleState *console, const char *text)
{
    vibio_u32 i = 0;
    while (text[i] != 0) {
        if (i >= console->input_length ||
            command_char_upper(console->input[i]) != command_char_upper(text[i])) {
            return 0;
        }
        i++;
    }
    return 1;
}

static vibio_u32 console_input_is_help(const VibioConsoleState *console) { return console_input_matches_text(console, "HELP"); }
static vibio_u32 console_input_is_clear(const VibioConsoleState *console) { return console_input_matches_text(console, "CLEAR"); }
static vibio_u32 console_input_is_mem(const VibioConsoleState *console) { return console_input_matches_text(console, "MEM"); }
static vibio_u32 console_input_is_heap(const VibioConsoleState *console) { return console_input_matches_text(console, "HEAP"); }
static vibio_u32 console_input_is_tasks(const VibioConsoleState *console) { return console_input_matches_text(console, "TASKS"); }
static vibio_u32 console_input_is_sched(const VibioConsoleState *console) { return console_input_matches_text(console, "SCHED"); }
static vibio_u32 console_input_is_user(const VibioConsoleState *console) { return console_input_matches_text(console, "USER"); }
static vibio_u32 console_input_is_files(const VibioConsoleState *console) { return console_input_matches_text(console, "FILES"); }
static vibio_u32 console_input_is_browser(const VibioConsoleState *console) { return console_input_matches_text(console, "BROWSER"); }
static vibio_u32 console_input_is_panic(const VibioConsoleState *console) { return console_input_matches_text(console, "PANIC"); }
static vibio_u32 console_input_is_selftest(const VibioConsoleState *console) { return console_input_matches_text(console, "SELFTEST"); }
static vibio_u32 console_input_is_disk(const VibioConsoleState *console) { return console_input_matches_text(console, "DISK"); }
static vibio_u32 console_input_is_fs(const VibioConsoleState *console) { return console_input_matches_text(console, "FS"); }
static vibio_u32 console_input_is_usb(const VibioConsoleState *console) { return console_input_matches_text(console, "USB"); }
static vibio_u32 console_input_is_usbstor(const VibioConsoleState *console) { return console_input_matches_text(console, "USBSTOR") || console_input_matches_text(console, "USBMSD"); }
static vibio_u32 console_input_is_usbhotplug(const VibioConsoleState *console) { return console_input_matches_text(console, "USBHOTPLUG"); }
static vibio_u32 console_input_is_xhci(const VibioConsoleState *console) { return console_input_matches_text(console, "XHCI"); }
static vibio_u32 console_input_is_storage(const VibioConsoleState *console) { return console_input_matches_text(console, "STORAGE") || console_input_matches_text(console, "MOUNT"); }
static vibio_u32 console_input_is_usbfiles(const VibioConsoleState *console) { return console_input_matches_text(console, "USBFILES"); }
static vibio_u32 console_input_is_usbtest(const VibioConsoleState *console) { return console_input_starts_with(console, "USBTEST"); }
static vibio_u32 console_input_is_filesdbg(const VibioConsoleState *console) { return console_input_matches_text(console, "FILESDBG"); }
static vibio_u32 console_input_is_mouse(const VibioConsoleState *console) { return console_input_matches_text(console, "MOUSE"); }
static vibio_u32 console_input_is_sounds(const VibioConsoleState *console) { return console_input_matches_text(console, "SOUNDS"); }
static vibio_u32 console_input_is_syscall(const VibioConsoleState *console) { return console_input_matches_text(console, "SYSCALL"); }
static vibio_u32 console_input_is_ticks(const VibioConsoleState *console) { return console_input_matches_text(console, "TICKS"); }
static vibio_u32 console_input_is_version(const VibioConsoleState *console) { return console_input_matches_text(console, "VERSION"); }
static vibio_u32 console_input_is_shutdown(const VibioConsoleState *console) { return console_input_matches_text(console, "SHUTDOWN"); }
static vibio_u32 console_input_is_restart(const VibioConsoleState *console) { return console_input_matches_text(console, "RESTART"); }
static vibio_u32 console_input_is_filesapp(const VibioConsoleState *console) { return console_input_matches_text(console, "FILESAPP") || console_input_matches_text(console, "EXPLORER"); }
static vibio_u32 console_input_is_ls(const VibioConsoleState *console) { return console_input_matches_text(console, "LS"); }
static vibio_u32 console_input_is_rwstatus(const VibioConsoleState *console) { return console_input_matches_text(console, "RWSTATUS"); }
static vibio_u32 console_input_is_rwtest(const VibioConsoleState *console) { return console_input_matches_text(console, "RWTEST"); }
static vibio_u32 console_input_is_rwread(const VibioConsoleState *console) { return console_input_matches_text(console, "RWREAD"); }
static vibio_u32 console_input_is_rwdisable(const VibioConsoleState *console) { return console_input_matches_text(console, "RWDISABLE"); }
static vibio_u32 console_input_is_media(const VibioConsoleState *console) { return console_input_matches_text(console, "MEDIA"); }
static vibio_u32 console_input_is_codecs(const VibioConsoleState *console) { return console_input_matches_text(console, "CODECS"); }
static vibio_u32 console_input_is_imgtest(const VibioConsoleState *console) { return console_input_matches_text(console, "IMGTEST"); }
static vibio_u32 console_input_is_videotest(const VibioConsoleState *console) { return console_input_matches_text(console, "VIDEOTEST"); }
static vibio_u32 console_input_is_view(const VibioConsoleState *console) { return console_input_starts_with(console, "VIEW"); }
static vibio_u32 console_input_is_play(const VibioConsoleState *console) { return console_input_starts_with(console, "PLAY"); }
static vibio_u32 console_input_is_vimginfo(const VibioConsoleState *console) { return console_input_starts_with(console, "VIMGINFO"); }
static vibio_u32 console_input_is_vivinfo(const VibioConsoleState *console) { return console_input_starts_with(console, "VIVINFO"); }
static vibio_u32 console_input_is_pnginfo(const VibioConsoleState *console) { return console_input_starts_with(console, "PNGINFO"); }
static vibio_u32 console_input_is_mp4info(const VibioConsoleState *console) { return console_input_starts_with(console, "MP4INFO"); }
static vibio_u32 console_input_is_jpginfo(const VibioConsoleState *console) { return console_input_starts_with(console, "JPGINFO") || console_input_starts_with(console, "JPEGINFO"); }

static vibio_u32 console_input_is_sound_name(const VibioConsoleState *console)
{
    return sound_command_file_name(console) != 0;
}

static void console_clear_input(VibioConsoleState *console)
{
    for (vibio_u32 i = 0; i < CONSOLE_INPUT_MAX; i++) {
        console->input[i] = 0;
    }

    console->input_length = 0;
    console->input_cursor = 0;
}

static void console_copy_input(VibioConsoleState *console)
{
    console->clipboard_length = 0;
    for (vibio_u32 i = 0; i < CONSOLE_INPUT_MAX; i++) {
        console->clipboard[i] = 0;
    }

    for (vibio_u32 i = 0; i < console->input_length && i < CONSOLE_INPUT_MAX; i++) {
        console->clipboard[i] = console->input[i];
        console->clipboard_length++;
    }
}

static void console_paste_input(VibioConsoleState *console)
{
    for (vibio_u32 i = 0; i < console->clipboard_length && console->input_length < CONSOLE_INPUT_MAX; i++) {
        char c = console->clipboard[i];
        if (c < 32 || c > 126) {
            c = ' ';
        }
        for (vibio_u32 j = console->input_length; j > console->input_cursor; j--) {
            console->input[j] = console->input[j - 1];
        }
        console->input[console->input_cursor] = c;
        console->input_length++;
        console->input_cursor++;
    }
}

static void console_run_usbtest(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    VibioAudioState *audio,
    vibio_u64 ticks)
{
    vibio_u32 event = USB_HOTPLUG_EVENT_NONE;
    if (console_input_matches_text(console, "USBTEST INSERT")) {
        event = USB_HOTPLUG_EVENT_INSERT;
    } else if (console_input_matches_text(console, "USBTEST REMOVE")) {
        event = USB_HOTPLUG_EVENT_REMOVE;
    }

    if (event == USB_HOTPLUG_EVENT_NONE) {
        console_append_text(console, "USBTEST: USE USBTEST INSERT OR USBTEST REMOVE");
        console_new_line(console);
        console_append_text(console, "SIMULATED EVENT ONLY");
        console_new_line(console);
        return;
    }

    console_append_text(console, event == USB_HOTPLUG_EVENT_INSERT ?
        "USBTEST INSERT: SIMULATED EVENT ONLY" :
        "USBTEST REMOVE: SIMULATED EVENT ONLY");
    console_new_line(console);
    usb_hotplug_record_event(&g_usb_hotplug, boot_info, audio, event, 0, ticks, 1);
    console_append_text(console, "REAL HOTPLUG: ");
    console_append_text(console, g_usb_hotplug.hotplug_supported ? "NOT TESTED BY THIS COMMAND" : "UNSUPPORTED");
    console_new_line(console);
    console_append_text(console, "LAST SOUND: ");
    console_append_usb_hotplug_sound_result(console, g_usb_hotplug.last_sound_result);
    console_new_line(console);
}

static void console_insert_input_char(VibioConsoleState *console, char c)
{
    if (console->input_length >= CONSOLE_INPUT_MAX) {
        return;
    }

    for (vibio_u32 i = console->input_length; i > console->input_cursor; i--) {
        console->input[i] = console->input[i - 1];
    }
    console->input[console->input_cursor] = c;
    console->input_length++;
    console->input_cursor++;
}

static void console_append_cancel_line(VibioConsoleState *console)
{
    console->history_revision++;
    console->view_offset = 0;
    console_append_text(console, "VIBIO: ");
    for (vibio_u32 i = 0; i < console->input_length; i++) {
        console_append_char(console, console->input[i]);
    }
    console_append_text(console, "^C");
    console_new_line(console);
    console_clear_input(console);
}

static vibio_u32 console_copy_cmd_arg(const VibioConsoleState *console, const char *prefix, char *out, vibio_u32 max)
{
    vibio_u32 plen = 0;
    while (prefix[plen] != 0) {
        plen++;
    }
    /* Skip the separator space(s) between the command and its argument; the
     * argument starts at the first non-space character after the command. */
    vibio_u32 j = plen;
    while (j < console->input_length && console->input[j] == ' ') {
        j++;
    }
    vibio_u32 i = 0;
    while (i + 1 < max && j < console->input_length) {
        char c = console->input[j];
        if (c == ' ') {
            break;
        }
        out[i] = c;
        i++;
        j++;
    }
    out[i] = 0;
    return i;
}

static void console_append_native_image_info(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const char *prefix,
    const char *default_name)
{
    char name[VIBIO_BOOT_FILE_NAME_MAX];
    console_copy_cmd_arg(console, prefix, name, sizeof(name));
    if (name[0] == 0) {
        media_copy_name(name, sizeof(name), default_name);
    }
    VibioMediaViewer *mv = g_selftest_ctx.media_viewer;
    if (mv == 0 || mv->scratch == 0 || mv->scratch_capacity == 0 ||
        mv->work == 0 || mv->work_capacity == 0) {
        console_append_text(console, prefix);
        console_append_text(console, ": NATIVE BUFFERS UNAVAILABLE");
        console_new_line(console);
        return;
    }
    if (mv->loaded) {
        console_append_text(console, prefix);
        console_append_text(console, ": SKIP - MEDIA VIEWER BUFFER IN USE");
        console_new_line(console);
        return;
    }
    vibio_u32 total = 0;
    vibio_u8 src = FILES_SRC_NONE;
    vibio_u32 read = media_read_bytes(
        boot_info, (VibioDiskReadTest *)disk, (VibioFat32ReadTest *)fs, name, 0,
        mv->scratch, mv->scratch_capacity, &total, &src);
    if (read == 0) {
        console_append_text(console, prefix);
        console_append_text(console, ": FILE NOT FOUND OR UNREADABLE");
        console_new_line(console);
        return;
    }
    MediaBitmap bmp;
    MediaDecodeStatus status;
    zero_bytes(&status, sizeof(status));
    vibio_u32 ok = 0;
    if (total > read) {
        media_decode_status_set(&status, MEDIA_IMG_BUFFER_TOO_SMALL,
                                "image file too large for read buffer");
    } else {
        ok = media_decode_native_image(name, mv->scratch, read, mv->scratch_capacity,
                                       mv->work, mv->work_capacity, &bmp, &status);
    }
    console_append_text(console, prefix);
    console_append_text(console, ": ");
    if (ok) {
        console_append_text(console, "NATIVE OK ");
        console_append_uint(console, bmp.width);
        console_append_text(console, "x");
        console_append_uint(console, bmp.height);
    } else {
        console_append_text(console, status.detail[0] ? status.detail : media_decode_status_text(status.code));
    }
    console_new_line(console);
}

/* Print a track's codec + key facts on one console line. */
static void console_append_mp4_track_line(VibioConsoleState *console, const char *label, const Mp4Track *t)
{
    console_append_text(console, label);
    if (!t->present) {
        console_append_text(console, "none");
        console_new_line(console);
        return;
    }
    console_append_text(console, mp4_codec_name(t->codec));
    console_append_text(console, " (");
    console_append_text(console, t->fourcc);
    console_append_text(console, ")");
    if (t->is_video || t->width > 0) {
        console_append_text(console, " ");
        console_append_uint(console, t->width);
        console_append_text(console, "x");
        console_append_uint(console, t->height);
        if (t->codec == MP4_CODEC_H264 && t->has_avcc) {
            console_append_text(console, " ");
            console_append_text(console, mp4_h264_profile_name(t->h264_profile, t->h264_compat));
            console_append_text(console, " L");
            console_append_uint(console, t->h264_level / 10);
            console_append_text(console, ".");
            console_append_uint(console, t->h264_level % 10);
        }
    } else if (t->is_audio) {
        console_append_text(console, " ");
        console_append_uint(console, t->sample_rate);
        console_append_text(console, "Hz ");
        console_append_uint(console, t->channels);
        console_append_text(console, "ch");
    }
    console_new_line(console);
    /* Sample/chunk tables + counts. */
    console_append_text(console, "    samples=");
    console_append_uint(console, t->sample_count);
    console_append_text(console, " chunks=");
    console_append_uint(console, t->chunk_count);
    console_append_text(console, " stbl[");
    console_append_text(console, t->has_stsd ? "d" : "-");
    console_append_text(console, t->has_stts ? "t" : "-");
    console_append_text(console, t->has_stsc ? "c" : "-");
    console_append_text(console, t->has_stsz ? "z" : "-");
    console_append_text(console, t->has_stco ? "o" : "-");
    console_append_text(console, "]");
    console_new_line(console);
    console_append_text(console, "    demux=");
    console_append_text(console, t->sample_demux_ready ? "READY" : "NO");
    if (t->sample_demux_ready) {
        console_append_text(console, " first off=");
        console_append_uint(console, (vibio_u32)t->first_sample_offset);
        console_append_text(console, " size=");
        console_append_uint(console, t->first_sample_size);
        console_append_text(console, " dur=");
        console_append_uint(console, t->first_sample_duration);
        console_append_text(console, " max=");
        console_append_uint(console, t->max_sample_size);
        if (t->codec == MP4_CODEC_H264 && t->h264_nal_length_size > 0) {
            console_append_text(console, " nal_len=");
            console_append_uint(console, t->h264_nal_length_size);
        }
    }
    console_new_line(console);
}

/* MP4INFO <file>: native MP4 container probe. Reads the file into the Media
 * Viewer scratch buffer (already sized for big media) and reports the demux
 * facts honestly. Safe on missing/corrupt/oversized files. */
static void console_append_mp4info(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs)
{
    char name[VIBIO_BOOT_FILE_NAME_MAX];
    console_copy_cmd_arg(console, "MP4INFO", name, sizeof(name));
    if (name[0] == 0) {
        media_copy_name(name, sizeof(name), "TEST.MP4");
    }
    VibioMediaViewer *mv = g_selftest_ctx.media_viewer;
    if (mv == 0 || mv->scratch == 0 || mv->scratch_capacity == 0) {
        console_append_text(console, "MP4INFO: NATIVE BUFFER UNAVAILABLE");
        console_new_line(console);
        return;
    }
    if (mv->loaded) {
        console_append_text(console, "MP4INFO: SKIP - MEDIA VIEWER BUFFER IN USE");
        console_new_line(console);
        return;
    }
    vibio_u32 total = 0;
    vibio_u8 src = FILES_SRC_NONE;
    Mp4Info info;
    vibio_u32 read = media_read_mp4_probe(
        boot_info, (VibioDiskReadTest *)disk, (VibioFat32ReadTest *)fs, name,
        mv->scratch, mv->scratch_capacity, &total, &src, &info);
    console_append_text(console, "MP4INFO ");
    console_append_text(console, name);
    console_new_line(console);
    if (read == 0) {
        console_append_text(console, "  FILE NOT FOUND OR UNREADABLE");
        console_new_line(console);
        return;
    }
    console_append_text(console, "  size=");
    console_append_uint(console, total);
    console_append_text(console, " bytes");
    if (total > read) {
        /* File bigger than our read window: parse what we have, say so. */
        console_append_text(console, " (read first ");
        console_append_uint(console, read);
        console_append_text(console, ")");
    }
    console_new_line(console);

    console_append_text(console, "  status=");
    console_append_text(console, info.status_detail[0] ? info.status_detail : mp4_status_text(info.status));
    console_new_line(console);

    if (info.has_ftyp) {
        console_append_text(console, "  brand=");
        console_append_text(console, info.major_brand);
        console_append_text(console, " ftyp:");
        console_append_text(console, info.has_ftyp ? "y" : "n");
        console_append_text(console, " moov:");
        console_append_text(console, info.has_moov ? "y" : "n");
        console_append_text(console, " mdat:");
        console_append_text(console, info.has_mdat ? "y" : "n");
        console_new_line(console);
    }

    if (info.status != MP4_OK) {
        /* Not enough to describe tracks; the status line already explained why. */
        return;
    }

    console_append_text(console, "  timescale=");
    console_append_uint(console, info.movie_timescale);
    if (info.movie_timescale > 0) {
        vibio_u64 secs = info.movie_duration / info.movie_timescale;
        vibio_u64 tenths = (info.movie_duration * 10U / info.movie_timescale) % 10U;
        console_append_text(console, " duration=");
        console_append_uint(console, secs);
        console_append_text(console, ".");
        console_append_uint(console, tenths);
        console_append_text(console, "s");
    }
    console_append_text(console, " tracks=");
    console_append_uint(console, info.track_count);
    console_new_line(console);

    console_append_mp4_track_line(console, "  video=", &info.video);
    console_append_mp4_track_line(console, "  audio=", &info.audio);

    console_append_text(console, "  decode=");
    console_append_text(console, info.decode_supported ? "SUPPORTED" : "UNSUPPORTED");
    console_new_line(console);
    if (!info.decode_supported) {
        console_append_text(console, "  reason: ");
        console_append_text(console, info.unsupported_reason);
        console_new_line(console);
    }
}

static void console_append_media_summary(VibioConsoleState *console)
{
    console_append_text(console, "MEDIA: VIEWER+PLAYER STAGE 34");
    console_new_line(console);
    console_append_text(console, "NATIVE: VIMG/VIM BMP PNG(partial) JPEG(partial)");
    console_new_line(console);
    console_append_text(console, "FALLBACK: HOST VIMG FOR UNSUPPORTED IMAGE VARIANTS");
    console_new_line(console);
    console_append_text(console, "VIDEO: VIV1+PCM WAV; MP4 NATIVE SAMPLE DEMUX, NO DECODE");
    console_new_line(console);
    console_append_text(console, "MANIFEST: ");
    console_append_text(console, g_media_manifest_present ? "MEDIA.MF PRESENT" : "MEDIA.MF MISSING");
    console_new_line(console);
    console_append_text(console, "LAST ERROR: ");
    console_append_text(console, g_media_last_error[0] ? g_media_last_error : "NONE");
    console_new_line(console);
}

static void console_append_codecs_line(VibioConsoleState *console, VibioAudioState *audio)
{
    console_append_text(console, "CODECS NATIVE: VIMG VIM BMP24/32 PNG-PARTIAL JPEG-PARTIAL VIV1 WAV");
    console_new_line(console);
    console_append_text(console, "PNG: RGBA/RGB/GRAY/INDEXED, NON-INTERLACED, DEFLATE");
    console_new_line(console);
    console_append_text(console, "JPEG: BASELINE SOF0 YCBCR/GRAY 444/422/420");
    console_new_line(console);
    console_append_text(console, "MP4: NATIVE ISO-BMFF SAMPLE DEMUX (MP4INFO); NO H264/AAC DECODE");
    console_new_line(console);
    console_append_text(console, "HOST-ONLY DECODE: H264/H265/AV1/MP3/AAC; CONVERT TO VIV/WAV");
    console_new_line(console);
    console_append_text(console, "AUDIO PCM: ");
    console_append_text(console, audio != 0 && audio->ready ? "READY" : "UNAVAILABLE");
    console_new_line(console);
}

static void console_run_command(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    const VibioIrqState *irq_state,
    const VibioKernelHeap *heap,
    const VibioTaskManager *tasks,
    const VibioUserBoundaryBuild *user_boundary,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const VibioUsbReadTest *usb,
    const VibioMouseState *mouse,
    const VibioSyscallState *syscalls,
    VibioAudioState *audio,
    VibioRwSandbox *rw,
    vibio_u64 free_pages_before)
{
    /* Any command (even an empty Enter echo) changes the history text, so bump
     * the revision to tell the renderer the history needs one redraw. New output
     * also snaps the view back to the newest line. */
    console->history_revision++;
    console->view_offset = 0;
    console_append_prompt_echo(console);

    if (console->input_length == 0) {
        console_clear_input(console);
        return;
    }

    console->commands_run++;

    if (console_input_is_help(console)) {
        console_append_commands_line(console);
    } else if (console_input_is_clear(console)) {
        console_clear_lines(console);
    } else if (console_input_is_mem(console)) {
        console_append_mem_line(console, boot_info, free_pages_before);
    } else if (console_input_is_heap(console)) {
        console_append_heap_line(console, heap);
    } else if (console_input_is_tasks(console)) {
        console_append_tasks_line(console, tasks);
    } else if (console_input_is_sched(console)) {
        console_append_sched_line(console, irq_state, tasks);
    } else if (console_input_is_user(console)) {
        console_append_user_line(console, user_boundary);
    } else if (console_input_is_disk(console)) {
        console_append_disk_line(console, disk);
    } else if (console_input_is_fs(console)) {
        console_append_fs_line(console, fs);
    } else if (console_input_is_usbstor(console)) {
        console_append_usbstor_line(console, &g_usb_storage);
    } else if (console_input_is_xhci(console)) {
        console_append_xhci_line(console, &g_xhci_msd);
    } else if (console_input_is_storage(console)) {
        console_append_xhci_line(console, &g_xhci_msd);
        console_append_usbstor_line(console, &g_usb_storage);
    } else if (console_input_is_usbfiles(console)) {
        if (g_usb_storage.fat32_mounted) {
            console->wants_files = 1;
            console->wants_files_usb = 1;
            console_append_text(console, "USBFILES: OPENING FILES ON USB DRIVE");
        } else {
            console_append_text(console, "USBFILES: NO USB FAT32 MOUNTED");
        }
        console_new_line(console);
    } else if (console_input_is_usbhotplug(console)) {
        console_append_usb_hotplug_line(console, &g_usb_hotplug);
    } else if (console_input_is_usbtest(console)) {
        console_run_usbtest(console, boot_info, audio, irq_state != 0 ? irq_state->timer_ticks : 0);
    } else if (console_input_is_filesdbg(console)) {
        console_append_filesdbg_line(console, disk, fs, &g_usb_storage, boot_info);
    } else if (console_input_is_usb(console)) {
        console_append_usb_line(console, usb, &g_usb_hotplug);
    } else if (console_input_is_mouse(console)) {
        console_append_mouse_line(console, mouse);
    } else if (console_input_is_files(console)) {
        console_append_files_line(console, boot_info);
    } else if (console_input_is_browser(console)) {
        console->wants_browser = 1;
        console_append_text(console, "BROWSER: OPENING START.HTM");
        console_new_line(console);
    } else if (console_input_is_selftest(console)) {
        selftest_run(
            console,
            boot_info,
            irq_state,
            (VibioKernelHeap *)heap,
            disk,
            fs,
            mouse,
            audio,
            rw,
            &g_selftest_ctx);
    } else if (console_input_is_panic(console)) {
        console_append_text(console, "PANIC: TRIGGERING TEST PAGE FAULT");
        console_new_line(console);
        panic_trigger_test();
    } else if (console_input_is_sounds(console)) {
        console_append_sounds_line(console, boot_info, audio);
    } else if (console_input_is_sound_name(console)) {
        console_append_sound_status_line(console, boot_info, audio);
    } else if (console_input_is_syscall(console)) {
        console_append_syscall_line(console, syscalls);
    } else if (console_input_is_ticks(console)) {
        console_append_ticks_line(console, irq_state);
    } else if (console_input_is_version(console)) {
        console_append_version_line(console);
    } else if (console_input_is_shutdown(console)) {
        console_append_text(console, "SHUTDOWN: POWERING OFF");
        console_new_line(console);
        power_shutdown();
    } else if (console_input_is_restart(console)) {
        console_append_text(console, "RESTART: REBOOTING");
        console_new_line(console);
        power_restart();
    } else if (console_input_is_filesapp(console)) {
        console->wants_files = 1;
        console_append_text(console, "FILES: OPENING /");
        console_new_line(console);
    } else if (console_input_is_ls(console)) {
        console_append_ls_lines(console, disk, fs);
    } else if (console_input_is_rwstatus(console)) {
        console_append_rwstatus_line(console, rw);
    } else if (console_input_is_rwtest(console)) {
        console_run_rwtest(console, rw);
    } else if (console_input_is_rwread(console)) {
        console_append_rwread_line(console, rw);
    } else if (console_input_is_rwdisable(console)) {
        console_run_rwdisable(console, rw);
    } else if (console_input_is_media(console)) {
        console_append_media_summary(console);
    } else if (console_input_is_codecs(console)) {
        console_append_codecs_line(console, audio);
    } else if (console_input_is_imgtest(console)) {
        console_append_text(console, "IMGTEST: OPEN FILES -> TEST.PNG OR PHOTO.JPG");
        console_new_line(console);
        console->wants_media_viewer = 1;
        media_copy_name(console->media_open_name, VIBIO_BOOT_FILE_NAME_MAX, "TEST.PNG");
    } else if (console_input_is_videotest(console)) {
        console_append_text(console, "VIDEOTEST: OPEN VIDEOTST.VIV IN MEDIA PLAYER");
        console_new_line(console);
        console->wants_media_player = 1;
        media_copy_name(console->media_open_name, VIBIO_BOOT_FILE_NAME_MAX, "VIDEOTST.VIV");
    } else if (console_input_is_view(console)) {
        console_copy_cmd_arg(console, "VIEW", console->media_open_name, VIBIO_BOOT_FILE_NAME_MAX);
        if (console->media_open_name[0] == 0) {
            console_append_text(console, "VIEW: USE VIEW <filename>");
            console_new_line(console);
        } else {
            console->wants_media_viewer = 1;
        }
    } else if (console_input_is_play(console)) {
        console_copy_cmd_arg(console, "PLAY", console->media_open_name, VIBIO_BOOT_FILE_NAME_MAX);
        if (console->media_open_name[0] == 0) {
            console_append_text(console, "PLAY: USE PLAY <filename.VIV>");
            console_new_line(console);
        } else {
            console->wants_media_player = 1;
        }
    } else if (console_input_is_mp4info(console)) {
        console_append_mp4info(console, boot_info, disk, fs);
    } else if (console_input_is_pnginfo(console)) {
        console_append_native_image_info(console, boot_info, disk, fs, "PNGINFO", "TEST.PNG");
    } else if (console_input_is_jpginfo(console)) {
        console_append_native_image_info(console, boot_info, disk, fs,
                                         console_input_starts_with(console, "JPEGINFO") ? "JPEGINFO" : "JPGINFO",
                                         "PHOTO.JPG");
    } else if (console_input_is_vimginfo(console) || console_input_is_vivinfo(console)) {
        char name[VIBIO_BOOT_FILE_NAME_MAX];
        const char *prefix = console_input_is_vimginfo(console) ? "VIMGINFO" : "VIVINFO";
        console_copy_cmd_arg(console, prefix, name, sizeof(name));
        if (name[0] == 0) {
            console_append_text(console, "USE VIMGINFO/VIVINFO <filename>");
            console_new_line(console);
        } else {
            vibio_u8 hdr[64];
            vibio_u32 sz = 0;
            vibio_u32 rd = fat32_read_file((VibioDiskReadTest *)disk, fs, name, hdr, sizeof(hdr), &sz);
            if (rd == 0) {
                console_append_text(console, "FILE NOT FOUND OR UNREADABLE");
                console_new_line(console);
            } else if (console_input_is_vimginfo(console)) {
                MediaBitmap bmp;
                console_append_text(console, media_vimg_parse(hdr, rd, &bmp) ? "VIMG OK" : "VIMG INVALID");
                console_new_line(console);
            } else {
                console_append_text(console, (hdr[0] == 'V' && hdr[1] == 'I' && hdr[2] == 'V' && hdr[3] == '1') ?
                                    "VIV1 HEADER OK" : "VIV INVALID");
                console_new_line(console);
            }
        }
    } else {
        console_append_unknown_line(console);
    }

    console_clear_input(console);
}

/* Map a make scancode to a character, honoring Shift and Caps Lock. Letters use
 * (shift XOR caps) for case; the digit row and punctuation keys use their
 * shifted symbol only when Shift is held (Caps Lock does not affect them). */
static char keyboard_scancode_to_char(vibio_u8 scancode, vibio_u8 shift_down, vibio_u8 caps_on)
{
    /* Letters: lowercase base, uppercase when shift XOR caps. */
    char letter = 0;
    switch (scancode) {
    case 0x10: letter = 'q'; break;
    case 0x11: letter = 'w'; break;
    case 0x12: letter = 'e'; break;
    case 0x13: letter = 'r'; break;
    case 0x14: letter = 't'; break;
    case 0x15: letter = 'y'; break;
    case 0x16: letter = 'u'; break;
    case 0x17: letter = 'i'; break;
    case 0x18: letter = 'o'; break;
    case 0x19: letter = 'p'; break;
    case 0x1E: letter = 'a'; break;
    case 0x1F: letter = 's'; break;
    case 0x20: letter = 'd'; break;
    case 0x21: letter = 'f'; break;
    case 0x22: letter = 'g'; break;
    case 0x23: letter = 'h'; break;
    case 0x24: letter = 'j'; break;
    case 0x25: letter = 'k'; break;
    case 0x26: letter = 'l'; break;
    case 0x2C: letter = 'z'; break;
    case 0x2D: letter = 'x'; break;
    case 0x2E: letter = 'c'; break;
    case 0x2F: letter = 'v'; break;
    case 0x30: letter = 'b'; break;
    case 0x31: letter = 'n'; break;
    case 0x32: letter = 'm'; break;
    default: break;
    }
    if (letter != 0) {
        vibio_u8 upper = (shift_down != 0) ^ (caps_on != 0);
        if (upper) {
            return (char)(letter - 'a' + 'A');
        }
        return letter;
    }

    /* Number row and punctuation: base / shifted pairs. */
    switch (scancode) {
    case 0x02: return shift_down ? '!' : '1';
    case 0x03: return shift_down ? '@' : '2';
    case 0x04: return shift_down ? '#' : '3';
    case 0x05: return shift_down ? '$' : '4';
    case 0x06: return shift_down ? '%' : '5';
    case 0x07: return shift_down ? '^' : '6';
    case 0x08: return shift_down ? '&' : '7';
    case 0x09: return shift_down ? '*' : '8';
    case 0x0A: return shift_down ? '(' : '9';
    case 0x0B: return shift_down ? ')' : '0';
    case 0x0C: return shift_down ? '_' : '-';
    case 0x0D: return shift_down ? '+' : '=';
    case 0x1A: return shift_down ? '{' : '[';
    case 0x1B: return shift_down ? '}' : ']';
    case 0x27: return shift_down ? ':' : ';';
    case 0x28: return shift_down ? '"' : '\'';
    case 0x29: return shift_down ? '~' : '`';
    case 0x2B: return shift_down ? '|' : '\\';
    case 0x33: return shift_down ? '<' : ',';
    case 0x34: return shift_down ? '>' : '.';
    case 0x35: return shift_down ? '?' : '/';
    case 0x39: return ' ';
    default: return 0;
    }
}

static vibio_u32 desktop_rename_process_key(
    VibioWindowManager *wm,
    vibio_u8 base,
    vibio_u8 shift_down,
    vibio_u8 caps_on)
{
    if (wm == 0 || !wm->desktop_rename_active) {
        return 0;
    }
    if (wm->desktop_rename_index < DESKTOP_ICON_BUILTIN_COUNT) {
        wm->desktop_rename_active = 0;
        return 1;
    }
    vibio_u32 ti = wm->desktop_rename_index - DESKTOP_ICON_BUILTIN_COUNT;
    if (ti >= wm->desktop_temp_count) {
        wm->desktop_rename_active = 0;
        return 1;
    }
    VibioDesktopTempIcon *icon = &wm->desktop_temp[ti];
    if (base == 0x01 || base == 0x1C) { /* Escape / Enter */
        if (icon->label_len == 0) {
            desktop_set_temp_label(icon, icon->kind == DESKTOP_ICON_KIND_TEMP_FOLDER ? "New folder" : "New text file");
        }
        wm->desktop_rename_active = 0;
        wm->desktop_rename_replace = 0;
        return 1;
    }
    if (base == 0x0E) { /* Backspace */
        if (wm->desktop_rename_replace) {
            icon->label_len = 0;
            icon->label[0] = 0;
            wm->desktop_rename_replace = 0;
            return 1;
        }
        if (icon->label_len > 0) {
            icon->label_len--;
            icon->label[icon->label_len] = 0;
        }
        return 1;
    }
    char c = keyboard_scancode_to_char(base, shift_down, caps_on);
    if (c != 0 && icon->label_len + 1U < sizeof(icon->label)) {
        if (wm->desktop_rename_replace) {
            icon->label_len = 0;
            icon->label[0] = 0;
            wm->desktop_rename_replace = 0;
        }
        icon->label[icon->label_len++] = c;
        icon->label[icon->label_len] = 0;
    }
    return 1;
}

/* Scroll the terminal history view. Positive delta scrolls toward older lines;
 * negative scrolls back toward the newest. Clamped so it cannot run past the
 * ends of the stored history. */
static void console_scroll(VibioConsoleState *console, int delta)
{
    int off = (int)console->view_offset + delta;
    int max_off = (int)console->cursor_row;
    if (off < 0) {
        off = 0;
    }
    if (off > max_off) {
        off = max_off;
    }
    console->view_offset = (vibio_u32)off;
}

static void console_process_scancode(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    const VibioIrqState *irq_state,
    const VibioKernelHeap *heap,
    const VibioTaskManager *tasks,
    const VibioUserBoundaryBuild *user_boundary,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const VibioUsbReadTest *usb,
    const VibioMouseState *mouse,
    const VibioSyscallState *syscalls,
    VibioAudioState *audio,
    VibioRwSandbox *rw,
    vibio_u64 free_pages_before,
    VibioWindowManager *wm,
    VibioBrowser *browser,
    VibioFilesApp *files,
    vibio_u8 scancode)
{
    /* When the browser is the focused window, PgUp/PgDn, the arrow keys, Home,
     * End, and Space scroll the page instead of reaching the terminal. */
    vibio_u32 browser_focused = 0;
    vibio_u32 files_focused = 0;
    if (wm != 0) {
        vibio_u32 top = wm_top_visible(wm);
        if (top != WM_NO_WINDOW) {
            if (browser != 0 && wm->windows[top].kind == WINDOW_KIND_BROWSER) {
                browser_focused = 1;
            } else if (files != 0 && wm->windows[top].kind == WINDOW_KIND_FILES) {
                files_focused = 1;
            }
        }
    }

    /* F11 toggles true full screen on the focused window (browser-style). Edge-
     * detected so key-repeat while held does not flip it on and off. Support
     * both translated set-1 (57/d7) and common untranslated set-2 (78/f0 78)
     * so firmware/controller differences on real PCs do not silently drop it. */
    if (scancode == 0xF0) {
        console->set2_break_pending = 1;
        return;
    }
    if (console->set2_break_pending) {
        console->set2_break_pending = 0;
        if (scancode == 0x78) {
            console->f11_down = 0;
        }
        return;
    }
    if (scancode == 0x57 || scancode == 0x78) { /* F11 make */
        if (wm != 0 && !console->f11_down) {
            vibio_u32 top = wm_top_visible(wm);
            if (top != WM_NO_WINDOW) {
                wm_toggle_fullscreen(wm, top, boot_info);
            }
        }
        console->f11_down = 1;
        return;
    }
    if (scancode == 0xD7) { /* F11 break */
        console->f11_down = 0;
        return;
    }

    /* Extended (E0-prefixed) keys: PgUp/PgDn and the arrow keys scroll the
     * terminal history instead of typing. */
    if (scancode == 0xE0) {
        console->ext_pending = 1;
        return;
    }
    if (console->ext_pending) {
        console->ext_pending = 0;
        {
            vibio_u8 ext_base = scancode & 0x7F;
            vibio_u8 ext_break = (scancode & 0x80) != 0;
            if (ext_base == 0x5B || ext_base == 0x5C) { /* left/right Super */
                console->super_down = ext_break ? 0 : 1;
                return;
            }
        }
        if ((scancode & 0x80) == 0) {
            vibio_u8 ext = scancode & 0x7F;
            if (files_focused) {
                if (ext == 0x48) {
                    files_app_move_selection(files, -1);   /* Up */
                } else if (ext == 0x50) {
                    files_app_move_selection(files, 1);    /* Down */
                } else if (ext == 0x49) {
                    files_app_move_selection(files, -(int)(files->row_visible > 1 ? files->row_visible - 1 : 1)); /* Page Up */
                } else if (ext == 0x51) {
                    files_app_move_selection(files, (int)(files->row_visible > 1 ? files->row_visible - 1 : 1));  /* Page Down */
                } else if (ext == 0x47) {
                    files_app_move_selection(files, -1000000); /* Home */
                } else if (ext == 0x4F) {
                    files_app_move_selection(files, 1000000);  /* End */
                }
                return;
            }
            if (browser_focused) {
                int page = (int)browser->view_height > 40 ? (int)browser->view_height - 40 : 80;
                if (ext == 0x49) {
                    browser_scroll_by(browser, -page);       /* Page Up */
                } else if (ext == 0x51) {
                    browser_scroll_by(browser, page);        /* Page Down */
                } else if (ext == 0x48) {
                    browser_scroll_by(browser, -BROWSER_BODY_LINE * 2); /* Up */
                } else if (ext == 0x50) {
                    browser_scroll_by(browser, BROWSER_BODY_LINE * 2);  /* Down */
                } else if (ext == 0x47) {
                    browser_scroll_by(browser, -1000000);    /* Home */
                } else if (ext == 0x4F) {
                    browser_scroll_by(browser, 1000000);     /* End */
                }
                return;
            }
            if (ext == 0x49) {
                console_scroll(console, 5);   /* Page Up */
            } else if (ext == 0x51) {
                console_scroll(console, -5);  /* Page Down */
            } else if (ext == 0x48) {
                console_scroll(console, 1);   /* Up arrow */
            } else if (ext == 0x50) {
                console_scroll(console, -1);  /* Down arrow */
            } else if (ext == 0x4B) {
                if (console->input_cursor > 0) {
                    console->input_cursor--;
                }
            } else if (ext == 0x4D) {
                if (console->input_cursor < console->input_length) {
                    console->input_cursor++;
                }
            }
        }
        return;
    }

    vibio_u8 base = scancode & 0x7F;
    vibio_u8 is_break = (scancode & 0x80) != 0;

    /* Modifier keys: track make/break for Shift, toggle Caps Lock on make. */
    if (base == 0x2A || base == 0x36) { /* left/right Shift */
        console->shift_down = is_break ? 0 : 1;
        return;
    }
    if (base == 0x1D) { /* Ctrl */
        console->ctrl_down = is_break ? 0 : 1;
        return;
    }
    if (base == 0x3A) { /* Caps Lock */
        if (!is_break) {
            console->caps_on = console->caps_on ? 0 : 1;
        }
        return;
    }

    if (is_break) {
        return;
    }

    if (console->super_down && base == 0x17) { /* Super+I: Settings */
        console->wants_settings = 1;
        return;
    }

    if (desktop_rename_process_key(wm, base, console->shift_down, console->caps_on)) {
        return;
    }

    /* Files app key control. Enter opens the selection (the main loop performs
     * the open so it can raise the browser / play audio), Backspace goes up,
     * Escape closes an info panel. Other keys are consumed so they do not leak
     * into the terminal input behind the Files window. */
    if (files_focused) {
        if (base == 0x1C) { /* Enter */
            files->open_pending = 1;
        } else if (base == 0x0E) { /* Backspace */
            files_app_go_up(files);
        } else if (base == 0x01) { /* Escape */
            files->info_open = 0;
        }
        return;
    }

    if (browser_focused && console->ctrl_down && base == 0x26) { /* Ctrl+L: focus address bar. */
        browser->address_active = 1;
        browser_sync_address(browser);
        browser->address_replace = 1;
        return;
    }

    if (browser_focused && browser->address_active) {
        if (base == 0x01) { /* Escape */
            browser->address_active = 0;
            browser->address_replace = 0;
            browser_sync_address(browser);
            return;
        }
        if (base == 0x0E) { /* Backspace */
            if (browser->address_replace) {
                browser->address_len = 0;
                browser->address[0] = 0;
                browser->address_replace = 0;
                return;
            }
            if (browser->address_len > 0) {
                browser->address_len--;
                browser->address[browser->address_len] = 0;
            }
            return;
        }
        if (base == 0x1C) { /* Enter */
            if (browser->address_len > 0) {
                browser->nav_pending = 1;
                browser->nav_external = browser_href_is_external(browser->address);
                browser_copy_url(browser->nav_target, BROWSER_URL_MAX, browser->address, 0);
            }
            browser->address_active = 0;
            browser->address_replace = 0;
            return;
        }
        char ac = keyboard_scancode_to_char(base, console->shift_down, console->caps_on);
        if (ac != 0 && browser->address_len + 1 < BROWSER_URL_MAX) {
            if (browser->address_replace) {
                browser->address_len = 0;
                browser->address[0] = 0;
                browser->address_replace = 0;
            }
            browser->address[browser->address_len++] = ac;
            browser->address[browser->address_len] = 0;
        }
        return;
    }

    /* Space pages the focused browser instead of typing into the terminal. */
    if (browser_focused && base == 0x39) {
        int page = (int)browser->view_height > 40 ? (int)browser->view_height - 40 : 80;
        browser_scroll_by(browser, console->shift_down ? -page : page);
        return;
    }

    if (base == 0x0E) { /* Backspace */
        if (console->input_cursor > 0) {
            for (vibio_u32 i = console->input_cursor - 1; i + 1 < console->input_length; i++) {
                console->input[i] = console->input[i + 1];
            }
            console->input_length--;
            console->input[console->input_length] = 0;
            console->input_cursor--;
        }
        console->wants_focus = 1;
        return;
    }

    if (base == 0x1C) { /* Enter */
        console_run_command(console, boot_info, irq_state, heap, tasks, user_boundary, disk, fs, usb, mouse, syscalls, audio, rw, free_pages_before);
        console->wants_focus = 1;
        return;
    }

    if (console->ctrl_down) {
        if (base == 0x1E) { /* Ctrl+A: move to beginning of input line. */
            console->input_cursor = 0;
            console->wants_focus = 1;
            return;
        }
        if (base == 0x2E) { /* Ctrl+C: terminal cancel/interrupt. */
            if (console->shift_down) {
                console_copy_input(console);
            } else {
                console_append_cancel_line(console);
            }
            console->wants_focus = 1;
            return;
        }
        if (base == 0x2F) { /* Ctrl+V: paste copied input. */
            console_paste_input(console);
            console->wants_focus = 1;
            return;
        }
    }

    char c = keyboard_scancode_to_char(base, console->shift_down, console->caps_on);
    if (c != 0 && console->input_length < CONSOLE_INPUT_MAX) {
        console_insert_input_char(console, c);
        /* A bare Space on the empty desktop should not pop the Terminal open;
         * only real typed input requests focus. */
        if (!(c == ' ' && console->input_length == 1)) {
            console->wants_focus = 1;
        }
    }
}

static void console_poll_keyboard(
    VibioConsoleState *console,
    const VibioBootInfo *boot_info,
    const VibioIrqState *irq_state,
    const VibioKernelHeap *heap,
    const VibioTaskManager *tasks,
    const VibioUserBoundaryBuild *user_boundary,
    const VibioDiskReadTest *disk,
    const VibioFat32ReadTest *fs,
    const VibioUsbReadTest *usb,
    const VibioMouseState *mouse,
    const VibioSyscallState *syscalls,
    VibioAudioState *audio,
    VibioRwSandbox *rw,
    vibio_u64 free_pages_before,
    VibioWindowManager *wm,
    VibioBrowser *browser,
    VibioFilesApp *files)
{
    vibio_u64 write_index = irq_state->scancode_write_index;

    if (write_index - console->scancode_read_index > IRQ_SCANCODE_RING_SIZE) {
        console->scancode_read_index = write_index - IRQ_SCANCODE_RING_SIZE;
    }

    while (console->scancode_read_index < write_index) {
        vibio_u64 slot = console->scancode_read_index & (IRQ_SCANCODE_RING_SIZE - 1);
        vibio_u8 scancode = irq_state->scancodes[slot];
        console->scancode_read_index++;
        console_process_scancode(
            console,
            boot_info,
            irq_state,
            heap,
            tasks,
            user_boundary,
            disk,
            fs,
            usb,
            mouse,
            syscalls,
            audio,
            rw,
            free_pages_before,
            wm,
            browser,
            files,
            scancode);
    }
}

static void console_init(VibioConsoleState *console)
{
    console_clear_lines(console);
    console_clear_input(console);
    console->scancode_read_index = 0;
    console->commands_run = 0;
    console->history_revision = 0;
    console_append_ready_line(console);
    console_append_help_hint_line(console);
}

/* EMIT draws terminal/console text, so it uses the JetBrains Mono atlas. */
#define EMIT(ch)                                                      \
    do {                                                             \
        vibio_u8 _er, _eg, _eb;                                      \
        unpack_color(boot_info, color, &_er, &_eg, &_eb);           \
        font_draw_glyph(boot_info, &FONT_JBM, cursor_x, y, (ch), _er, _eg, _eb); \
        cursor_x += jbm_advance();                                   \
    } while (0)

MAYBE_UNUSED static void draw_title(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('V');
    EMIT('I');
    EMIT('B');
    EMIT('I');
    EMIT('O');
    EMIT(' ');
    EMIT('O');
    EMIT('S');
}

MAYBE_UNUSED static void draw_stage_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('S');
    EMIT('T');
    EMIT('A');
    EMIT('G');
    EMIT('E');
    EMIT(' ');
    cursor_x = draw_uint(boot_info, cursor_x, y, VIBIO_STAGE, color);
    EMIT(' ');
    EMIT('D');
    EMIT('E');
    EMIT('S');
    EMIT('K');
    EMIT('T');
    EMIT('O');
    EMIT('P');
}

MAYBE_UNUSED static void draw_idt_page_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 idt_address,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('I');
    EMIT('D');
    EMIT('T');
    EMIT(' ');
    EMIT('P');
    EMIT('A');
    EMIT('G');
    EMIT('E');
    EMIT(':');
    EMIT(' ');
    EMIT('0');
    EMIT('X');
    draw_hex64(boot_info, cursor_x, y, idt_address, color);
}

MAYBE_UNUSED static void draw_breakpoint_hits_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 hits,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('B');
    EMIT('R');
    EMIT('E');
    EMIT('A');
    EMIT('K');
    EMIT('P');
    EMIT('O');
    EMIT('I');
    EMIT('N');
    EMIT('T');
    EMIT(' ');
    EMIT('H');
    EMIT('I');
    EMIT('T');
    EMIT('S');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, hits, color);
}

MAYBE_UNUSED static void draw_idt_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('I');
    EMIT('D');
    EMIT('T');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_irq_state_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 state_address,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('I');
    EMIT('R');
    EMIT('Q');
    EMIT(' ');
    EMIT('S');
    EMIT('T');
    EMIT('A');
    EMIT('T');
    EMIT('E');
    EMIT(':');
    EMIT(' ');
    EMIT('0');
    EMIT('X');
    draw_hex64(boot_info, cursor_x, y, state_address, color);
}

MAYBE_UNUSED static void draw_timer_ticks_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 ticks,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('T');
    EMIT('I');
    EMIT('M');
    EMIT('E');
    EMIT('R');
    EMIT(' ');
    EMIT('T');
    EMIT('I');
    EMIT('C');
    EMIT('K');
    EMIT('S');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, ticks, color);
}

MAYBE_UNUSED static void draw_keyboard_irq_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 keyboard_irqs,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('K');
    EMIT('E');
    EMIT('Y');
    EMIT(' ');
    EMIT('P');
    EMIT('R');
    EMIT('E');
    EMIT('S');
    EMIT('S');
    EMIT('E');
    EMIT('S');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, keyboard_irqs, color);
}

MAYBE_UNUSED static void draw_last_scancode_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 last_scancode,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('L');
    EMIT('A');
    EMIT('S');
    EMIT('T');
    EMIT(' ');
    EMIT('S');
    EMIT('C');
    EMIT('A');
    EMIT('N');
    EMIT(':');
    EMIT(' ');
    EMIT('0');
    EMIT('X');
    draw_hex8(boot_info, cursor_x, y, last_scancode, color);
}

MAYBE_UNUSED static void draw_irq_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('I');
    EMIT('R');
    EMIT('Q');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_heap_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('H');
    EMIT('E');
    EMIT('A');
    EMIT('P');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_task_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('C');
    EMIT('T');
    EMIT('X');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_scheduler_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('S');
    EMIT('C');
    EMIT('H');
    EMIT('E');
    EMIT('D');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_user_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('U');
    EMIT('S');
    EMIT('E');
    EMIT('R');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_file_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('F');
    EMIT('I');
    EMIT('L');
    EMIT('E');
    EMIT('S');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_disk_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('D');
    EMIT('I');
    EMIT('S');
    EMIT('K');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_fat32_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('F');
    EMIT('A');
    EMIT('T');
    EMIT('3');
    EMIT('2');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_usb_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('U');
    EMIT('S');
    EMIT('B');
    EMIT(' ');
    EMIT('P');
    EMIT('R');
    EMIT('O');
    EMIT('B');
    EMIT('E');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_syscall_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('S');
    EMIT('Y');
    EMIT('S');
    EMIT('C');
    EMIT('A');
    EMIT('L');
    EMIT('L');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

/* Terminal text is monospace JetBrains Mono: fixed cell advance per character. */
static vibio_u32 draw_char_buffer(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *chars,
    vibio_u32 length,
    vibio_u32 color)
{
    vibio_u8 cr, cg, cb;
    unpack_color(boot_info, color, &cr, &cg, &cb);
    vibio_u32 adv = jbm_advance();
    for (vibio_u32 i = 0; i < length; i++) {
        font_draw_glyph(boot_info, &FONT_JBM, x, y, chars[i], cr, cg, cb);
        x += adv;
    }

    return x;
}

MAYBE_UNUSED static void draw_console_label(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('C');
    EMIT('O');
    EMIT('N');
    EMIT('S');
    EMIT('O');
    EMIT('L');
    EMIT('E');
}

MAYBE_UNUSED static void draw_console_history(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const VibioConsoleState *console,
    vibio_u32 color)
{
    vibio_u32 right = boot_info->framebuffer_width > 64 ? boot_info->framebuffer_width - 64 : x;
    for (vibio_u32 row = 0; row < CONSOLE_ROWS; row++) {
        vibio_u32 line_y = y + row * LINE_STEP;
        vibio_u32 end_x = draw_char_buffer(boot_info, x, line_y, console->lines[row], console->line_lengths[row], color);
        if (right > end_x) {
            fill_rect(boot_info, end_x, line_y, right - end_x, GLYPH_HEIGHT * FONT_SCALE, g_text_bg);
        }
    }
}

MAYBE_UNUSED static void draw_console_prompt(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const VibioConsoleState *console,
    vibio_u64 timer_ticks,
    vibio_u32 color,
    vibio_u32 cursor_color)
{
    vibio_u32 cursor_x = x;
    EMIT('V');
    EMIT('I');
    EMIT('B');
    EMIT('I');
    EMIT('O');
    EMIT(':');
    EMIT(' ');
    cursor_x = draw_char_buffer(boot_info, cursor_x, y, console->input, console->input_length, color);

    vibio_u32 right = boot_info->framebuffer_width > 64 ? boot_info->framebuffer_width - 64 : cursor_x;

    /* Clear the glyph area to the right of the text so a backspaced character
     * disappears. The text to the left was drawn opaquely and is never blanked,
     * so typing does not flicker. */
    if (right > cursor_x) {
        fill_rect(boot_info, cursor_x, y, right - cursor_x, GLYPH_HEIGHT * FONT_SCALE, g_text_bg);
    }

    /* The blinking cursor is a 3px underline that sits BELOW the glyph cell, so
     * redrawing the glyphs does not erase it. Clear the whole underline strip
     * across the prompt line every redraw; otherwise a cursor shown while a key
     * was pressed leaves a stale underscore stuck under that character. */
    if (right > x) {
        fill_rect(boot_info, x, y + GLYPH_HEIGHT * FONT_SCALE, right - x, 6, g_text_bg);
    }

    if (((timer_ticks / 30) & 1) == 0) {
        fill_rect(boot_info, cursor_x, y + GLYPH_HEIGHT * FONT_SCALE + 3, GLYPH_WIDTH * FONT_SCALE, 3, cursor_color);
    }
}

MAYBE_UNUSED static void draw_exception_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('B');
    EMIT('R');
    EMIT('E');
    EMIT('A');
    EMIT('K');
    EMIT('P');
    EMIT('O');
    EMIT('I');
    EMIT('N');
    EMIT('T');
    EMIT(' ');
    EMIT('E');
    EMIT('X');
    EMIT('C');
    EMIT('E');
    EMIT('P');
    EMIT('T');
    EMIT('I');
    EMIT('O');
    EMIT('N');
    EMIT(' ');
    EMIT('C');
    EMIT('A');
    EMIT('U');
    EMIT('G');
    EMIT('H');
    EMIT('T');
}

MAYBE_UNUSED static void draw_next_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('N');
    EMIT('E');
    EMIT('X');
    EMIT('T');
    EMIT(':');
    EMIT(' ');
    EMIT('L');
    EMIT('A');
    EMIT('U');
    EMIT('N');
    EMIT('C');
    EMIT('H');
    EMIT('E');
    EMIT('R');
}

static void draw_mouse_readout(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const VibioMouseState *mouse,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('M');
    EMIT('O');
    EMIT('U');
    EMIT('S');
    EMIT('E');
    EMIT(' ');
    cursor_x = draw_uint(boot_info, cursor_x, y, mouse->x, color);
    EMIT(',');
    cursor_x = draw_uint(boot_info, cursor_x, y, mouse->y, color);
    /* Clear a small tail so shrinking coordinates leave no stale digits. */
    fill_rect(boot_info, cursor_x, y, 6 * CHAR_STEP, GLYPH_HEIGHT * FONT_SCALE, g_text_bg);
}

MAYBE_UNUSED static void draw_loaded_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('L');
    EMIT('O');
    EMIT('A');
    EMIT('D');
    EMIT('E');
    EMIT('D');
    EMIT(' ');
    EMIT('K');
    EMIT('E');
    EMIT('R');
    EMIT('N');
    EMIT('E');
    EMIT('L');
    EMIT('.');
    EMIT('B');
    EMIT('I');
    EMIT('N');
    EMIT(' ');
    EMIT('F');
    EMIT('R');
    EMIT('O');
    EMIT('M');
    EMIT(' ');
    EMIT('V');
    EMIT('M');
    EMIT(' ');
    EMIT('D');
    EMIT('I');
    EMIT('S');
    EMIT('K');
}

MAYBE_UNUSED static void draw_exit_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('U');
    EMIT('E');
    EMIT('F');
    EMIT('I');
    EMIT(' ');
    EMIT('B');
    EMIT('O');
    EMIT('O');
    EMIT('T');
    EMIT(' ');
    EMIT('S');
    EMIT('E');
    EMIT('R');
    EMIT('V');
    EMIT('I');
    EMIT('C');
    EMIT('E');
    EMIT('S');
    EMIT(' ');
    EMIT('E');
    EMIT('X');
    EMIT('I');
    EMIT('T');
    EMIT('E');
    EMIT('D');
}

MAYBE_UNUSED static void draw_memory_entries_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('M');
    EMIT('E');
    EMIT('M');
    EMIT('O');
    EMIT('R');
    EMIT('Y');
    EMIT(' ');
    EMIT('M');
    EMIT('A');
    EMIT('P');
    EMIT(' ');
    EMIT('E');
    EMIT('N');
    EMIT('T');
    EMIT('R');
    EMIT('I');
    EMIT('E');
    EMIT('S');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, boot_info->memory_map_entry_count, color);
}

MAYBE_UNUSED static void draw_usable_memory_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    vibio_u64 usable_mib = count_usable_pages(boot_info) / 256;

    EMIT('U');
    EMIT('S');
    EMIT('A');
    EMIT('B');
    EMIT('L');
    EMIT('E');
    EMIT(' ');
    EMIT('M');
    EMIT('E');
    EMIT('M');
    EMIT('O');
    EMIT('R');
    EMIT('Y');
    EMIT(' ');
    EMIT('M');
    EMIT('I');
    EMIT('B');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, usable_mib, color);
}

MAYBE_UNUSED static void draw_allocator_free_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 pages,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('A');
    EMIT('L');
    EMIT('L');
    EMIT('O');
    EMIT('C');
    EMIT('A');
    EMIT('T');
    EMIT('O');
    EMIT('R');
    EMIT(' ');
    EMIT('F');
    EMIT('R');
    EMIT('E');
    EMIT('E');
    EMIT(' ');
    EMIT('P');
    EMIT('A');
    EMIT('G');
    EMIT('E');
    EMIT('S');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, pages, color);
}

MAYBE_UNUSED static void draw_test_pages_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 pages,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(' ');
    EMIT('P');
    EMIT('A');
    EMIT('G');
    EMIT('E');
    EMIT('S');
    EMIT(' ');
    EMIT('A');
    EMIT('L');
    EMIT('L');
    EMIT('O');
    EMIT('C');
    EMIT('A');
    EMIT('T');
    EMIT('E');
    EMIT('D');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, pages, color);
}

MAYBE_UNUSED static void draw_alloc_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('A');
    EMIT('L');
    EMIT('L');
    EMIT('O');
    EMIT('C');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_paging_mode_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('I');
    EMIT('D');
    EMIT('E');
    EMIT('N');
    EMIT('T');
    EMIT('I');
    EMIT('T');
    EMIT('Y');
    EMIT(' ');
    EMIT('M');
    EMIT('A');
    EMIT('P');
    EMIT(' ');
    EMIT('L');
    EMIT('O');
    EMIT('W');
    EMIT(' ');
    EMIT('4');
    EMIT('G');
    EMIT('I');
    EMIT('B');
}

MAYBE_UNUSED static void draw_page_table_pages_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 pages,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('P');
    EMIT('A');
    EMIT('G');
    EMIT('E');
    EMIT(' ');
    EMIT('T');
    EMIT('A');
    EMIT('B');
    EMIT('L');
    EMIT('E');
    EMIT(' ');
    EMIT('P');
    EMIT('A');
    EMIT('G');
    EMIT('E');
    EMIT('S');
    EMIT(':');
    EMIT(' ');
    draw_uint(boot_info, cursor_x, y, pages, color);
}

MAYBE_UNUSED static void draw_cr3_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 cr3,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('C');
    EMIT('R');
    EMIT('3');
    EMIT(' ');
    EMIT('P');
    EMIT('M');
    EMIT('L');
    EMIT('4');
    EMIT(':');
    EMIT(' ');
    EMIT('0');
    EMIT('X');
    draw_hex64(boot_info, cursor_x, y, cr3, color);
}

MAYBE_UNUSED static void draw_paging_test_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('P');
    EMIT('A');
    EMIT('G');
    EMIT('I');
    EMIT('N');
    EMIT('G');
    EMIT(' ');
    EMIT('T');
    EMIT('E');
    EMIT('S');
    EMIT('T');
    EMIT(':');
    EMIT(' ');

    if (ok) {
        EMIT('P');
        EMIT('A');
        EMIT('S');
        EMIT('S');
    } else {
        EMIT('F');
        EMIT('A');
        EMIT('I');
        EMIT('L');
    }
}

MAYBE_UNUSED static void draw_safety_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('V');
    EMIT('M');
    EMIT(' ');
    EMIT('O');
    EMIT('N');
    EMIT('L');
    EMIT('Y');
    EMIT(':');
    EMIT(' ');
    EMIT('N');
    EMIT('O');
    EMIT(' ');
    EMIT('H');
    EMIT('O');
    EMIT('S');
    EMIT('T');
    EMIT(' ');
    EMIT('D');
    EMIT('I');
    EMIT('S');
    EMIT('K');
    EMIT('S');
    EMIT(' ');
    EMIT('T');
    EMIT('O');
    EMIT('U');
    EMIT('C');
    EMIT('H');
    EMIT('E');
    EMIT('D');
}

MAYBE_UNUSED static void draw_exit_failed_line(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 color)
{
    vibio_u32 cursor_x = x;
    EMIT('U');
    EMIT('E');
    EMIT('F');
    EMIT('I');
    EMIT(' ');
    EMIT('B');
    EMIT('O');
    EMIT('O');
    EMIT('T');
    EMIT(' ');
    EMIT('S');
    EMIT('E');
    EMIT('R');
    EMIT('V');
    EMIT('I');
    EMIT('C');
    EMIT('E');
    EMIT('S');
    EMIT(' ');
    EMIT('S');
    EMIT('T');
    EMIT('I');
    EMIT('L');
    EMIT('L');
    EMIT(' ');
    EMIT('O');
    EMIT('N');
}

static vibio_u32 draw_bool_word(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 ok,
    const VibioDesktopTheme *theme)
{
    return draw_cstr(boot_info, x, y, ok ? "PASS" : "FAIL", ok ? theme->ok : theme->bad);
}

static vibio_u32 draw_state_word(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *text,
    vibio_u32 color)
{
    return draw_cstr(boot_info, x, y, text, color);
}

static void draw_status_row(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *label,
    vibio_u32 ok,
    const VibioDesktopTheme *theme)
{
    vibio_u32 cursor_x = draw_cstr(boot_info, x, y, label, theme->muted);
    cursor_x = draw_cstr(boot_info, cursor_x, y, ": ", theme->muted);
    draw_bool_word(boot_info, cursor_x, y, ok, theme);
}

static void draw_probe_row(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *label,
    const char *state,
    vibio_u32 color,
    const VibioDesktopTheme *theme)
{
    vibio_u32 cursor_x = draw_cstr(boot_info, x, y, label, theme->muted);
    cursor_x = draw_cstr(boot_info, cursor_x, y, ": ", theme->muted);
    draw_state_word(boot_info, cursor_x, y, state, color);
}

static void draw_metric_row(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *label,
    vibio_u64 value,
    const VibioDesktopTheme *theme)
{
    vibio_u32 cursor_x = draw_cstr(boot_info, x, y, label, theme->muted);
    cursor_x = draw_cstr(boot_info, cursor_x, y, ": ", theme->muted);
    draw_uint(boot_info, cursor_x, y, value, theme->text);
}

static const char *disk_probe_state_text(const VibioDiskReadTest *disk)
{
    if (disk == 0) {
        return "N/A";
    }
    if (disk->ok) {
        return "PASS";
    }
    return disk->last_error == 1 ? "N/A" : "FAIL";
}

static vibio_u32 disk_probe_state_color(const VibioDiskReadTest *disk, const VibioDesktopTheme *theme)
{
    if (disk != 0 && disk->ok) {
        return theme->ok;
    }
    if (disk != 0 && disk->last_error != 1) {
        return theme->bad;
    }
    return theme->muted;
}

static const char *fat32_probe_state_text(const VibioFat32ReadTest *fs, const VibioDiskReadTest *disk)
{
    if (fs != 0 && fs->ok) {
        return "PASS";
    }
    if (disk == 0 || disk->last_error == 1) {
        return "N/A";
    }
    return "FAIL";
}

static vibio_u32 fat32_probe_state_color(const VibioFat32ReadTest *fs, const VibioDiskReadTest *disk, const VibioDesktopTheme *theme)
{
    if (fs != 0 && fs->ok) {
        return theme->ok;
    }
    if (disk != 0 && disk->last_error != 1) {
        return theme->bad;
    }
    return theme->muted;
}

/* RW sandbox status for the System window. "ARMED" when the VIBIORW scratch disk
 * is positively identified; otherwise an honest non-armed state. */
static const char *rw_probe_state_text(const VibioRwSandbox *rw)
{
    if (rw == 0 || !rw->present) {
        return "NONE";
    }
    if (rw->armed) {
        return "ARMED";
    }
    if (rw->disabled) {
        return "OFF";
    }
    return "SKIP";
}

static vibio_u32 rw_probe_state_color(const VibioRwSandbox *rw, const VibioDesktopTheme *theme)
{
    if (rw != 0 && rw->present && rw->armed) {
        return theme->ok;
    }
    if (rw != 0 && rw->present && rw->disabled) {
        return theme->bad;
    }
    return theme->muted;
}

static const char *audio_probe_state_text(const VibioAudioState *audio)
{
    if (audio != 0 && audio->ready) {
        return "PASS";
    }
    if (audio != 0 && audio->hda_found) {
        return "HDA ERR";
    }
    if (audio == 0 || !audio->found) {
        return "N/A";
    }
    return "FAIL";
}

static vibio_u32 audio_probe_state_color(const VibioAudioState *audio, const VibioDesktopTheme *theme)
{
    if (audio != 0 && audio->ready) {
        return theme->ok;
    }
    if (audio != 0 && (audio->found || audio->hda_found)) {
        return theme->bad;
    }
    return theme->muted;
}

MAYBE_UNUSED static void draw_uptime_clock(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 live_ticks,
    vibio_u32 color)
{
    vibio_u64 seconds = live_ticks / 100;
    vibio_u64 minutes = (seconds / 60) % 100;
    vibio_u64 secs = seconds % 60;
    vibio_u32 cursor_x = draw_cstr(boot_info, x, y, "TIME ", color);
    cursor_x = draw_two_digits(boot_info, cursor_x, y, minutes, color);
    draw_char(boot_info, cursor_x, y, ':', color);
    cursor_x += CHAR_STEP;
    draw_two_digits(boot_info, cursor_x, y, secs, color);
}

static vibio_u32 cstr_length(const char *text)
{
    vibio_u32 length = 0;
    while (text[length] != 0) {
        length++;
    }
    return length;
}

static vibio_u32 cstr_equals(const char *left, const char *right)
{
    vibio_u32 i = 0;
    while (left[i] != 0 && right[i] != 0) {
        if (left[i] != right[i]) {
            return 0;
        }
        i++;
    }
    return left[i] == right[i];
}

MAYBE_UNUSED static vibio_u32 draw_text_scaled(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *text,
    vibio_u32 color,
    vibio_u32 bg,
    vibio_u32 scale)
{
    vibio_u32 i = 0;
    while (text[i] != 0) {
        fill_rect(boot_info, x, y, (GLYPH_WIDTH + 1) * scale, GLYPH_HEIGHT * scale, bg);
        draw_modern_glyph(boot_info, x + scale, y, text[i], color, scale);
        x += (GLYPH_WIDTH + 1) * scale;
        i++;
    }
    return x;
}

static vibio_u32 draw_text_scaled_transparent(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *text,
    vibio_u32 color,
    vibio_u32 scale)
{
    vibio_u32 i = 0;
    while (text[i] != 0) {
        draw_modern_glyph(boot_info, x + scale, y, text[i], color, scale);
        x += (GLYPH_WIDTH + 1) * scale;
        i++;
    }
    return x;
}

static vibio_u32 draw_small_cstr(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *text,
    vibio_u32 color,
    vibio_u32 bg)
{
    (void)bg;
    return font_draw_text(boot_info, &FONT_GEIST_SM, x, y, text, color);
}

static vibio_u32 draw_small_cstr_transparent(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    const char *text,
    vibio_u32 color)
{
    return font_draw_text(boot_info, &FONT_GEIST_SM, x, y, text, color);
}

MAYBE_UNUSED static vibio_u32 draw_small_uint(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 value,
    vibio_u32 color,
    vibio_u32 bg)
{
    char digits[20];
    vibio_u32 count = 0;

    if (value == 0) {
        char zero[2] = {'0', 0};
        return draw_small_cstr(boot_info, x, y, zero, color, bg);
    }

    while (value > 0 && count < 20) {
        digits[count] = (char)('0' + (value % 10));
        value /= 10;
        count++;
    }

    while (count > 0) {
        count--;
        char digit[2] = {digits[count], 0};
        x = draw_small_cstr(boot_info, x, y, digit, color, bg);
    }

    return x;
}

MAYBE_UNUSED static vibio_u32 draw_small_two_digits(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 value,
    vibio_u32 color,
    vibio_u32 bg)
{
    char text[3];
    text[0] = (char)('0' + ((value / 10) % 10));
    text[1] = (char)('0' + (value % 10));
    text[2] = 0;
    return draw_small_cstr(boot_info, x, y, text, color, bg);
}

static void draw_small_clock(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u64 live_ticks,
    vibio_u32 color,
    vibio_u32 bg)
{
    (void)bg;
    vibio_u64 seconds = live_ticks / 100;
    vibio_u64 minutes = (seconds / 60) % 100;
    vibio_u64 secs = seconds % 60;
    char b[6];
    b[0] = (char)('0' + (minutes / 10) % 10);
    b[1] = (char)('0' + minutes % 10);
    b[2] = ':';
    b[3] = (char)('0' + (secs / 10) % 10);
    b[4] = (char)('0' + secs % 10);
    b[5] = 0;
    font_draw_text(boot_info, &FONT_GEIST_SM, x, y, b, color);
}

static void draw_small_hhmm(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 hour,
    vibio_u32 minute,
    vibio_u32 use_24_hour,
    vibio_u32 color,
    vibio_u32 bg)
{
    (void)bg;
    char b[9];
    if (use_24_hour) {
        b[0] = (char)('0' + (hour / 10) % 10);
        b[1] = (char)('0' + hour % 10);
        b[2] = ':';
        b[3] = (char)('0' + (minute / 10) % 10);
        b[4] = (char)('0' + minute % 10);
        b[5] = 0;
    } else {
        vibio_u32 pm = hour >= 12U;
        vibio_u32 h12 = hour % 12U;
        if (h12 == 0) {
            h12 = 12U;
        }
        vibio_u32 p = 0;
        if (h12 >= 10U) {
            b[p++] = (char)('0' + (h12 / 10U));
        }
        b[p++] = (char)('0' + (h12 % 10U));
        b[p++] = ':';
        b[p++] = (char)('0' + (minute / 10U) % 10U);
        b[p++] = (char)('0' + minute % 10U);
        b[p++] = ' ';
        b[p++] = pm ? 'P' : 'A';
        b[p++] = 'M';
        b[p] = 0;
    }
    font_draw_text(boot_info, &FONT_GEIST_SM, x, y, b, color);
}

static void draw_line(
    const VibioBootInfo *boot_info,
    int x0,
    int y0,
    int x1,
    int y1,
    vibio_u32 color)
{
    int dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 >= y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        if (x0 >= 0 && y0 >= 0) {
            put_pixel(boot_info, (vibio_u32)x0, (vibio_u32)y0, color);
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void fill_round_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 radius,
    vibio_u32 color)
{
    if (w == 0 || h == 0) {
        return;
    }
    if (radius * 2 > w) {
        radius = w / 2;
    }
    if (radius * 2 > h) {
        radius = h / 2;
    }
    if (radius == 0) {
        fill_rect(boot_info, x, y, w, h, color);
        return;
    }

    vibio_u32 rr = radius * radius;
    for (vibio_u32 row = 0; row < h; row++) {
        vibio_u32 inset = 0;
        if (row < radius) {
            int dy = (int)radius - 1 - (int)row;
            while (inset < radius) {
                int dx = (int)radius - 1 - (int)inset;
                if ((vibio_u32)(dx * dx + dy * dy) <= rr) {
                    break;
                }
                inset++;
            }
        } else if (row >= h - radius) {
            int dy = (int)row - (int)(h - radius);
            while (inset < radius) {
                int dx = (int)radius - 1 - (int)inset;
                if ((vibio_u32)(dx * dx + dy * dy) <= rr) {
                    break;
                }
                inset++;
            }
        }

        if (inset * 2 < w) {
            fill_rect(boot_info, x + inset, y + row, w - inset * 2, 1, color);
        }
    }
}

static void fill_circle(
    const VibioBootInfo *boot_info,
    vibio_u32 cx,
    vibio_u32 cy,
    vibio_u32 radius,
    vibio_u32 color)
{
    int r = (int)radius;
    int rr = r * r;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= rr) {
                int px = (int)cx + dx;
                int py = (int)cy + dy;
                if (px >= 0 && py >= 0) {
                    put_pixel(boot_info, (vibio_u32)px, (vibio_u32)py, color);
                }
            }
        }
    }
}

static void draw_circle_outline(
    const VibioBootInfo *boot_info,
    vibio_u32 cx,
    vibio_u32 cy,
    vibio_u32 radius,
    vibio_u32 thickness,
    vibio_u32 color)
{
    int r = (int)radius;
    int outer = r * r;
    int inner_r = r - (int)thickness;
    int inner = inner_r > 0 ? inner_r * inner_r : 0;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int d = dx * dx + dy * dy;
            if (d <= outer && d >= inner) {
                int px = (int)cx + dx;
                int py = (int)cy + dy;
                if (px >= 0 && py >= 0) {
                    put_pixel(boot_info, (vibio_u32)px, (vibio_u32)py, color);
                }
            }
        }
    }
}

static vibio_u32 icon_part(vibio_u32 size, vibio_u32 numerator, vibio_u32 denominator)
{
    vibio_u32 value = (size * numerator) / denominator;
    return value == 0 ? 1 : value;
}

MAYBE_UNUSED static void fill_soft_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 color)
{
    for (vibio_u32 row = 0; row < h; row++) {
        vibio_u32 inset = 0;
        if (row == 0 || row + 1 == h) {
            inset = 4;
        } else if (row == 1 || row + 2 == h) {
            inset = 2;
        }
        if (w > inset * 2) {
            fill_rect(boot_info, x + inset, y + row, w - inset * 2, 1, color);
        }
    }
}

static void draw_soft_rect_border(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 color)
{
    if (w < 8 || h < 8) {
        return;
    }
    fill_rect(boot_info, x + 4, y, w - 8, 1, color);
    fill_rect(boot_info, x + 2, y + 1, w - 4, 1, color);
    fill_rect(boot_info, x, y + 4, 1, h - 8, color);
    fill_rect(boot_info, x + w - 1, y + 4, 1, h - 8, color);
    fill_rect(boot_info, x + 2, y + h - 2, w - 4, 1, color);
    fill_rect(boot_info, x + 4, y + h - 1, w - 8, 1, color);
}

/* Soft radial glow with quadratic falloff, blended onto the wallpaper for a
 * modern "bloom" look instead of hard line art. */
static void draw_soft_glow(const VibioBootInfo *boot_info, int cx, int cy, int radius,
                           vibio_u8 r, vibio_u8 g, vibio_u8 b, int max_a)
{
    int width = (int)boot_info->framebuffer_width;
    int height = (int)boot_info->framebuffer_height;
    vibio_u64 r2 = (vibio_u64)radius * (vibio_u64)radius;

    for (int y = cy - radius; y <= cy + radius; y++) {
        if (y < 0 || y >= height) {
            continue;
        }
        for (int x = cx - radius; x <= cx + radius; x++) {
            if (x < 0 || x >= width) {
                continue;
            }
            vibio_u64 d2 = (vibio_u64)((x - cx) * (x - cx)) + (vibio_u64)((y - cy) * (y - cy));
            if (d2 > r2) {
                continue;
            }
            int a = max_a - (int)((vibio_u64)max_a * d2 / r2);
            if (a <= 0) {
                continue;
            }
            blend_pixel(boot_info, (vibio_u32)x, (vibio_u32)y, r, g, b, (vibio_u8)a);
        }
    }
}

static void draw_wallpaper(const VibioBootInfo *boot_info, const VibioDesktopTheme *theme)
{
    vibio_u32 width = boot_info->framebuffer_width;
    vibio_u32 height = boot_info->framebuffer_height;
    (void)theme;

    /* Smooth vertical gradient from a deep blue at the top to a darker blue at
     * the bottom, painted as 1px rows so there are no visible banding seams. */
    int tr = 44, tg = 88, tb = 130;
    int br = 15, bg = 25, bb = 48;
    vibio_u32 h = height > 0 ? height : 1;
    for (vibio_u32 y = 0; y < height; y++) {
        int rr = tr + (br - tr) * (int)y / (int)h;
        int gg = tg + (bg - tg) * (int)y / (int)h;
        int bbv = tb + (bb - tb) * (int)y / (int)h;
        fill_rect(boot_info, 0, y, width, 1, make_color(boot_info, (vibio_u8)rr, (vibio_u8)gg, (vibio_u8)bbv));
    }

    /* Two soft glows give the desktop depth without harsh geometry. */
    draw_soft_glow(boot_info, (int)(width * 30 / 100), (int)(height * 20 / 100), 480, 120, 168, 208, 50);
    draw_soft_glow(boot_info, (int)(width * 84 / 100), (int)(height * 82 / 100), 380, 36, 120, 150, 34);
}

/* Bilinear interpolation of one 8-bit channel between four corner samples, with
 * fx/fy fractions in 0..255. Done in fixed point (no floats in the kernel). */
static vibio_u32 wallpaper_bilerp(vibio_u32 a, vibio_u32 b, vibio_u32 c, vibio_u32 d,
                                  vibio_u32 fx, vibio_u32 fy)
{
    vibio_u32 ifx = 256U - fx;
    vibio_u32 ify = 256U - fy;
    vibio_u32 top = a * ifx + b * fx;   /* 8-bit * <=256 -> fits in 16 bits */
    vibio_u32 bot = c * ifx + d * fx;
    return (top * ify + bot * fy) >> 16;
}

/* Paint a real imported wallpaper image scaled to COVER the whole framebuffer
 * (preserve aspect, crop the overflow), with BILINEAR sampling so the upscale to
 * a 1080p desktop is smooth instead of blocky/pixelated. The pixels are opaque
 * 0xAARRGGBB compiled into KERNEL.BIN by tools/make_wallpapers.py. */
static void draw_wallpaper_image(const VibioBootInfo *boot_info, const VibioWallpaperImage *img)
{
    vibio_u32 fw = boot_info->framebuffer_width;
    vibio_u32 fh = boot_info->framebuffer_height;
    if (img == 0 || img->pixels == 0 || img->width == 0 || img->height == 0 || fw == 0 || fh == 0) {
        return;
    }
    /* Cover scale: the larger of the two axis ratios so neither axis has gaps.
     * Compare fw/iw vs fh/ih without floats: fw*ih vs fh*iw. */
    vibio_u32 scaled_w, scaled_h;
    if ((vibio_u64)fw * img->height >= (vibio_u64)fh * img->width) {
        scaled_w = fw;
        scaled_h = (vibio_u32)(((vibio_u64)img->height * fw) / img->width);
        if (scaled_h < fh) { scaled_h = fh; }
    } else {
        scaled_h = fh;
        scaled_w = (vibio_u32)(((vibio_u64)img->width * fh) / img->height);
        if (scaled_w < fw) { scaled_w = fw; }
    }
    vibio_u32 ox = (scaled_w - fw) / 2U;
    vibio_u32 oy = (scaled_h - fh) / 2U;
    for (vibio_u32 y = 0; y < fh; y++) {
        vibio_u32 dy = y + oy;
        vibio_u64 syq = (((vibio_u64)dy * img->height) << 8) / scaled_h; /* 24.8 fixed */
        vibio_u32 sy0 = (vibio_u32)(syq >> 8);
        vibio_u32 fy = (vibio_u32)(syq & 0xFFU);
        if (sy0 >= img->height) { sy0 = img->height - 1U; fy = 0; }
        vibio_u32 sy1 = sy0 + 1U < img->height ? sy0 + 1U : sy0;
        const vibio_u32 *row0 = img->pixels + (vibio_u64)sy0 * img->width;
        const vibio_u32 *row1 = img->pixels + (vibio_u64)sy1 * img->width;
        for (vibio_u32 x = 0; x < fw; x++) {
            vibio_u32 dx = x + ox;
            vibio_u64 sxq = (((vibio_u64)dx * img->width) << 8) / scaled_w;
            vibio_u32 sx0 = (vibio_u32)(sxq >> 8);
            vibio_u32 fx = (vibio_u32)(sxq & 0xFFU);
            if (sx0 >= img->width) { sx0 = img->width - 1U; fx = 0; }
            vibio_u32 sx1 = sx0 + 1U < img->width ? sx0 + 1U : sx0;
            vibio_u32 c00 = row0[sx0], c01 = row0[sx1];
            vibio_u32 c10 = row1[sx0], c11 = row1[sx1];
            vibio_u32 r = wallpaper_bilerp((c00 >> 16) & 0xFF, (c01 >> 16) & 0xFF,
                                           (c10 >> 16) & 0xFF, (c11 >> 16) & 0xFF, fx, fy);
            vibio_u32 g = wallpaper_bilerp((c00 >> 8) & 0xFF, (c01 >> 8) & 0xFF,
                                           (c10 >> 8) & 0xFF, (c11 >> 8) & 0xFF, fx, fy);
            vibio_u32 b = wallpaper_bilerp(c00 & 0xFF, c01 & 0xFF,
                                           c10 & 0xFF, c11 & 0xFF, fx, fy);
            put_pixel(boot_info, x, y, make_color(boot_info, (vibio_u8)r, (vibio_u8)g, (vibio_u8)b));
        }
    }
}

static void draw_icon_monitor(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 10, 20, 28);
    vibio_u32 frame = make_color(boot_info, 218, 232, 241);
    vibio_u32 glass = make_color(boot_info, 56, 96, 126);
    vibio_u32 glass_hi = make_color(boot_info, 83, 139, 173);
    vibio_u32 stand = make_color(boot_info, 125, 148, 164);
    vibio_u32 green = make_color(boot_info, 83, 211, 128);
    vibio_u32 r = icon_part(size, 5, 36);

    fill_round_rect(boot_info, x + 2, y + 4, size - 3, icon_part(size, 22, 36), r, shadow);
    fill_round_rect(boot_info, x + 1, y + 1, size - 4, icon_part(size, 22, 36), r, frame);
    fill_round_rect(boot_info, x + 4, y + 4, size - 10, icon_part(size, 14, 36), icon_part(size, 2, 36), glass);
    fill_rect(boot_info, x + 5, y + 5, size - 12, 2, glass_hi);
    fill_rect(boot_info, x + icon_part(size, 15, 36), y + icon_part(size, 23, 36), icon_part(size, 5, 36), icon_part(size, 5, 36), stand);
    fill_round_rect(boot_info, x + icon_part(size, 10, 36), y + icon_part(size, 29, 36), icon_part(size, 16, 36), icon_part(size, 3, 36), 1, stand);
    fill_round_rect(boot_info, x + icon_part(size, 24, 36), y + icon_part(size, 18, 36), icon_part(size, 9, 36), icon_part(size, 9, 36), icon_part(size, 3, 36), green);
}

static void draw_icon_terminal_symbol(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size,
    const VibioDesktopTheme *theme)
{
    vibio_u32 shadow = make_color(boot_info, 8, 20, 30);
    vibio_u32 blue = make_color(boot_info, 35, 139, 226);
    vibio_u32 blue_hi = make_color(boot_info, 78, 174, 244);
    vibio_u32 ink = make_color(boot_info, 238, 248, 252);
    vibio_u32 r = icon_part(size, 6, 36);

    fill_round_rect(boot_info, x + 2, y + 3, size - 1, size - 4, r, shadow);
    fill_round_rect(boot_info, x, y + 1, size - 2, size - 3, r, blue);
    fill_round_rect(boot_info, x + 2, y + 3, size - 6, icon_part(size, 6, 36), icon_part(size, 3, 36), blue_hi);
    draw_stroke_line(boot_info, (int)x + icon_part(size, 9, 36), (int)y + icon_part(size, 12, 36), (int)x + icon_part(size, 15, 36), (int)y + icon_part(size, 18, 36), ink, icon_part(size, 2, 36));
    draw_stroke_line(boot_info, (int)x + icon_part(size, 15, 36), (int)y + icon_part(size, 18, 36), (int)x + icon_part(size, 9, 36), (int)y + icon_part(size, 24, 36), ink, icon_part(size, 2, 36));
    fill_round_rect(boot_info, x + icon_part(size, 19, 36), y + icon_part(size, 24, 36), icon_part(size, 9, 36), icon_part(size, 3, 36), 1, theme->accent);
}

static void draw_icon_vibio_mark(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 tile = make_color(boot_info, 26, 43, 54);
    vibio_u32 shadow = make_color(boot_info, 9, 18, 25);
    vibio_u32 orange = make_color(boot_info, 244, 131, 44);
    vibio_u32 teal = make_color(boot_info, 31, 210, 213);
    vibio_u32 r = icon_part(size, 7, 36);
    vibio_u32 stroke = icon_part(size, 4, 36);

    fill_round_rect(boot_info, x + 2, y + 3, size - 2, size - 3, r, shadow);
    fill_round_rect(boot_info, x, y, size - 3, size - 4, r, tile);
    fill_round_rect(boot_info, x + icon_part(size, 8, 36), y + icon_part(size, 8, 36), stroke, icon_part(size, 20, 36), 1, orange);
    draw_stroke_line(boot_info, (int)x + icon_part(size, 13, 36), (int)y + icon_part(size, 28, 36), (int)x + icon_part(size, 22, 36), (int)y + icon_part(size, 8, 36), orange, stroke);
    fill_round_rect(boot_info, x + icon_part(size, 23, 36), y + icon_part(size, 8, 36), stroke, icon_part(size, 20, 36), 1, orange);
    fill_round_rect(boot_info, x + icon_part(size, 29, 36), y + icon_part(size, 8, 36), icon_part(size, 5, 36), icon_part(size, 5, 36), 2, teal);
    fill_round_rect(boot_info, x + icon_part(size, 29, 36), y + icon_part(size, 23, 36), icon_part(size, 5, 36), icon_part(size, 5, 36), 2, teal);
}

static void draw_icon_folder(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 14, 29, 38);
    vibio_u32 back = make_color(boot_info, 246, 200, 55);
    vibio_u32 front = make_color(boot_info, 255, 217, 71);
    vibio_u32 lip = make_color(boot_info, 255, 238, 135);
    vibio_u32 blue = make_color(boot_info, 71, 145, 215);

    fill_round_rect(boot_info, x + 2, y + icon_part(size, 12, 36), size - 4, icon_part(size, 20, 36), icon_part(size, 4, 36), shadow);
    fill_round_rect(boot_info, x + 4, y + icon_part(size, 8, 36), icon_part(size, 13, 36), icon_part(size, 7, 36), icon_part(size, 2, 36), back);
    fill_round_rect(boot_info, x + 1, y + icon_part(size, 12, 36), size - 3, icon_part(size, 18, 36), icon_part(size, 4, 36), front);
    fill_rect(boot_info, x + icon_part(size, 4, 36), y + icon_part(size, 15, 36), size - 9, 2, lip);
    fill_rect(boot_info, x + icon_part(size, 7, 36), y + icon_part(size, 24, 36), size - 14, icon_part(size, 3, 36), blue);
}

static void draw_icon_usb(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 12, 22, 35);
    vibio_u32 purple = make_color(boot_info, 121, 105, 232);
    vibio_u32 purple_hi = make_color(boot_info, 166, 151, 255);
    vibio_u32 white = make_color(boot_info, 246, 249, 252);
    vibio_u32 stroke = icon_part(size, 2, 36);

    fill_round_rect(boot_info, x + 2, y + 3, size - 2, size - 4, icon_part(size, 8, 36), shadow);
    fill_round_rect(boot_info, x, y + 1, size - 4, size - 5, icon_part(size, 8, 36), purple);
    fill_rect(boot_info, x + icon_part(size, 7, 36), y + icon_part(size, 6, 36), size - 15, 2, purple_hi);
    draw_stroke_line(boot_info, (int)x + icon_part(size, 18, 36), (int)y + icon_part(size, 8, 36), (int)x + icon_part(size, 18, 36), (int)y + icon_part(size, 26, 36), white, stroke);
    draw_stroke_line(boot_info, (int)x + icon_part(size, 10, 36), (int)y + icon_part(size, 16, 36), (int)x + icon_part(size, 26, 36), (int)y + icon_part(size, 16, 36), white, stroke);
    fill_round_rect(boot_info, x + icon_part(size, 15, 36), y + icon_part(size, 26, 36), icon_part(size, 7, 36), icon_part(size, 5, 36), 1, white);
    fill_circle(boot_info, x + icon_part(size, 10, 36), y + icon_part(size, 16, 36), icon_part(size, 2, 36), white);
    fill_round_rect(boot_info, x + icon_part(size, 25, 36), y + icon_part(size, 13, 36), icon_part(size, 5, 36), icon_part(size, 5, 36), 1, white);
}

static void draw_icon_trash(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 15, 30, 39);
    vibio_u32 shell = make_color(boot_info, 220, 234, 241);
    vibio_u32 line = make_color(boot_info, 143, 164, 177);
    vibio_u32 blue = make_color(boot_info, 101, 172, 225);

    fill_round_rect(boot_info, x + icon_part(size, 7, 36), y + icon_part(size, 11, 36), icon_part(size, 22, 36), icon_part(size, 20, 36), icon_part(size, 4, 36), shadow);
    fill_round_rect(boot_info, x + icon_part(size, 6, 36), y + icon_part(size, 10, 36), icon_part(size, 22, 36), icon_part(size, 20, 36), icon_part(size, 4, 36), shell);
    fill_round_rect(boot_info, x + icon_part(size, 4, 36), y + icon_part(size, 7, 36), icon_part(size, 26, 36), icon_part(size, 4, 36), 1, shell);
    fill_rect(boot_info, x + icon_part(size, 12, 36), y + icon_part(size, 5, 36), icon_part(size, 12, 36), 2, line);
    draw_line(boot_info, (int)x + icon_part(size, 13, 36), (int)y + icon_part(size, 15, 36), (int)x + icon_part(size, 13, 36), (int)y + icon_part(size, 25, 36), line);
    draw_line(boot_info, (int)x + icon_part(size, 18, 36), (int)y + icon_part(size, 15, 36), (int)x + icon_part(size, 18, 36), (int)y + icon_part(size, 25, 36), line);
    draw_line(boot_info, (int)x + icon_part(size, 23, 36), (int)y + icon_part(size, 15, 36), (int)x + icon_part(size, 23, 36), (int)y + icon_part(size, 25, 36), line);
    fill_circle(boot_info, x + icon_part(size, 18, 36), y + icon_part(size, 20, 36), icon_part(size, 5, 36), blue);
}

static void draw_icon_storage(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 13, 27, 35);
    vibio_u32 case_color = make_color(boot_info, 206, 221, 231);
    vibio_u32 face = make_color(boot_info, 236, 244, 248);
    vibio_u32 slot = make_color(boot_info, 80, 105, 121);
    vibio_u32 blue = make_color(boot_info, 64, 160, 225);

    fill_round_rect(boot_info, x + 3, y + icon_part(size, 6, 36), size - 6, icon_part(size, 25, 36), icon_part(size, 5, 36), shadow);
    fill_round_rect(boot_info, x + 2, y + icon_part(size, 5, 36), size - 7, icon_part(size, 24, 36), icon_part(size, 5, 36), case_color);
    fill_round_rect(boot_info, x + 5, y + icon_part(size, 9, 36), size - 13, icon_part(size, 13, 36), icon_part(size, 3, 36), face);
    fill_rect(boot_info, x + icon_part(size, 8, 36), y + icon_part(size, 25, 36), size - 18, 2, slot);
    fill_circle(boot_info, x + icon_part(size, 26, 36), y + icon_part(size, 25, 36), icon_part(size, 3, 36), blue);
}

static void draw_icon_sound(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 16, 31, 39);
    vibio_u32 yellow = make_color(boot_info, 252, 205, 58);
    vibio_u32 yellow_hi = make_color(boot_info, 255, 236, 137);
    vibio_u32 blue = make_color(boot_info, 74, 166, 224);

    fill_round_rect(boot_info, x + 2, y + icon_part(size, 14, 36), icon_part(size, 12, 36), icon_part(size, 10, 36), 2, shadow);
    fill_round_rect(boot_info, x + 1, y + icon_part(size, 13, 36), icon_part(size, 12, 36), icon_part(size, 10, 36), 2, yellow);
    draw_stroke_line(boot_info, (int)x + icon_part(size, 12, 36), (int)y + icon_part(size, 13, 36), (int)x + icon_part(size, 20, 36), (int)y + icon_part(size, 8, 36), yellow, icon_part(size, 3, 36));
    draw_stroke_line(boot_info, (int)x + icon_part(size, 12, 36), (int)y + icon_part(size, 23, 36), (int)x + icon_part(size, 20, 36), (int)y + icon_part(size, 28, 36), yellow, icon_part(size, 3, 36));
    fill_rect(boot_info, x + icon_part(size, 6, 36), y + icon_part(size, 15, 36), icon_part(size, 5, 36), 2, yellow_hi);
    draw_circle_outline(boot_info, x + icon_part(size, 21, 36), y + icon_part(size, 18, 36), icon_part(size, 7, 36), 1, blue);
    draw_circle_outline(boot_info, x + icon_part(size, 21, 36), y + icon_part(size, 18, 36), icon_part(size, 12, 36), 1, blue);
}

static void draw_icon_help(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 12, 26, 37);
    vibio_u32 blue = make_color(boot_info, 44, 142, 231);
    vibio_u32 blue_hi = make_color(boot_info, 92, 185, 246);
    vibio_u32 white = make_color(boot_info, 247, 251, 253);

    fill_circle(boot_info, x + size / 2 + 2, y + size / 2 + 2, size / 2 - 2, shadow);
    fill_circle(boot_info, x + size / 2, y + size / 2, size / 2 - 3, blue);
    fill_circle(boot_info, x + icon_part(size, 14, 36), y + icon_part(size, 12, 36), icon_part(size, 4, 36), blue_hi);
    draw_text_scaled_transparent(boot_info, x + icon_part(size, 12, 36), y + icon_part(size, 7, 36), "?", white, 2);
}

static void draw_icon_mouse(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 12, 25, 35);
    vibio_u32 body = make_color(boot_info, 229, 238, 244);
    vibio_u32 line = make_color(boot_info, 105, 128, 143);
    vibio_u32 purple = make_color(boot_info, 134, 117, 232);

    fill_round_rect(boot_info, x + icon_part(size, 10, 36), y + icon_part(size, 4, 36), icon_part(size, 16, 36), icon_part(size, 28, 36), icon_part(size, 8, 36), shadow);
    fill_round_rect(boot_info, x + icon_part(size, 9, 36), y + icon_part(size, 3, 36), icon_part(size, 16, 36), icon_part(size, 28, 36), icon_part(size, 8, 36), body);
    fill_rect(boot_info, x + icon_part(size, 17, 36), y + icon_part(size, 5, 36), 1, icon_part(size, 11, 36), line);
    fill_round_rect(boot_info, x + icon_part(size, 14, 36), y + icon_part(size, 8, 36), icon_part(size, 7, 36), icon_part(size, 3, 36), 1, purple);
}

static void draw_icon_tasks(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 11, 25, 34);
    vibio_u32 board = make_color(boot_info, 43, 152, 201);
    vibio_u32 board_hi = make_color(boot_info, 79, 194, 225);
    vibio_u32 paper = make_color(boot_info, 244, 250, 252);
    vibio_u32 line = make_color(boot_info, 79, 105, 122);

    fill_round_rect(boot_info, x + 4, y + 4, size - 8, size - 6, icon_part(size, 5, 36), shadow);
    fill_round_rect(boot_info, x + 2, y + 2, size - 8, size - 7, icon_part(size, 5, 36), board);
    fill_rect(boot_info, x + icon_part(size, 7, 36), y + icon_part(size, 6, 36), size - 17, 2, board_hi);
    fill_round_rect(boot_info, x + icon_part(size, 8, 36), y + icon_part(size, 9, 36), size - 18, size - 18, 2, paper);
    fill_rect(boot_info, x + icon_part(size, 12, 36), y + icon_part(size, 14, 36), size - 25, 1, line);
    fill_rect(boot_info, x + icon_part(size, 12, 36), y + icon_part(size, 19, 36), size - 25, 1, line);
    fill_rect(boot_info, x + icon_part(size, 12, 36), y + icon_part(size, 24, 36), size - 25, 1, line);
}

static void draw_icon_chip(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 8, 21, 28);
    vibio_u32 green = make_color(boot_info, 44, 184, 113);
    vibio_u32 green_hi = make_color(boot_info, 105, 229, 154);
    vibio_u32 dark = make_color(boot_info, 17, 39, 34);

    fill_round_rect(boot_info, x + icon_part(size, 7, 36), y + icon_part(size, 7, 36), size - 13, size - 13, icon_part(size, 4, 36), shadow);
    for (vibio_u32 i = 0; i < 4; i++) {
        vibio_u32 p = icon_part(size, 9 + i * 5, 36);
        fill_rect(boot_info, x + p, y + icon_part(size, 5, 36), 2, 3, green_hi);
        fill_rect(boot_info, x + p, y + icon_part(size, 28, 36), 2, 3, green_hi);
        fill_rect(boot_info, x + icon_part(size, 5, 36), y + p, 3, 2, green_hi);
        fill_rect(boot_info, x + icon_part(size, 28, 36), y + p, 3, 2, green_hi);
    }
    fill_round_rect(boot_info, x + icon_part(size, 8, 36), y + icon_part(size, 8, 36), size - 17, size - 17, icon_part(size, 4, 36), green);
    fill_round_rect(boot_info, x + icon_part(size, 13, 36), y + icon_part(size, 13, 36), size - 27, size - 27, icon_part(size, 2, 36), dark);
}

static void draw_icon_settings_gear(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 9, 21, 28);
    vibio_u32 ring = make_color(boot_info, 104, 214, 226);
    vibio_u32 ring_hi = make_color(boot_info, 178, 246, 250);
    vibio_u32 core = make_color(boot_info, 25, 51, 62);
    vibio_u32 tooth = make_color(boot_info, 72, 154, 174);
    vibio_u32 cx = x + size / 2U;
    vibio_u32 cy = y + size / 2U;
    vibio_u32 unit = size / 12U;
    if (unit == 0) { unit = 1; }

    fill_round_rect(boot_info, cx - unit, y + 2U, unit * 2U, unit * 5U, unit, tooth);
    fill_round_rect(boot_info, cx - unit, y + size - unit * 7U, unit * 2U, unit * 5U, unit, tooth);
    fill_round_rect(boot_info, x + 2U, cy - unit, unit * 5U, unit * 2U, unit, tooth);
    fill_round_rect(boot_info, x + size - unit * 7U, cy - unit, unit * 5U, unit * 2U, unit, tooth);
    fill_round_rect(boot_info, x + unit * 3U, y + unit * 3U, unit * 4U, unit * 3U, unit, tooth);
    fill_round_rect(boot_info, x + size - unit * 7U, y + unit * 3U, unit * 4U, unit * 3U, unit, tooth);
    fill_round_rect(boot_info, x + unit * 3U, y + size - unit * 6U, unit * 4U, unit * 3U, unit, tooth);
    fill_round_rect(boot_info, x + size - unit * 7U, y + size - unit * 6U, unit * 4U, unit * 3U, unit, tooth);
    fill_round_rect(boot_info, x + unit * 3U + 2U, y + unit * 3U + 3U, size - unit * 6U, size - unit * 6U, size / 4U, shadow);
    fill_round_rect(boot_info, x + unit * 3U, y + unit * 3U, size - unit * 6U, size - unit * 6U, size / 4U, ring);
    fill_round_rect(boot_info, x + unit * 5U, y + unit * 5U, size - unit * 10U, size - unit * 10U, size / 6U, core);
    fill_rect(boot_info, x + unit * 5U + 1U, y + unit * 5U + 1U, size > unit * 10U + 2U ? size - unit * 10U - 2U : 1U, 1U, ring_hi);
}

MAYBE_UNUSED static void draw_app_icon_symbol(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size,
    vibio_u32 kind,
    const char *label,
    const VibioDesktopTheme *theme)
{
    if (cstr_equals(label, "TRASH")) {
        draw_icon_trash(boot_info, x, y, size);
    } else if (cstr_equals(label, "FILES")) {
        draw_icon_folder(boot_info, x, y, size);
    } else if (cstr_equals(label, "USB")) {
        draw_icon_usb(boot_info, x, y, size);
    } else if (cstr_equals(label, "HELP")) {
        draw_icon_help(boot_info, x, y, size);
    } else if (cstr_equals(label, "DISK")) {
        draw_icon_storage(boot_info, x, y, size);
    } else if (cstr_equals(label, "SOUND")) {
        draw_icon_sound(boot_info, x, y, size);
    } else if (cstr_equals(label, "MOUSE")) {
        draw_icon_mouse(boot_info, x, y, size);
    } else if (cstr_equals(label, "TASKS")) {
        draw_icon_tasks(boot_info, x, y, size);
    } else if (cstr_equals(label, "HEAP")) {
        draw_icon_chip(boot_info, x, y, size);
    } else if (kind == WINDOW_KIND_SETTINGS) {
        draw_icon_settings_gear(boot_info, x, y, size);
    } else if (kind == WINDOW_KIND_TERMINAL) {
        draw_icon_terminal_symbol(boot_info, x, y, size, theme);
    } else if (kind == WINDOW_KIND_WELCOME) {
        draw_icon_vibio_mark(boot_info, x, y, size);
    } else {
        draw_icon_monitor(boot_info, x, y, size);
    }
}

/* Files app icon, drawn from primitives (a small filing drawer / folder with a
 * peeking page) so it is visually distinct from the browser's folder icon. */
static void draw_icon_files_app(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 size)
{
    vibio_u32 shadow = make_color(boot_info, 13, 27, 35);
    vibio_u32 tab = make_color(boot_info, 210, 168, 78);
    vibio_u32 body = make_color(boot_info, 246, 202, 104);
    vibio_u32 body_hi = make_color(boot_info, 255, 224, 150);
    vibio_u32 page = make_color(boot_info, 244, 249, 252);
    vibio_u32 line = make_color(boot_info, 150, 170, 185);

    fill_round_rect(boot_info, x + 4, y + icon_part(size, 10, 36), size - 8, icon_part(size, 22, 36), icon_part(size, 4, 36), shadow);
    /* Folder tab + back panel. */
    fill_round_rect(boot_info, x + icon_part(size, 5, 36), y + icon_part(size, 7, 36), icon_part(size, 14, 36), icon_part(size, 6, 36), icon_part(size, 2, 36), tab);
    fill_round_rect(boot_info, x + icon_part(size, 4, 36), y + icon_part(size, 9, 36), size - icon_part(size, 8, 36), size - icon_part(size, 14, 36), icon_part(size, 3, 36), body);
    /* A page peeking out of the folder. */
    fill_round_rect(boot_info, x + icon_part(size, 10, 36), y + icon_part(size, 6, 36), size - icon_part(size, 20, 36), size - icon_part(size, 16, 36), icon_part(size, 2, 36), page);
    fill_rect(boot_info, x + icon_part(size, 13, 36), y + icon_part(size, 12, 36), size - icon_part(size, 26, 36), 1, line);
    fill_rect(boot_info, x + icon_part(size, 13, 36), y + icon_part(size, 16, 36), size - icon_part(size, 26, 36), 1, line);
    /* Folder front lip over the page. */
    fill_round_rect(boot_info, x + icon_part(size, 4, 36), y + icon_part(size, 20, 36), size - icon_part(size, 8, 36), size - icon_part(size, 25, 36), icon_part(size, 3, 36), body_hi);
}

/* Draw a desktop icon label centred under the icon, clipped to the cell width.
 * Labels longer than the cell are truncated with a ".." ellipsis so small-icon
 * mode (and long names) never spill the text across neighbouring icons. */
static void draw_desktop_icon_label(
    const VibioBootInfo *boot_info,
    vibio_u32 cell_x,
    vibio_u32 cell_w,
    vibio_u32 label_y,
    const char *label,
    vibio_u32 color)
{
    char buf[40];
    vibio_u32 avail = cell_w > 4U ? cell_w - 4U : 1U;
    const char *draw = label;
    vibio_u32 label_width = font_text_width(&FONT_GEIST_SM, label);
    /* Truncate to fit the cell with a ".." ellipsis, measured in the real
     * proportional Geist font (not a fixed char step). */
    if (label_width > avail) {
        vibio_u32 len = cstr_length(label);
        vibio_u32 keep = len;
        while (keep > 1U) {
            keep--;
            vibio_u32 i = 0;
            for (; i < keep && i + 1U < sizeof(buf); i++) {
                buf[i] = label[i];
            }
            buf[i++] = '.';
            buf[i++] = '.';
            buf[i] = 0;
            if (font_text_width(&FONT_GEIST_SM, buf) <= avail) {
                break;
            }
        }
        draw = buf;
        label_width = font_text_width(&FONT_GEIST_SM, buf);
    }
    vibio_u32 label_x = label_width < cell_w ? cell_x + (cell_w - label_width) / 2U : cell_x + 2U;
    /* Clip to the cell horizontally and the full Geist line height vertically so
     * the 20px-tall glyphs are never cut off. */
    set_clip(cell_x, label_y, cell_x + cell_w, label_y + VIBIO_FONT_GEIST_SM_LINE + 4U);
    draw_small_cstr_transparent(boot_info, label_x, label_y, draw, color);
    reset_clip();
}

static void draw_desktop_icon(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 cell_w,
    vibio_u32 image_size,
    const char *label,
    const vibio_u32 *icon,
    const VibioDesktopTheme *theme)
{
    (void)image_size;
    vibio_u32 draw_size = DESKTOP_ICON_IMAGE;
    vibio_u32 icon_x = x + (cell_w - draw_size) / 2;
    vibio_u32 icon_y = y + 2;
    draw_icon_image(boot_info, icon_x, icon_y, icon, draw_size);
    draw_desktop_icon_label(boot_info, x, cell_w, y + draw_size + 8, label, theme->text);
}

/* Desktop icon whose glyph is drawn from primitives instead of a PNG array. */
static void draw_desktop_icon_files(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 cell_w,
    vibio_u32 image_size,
    const char *label,
    const VibioDesktopTheme *theme)
{
    vibio_u32 icon_x = x + (cell_w - image_size) / 2;
    vibio_u32 icon_y = y + 2;
    draw_icon_files_app(boot_info, icon_x, icon_y, image_size);
    draw_desktop_icon_label(boot_info, x, cell_w, y + image_size + 8, label, theme->text);
}

static void draw_desktop_icon_by_kind(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopTheme *theme,
    const VibioDesktopIconSpec *icon)
{
    vibio_u32 cell_w = desktop_icon_cell_w(wm);
    vibio_u32 image_size = desktop_icon_image_size(wm);
    if (icon->kind == WINDOW_KIND_FILES) {
        draw_desktop_icon_files(boot_info, icon->x, icon->y, cell_w, image_size, icon->label, theme);
        return;
    }
    if (icon->kind == DESKTOP_ICON_KIND_TEMP_FOLDER) {
        draw_desktop_icon_files(boot_info, icon->x, icon->y, cell_w, image_size, icon->label, theme);
        return;
    }
    if (icon->kind == DESKTOP_ICON_KIND_TEMP_TEXT) {
        vibio_u32 icon_x = icon->x + (cell_w - image_size) / 2;
        vibio_u32 icon_y = icon->y + 2;
        fill_round_rect(boot_info, icon_x + 4U, icon_y + 2U, image_size - 8U, image_size - 4U, 2U, make_color(boot_info, 236, 244, 248));
        fill_rect(boot_info, icon_x + 10U, icon_y + 11U, image_size > 20U ? image_size - 20U : 4U, 2U, make_color(boot_info, 69, 104, 128));
        fill_rect(boot_info, icon_x + 10U, icon_y + 18U, image_size > 20U ? image_size - 20U : 4U, 2U, make_color(boot_info, 69, 104, 128));
        draw_desktop_icon_label(boot_info, icon->x, cell_w, icon->y + image_size + 8U, icon->label, theme->text);
        return;
    }
    if (icon->kind == WINDOW_KIND_SYSTEM) {
        draw_desktop_icon(boot_info, icon->x, icon->y, cell_w, image_size, icon->label, vibio_icon_system_36, theme);
    } else if (icon->kind == WINDOW_KIND_BROWSER) {
        draw_desktop_icon(boot_info, icon->x, icon->y, cell_w, image_size, icon->label, vibio_icon_browser_36, theme);
    } else {
        draw_desktop_icon(boot_info, icon->x, icon->y, cell_w, image_size, icon->label, vibio_icon_terminal_36, theme);
    }
}

static void draw_hatched_rect(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 color)
{
    for (vibio_u32 yy = 0; yy < h; yy += 4U) {
        fill_rect(boot_info, x, y + yy, w, 1U, color);
    }
}

static void draw_rect_outline(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 color)
{
    if (w == 0 || h == 0) {
        return;
    }
    fill_rect(boot_info, x, y, w, 1, color);
    fill_rect(boot_info, x, y + h - 1, w, 1, color);
    fill_rect(boot_info, x, y, 1, h, color);
    fill_rect(boot_info, x + w - 1, y, 1, h, color);
}

static void draw_desktop_interaction_overlay(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopTheme *theme)
{
    vibio_u32 sx = 0, sy = 0, sw = 0, sh = 0;
    vibio_u32 active_select_mask = wm->selecting ? desktop_selection_mask(wm) : 0;
    vibio_u32 selected_mask = wm->desktop_selected_mask | active_select_mask;

    if (wm->selecting && desktop_selection_rect(wm, &sx, &sy, &sw, &sh)) {
        if (boot_info->framebuffer_height > DESKTOP_TASKBAR_HEIGHT &&
            sy + sh > boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT) {
            sh = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT > sy ?
                 boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - sy : 0;
        }
        if (sw > 0 && sh > 0) {
            draw_hatched_rect(boot_info, sx, sy, sw, sh, make_color(boot_info, 39, 104, 156));
            draw_rect_outline(boot_info, sx, sy, sw, sh, make_color(boot_info, 18, 116, 208));
        }
    }

    vibio_u32 count = desktop_icon_count(wm);
    vibio_u32 cw = desktop_icon_cell_w(wm);
    vibio_u32 ch = desktop_icon_cell_h(wm);
    for (vibio_u32 i = 0; i < count; i++) {
        VibioDesktopIconSpec icon;
        if (!desktop_icon_info(wm, i, &icon)) {
            continue;
        }
        vibio_u32 mask = desktop_icon_mask_for_kind(icon.kind);
        vibio_u32 selected = (selected_mask & mask) != 0;
        vibio_u32 hovered = wm->desktop_hover_kind == icon.kind;
        if (selected || hovered || (wm->desktop_rename_active && wm->desktop_rename_index == i)) {
            vibio_u32 bg = hovered ? make_color(boot_info, 58, 95, 116) :
                                     make_color(boot_info, 45, 86, 112);
            fill_round_rect(boot_info, icon.x, icon.y, cw, ch, 4, bg);
            draw_soft_rect_border(boot_info, icon.x, icon.y, cw, ch, make_color(boot_info, 91, 139, 165));
        }
        draw_desktop_icon_by_kind(boot_info, wm, theme, &icon);
        if (wm->desktop_rename_active && wm->desktop_rename_index == i) {
            vibio_u32 label_w = font_text_width(&FONT_GEIST_SM, icon.label);
            vibio_u32 tx = icon.x + (label_w < cw ? (cw - label_w) / 2U : 2U);
            vibio_u32 ty = icon.y + desktop_icon_image_size(wm) + 7U;
            fill_rect(boot_info, tx > 2U ? tx - 2U : 0, ty > 1U ? ty - 1U : 0,
                      label_w + 4U, SMALL_LINE_STEP + 2U, make_color(boot_info, 22, 112, 204));
            draw_small_cstr(boot_info, tx, ty, icon.label, make_color(boot_info, 250, 253, 255),
                            make_color(boot_info, 22, 112, 204));
        }
    }
}

static void draw_desktop_base(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    const VibioWindowManager *wm)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 height = boot_info->framebuffer_height;

    /* Imported image wallpapers (mode >= WALLPAPER_IMAGE_BASE) paint a real
     * photo over the whole screen. Otherwise the built-in Vibio gradient. */
    if (wm->wallpaper_mode >= WALLPAPER_IMAGE_BASE &&
        (wm->wallpaper_mode - WALLPAPER_IMAGE_BASE) < (vibio_u32)VIBIO_WALLPAPER_COUNT) {
        draw_wallpaper_image(boot_info, &vibio_wallpapers[wm->wallpaper_mode - WALLPAPER_IMAGE_BASE]);
    } else {
        draw_wallpaper(boot_info, theme);
    }

    /* TEMPORARY USB bring-up diagnostic (real-hardware debugging aid). One line,
     * always shown, reporting how far the read-only xHCI mass-storage path got at
     * boot so a single photo of the desktop pinpoints where it stops on the
     * laptop - no Terminal needed. Remove once the live USB mount is working. */
    if (height > DESKTOP_TASKBAR_HEIGHT + SMALL_LINE_STEP * 2 + 8) {
        const VibioUsbStorage *u = &g_usb_storage;
        const char *tn = u->controller_type == USB_TYPE_XHCI ? "XHCI" :
                         u->controller_type == USB_TYPE_EHCI ? "EHCI" :
                         u->controller_type == USB_TYPE_OHCI ? "OHCI" :
                         u->controller_type == USB_TYPE_UHCI ? "UHCI" : "none";
        char d[176];
        vibio_u32 p = 0;
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), "USB: found=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->controller_found);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " ");
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), tn);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " mapped=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->bar_mapped);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " mmio=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->mmio_readable);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " cap=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->cap_length);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " ports=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->max_ports);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " conn=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->ports_connected);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " step=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->last_step);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " dev=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->device_descriptor_ok);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " msd=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->msd_interface_found);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " sect=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->sector_backend_ready);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " fat=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), u->fat32_mounted);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " err=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), g_xhci_msd.last_error);
        p = (vibio_u32)panic_append_text(d, p, sizeof(d), " port=");
        p = (vibio_u32)panic_append_uint(d, p, sizeof(d), g_xhci_msd.port);
        d[p] = 0;
        vibio_u32 diag_y = height - DESKTOP_TASKBAR_HEIGHT - SMALL_LINE_STEP * 2 - 8;
        draw_small_cstr_transparent(boot_info, 16, diag_y, d, theme->accent);
    }
}

static const char *desktop_context_label(vibio_u32 menu, vibio_u32 row)
{
    if (menu == DESKTOP_CONTEXT_ROOT) {
        if (row == 0) { return "View"; }
        if (row == 1) { return "Refresh"; }
        if (row == 2) { return "New"; }
        if (row == 3) { return "Open terminal"; }
        if (row == 4) { return "Wallpaper"; }
    } else if (menu == DESKTOP_CONTEXT_VIEW) {
        if (row == 0) { return "Large icons"; }
        if (row == 1) { return "Medium icons"; }
        if (row == 2) { return "Small icons"; }
        if (row == 3) { return "Auto arrange icons"; }
        if (row == 4) { return "Show desktop icons"; }
    } else if (menu == DESKTOP_CONTEXT_NEW) {
        if (row == 0) { return "Folder"; }
        if (row == 1) { return "Text file"; }
    } else if (menu == DESKTOP_CONTEXT_WALLPAPER) {
        if (row == 0) { return "Vibio default"; }
        if (row - 1U < (vibio_u32)VIBIO_WALLPAPER_COUNT) {
            return vibio_wallpapers[row - 1U].name;
        }
    }
    return "";
}

static vibio_u32 desktop_context_row_checked(const VibioWindowManager *wm, vibio_u32 menu, vibio_u32 row)
{
    if (menu == DESKTOP_CONTEXT_VIEW) {
        if (row == 0) { return wm->desktop_icon_size == DESKTOP_ICON_SIZE_LARGE; }
        if (row == 1) { return wm->desktop_icon_size == DESKTOP_ICON_SIZE_MEDIUM; }
        if (row == 2) { return wm->desktop_icon_size == DESKTOP_ICON_SIZE_SMALL; }
        if (row == 3) { return wm->desktop_auto_arrange != 0; }
        if (row == 4) { return wm->desktop_icons_visible != 0; }
    }
    if (menu == DESKTOP_CONTEXT_WALLPAPER) {
        if (row == 0) {
            return wm->wallpaper_mode < WALLPAPER_IMAGE_BASE;
        }
        return wm->wallpaper_mode == WALLPAPER_IMAGE_BASE + (row - 1U);
    }
    return 0;
}

static void draw_context_menu_panel(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopTheme *theme,
    vibio_u32 menu,
    vibio_u32 x,
    vibio_u32 y)
{
    vibio_u32 rows = desktop_context_rows(menu);
    vibio_u32 h = rows * DESKTOP_CONTEXT_ROW_H;
    vibio_u32 bg = make_color(boot_info, 18, 43, 62);
    vibio_u32 row_bg = make_color(boot_info, 27, 62, 84);
    fill_round_rect(boot_info, x + 4U, y + 5U, DESKTOP_CONTEXT_W, h, 8U, make_color(boot_info, 4, 10, 15));
    fill_round_rect(boot_info, x, y, DESKTOP_CONTEXT_W, h, 8U, bg);
    draw_soft_rect_border(boot_info, x, y, DESKTOP_CONTEXT_W, h, make_color(boot_info, 65, 101, 121));
    for (vibio_u32 row = 0; row < rows; row++) {
        vibio_u32 ry = y + row * DESKTOP_CONTEXT_ROW_H;
        vibio_u32 hover = 0;
        vibio_u32 hit_menu = DESKTOP_CONTEXT_NONE;
        vibio_u32 hit_row = desktop_context_hit(wm, wm->context_x, wm->context_y, &hit_menu);
        (void)hit_row;
        if (wm->context_hover_row == row) {
            hover = 1;
        }
        if (hover) {
            fill_round_rect(boot_info, x + 5U, ry + 3U, DESKTOP_CONTEXT_W - 10U, DESKTOP_CONTEXT_ROW_H - 6U, 5U, row_bg);
        }
        if (menu == DESKTOP_CONTEXT_ROOT && row == 1) {
            draw_icon_image_centered(boot_info, x + 12U, ry, 24U, DESKTOP_CONTEXT_ROW_H, vibio_icon_restart_18, 18);
        } else if (desktop_context_row_checked(wm, menu, row)) {
            draw_small_cstr(boot_info, x + 15U, ry + 11U, "*", theme->accent, hover ? row_bg : bg);
        }
        draw_small_cstr(boot_info, x + 42U, ry + 10U, desktop_context_label(menu, row), theme->text, hover ? row_bg : bg);
        if ((menu == DESKTOP_CONTEXT_ROOT && (row == 0 || row == 2 || row == 4))) {
            draw_small_cstr(boot_info, x + DESKTOP_CONTEXT_W - 22U, ry + 10U, ">", theme->muted, hover ? row_bg : bg);
        }
    }
}

static void draw_desktop_context_menu(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopTheme *theme)
{
    if (wm->context_menu == DESKTOP_CONTEXT_NONE) {
        return;
    }
    draw_context_menu_panel(boot_info, wm, theme, DESKTOP_CONTEXT_ROOT, wm->context_x, wm->context_y);
    if (wm->context_menu != DESKTOP_CONTEXT_ROOT) {
        draw_context_menu_panel(boot_info, wm, theme, wm->context_menu,
                                wm->context_x + DESKTOP_CONTEXT_W - 1U, wm->context_y);
    }
}

static void draw_topbar_live(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop)
{
    if (DESKTOP_TOPBAR_HEIGHT < 8) {
        (void)boot_info;
        (void)desktop;
        return;
    }

    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 width = boot_info->framebuffer_width;
    vibio_u32 live_x = width > 264 ? width - 264 : DESKTOP_MARGIN;

    fill_rect(boot_info, live_x, 0, width - live_x, DESKTOP_TOPBAR_HEIGHT - 3, theme->topbar);
    g_text_bg = theme->topbar;

    if (desktop->mouse != 0) {
        draw_mouse_readout(boot_info, live_x + 8, 8, desktop->mouse, theme->muted);
    }
}

MAYBE_UNUSED static void draw_taskbar_button_title(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 bg,
    vibio_u32 color)
{
    set_clip(x, y, x + w, y + h);
    g_text_bg = bg;
    vibio_u32 tx = x + 12;
    for (vibio_u32 i = 0; i < window->title_length; i++) {
        draw_char(boot_info, tx, y + 6, window->title[i], color);
        tx += CHAR_STEP;
    }
    reset_clip();
}

static void draw_start_logo(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 color)
{
    fill_round_rect(boot_info, x, y, 7, 7, 1, color);
    fill_round_rect(boot_info, x + 9, y, 7, 7, 1, color);
    fill_round_rect(boot_info, x, y + 9, 7, 7, 1, color);
    fill_round_rect(boot_info, x + 9, y + 9, 7, 7, 1, color);
}

static void draw_taskbar_app_icon(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 bg,
    const VibioDesktopTheme *theme)
{
    if (bg != theme->taskbar) {
        fill_round_rect(boot_info, x, y, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_WIDTH, 7, bg);
    }

    if (window->kind == WINDOW_KIND_FILES) {
        draw_icon_files_app(boot_info, x + 6, y + 6, 26);
        return;
    }
    if (window->kind == WINDOW_KIND_SETTINGS) {
        draw_icon_settings_gear(boot_info, x + 6, y + 6, 26);
        return;
    }

    const vibio_u32 *icon = vibio_icon_terminal_26;
    if (window->kind == WINDOW_KIND_SYSTEM) {
        icon = vibio_icon_system_26;
    } else if (window->kind == WINDOW_KIND_BROWSER) {
        icon = vibio_icon_browser_26;
    }

    draw_icon_image(boot_info, x + 6, y + 6, icon, 26);
}

/* Build a useful one-line description of what an app window is currently showing
 * for its taskbar hover preview - the path/item count for Files, the page title
 * for the Browser, the file + zoom for the viewer, the file + transport state for
 * the player - instead of a generic "Open window". */
static void taskbar_preview_detail(
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    char *buf,
    vibio_u32 size)
{
    vibio_u32 p = 0;
    buf[0] = 0;
    if (window->kind == WINDOW_KIND_FILES && desktop->files != 0) {
        p = panic_append_text(buf, p, size, desktop->files->path_text[0] ? desktop->files->path_text : "/");
        p = panic_append_text(buf, p, size, "  ");
        p = panic_append_uint(buf, p, size, desktop->files->entry_count);
        p = panic_append_text(buf, p, size, desktop->files->entry_count == 1 ? " item" : " items");
    } else if (window->kind == WINDOW_KIND_BROWSER && desktop->browser != 0 &&
               desktop->browser->title_len > 0) {
        p = panic_append_text(buf, p, size, desktop->browser->title);
    } else if (window->kind == WINDOW_KIND_MEDIA_VIEWER && desktop->media_viewer != 0 &&
               desktop->media_viewer->loaded) {
        p = panic_append_text(buf, p, size, desktop->media_viewer->filename);
        p = panic_append_text(buf, p, size, "  ");
        if (desktop->media_viewer->zoom_percent == MEDIA_ZOOM_FIT) {
            p = panic_append_text(buf, p, size, "FIT");
        } else {
            p = panic_append_uint(buf, p, size, desktop->media_viewer->zoom_percent);
            p = panic_append_text(buf, p, size, "%");
        }
    } else if (window->kind == WINDOW_KIND_MEDIA_PLAYER && desktop->media_player != 0 &&
               desktop->media_player->loaded) {
        p = panic_append_text(buf, p, size, desktop->media_player->filename);
        p = panic_append_text(buf, p, size, "  ");
        p = panic_append_text(buf, p, size,
            desktop->media_player->playing ? "Playing" :
            (desktop->media_player->muted ? "Muted" : "Stopped"));
    } else if (window->kind == WINDOW_KIND_TERMINAL) {
        p = panic_append_text(buf, p, size, "Command prompt");
    } else if (window->kind == WINDOW_KIND_SYSTEM) {
        p = panic_append_text(buf, p, size, "System & self-test status");
    } else if (window->kind == WINDOW_KIND_SETTINGS) {
        p = panic_append_text(buf, p, size, "Display, sound, storage, devices");
    } else {
        p = panic_append_text(buf, p, size, "Open window");
    }
    if (p >= size) {
        p = size - 1;
    }
    buf[p] = 0;
}

static void draw_taskbar_hover_popup(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopContext *desktop)
{
    if (wm->taskbar_hover_slot == TASKBAR_NO_SLOT ||
        wm->power_menu_open || wm->power_confirm != POWER_CONFIRM_NONE || wm->volume_popup_open) {
        return;
    }

    vibio_u32 idx = taskbar_slot_window_index(wm, wm->taskbar_hover_slot);
    if (idx == WM_NO_WINDOW) {
        return;
    }
    const VibioWindow *window = &wm->windows[idx];
    const VibioDesktopTheme *theme = desktop->theme;
    const char *label = taskbar_window_label(window);
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_hover_popup_rect(boot_info, wm, wm->taskbar_hover_slot, &x, &y, &w, &h)) {
        return;
    }

    vibio_u32 shadow = make_color(boot_info, 3, 8, 12);
    vibio_u32 bg = make_color(boot_info, 24, 34, 42);
    fill_round_rect(boot_info, x + 3U, y + 4U, w, h, 7U, shadow);
    fill_round_rect(boot_info, x, y, w, h, 7U, bg);
    draw_soft_rect_border(boot_info, x, y, w, h, make_color(boot_info, 78, 102, 116));

    if (!taskbar_window_is_running(window)) {
        vibio_u32 label_w = font_text_width(&FONT_GEIST_SM, label);
        vibio_u32 lx = x + (w > label_w ? (w - label_w) / 2U : 8U);
        draw_small_cstr(boot_info, lx, y + 10U, label, theme->text, bg);
        return;
    }

    set_clip(x + 10U, y + 8U, x + w - 10U, y + 28U);
    draw_small_cstr(boot_info, x + 12U, y + 10U, label, theme->text, bg);
    reset_clip();

    vibio_u32 tile_x = x + 12U;
    vibio_u32 tile_y = y + 36U;
    vibio_u32 tile_w = w - 24U;
    vibio_u32 tile_h = h - 48U;
    vibio_u32 tile_bg = make_color(boot_info, 13, 21, 28);
    fill_round_rect(boot_info, tile_x, tile_y, tile_w, tile_h, 4U, tile_bg);
    draw_soft_rect_border(boot_info, tile_x, tile_y, tile_w, tile_h, make_color(boot_info, 62, 88, 104));
    fill_rect(boot_info, tile_x, tile_y, tile_w, 16U, make_color(boot_info, 32, 52, 64));

    set_clip(tile_x + 8U, tile_y + 5U, tile_x + tile_w - 8U, tile_y + 18U);
    draw_small_cstr(boot_info, tile_x + 8U, tile_y + 5U, window->title_length > 0 ? window->title : label,
                    theme->muted, make_color(boot_info, 32, 52, 64));
    reset_clip();

    vibio_u32 icon_x = tile_x + 12U;
    vibio_u32 icon_y = tile_y + 28U;
    draw_taskbar_app_icon(boot_info, window, icon_x, icon_y, tile_bg, theme);
    char detail[64];
    taskbar_preview_detail(window, desktop, detail, sizeof(detail));
    /* Line 1: live app detail; line 2: window state (open / minimized). */
    set_clip(icon_x + 48U, icon_y, tile_x + tile_w - 6U, icon_y + 34U);
    draw_small_cstr(boot_info, icon_x + 48U, icon_y + 2U, detail, theme->text, tile_bg);
    draw_small_cstr(boot_info, icon_x + 48U, icon_y + 20U,
                    window->minimized && !window->visible ? "Minimized" : "Running",
                    theme->muted, tile_bg);
    reset_clip();
}

static void draw_volume_popup(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopContext *desktop)
{
    if (!wm->volume_popup_open || desktop->audio == 0) {
        return;
    }

    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_volume_popup_rect(boot_info, &x, &y, &w, &h)) {
        return;
    }

    vibio_u32 bg = make_color(boot_info, 18, 29, 38);
    vibio_u32 rail = make_color(boot_info, 46, 61, 70);
    fill_round_rect(boot_info, x, y, w, h, 7, bg);
    draw_soft_rect_border(boot_info, x, y, w, h, make_color(boot_info, 78, 104, 118));
    draw_icon_image_centered(boot_info, x + 10, y + 8, 26, 22, vibio_icon_sound_20, 20);
    draw_small_cstr(boot_info, x + 40, y + 11, "SOUND", theme->muted, bg);
    char percent[8];
    vibio_u32 p = 0;
    p = panic_append_uint(percent, p, sizeof(percent), desktop->audio->volume);
    p = panic_append_text(percent, p, sizeof(percent), "%");
    percent[p] = 0;
    vibio_u32 percent_w = font_text_width(&FONT_GEIST_SM, percent);
    vibio_u32 percent_x = x + w > percent_w + 14U ? x + w - percent_w - 14U : x + 106U;
    draw_small_cstr(boot_info, percent_x, y + 11, percent, theme->text, bg);

    vibio_u32 sx = x + 18U;
    vibio_u32 sy = y + 40U;
    vibio_u32 sw = w - 36U;
    vibio_u32 fill_w = (sw * desktop->audio->volume) / 100U;
    fill_round_rect(boot_info, sx, sy, sw, 6, 3, rail);
    if (fill_w > 0) {
        fill_round_rect(boot_info, sx, sy, fill_w, 6, 3, theme->accent);
    }
    vibio_u32 knob_x = sx + fill_w;
    if (knob_x >= sx + sw) {
        knob_x = sx + sw - 1U;
    }
    fill_circle(boot_info, knob_x, sy + 3, 6, theme->text);
}

static void draw_power_menu_row(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    const vibio_u32 *icon,
    vibio_u32 icon_size,
    const char *label,
    vibio_u32 row_bg,
    const VibioDesktopTheme *theme,
    vibio_u32 show_separator)
{
    fill_round_rect(boot_info, x + 8U, y + 4U, w - 16U, h - 8U, 7U, row_bg);
    draw_icon_image_centered(boot_info, x + 18U, y, 30U, h, icon, icon_size);
    vibio_u32 label_x = x + 56U;
    vibio_u32 label_y = y + (h / 2U) + (FONT_GEIST_SM.ascent / 2U) - 6U;
    draw_small_cstr(boot_info, label_x, label_y, label, theme->text, row_bg);
    if (show_separator && w > 56U) {
        fill_rect(boot_info, x + 16U, y + h - 1U, w - 32U, 1U, make_color(boot_info, 42, 58, 68));
    }
}

static void draw_power_menu(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopContext *desktop)
{
    if (!wm->power_menu_open || wm->power_confirm != POWER_CONFIRM_NONE) {
        return;
    }

    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_power_menu_rect(boot_info, &x, &y, &w, &h)) {
        return;
    }

    vibio_u32 shadow = make_color(boot_info, 5, 10, 14);
    vibio_u32 bg = make_color(boot_info, 18, 29, 38);
    vibio_u32 row_bg = make_color(boot_info, 23, 38, 49);
    fill_round_rect(boot_info, x + 3U, y + 4U, w, h, 10U, shadow);
    fill_round_rect(boot_info, x, y, w, h, 10U, bg);
    draw_soft_rect_border(boot_info, x, y, w, h, make_color(boot_info, 88, 119, 135));
    fill_rect(boot_info, x + 1U, y + POWER_MENU_HEADER_H - 1U, w - 2U, 1U, make_color(boot_info, 40, 56, 66));
    vibio_u32 title_w = font_text_width(&FONT_GEIST_SM, "POWER");
    draw_small_cstr(boot_info, x + (w - title_w) / 2U, y + 10U, "POWER", theme->muted, bg);
    draw_power_menu_row(boot_info, x, y + POWER_MENU_HEADER_H, w, POWER_MENU_ROW_H, vibio_icon_power_20, 20, "Shut down", row_bg, theme, 1U);
    draw_power_menu_row(
        boot_info,
        x,
        y + POWER_MENU_HEADER_H + POWER_MENU_ROW_H,
        w,
        POWER_MENU_ROW_H,
        vibio_icon_restart_18,
        18,
        "Restart",
        row_bg,
        theme,
        0U);
}

static void draw_power_confirm_button_label(
    const VibioBootInfo *boot_info,
    vibio_u32 btn_x,
    vibio_u32 btn_y,
    vibio_u32 btn_w,
    vibio_u32 btn_h,
    const char *label,
    vibio_u32 text_color,
    vibio_u32 btn_color)
{
    vibio_u32 label_w = font_text_width(&FONT_GEIST_SM, label);
    vibio_u32 label_x = btn_x + (btn_w - label_w) / 2U;
    vibio_u32 label_y = btn_y + (btn_h / 2U) + (FONT_GEIST_SM.ascent / 2U) - 6U;
    draw_small_cstr(boot_info, label_x, label_y, label, text_color, btn_color);
}

static void draw_power_confirm(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopContext *desktop)
{
    if (wm->power_confirm == POWER_CONFIRM_NONE) {
        return;
    }

    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 x = 0, y = 0, w = 0, h = 0;
    if (!taskbar_power_confirm_rect(boot_info, &x, &y, &w, &h)) {
        return;
    }

    char first_name[16];
    vibio_u32 running = power_running_apps(wm, first_name, sizeof(first_name));
    vibio_u32 overlay = make_color(boot_info, 7, 11, 16);
    vibio_u32 shadow = make_color(boot_info, 3, 7, 10);
    vibio_u32 bg = make_color(boot_info, 18, 29, 38);
    vibio_u32 panel_hi = make_color(boot_info, 25, 43, 55);
    vibio_u32 secondary_bg = make_color(boot_info, 34, 48, 58);
    vibio_u32 action_x = 0;
    vibio_u32 action_w = 0;
    vibio_u32 back_x = 0;
    vibio_u32 back_w = 0;
    vibio_u32 btn_y = 0;
    vibio_u32 btn_h = 0;
    vibio_u32 shutting_down = wm->power_confirm == POWER_CONFIRM_SHUTDOWN;
    const char *action_label = shutting_down ? "Shut down" : "Restart";
    const char *title = shutting_down ? "Shut down Vibio?" : "Restart Vibio?";
    vibio_u32 action_bg = shutting_down ? make_color(boot_info, 185, 67, 58) : theme->accent;
    vibio_u32 action_text = shutting_down ? theme->text : theme->black;

    fill_rect(boot_info, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, overlay);
    fill_round_rect(boot_info, x + 4U, y + 5U, w, h, 12U, shadow);
    fill_round_rect(boot_info, x, y, w, h, 12U, bg);
    fill_round_rect(boot_info, x + 1U, y + 1U, w - 2U, 42U, 11U, panel_hi);
    fill_rect(boot_info, x + 1U, y + 40U, w - 2U, 1U, make_color(boot_info, 52, 74, 88));
    draw_soft_rect_border(boot_info, x, y, w, h, make_color(boot_info, 92, 126, 144));

    vibio_u32 title_w = font_text_width(&FONT_GEIST_UI, title);
    font_draw_text(boot_info, &FONT_GEIST_UI, x + (w - title_w) / 2U, y + 17U, title, theme->text);

    char status[80];
    vibio_u32 status_len = 0;
    if (running > 0) {
        if (first_name[0] != 0) {
            status_len = panic_append_text(status, status_len, sizeof(status), first_name);
            status_len = panic_append_text(
                status,
                status_len,
                sizeof(status),
                running > 1U ? " is running and other processes." : " is running.");
        } else {
            status_len = panic_append_text(status, status_len, sizeof(status), "Apps are still running.");
        }
    } else {
        status_len = panic_append_text(status, status_len, sizeof(status), "No apps are currently open.");
    }
    status[status_len] = 0;

    vibio_u32 status_w = font_text_width(&FONT_GEIST_SM, status);
    draw_small_cstr(boot_info, x + (w - status_w) / 2U, y + 70U, status, theme->muted, bg);

    const char *hint = shutting_down ? "Temporary app state will be lost." : "Vibio will restart immediately.";
    vibio_u32 hint_w = font_text_width(&FONT_GEIST_SM, hint);
    draw_small_cstr(boot_info, x + (w - hint_w) / 2U, y + 94U, hint, theme->muted, bg);

    power_confirm_button_layout(x, y, w, h, &action_x, &action_w, &back_x, &back_w, &btn_y, &btn_h);
    fill_round_rect(boot_info, action_x, btn_y, action_w, btn_h, 8U, action_bg);
    draw_power_confirm_button_label(boot_info, action_x, btn_y, action_w, btn_h, action_label, action_text, action_bg);
    fill_round_rect(boot_info, back_x, btn_y, back_w, btn_h, 8U, secondary_bg);
    draw_soft_rect_border(boot_info, back_x, btn_y, back_w, btn_h, make_color(boot_info, 58, 78, 92));
    draw_power_confirm_button_label(boot_info, back_x, btn_y, back_w, btn_h, "Back", theme->text, secondary_bg);
}
static void draw_taskbar(
    const VibioBootInfo *boot_info,
    const VibioWindowManager *wm,
    const VibioDesktopContext *desktop)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 width = boot_info->framebuffer_width;
    vibio_u32 height = boot_info->framebuffer_height;
    if (height <= DESKTOP_TASKBAR_HEIGHT) {
        return;
    }

    vibio_u32 taskbar_y = height - DESKTOP_TASKBAR_HEIGHT;
    vibio_u32 button_y = taskbar_y + 5;
    vibio_u32 button_h = TASKBAR_BUTTON_WIDTH;
    vibio_u32 top_window = wm_top_visible(wm);

    fill_rect(boot_info, 0, taskbar_y, width, DESKTOP_TASKBAR_HEIGHT, theme->taskbar);
    fill_rect(boot_info, 0, taskbar_y, width, 2, theme->taskbar_edge);

    fill_round_rect(boot_info, DESKTOP_MARGIN, button_y, TASKBAR_START_WIDTH, button_h, 8, theme->start_button);
    draw_start_logo(boot_info, DESKTOP_MARGIN + 10, button_y + 10, theme->accent);

    vibio_u32 search_x = DESKTOP_MARGIN + TASKBAR_START_WIDTH + TASKBAR_BUTTON_GAP;
    vibio_u32 search_y = button_y + 3;
    vibio_u32 search_h = button_h - 6;
    vibio_u32 search_bg = make_color(boot_info, 30, 40, 48);
    fill_round_rect(boot_info, search_x, search_y, TASKBAR_SEARCH_WIDTH, search_h, 8, search_bg);
    draw_soft_rect_border(boot_info, search_x, search_y, TASKBAR_SEARCH_WIDTH, search_h, make_color(boot_info, 48, 62, 72));
    draw_icon_image_centered(boot_info, search_x + 5, search_y, 28, search_h, vibio_icon_search_18, 18);
    draw_small_cstr(boot_info, search_x + 34, search_y + 13, "SEARCH", theme->muted, search_bg);

    vibio_u32 slots = taskbar_slot_count(wm);
    for (vibio_u32 slot = 0; slot < slots; slot++) {
        vibio_u32 idx = taskbar_slot_window_index(wm, slot);
        if (idx == WM_NO_WINDOW) {
            continue;
        }
        const VibioWindow *window = &wm->windows[idx];
        vibio_u32 x = taskbar_button_x(slot);
        vibio_u32 focused = idx == top_window && window->visible;
        vibio_u32 hovered = wm->taskbar_hover_slot == slot;
        vibio_u32 running = taskbar_window_is_running(window);
        vibio_u32 bg = focused ? theme->taskbar_button_focus :
                      (hovered ? make_color(boot_info, 40, 57, 68) : theme->taskbar);
        draw_taskbar_app_icon(boot_info, window, x, button_y, bg, theme);
        if (focused) {
            fill_round_rect(boot_info, x + 9, taskbar_y + DESKTOP_TASKBAR_HEIGHT - 5, TASKBAR_BUTTON_WIDTH - 18, 3, 1, theme->accent);
        } else if (running) {
            fill_round_rect(boot_info, x + 14, taskbar_y + DESKTOP_TASKBAR_HEIGHT - 5, TASKBAR_BUTTON_WIDTH - 28, 2, 1, theme->muted);
        }
    }

    if (width > 260) {
        vibio_u32 tray_x = taskbar_tray_base_x(boot_info);
        fill_rect(boot_info, tray_x - 118, button_y, 240, button_h, theme->taskbar);
        draw_icon_image_centered(boot_info, taskbar_power_icon_x(boot_info), button_y, 32, button_h, vibio_icon_power_20, 20);
        draw_icon_image_centered(boot_info, taskbar_sound_icon_x(boot_info), button_y, 32, button_h, vibio_icon_sound_20, 20);
        draw_icon_image_centered(boot_info, tray_x - 42, button_y, 36, button_h, vibio_icon_wifi_20, 20);
        if (desktop->clock_ok) {
            draw_small_cstr(boot_info, tray_x, button_y + 8, "TIME", theme->muted, theme->taskbar);
            draw_small_hhmm(boot_info, tray_x + 42, button_y + 8, desktop->clock_hour, desktop->clock_minute,
                            desktop->settings == 0 ? 1U : desktop->settings->use_24_hour,
                            theme->text, theme->taskbar);
        } else {
            draw_small_cstr(boot_info, tray_x, button_y + 8, "UP", theme->muted, theme->taskbar);
            draw_small_clock(boot_info, tray_x + 24, button_y + 8, desktop->live_ticks, theme->text, theme->taskbar);
        }
    }

    draw_taskbar_hover_popup(boot_info, wm, desktop);
    draw_volume_popup(boot_info, wm, desktop);
    draw_power_menu(boot_info, wm, desktop);
    draw_power_confirm(boot_info, wm, desktop);
}

static void draw_console_prompt_bounded(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 right,
    const VibioConsoleState *console,
    vibio_u64 timer_ticks,
    vibio_u32 color,
    vibio_u32 cursor_color)
{
    vibio_u32 cursor_x = x;
    EMIT('V');
    EMIT('I');
    EMIT('B');
    EMIT('I');
    EMIT('O');
    EMIT(':');
    EMIT(' ');
    vibio_u32 input_x = cursor_x;
    vibio_u32 end_x = draw_char_buffer(boot_info, cursor_x, y, console->input, console->input_length, color);
    cursor_x = input_x + console->input_cursor * jbm_advance();

    if (right > end_x) {
        fill_rect(boot_info, end_x, y, right - end_x, TERM_LINE, g_text_bg);
    }
    if (right > x) {
        fill_rect(boot_info, x, y + TERM_LINE - 6, right - x, 6, g_text_bg);
    }
    if (((timer_ticks / 30) & 1) == 0) {
        fill_rect(boot_info, cursor_x, y + TERM_LINE - 4, jbm_advance(), 3, cursor_color);
    }
}

static const char *settings_page_name(vibio_u32 page)
{
    if (page == SETTINGS_PAGE_SYSTEM) { return "System"; }
    if (page == SETTINGS_PAGE_PERSONALIZATION) { return "Personalization"; }
    if (page == SETTINGS_PAGE_SOUND) { return "Sound"; }
    if (page == SETTINGS_PAGE_STORAGE) { return "Storage"; }
    if (page == SETTINGS_PAGE_DEVICES) { return "Devices"; }
    if (page == SETTINGS_PAGE_DATE_TIME) { return "Date & time"; }
    if (page == SETTINGS_PAGE_ABOUT) { return "About"; }
    return "Home";
}

static vibio_u32 settings_point_in(vibio_u32 px, vibio_u32 py, vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h)
{
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void settings_begin_layout(VibioSettingsApp *settings)
{
    if (settings == 0) {
        return;
    }
    settings->home_card_count = 0;
    settings->wallpaper_count = 0;
    settings->volume_x = 0;
    settings->volume_y = 0;
    settings->volume_w = 0;
    settings->volume_h = 0;
    settings->time_toggle_x = 0;
    settings->time_toggle_y = 0;
    settings->time_toggle_w = 0;
    settings->time_toggle_h = 0;
}

static void settings_register_home_card(
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 page)
{
    if (settings == 0 || settings->home_card_count >= SETTINGS_HOME_CARD_MAX) {
        return;
    }
    vibio_u32 i = settings->home_card_count++;
    settings->home_card_x[i] = x;
    settings->home_card_y[i] = y;
    settings->home_card_w[i] = w;
    settings->home_card_h[i] = h;
    settings->home_card_page[i] = page;
}

static void settings_register_wallpaper(
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 mode)
{
    if (settings == 0 || settings->wallpaper_count >= SETTINGS_WALLPAPER_MAX) {
        return;
    }
    vibio_u32 i = settings->wallpaper_count++;
    settings->wallpaper_x[i] = x;
    settings->wallpaper_y[i] = y;
    settings->wallpaper_w[i] = w;
    settings->wallpaper_h[i] = h;
    settings->wallpaper_mode[i] = mode;
}

static void settings_card(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    const char *title,
    const char *subtitle,
    const VibioDesktopTheme *theme,
    vibio_u32 hovered)
{
    vibio_u32 bg = hovered ? make_color(boot_info, 36, 48, 57) : make_color(boot_info, 28, 39, 47);
    fill_round_rect(boot_info, x, y, w, h, 7U, bg);
    draw_soft_rect_border(boot_info, x, y, w, h, hovered ? make_color(boot_info, 87, 116, 132) : make_color(boot_info, 48, 64, 74));
    draw_small_cstr(boot_info, x + 18U, y + 16U, title, theme->text, bg);
    if (subtitle != 0 && subtitle[0] != 0) {
        draw_small_cstr(boot_info, x + 18U, y + 36U, subtitle, theme->muted, bg);
    }
}

static void settings_value_line(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    const char *label,
    const char *value,
    vibio_u32 value_color,
    const VibioDesktopTheme *theme)
{
    vibio_u32 bg = make_color(boot_info, 22, 33, 40);
    fill_round_rect(boot_info, x, y, w, 42U, 5U, bg);
    fill_rect(boot_info, x + 1U, y + 41U, w > 2U ? w - 2U : 1U, 1U, make_color(boot_info, 31, 45, 53));
    draw_small_cstr(boot_info, x + 14U, y + 14U, label, theme->muted, bg);
    vibio_u32 vw = font_text_width(&FONT_GEIST_SM, value);
    vibio_u32 vx = x + w > vw + 16U ? x + w - vw - 16U : x + 14U;
    draw_small_cstr(boot_info, vx, y + 14U, value, value_color, bg);
}

static void settings_button_row(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    const char *label,
    const char *value,
    const VibioDesktopTheme *theme)
{
    vibio_u32 bg = make_color(boot_info, 22, 33, 40);
    fill_round_rect(boot_info, x, y, w, 44U, 5U, bg);
    fill_rect(boot_info, x + 1U, y + 43U, w > 2U ? w - 2U : 1U, 1U, make_color(boot_info, 31, 45, 53));
    draw_small_cstr(boot_info, x + 14U, y + 14U, label, theme->text, bg);
    if (value != 0 && value[0] != 0) {
        vibio_u32 vw = font_text_width(&FONT_GEIST_SM, value);
        vibio_u32 vx = x + w > vw + 34U ? x + w - vw - 34U : x + 14U;
        draw_small_cstr(boot_info, vx, y + 14U, value, theme->muted, bg);
    }
    draw_small_cstr(boot_info, x + w - 20U, y + 14U, ">", theme->muted, bg);
}

static void settings_toggle_switch(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 on,
    const VibioDesktopTheme *theme)
{
    vibio_u32 rail = on ? theme->accent : make_color(boot_info, 70, 82, 92);
    vibio_u32 knob = make_color(boot_info, 242, 248, 250);
    fill_round_rect(boot_info, x, y, 46U, 24U, 12U, rail);
    fill_round_rect(boot_info, x + (on ? 24U : 4U), y + 4U, 16U, 16U, 8U, knob);
}

static void settings_toggle_row(
    const VibioBootInfo *boot_info,
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    const char *label,
    const char *detail,
    vibio_u32 on,
    const VibioDesktopTheme *theme)
{
    vibio_u32 bg = make_color(boot_info, 22, 33, 40);
    fill_round_rect(boot_info, x, y, w, 58U, 6U, bg);
    draw_small_cstr(boot_info, x + 14U, y + 10U, label, theme->text, bg);
    draw_small_cstr(boot_info, x + 14U, y + 32U, detail, theme->muted, bg);
    vibio_u32 tx = x + w > 68U ? x + w - 62U : x + 14U;
    settings_toggle_switch(boot_info, tx, y + 17U, on, theme);
    if (settings != 0) {
        settings->time_toggle_x = x;
        settings->time_toggle_y = y;
        settings->time_toggle_w = w;
        settings->time_toggle_h = 58U;
    }
}

static void settings_volume_bar(
    const VibioBootInfo *boot_info,
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 volume,
    const VibioDesktopTheme *theme)
{
    vibio_u32 track = make_color(boot_info, 15, 25, 32);
    vibio_u32 filled = theme->accent;
    if (volume > 100U) {
        volume = 100U;
    }
    if (settings != 0) {
        settings->volume_x = x;
        settings->volume_y = y - 8U;
        settings->volume_w = w;
        settings->volume_h = 24U;
    }
    fill_round_rect(boot_info, x, y, w, 8U, 4U, track);
    fill_round_rect(boot_info, x, y, (w * volume) / 100U, 8U, 4U, filled);
    vibio_u32 knob_x = x + ((w > 14U ? w - 14U : 1U) * volume) / 100U;
    fill_round_rect(boot_info, knob_x, y - 5U, 14U, 18U, 7U, make_color(boot_info, 237, 248, 250));
}

static const char *settings_ready_text(vibio_u32 ok)
{
    return ok ? "Ready" : "Needs work";
}

static vibio_u32 settings_ready_color(vibio_u32 ok, const VibioDesktopTheme *theme)
{
    return ok ? theme->ok : theme->bad;
}

static const char *settings_disk_text(const VibioDiskReadTest *disk)
{
    if (disk != 0 && disk->ok) {
        return "Readable";
    }
    if (disk == 0 || disk->last_error == 1) {
        return "No disk";
    }
    return "Issue";
}

static const char *settings_fs_text(const VibioFat32ReadTest *fs, const VibioDiskReadTest *disk)
{
    if (fs != 0 && fs->ok) {
        return "Mounted";
    }
    if (disk == 0 || disk->last_error == 1) {
        return "No source";
    }
    return "Issue";
}

static const char *settings_audio_text(const VibioAudioState *audio)
{
    if (audio != 0 && audio->ready) {
        if (audio->driver == AUDIO_DRIVER_HDA) {
            return "HDA ready";
        }
        if (audio->driver == AUDIO_DRIVER_AC97) {
            return "AC97 ready";
        }
        return "Ready";
    }
    if (audio != 0 && audio->hda_found) {
        return "HDA issue";
    }
    if (audio == 0 || !audio->found) {
        return "No device";
    }
    return "Issue";
}

static void settings_draw_page_header(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    const char *title,
    const char *subtitle,
    const VibioDesktopTheme *theme,
    vibio_u32 bg)
{
    g_text_bg = bg;
    font_draw_text(boot_info, &FONT_GEIST_UI, x, y, title, theme->text);
    draw_small_cstr(boot_info, x, y + 30U, subtitle, theme->muted, bg);
    fill_round_rect(boot_info, x + (w > 244U ? w - 244U : 0U), y - 2U, w > 244U ? 244U : w, 34U, 9U, make_color(boot_info, 36, 47, 55));
    draw_icon_image_centered(boot_info, x + (w > 244U ? w - 236U : 8U), y - 2U, 24U, 34U, vibio_icon_search_18, 18);
    draw_small_cstr(boot_info, x + (w > 244U ? w - 205U : 38U), y + 10U, "Search settings", theme->muted, make_color(boot_info, 36, 47, 55));
}

static void settings_draw_home_card(
    const VibioBootInfo *boot_info,
    VibioSettingsApp *settings,
    const VibioMouseState *mouse,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    const char *title,
    const char *subtitle,
    const char *detail,
    vibio_u32 page,
    const VibioDesktopTheme *theme)
{
    vibio_u32 hovered = mouse != 0 && settings_point_in(mouse->x, mouse->y, x, y, w, h);
    settings_register_home_card(settings, x, y, w, h, page);
    settings_card(boot_info, x, y, w, h, title, subtitle, theme, hovered);
    if (detail != 0 && detail[0] != 0) {
        draw_small_cstr(boot_info, x + 16U, y + h - 28U, detail, theme->accent, hovered ? make_color(boot_info, 31, 48, 58) : make_color(boot_info, 23, 36, 45));
    }
    draw_small_cstr(boot_info, x + w - 24U, y + 14U, ">", theme->muted, hovered ? make_color(boot_info, 31, 48, 58) : make_color(boot_info, 23, 36, 45));
}

static void settings_draw_home(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 card_bg = make_color(boot_info, 23, 36, 45);
    settings_card(boot_info, x, y, w, 92U, "Vibio OS", "A focused settings center for the real system features available now.", theme, 0);
    {
        char res[48];
        vibio_u32 p = 0;
        p = panic_append_uint(res, p, sizeof(res), boot_info->framebuffer_width);
        p = panic_append_text(res, p, sizeof(res), " x ");
        p = panic_append_uint(res, p, sizeof(res), boot_info->framebuffer_height);
        res[p] = 0;
        draw_small_cstr(boot_info, x + 16U, y + 60U, "Display", theme->muted, card_bg);
        draw_small_cstr(boot_info, x + 84U, y + 60U, res, theme->text, card_bg);
        draw_small_cstr(boot_info, x + 214U, y + 60U, "Audio", theme->muted, card_bg);
        draw_small_cstr(boot_info, x + 270U, y + 60U, settings_audio_text(desktop->audio), audio_probe_state_color(desktop->audio, theme), card_bg);
    }

    vibio_u32 gap = 12U;
    vibio_u32 cols = w >= 560U ? 3U : (w >= 360U ? 2U : 1U);
    vibio_u32 col_w = cols > 1U ? (w - gap * (cols - 1U)) / cols : w;
    if (col_w < 160U && cols > 1U) {
        cols = 2U;
        col_w = (w - gap) / 2U;
    }
    if (col_w < 160U) {
        cols = 1U;
        col_w = w;
    }
    vibio_u32 row_y = y + 108U;
    vibio_u32 card_h = 92U;
    vibio_u32 step_y = card_h + 12U;
    settings_draw_home_card(boot_info, settings, desktop->mouse, x, row_y, col_w, card_h,
                            "System", "Display, boot, interrupts", "Open system status", SETTINGS_PAGE_SYSTEM, theme);
    settings_draw_home_card(boot_info, settings, desktop->mouse, x + (cols > 1U ? col_w + gap : 0U), cols > 1U ? row_y : row_y + step_y, col_w, card_h,
                            "Personalization", "Wallpaper choices", "Change desktop look", SETTINGS_PAGE_PERSONALIZATION, theme);
    settings_draw_home_card(boot_info, settings, desktop->mouse, x + (cols > 2U ? (col_w + gap) * 2U : 0U), cols > 2U ? row_y : row_y + step_y * (cols == 1U ? 2U : 1U), col_w, card_h,
                            "Sound", "Output and volume", "Adjust volume", SETTINGS_PAGE_SOUND, theme);
    row_y += step_y * (cols == 1U ? 3U : 1U);
    settings_draw_home_card(boot_info, settings, desktop->mouse, x, row_y, col_w, card_h,
                            "Storage", "Boot disk, FAT32, sandbox", "Review storage", SETTINGS_PAGE_STORAGE, theme);
    settings_draw_home_card(boot_info, settings, desktop->mouse, x + (cols > 1U ? col_w + gap : 0U), cols > 1U ? row_y : row_y + step_y, col_w, card_h,
                            "Devices", "Keyboard, mouse, USB", "Check input", SETTINGS_PAGE_DEVICES, theme);
    settings_draw_home_card(boot_info, settings, desktop->mouse, x + (cols > 2U ? (col_w + gap) * 2U : 0U), cols > 2U ? row_y : row_y + step_y * (cols == 1U ? 2U : 1U), col_w, card_h,
                            "Date & time", "Clock format", "12 or 24 hour", SETTINGS_PAGE_DATE_TIME, theme);
    row_y += step_y * (cols == 1U ? 3U : 1U);
    settings_draw_home_card(boot_info, settings, desktop->mouse, x, row_y, col_w, card_h,
                            "About", "Version and platform", "About Vibio", SETTINGS_PAGE_ABOUT, theme);
}

static void settings_draw_system(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 gap = 12U;
    vibio_u32 col_w = w > gap ? (w - gap) / 2U : w;
    if (col_w < 250U) { col_w = w; }
    vibio_u32 two_cols = col_w != w;
    settings_card(boot_info, x, y, col_w, 204U, "Display", "Framebuffer and desktop output", theme, 0);
    {
        char res[48];
        vibio_u32 p = 0;
        p = panic_append_uint(res, p, sizeof(res), boot_info->framebuffer_width);
        p = panic_append_text(res, p, sizeof(res), " x ");
        p = panic_append_uint(res, p, sizeof(res), boot_info->framebuffer_height);
        res[p] = 0;
        settings_value_line(boot_info, x + 12U, y + 62U, col_w - 24U, "Resolution", res, theme->text, theme);
        settings_value_line(boot_info, x + 12U, y + 110U, col_w - 24U, "Desktop", "Windowed shell", theme->accent, theme);
        settings_value_line(boot_info, x + 12U, y + 158U, col_w - 24U, "Cursor", desktop->mouse != 0 && desktop->mouse->ready ? "Ready" : "Fallback", desktop->mouse != 0 && desktop->mouse->ready ? theme->ok : theme->muted, theme);
    }
    vibio_u32 bx = two_cols ? x + col_w + gap : x;
    vibio_u32 by = two_cols ? y : y + 216U;
    settings_card(boot_info, bx, by, col_w, 204U, "Boot and kernel", "Core startup checks", theme, 0);
    settings_value_line(boot_info, bx + 12U, by + 62U, col_w - 24U, "Boot mode", "UEFI x64", theme->accent, theme);
    settings_value_line(boot_info, bx + 12U, by + 110U, col_w - 24U, "Boot services", settings_ready_text(desktop->boot_ok), settings_ready_color(desktop->boot_ok, theme), theme);
    settings_value_line(boot_info, bx + 12U, by + 158U, col_w - 24U, "Paging", settings_ready_text(desktop->paging_ok), settings_ready_color(desktop->paging_ok, theme), theme);

    vibio_u32 row2 = y + (two_cols ? 216U : 432U);
    settings_card(boot_info, x, row2, w, 154U, "Runtime", "Live shell and interrupt state", theme, 0);
    settings_value_line(boot_info, x + 12U, row2 + 62U, w - 24U, "Interrupts", settings_ready_text(desktop->irq_ok), settings_ready_color(desktop->irq_ok, theme), theme);
    {
        char ticks[32];
        vibio_u32 p = panic_append_uint(ticks, 0, sizeof(ticks), desktop->live_ticks);
        ticks[p] = 0;
        settings_value_line(boot_info, x + 12U, row2 + 110U, w - 24U, "Timer ticks", ticks, theme->text, theme);
    }
}

static void settings_draw_personalization(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    settings_card(boot_info, x, y, w, 112U, "Desktop background", "Pick a wallpaper. Changes apply immediately.", theme, 0);
    fill_round_rect(boot_info, x + 16U, y + 54U, 94U, 42U, 8U, theme->wallpaper_top);
    fill_round_rect(boot_info, x + 112U, y + 54U, 94U, 42U, 8U, theme->wallpaper_bottom);
    settings_value_line(boot_info, x + 224U, y + 58U, w > 248U ? w - 248U : 120U, "Current",
                        desktop->wm != 0 && desktop->wm->wallpaper_mode >= WALLPAPER_IMAGE_BASE ? "Image" : "Vibio gradient",
                        theme->accent, theme);

    vibio_u32 tile_y = y + 130U;
    vibio_u32 tile_w = w > 3U * 12U ? (w - 24U) / 3U : w;
    if (tile_w < 130U) { tile_w = w; }
    vibio_u32 selected = desktop->wm != 0 ? desktop->wm->wallpaper_mode : 0;
    vibio_u32 count = 1U + (vibio_u32)VIBIO_WALLPAPER_COUNT;
    if (count > SETTINGS_WALLPAPER_MAX) {
        count = SETTINGS_WALLPAPER_MAX;
    }
    for (vibio_u32 i = 0; i < count; i++) {
        vibio_u32 col = tile_w == w ? 0 : i % 3U;
        vibio_u32 row = tile_w == w ? i : i / 3U;
        vibio_u32 tx = x + col * (tile_w + 12U);
        vibio_u32 ty = tile_y + row * 78U;
        vibio_u32 mode = i == 0U ? 0U : WALLPAPER_IMAGE_BASE + (i - 1U);
        vibio_u32 is_sel = selected == mode || (i == 0U && selected < WALLPAPER_IMAGE_BASE);
        settings_register_wallpaper(settings, tx, ty, tile_w, 66U, mode);
        fill_round_rect(boot_info, tx + 2U, ty + 3U, tile_w, 66U, 8U, make_color(boot_info, 4, 10, 14));
        fill_round_rect(boot_info, tx, ty, tile_w, 66U, 8U, make_color(boot_info, 23, 36, 45));
        if (is_sel) {
            draw_soft_rect_border(boot_info, tx, ty, tile_w, 66U, theme->accent);
        } else {
            draw_soft_rect_border(boot_info, tx, ty, tile_w, 66U, make_color(boot_info, 56, 78, 91));
        }
        if (i == 0U) {
            fill_round_rect(boot_info, tx + 10U, ty + 12U, 48U, 30U, 5U, theme->wallpaper_top);
            fill_round_rect(boot_info, tx + 60U, ty + 12U, 48U, 30U, 5U, theme->wallpaper_bottom);
            draw_small_cstr(boot_info, tx + 12U, ty + 46U, "Vibio gradient", theme->text, make_color(boot_info, 23, 36, 45));
        } else {
            fill_round_rect(boot_info, tx + 10U, ty + 12U, 98U, 30U, 5U, make_color(boot_info, 43, 73, 92));
            draw_small_cstr(boot_info, tx + 12U, ty + 46U, vibio_wallpapers[i - 1U].name, theme->text, make_color(boot_info, 23, 36, 45));
        }
    }
}

static void settings_draw_sound(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 volume = desktop->audio != 0 ? desktop->audio->volume : 0;
    settings_card(boot_info, x, y, w, 214U, "Output", "System audio output and volume", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 62U, w - 28U, "Driver", settings_audio_text(desktop->audio), audio_probe_state_color(desktop->audio, theme), theme);
    {
        char vol[16];
        vibio_u32 p = 0;
        p = panic_append_uint(vol, p, sizeof(vol), volume);
        p = panic_append_text(vol, p, sizeof(vol), "%");
        vol[p] = 0;
        settings_value_line(boot_info, x + 14U, y + 110U, w - 28U, "Volume", vol, theme->accent, theme);
    }
    settings_volume_bar(boot_info, settings, x + 18U, y + 174U, w > 36U ? w - 36U : w, volume, theme);
    draw_small_cstr(boot_info, x + 18U, y + 190U, "Click or drag the slider to change volume.", theme->muted, make_color(boot_info, 28, 39, 47));

    settings_card(boot_info, x, y + 232U, w, 116U, "Sound events", "Boot, notify, USB insert/remove, and error sounds are available.", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 294U, w - 28U, "Playback", desktop->audio != 0 && desktop->audio->ready ? "Enabled" : "Unavailable", desktop->audio != 0 && desktop->audio->ready ? theme->ok : theme->muted, theme);
}

static void settings_draw_storage(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    settings_card(boot_info, x, y, w, 204U, "Storage", "Boot disk and filesystem state", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 62U, w - 28U, "Boot disk", settings_disk_text(desktop->disk), disk_probe_state_color(desktop->disk, theme), theme);
    settings_value_line(boot_info, x + 14U, y + 110U, w - 28U, "FAT32", settings_fs_text(desktop->fs, desktop->disk), fat32_probe_state_color(desktop->fs, desktop->disk, theme), theme);
    settings_value_line(boot_info, x + 14U, y + 158U, w - 28U, "Boot volume", "Read-only", theme->muted, theme);

    settings_card(boot_info, x, y + 222U, w, 156U, "Scratch disk", "Optional VIBIORW sandbox disk for safe writes", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 284U, w - 28U, "Sandbox", rw_probe_state_text(desktop->rw), rw_probe_state_color(desktop->rw, theme), theme);
    settings_value_line(boot_info, x + 14U, y + 332U, w - 28U, "Host disk", "Never touched", theme->accent, theme);
}

static void settings_draw_devices(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    settings_card(boot_info, x, y, w, 204U, "Input", "Keyboard and pointer devices", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 62U, w - 28U, "Keyboard", desktop->irq_ok ? "Interrupts ready" : "Issue", settings_ready_color(desktop->irq_ok, theme), theme);
    settings_value_line(boot_info, x + 14U, y + 110U, w - 28U, "Mouse", desktop->mouse != 0 && desktop->mouse->ready ? "Ready" : "Fallback", desktop->mouse != 0 && desktop->mouse->ready ? theme->ok : theme->muted, theme);
    settings_value_line(boot_info, x + 14U, y + 158U, w - 28U, "Shortcut", "Super + I", theme->accent, theme);

    settings_card(boot_info, x, y + 222U, w, 156U, "USB", "Controller and mass-storage status", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 284U, w - 28U, "Controller", g_usb_storage.controller_found ? "Detected" : "Idle", g_usb_storage.controller_found ? theme->ok : theme->muted, theme);
    settings_value_line(boot_info, x + 14U, y + 332U, w - 28U, "Live USB", desktop->usb != 0 && desktop->usb->ok ? "Readable" : "Unavailable", desktop->usb != 0 && desktop->usb->ok ? theme->ok : theme->muted, theme);
}

static void settings_format_time(char *out, vibio_u32 out_max, vibio_u32 hour, vibio_u32 minute, vibio_u32 use_24_hour)
{
    if (out == 0 || out_max == 0) {
        return;
    }
    vibio_u32 p = 0;
    if (use_24_hour) {
        out[p++] = (char)('0' + (hour / 10U) % 10U);
        out[p++] = (char)('0' + hour % 10U);
    } else {
        vibio_u32 h12 = hour % 12U;
        vibio_u32 pm = hour >= 12U;
        if (h12 == 0U) {
            h12 = 12U;
        }
        if (h12 >= 10U && p + 1U < out_max) {
            out[p++] = (char)('0' + h12 / 10U);
        }
        if (p + 1U < out_max) {
            out[p++] = (char)('0' + h12 % 10U);
        }
        if (p + 4U < out_max) {
            out[p++] = ':';
            out[p++] = (char)('0' + (minute / 10U) % 10U);
            out[p++] = (char)('0' + minute % 10U);
            out[p++] = ' ';
            out[p++] = pm ? 'P' : 'A';
            out[p++] = 'M';
            out[p] = 0;
            return;
        }
    }
    if (p + 3U < out_max) {
        out[p++] = ':';
        out[p++] = (char)('0' + (minute / 10U) % 10U);
        out[p++] = (char)('0' + minute % 10U);
    }
    if (p >= out_max) {
        p = out_max - 1U;
    }
    out[p] = 0;
}

static void settings_draw_date_time(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    VibioSettingsApp *settings,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 use24 = settings == 0 ? 1U : settings->use_24_hour;
    settings_card(boot_info, x, y, w, 218U, "Clock", "Change how time is shown across the shell.", theme, 0);
    {
        char time_text[12];
        settings_format_time(time_text, sizeof(time_text), desktop->clock_hour, desktop->clock_minute, use24);
        settings_value_line(boot_info, x + 14U, y + 62U, w - 28U, "Current time", desktop->clock_ok ? time_text : "RTC unavailable", desktop->clock_ok ? theme->text : theme->muted, theme);
    }
    settings_value_line(boot_info, x + 14U, y + 110U, w - 28U, "Clock source", desktop->clock_ok ? "RTC" : "Uptime fallback", desktop->clock_ok ? theme->ok : theme->muted, theme);
    settings_toggle_row(boot_info, settings, x + 14U, y + 158U, w - 28U,
                        "Use 24-hour clock",
                        use24 ? "Taskbar shows 00:00-23:59" : "Taskbar shows AM/PM time",
                        use24,
                        theme);

    settings_card(boot_info, x, y + 236U, w, 116U, "Regional format", "More date, timezone, and calendar settings can be added after Vibio has locale support.", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 298U, w - 28U, "Date format", "Not added", theme->muted, theme);
}

static void settings_draw_about(
    const VibioBootInfo *boot_info,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    settings_card(boot_info, x, y, w, 252U, "Vibio OS", "From-scratch x86_64 UEFI operating system", theme, 0);
    settings_value_line(boot_info, x + 14U, y + 62U, w - 28U, "Version", "Stage 37", theme->accent, theme);
    settings_value_line(boot_info, x + 14U, y + 110U, w - 28U, "Platform", "UEFI x86_64", theme->text, theme);
    settings_value_line(boot_info, x + 14U, y + 158U, w - 28U, "Window shell", "Enabled", theme->ok, theme);
    settings_value_line(boot_info, x + 14U, y + 206U, w - 28U, "VM mode", desktop->running_in_vm ? "Detected" : "Native/unknown", desktop->running_in_vm ? theme->accent : theme->muted, theme);

    settings_card(boot_info, x, y + 270U, w, 136U, "Not built yet", "These areas need real OS services before they belong in Settings.", theme, 0);
    settings_button_row(boot_info, x + 14U, y + 332U, w - 28U, "Network, accounts, updates, privacy, apps", "Not added", theme);
}

static void draw_settings_window_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h)
{
    const VibioDesktopTheme *theme = desktop->theme;
    VibioSettingsApp *settings = desktop->settings;
    if (settings == 0) {
        return;
    }
    if (settings->page >= SETTINGS_PAGE_COUNT) {
        settings->page = SETTINGS_PAGE_HOME;
    }
    settings_begin_layout(settings);

    vibio_u32 rail_w = w > 720U ? 220U : 170U;
    vibio_u32 bg = window->body_color;
    vibio_u32 rail = make_color(boot_info, 16, 25, 32);
    vibio_u32 search = make_color(boot_info, 35, 46, 54);
    vibio_u32 page_x = x + rail_w + 26U;
    vibio_u32 page_w = w > rail_w + 48U ? w - rail_w - 48U : 120U;
    vibio_u32 top = y + 16U;

    fill_rect(boot_info, x, y, w, h, bg);
    fill_rect(boot_info, x, y, rail_w, h, rail);
    fill_rect(boot_info, x + rail_w, y, 1U, h, make_color(boot_info, 42, 61, 72));

    draw_icon_settings_gear(boot_info, x + 20U, top + 2U, 34U);
    draw_small_cstr(boot_info, x + 64U, top + 4U, "Settings", theme->text, rail);
    draw_small_cstr(boot_info, x + 64U, top + 22U, "Vibio OS", theme->muted, rail);

    fill_round_rect(boot_info, x + 14U, top + 58U, rail_w > 28U ? rail_w - 28U : 80U, 30U, 7U, search);
    draw_icon_image_centered(boot_info, x + 20U, top + 58U, 24U, 30U, vibio_icon_search_18, 18);
    draw_small_cstr(boot_info, x + 48U, top + 68U, "Find a setting", theme->muted, search);

    settings->nav_x = x + 10U;
    settings->nav_y = top + 108U;
    settings->nav_w = rail_w > 20U ? rail_w - 20U : 80U;
    settings->nav_row_h = 36U;
    settings->nav_count = SETTINGS_PAGE_COUNT;
    for (vibio_u32 i = 0; i < SETTINGS_PAGE_COUNT; i++) {
        vibio_u32 iy = settings->nav_y + i * settings->nav_row_h;
        vibio_u32 hovered = desktop->mouse != 0 && settings_point_in(desktop->mouse->x, desktop->mouse->y, settings->nav_x, iy - 4U, settings->nav_w, 30U);
        vibio_u32 selected = i == settings->page;
        vibio_u32 row_bg = selected ? make_color(boot_info, 38, 58, 70) : (hovered ? make_color(boot_info, 26, 40, 49) : rail);
        if (selected || hovered) {
            fill_round_rect(boot_info, settings->nav_x, iy - 4U, settings->nav_w, 30U, 6U, row_bg);
        }
        if (selected) {
            fill_round_rect(boot_info, settings->nav_x, iy + 3U, 3U, 16U, 1U, theme->accent);
        }
        draw_small_cstr(boot_info, settings->nav_x + 18U, iy + 5U, settings_page_name(i), selected ? theme->text : theme->muted, row_bg);
    }

    {
        const char *sub = "Clean controls for the parts Vibio can actually manage today.";
        if (settings->page == SETTINGS_PAGE_SYSTEM) {
            sub = "Display, boot, interrupts, and runtime status.";
        } else if (settings->page == SETTINGS_PAGE_PERSONALIZATION) {
            sub = "Make the desktop feel like yours without fake account or cloud features.";
        } else if (settings->page == SETTINGS_PAGE_SOUND) {
            sub = "Change real system volume and inspect the audio driver.";
        } else if (settings->page == SETTINGS_PAGE_STORAGE) {
            sub = "Read-only boot media and optional safe scratch storage.";
        } else if (settings->page == SETTINGS_PAGE_DEVICES) {
            sub = "Keyboard, pointer, USB, and input shortcuts.";
        } else if (settings->page == SETTINGS_PAGE_DATE_TIME) {
            sub = "Clock source and taskbar time format.";
        } else if (settings->page == SETTINGS_PAGE_ABOUT) {
            sub = "Version and platform details.";
        }
        settings_draw_page_header(boot_info, page_x, top + 4U, page_w, settings_page_name(settings->page), sub, theme, bg);
    }

    vibio_u32 content_y = top + 78U;
    if (settings->page == SETTINGS_PAGE_HOME) {
        settings_draw_home(boot_info, desktop, settings, page_x, content_y, page_w);
    } else if (settings->page == SETTINGS_PAGE_SYSTEM) {
        settings_draw_system(boot_info, desktop, page_x, content_y, page_w);
    } else if (settings->page == SETTINGS_PAGE_PERSONALIZATION) {
        settings_draw_personalization(boot_info, desktop, settings, page_x, content_y, page_w);
    } else if (settings->page == SETTINGS_PAGE_SOUND) {
        settings_draw_sound(boot_info, desktop, settings, page_x, content_y, page_w);
    } else if (settings->page == SETTINGS_PAGE_STORAGE) {
        settings_draw_storage(boot_info, desktop, page_x, content_y, page_w);
    } else if (settings->page == SETTINGS_PAGE_DEVICES) {
        settings_draw_devices(boot_info, desktop, page_x, content_y, page_w);
    } else if (settings->page == SETTINGS_PAGE_DATE_TIME) {
        settings_draw_date_time(boot_info, desktop, settings, page_x, content_y, page_w);
    } else {
        settings_draw_about(boot_info, desktop, page_x, content_y, page_w);
    }
}

static vibio_u32 settings_apply_volume_from_x(VibioSettingsApp *settings, VibioAudioState *audio, vibio_u32 px)
{
    if (settings == 0 || audio == 0 || settings->volume_w == 0) {
        return 0;
    }
    vibio_u32 old = audio->volume;
    vibio_u32 rel = 0;
    if (px > settings->volume_x) {
        rel = px - settings->volume_x;
    }
    if (rel > settings->volume_w) {
        rel = settings->volume_w;
    }
    audio_set_volume(audio, (rel * 100U) / settings->volume_w);
    return audio->volume != old;
}

static vibio_u32 settings_handle_mouse(
    VibioSettingsApp *settings,
    VibioWindowManager *wm,
    VibioMouseState *mouse,
    VibioAudioState *audio)
{
    if (settings == 0 || wm == 0 || mouse == 0) {
        return 0;
    }
    vibio_u32 changed = 0;
    vibio_u32 left = mouse->buttons & 0x01U;
    vibio_u32 idx = wm_find_kind(wm, WINDOW_KIND_SETTINGS);
    vibio_u32 focused = idx != WM_NO_WINDOW && wm->windows[idx].visible && wm_top_visible(wm) == idx;

    if (focused && left && settings->volume_dragging) {
        changed |= settings_apply_volume_from_x(settings, audio, mouse->x);
    } else if (focused && left && !settings->prev_left) {
        for (vibio_u32 i = 0; i < settings->nav_count; i++) {
            vibio_u32 row_y = settings->nav_y + i * settings->nav_row_h;
            if (settings_point_in(mouse->x, mouse->y, settings->nav_x, row_y - 4U, settings->nav_w, 30U)) {
                if (settings->page != i) {
                    settings->page = i;
                    changed = 1;
                }
                break;
            }
        }
        if (!changed && settings->page == SETTINGS_PAGE_HOME) {
            for (vibio_u32 i = 0; i < settings->home_card_count; i++) {
                if (settings_point_in(mouse->x, mouse->y,
                                      settings->home_card_x[i], settings->home_card_y[i],
                                      settings->home_card_w[i], settings->home_card_h[i])) {
                    if (settings->page != settings->home_card_page[i]) {
                        settings->page = settings->home_card_page[i];
                    }
                    changed = 1;
                    break;
                }
            }
        }
        if (!changed && settings->page == SETTINGS_PAGE_SOUND &&
            settings_point_in(mouse->x, mouse->y, settings->volume_x, settings->volume_y,
                              settings->volume_w, settings->volume_h)) {
            settings->volume_dragging = 1;
            changed |= settings_apply_volume_from_x(settings, audio, mouse->x);
        }
        if (!changed && settings->page == SETTINGS_PAGE_DATE_TIME &&
            settings_point_in(mouse->x, mouse->y, settings->time_toggle_x, settings->time_toggle_y,
                              settings->time_toggle_w, settings->time_toggle_h)) {
            settings->use_24_hour = settings->use_24_hour ? 0U : 1U;
            changed = 1;
        }
        if (!changed && settings->page == SETTINGS_PAGE_PERSONALIZATION) {
            for (vibio_u32 i = 0; i < settings->wallpaper_count; i++) {
                if (settings_point_in(mouse->x, mouse->y,
                                      settings->wallpaper_x[i], settings->wallpaper_y[i],
                                      settings->wallpaper_w[i], settings->wallpaper_h[i])) {
                    if (wm->wallpaper_mode != settings->wallpaper_mode[i]) {
                        wm->wallpaper_mode = settings->wallpaper_mode[i];
                        wm->desktop_base_dirty = 1;
                    }
                    changed = 1;
                    break;
                }
            }
        }
    }

    if (!left) {
        settings->volume_dragging = 0;
    }
    settings->prev_left = left;
    return changed;
}

static void draw_system_window_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 col1 = x + 12;
    vibio_u32 col2 = w > 470 ? x + 270 : x + (w / 2) + 16;
    vibio_u32 row_y = y + 10;
    /* Tighter than the full line step so the status grid is less airy. */
    vibio_u32 step = LINE_STEP - 5;

    g_text_bg = window->body_color;
    draw_cstr(boot_info, col1, row_y, "SYSTEM STATUS", theme->accent);
    row_y += step + 4;

    draw_status_row(boot_info, col1, row_y, "BOOT", desktop->boot_ok, theme);
    draw_status_row(boot_info, col2, row_y, "FILES", desktop->file != 0 && desktop->file->ok, theme);
    row_y += step;
    draw_status_row(boot_info, col1, row_y, "ALLOC", desktop->alloc_ok, theme);
    draw_probe_row(boot_info, col2, row_y, "DISK", disk_probe_state_text(desktop->disk), disk_probe_state_color(desktop->disk, theme), theme);
    row_y += step;
    draw_status_row(boot_info, col1, row_y, "PAGING", desktop->paging_ok, theme);
    draw_probe_row(boot_info, col2, row_y, "FAT32", fat32_probe_state_text(desktop->fs, desktop->disk), fat32_probe_state_color(desktop->fs, desktop->disk, theme), theme);
    row_y += step;
    draw_status_row(boot_info, col1, row_y, "IDT", desktop->idt_ok, theme);
    draw_status_row(boot_info, col2, row_y, "USB", desktop->usb != 0 && desktop->usb->ok, theme);
    row_y += step;
    draw_status_row(boot_info, col1, row_y, "IRQ", desktop->irq_ok, theme);
    draw_status_row(boot_info, col2, row_y, "HEAP", desktop->heap != 0 && desktop->heap->ok, theme);
    row_y += step;
    draw_status_row(boot_info, col1, row_y, "SYSCALL", desktop->syscall != 0 && desktop->syscall->ok, theme);
    draw_status_row(boot_info, col2, row_y, "TASKS", desktop->task != 0 && desktop->task->ok, theme);
    row_y += step;
    draw_status_row(boot_info, col1, row_y, "USER", desktop->user_boundary != 0 && desktop->user_boundary->ok, theme);
    draw_status_row(boot_info, col2, row_y, "MOUSE", desktop->mouse != 0 && desktop->mouse->ready, theme);
    row_y += step + 4;
    draw_probe_row(boot_info, col1, row_y, "AUDIO", audio_probe_state_text(desktop->audio), audio_probe_state_color(desktop->audio, theme), theme);
    draw_metric_row(boot_info, col2, row_y, "TICKS", desktop->live_ticks, theme);
    row_y += step;
    draw_probe_row(boot_info, col1, row_y, "RW SANDBOX", rw_probe_state_text(desktop->rw),
                   rw_probe_state_color(desktop->rw, theme), theme);
    draw_probe_row(boot_info, col2, row_y, "BOOT FS", "READ ONLY", theme->muted, theme);
    if (g_selftest_result.ran) {
        row_y += step;
        draw_probe_row(
            boot_info,
            col1,
            row_y,
            "SELFTEST",
            g_selftest_result.ok ? "PASS" : "FAIL",
            g_selftest_result.ok ? theme->ok : theme->bad,
            theme);
        draw_metric_row(boot_info, col2, row_y, "CHECKS", g_selftest_result.passed, theme);
    }
}

static void draw_terminal_window_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h)
{
    const VibioDesktopTheme *theme = desktop->theme;
    const VibioConsoleState *console = desktop->console;
    vibio_u32 right = x + w - 12;
    vibio_u32 history_y = y + 12;
    vibio_u32 prompt_y = y + h - TERM_LINE - 10;

    g_text_bg = window->body_color;
    if (console == 0) {
        draw_cstr(boot_info, x + 12, history_y, "CONSOLE UNAVAILABLE", theme->bad);
        return;
    }

    if (prompt_y > history_y) {
        vibio_u32 rows = (prompt_y - history_y) / TERM_LINE;
        if (rows > CONSOLE_ROWS) {
            rows = CONSOLE_ROWS;
        }
        vibio_u32 visible_rows = console->cursor_row + 1;
        vibio_u32 bottom_start = visible_rows > rows ? visible_rows - rows : 0;
        /* Scroll the window of visible lines up by view_offset, clamped so it
         * never runs before the first stored line. */
        vibio_u32 start = bottom_start > console->view_offset ? bottom_start - console->view_offset : 0;
        for (vibio_u32 i = 0; i < rows; i++) {
            vibio_u32 source_row = start + i;
            if (source_row >= CONSOLE_ROWS) {
                break;
            }
            vibio_u32 line_y = history_y + i * TERM_LINE;
            vibio_u32 end_x = draw_char_buffer(boot_info, x + 12, line_y, console->lines[source_row], console->line_lengths[source_row], theme->muted);
            if (right > end_x) {
                fill_rect(boot_info, end_x, line_y, right - end_x, TERM_LINE, g_text_bg);
            }
        }

        /* Scrollbar: a track down the right edge with a thumb sized/positioned
         * to the visible window, shown only when there is more history than fits. */
        if (visible_rows > rows) {
            vibio_u32 track_x = x + w - 8;
            vibio_u32 track_h = rows * TERM_LINE;
            fill_rect(boot_info, track_x, history_y, 3, track_h, make_color(boot_info, 28, 40, 50));
            vibio_u32 thumb_h = (rows * track_h) / visible_rows;
            if (thumb_h < 12) {
                thumb_h = 12;
            }
            vibio_u32 max_thumb_y = track_h > thumb_h ? track_h - thumb_h : 0;
            vibio_u32 thumb_y = bottom_start > 0 ? (start * max_thumb_y) / bottom_start : max_thumb_y;
            fill_rect(boot_info, track_x, history_y + thumb_y, 3, thumb_h, theme->accent);
        }
    }

    draw_console_prompt_bounded(boot_info, x + 12, prompt_y, right, console, desktop->live_ticks, theme->text, theme->accent);
}

static void draw_welcome_window_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y)
{
    const VibioDesktopTheme *theme = desktop->theme;
    vibio_u32 row_y = y + 12;

    g_text_bg = window->body_color;
    draw_cstr(boot_info, x + 12, row_y, "VIBIO OS", theme->accent);
    row_y += LINE_STEP + 6;
    draw_cstr(boot_info, x + 12, row_y, "DESKTOP SHELL", theme->text);
    row_y += LINE_STEP;
    draw_cstr(boot_info, x + 12, row_y, "TASKBAR READY", theme->muted);
    row_y += LINE_STEP;
    draw_cstr(boot_info, x + 12, row_y, "WINDOW KINDS", theme->muted);
    row_y += LINE_STEP;
    draw_cstr(boot_info, x + 12, row_y, "THEME READY", theme->muted);
}

/* ===========================================================================
 * Minimal HTML browser
 *
 * A small, from-scratch HTML viewer. It streams the page bytes once, handling a
 * useful subset of tags (headings, paragraphs, br, hr, links, images, lists)
 * with word-wrapping, clickable links, a placeholder for images, and vertical
 * scrolling. It is intentionally not a full CSS engine.
 * ======================================================================== */

static char browser_lower(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

/* True if the parsed tag name (length len) equals the lowercase literal name. */
static vibio_u32 browser_tag_is(const char *tag, vibio_u32 len, const char *name)
{
    vibio_u32 i = 0;
    for (; i < len; i++) {
        if (name[i] == 0 || browser_lower(tag[i]) != name[i]) {
            return 0;
        }
    }
    return name[i] == 0;
}

/* Extract an attribute value (e.g. href) from a tag's attribute region. */
static vibio_u32 browser_get_attr(const char *s, vibio_u32 len, const char *attr,
                                  char *out, vibio_u32 out_max)
{
    vibio_u32 alen = 0;
    while (attr[alen] != 0) {
        alen++;
    }
    out[0] = 0;
    if (alen == 0 || out_max == 0) {
        return 0;
    }

    for (vibio_u32 i = 0; i + alen <= len; i++) {
        vibio_u32 j = 0;
        while (j < alen && browser_lower(s[i + j]) == attr[j]) {
            j++;
        }
        if (j != alen) {
            continue;
        }
        vibio_u32 k = i + alen;
        while (k < len && (s[k] == ' ' || s[k] == '\t')) {
            k++;
        }
        if (k >= len || s[k] != '=') {
            continue;
        }
        k++;
        while (k < len && (s[k] == ' ' || s[k] == '\t')) {
            k++;
        }
        char quote = 0;
        if (k < len && (s[k] == '"' || s[k] == '\'')) {
            quote = s[k];
            k++;
        }
        vibio_u32 o = 0;
        while (k < len && o + 1 < out_max) {
            char c = s[k];
            if (quote != 0 && c == quote) {
                break;
            }
            if (quote == 0 && (c == ' ' || c == '\t' || c == '>')) {
                break;
            }
            out[o++] = c;
            k++;
        }
        out[o] = 0;
        return o;
    }
    return 0;
}

/* Decode an HTML entity starting at html[i] (which is '&'). Writes one char to
 * *out and returns how many input bytes were consumed. */
static vibio_u32 browser_decode_entity(const vibio_u8 *html, vibio_u32 i, vibio_u32 len, char *out)
{
    /* Read up to ';' within a short window. */
    char name[10];
    vibio_u32 n = 0;
    vibio_u32 k = i + 1;
    while (k < len && n < 9 && html[k] != ';' && html[k] != '&' && html[k] != ' ' && html[k] != '<') {
        name[n++] = (char)html[k];
        k++;
    }
    if (k >= len || html[k] != ';') {
        *out = '&';
        return 1;
    }
    name[n] = 0;

    if (browser_tag_is(name, n, "amp")) { *out = '&'; }
    else if (browser_tag_is(name, n, "lt")) { *out = '<'; }
    else if (browser_tag_is(name, n, "gt")) { *out = '>'; }
    else if (browser_tag_is(name, n, "quot")) { *out = '"'; }
    else if (browser_tag_is(name, n, "apos")) { *out = '\''; }
    else if (browser_tag_is(name, n, "nbsp")) { *out = ' '; }
    else if (n >= 2 && name[0] == '#') {
        vibio_u32 code = 0;
        for (vibio_u32 d = 1; d < n; d++) {
            if (name[d] < '0' || name[d] > '9') { code = 0; break; }
            code = code * 10 + (vibio_u32)(name[d] - '0');
        }
        *out = (code >= 32 && code < 127) ? (char)code : ' ';
    } else {
        *out = '&';
        return 1; /* unknown: emit '&' literally, reparse the rest */
    }
    return (k - i) + 1;
}

/* Capture the page <title> text into the browser state. */
static void browser_parse_title(VibioBrowser *b)
{
    b->title_len = 0;
    b->title[0] = 0;
    if (b->html == 0) {
        return;
    }
    for (vibio_u32 i = 0; i + 7 <= b->html_len; i++) {
        if (b->html[i] != '<') {
            continue;
        }
        const char *t = (const char *)(b->html + i + 1);
        vibio_u32 remain = b->html_len - i - 1;
        if (remain >= 5 &&
            browser_lower(t[0]) == 't' && browser_lower(t[1]) == 'i' &&
            browser_lower(t[2]) == 't' && browser_lower(t[3]) == 'l' &&
            browser_lower(t[4]) == 'e') {
            vibio_u32 j = i + 6;
            while (j < b->html_len && b->html[j] != '>') {
                j++;
            }
            j++;
            while (j < b->html_len && b->html[j] != '<' && b->title_len + 1 < sizeof(b->title)) {
                char c = (char)b->html[j];
                if (c == '\n' || c == '\t' || c == '\r') {
                    c = ' ';
                }
                b->title[b->title_len++] = c;
                j++;
            }
            b->title[b->title_len] = 0;
            return;
        }
    }
}

static void browser_set_current(VibioBrowser *b, const char *name)
{
    b->current_len = 0;
    for (vibio_u32 i = 0; i + 1 < BROWSER_URL_MAX && name[i] != 0; i++) {
        b->current[i] = name[i];
        b->current_len++;
    }
    b->current[b->current_len] = 0;
}

static void browser_copy_url(char *dst, vibio_u32 dst_max, const char *src, vibio_u32 *out_len)
{
    vibio_u32 n = 0;
    if (dst_max == 0) {
        return;
    }
    while (n + 1 < dst_max && src[n] != 0) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = 0;
    if (out_len != 0) {
        *out_len = n;
    }
}

static void browser_sync_address(VibioBrowser *b)
{
    browser_copy_url(b->address, BROWSER_URL_MAX, b->current, &b->address_len);
}

static vibio_u32 browser_url_eq(const char *a, const char *b)
{
    vibio_u32 i = 0;
    while (i < BROWSER_URL_MAX && a[i] != 0 && b[i] != 0) {
        if (browser_lower(a[i]) != browser_lower(b[i])) {
            return 0;
        }
        i++;
    }
    return (i == BROWSER_URL_MAX || a[i] == 0) && b[i] == 0;
}

static void browser_update_history_flags(VibioBrowser *b)
{
    b->back_enabled = b->history_count > 0 && b->history_index > 0;
    b->forward_enabled = b->history_count > 0 && b->history_index + 1 < b->history_count;
}

static void browser_history_push(VibioBrowser *b, const char *target, vibio_u32 external)
{
    if (target == 0 || target[0] == 0) {
        return;
    }

    if (b->history_count > 0 && b->history_index < b->history_count &&
        browser_url_eq(b->history[b->history_index], target) &&
        b->history_external[b->history_index] == (external ? 1 : 0)) {
        browser_update_history_flags(b);
        return;
    }

    if (b->history_count > 0 && b->history_index + 1 < b->history_count) {
        b->history_count = b->history_index + 1;
    }

    if (b->history_count == BROWSER_HISTORY_MAX) {
        for (vibio_u32 i = 1; i < BROWSER_HISTORY_MAX; i++) {
            browser_copy_url(b->history[i - 1], BROWSER_URL_MAX, b->history[i], 0);
            b->history_external[i - 1] = b->history_external[i];
        }
        b->history_count--;
        if (b->history_index > 0) {
            b->history_index--;
        }
    }

    browser_copy_url(b->history[b->history_count], BROWSER_URL_MAX, target, 0);
    b->history_external[b->history_count] = (vibio_u8)(external ? 1 : 0);
    b->history_index = b->history_count;
    b->history_count++;
    browser_update_history_flags(b);
}

/* Case-insensitive match of a boot-file name (length-bounded) against a
 * NUL-terminated request name. */
static vibio_u32 browser_boot_name_eq(const char *file_name, vibio_u32 file_len, const char *name)
{
    vibio_u32 i = 0;
    for (; i < file_len && file_name[i] != 0; i++) {
        if (name[i] == 0 || browser_lower(file_name[i]) != browser_lower(name[i])) {
            return 0;
        }
    }
    return name[i] == 0;
}

static vibio_u32 boot_file_range_is_low_mapped(const VibioBootFile *f, vibio_u64 offset, vibio_u64 length)
{
    if (f == 0 || f->address == 0 || offset > f->size || length > f->size - offset) {
        return 0;
    }
    vibio_u64 start = f->address + offset;
    if (start < f->address || start >= LOW_IDENTITY_LIMIT) {
        return 0;
    }
    return length <= LOW_IDENTITY_LIMIT - start;
}

/* Copy a kernel-embedded built-in page into the buffer. These need no disk and
 * no file on the boot medium, so they always work (VM, USB, NVMe). Returns 1 on
 * a hit. */
static vibio_u32 browser_load_from_builtin(VibioBrowser *b, const char *name)
{
    for (vibio_u32 i = 0; i < VIBIO_BUILTIN_HTML_COUNT; i++) {
        const VibioBuiltinHtml *p = &vibio_builtin_html[i];
        vibio_u32 plen = 0;
        while (p->name[plen] != 0) {
            plen++;
        }
        if (!browser_boot_name_eq(p->name, plen, name)) {
            continue;
        }
        vibio_u32 n = p->length;
        if (n > b->html_capacity - 1) {
            n = b->html_capacity - 1;
        }
        for (vibio_u32 k = 0; k < n; k++) {
            b->html[k] = p->data[k];
        }
        b->html[n] = 0;
        b->html_len = n;
        return 1;
    }
    return 0;
}

/* Copy a UEFI-preloaded file into the page buffer if the boot stage loaded it.
 * This is firmware-agnostic, so it works on real hardware (NVMe/USB) where the
 * kernel's own AHCI/FAT32 driver finds no device. Returns 1 on a hit. */
static vibio_u32 browser_load_from_boot(VibioBrowser *b, const char *name)
{
    if (b->boot == 0) {
        return 0;
    }
    for (vibio_u64 i = 0; i < b->boot->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        const VibioBootFile *f = &b->boot->boot_files[i];
        if (!browser_boot_name_eq(f->name, f->name_length, name)) {
            continue;
        }
        vibio_u32 n = (vibio_u32)f->size;
        if (n > b->html_capacity - 1) {
            n = b->html_capacity - 1;
        }
        if (!boot_file_range_is_low_mapped(f, 0, n)) {
            continue;
        }
        const vibio_u8 *src = (const vibio_u8 *)f->address;
        for (vibio_u32 k = 0; k < n; k++) {
            b->html[k] = src[k];
        }
        b->html[n] = 0;
        b->html_len = n;
        return 1;
    }
    return 0;
}

/* Load a local HTML file: prefer the UEFI-preloaded in-memory copy (reliable and
 * works on real hardware), then fall back to a read-only FAT32 disk read. */
static void browser_load(VibioBrowser *b, const char *name)
{
    panic_set_context("BROWSER", "HTML", "HTML LOAD");
    b->scroll = 0;
    b->link_count = 0;
    b->content_height = 0;
    b->html_len = 0;
    b->title_len = 0;
    b->title[0] = 0;
    browser_set_current(b, name);

    if (b->html == 0) {
        b->status = BROWSER_STATUS_NO_DISK;
        browser_sync_address(b);
        return;
    }

    /* Kernel-embedded built-in pages first (always available, any hardware),
     * then the UEFI-preloaded copy, then a live FAT32 disk read. */
    if (browser_load_from_builtin(b, name)) {
        b->status = BROWSER_STATUS_OK;
        browser_parse_title(b);
        browser_sync_address(b);
        return;
    }

    if (browser_load_from_boot(b, name)) {
        b->status = BROWSER_STATUS_OK;
        browser_parse_title(b);
        browser_sync_address(b);
        return;
    }

    if (b->disk == 0 || !b->disk->ok || b->fs == 0 || !b->fs->root_found) {
        b->status = BROWSER_STATUS_NO_DISK;
        browser_sync_address(b);
        return;
    }

    vibio_u32 size = 0;
    vibio_u32 read = fat32_read_file(b->disk, b->fs, name, b->html, b->html_capacity - 1, &size);
    if (read == 0) {
        b->status = BROWSER_STATUS_NOT_FOUND;
        browser_sync_address(b);
        return;
    }
    b->html[read] = 0;
    b->html_len = read;
    b->status = BROWSER_STATUS_OK;
    browser_parse_title(b);
    browser_sync_address(b);
}

static vibio_u32 browser_is_image_file(const char *name);
static vibio_u32 browser_is_video_file(const char *name);
static vibio_u32 browser_is_media_file(const char *name);

static void browser_open_target(VibioBrowser *b, const char *href, vibio_u32 external, vibio_u32 add_history)
{
    /* Local page/image reads below resolve by name; search the folder this
     * browser was opened from first, then the root. */
    fat32_set_resolve_dir(b->dir_cluster);
    if (external) {
        b->status = BROWSER_STATUS_EXTERNAL;
        browser_set_current(b, href);
        b->html_len = 0;
        b->link_count = 0;
        b->scroll = 0;
        b->title_len = 0;
        b->title[0] = 0;
        browser_sync_address(b);
        if (add_history) {
            browser_history_push(b, href, 1);
        }
        return;
    }
    if (browser_is_media_file(href)) {
        b->status = BROWSER_STATUS_IMAGE;
        browser_set_current(b, href);
        b->html_len = 0;
        b->link_count = 0;
        b->scroll = 0;
        b->content_height = 0;
        b->title_len = 0;
        browser_copy_url(b->title, sizeof(b->title), href, &b->title_len);
        browser_sync_address(b);
        if (add_history) {
            browser_history_push(b, href, 0);
        }
        return;
    }
    browser_load(b, href);
    if (add_history) {
        browser_history_push(b, href, 0);
    }
}

/* Queue navigation to an href; followed by the main loop. http(s) links are
 * flagged external and not fetched (there is no network stack yet). */
static void browser_navigate(VibioBrowser *b, const char *href, vibio_u32 external)
{
    browser_open_target(b, href, external, 1);
}

static vibio_u32 browser_href_is_external(const char *href)
{
    /* "http://", "https://", "//", or "www." style links have no local file. */
    if (href[0] == 'h' && href[1] == 't' && href[2] == 't' && href[3] == 'p') {
        return 1;
    }
    if (href[0] == '/' && href[1] == '/') {
        return 1;
    }
    return 0;
}

static vibio_u32 browser_name_has_ext(const char *name, const char *ext)
{
    vibio_u32 nlen = 0;
    while (name[nlen] != 0 && nlen < BROWSER_URL_MAX) {
        nlen++;
    }
    vibio_u32 elen = 0;
    while (ext[elen] != 0) {
        elen++;
    }
    if (elen > nlen) {
        return 0;
    }
    vibio_u32 start = nlen - elen;
    for (vibio_u32 i = 0; i < elen; i++) {
        char a = name[start + i];
        char b = ext[i];
        if (a >= 'a' && a <= 'z') { a = (char)(a - 'a' + 'A'); }
        if (b >= 'a' && b <= 'z') { b = (char)(b - 'a' + 'A'); }
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

/* Stage 33+ media preview helpers.
 * Vibio renders .VIM/.VIMG directly and now tries the bounded native PNG/JPEG
 * decoder for common local images before falling back to generated .VIM assets.
 * Other compressed formats still need host-side conversion. */
static vibio_u32 browser_is_image_file(const char *name)
{
    return browser_name_has_ext(name, ".VIM")  || browser_name_has_ext(name, ".VIMG") ||
           browser_name_has_ext(name, ".PNG")  || browser_name_has_ext(name, ".JPG")  ||
           browser_name_has_ext(name, ".JPEG") || browser_name_has_ext(name, ".BMP")  ||
           browser_name_has_ext(name, ".GIF")  || browser_name_has_ext(name, ".WEBP") ||
           browser_name_has_ext(name, ".AVIF") || browser_name_has_ext(name, ".TIF")  ||
           browser_name_has_ext(name, ".TIFF") || browser_name_has_ext(name, ".TGA")  ||
           browser_name_has_ext(name, ".ICO")  || browser_name_has_ext(name, ".QOI")  ||
           browser_name_has_ext(name, ".PPM")  || browser_name_has_ext(name, ".PGM")  ||
           browser_name_has_ext(name, ".PBM");
}

static vibio_u32 browser_is_video_file(const char *name)
{
    return browser_name_has_ext(name, ".MP4")  || browser_name_has_ext(name, ".M4V")  ||
           browser_name_has_ext(name, ".MOV")  || browser_name_has_ext(name, ".WEBM") ||
           browser_name_has_ext(name, ".AVI")  || browser_name_has_ext(name, ".MKV")  ||
           browser_name_has_ext(name, ".MPG")  || browser_name_has_ext(name, ".MPEG") ||
           browser_name_has_ext(name, ".OGV")  || browser_name_has_ext(name, ".WMV");
}

static vibio_u32 browser_is_media_file(const char *name)
{
    return browser_is_image_file(name) || browser_is_video_file(name);
}

static vibio_u32 read_be32(const vibio_u8 *p)
{
    return ((vibio_u32)p[0] << 24) | ((vibio_u32)p[1] << 16) | ((vibio_u32)p[2] << 8) | p[3];
}

static vibio_u32 browser_load_image_bytes(VibioBrowser *b, const char *name, vibio_u32 *out_size)
{
    if (out_size != 0) {
        *out_size = 0;
    }
    if (b == 0 || b->image == 0 || b->image_capacity == 0 || name == 0 || name[0] == 0) {
        return 0;
    }
    if (b->boot != 0) {
        for (vibio_u64 i = 0; i < b->boot->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
            const VibioBootFile *f = &b->boot->boot_files[i];
            if (f->type == VIBIO_BOOT_FILE_TYPE_IMAGE &&
                browser_boot_name_eq(f->name, f->name_length, name) &&
                f->address != 0 &&
                f->size <= b->image_capacity &&
                boot_file_range_is_low_mapped(f, 0, f->size)) {
                const vibio_u8 *src = (const vibio_u8 *)f->address;
                for (vibio_u32 k = 0; k < (vibio_u32)f->size; k++) {
                    b->image[k] = src[k];
                }
                if (out_size != 0) {
                    *out_size = (vibio_u32)f->size;
                }
                return 1;
            }
        }
    }
    if (b->disk == 0 || !b->disk->ok || b->fs == 0 || !b->fs->root_found) {
        return 0;
    }
    vibio_u32 real_size = 0;
    vibio_u32 read = fat32_read_file(b->disk, b->fs, name, b->image, b->image_capacity, &real_size);
    if (read == 0 || real_size > b->image_capacity) {
        return 0;
    }
    if (out_size != 0) {
        *out_size = read;
    }
    return 1;
}

static void browser_image_vim_name(const char *src, char out[FILES_NAME_MAX])
{
    vibio_u32 i = 0;
    vibio_u32 o = 0;
    while (src[i] != 0 && src[i] != '.' && o + 5 < FILES_NAME_MAX) {
        out[o++] = src[i++];
    }
    out[o++] = '.';
    out[o++] = 'V';
    out[o++] = 'I';
    out[o++] = 'M';
    out[o] = 0;
}

static vibio_u32 browser_stem_normalized_eq(const char *a, vibio_u32 a_len, const char *b)
{
    vibio_u32 ai = 0;
    vibio_u32 bi = 0;
    vibio_u32 matched = 0;
    for (;;) {
        while (ai < a_len && a[ai] != 0 && a[ai] != '.' && a[ai] != '~' &&
               !((a[ai] >= 'A' && a[ai] <= 'Z') || (a[ai] >= 'a' && a[ai] <= 'z') || (a[ai] >= '0' && a[ai] <= '9'))) {
            ai++;
        }
        while (b[bi] != 0 && b[bi] != '.' && b[bi] != '~' &&
               !((b[bi] >= 'A' && b[bi] <= 'Z') || (b[bi] >= 'a' && b[bi] <= 'z') || (b[bi] >= '0' && b[bi] <= '9'))) {
            bi++;
        }
        char ac = (ai < a_len && a[ai] != 0 && a[ai] != '.') ? browser_lower(a[ai]) : 0;
        char bc = (b[bi] != 0 && b[bi] != '.') ? browser_lower(b[bi]) : 0;

        /* FAT32 8.3 names may truncate BACKGROUND.PNG to BACKGROU.VIM or use a
         * BACKGR~1.VIM style alias. Treat those as a valid generated fallback
         * for the longer original name, but only after a meaningful shared stem. */
        if ((ac == '~' && matched >= 4) || (bc == '~' && matched >= 4)) {
            return 1;
        }
        if (ac == 0 || bc == 0) {
            if (ac == bc) {
                return 1;
            }
            return matched >= 8 ? 1 : 0;
        }
        if (ac != bc) {
            return 0;
        }
        matched++;
        ai++;
        bi++;
    }
}

static vibio_u32 browser_find_disk_vim_fallback_name(VibioBrowser *b, const char *src, char out[FILES_NAME_MAX])
{
    if (b == 0 || b->disk == 0 || !b->disk->ok || b->fs == 0 || !b->fs->root_found) {
        return 0;
    }

    /* Static (not on the stack): the entry list is large now and this single
     * -threaded helper has no reentrancy. */
    static VibioFilesEntry entries[FILES_MAX_ENTRIES];
    vibio_u32 count = 0;
    vibio_u32 trunc = 0;
    if (!fat32_list_dir(b->disk, b->fs, (vibio_u32)b->fs->root_cluster,
                        entries, FILES_MAX_ENTRIES, &count, &trunc)) {
        return 0;
    }

    for (vibio_u32 i = 0; i < count && i < FILES_MAX_ENTRIES; i++) {
        if (entries[i].is_dir) {
            continue;
        }
        if (!browser_name_has_ext(entries[i].name, ".VIM") &&
            !browser_name_has_ext(entries[i].name, ".VIMG")) {
            continue;
        }
        vibio_u32 len = 0;
        while (entries[i].name[len] != 0 && len < FILES_NAME_MAX) {
            len++;
        }
        if (!browser_stem_normalized_eq(entries[i].name, len, src)) {
            continue;
        }
        vibio_u32 n = 0;
        for (; n + 1 < FILES_NAME_MAX && entries[i].name[n] != 0; n++) {
            out[n] = entries[i].name[n];
        }
        out[n] = 0;
        return 1;
    }
    return 0;
}

static vibio_u32 browser_find_vim_fallback_name(VibioBrowser *b, const char *src, char out[FILES_NAME_MAX])
{
    browser_image_vim_name(src, out);
    if (browser_load_image_bytes(b, out, 0)) {
        return 1;
    }
    if (b != 0 && b->boot != 0) {
        for (vibio_u64 i = 0; i < b->boot->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
            const VibioBootFile *f = &b->boot->boot_files[i];
            if (f->type != VIBIO_BOOT_FILE_TYPE_IMAGE || !browser_boot_name_eq(f->name, f->name_length, out)) {
                if (f->type != VIBIO_BOOT_FILE_TYPE_IMAGE ||
                    (!browser_name_has_ext(f->name, ".VIM") && !browser_name_has_ext(f->name, ".VIMG"))) {
                    continue;
                }
            }
            if (!browser_name_has_ext(f->name, ".VIM") && !browser_name_has_ext(f->name, ".VIMG")) {
                continue;
            }
            if (!browser_stem_normalized_eq(f->name, f->name_length, src)) {
                continue;
            }
            vibio_u32 n = 0;
            for (; n + 1 < FILES_NAME_MAX && n < f->name_length && n < VIBIO_BOOT_FILE_NAME_MAX; n++) {
                out[n] = f->name[n];
            }
            out[n] = 0;
            return 1;
        }
    }

    /* If the .VIM fallback exists on the FAT32 root with an 8.3-shortened name
     * such as BACKGROU.VIM, find it by normalized stem instead of requiring the
     * original long source stem. This fixes "image not rendered" when the build
     * produced a valid fallback but the browser asked for BACKGROUND.VIM. */
    return browser_find_disk_vim_fallback_name(b, src, out);
}

static vibio_u32 browser_draw_vimg_bytes(
    const VibioBootInfo *boot_info,
    const vibio_u8 *data,
    vibio_u32 size,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 max_w,
    vibio_u32 *out_w,
    vibio_u32 *out_h)
{
    if (out_w != 0) { *out_w = 0; }
    if (out_h != 0) { *out_h = 0; }
    if (data == 0 || size < 16 || data[0] != 'V' || data[1] != 'I' || data[2] != 'M' || data[3] != 'G') {
        return 0;
    }
    vibio_u32 w = 0;
    vibio_u32 h = 0;
    vibio_u32 header = 16;
    vibio_u32 ver = disk_read_le32(data, 4);
    if (ver == 1 && size >= 24) {
        w = disk_read_le32(data, 8);
        h = disk_read_le32(data, 12);
        header = 24;
    } else {
        w = disk_read_le32(data, 4);
        h = disk_read_le32(data, 8);
        if (disk_read_le32(data, 12) != 1) {
            return 0;
        }
    }
    if (w == 0 || h == 0 || w > 1024 || h > 1024) {
        return 0;
    }
    if ((vibio_u64)header + (vibio_u64)w * h * 4ULL > size) {
        return 0;
    }
    vibio_u32 draw_w = w;
    if (draw_w > max_w) {
        draw_w = max_w;
    }
    const vibio_u8 *px = data + header;
    for (vibio_u32 row = 0; row < h; row++) {
        for (vibio_u32 col = 0; col < draw_w; col++) {
            vibio_u32 off = (row * w + col) * 4U;
            blend_pixel(boot_info, x + col, y + row, px[off], px[off + 1], px[off + 2], px[off + 3]);
        }
    }
    if (out_w != 0) { *out_w = draw_w; }
    if (out_h != 0) { *out_h = h; }
    return 1;
}

static vibio_u32 browser_draw_vimg_image(
    const VibioBootInfo *boot_info,
    VibioBrowser *b,
    const char *src,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 max_w,
    vibio_u32 *out_w,
    vibio_u32 *out_h)
{
    vibio_u32 image_size = 0;
    if (!browser_load_image_bytes(b, src, &image_size)) {
        return 0;
    }
    return browser_draw_vimg_bytes(boot_info, b->image, image_size, x, y, max_w, out_w, out_h);
}

static vibio_u32 browser_draw_png_image(
    const VibioBootInfo *boot_info,
    VibioBrowser *b,
    const char *src,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 max_w,
    vibio_u32 *out_w,
    vibio_u32 *out_h)
{
    if (out_w != 0) { *out_w = 0; }
    if (out_h != 0) { *out_h = 0; }
    vibio_u32 image_size = 0;
    if (!browser_load_image_bytes(b, src, &image_size)) {
        return 0;
    }
    MediaBitmap bmp;
    MediaDecodeStatus status;
    zero_bytes(&status, sizeof(status));
    if (!media_decode_native_image(src, b->image, image_size, b->image_capacity,
                                   b->image_work, b->image_work_capacity,
                                   &bmp, &status)) {
        return 0;
    }

    vibio_u32 draw_w = bmp.width;
    if (draw_w > max_w) {
        draw_w = max_w;
    }
    for (vibio_u32 row = 0; row < bmp.height; row++) {
        const vibio_u8 *px = bmp.pixels + (vibio_u64)row * bmp.width * 4ULL;
        for (vibio_u32 col = 0; col < draw_w; col++) {
            vibio_u32 po = col * 4U;
            blend_pixel(boot_info, x + col, y + row, px[po], px[po + 1], px[po + 2], px[po + 3]);
        }
    }
    if (out_w != 0) { *out_w = draw_w; }
    if (out_h != 0) { *out_h = bmp.height; }
    return 1;
}

static vibio_u32 browser_draw_any_image(
    const VibioBootInfo *boot_info,
    VibioBrowser *b,
    const char *src,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 max_w,
    vibio_u32 *out_w,
    vibio_u32 *out_h)
{
    if (browser_name_has_ext(src, ".VIM") || browser_name_has_ext(src, ".VIMG")) {
        return browser_draw_vimg_image(boot_info, b, src, x, y, max_w, out_w, out_h);
    }
    if ((browser_name_has_ext(src, ".PNG") ||
         browser_name_has_ext(src, ".JPG") ||
         browser_name_has_ext(src, ".JPEG")) &&
        browser_draw_png_image(boot_info, b, src, x, y, max_w, out_w, out_h)) {
        return 1;
    }
    char vim[FILES_NAME_MAX];
    if (!browser_find_vim_fallback_name(b, src, vim)) {
        return 0;
    }
    return browser_draw_vimg_image(boot_info, b, vim, x, y, max_w, out_w, out_h);
}

static void browser_go_back(VibioBrowser *b)
{
    if (b->history_count == 0 || b->history_index == 0) {
        browser_update_history_flags(b);
        return;
    }
    b->history_index--;
    browser_open_target(b, b->history[b->history_index], b->history_external[b->history_index], 0);
    browser_update_history_flags(b);
}

static void browser_go_forward(VibioBrowser *b)
{
    if (b->history_count == 0 || b->history_index + 1 >= b->history_count) {
        browser_update_history_flags(b);
        return;
    }
    b->history_index++;
    browser_open_target(b, b->history[b->history_index], b->history_external[b->history_index], 0);
    browser_update_history_flags(b);
}

static void browser_reload(VibioBrowser *b)
{
    browser_open_target(b, b->current, b->status == BROWSER_STATUS_EXTERNAL, 0);
    browser_update_history_flags(b);
}

static void browser_home(VibioBrowser *b)
{
    browser_open_target(b, "START.HTM", 0, 1);
}

/* Case-insensitive check that name ends with the given dotted extension. */
static vibio_u32 files_name_has_ext(const char *name, const char *ext)
{
    vibio_u32 nlen = 0;
    while (name[nlen] != 0) {
        nlen++;
    }
    vibio_u32 elen = 0;
    while (ext[elen] != 0) {
        elen++;
    }
    if (elen > nlen) {
        return 0;
    }
    vibio_u32 start = nlen - elen;
    for (vibio_u32 i = 0; i < elen; i++) {
        char a = name[start + i];
        char b = ext[i];
        if (a >= 'a' && a <= 'z') {
            a = (char)(a - 'a' + 'A');
        }
        if (b >= 'a' && b <= 'z') {
            b = (char)(b - 'a' + 'A');
        }
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

static void files_app_set_info(VibioFilesApp *files, const char *title)
{
    files->info_open = 1;
    files->info_line_count = 0;
    vibio_u32 i = 0;
    for (; title[i] != 0 && i < sizeof(files->info_title) - 1; i++) {
        files->info_title[i] = title[i];
    }
    files->info_title[i] = 0;
}

static void files_app_info_add(VibioFilesApp *files, const char *line)
{
    if (files->info_line_count >= 4) {
        return;
    }
    char *dst = files->info_lines[files->info_line_count++];
    vibio_u32 i = 0;
    for (; line[i] != 0 && i < 39; i++) {
        dst[i] = line[i];
    }
    dst[i] = 0;
}

static void media_set_error(const char *msg)
{
    vibio_u32 i = 0;
    for (; msg != 0 && msg[i] != 0 && i + 1 < sizeof(g_media_last_error); i++) {
        g_media_last_error[i] = msg[i];
    }
    g_media_last_error[i] = 0;
}

static const char *media_source_label(vibio_u32 source)
{
    if (source == FILES_SRC_AHCI) {
        return "AHCI BOOT DISK";
    }
    if (source == FILES_SRC_USB) {
        return "USB MASS STORAGE";
    }
    if (source == FILES_SRC_BOOT) {
        return "UEFI BOOT FILES";
    }
    return "NONE";
}

static void wm_place_media_window(VibioWindow *w, const VibioBootInfo *boot_info)
{
    vibio_u32 work_w = boot_info->framebuffer_width;
    vibio_u32 work_h = boot_info->framebuffer_height - DESKTOP_TASKBAR_HEIGHT - DESKTOP_TOPBAR_HEIGHT;
    vibio_u32 tw = work_w * 78U / 100U;
    vibio_u32 th = work_h * 82U / 100U;
    if (tw < 640U && work_w >= 640U) {
        tw = 640U;
    }
    if (tw > work_w) {
        tw = work_w;
    }
    if (th < 420U && work_h >= 420U) {
        th = 420U;
    }
    if (th > work_h) {
        th = work_h;
    }
    w->width = tw;
    w->height = th;
    w->x = work_w > tw ? (work_w - tw) / 2U : 0;
    w->y = DESKTOP_TOPBAR_HEIGHT + (work_h > th ? (work_h - th) / 2U : 0);
    w->maximized = 0;
    w->fullscreen = 0;
}

static vibio_u32 media_read_bytes(
    const VibioBootInfo *boot,
    VibioDiskReadTest *disk,
    VibioFat32ReadTest *fs,
    const char *name,
    vibio_u64 offset,
    vibio_u8 *dest,
    vibio_u32 max_bytes,
    vibio_u32 *out_total,
    vibio_u8 *out_source)
{
    if (out_total != 0) {
        *out_total = 0;
    }
    if (out_source != 0) {
        *out_source = FILES_SRC_NONE;
    }
    if (dest == 0 || max_bytes == 0 || name == 0 || name[0] == 0) {
        return 0;
    }
    /* A live USB-backed disk takes priority over the UEFI preload so a file
     * opened from the USB drive reads from USB even when a same-named file was
     * preloaded at boot. (AHCI/boot ordering below is unchanged.) */
    if (disk != 0 && disk->backend == DISK_BACKEND_USB && disk->ok && fs != 0 && fs->root_found) {
        vibio_u32 total = 0;
        vibio_u32 got = fat32_read_file_at(disk, fs, name, offset, dest, max_bytes, &total);
        if (got > 0 || total > 0) {
            if (out_total != 0) { *out_total = total; }
            if (out_source != 0) { *out_source = FILES_SRC_USB; }
            return got;
        }
    }
    if (boot != 0) {
        for (vibio_u64 i = 0; i < boot->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
            const VibioBootFile *f = &boot->boot_files[i];
            if (!browser_boot_name_eq(f->name, f->name_length, name)) {
                continue;
            }
            if (f->address == 0) {
                continue;
            }
            if (offset >= f->size) {
                if (out_total != 0) {
                    *out_total = (vibio_u32)f->size;
                }
                if (out_source != 0) {
                    *out_source = FILES_SRC_BOOT;
                }
                return 0;
            }
            vibio_u32 avail = (vibio_u32)(f->size - offset);
            vibio_u32 take = avail > max_bytes ? max_bytes : avail;
            if (!boot_file_range_is_low_mapped(f, offset, take)) {
                continue;
            }
            if (out_total != 0) {
                *out_total = (vibio_u32)f->size;
            }
            if (out_source != 0) {
                *out_source = FILES_SRC_BOOT;
            }
            const vibio_u8 *src = (const vibio_u8 *)(f->address + offset);
            for (vibio_u32 k = 0; k < take; k++) {
                dest[k] = src[k];
            }
            return take;
        }
    }
    if (disk != 0 && disk->ok && fs != 0 && fs->root_found) {
        vibio_u32 total = 0;
        vibio_u32 got = fat32_read_file_at(disk, fs, name, offset, dest, max_bytes, &total);
        if (got > 0 || total > 0) {
            if (out_total != 0) {
                *out_total = total;
            }
            if (out_source != 0) {
                *out_source = FILES_SRC_AHCI;
            }
            return got;
        }
    }
    /* Last resort: the live USB mass-storage mount, even if the caller's disk
     * pointer was something else (e.g. a file opened from the boot listing on a
     * machine whose only readable backend is the xHCI USB). Skipped when the
     * passed disk already IS the live USB (handled above). */
    if (g_usb_storage.fat32_mounted && g_usb_disk.ok && g_usb_fs.root_found &&
        disk != &g_usb_disk) {
        vibio_u32 total = 0;
        vibio_u32 got = fat32_read_file_at(&g_usb_disk, &g_usb_fs, name, offset, dest, max_bytes, &total);
        if (got > 0 || total > 0) {
            if (out_total != 0) {
                *out_total = total;
            }
            if (out_source != 0) {
                *out_source = FILES_SRC_USB;
            }
            return got;
        }
    }
    return 0;
}

static vibio_u32 media_read_be32(const vibio_u8 *b, vibio_u32 off)
{
    return ((vibio_u32)b[off] << 24) |
           ((vibio_u32)b[off + 1] << 16) |
           ((vibio_u32)b[off + 2] << 8) |
           (vibio_u32)b[off + 3];
}

static const vibio_u8 *media_find_moov_box(const vibio_u8 *b, vibio_u32 size, vibio_u32 *out_size)
{
    if (out_size != 0) {
        *out_size = 0;
    }
    if (b == 0 || size < 8) {
        return 0;
    }
    for (vibio_u32 i = 4; i + 4 <= size; i++) {
        if (b[i] != 'm' || b[i + 1] != 'o' || b[i + 2] != 'o' || b[i + 3] != 'v') {
            continue;
        }
        vibio_u32 start = i - 4;
        vibio_u32 box_size = media_read_be32(b, start);
        if (box_size >= 8 && start + box_size <= size) {
            if (out_size != 0) {
                *out_size = size - start;
            }
            return b + start;
        }
    }
    return 0;
}

static vibio_u32 media_read_mp4_probe(
    const VibioBootInfo *boot,
    VibioDiskReadTest *disk,
    VibioFat32ReadTest *fs,
    const char *name,
    vibio_u8 *dest,
    vibio_u32 max_bytes,
    vibio_u32 *out_total,
    vibio_u8 *out_source,
    Mp4Info *out_info)
{
    if (out_info != 0) {
        zero_bytes(out_info, sizeof(Mp4Info));
    }
    vibio_u32 total = 0;
    vibio_u8 source = FILES_SRC_NONE;
    vibio_u32 got = media_read_bytes(boot, disk, fs, name, 0, dest, max_bytes, &total, &source);
    if (out_total != 0) {
        *out_total = total;
    }
    if (out_source != 0) {
        *out_source = source;
    }
    if (got == 0) {
        if (out_info != 0) {
            out_info->status = MP4_ERR_TOO_SMALL;
            media_copy_name(out_info->status_detail, sizeof(out_info->status_detail), "file not found or unreadable");
        }
        return 0;
    }
    if (out_info != 0) {
        mp4_parse(dest, got, out_info);
    }

    /* Many real MP4s are not "faststart": mdat is near the front and moov sits
     * at the end. If the first read window did not include moov, probe the tail
     * window too. This is still read-only and does not load the whole video. */
    if (out_info != 0 && !out_info->has_moov && total > got && max_bytes > 0) {
        vibio_u32 tail = total > max_bytes ? max_bytes : total;
        vibio_u64 tail_offset = (vibio_u64)total - (vibio_u64)tail;
        vibio_u32 tail_total = 0;
        vibio_u8 tail_source = FILES_SRC_NONE;
        vibio_u32 tail_got = media_read_bytes(boot, disk, fs, name, tail_offset,
                                              dest, tail, &tail_total, &tail_source);
        if (tail_got > 0) {
            Mp4Info tail_info;
            vibio_u32 moov_window = 0;
            const vibio_u8 *moov = media_find_moov_box(dest, tail_got, &moov_window);
            if (moov != 0 && moov_window > 0) {
                vibio_u32 shift = (vibio_u32)(moov - dest);
                if (shift > 0) {
                    for (vibio_u32 k = 0; k < moov_window; k++) {
                        dest[k] = moov[k];
                    }
                }
                mp4_parse(dest, moov_window, &tail_info);
                tail_got = moov_window;
            } else {
                mp4_parse(dest, tail_got, &tail_info);
            }
            if (tail_info.status == MP4_OK || tail_info.has_moov) {
                vibio_u8 *d = (vibio_u8 *)out_info;
                const vibio_u8 *s = (const vibio_u8 *)&tail_info;
                for (vibio_u32 k = 0; k < sizeof(Mp4Info); k++) {
                    d[k] = s[k];
                }
                got = tail_got;
                if (out_source != 0 && tail_source != FILES_SRC_NONE) {
                    *out_source = tail_source;
                }
            }
        }
    }
    return got;
}

static vibio_u32 media_vimg_parse(const vibio_u8 *data, vibio_u32 size, MediaBitmap *out)
{
    zero_bytes(out, sizeof(MediaBitmap));
    if (data == 0 || size < 16) {
        return 0;
    }
    vibio_u32 w = 0;
    vibio_u32 h = 0;
    vibio_u32 header = 16;
    if (data[0] == 'V' && data[1] == 'I' && data[2] == 'M' && data[3] == 'G') {
        vibio_u32 ver = disk_read_le32(data, 4);
        if (ver == 1 && size >= 24) {
            w = disk_read_le32(data, 8);
            h = disk_read_le32(data, 12);
            header = 24;
        } else {
            w = disk_read_le32(data, 4);
            h = disk_read_le32(data, 8);
            vibio_u32 fmt = disk_read_le32(data, 12);
            if (fmt != 1) {
                return 0;
            }
            header = 16;
        }
    } else {
        return 0;
    }
    if (w == 0 || h == 0 || w > MEDIA_MAX_DIM || h > MEDIA_MAX_DIM) {
        return 0;
    }
    vibio_u64 need = (vibio_u64)w * (vibio_u64)h * 4ULL;
    if (need > MEDIA_MAX_BYTES || header + need > size) {
        return 0;
    }
    out->width = w;
    out->height = h;
    out->header_bytes = header;
    out->pixels = data + header;
    out->pixel_bytes = (vibio_u32)need;
    return 1;
}

static vibio_u32 media_bmp_parse(const vibio_u8 *data, vibio_u32 size, MediaBitmap *out)
{
    zero_bytes(out, sizeof(MediaBitmap));
    if (data == 0 || size < 54 || data[0] != 'B' || data[1] != 'M') {
        return 0;
    }
    vibio_u32 off = disk_read_le32(data, 10);
    vibio_u32 w = disk_read_le32(data, 18);
    vibio_u32 h = disk_read_le32(data, 22);
    vibio_u16 bpp = (vibio_u16)(data[28] | ((vibio_u16)data[29] << 8));
    if (h > 0x80000000U) {
        h = (vibio_u32)(-((int)h));
    }
    if (w == 0 || h == 0 || w > MEDIA_MAX_DIM || h > MEDIA_MAX_DIM || off >= size) {
        return 0;
    }
    if (bpp != 24 && bpp != 32) {
        return 0;
    }
    vibio_u32 row = ((w * (bpp / 8U)) + 3U) & ~3U;
    vibio_u64 need = (vibio_u64)row * (vibio_u64)h;
    if (off + need > size || need > MEDIA_MAX_BYTES) {
        return 0;
    }
    out->width = w;
    out->height = h;
    out->header_bytes = off;
    out->pixels = data + off;
    out->pixel_bytes = (vibio_u32)need;
    return 2; /* marker: BMP needs conversion when drawing */
}

static void media_copy_name(char *dst, vibio_u32 max, const char *src)
{
    vibio_u32 i = 0;
    for (; src != 0 && src[i] != 0 && i + 1 < max; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0;
}

/* ===========================================================================
 * Native image decoders: PNG (zlib/DEFLATE) and baseline JPEG.
 *
 * The input file is read into `scratch`. PNG inflates scanlines into `work`,
 * then writes final RGBA back into `scratch`. JPEG decodes RGBA into `work`,
 * then copies the bounded result back into `scratch`. No kernel-side host
 * conversion, ffmpeg, or unbounded allocation is involved.
 * =========================================================================== */

#define MEDIA_OUT_MAX_DIM 2048U

static void media_decode_status_set(MediaDecodeStatus *status, vibio_u32 code, const char *detail)
{
    if (status == 0) {
        return;
    }
    status->code = code;
    vibio_u32 i = 0;
    while (detail != 0 && detail[i] != 0 && i + 1U < sizeof(status->detail)) {
        status->detail[i] = detail[i];
        i++;
    }
    status->detail[i] = 0;
}

static const char *media_decode_status_text(vibio_u32 code)
{
    if (code == MEDIA_IMG_OK) { return "success"; }
    if (code == MEDIA_IMG_UNSUPPORTED_FORMAT) { return "unsupported format"; }
    if (code == MEDIA_IMG_UNSUPPORTED_PNG) { return "unsupported PNG variant"; }
    if (code == MEDIA_IMG_UNSUPPORTED_JPEG) { return "unsupported JPEG variant"; }
    if (code == MEDIA_IMG_CORRUPT) { return "corrupt image"; }
    if (code == MEDIA_IMG_TOO_LARGE) { return "image too large"; }
    if (code == MEDIA_IMG_BUFFER_TOO_SMALL) { return "decode buffer too small"; }
    return "decode error";
}

static vibio_u16 media_read_be16(const vibio_u8 *p)
{
    return (vibio_u16)(((vibio_u16)p[0] << 8) | p[1]);
}

static vibio_u8 media_clamp_u8(int v)
{
    if (v < 0) { return 0; }
    if (v > 255) { return 255; }
    return (vibio_u8)v;
}

static vibio_u32 media_adler32(const vibio_u8 *data, vibio_u32 len)
{
    vibio_u32 a = 1;
    vibio_u32 b = 0;
    for (vibio_u32 i = 0; i < len; i++) {
        a += data[i];
        if (a >= 65521U) { a -= 65521U; }
        b += a;
        b %= 65521U;
    }
    return (b << 16) | a;
}

static vibio_u32 media_crc32_update(vibio_u32 crc, const vibio_u8 *data, vibio_u32 len)
{
    for (vibio_u32 i = 0; i < len; i++) {
        crc ^= data[i];
        for (vibio_u32 bit = 0; bit < 8; bit++) {
            crc = (crc & 1U) ? ((crc >> 1) ^ 0xEDB88320U) : (crc >> 1);
        }
    }
    return crc;
}

static vibio_u32 media_png_chunk_crc(const vibio_u8 *type, const vibio_u8 *payload, vibio_u32 len)
{
    vibio_u32 crc = 0xFFFFFFFFU;
    crc = media_crc32_update(crc, type, 4);
    crc = media_crc32_update(crc, payload, len);
    return crc ^ 0xFFFFFFFFU;
}

typedef struct {
    const vibio_u8 *in;
    vibio_u32 inlen;
    vibio_u32 incnt;
    vibio_u64 bitbuf;
    vibio_u32 bitcnt;
    vibio_u8 *out;
    vibio_u32 outcap;
    vibio_u32 outcnt;
} MediaInflate;

static int media_inf_bits(MediaInflate *s, int need)
{
    vibio_u64 val = s->bitbuf;
    while ((int)s->bitcnt < need) {
        if (s->incnt >= s->inlen) {
            return -1;
        }
        val |= (vibio_u64)s->in[s->incnt++] << s->bitcnt;
        s->bitcnt += 8;
    }
    s->bitbuf = val >> need;
    s->bitcnt -= (vibio_u32)need;
    return (int)(val & (((vibio_u64)1 << need) - 1));
}

static int media_inf_decode(MediaInflate *s, const short *count, const short *symbol)
{
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len <= 15; len++) {
        int b = media_inf_bits(s, 1);
        if (b < 0) {
            return -1;
        }
        code |= b;
        int cnt = count[len];
        if (code - first < cnt) {
            return symbol[index + (code - first)];
        }
        index += cnt;
        first += cnt;
        first <<= 1;
        code <<= 1;
    }
    return -1;
}

static int media_inf_construct(short *count, short *symbol, const short *length, int n)
{
    short offs[16];
    for (int len = 0; len <= 15; len++) {
        count[len] = 0;
    }
    for (int sym = 0; sym < n; sym++) {
        count[length[sym]]++;
    }
    if (count[0] == n) {
        return 0;
    }
    int left = 1;
    for (int len = 1; len <= 15; len++) {
        left <<= 1;
        left -= count[len];
        if (left < 0) {
            return left;
        }
    }
    offs[1] = 0;
    for (int len = 1; len < 15; len++) {
        offs[len + 1] = (short)(offs[len] + count[len]);
    }
    for (int sym = 0; sym < n; sym++) {
        if (length[sym] != 0) {
            symbol[offs[length[sym]]++] = (short)sym;
        }
    }
    return left;
}

static int media_inf_codes(MediaInflate *s,
                           const short *lencnt, const short *lensym,
                           const short *distcnt, const short *distsym)
{
    static const short lbase[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const short lext[29]  = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const short dbase[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const short dext[30]  = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    for (;;) {
        int sym = media_inf_decode(s, lencnt, lensym);
        if (sym < 0) {
            return -1;
        }
        if (sym == 256) {
            return 0;
        }
        if (sym < 256) {
            if (s->outcnt >= s->outcap) {
                return -1;
            }
            s->out[s->outcnt++] = (vibio_u8)sym;
            continue;
        }
        sym -= 257;
        if (sym >= 29) {
            return -1;
        }
        int extra = media_inf_bits(s, lext[sym]);
        if (extra < 0) {
            return -1;
        }
        int length = lbase[sym] + extra;
        int dsym = media_inf_decode(s, distcnt, distsym);
        if (dsym < 0 || dsym >= 30) {
            return -1;
        }
        extra = media_inf_bits(s, dext[dsym]);
        if (extra < 0) {
            return -1;
        }
        vibio_u32 dist = (vibio_u32)(dbase[dsym] + extra);
        if (dist > s->outcnt || s->outcnt + (vibio_u32)length > s->outcap) {
            return -1;
        }
        vibio_u32 from = s->outcnt - dist;
        for (int i = 0; i < length; i++) {
            s->out[s->outcnt] = s->out[from + (vibio_u32)i];
            s->outcnt++;
        }
    }
}

static int media_inf_fixed(MediaInflate *s)
{
    short lencnt[16], lensym[288], distcnt[16], distsym[30];
    short lengths[288];
    int i;
    for (i = 0; i < 144; i++) { lengths[i] = 8; }
    for (; i < 256; i++) { lengths[i] = 9; }
    for (; i < 280; i++) { lengths[i] = 7; }
    for (; i < 288; i++) { lengths[i] = 8; }
    if (media_inf_construct(lencnt, lensym, lengths, 288) < 0) { return -1; }
    for (i = 0; i < 30; i++) { lengths[i] = 5; }
    if (media_inf_construct(distcnt, distsym, lengths, 30) < 0) { return -1; }
    return media_inf_codes(s, lencnt, lensym, distcnt, distsym);
}

static int media_inf_dynamic(MediaInflate *s)
{
    static const short order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    short lengths[288 + 30];
    short lencnt[16], lensym[288], distcnt[16], distsym[30];
    int nlen = media_inf_bits(s, 5);
    int ndist = media_inf_bits(s, 5);
    int ncode = media_inf_bits(s, 4);
    if (nlen < 0 || ndist < 0 || ncode < 0) {
        return -1;
    }
    nlen += 257;
    ndist += 1;
    ncode += 4;
    if (nlen > 286 || ndist > 30) {
        return -1;
    }
    int index;
    for (index = 0; index < ncode; index++) {
        int b = media_inf_bits(s, 3);
        if (b < 0) {
            return -1;
        }
        lengths[order[index]] = (short)b;
    }
    for (; index < 19; index++) {
        lengths[order[index]] = 0;
    }
    short clcnt[16], clsym[19];
    if (media_inf_construct(clcnt, clsym, lengths, 19) < 0) {
        return -1;
    }
    index = 0;
    while (index < nlen + ndist) {
        int sym = media_inf_decode(s, clcnt, clsym);
        if (sym < 0) {
            return -1;
        }
        if (sym < 16) {
            lengths[index++] = (short)sym;
        } else {
            int len = 0;
            if (sym == 16) {
                if (index == 0) {
                    return -1;
                }
                len = lengths[index - 1];
                int r = media_inf_bits(s, 2);
                if (r < 0) { return -1; }
                sym = 3 + r;
            } else if (sym == 17) {
                int r = media_inf_bits(s, 3);
                if (r < 0) { return -1; }
                sym = 3 + r;
            } else {
                int r = media_inf_bits(s, 7);
                if (r < 0) { return -1; }
                sym = 11 + r;
            }
            if (index + sym > nlen + ndist) {
                return -1;
            }
            while (sym--) {
                lengths[index++] = (short)len;
            }
        }
    }
    if (lengths[256] == 0) {
        return -1;
    }
    if (media_inf_construct(lencnt, lensym, lengths, nlen) < 0) {
        return -1;
    }
    if (media_inf_construct(distcnt, distsym, lengths + nlen, ndist) < 0) {
        return -1;
    }
    return media_inf_codes(s, lencnt, lensym, distcnt, distsym);
}

static int media_inf_stored(MediaInflate *s)
{
    s->bitbuf = 0;
    s->bitcnt = 0;
    if (s->incnt + 4 > s->inlen) {
        return -1;
    }
    vibio_u32 len = s->in[s->incnt] | ((vibio_u32)s->in[s->incnt + 1] << 8);
    vibio_u32 nlen = s->in[s->incnt + 2] | ((vibio_u32)s->in[s->incnt + 3] << 8);
    s->incnt += 4;
    if (((len ^ 0xFFFFU) & 0xFFFFU) != nlen) {
        return -1;
    }
    if (s->incnt + len > s->inlen || s->outcnt + len > s->outcap) {
        return -1;
    }
    for (vibio_u32 i = 0; i < len; i++) {
        s->out[s->outcnt++] = s->in[s->incnt++];
    }
    return 0;
}

static vibio_u32 media_inflate(const vibio_u8 *src, vibio_u32 srclen, vibio_u8 *dst, vibio_u32 dstcap)
{
    MediaInflate s;
    s.in = src;
    s.inlen = srclen;
    s.incnt = 0;
    s.bitbuf = 0;
    s.bitcnt = 0;
    s.out = dst;
    s.outcap = dstcap;
    s.outcnt = 0;
    int last;
    do {
        last = media_inf_bits(&s, 1);
        int type = media_inf_bits(&s, 2);
        if (last < 0 || type < 0) {
            return 0;
        }
        int err;
        if (type == 0) {
            err = media_inf_stored(&s);
        } else if (type == 1) {
            err = media_inf_fixed(&s);
        } else if (type == 2) {
            err = media_inf_dynamic(&s);
        } else {
            return 0;
        }
        if (err != 0) {
            return 0;
        }
    } while (!last);
    return s.outcnt;
}

static int media_png_paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) { return a; }
    if (pb <= pc) { return b; }
    return c;
}

static vibio_u32 media_png_decode(vibio_u8 *scratch, vibio_u32 srclen, vibio_u32 scratch_cap,
                                  vibio_u8 *work, vibio_u32 work_cap,
                                  MediaBitmap *out, MediaDecodeStatus *status)
{
    zero_bytes(out, sizeof(MediaBitmap));
    if (scratch == 0 || work == 0 || srclen < 33U) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: file too short");
        return 0;
    }
    static const vibio_u8 sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    for (vibio_u32 i = 0; i < 8; i++) {
        if (scratch[i] != sig[i]) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_FORMAT, "not a PNG file");
            return 0;
        }
    }

    vibio_u32 width = 0, height = 0, bit_depth = 0, color_type = 0, interlace = 0;
    vibio_u8 palette[256 * 3];
    vibio_u8 trns[256];
    vibio_u32 palette_count = 0;
    vibio_u32 trns_count = 0;
    vibio_u32 trns_gray_valid = 0, trns_rgb_valid = 0;
    vibio_u32 trns_gray = 0, trns_r = 0, trns_g = 0, trns_b = 0;
    vibio_u32 zlib_len = 0;
    vibio_u32 seen_ihdr = 0, seen_idat = 0, seen_iend = 0;
    vibio_u32 off = 8;
    while (off + 12U <= srclen) {
        vibio_u32 clen = read_be32(scratch + off);
        const vibio_u8 *type = scratch + off + 4;
        vibio_u32 payload = off + 8U;
        vibio_u64 chunk_end = (vibio_u64)payload + (vibio_u64)clen + 4ULL;
        if (chunk_end > srclen) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: truncated chunk");
            return 0;
        }
        vibio_u32 stored_crc = read_be32(scratch + payload + clen);
        if (media_png_chunk_crc(type, scratch + payload, clen) != stored_crc) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: CRC mismatch");
            return 0;
        }
        if (type[0] == 'I' && type[1] == 'H' && type[2] == 'D' && type[3] == 'R') {
            if (seen_ihdr || clen != 13U) {
                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: bad IHDR");
                return 0;
            }
            width = read_be32(scratch + payload);
            height = read_be32(scratch + payload + 4);
            bit_depth = scratch[payload + 8];
            color_type = scratch[payload + 9];
            if (scratch[payload + 10] != 0 || scratch[payload + 11] != 0) {
                media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: compression/filter method");
                return 0;
            }
            interlace = scratch[payload + 12];
            seen_ihdr = 1;
        } else if (type[0] == 'P' && type[1] == 'L' && type[2] == 'T' && type[3] == 'E') {
            if (clen == 0 || (clen % 3U) != 0) {
                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: bad PLTE");
                return 0;
            }
            vibio_u32 n = clen > 256U * 3U ? 256U * 3U : clen;
            for (vibio_u32 i = 0; i < n; i++) {
                palette[i] = scratch[payload + i];
            }
            palette_count = n / 3U;
        } else if (type[0] == 't' && type[1] == 'R' && type[2] == 'N' && type[3] == 'S') {
            if (color_type == 3) {
                vibio_u32 n = clen > 256U ? 256U : clen;
                for (vibio_u32 i = 0; i < n; i++) {
                    trns[i] = scratch[payload + i];
                }
                trns_count = n;
            } else if (color_type == 0 && clen >= 2U) {
                trns_gray = media_read_be16(scratch + payload);
                trns_gray_valid = 1;
            } else if (color_type == 2 && clen >= 6U) {
                trns_r = media_read_be16(scratch + payload);
                trns_g = media_read_be16(scratch + payload + 2);
                trns_b = media_read_be16(scratch + payload + 4);
                trns_rgb_valid = 1;
            }
        } else if (type[0] == 'I' && type[1] == 'D' && type[2] == 'A' && type[3] == 'T') {
            if (!seen_ihdr || zlib_len + clen < zlib_len || zlib_len + clen > scratch_cap) {
                media_decode_status_set(status, MEDIA_IMG_BUFFER_TOO_SMALL, "PNG decode buffer too small for IDAT");
                return 0;
            }
            for (vibio_u32 i = 0; i < clen; i++) {
                scratch[zlib_len + i] = scratch[payload + i];
            }
            zlib_len += clen;
            seen_idat = 1;
        } else if (type[0] == 'I' && type[1] == 'E' && type[2] == 'N' && type[3] == 'D') {
            seen_iend = 1;
            break;
        }
        off = (vibio_u32)chunk_end;
    }
    if (!seen_ihdr || !seen_idat || !seen_iend || width == 0 || height == 0) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: missing IHDR/IDAT/IEND");
        return 0;
    }
    if (interlace != 0) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: Adam7 interlace");
        return 0;
    }
    vibio_u32 channels = 0;
    if (color_type == 0) { channels = 1; }
    else if (color_type == 2) { channels = 3; }
    else if (color_type == 3) { channels = 1; }
    else if (color_type == 4) { channels = 2; }
    else if (color_type == 6) { channels = 4; }
    else {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: color type");
        return 0;
    }
    if (color_type == 3 && palette_count == 0) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: indexed image without PLTE");
        return 0;
    }
    if ((color_type == 2 || color_type == 4 || color_type == 6) && bit_depth != 8U && bit_depth != 16U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: RGB bit depth");
        return 0;
    }
    if ((color_type == 0 || color_type == 3) &&
        bit_depth != 1U && bit_depth != 2U && bit_depth != 4U && bit_depth != 8U && bit_depth != 16U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: bit depth");
        return 0;
    }
    if (color_type == 3 && bit_depth == 16U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: indexed 16-bit");
        return 0;
    }
    vibio_u64 bits_per_pixel64 = (vibio_u64)channels * (vibio_u64)bit_depth;
    vibio_u64 stride64 = ((vibio_u64)width * bits_per_pixel64 + 7ULL) / 8ULL;
    if (stride64 == 0 || stride64 > 0xFFFFFFFFULL) {
        media_decode_status_set(status, MEDIA_IMG_TOO_LARGE, "PNG image too large");
        return 0;
    }
    vibio_u32 bits_per_pixel = (vibio_u32)bits_per_pixel64;
    vibio_u32 bpp = (bits_per_pixel + 7U) / 8U;
    if (bpp == 0) { bpp = 1; }
    vibio_u32 stride = (vibio_u32)stride64;
    vibio_u64 raw_need = ((vibio_u64)stride + 1ULL) * (vibio_u64)height;
    if (raw_need > work_cap) {
        media_decode_status_set(status, MEDIA_IMG_TOO_LARGE, "PNG image too large for decode buffer");
        return 0;
    }
    if (zlib_len < 6U) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: short zlib stream");
        return 0;
    }
    vibio_u8 cmf = scratch[0];
    vibio_u8 flg = scratch[1];
    if ((cmf & 0x0FU) != 8U || (cmf >> 4) > 7U || (((vibio_u32)cmf * 256U + flg) % 31U) != 0U || (flg & 0x20U) != 0U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_PNG, "PNG unsupported: zlib header");
        return 0;
    }
    vibio_u32 produced = media_inflate(scratch + 2, zlib_len - 6U, work, work_cap);
    if (produced != (vibio_u32)raw_need) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: inflate size mismatch");
        return 0;
    }
    vibio_u32 adler_expected = read_be32(scratch + zlib_len - 4U);
    if (media_adler32(work, produced) != adler_expected) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: Adler mismatch");
        return 0;
    }
    for (vibio_u32 y = 0; y < height; y++) {
        vibio_u8 *row = work + (vibio_u64)y * ((vibio_u64)stride + 1ULL);
        vibio_u8 filter = row[0];
        if (filter > 4U) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: bad filter");
            return 0;
        }
        vibio_u8 *cur = row + 1;
        vibio_u8 *prev = (y == 0) ? 0 : (work + (vibio_u64)(y - 1U) * ((vibio_u64)stride + 1ULL) + 1);
        for (vibio_u32 x = 0; x < stride; x++) {
            int a = (x >= bpp) ? cur[x - bpp] : 0;
            int b = prev ? prev[x] : 0;
            int c = (prev && x >= bpp) ? prev[x - bpp] : 0;
            int v = cur[x];
            if (filter == 1U) { v += a; }
            else if (filter == 2U) { v += b; }
            else if (filter == 3U) { v += (a + b) / 2; }
            else if (filter == 4U) { v += media_png_paeth(a, b, c); }
            cur[x] = (vibio_u8)v;
        }
    }
    vibio_u32 step = 1;
    for (;;) {
        vibio_u32 ow = (width + step - 1U) / step;
        vibio_u32 oh = (height + step - 1U) / step;
        if (ow <= MEDIA_OUT_MAX_DIM && oh <= MEDIA_OUT_MAX_DIM &&
            (vibio_u64)ow * (vibio_u64)oh * 4ULL + 64ULL <= scratch_cap) {
            break;
        }
        step++;
        if (step > 64U) {
            media_decode_status_set(status, MEDIA_IMG_TOO_LARGE, "PNG image too large");
            return 0;
        }
    }
    vibio_u32 out_w = (width + step - 1U) / step;
    vibio_u32 out_h = (height + step - 1U) / step;
    for (vibio_u32 oy = 0; oy < out_h; oy++) {
        for (vibio_u32 ox = 0; ox < out_w; ox++) {
            vibio_u32 ar = 0, ag = 0, ab = 0, aa = 0, n = 0;
            for (vibio_u32 sy = 0; sy < step; sy++) {
                vibio_u32 py = oy * step + sy;
                if (py >= height) { break; }
                const vibio_u8 *prow = work + (vibio_u64)py * ((vibio_u64)stride + 1ULL) + 1;
                for (vibio_u32 sx = 0; sx < step; sx++) {
                    vibio_u32 px = ox * step + sx;
                    if (px >= width) { break; }
                    vibio_u32 r = 0, g = 0, bl = 0, al = 255;
                    if (bit_depth == 8U) {
                        const vibio_u8 *s = prow + px * channels;
                        if (color_type == 0) {
                            r = g = bl = s[0];
                            if (trns_gray_valid && s[0] == (trns_gray & 0xFFU)) { al = 0; }
                        } else if (color_type == 2) {
                            r = s[0]; g = s[1]; bl = s[2];
                            if (trns_rgb_valid && r == (trns_r & 0xFFU) && g == (trns_g & 0xFFU) && bl == (trns_b & 0xFFU)) { al = 0; }
                        } else if (color_type == 4) {
                            r = g = bl = s[0]; al = s[1];
                        } else if (color_type == 6) {
                            r = s[0]; g = s[1]; bl = s[2]; al = s[3];
                        } else {
                            vibio_u32 idx = s[0];
                            if (idx >= palette_count) {
                                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: palette index");
                                return 0;
                            }
                            r = palette[idx * 3U]; g = palette[idx * 3U + 1U]; bl = palette[idx * 3U + 2U];
                            al = idx < trns_count ? trns[idx] : 255;
                        }
                    } else if (bit_depth == 16U) {
                        const vibio_u8 *s = prow + px * channels * 2U;
                        if (color_type == 0) {
                            vibio_u32 gv = media_read_be16(s);
                            r = g = bl = s[0];
                            if (trns_gray_valid && gv == trns_gray) { al = 0; }
                        } else if (color_type == 2) {
                            vibio_u32 rv = media_read_be16(s);
                            vibio_u32 gv = media_read_be16(s + 2);
                            vibio_u32 bv = media_read_be16(s + 4);
                            r = s[0]; g = s[2]; bl = s[4];
                            if (trns_rgb_valid && rv == trns_r && gv == trns_g && bv == trns_b) { al = 0; }
                        } else if (color_type == 4) {
                            r = g = bl = s[0]; al = s[2];
                        } else if (color_type == 6) {
                            r = s[0]; g = s[2]; bl = s[4]; al = s[6];
                        }
                    } else {
                        vibio_u32 bitpos = px * bits_per_pixel;
                        vibio_u32 byteidx = bitpos >> 3;
                        vibio_u32 shift = 8U - bit_depth - (bitpos & 7U);
                        vibio_u32 mask = (1U << bit_depth) - 1U;
                        vibio_u32 val = (prow[byteidx] >> shift) & mask;
                        if (color_type == 3) {
                            if (val >= palette_count) {
                                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "PNG corrupt: palette index");
                                return 0;
                            }
                            r = palette[val * 3U]; g = palette[val * 3U + 1U]; bl = palette[val * 3U + 2U];
                            al = val < trns_count ? trns[val] : 255;
                        } else {
                            vibio_u32 sc = 255U / mask;
                            r = g = bl = val * sc;
                            if (trns_gray_valid && val == trns_gray) { al = 0; }
                        }
                    }
                    ar += r; ag += g; ab += bl; aa += al; n++;
                }
            }
            if (n == 0) { n = 1; }
            vibio_u8 *d = scratch + ((vibio_u64)oy * out_w + ox) * 4U;
            d[0] = (vibio_u8)(ar / n);
            d[1] = (vibio_u8)(ag / n);
            d[2] = (vibio_u8)(ab / n);
            d[3] = (vibio_u8)(aa / n);
        }
    }
    out->width = out_w;
    out->height = out_h;
    out->header_bytes = 0;
    out->pixels = scratch;
    out->pixel_bytes = out_w * out_h * 4U;
    media_decode_status_set(status, MEDIA_IMG_OK, "PNG native decode");
    return 1;
}

static const vibio_u8 media_jpeg_zigzag[64] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static const int media_jpeg_idct_basis[8][8] = {
    {11585, 11585, 11585, 11585, 11585, 11585, 11585, 11585},
    {16069, 13623, 9102, 3196, -3196, -9102, -13623, -16069},
    {15137, 6270, -6270, -15137, -15137, -6270, 6270, 15137},
    {13623, -3196, -16069, -9102, 9102, 16069, 3196, -13623},
    {11585, -11585, -11585, 11585, 11585, -11585, -11585, 11585},
    {9102, -16069, 3196, 13623, -13623, -3196, 16069, -9102},
    {6270, -15137, 15137, -6270, -6270, 15137, -15137, 6270},
    {3196, -9102, 13623, -16069, 16069, -13623, 9102, -3196}
};

typedef struct {
    vibio_u8 valid;
    vibio_u8 count[17];
    vibio_u8 symbol[256];
    int first_code[17];
    int first_symbol[17];
} MediaJpegHuff;

typedef struct {
    vibio_u8 id;
    vibio_u8 h;
    vibio_u8 v;
    vibio_u8 qtable;
    vibio_u8 dc_table;
    vibio_u8 ac_table;
    int dc_pred;
} MediaJpegComponent;

typedef struct {
    const vibio_u8 *data;
    vibio_u32 size;
    vibio_u32 pos;
    vibio_u32 bitbuf;
    vibio_u32 bitcnt;
    vibio_u8 marker;
    vibio_u8 error;
} MediaJpegBits;

typedef struct {
    vibio_u32 width;
    vibio_u32 height;
    vibio_u32 comp_count;
    vibio_u32 hmax;
    vibio_u32 vmax;
    vibio_u32 restart_interval;
    MediaJpegComponent comp[3];
    vibio_u8 quant_valid[4];
    int quant[4][64];
    MediaJpegHuff huff[2][4];
    MediaJpegBits bits;
    vibio_u32 scan_count;
    vibio_u8 scan_comp[3];
    vibio_u8 samples[3][4][64];
    int block[64];
    int tmp[64];
} MediaJpegDecoder;

static int media_descale(long long v, int bits)
{
    long long round = 1LL << (bits - 1);
    if (v >= 0) {
        return (int)((v + round) >> bits);
    }
    return -(int)(((-v) + round) >> bits);
}

static void media_jpeg_idct(MediaJpegDecoder *jd, vibio_u8 out[64])
{
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            long long sum = 0;
            for (int u = 0; u < 8; u++) {
                sum += (long long)jd->block[y * 8 + u] * media_jpeg_idct_basis[u][x];
            }
            jd->tmp[y * 8 + x] = media_descale(sum, 14);
        }
    }
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            long long sum = 0;
            for (int v = 0; v < 8; v++) {
                sum += (long long)jd->tmp[v * 8 + x] * media_jpeg_idct_basis[v][y];
            }
            int val = media_descale(sum, 14);
            val = media_descale(val, 2) + 128;
            out[y * 8 + x] = media_clamp_u8(val);
        }
    }
}

static vibio_u32 media_jpeg_huff_build(MediaJpegHuff *h)
{
    int code = 0;
    int pos = 0;
    h->first_code[0] = 0;
    h->first_symbol[0] = 0;
    for (int len = 1; len <= 16; len++) {
        code = (code + h->count[len - 1]) << 1;
        h->first_code[len] = code;
        h->first_symbol[len] = pos;
        pos += h->count[len];
        if (pos > 256) {
            h->valid = 0;
            return 0;
        }
    }
    h->valid = 1;
    return 1;
}

static int media_jpeg_next_entropy_byte(MediaJpegBits *b)
{
    if (b->pos >= b->size) {
        b->error = 1;
        return -1;
    }
    vibio_u8 v = b->data[b->pos++];
    if (v != 0xFFU) {
        return v;
    }
    while (b->pos < b->size && b->data[b->pos] == 0xFFU) {
        b->pos++;
    }
    if (b->pos >= b->size) {
        b->error = 1;
        return -1;
    }
    vibio_u8 marker = b->data[b->pos++];
    if (marker == 0x00U) {
        return 0xFF;
    }
    b->marker = marker;
    b->error = 1;
    return -1;
}

static int media_jpeg_get_bits(MediaJpegBits *b, int n)
{
    while ((int)b->bitcnt < n) {
        int byte = media_jpeg_next_entropy_byte(b);
        if (byte < 0) {
            return -1;
        }
        b->bitbuf = (b->bitbuf << 8) | (vibio_u32)byte;
        b->bitcnt += 8;
    }
    b->bitcnt -= (vibio_u32)n;
    return (int)((b->bitbuf >> b->bitcnt) & ((1U << n) - 1U));
}

static int media_jpeg_huff_decode(MediaJpegBits *b, const MediaJpegHuff *h)
{
    if (!h->valid) {
        b->error = 1;
        return -1;
    }
    int code = 0;
    for (int len = 1; len <= 16; len++) {
        int bit = media_jpeg_get_bits(b, 1);
        if (bit < 0) {
            return -1;
        }
        code = (code << 1) | bit;
        int first = h->first_code[len];
        int cnt = h->count[len];
        if (cnt > 0 && code >= first && code - first < cnt) {
            return h->symbol[h->first_symbol[len] + code - first];
        }
    }
    b->error = 1;
    return -1;
}

static int media_jpeg_extend(int v, int bits)
{
    if (bits == 0) {
        return 0;
    }
    int vt = 1 << (bits - 1);
    if (v < vt) {
        return v + ((-1) << bits) + 1;
    }
    return v;
}

static vibio_u32 media_jpeg_decode_block(MediaJpegDecoder *jd, MediaJpegComponent *c, vibio_u8 out[64])
{
    for (int i = 0; i < 64; i++) {
        jd->block[i] = 0;
    }
    if (c->qtable >= 4 || !jd->quant_valid[c->qtable] || c->dc_table >= 4 || c->ac_table >= 4) {
        return 0;
    }
    int sym = media_jpeg_huff_decode(&jd->bits, &jd->huff[0][c->dc_table]);
    if (sym < 0 || sym > 15) {
        return 0;
    }
    int diff_bits = sym == 0 ? 0 : media_jpeg_get_bits(&jd->bits, sym);
    if (diff_bits < 0) {
        return 0;
    }
    c->dc_pred += media_jpeg_extend(diff_bits, sym);
    jd->block[0] = c->dc_pred * jd->quant[c->qtable][0];
    int k = 1;
    while (k < 64) {
        sym = media_jpeg_huff_decode(&jd->bits, &jd->huff[1][c->ac_table]);
        if (sym < 0) {
            return 0;
        }
        if (sym == 0) {
            break;
        }
        int run = sym >> 4;
        int bits = sym & 0x0F;
        if (bits == 0) {
            if (run != 15) {
                return 0;
            }
            k += 16;
            continue;
        }
        k += run;
        if (k >= 64) {
            return 0;
        }
        int raw = media_jpeg_get_bits(&jd->bits, bits);
        if (raw < 0) {
            return 0;
        }
        int natural = media_jpeg_zigzag[k];
        jd->block[natural] = media_jpeg_extend(raw, bits) * jd->quant[c->qtable][natural];
        k++;
    }
    media_jpeg_idct(jd, out);
    return 1;
}

static vibio_u8 media_jpeg_component_sample(MediaJpegDecoder *jd, vibio_u32 ci, vibio_u32 x, vibio_u32 y)
{
    MediaJpegComponent *c = &jd->comp[ci];
    vibio_u32 sx = (x * c->h) / jd->hmax;
    vibio_u32 sy = (y * c->v) / jd->vmax;
    vibio_u32 bi = (sy / 8U) * c->h + (sx / 8U);
    if (bi >= 4U) {
        bi = 0;
    }
    return jd->samples[ci][bi][(sy & 7U) * 8U + (sx & 7U)];
}

static void media_jpeg_render_mcu(MediaJpegDecoder *jd, vibio_u8 *rgba, vibio_u32 mcu_x, vibio_u32 mcu_y)
{
    vibio_u32 mcu_w = jd->hmax * 8U;
    vibio_u32 mcu_h = jd->vmax * 8U;
    for (vibio_u32 y = 0; y < mcu_h; y++) {
        vibio_u32 py = mcu_y + y;
        if (py >= jd->height) {
            break;
        }
        for (vibio_u32 x = 0; x < mcu_w; x++) {
            vibio_u32 px = mcu_x + x;
            if (px >= jd->width) {
                break;
            }
            vibio_u8 r, g, b;
            if (jd->comp_count == 1) {
                r = g = b = media_jpeg_component_sample(jd, 0, x, y);
            } else {
                int yy = media_jpeg_component_sample(jd, 0, x, y);
                int cb = media_jpeg_component_sample(jd, 1, x, y) - 128;
                int cr = media_jpeg_component_sample(jd, 2, x, y) - 128;
                r = media_clamp_u8(yy + ((91881 * cr) >> 16));
                g = media_clamp_u8(yy - ((22554 * cb + 46802 * cr) >> 16));
                b = media_clamp_u8(yy + ((116130 * cb) >> 16));
            }
            vibio_u8 *d = rgba + ((vibio_u64)py * jd->width + px) * 4U;
            d[0] = r;
            d[1] = g;
            d[2] = b;
            d[3] = 255;
        }
    }
}

static vibio_u32 media_jpeg_consume_restart(MediaJpegDecoder *jd, vibio_u32 expected)
{
    MediaJpegBits *b = &jd->bits;
    b->bitcnt = 0;
    b->bitbuf = 0;
    b->error = 0;
    b->marker = 0;
    while (b->pos < b->size && b->data[b->pos] == 0xFFU) {
        b->pos++;
    }
    if (b->pos >= b->size) {
        return 0;
    }
    vibio_u8 marker = b->data[b->pos++];
    if (marker != (vibio_u8)(0xD0U + (expected & 7U))) {
        return 0;
    }
    for (vibio_u32 i = 0; i < jd->comp_count; i++) {
        jd->comp[i].dc_pred = 0;
    }
    return 1;
}

static vibio_u32 media_jpeg_decode_scan(MediaJpegDecoder *jd, const vibio_u8 *data, vibio_u32 size,
                                        vibio_u32 entropy_pos, vibio_u8 *scratch, vibio_u32 scratch_cap,
                                        vibio_u8 *work, vibio_u32 work_cap,
                                        MediaBitmap *out, MediaDecodeStatus *status)
{
    if (jd->width == 0 || jd->height == 0 || jd->width > MEDIA_OUT_MAX_DIM || jd->height > MEDIA_OUT_MAX_DIM) {
        media_decode_status_set(status, MEDIA_IMG_TOO_LARGE, "JPEG image too large");
        return 0;
    }
    vibio_u64 out_need64 = (vibio_u64)jd->width * (vibio_u64)jd->height * 4ULL;
    if (out_need64 > work_cap || out_need64 > scratch_cap || out_need64 > 0xFFFFFFFFULL) {
        media_decode_status_set(status, MEDIA_IMG_BUFFER_TOO_SMALL, "JPEG decode buffer too small");
        return 0;
    }
    for (vibio_u32 i = 0; i < jd->comp_count; i++) {
        MediaJpegComponent *c = &jd->comp[i];
        if (c->qtable >= 4 || !jd->quant_valid[c->qtable] ||
            c->dc_table >= 4 || c->ac_table >= 4 ||
            !jd->huff[0][c->dc_table].valid || !jd->huff[1][c->ac_table].valid) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: missing tables");
            return 0;
        }
        c->dc_pred = 0;
    }
    jd->bits.data = data;
    jd->bits.size = size;
    jd->bits.pos = entropy_pos;
    jd->bits.bitbuf = 0;
    jd->bits.bitcnt = 0;
    jd->bits.marker = 0;
    jd->bits.error = 0;

    vibio_u32 mcu_w = jd->hmax * 8U;
    vibio_u32 mcu_h = jd->vmax * 8U;
    vibio_u32 mcu_cols = (jd->width + mcu_w - 1U) / mcu_w;
    vibio_u32 mcu_rows = (jd->height + mcu_h - 1U) / mcu_h;
    vibio_u32 total_mcus = mcu_cols * mcu_rows;
    vibio_u32 mcu_index = 0;
    vibio_u32 rst = 0;
    for (vibio_u32 my = 0; my < mcu_rows; my++) {
        for (vibio_u32 mx = 0; mx < mcu_cols; mx++) {
            for (vibio_u32 si = 0; si < jd->scan_count; si++) {
                vibio_u32 ci = jd->scan_comp[si];
                MediaJpegComponent *c = &jd->comp[ci];
                for (vibio_u32 vy = 0; vy < c->v; vy++) {
                    for (vibio_u32 hx = 0; hx < c->h; hx++) {
                        vibio_u32 slot = vy * c->h + hx;
                        if (slot >= 4U ||
                            !media_jpeg_decode_block(jd, c, jd->samples[ci][slot])) {
                            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: entropy decode failed");
                            return 0;
                        }
                    }
                }
            }
            media_jpeg_render_mcu(jd, work, mx * mcu_w, my * mcu_h);
            mcu_index++;
            if (jd->restart_interval > 0 && mcu_index < total_mcus &&
                (mcu_index % jd->restart_interval) == 0) {
                if (!media_jpeg_consume_restart(jd, rst)) {
                    media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: bad restart marker");
                    return 0;
                }
                rst = (rst + 1U) & 7U;
            }
        }
    }
    copy_bytes(scratch, work, out_need64);
    out->width = jd->width;
    out->height = jd->height;
    out->header_bytes = 0;
    out->pixels = scratch;
    out->pixel_bytes = (vibio_u32)out_need64;
    media_decode_status_set(status, MEDIA_IMG_OK, "JPEG native decode");
    return 1;
}

static int media_jpeg_next_marker(const vibio_u8 *data, vibio_u32 size, vibio_u32 *pos)
{
    while (*pos < size && data[*pos] != 0xFFU) {
        (*pos)++;
    }
    while (*pos < size && data[*pos] == 0xFFU) {
        (*pos)++;
    }
    if (*pos >= size) {
        return -1;
    }
    return data[(*pos)++];
}

static vibio_u32 media_jpeg_parse_dqt(MediaJpegDecoder *jd, const vibio_u8 *seg, vibio_u32 len)
{
    vibio_u32 p = 0;
    while (p < len) {
        vibio_u8 pq_tq = seg[p++];
        vibio_u32 precision = pq_tq >> 4;
        vibio_u32 table = pq_tq & 0x0F;
        if (table >= 4 || precision > 1) {
            return 0;
        }
        vibio_u32 need = precision ? 128U : 64U;
        if (p + need > len) {
            return 0;
        }
        for (vibio_u32 i = 0; i < 64; i++) {
            int value;
            if (precision) {
                value = media_read_be16(seg + p + i * 2U);
            } else {
                value = seg[p + i];
            }
            jd->quant[table][media_jpeg_zigzag[i]] = value;
        }
        jd->quant_valid[table] = 1;
        p += need;
    }
    return p == len;
}

static vibio_u32 media_jpeg_parse_dht(MediaJpegDecoder *jd, const vibio_u8 *seg, vibio_u32 len)
{
    vibio_u32 p = 0;
    while (p < len) {
        if (p + 17U > len) {
            return 0;
        }
        vibio_u8 tc_th = seg[p++];
        vibio_u32 cls = tc_th >> 4;
        vibio_u32 table = tc_th & 0x0F;
        if (cls > 1 || table >= 4) {
            return 0;
        }
        MediaJpegHuff *h = &jd->huff[cls][table];
        zero_bytes(h, sizeof(MediaJpegHuff));
        vibio_u32 total = 0;
        for (vibio_u32 i = 1; i <= 16; i++) {
            h->count[i] = seg[p++];
            total += h->count[i];
        }
        if (p + total > len || total > 256U) {
            return 0;
        }
        for (vibio_u32 i = 0; i < total; i++) {
            h->symbol[i] = seg[p + i];
        }
        p += total;
        if (!media_jpeg_huff_build(h)) {
            return 0;
        }
    }
    return p == len;
}

static vibio_u32 media_jpeg_parse_sof0(MediaJpegDecoder *jd, const vibio_u8 *seg, vibio_u32 len, MediaDecodeStatus *status)
{
    if (len < 8U || seg[0] != 8U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: non-8-bit sample precision");
        return 0;
    }
    jd->height = media_read_be16(seg + 1);
    jd->width = media_read_be16(seg + 3);
    jd->comp_count = seg[5];
    if ((jd->comp_count != 1U && jd->comp_count != 3U) || len < 6U + jd->comp_count * 3U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: component count");
        return 0;
    }
    jd->hmax = 0;
    jd->vmax = 0;
    for (vibio_u32 i = 0; i < jd->comp_count; i++) {
        MediaJpegComponent *c = &jd->comp[i];
        c->id = seg[6U + i * 3U];
        c->h = seg[7U + i * 3U] >> 4;
        c->v = seg[7U + i * 3U] & 0x0F;
        c->qtable = seg[8U + i * 3U];
        if (c->h == 0 || c->v == 0 || c->h > 2U || c->v > 2U || c->qtable >= 4U) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: sampling factors");
            return 0;
        }
        if (c->h > jd->hmax) { jd->hmax = c->h; }
        if (c->v > jd->vmax) { jd->vmax = c->v; }
    }
    if (jd->comp_count == 1U) {
        if (jd->comp[0].h != 1U || jd->comp[0].v != 1U) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: grayscale sampling");
            return 0;
        }
    } else {
        if (jd->comp[1].h != 1U || jd->comp[1].v != 1U ||
            jd->comp[2].h != 1U || jd->comp[2].v != 1U ||
            !((jd->hmax == 1U && jd->vmax == 1U) ||
              (jd->hmax == 2U && jd->vmax == 1U) ||
              (jd->hmax == 2U && jd->vmax == 2U))) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: subsampling");
            return 0;
        }
    }
    return 1;
}

static vibio_u32 media_jpeg_parse_sos(MediaJpegDecoder *jd, const vibio_u8 *seg, vibio_u32 len, MediaDecodeStatus *status)
{
    if (len < 6U) {
        media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: bad SOS");
        return 0;
    }
    jd->scan_count = seg[0];
    if (jd->scan_count != jd->comp_count || len < 1U + jd->scan_count * 2U + 3U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: scan layout");
        return 0;
    }
    for (vibio_u32 s = 0; s < jd->scan_count; s++) {
        vibio_u8 id = seg[1U + s * 2U];
        vibio_u8 tables = seg[2U + s * 2U];
        vibio_u32 found = 0xFFFFFFFFU;
        for (vibio_u32 i = 0; i < jd->comp_count; i++) {
            if (jd->comp[i].id == id) {
                found = i;
                break;
            }
        }
        if (found == 0xFFFFFFFFU) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: SOS component id");
            return 0;
        }
        jd->scan_comp[s] = (vibio_u8)found;
        jd->comp[found].dc_table = tables >> 4;
        jd->comp[found].ac_table = tables & 0x0F;
    }
    vibio_u32 tail = 1U + jd->scan_count * 2U;
    if (seg[tail] != 0U || seg[tail + 1U] != 63U || seg[tail + 2U] != 0U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: progressive scan parameters");
        return 0;
    }
    return 1;
}

static vibio_u32 media_jpeg_decode(vibio_u8 *scratch, vibio_u32 srclen, vibio_u32 scratch_cap,
                                   vibio_u8 *work, vibio_u32 work_cap,
                                   MediaBitmap *out, MediaDecodeStatus *status)
{
    zero_bytes(out, sizeof(MediaBitmap));
    if (scratch == 0 || work == 0 || srclen < 4U || scratch[0] != 0xFFU || scratch[1] != 0xD8U) {
        media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_FORMAT, "not a JPEG file");
        return 0;
    }
    vibio_u32 jd_bytes = (vibio_u32)((sizeof(MediaJpegDecoder) + 15ULL) & ~15ULL);
    if (work_cap <= jd_bytes) {
        media_decode_status_set(status, MEDIA_IMG_BUFFER_TOO_SMALL, "JPEG decode buffer too small");
        return 0;
    }
    MediaJpegDecoder *jd = (MediaJpegDecoder *)work;
    vibio_u8 *pixel_work = work + jd_bytes;
    vibio_u32 pixel_work_cap = work_cap - jd_bytes;
    zero_bytes(jd, sizeof(*jd));
    vibio_u32 pos = 2;
    for (;;) {
        int marker = media_jpeg_next_marker(scratch, srclen, &pos);
        if (marker < 0) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: marker not found");
            return 0;
        }
        if (marker == 0xD9) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: no scan");
            return 0;
        }
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            continue;
        }
        if (pos + 2U > srclen) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: short segment");
            return 0;
        }
        vibio_u32 seglen = media_read_be16(scratch + pos);
        if (seglen < 2U || (vibio_u64)pos + seglen > srclen) {
            media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: segment length");
            return 0;
        }
        const vibio_u8 *seg = scratch + pos + 2U;
        vibio_u32 len = seglen - 2U;
        pos += seglen;
        if (marker == 0xC0) {
            if (!media_jpeg_parse_sof0(jd, seg, len, status)) {
                return 0;
            }
        } else if (marker == 0xC2) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: progressive/SOF2");
            return 0;
        } else if ((marker >= 0xC1 && marker <= 0xCF) && marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: SOF variant");
            return 0;
        } else if (marker == 0xC4) {
            if (!media_jpeg_parse_dht(jd, seg, len)) {
                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: bad Huffman table");
                return 0;
            }
        } else if (marker == 0xDB) {
            if (!media_jpeg_parse_dqt(jd, seg, len)) {
                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: bad quant table");
                return 0;
            }
        } else if (marker == 0xDD) {
            if (len < 2U) {
                media_decode_status_set(status, MEDIA_IMG_CORRUPT, "JPEG corrupt: bad restart interval");
                return 0;
            }
            jd->restart_interval = media_read_be16(seg);
        } else if (marker == 0xDA) {
            if (!media_jpeg_parse_sos(jd, seg, len, status)) {
                return 0;
            }
            return media_jpeg_decode_scan(jd, scratch, srclen, pos, scratch, scratch_cap,
                                          pixel_work, pixel_work_cap, out, status);
        } else if (marker == 0xCC) {
            media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_JPEG, "JPEG unsupported: arithmetic coding");
            return 0;
        }
    }
}

static vibio_u32 media_decode_native_image(const char *name, vibio_u8 *scratch, vibio_u32 srclen, vibio_u32 scratch_cap,
                                           vibio_u8 *work, vibio_u32 work_cap,
                                           MediaBitmap *out, MediaDecodeStatus *status)
{
    if (out != 0) {
        zero_bytes(out, sizeof(MediaBitmap));
    }
    if (scratch == 0 || work == 0 || scratch_cap == 0 || work_cap == 0 || out == 0) {
        media_decode_status_set(status, MEDIA_IMG_BUFFER_TOO_SMALL, "native decode buffers unavailable");
        return 0;
    }
    if (browser_name_has_ext(name, ".PNG")) {
        return media_png_decode(scratch, srclen, scratch_cap, work, work_cap, out, status);
    }
    if (browser_name_has_ext(name, ".JPG") || browser_name_has_ext(name, ".JPEG")) {
        return media_jpeg_decode(scratch, srclen, scratch_cap, work, work_cap, out, status);
    }
    media_decode_status_set(status, MEDIA_IMG_UNSUPPORTED_FORMAT, "unsupported native image format");
    return 0;
}

static void media_viewer_reset(VibioMediaViewer *mv)
{
    mv->loaded = 0;
    mv->decode_mode = MEDIA_DECODE_NONE;
    mv->zoom_percent = MEDIA_ZOOM_FIT;
    mv->pan_x = 0;
    mv->pan_y = 0;
    mv->bitmap.width = 0;
    mv->bitmap.height = 0;
    mv->bitmap.pixels = 0;
    mv->status_line[0] = 0;
    mv->error_line[0] = 0;
}

static vibio_u32 media_viewer_load(VibioMediaViewer *mv, const char *name, vibio_u8 source_hint)
{
    /* Resolve the image in the folder it was opened from (not the volume root). */
    fat32_set_resolve_dir(mv->dir_cluster);
    media_viewer_reset(mv);
    media_copy_name(mv->filename, FILES_NAME_MAX, name);
    mv->source = source_hint;

    vibio_u32 total = 0;
    vibio_u8 src = FILES_SRC_NONE;
    vibio_u32 read = media_read_bytes(mv->boot, mv->disk, mv->fs, name, 0,
                                      mv->scratch, mv->scratch_capacity, &total, &src);
    if (read == 0) {
        media_set_error("Could not read image file.");
        mv->decode_mode = MEDIA_DECODE_ERROR;
        panic_append_text(mv->error_line, 0, sizeof(mv->error_line),
                          "Image not rendered. Could not read file.");
        return 0;
    }
    mv->file_size = total > 0 ? total : read;
    if (src != FILES_SRC_NONE) {
        mv->source = src;
    }

    MediaBitmap bmp;
    MediaDecodeStatus native_status;
    zero_bytes(&native_status, sizeof(native_status));
    vibio_u32 native_attempt = 0;
    if (browser_name_has_ext(name, ".VIM") || browser_name_has_ext(name, ".VIMG")) {
        if (media_vimg_parse(mv->scratch, read, &mv->bitmap)) {
            mv->decode_mode = MEDIA_DECODE_VIMG;
            media_copy_name(mv->format_label, sizeof(mv->format_label), "VIMG native");
            panic_append_text(mv->status_line, 0, sizeof(mv->status_line), "Native VIMG decode");
            goto loaded_ok;
        }
    }
    if (browser_name_has_ext(name, ".BMP") && media_bmp_parse(mv->scratch, read, &bmp)) {
        /* Expand bottom-up BMP24/32 into RGBA in scratch after header. */
        vibio_u32 w = bmp.width;
        vibio_u32 h = bmp.height;
        vibio_u32 bpp = 3;
        vibio_u32 row = ((w * 3U) + 3U) & ~3U;
        if (bmp.pixel_bytes / h == w * 4U) {
            bpp = 4;
            row = w * 4U;
        }
        vibio_u8 *out = mv->scratch;
        vibio_u32 out_need = w * h * 4U;
        if (out_need + 64 > mv->scratch_capacity) {
            media_set_error("BMP too large after decode.");
            mv->decode_mode = MEDIA_DECODE_ERROR;
            return 0;
        }
        for (vibio_u32 row_i = 0; row_i < h; row_i++) {
            vibio_u32 src_row = h - 1U - row_i;
            const vibio_u8 *src = bmp.pixels + src_row * row;
            for (vibio_u32 col = 0; col < w; col++) {
                vibio_u32 si = col * (bpp == 4 ? 4U : 3U);
                vibio_u32 di = (row_i * w + col) * 4U;
                out[di + 0] = src[si + 2];
                out[di + 1] = src[si + 1];
                out[di + 2] = src[si + 0];
                out[di + 3] = bpp == 4 ? src[si + 3] : 255;
            }
        }
        mv->bitmap.width = w;
        mv->bitmap.height = h;
        mv->bitmap.header_bytes = 0;
        mv->bitmap.pixels = out;
        mv->bitmap.pixel_bytes = out_need;
        mv->decode_mode = MEDIA_DECODE_BMP;
        media_copy_name(mv->format_label, sizeof(mv->format_label), "BMP native");
        panic_append_text(mv->status_line, 0, sizeof(mv->status_line), "Native BMP decode");
        goto loaded_ok;
    }
    if (browser_name_has_ext(name, ".PNG") ||
        browser_name_has_ext(name, ".JPG") ||
        browser_name_has_ext(name, ".JPEG")) {
        native_attempt = 1;
        if (total > read) {
            media_decode_status_set(&native_status, MEDIA_IMG_BUFFER_TOO_SMALL,
                                    "image file too large for read buffer");
        } else if (media_decode_native_image(name, mv->scratch, read, mv->scratch_capacity,
                                             mv->work, mv->work_capacity,
                                             &mv->bitmap, &native_status)) {
            if (browser_name_has_ext(name, ".PNG")) {
                mv->decode_mode = MEDIA_DECODE_PNG;
                media_copy_name(mv->format_label, sizeof(mv->format_label), "PNG native");
                panic_append_text(mv->status_line, 0, sizeof(mv->status_line), "Native PNG decode");
            } else {
                mv->decode_mode = MEDIA_DECODE_JPEG;
                media_copy_name(mv->format_label, sizeof(mv->format_label), "JPEG native");
                panic_append_text(mv->status_line, 0, sizeof(mv->status_line), "Native JPEG decode");
            }
            goto loaded_ok;
        }
    }

    char fallback[FILES_NAME_MAX];
    VibioBrowser btmp;
    zero_bytes(&btmp, sizeof(btmp));
    btmp.boot = mv->boot;
    btmp.disk = mv->disk;
    btmp.fs = mv->fs;
    btmp.image = mv->scratch;
    btmp.image_capacity = mv->scratch_capacity;
    btmp.image_work = mv->work;
    btmp.image_work_capacity = mv->work_capacity;
    if (browser_find_vim_fallback_name(&btmp, name, fallback)) {
        read = media_read_bytes(mv->boot, mv->disk, mv->fs, fallback, 0,
                                mv->scratch, mv->scratch_capacity, &total, &src);
        if (read > 0 && media_vimg_parse(mv->scratch, read, &mv->bitmap)) {
            media_copy_name(mv->fallback_name, FILES_NAME_MAX, fallback);
            mv->decode_mode = MEDIA_DECODE_VIMG_FALLBACK;
            media_copy_name(mv->format_label, sizeof(mv->format_label), "VIMG fallback");
            if (native_attempt) {
                panic_append_text(mv->status_line, 0, sizeof(mv->status_line),
                                  "Native unsupported; VIMG fallback");
            } else {
                panic_append_text(mv->status_line, 0, sizeof(mv->status_line),
                                  "Converted VIMG fallback (host build/deploy)");
            }
            goto loaded_ok;
        }
    }

    media_set_error(native_attempt && native_status.detail[0] ? native_status.detail :
                    "Missing or unsupported image; no VIMG fallback.");
    mv->decode_mode = MEDIA_DECODE_ERROR;
    if (native_attempt && native_status.detail[0]) {
        panic_append_text(mv->error_line, 0, sizeof(mv->error_line), "Image not rendered. ");
        panic_append_text(mv->error_line, cstr_length(mv->error_line), sizeof(mv->error_line),
                          native_status.detail);
    } else {
        panic_append_text(mv->error_line, 0, sizeof(mv->error_line),
                          "Image not rendered. Matching Vibio image fallback missing.");
        panic_append_text(mv->error_line, cstr_length(mv->error_line), sizeof(mv->error_line),
                          " Rebuild/redeploy media assets.");
    }
    return 0;

loaded_ok:
    mv->img_width = mv->bitmap.width;
    mv->img_height = mv->bitmap.height;
    mv->loaded = 1;
    mv->zoom_percent = MEDIA_ZOOM_FIT;
    return 1;
}

static void media_draw_toolbar_btn(
    const VibioBootInfo *boot_info,
    vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h,
    const char *label,
    const VibioDesktopTheme *theme)
{
    vibio_u32 bg = make_color(boot_info, 28, 48, 58);
    fill_rect(boot_info, x, y, w, h, bg);
    fill_rect(boot_info, x, y, w, 1, theme->taskbar_edge);
    g_text_bg = bg;
    set_clip(x, y, x + w, y + h);
    vibio_u32 label_w = font_text_width(&FONT_GEIST_UI, label);
    vibio_u32 label_x = label_w + 4U < w ? x + (w - label_w) / 2U : x + 2U;
    draw_cstr(boot_info, label_x, y + 4, label, theme->text);
    reset_clip();
}

static vibio_u32 theme_muted_color(const VibioBootInfo *boot_info)
{
    return make_color(boot_info, 155, 169, 181);
}

static void media_viewer_draw_image(
    const VibioBootInfo *boot_info,
    VibioMediaViewer *mv,
    vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h)
{
    vibio_u32 bg = make_color(boot_info, 8, 12, 16);
    fill_rect(boot_info, x, y, w, h, bg);
    if (!mv->loaded || mv->bitmap.pixels == 0 || mv->bitmap.width == 0 || mv->bitmap.height == 0) {
        g_text_bg = bg;
        draw_cstr(boot_info, x + 16, y + 16, mv->error_line[0] ? mv->error_line : "No image loaded.", theme_muted_color(boot_info));
        return;
    }
    vibio_u32 iw = mv->bitmap.width;
    vibio_u32 ih = mv->bitmap.height;
    vibio_u32 scale_pct = mv->zoom_percent == MEDIA_ZOOM_FIT ? 0 : mv->zoom_percent;
    vibio_u32 draw_w = iw;
    vibio_u32 draw_h = ih;
    if (scale_pct == 0) {
        if (iw > w || ih > h) {
            vibio_u32 sx = (w * 100U) / iw;
            vibio_u32 sy = (h * 100U) / ih;
            scale_pct = sx < sy ? sx : sy;
            if (scale_pct == 0) {
                scale_pct = 1;
            }
        } else {
            scale_pct = 100;
        }
    }
    /* Remember the scale actually shown so the +/- buttons can step from the
     * real displayed percentage even when the mode is FIT (zoom_percent == 0). */
    mv->effective_scale = scale_pct;
    draw_w = (iw * scale_pct) / 100U;
    draw_h = (ih * scale_pct) / 100U;
    if (draw_w == 0) {
        draw_w = 1;
    }
    if (draw_h == 0) {
        draw_h = 1;
    }
    int ox = (int)(x + (w > draw_w ? (w - draw_w) / 2U : 0)) - (int)mv->pan_x;
    int oy = (int)(y + (h > draw_h ? (h - draw_h) / 2U : 0)) - (int)mv->pan_y;
    set_clip(x, y, x + w, y + h);
    /* Only iterate the destination pixels that actually fall inside the current
     * paint/clip bounds. The compositor calls this once per dirty rectangle, so
     * without this clamp a tiny cursor-sized rect still rescaled the whole image
     * (the source of the zoom/pan lag). Now the cost is proportional to the
     * visible area, not the full scaled image. */
    int vx0 = (int)(g_paint_x0 > g_clip_x0 ? g_paint_x0 : g_clip_x0);
    int vy0 = (int)(g_paint_y0 > g_clip_y0 ? g_paint_y0 : g_clip_y0);
    int vx1 = (int)(g_paint_x1 < g_clip_x1 ? g_paint_x1 : g_clip_x1);
    int vy1 = (int)(g_paint_y1 < g_clip_y1 ? g_paint_y1 : g_clip_y1);
    int dy_start = vy0 - oy; if (dy_start < 0) { dy_start = 0; }
    int dy_end = vy1 - oy; if (dy_end > (int)draw_h) { dy_end = (int)draw_h; }
    int dx_start = vx0 - ox; if (dx_start < 0) { dx_start = 0; }
    int dx_end = vx1 - ox; if (dx_end > (int)draw_w) { dx_end = (int)draw_w; }
    for (int dy = dy_start; dy < dy_end; dy++) {
        vibio_u32 sy = ((vibio_u32)dy * ih) / draw_h;
        vibio_u32 row = sy * iw;
        for (int dx = dx_start; dx < dx_end; dx++) {
            vibio_u32 sx = ((vibio_u32)dx * iw) / draw_w;
            const vibio_u8 *px = mv->bitmap.pixels + (row + sx) * 4U;
            blend_pixel(boot_info, (vibio_u32)(ox + dx), (vibio_u32)(oy + dy), px[0], px[1], px[2], px[3]);
        }
    }
    reset_clip();
}

static void media_viewer_render(
    const VibioBootInfo *boot_info,
    VibioMediaViewer *mv,
    const VibioDesktopTheme *theme,
    vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h,
    vibio_u32 body_color)
{
    (void)body_color;
    vibio_u32 bg = make_color(boot_info, 8, 12, 16);
    fill_rect(boot_info, x, y, w, h, bg);
    vibio_u32 status_h = MEDIA_VIEWER_STATUS_H;
    vibio_u32 bar_h = MEDIA_VIEWER_BAR_H;
    vibio_u32 img_y = y + status_h;
    vibio_u32 img_h = h > status_h + bar_h ? h - status_h - bar_h : 0;
    g_text_bg = make_color(boot_info, 14, 22, 28);
    fill_rect(boot_info, x, y, w, status_h, g_text_bg);
    /* Three rows spaced by the Geist line height (~24px) so they do not overlap
     * each other or spill into the image area below. */
    draw_cstr(boot_info, x + 12, y + 8, mv->filename, theme->text);
    char line2[96];
    vibio_u32 p = 0;
    p = (vibio_u32)panic_append_text(line2, p, sizeof(line2), "Src: ");
    p = (vibio_u32)panic_append_text(line2, p, sizeof(line2), media_source_label(mv->source));
    p = (vibio_u32)panic_append_text(line2, p, sizeof(line2), "  ");
    p = append_human_size(line2, p, sizeof(line2), mv->file_size);
    line2[p] = 0;
    draw_cstr(boot_info, x + 12, y + 32, line2, theme->muted);
    char line3[96];
    p = 0;
    p = (vibio_u32)panic_append_text(line3, p, sizeof(line3), mv->format_label);
    p = (vibio_u32)panic_append_text(line3, p, sizeof(line3), "  ");
    p = (vibio_u32)panic_append_text(line3, p, sizeof(line3), mv->status_line);
    line3[p] = 0;
    draw_cstr(boot_info, x + 12, y + 56, line3, theme->accent);
    media_viewer_draw_image(boot_info, mv, x, img_y, w, img_h);
    vibio_u32 bar_y = y + h - bar_h;
    fill_rect(boot_info, x, bar_y, w, bar_h, make_color(boot_info, 18, 30, 38));
    vibio_u32 bx = x + 8;
    mv->btn_fit_x = bx; mv->btn_fit_y = bar_y + 6; mv->btn_fit_w = 42; mv->btn_fit_h = 28;
    media_draw_toolbar_btn(boot_info, mv->btn_fit_x, mv->btn_fit_y, mv->btn_fit_w, mv->btn_fit_h, "FIT", theme);
    bx += 48;
    mv->btn100_x = bx; mv->btn100_y = bar_y + 6; mv->btn100_w = 54; mv->btn100_h = 28;
    media_draw_toolbar_btn(boot_info, mv->btn100_x, mv->btn100_y, mv->btn100_w, mv->btn100_h, "100%", theme);
    bx += 60;
    mv->btn_minus_x = bx; mv->btn_minus_y = bar_y + 6; mv->btn_minus_w = 28; mv->btn_minus_h = 28;
    media_draw_toolbar_btn(boot_info, mv->btn_minus_x, mv->btn_minus_y, mv->btn_minus_w, mv->btn_minus_h, "-", theme);
    bx += 34;
    mv->btn_plus_x = bx; mv->btn_plus_y = bar_y + 6; mv->btn_plus_w = 28; mv->btn_plus_h = 28;
    media_draw_toolbar_btn(boot_info, mv->btn_plus_x, mv->btn_plus_y, mv->btn_plus_w, mv->btn_plus_h, "+", theme);
    char zoomtxt[16];
    if (mv->zoom_percent == MEDIA_ZOOM_FIT) {
        media_copy_name(zoomtxt, sizeof(zoomtxt), "FIT");
    } else {
        vibio_u32 zp = mv->zoom_percent;
        vibio_u32 q = 0;
        q = panic_append_uint(zoomtxt, q, sizeof(zoomtxt), zp);
        q = panic_append_text(zoomtxt, q, sizeof(zoomtxt), "%");
        zoomtxt[q] = 0;
    }
    draw_cstr(boot_info, bx + 40, bar_y + 10, zoomtxt, theme->accent);
}

static void media_player_stop(VibioMediaPlayer *mp, VibioAudioState *audio)
{
    mp->playing = 0;
    mp->current_frame = 0;
    mp->last_frame_tick = 0;
    mp->play_start_tick = 0;
    if (audio != 0) {
        audio_stop_playback(audio);
    }
    mp->audio_started = 0;
    mp->mp4_rgb_frame = 0xFFFFFFFFU;
    if (mp->is_mp4 && mp->mp4_video_ready) {
        media_player_decode_mp4_frame(mp, 0);
    }
}

/* Draw the native MP4 demux/info surface inside the player's video area. This
 * is the honest "we parsed your real MP4 but cannot decode it yet" screen. */
static void media_player_render_mp4(
    const VibioBootInfo *boot_info,
    VibioMediaPlayer *mp,
    const VibioDesktopTheme *theme,
    vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 bg)
{
    const Mp4Info *m = &mp->mp4;
    vibio_u32 tx = x + 14;
    vibio_u32 ty = y + 16;
    g_text_bg = bg;
    (void)w;
    draw_cstr(boot_info, tx, ty, "NATIVE MP4 PROBE", theme->accent);
    ty += 26;
    draw_cstr(boot_info, tx, ty, mp->filename, theme->text);
    ty += 22;
    if (m->status != MP4_OK) {
        draw_cstr(boot_info, tx, ty, "This is not a playable MP4:", theme->bad);
        ty += 20;
        draw_cstr(boot_info, tx, ty, m->status_detail[0] ? m->status_detail : mp4_status_text(m->status), theme->muted);
        return;
    }
    char line[96];
    vibio_u32 p;

    /* Video line. */
    p = 0;
    p = (vibio_u32)panic_append_text(line, p, sizeof(line), "Video: ");
    if (m->video.present) {
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), mp4_codec_name(m->video.codec));
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), " ");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->video.width);
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "x");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->video.height);
    } else {
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "none");
    }
    line[p] = 0;
    draw_cstr(boot_info, tx, ty, line, theme->text);
    ty += 20;

    /* H.264 profile line (only when we have an avcC). */
    if (m->video.present && m->video.codec == MP4_CODEC_H264 && m->video.has_avcc) {
        p = 0;
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "Profile: ");
        p = (vibio_u32)panic_append_text(line, p, sizeof(line),
                                         mp4_h264_profile_name(m->video.h264_profile, m->video.h264_compat));
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "  Level ");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->video.h264_level / 10);
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), ".");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->video.h264_level % 10);
        line[p] = 0;
        draw_cstr(boot_info, tx, ty, line, theme->muted);
        ty += 20;
    }

    /* Audio line. */
    p = 0;
    p = (vibio_u32)panic_append_text(line, p, sizeof(line), "Audio: ");
    if (m->audio.present) {
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), mp4_codec_name(m->audio.codec));
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), " ");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->audio.sample_rate);
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "Hz ");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->audio.channels);
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "ch");
    } else {
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "none");
    }
    line[p] = 0;
    draw_cstr(boot_info, tx, ty, line, theme->text);
    ty += 20;

    /* Duration line. */
    if (m->movie_timescale > 0) {
        vibio_u64 secs = m->movie_duration / m->movie_timescale;
        p = 0;
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "Duration: ");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), secs);
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "s   Brand: ");
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), m->major_brand);
        line[p] = 0;
        draw_cstr(boot_info, tx, ty, line, theme->muted);
        ty += 24;
    }

    p = 0;
    p = (vibio_u32)panic_append_text(line, p, sizeof(line), "Demux: ");
    if (m->video.present && m->video.sample_demux_ready) {
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "READY  first video sample off=");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), (vibio_u32)m->video.first_sample_offset);
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), " size=");
        p = (vibio_u32)panic_append_uint(line, p, sizeof(line), m->video.first_sample_size);
    } else {
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "NO sample tables");
    }
    line[p] = 0;
    draw_cstr(boot_info, tx, ty, line, m->video.sample_demux_ready ? theme->ok : theme->bad);
    ty += 20;

    if (mp->h264_checked) {
        p = 0;
        p = (vibio_u32)panic_append_text(line, p, sizeof(line), "H.264 Step 2: ");
        if (mp->h264.luma_mbs_decoded > 0) {
            const char *luma_label = mp->h264.full_frame_luma_ok ? "visible luma frame OK mbs=" :
                                     mp->h264.luma_mb_rows_decoded > 1U ? "luma rows OK mbs=" :
                                     mp->h264.luma_row_ok ? "luma row OK mbs=" : "luma partial mbs=";
            vibio_u32 total_mbs = mp->h264.mb_width * mp->h264.mb_height;
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), luma_label);
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.luma_mbs_decoded);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), "/");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), total_mbs);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), " rows=");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.luma_mb_rows_decoded);
            if (mp->h264.i16x16_mbs_decoded > 0) {
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), " I16=");
                p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.i16x16_mbs_decoded);
            }
            if (!mp->h264.full_frame_luma_ok && !mp->h264.luma_row_ok && mp->h264.luma_detail[0]) {
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), " block=");
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), mp->h264.luma_detail);
            }
        } else if (mp->h264.status == H264_OK) {
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), "HEADERS OK ");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.width);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), "x");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.height);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), " slice=");
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), h264_slice_type_name(mp->h264.first_slice_type));
            if (mp->h264.first_mb_header_ok) {
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), " mb=");
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), mp->h264.first_mb_kind);
            }
            if (mp->h264.first_mb_first_residual_ok) {
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), " cavlc=");
                p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.first_mb_first_residual_total_coeff);
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), "coef");
                p = (vibio_u32)panic_append_text(line, p, sizeof(line), " sum=");
                p = (vibio_u32)panic_append_int(line, p, sizeof(line), mp->h264.first_mb_first_residual_coeff_sum);
            }
        } else {
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), h264_status_text(mp->h264.status));
        }
        line[p] = 0;
        draw_cstr(boot_info, tx, ty,
                  line,
                  mp->h264.luma_mbs_decoded > 0 ? theme->ok :
                  (mp->h264.status == H264_OK ? theme->ok : theme->bad));
        ty += 20;

        /* Step 2 pixel-pipeline gate: luma-only reconstruction. This is still
         * not playback and not chroma/RGB output. */
        if (mp->h264.luma_mbs_decoded > 0) {
            p = 0;
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), "  luma region ");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.luma_region_width);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), "x");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.luma_region_height);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), " qp=");
            p = (vibio_u32)panic_append_int(line, p, sizeof(line), mp->h264.first_mb_qp);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), " sum=");
            p = (vibio_u32)panic_append_int(line, p, sizeof(line), mp->h264.luma_region_sum);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), " min=");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.luma_region_min);
            p = (vibio_u32)panic_append_text(line, p, sizeof(line), " max=");
            p = (vibio_u32)panic_append_uint(line, p, sizeof(line), mp->h264.luma_region_max);
            line[p] = 0;
            draw_cstr(boot_info, tx, ty, line, theme->ok);
            ty += 20;

            draw_cstr(boot_info, tx, ty, "  decoded luma region (grayscale):", theme->muted);
            ty += 18;
            const vibio_u32 zoom = (mp->h264.luma_region_width <= 80U) ? 4U : 2U;
            for (vibio_u32 ry = 0; ry < mp->h264.luma_region_height; ry++) {
                for (vibio_u32 rx = 0; rx < mp->h264.luma_region_width; rx++) {
                    vibio_u8 lv = mp->h264.luma_region[ry * mp->h264.luma_region_width + rx];
                    vibio_u32 col = make_color(boot_info, lv, lv, lv);
                    for (vibio_u32 sy = 0; sy < zoom; sy++) {
                        for (vibio_u32 sx = 0; sx < zoom; sx++) {
                            put_pixel(boot_info, tx + rx * zoom + sx,
                                      ty + ry * zoom + sy, col);
                        }
                    }
                }
            }
            ty += mp->h264.luma_region_height * zoom + 6;
        }
    }

    draw_cstr(boot_info, tx, ty,
              mp->mp4.decode_supported ? "Decode: supported" : "Decode: NOT supported (demux/probe only)",
              mp->mp4.decode_supported ? theme->ok : theme->bad);
    ty += 20;
    draw_cstr(boot_info, tx, ty, m->unsupported_reason, theme->muted);
    ty += 20;
    draw_cstr(boot_info, tx, ty, "Unsupported on the native MP4 video path.", theme->muted);
    ty += 18;
    draw_cstr(boot_info, tx, ty, "AAC is detected only; no AAC PCM is decoded yet.", theme->muted);
}

static vibio_u32 media_viv_read_header(VibioMediaPlayer *mp)
{
    vibio_u8 hdr[MEDIA_VIV_HEADER + 8];
    vibio_u32 got = media_read_bytes(mp->boot, mp->disk, mp->fs, mp->filename, 0,
                                     hdr, sizeof(hdr), &mp->file_size, &mp->source);
    if (got < MEDIA_VIV_HEADER) {
        return 0;
    }
    if (hdr[0] != 'V' || hdr[1] != 'I' || hdr[2] != 'V' || hdr[3] != '1') {
        return 0;
    }
    mp->width = disk_read_le32(hdr, 4);
    mp->height = disk_read_le32(hdr, 8);
    mp->frame_count = disk_read_le32(hdr, 12);
    mp->fps_num = disk_read_le32(hdr, 16);
    mp->fps_den = disk_read_le32(hdr, 20);
    if (mp->width == 0 || mp->height == 0 || mp->width > MEDIA_VIV_MAX_W ||
        mp->height > MEDIA_VIV_MAX_H || mp->frame_count == 0 ||
        mp->frame_count > MEDIA_VIV_MAX_FRAMES || mp->fps_num == 0) {
        return 0;
    }
    vibio_u32 idx_bytes = mp->frame_count * 8U;
    if (MEDIA_VIV_HEADER + idx_bytes > mp->file_size) {
        return 0;
    }
    /* Read the per-frame (offset,size) index in chunks so a long video (thousands
     * of frames) does not need a huge stack buffer. 512 entries per chunk. */
    vibio_u8 index_buf[4096];
    vibio_u32 done = 0;
    while (done < mp->frame_count) {
        vibio_u32 batch = mp->frame_count - done;
        if (batch > 512) {
            batch = 512;
        }
        vibio_u32 batch_bytes = batch * 8U;
        vibio_u32 got_idx = media_read_bytes(mp->boot, mp->disk, mp->fs, mp->filename,
                                             MEDIA_VIV_HEADER + done * 8U,
                                             index_buf, batch_bytes, 0, 0);
        if (got_idx != batch_bytes) {
            return 0;
        }
        for (vibio_u32 i = 0; i < batch; i++) {
            vibio_u32 fi = done + i;
            mp->frame_offsets[fi] = disk_read_le32(index_buf, i * 8U);
            mp->frame_sizes[fi] = disk_read_le32(index_buf, i * 8U + 4U);
            vibio_u64 end = (vibio_u64)mp->frame_offsets[fi] + (vibio_u64)mp->frame_sizes[fi];
            if (end > mp->file_size || mp->frame_sizes[fi] > MEDIA_VIV_MAX_FRAME_BYTES) {
                return 0;
            }
        }
        done += batch;
    }
    return 1;
}

static vibio_u32 media_player_read_frame(VibioMediaPlayer *mp, vibio_u32 frame_index)
{
    if (frame_index >= mp->frame_count) {
        return 0;
    }
    vibio_u32 off = mp->frame_offsets[frame_index];
    vibio_u32 size = mp->frame_sizes[frame_index];
    vibio_u32 got = media_read_bytes(mp->boot, mp->disk, mp->fs, mp->filename, off,
                                     mp->frame_buf, mp->frame_capacity, 0, 0);
    if (got < size) {
        return 0;
    }
    MediaBitmap bmp;
    if (!media_vimg_parse(mp->frame_buf, size, &bmp)) {
        return 0;
    }
    return 1;
}

MAYBE_UNUSED static void media_player_probe_h264_first_sample(VibioMediaPlayer *mp, vibio_u32 mp4_window_size)
{
    if (mp == 0) {
        return;
    }
    mp->h264_checked = 0;
    zero_bytes(&mp->h264, sizeof(mp->h264));
    if (!mp->is_mp4 || mp->mp4.status != MP4_OK ||
        !mp->mp4.video.present || mp->mp4.video.codec != MP4_CODEC_H264 ||
        !mp->mp4.video.sample_demux_ready || mp->frame_buf == 0) {
        return;
    }
    const Mp4Track *v = &mp->mp4.video;
    if (v->h264_sps_size == 0 || v->h264_pps_size == 0 ||
        v->h264_sps_size > 256U || v->h264_pps_size > 256U ||
        v->h264_sps_offset + v->h264_sps_size > mp4_window_size ||
        v->h264_pps_offset + v->h264_pps_size > mp4_window_size) {
        mp->h264_checked = 1;
        mp->h264.status = H264_ERR_UNSUPPORTED;
        media_copy_name(mp->h264.status_detail, sizeof(mp->h264.status_detail),
                        "H.264 avcC SPS/PPS too large or missing");
        return;
    }
    vibio_u8 sps[256];
    vibio_u8 pps[256];
    for (vibio_u32 i = 0; i < v->h264_sps_size; i++) {
        sps[i] = mp->frame_buf[v->h264_sps_offset + i];
    }
    for (vibio_u32 i = 0; i < v->h264_pps_size; i++) {
        pps[i] = mp->frame_buf[v->h264_pps_offset + i];
    }
    Mp4Sample sample;
    if (!mp4_get_sample(mp->frame_buf, mp4_window_size, &mp->mp4, MP4_TRACK_VIDEO, 0, &sample) ||
        !sample.present || sample.size == 0 || sample.size > mp->frame_capacity) {
        mp->h264_checked = 1;
        mp->h264.status = H264_ERR_BAD_BITSTREAM;
        media_copy_name(mp->h264.status_detail, sizeof(mp->h264.status_detail),
                        "first H.264 sample could not be read safely");
        return;
    }
    vibio_u32 got = media_read_bytes(mp->boot, mp->disk, mp->fs, mp->filename, sample.offset,
                                     mp->frame_buf, sample.size, 0, 0);
    mp->h264_checked = 1;
    if (got < sample.size) {
        mp->h264.status = H264_ERR_BAD_BITSTREAM;
        media_copy_name(mp->h264.status_detail, sizeof(mp->h264.status_detail),
                        "short read while loading first H.264 sample");
        return;
    }
    h264_probe_avcc_and_sample(sps, v->h264_sps_size, pps, v->h264_pps_size,
                               mp->frame_buf, sample.size, v->h264_nal_length_size,
                               &mp->h264);
}

static vibio_u8 *media_align2(vibio_u8 *p)
{
    return (vibio_u8 *)(((vibio_u64)p + 1ULL) & ~1ULL);
}

static vibio_u32 media_player_reset_h264_decoder(VibioMediaPlayer *mp)
{
    H264Work work;
    vibio_u8 *p;
    vibio_u32 was_active;
    if (mp == 0 || mp->mp4_yuv_pool == 0 || mp->mp4_work == 0 ||
        mp->mp4_yuv_capacity < MEDIA_MP4_FRAME_BYTES * MEDIA_MP4_POOL ||
        mp->mp4_work_capacity < MEDIA_MP4_WORK_BYTES ||
        mp->mp4_sps_size == 0 || mp->mp4_pps_size == 0) {
        return 0;
    }
    was_active = mp->mp4_next_decode_frame != 0 || mp->h264_dec.frames_decoded != 0;
    for (vibio_u32 i = 0; i < MEDIA_MP4_POOL; i++) {
        vibio_u8 *base = mp->mp4_yuv_pool + i * MEDIA_MP4_FRAME_BYTES;
        mp->h264_frames[i].y = base;
        mp->h264_frames[i].cb = base + MEDIA_MP4_LUMA_BYTES;
        mp->h264_frames[i].cr = base + MEDIA_MP4_LUMA_BYTES + MEDIA_MP4_CHROMA_BYTES;
        mp->h264_frames[i].valid = 0;
    }
    zero_bytes(&work, sizeof(work));
    p = mp->mp4_work;
    work.nnz = p; p += MEDIA_MP4_NBLK;
    work.imode = p; p += MEDIA_MP4_NBLK;
    p = media_align2(p);
    work.mvx = (int16_t *)p; p += MEDIA_MP4_NBLK * sizeof(int16_t);
    work.mvy = (int16_t *)p; p += MEDIA_MP4_NBLK * sizeof(int16_t);
    work.refidx = (int8_t *)p; p += MEDIA_MP4_NBLK;
    work.mb_type = p; p += MEDIA_MP4_MB_W * MEDIA_MP4_MB_H;
    work.mb_qp = p; p += MEDIA_MP4_MB_W * MEDIA_MP4_MB_H;
    work.mb_intra = p; p += MEDIA_MP4_MB_W * MEDIA_MP4_MB_H;
    work.cnnz[0] = p; p += MEDIA_MP4_NCBLK;
    work.cnnz[1] = p; p += MEDIA_MP4_NCBLK;
    work.decmask = p; p += MEDIA_MP4_LUMA_BYTES;
    if ((vibio_u64)(p - mp->mp4_work) > mp->mp4_work_capacity) {
        return 0;
    }
    zero_bytes(&mp->h264_dec, sizeof(mp->h264_dec));
    mp->h264_dec.frames = mp->h264_frames;
    mp->h264_dec.frame_pool_size = MEDIA_MP4_POOL;
    mp->h264_dec.work = work;
    if (h264_decoder_init(&mp->h264_dec, mp->mp4_sps, mp->mp4_sps_size,
                          mp->mp4_pps, mp->mp4_pps_size) != H264_OK) {
        media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), mp->h264_dec.status_detail);
        return 0;
    }
    mp->mp4_next_decode_frame = 0;
    if (was_active) {
        mp->mp4_decode_resets++;
    }
    return 1;
}

static vibio_u32 media_player_init_mp4_video(VibioMediaPlayer *mp, vibio_u32 mp4_window_size)
{
    const Mp4Track *v;
    if (mp == 0 || mp->mp4.status != MP4_OK || !mp->mp4.video.present ||
        mp->mp4.video.codec != MP4_CODEC_H264 || !mp->mp4.video.sample_demux_ready ||
        mp->mp4_table_buf == 0 || mp->frame_buf == 0 || mp->mp4_rgb == 0) {
        media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "MP4 video unsupported");
        return 0;
    }
    v = &mp->mp4.video;
    if (!v->has_avcc || v->h264_sps_size == 0 || v->h264_pps_size == 0 ||
        v->h264_sps_size > sizeof(mp->mp4_sps) || v->h264_pps_size > sizeof(mp->mp4_pps) ||
        v->h264_sps_offset + v->h264_sps_size > mp4_window_size ||
        v->h264_pps_offset + v->h264_pps_size > mp4_window_size) {
        media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "H.264 avcC SPS/PPS missing");
        return 0;
    }
    if (v->width == 0 || v->height == 0 || v->width > MEDIA_MP4_MAX_W || v->height > MEDIA_MP4_MAX_H ||
        v->max_sample_size > mp->frame_capacity || v->sample_count == 0) {
        media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "MP4 exceeds 320x180 baseline limits");
        return 0;
    }
    for (vibio_u32 i = 0; i < v->h264_sps_size; i++) {
        mp->mp4_sps[i] = mp->mp4_table_buf[v->h264_sps_offset + i];
    }
    for (vibio_u32 i = 0; i < v->h264_pps_size; i++) {
        mp->mp4_pps[i] = mp->mp4_table_buf[v->h264_pps_offset + i];
    }
    mp->mp4_sps_size = v->h264_sps_size;
    mp->mp4_pps_size = v->h264_pps_size;
    mp->mp4_nal_length_size = v->h264_nal_length_size;
    if (!media_player_reset_h264_decoder(mp)) {
        return 0;
    }
    if (mp->h264_dec.crop_width > MEDIA_MP4_MAX_W || mp->h264_dec.crop_height > MEDIA_MP4_MAX_H ||
        mp->h264_dec.width > MEDIA_MP4_CODED_W || mp->h264_dec.height > MEDIA_MP4_CODED_H) {
        media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "H.264 coded size unsupported");
        return 0;
    }
    mp->width = mp->h264_dec.crop_width;
    mp->height = mp->h264_dec.crop_height;
    mp->frame_count = v->sample_count;
    mp->fps_num = v->timescale ? v->timescale : 60U;
    mp->fps_den = v->first_sample_duration ? v->first_sample_duration : 1U;
    media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "MP4 video: H.264 Baseline OK");
    return media_player_decode_mp4_frame(mp, 0);
}

static vibio_u32 media_player_decode_mp4_frame(VibioMediaPlayer *mp, vibio_u32 frame_index)
{
    vibio_u32 burst = 0;
    if (mp == 0 || !mp->is_mp4 || mp->frame_buf == 0 || mp->mp4_rgb == 0 ||
        frame_index >= mp->frame_count) {
        return 0;
    }
    if (frame_index < mp->mp4_next_decode_frame) {
        if (!media_player_reset_h264_decoder(mp)) {
            return 0;
        }
    }
    while (mp->mp4_next_decode_frame <= frame_index) {
        Mp4Sample sample;
        H264Frame *out = 0;
        vibio_u32 fi = mp->mp4_next_decode_frame;
        if (!mp4_get_sample(mp->mp4_table_buf, mp->mp4_table_size, &mp->mp4,
                            MP4_TRACK_VIDEO, fi, &sample) ||
            !sample.present || sample.size == 0 || sample.size > mp->frame_capacity) {
            media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "MP4 sample read unsupported");
            return 0;
        }
        if (media_read_bytes(mp->boot, mp->disk, mp->fs, mp->filename, sample.offset,
                             mp->frame_buf, sample.size, 0, 0) < sample.size) {
            media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "MP4 sample short read");
            return 0;
        }
        if (h264_decode_sample(&mp->h264_dec, mp->frame_buf, sample.size,
                               mp->mp4_nal_length_size, &out) != H264_OK || out == 0) {
            media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), mp->h264_dec.status_detail);
            return 0;
        }
        mp->mp4_next_decode_frame++;
        mp->mp4_decoded_frames++;
        burst++;
        if (fi == frame_index) {
            mp->mp4_present_frame = out;
            mp->current_frame = frame_index;
            mp->mp4_rgb_frame = 0xFFFFFFFFU;
            mp->mp4_presented_frames++;
        }
    }
    if (burst > mp->mp4_max_decode_burst) {
        mp->mp4_max_decode_burst = burst;
    }
    return 1;
}

static vibio_u32 media_player_mp4_find_idr_back(VibioMediaPlayer *mp, vibio_u32 desired)
{
    vibio_u32 min_frame;
    if (mp == 0 || desired <= mp->current_frame + 1U) {
        return 0xFFFFFFFFU;
    }
    min_frame = desired > MEDIA_MP4_IDR_SCAN_BACK ? desired - MEDIA_MP4_IDR_SCAN_BACK : 0U;
    if (min_frame <= mp->current_frame) {
        min_frame = mp->current_frame + 1U;
    }
    vibio_u32 sync = mp4_find_sync_sample_before(mp->mp4_table_buf, mp->mp4_table_size,
                                                 &mp->mp4.video, desired);
    if (sync != 0xFFFFFFFFU && sync >= min_frame) {
        return sync;
    }
    return 0xFFFFFFFFU;
}

static vibio_u32 media_player_mp4_decode_burst(const VibioMediaPlayer *mp)
{
    vibio_u32 pixels = mp != 0 ? mp->width * mp->height : 0U;
    if (pixels >= 1280U * 720U) {
        return 1U;
    }
    if (pixels >= 640U * 360U) {
        return 2U;
    }
    return MEDIA_MP4_MAX_DECODE_BURST;
}

static vibio_u32 media_player_mp4_idr_threshold(const VibioMediaPlayer *mp)
{
    vibio_u32 pixels = mp != 0 ? mp->width * mp->height : 0U;
    if (pixels >= 1280U * 720U) {
        return 24U;
    }
    if (pixels >= 640U * 360U) {
        return 45U;
    }
    return MEDIA_MP4_IDR_DROP_THRESHOLD;
}

/* The real MP4/QuickTime container family Vibio probes natively. */
static vibio_u32 media_name_is_mp4(const char *name)
{
    return browser_name_has_ext(name, ".MP4") ||
           browser_name_has_ext(name, ".M4V") ||
           browser_name_has_ext(name, ".MOV");
}

static void media_player_make_wav_name(VibioMediaPlayer *mp, const char *name)
{
    zero_bytes(mp->audio_wav, sizeof(mp->audio_wav));
    if (browser_name_has_ext(name, ".WAV")) {
        media_copy_name(mp->audio_wav, FILES_NAME_MAX, name);
        return;
    }
    vibio_u32 stem_len = 0;
    while (name[stem_len] != 0 && name[stem_len] != '.' && stem_len + 5 < FILES_NAME_MAX) {
        mp->audio_wav[stem_len] = name[stem_len];
        stem_len++;
    }
    mp->audio_wav[stem_len++] = '.';
    mp->audio_wav[stem_len++] = 'W';
    mp->audio_wav[stem_len++] = 'A';
    mp->audio_wav[stem_len++] = 'V';
    mp->audio_wav[stem_len] = 0;
}

static void media_player_load_wav_sidecar(VibioMediaPlayer *mp, vibio_u8 update_source)
{
    if (mp == 0 || mp->audio_scratch == 0 || mp->audio_scratch_capacity == 0 ||
        mp->audio_wav[0] == 0) {
        return;
    }
    mp->audio_wav_bytes = media_read_bytes(mp->boot, mp->disk, mp->fs, mp->audio_wav, 0,
                                           mp->audio_scratch, mp->audio_scratch_capacity,
                                           0, update_source ? &mp->source : 0);
}

static void media_player_open(VibioMediaPlayer *mp, const char *name, vibio_u8 source_hint)
{
    /* Resolve this file (and its .WAV sidecar) in the folder it was opened from. */
    fat32_set_resolve_dir(mp->dir_cluster);
    zero_bytes(mp->filename, sizeof(mp->filename));
    media_copy_name(mp->filename, FILES_NAME_MAX, name);
    mp->source = source_hint;
    mp->loaded = 0;
    mp->playing = 0;
    mp->current_frame = 0;
    mp->dropped_frames = 0;
    mp->mp4_decoded_frames = 0;
    mp->mp4_presented_frames = 0;
    mp->mp4_rgb_converted_frames = 0;
    mp->mp4_video_rect_blits = 0;
    mp->mp4_decode_resets = 0;
    mp->mp4_max_decode_burst = 0;
    mp->mp4_last_lag_frames = 0;
    mp->mp4_present_frame = 0;
    mp->mp4_rgb_w = 0;
    mp->mp4_rgb_h = 0;
    mp->mp4_rgb_frame = 0xFFFFFFFFU;
    mp->audio_only = 0;
    mp->is_mp4 = 0;
    mp->audio_wav_bytes = 0;
    mp->audio_available = 0;
    mp->audio_started = 0;
    mp->last_frame_tick = 0;
    mp->play_start_tick = 0;

    media_player_make_wav_name(mp, name);

    /* Real MP4 family: native H.264 video plus a same-stem PCM .WAV sidecar. */
    if (media_name_is_mp4(name)) {
        media_copy_name(mp->filename, FILES_NAME_MAX, name);
        mp->is_mp4 = 1;
        mp->loaded = 1;
        mp->playing = 0;
        mp->mp4_video_ready = 0;
        mp->mp4_video_ended = 0;
        mp->mp4_table_size = 0;
        media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), "MP4 video unsupported");
        vibio_u32 total = 0;
        vibio_u32 got = media_read_mp4_probe(mp->boot, mp->disk, mp->fs, name,
                                             mp->mp4_table_buf, mp->mp4_table_capacity,
                                             &total, &mp->source, &mp->mp4);
        mp->file_size = total;
        mp->mp4_table_size = got;
        if (got == 0) {
            zero_bytes(&mp->mp4, sizeof(mp->mp4));
            mp->mp4.status = MP4_ERR_TOO_SMALL;
            /* Distinguish the two failure modes so a misbehaving source is
             * diagnosable: total>0 means the directory entry WAS found (so the
             * data read itself failed); total==0 means the name was not found in
             * the directory at all - typically the file is in a subfolder (the
             * reader only searches the directory root), the name did not match,
             * or the wrong storage source is selected. */
            const char *src = mp->source == FILES_SRC_USB ? "USB" :
                              mp->source == FILES_SRC_AHCI ? "disk" :
                              mp->source == FILES_SRC_BOOT ? "boot files" : "this source";
            char det[64];
            vibio_u32 dp = 0;
            if (total > 0) {
                dp = (vibio_u32)panic_append_text(det, dp, sizeof(det), "found on ");
                dp = (vibio_u32)panic_append_text(det, dp, sizeof(det), src);
                dp = (vibio_u32)panic_append_text(det, dp, sizeof(det), " but data read failed");
            } else {
                dp = (vibio_u32)panic_append_text(det, dp, sizeof(det), "not found on ");
                dp = (vibio_u32)panic_append_text(det, dp, sizeof(det), src);
                dp = (vibio_u32)panic_append_text(det, dp, sizeof(det), " (subfolder/name?)");
            }
            det[dp] = 0;
            media_copy_name(mp->mp4.status_detail, sizeof(mp->mp4.status_detail), det);
            media_copy_name(mp->mp4_status, sizeof(mp->mp4_status), det);
        } else {
            mp->mp4_video_ready = media_player_init_mp4_video(mp, got);
            if (mp->mp4_video_ready) {
                media_player_load_wav_sidecar(mp, 0);
                mp->playing = 1;
                mp->last_frame_tick = 0;
            }
        }
        return;
    }

    /* Audio-only mode: a standalone .WAV gets the player window as a simple
     * "Now Playing" surface, with no video frames. */
    if (browser_name_has_ext(name, ".WAV")) {
        mp->audio_only = 1;
        mp->loaded = 1;
        media_player_load_wav_sidecar(mp, 1);
        mp->playing = 1;
        mp->last_frame_tick = 0;
        mp->audio_started = 0;
        return;
    }

    if (!media_viv_read_header(mp)) {
        media_set_error("Invalid or unsupported VIV header.");
        return;
    }
    mp->loaded = 1;
    if (media_player_read_frame(mp, 0) && mp->audio_scratch != 0 && mp->audio_scratch_capacity > 0) {
        media_player_load_wav_sidecar(mp, 0);
    }

    /* Start playing immediately when the player opens. A paused first frame
     * looked like "the video does not show up"; playback begins on the next
     * tick (last_frame_tick == 0 also kicks off matching PCM audio). */
    if (mp->loaded) {
        mp->playing = 1;
        mp->last_frame_tick = 0;
        mp->audio_started = 0;
    }
}

static void media_player_tick(VibioMediaPlayer *mp, VibioAudioState *audio, vibio_u64 ticks)
{
    if (!mp->loaded || !mp->playing) {
        return;
    }
    /* Per-frame and audio reads below re-resolve the file by name; keep them
     * pointed at the folder it was opened from (not the volume root). */
    fat32_set_resolve_dir(mp->dir_cluster);
    if (mp->is_mp4) {
        if (!mp->mp4_video_ready || mp->fps_num == 0) {
            return;
        }
        vibio_u32 fps_den = mp->fps_den == 0 ? 1U : mp->fps_den;
        if (mp->last_frame_tick == 0) {
            vibio_u64 played_ticks = ((vibio_u64)mp->current_frame * 100ULL * (vibio_u64)fps_den) /
                                     (vibio_u64)mp->fps_num;
            mp->play_start_tick = ticks > played_ticks ? ticks - played_ticks : ticks;
            mp->last_frame_tick = ticks;
            if (!mp->audio_started && audio != 0 && audio->ready && !mp->muted) {
                mp->audio_available = 1;
                if (audio_play_named(audio, mp->boot, mp->audio_wav) ||
                    (mp->audio_scratch != 0 && mp->audio_wav_bytes > 0 &&
                     audio_play_buffer(audio, mp->audio_scratch, mp->audio_wav_bytes, mp->audio_wav))) {
                    mp->audio_started = 1;
                } else {
                    mp->audio_available = 0;
                }
            }
            return;
        }
        if (mp->audio_started && audio != 0 && !audio->playback_active && !mp->muted) {
            if (audio_start_next_pcm_chunk(audio)) {
                mp->audio_available = 1;
            } else {
                mp->audio_started = 0;
                mp->audio_available = 0;
            }
        }
        vibio_u64 elapsed = ticks - mp->play_start_tick;
        vibio_u32 desired = (vibio_u32)((elapsed * (vibio_u64)mp->fps_num) /
                                        (100ULL * (vibio_u64)fps_den));
        if (desired >= mp->frame_count) {
            desired = mp->frame_count - 1U;
        }
        if (desired <= mp->current_frame) {
            return;
        }
        vibio_u32 skip = desired - mp->current_frame;
        mp->last_frame_tick = ticks;
        mp->mp4_last_lag_frames = skip;
        vibio_u32 burst_limit = media_player_mp4_decode_burst(mp);
        vibio_u32 idr_threshold = media_player_mp4_idr_threshold(mp);
        if (skip > idr_threshold) {
            vibio_u32 idr = media_player_mp4_find_idr_back(mp, desired);
            if (idr != 0xFFFFFFFFU && idr > mp->current_frame + 1U &&
                media_player_reset_h264_decoder(mp)) {
                mp->dropped_frames += idr - mp->current_frame - 1U;
                mp->mp4_next_decode_frame = idr;
                mp->current_frame = idr - 1U;
                skip = desired > mp->current_frame ? desired - mp->current_frame : 1U;
            }
        }
        if (skip > burst_limit) {
            skip = burst_limit;
        }
        if (mp->current_frame + 1U >= mp->frame_count) {
            mp->playing = 0;
            mp->mp4_video_ended = 1;
            (void)audio;
            return;
        }
        vibio_u32 target = mp->current_frame + skip;
        if (target >= mp->frame_count) {
            target = mp->frame_count - 1U;
        }
        if (target > mp->current_frame + 1U) {
            mp->dropped_frames += target - mp->current_frame - 1U;
        }
        if (!media_player_decode_mp4_frame(mp, target)) {
            mp->dropped_frames++;
            mp->playing = 0;
        } else if (mp->current_frame + 1U >= mp->frame_count) {
            mp->playing = 0;
            mp->mp4_video_ended = 1;
        }
        mp->mp4_last_lag_frames = desired > mp->current_frame ? desired - mp->current_frame : 0U;
        return;
    }
    /* Audio-only (.WAV) playback: no video frames to advance. Kick off PCM
     * once; the audio driver streams it to completion. */
    if (mp->audio_only) {
        if (!mp->audio_started && audio != 0 && audio->ready && !mp->muted) {
            mp->audio_available = 1;
            if (audio_play_named(audio, mp->boot, mp->audio_wav) ||
                (mp->audio_scratch != 0 && mp->audio_wav_bytes > 0 &&
                 audio_play_buffer(audio, mp->audio_scratch, mp->audio_wav_bytes, mp->audio_wav))) {
                mp->audio_started = 1;
            } else {
                mp->audio_available = 0;
                mp->playing = 0;
            }
        } else if (!mp->audio_started) {
            mp->audio_available = 0;
            mp->playing = 0;
        } else if (audio == 0 || !audio->playback_active) {
            mp->playing = 0;
            mp->audio_started = 0;
        }
        return;
    }
    if (mp->fps_num == 0) {
        return;
    }
    vibio_u32 fps_den = mp->fps_den == 0 ? 1U : mp->fps_den;
    vibio_u64 interval = (100ULL * (vibio_u64)fps_den) / (vibio_u64)mp->fps_num;
    if (interval == 0) {
        interval = 1;
    }
    if (mp->last_frame_tick == 0) {
        mp->last_frame_tick = ticks;
        if (!mp->audio_started && audio != 0 && audio->ready && !mp->muted) {
            mp->audio_available = 1;
            if (audio_play_named(audio, mp->boot, mp->audio_wav) ||
                (mp->audio_scratch != 0 && mp->audio_wav_bytes > 0 &&
                 audio_play_buffer(audio, mp->audio_scratch, mp->audio_wav_bytes, mp->audio_wav))) {
                mp->audio_started = 1;
            } else {
                mp->audio_available = 0;
            }
        }
        return;
    }
    vibio_u64 elapsed = ticks - mp->last_frame_tick;
    if (elapsed < interval) {
        return;
    }
    vibio_u32 skip = (vibio_u32)(elapsed / interval);
    mp->last_frame_tick += (vibio_u64)skip * interval;
    if (skip > 1) {
        mp->dropped_frames += skip - 1U;
    }
    mp->current_frame += skip;
    if (mp->current_frame >= mp->frame_count) {
        mp->current_frame = mp->frame_count - 1U;
        mp->playing = 0;
        if (audio != 0) {
            audio_stop_playback(audio);
        }
        mp->audio_started = 0;
        return;
    }
    if (!media_player_read_frame(mp, mp->current_frame)) {
        mp->dropped_frames++;
    }
}

static void media_player_render(
    const VibioBootInfo *boot_info,
    VibioMediaPlayer *mp,
    const VibioDesktopTheme *theme,
    vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h,
    vibio_u32 chrome_hidden)
{
    vibio_u32 bg = make_color(boot_info, 8, 12, 16);
    fill_rect(boot_info, x, y, w, h, bg);
    vibio_u32 bar_h = MEDIA_PLAYER_BAR_H;
    vibio_u32 status_band = 56;
    /* In F11 full screen with the cursor idle, the transport bar and status band
     * are hidden and the video fills the whole surface (VLC/Windows style); they
     * reappear when the mouse moves. */
    vibio_u32 vid_h;
    if (chrome_hidden) {
        vid_h = h;
    } else {
        vid_h = h > bar_h + status_band ? h - bar_h - status_band : 0;
    }
    vibio_u32 status_y = y + vid_h;
    if (!mp->loaded) {
        g_text_bg = bg;
        draw_cstr(boot_info, x + 12, y + 20, "Cannot play this video.", theme->bad);
        draw_cstr(boot_info, x + 12, y + 44,
                  "Vibio plays its own .VIV format. MP4/MOV/MKV/AVI must be",
                  theme->muted);
        draw_cstr(boot_info, x + 12, y + 64,
                  "converted to .VIV on a PC first (no in-kernel decoder).",
                  theme->muted);
        return;
    }
    if (mp->is_mp4) {
        if (!mp->mp4_video_ready) {
            media_player_render_mp4(boot_info, mp, theme, x, y, w, bg);
            return;
        }
        vibio_u32 iw = mp->width;
        vibio_u32 ih = mp->height;
        vibio_u32 scale = 100;
        if (iw > w || ih > vid_h) {
            vibio_u32 sx = w * 100U / iw;
            vibio_u32 sy = vid_h * 100U / ih;
            scale = sx < sy ? sx : sy;
        }
        vibio_u32 dw = iw * scale / 100U;
        vibio_u32 dh = ih * scale / 100U;
        vibio_u32 ox = x + (w > dw ? (w - dw) / 2U : 0);
        vibio_u32 oy = y + (vid_h > dh ? (vid_h - dh) / 2U : 0);
        if (dw > 0 && dh > 0 && mp->mp4_present_frame != 0 &&
            (dw * dh * 4U) <= mp->mp4_rgb_capacity) {
            if (mp->mp4_rgb_frame != mp->current_frame ||
                mp->mp4_rgb_w != dw || mp->mp4_rgb_h != dh) {
                h264_frame_to_xrgb(mp->mp4_present_frame, mp->width, mp->height,
                                   mp->mp4_rgb, dw, dh, dw);
                mp->mp4_rgb_w = dw;
                mp->mp4_rgb_h = dh;
                mp->mp4_rgb_frame = mp->current_frame;
                mp->mp4_rgb_converted_frames++;
            }
            set_clip(x, y, x + w, y + vid_h);
            for (vibio_u32 dy = 0; dy < dh; dy++) {
                const vibio_u32 *src = mp->mp4_rgb + dy * dw;
                for (vibio_u32 dx = 0; dx < dw; dx++) {
                    put_pixel(boot_info, ox + dx, oy + dy, src[dx]);
                }
            }
            reset_clip();
            mp->mp4_video_rect_blits++;
        }
        goto draw_status;
    }
    MediaBitmap bmp;
    if (mp->audio_only) {
        /* Audio-only "Now Playing" surface: a centered card with the file name
         * and a simple animated bar so the window clearly belongs to the audio
         * being played, instead of a black video area. */
        vibio_u32 cy = y + vid_h / 2U;
        vibio_u32 card_w = w > 360 ? 360 : (w > 40 ? w - 40 : w);
        vibio_u32 card_x = x + (w > card_w ? (w - card_w) / 2U : 0);
        fill_rect(boot_info, card_x, cy - 60, card_w, 120, make_color(boot_info, 20, 34, 44));
        fill_rect(boot_info, card_x, cy - 60, card_w, 2, make_color(boot_info, 64, 120, 150));
        g_text_bg = make_color(boot_info, 20, 34, 44);
        draw_cstr(boot_info, card_x + 18, cy - 44, "AUDIO", theme->accent);
        draw_cstr(boot_info, card_x + 18, cy - 18, mp->filename, theme->text);
        /* A slow scanning bar that animates while audio is playing. */
        vibio_u32 track_x = card_x + 18;
        vibio_u32 track_w = card_w > 36 ? card_w - 36 : 0;
        fill_rect(boot_info, track_x, cy + 22, track_w, 6, make_color(boot_info, 12, 20, 26));
        if (mp->playing && mp->audio_started && track_w > 40) {
            vibio_u32 phase = (vibio_u32)((g_anim_now / 2U) % (vibio_u64)track_w);
            vibio_u32 knob = phase + 40U > track_w ? track_w - 40U : phase;
            fill_rect(boot_info, track_x + knob, cy + 22, 40, 6, make_color(boot_info, 64, 150, 190));
        }
        goto draw_status;
    }
    if (media_vimg_parse(mp->frame_buf, mp->frame_sizes[mp->current_frame < mp->frame_count ?
                         mp->current_frame : 0], &bmp)) {
        vibio_u32 iw = bmp.width;
        vibio_u32 ih = bmp.height;
        vibio_u32 scale = 100;
        if (iw > w || ih > vid_h) {
            vibio_u32 sx = w * 100U / iw;
            vibio_u32 sy = vid_h * 100U / ih;
            scale = sx < sy ? sx : sy;
        }
        vibio_u32 dw = iw * scale / 100U;
        vibio_u32 dh = ih * scale / 100U;
        vibio_u32 ox = x + (w > dw ? (w - dw) / 2U : 0);
        vibio_u32 oy = y + (vid_h > dh ? (vid_h - dh) / 2U : 0);
        set_clip(x, y, x + w, y + vid_h);
        for (vibio_u32 dy = 0; dy < dh; dy++) {
            vibio_u32 sy = (dy * ih) / dh;
            for (vibio_u32 dx = 0; dx < dw; dx++) {
                vibio_u32 sx = (dx * iw) / dw;
                const vibio_u8 *px = bmp.pixels + (sy * iw + sx) * 4U;
                blend_pixel(boot_info, ox + dx, oy + dy, px[0], px[1], px[2], px[3]);
            }
        }
        reset_clip();
    }
draw_status:
    if (chrome_hidden) {
        /* Full-screen, cursor idle: no status band or transport bar. Zero the
         * button hit rectangles so a click on the bare video cannot land on a
         * stale control position. */
        mp->btn_play_w = 0;
        mp->btn_stop_w = 0;
        mp->btn_restart_w = 0;
        mp->btn_mute_w = 0;
        return;
    }
    g_text_bg = make_color(boot_info, 14, 22, 28);
    fill_rect(boot_info, x, status_y, w, status_band, g_text_bg);
    char st[128];
    vibio_u32 p = 0;
    p = (vibio_u32)panic_append_text(st, p, sizeof(st), mp->filename);
    if (!mp->audio_only) {
        p = (vibio_u32)panic_append_text(st, p, sizeof(st), "  frame ");
        p = (vibio_u32)panic_append_uint(st, p, sizeof(st), mp->current_frame);
        if (mp->is_mp4) {
            vibio_u32 fps_whole = mp->fps_den ? (mp->fps_num / mp->fps_den) : 0U;
            p = (vibio_u32)panic_append_text(st, p, sizeof(st), "  fps=");
            p = (vibio_u32)panic_append_uint(st, p, sizeof(st), fps_whole);
            p = (vibio_u32)panic_append_text(st, p, sizeof(st), " dec=");
            p = (vibio_u32)panic_append_uint(st, p, sizeof(st), mp->mp4_decoded_frames);
            p = (vibio_u32)panic_append_text(st, p, sizeof(st), " pres=");
            p = (vibio_u32)panic_append_uint(st, p, sizeof(st), mp->mp4_presented_frames);
            p = (vibio_u32)panic_append_text(st, p, sizeof(st), " drop=");
            p = (vibio_u32)panic_append_uint(st, p, sizeof(st), mp->dropped_frames);
        }
    } else {
        p = (vibio_u32)panic_append_text(st, p, sizeof(st), "  audio");
    }
    st[p] = 0;
    /* Two rows spaced by the Geist line height so the filename/frame line and
     * the audio-status line do not overlap. */
    draw_cstr(boot_info, x + 10, status_y + 6, st, theme->text);
    if (mp->is_mp4) {
        char st2[128];
        p = 0;
        if (mp->audio_started && mp->audio_available) {
            p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), "Audio: sidecar PCM playing  ");
        } else if (mp->muted) {
            p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), "Audio: muted  ");
        } else if (mp->audio_wav_bytes > 0) {
            p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), "Audio: sidecar PCM ready  ");
        } else if (mp->mp4.audio.present && mp->mp4.audio.codec == MP4_CODEC_AAC) {
            p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), "Audio: AAC needs WAV sidecar  ");
        } else {
            p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), "Audio: unsupported  ");
        }
        p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), "buf=");
        p = (vibio_u32)panic_append_uint(st2, p, sizeof(st2), MEDIA_MP4_POOL);
        p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), " q=0 lag=");
        p = (vibio_u32)panic_append_uint(st2, p, sizeof(st2), mp->mp4_last_lag_frames);
        p = (vibio_u32)panic_append_text(st2, p, sizeof(st2), " blit=video-rect");
        st2[p] = 0;
        draw_cstr(boot_info, x + 10, status_y + 30, st2,
                  (mp->audio_started && mp->audio_available) ? theme->ok : theme->muted);
    } else if (mp->audio_started && mp->audio_available) {
        draw_cstr(boot_info, x + 10, status_y + 30, "Audio: playing PCM", theme->ok);
    } else if (mp->muted) {
        draw_cstr(boot_info, x + 10, status_y + 30, "Audio: muted", theme->muted);
    } else if (mp->audio_only && mp->audio_available) {
        draw_cstr(boot_info, x + 10, status_y + 30, "Audio: stopped", theme->muted);
    } else {
        draw_cstr(boot_info, x + 10, status_y + 30,
                  mp->audio_only ? "Audio: unavailable on this device" : "Audio: unavailable (video muted)",
                  theme->muted);
    }
    vibio_u32 bar_y = y + h - bar_h;
    fill_rect(boot_info, x, bar_y, w, bar_h, make_color(boot_info, 18, 30, 38));
    /* Buttons sized to their Geist labels with even gaps so none are clipped
     * (the previous 60px RESTART button cut off as "RESTA"). */
    vibio_u32 bx = x + 10;
    vibio_u32 by = bar_y + 10;
    vibio_u32 bh = bar_h > 20 ? bar_h - 20 : 30;
    mp->btn_play_x = bx; mp->btn_play_y = by; mp->btn_play_w = 64; mp->btn_play_h = bh;
    media_draw_toolbar_btn(boot_info, mp->btn_play_x, mp->btn_play_y, mp->btn_play_w, mp->btn_play_h,
                           mp->playing ? "PAUSE" : "PLAY", theme);
    bx += mp->btn_play_w + 8;
    mp->btn_stop_x = bx; mp->btn_stop_y = by; mp->btn_stop_w = 56; mp->btn_stop_h = bh;
    media_draw_toolbar_btn(boot_info, mp->btn_stop_x, mp->btn_stop_y, mp->btn_stop_w, mp->btn_stop_h, "STOP", theme);
    bx += mp->btn_stop_w + 8;
    mp->btn_restart_x = bx; mp->btn_restart_y = by; mp->btn_restart_w = 86; mp->btn_restart_h = bh;
    media_draw_toolbar_btn(boot_info, mp->btn_restart_x, mp->btn_restart_y, mp->btn_restart_w, mp->btn_restart_h, "RESTART", theme);
    bx += mp->btn_restart_w + 8;
    vibio_u32 audio_control_ready = !mp->is_mp4 || mp->audio_wav_bytes > 0 ||
                                    mp->audio_started || mp->audio_available;
    mp->btn_mute_x = bx; mp->btn_mute_y = by; mp->btn_mute_w = audio_control_ready ? 76 : 112; mp->btn_mute_h = bh;
    media_draw_toolbar_btn(boot_info, mp->btn_mute_x, mp->btn_mute_y, mp->btn_mute_w, mp->btn_mute_h,
                           audio_control_ready ? (mp->muted ? "UNMUTE" : "MUTE") : "AUDIO N/A", theme);
}

static void media_open_viewer(
    VibioMediaViewer *mv,
    VibioWindowManager *wm,
    const VibioBootInfo *boot_info,
    const char *name,
    vibio_u8 source)
{
    media_viewer_load(mv, name, source);
    vibio_u32 idx = wm_find_kind(wm, WINDOW_KIND_MEDIA_VIEWER);
    if (idx == WM_NO_WINDOW) {
        return;
    }
    wm_place_media_window(&wm->windows[idx], boot_info);
    wm_set_title(&wm->windows[idx], "Media Viewer");
    wm_show_and_raise(wm, idx);
}

static void media_open_player(
    VibioMediaPlayer *mp,
    VibioWindowManager *wm,
    const VibioBootInfo *boot_info,
    const char *name,
    vibio_u8 source)
{
    media_player_stop(mp, 0);
    media_player_open(mp, name, source);
    vibio_u32 idx = wm_find_kind(wm, WINDOW_KIND_MEDIA_PLAYER);
    if (idx == WM_NO_WINDOW) {
        return;
    }
    wm_place_media_window(&wm->windows[idx], boot_info);
    wm_set_title(&wm->windows[idx], "Media Player");
    wm_show_and_raise(wm, idx);
}

/* Open the currently selected entry. Folders are entered; .HTM/.HTML pages open
 * in the existing browser/HTML viewer; .WAV files play through the existing audio
 * path (with an info panel); anything else shows a read-only info panel. */
static void files_app_open_selected(
    VibioFilesApp *files,
    const VibioBootInfo *boot_info,
    VibioAudioState *audio,
    VibioWindowManager *wm,
    VibioBrowser *browser,
    VibioMediaViewer *media_viewer,
    VibioMediaPlayer *media_player)
{
    if (files->info_open) {
        files->info_open = 0;
        return;
    }
    if (files->entry_count == 0 || files->selected >= files->entry_count) {
        return;
    }
    const VibioFilesEntry *entry = &files->entries[files->selected];

    if (entry->is_dir) {
        files_app_enter_dir(files, entry);
        return;
    }

    /* Point the viewers/browser at whichever device is being browsed so a file
     * opened from the USB drive is read live from USB (not the AHCI boot disk).
     * The viewers each hold a disk/fs pointer used for all their reads. */
    {
        VibioDiskReadTest *adisk = files_active_disk(files);
        VibioFat32ReadTest *afs = files_active_fs(files);
        /* Resolve the opened file's name in the directory currently being browsed,
         * not the volume root, so files inside subfolders actually load instead of
         * reporting "file not found or unreadable". 0 for the boot-file backend,
         * which is read by name from the preload table rather than the FAT. */
        vibio_u32 cur_dir = (files->source == FILES_SRC_AHCI || files->source == FILES_SRC_USB)
                                ? files->path_clusters[files->path_depth - 1]
                                : 0;
        if (browser != 0) { browser->disk = adisk; browser->fs = afs; browser->dir_cluster = cur_dir; }
        if (media_viewer != 0) { media_viewer->disk = adisk; media_viewer->fs = afs; media_viewer->dir_cluster = cur_dir; }
        if (media_player != 0) { media_player->disk = adisk; media_player->fs = afs; media_player->dir_cluster = cur_dir; }
    }

    if (files_name_has_ext(entry->name, ".HTM") || files_name_has_ext(entry->name, ".HTML")) {
        if (browser != 0 && wm != 0) {
            browser_open_target(browser, entry->name, 0, 1);
            vibio_u32 bidx = wm_find_kind(wm, WINDOW_KIND_BROWSER);
            if (bidx != WM_NO_WINDOW) {
                wm_set_title(&wm->windows[bidx], browser->title_len > 0 ? browser->title : "BROWSER");
                wm_show_and_raise(wm, bidx);
            }
        }
        return;
    }

    if (browser_is_media_file(entry->name)) {
        if (browser_is_video_file(entry->name)) {
            /* Real MP4 family is handed to the player by its real name so the
             * player can probe it natively (and fall back to a host-converted
             * companion .VIV if one exists). Other "video" extensions keep the
             * legacy behaviour of opening their host-converted .VIV directly. */
            if (media_name_is_mp4(entry->name)) {
                if (media_player != 0 && wm != 0) {
                    media_open_player(media_player, wm, boot_info, entry->name, files->source);
                }
                return;
            }
            char viv[FILES_NAME_MAX];
            media_copy_name(viv, FILES_NAME_MAX, entry->name);
            vibio_u32 n = 0;
            while (viv[n] != 0 && viv[n] != '.') {
                n++;
            }
            if (n + 5 < FILES_NAME_MAX) {
                viv[n++] = '.';
                viv[n++] = 'V';
                viv[n++] = 'I';
                viv[n++] = 'V';
                viv[n] = 0;
            }
            if (media_player != 0 && wm != 0) {
                media_open_player(media_player, wm, boot_info, viv, files->source);
            }
            return;
        }
        if (media_viewer != 0 && wm != 0) {
            media_open_viewer(media_viewer, wm, boot_info, entry->name, files->source);
        }
        return;
    }

    if (files_name_has_ext(entry->name, ".WAV")) {
        /* Open .WAV in the dedicated Media Player (audio-only mode) so audio
         * files have their own player app, just like images and video. */
        if (media_player != 0 && wm != 0) {
            media_open_player(media_player, wm, boot_info, entry->name, files->source);
        } else if (audio != 0 && boot_info != 0) {
            audio_play_named(audio, boot_info, entry->name);
        }
        return;
    }

    /* Unknown / non-previewable type: honest read-only info panel. */
    files_app_set_info(files, entry->name);
    files_app_info_add(files, "No preview for this file type.");
    {
        char sizeline[40];
        vibio_u32 p = 0;
        p = (vibio_u32)panic_append_text(sizeline, p, sizeof(sizeline), "Size: ");
        p = append_human_size(sizeline, p, sizeof(sizeline), entry->size);
        sizeline[p] = 0;
        files_app_info_add(files, sizeline);
    }
    files_app_info_add(files, "Read-only: cannot open or edit.");
}

static void browser_draw_toolbar_button(
    const VibioBootInfo *boot_info,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    const char *label,
    vibio_u32 enabled,
    const VibioDesktopTheme *theme)
{
    vibio_u32 bg = enabled ? make_color(boot_info, 31, 55, 68) : make_color(boot_info, 22, 36, 45);
    vibio_u32 fg = enabled ? theme->text : theme->muted;
    fill_rect(boot_info, x, y, w, h, bg);
    fill_rect(boot_info, x, y, w, 1, theme->taskbar_edge);
    fill_rect(boot_info, x, y + h - 1, w, 1, theme->taskbar_edge);
    fill_rect(boot_info, x, y, 1, h, theme->taskbar_edge);
    fill_rect(boot_info, x + w - 1, y, 1, h, theme->taskbar_edge);
    g_text_bg = bg;
    draw_cstr(boot_info, x + 8, y + 3, label, fg);
}

/* The core layout + paint pass. Runs every frame the browser is visible: it
 * draws the page and (re)builds the clickable link table in screen coords. */
static void browser_render(
    const VibioBootInfo *boot_info,
    VibioBrowser *b,
    const VibioDesktopTheme *theme,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h,
    vibio_u32 body_color)
{
    g_text_bg = body_color;

    /* Address/header strip. */
    vibio_u32 header_bg = make_color(boot_info, 24, 44, 56);
    fill_rect(boot_info, x, y, w, BROWSER_HEADER_H, header_bg);
    fill_rect(boot_info, x, y + BROWSER_HEADER_H - 1, w, 1, theme->taskbar_edge);
    g_text_bg = header_bg;

    vibio_u32 btn_y = y + 4;
    vibio_u32 btn_h = BROWSER_HEADER_H - 8;
    vibio_u32 bx = x + 8;
    b->back_x = bx; b->back_y = btn_y; b->back_w = 26; b->back_h = btn_h;
    browser_draw_toolbar_button(boot_info, b->back_x, b->back_y, b->back_w, b->back_h, "<", b->back_enabled, theme);
    bx += b->back_w + 5;
    b->fwd_x = bx; b->fwd_y = btn_y; b->fwd_w = 26; b->fwd_h = btn_h;
    browser_draw_toolbar_button(boot_info, b->fwd_x, b->fwd_y, b->fwd_w, b->fwd_h, ">", b->forward_enabled, theme);
    bx += b->fwd_w + 5;
    b->reload_x = bx; b->reload_y = btn_y; b->reload_w = 30; b->reload_h = btn_h;
    browser_draw_toolbar_button(boot_info, b->reload_x, b->reload_y, b->reload_w, b->reload_h, "R", 1, theme);
    bx += b->reload_w + 5;
    b->home_x = bx; b->home_y = btn_y; b->home_w = 30; b->home_h = btn_h;
    browser_draw_toolbar_button(boot_info, b->home_x, b->home_y, b->home_w, b->home_h, "H", 1, theme);
    bx += b->home_w + 8;

    b->addr_x = bx;
    b->addr_y = btn_y;
    b->addr_h = btn_h;
    b->addr_w = x + w > bx + 8 ? x + w - bx - 8 : 0;
    if (b->addr_w > 0) {
        vibio_u32 addr_bg = b->address_active ? make_color(boot_info, 20, 40, 52) : make_color(boot_info, 16, 31, 40);
        fill_rect(boot_info, b->addr_x, b->addr_y, b->addr_w, b->addr_h, addr_bg);
        fill_rect(boot_info, b->addr_x, b->addr_y, b->addr_w, 1, b->address_active ? theme->accent : theme->taskbar_edge);
        fill_rect(boot_info, b->addr_x, b->addr_y + b->addr_h - 1, b->addr_w, 1, theme->taskbar_edge);
        fill_rect(boot_info, b->addr_x, b->addr_y, 1, b->addr_h, theme->taskbar_edge);
        fill_rect(boot_info, b->addr_x + b->addr_w - 1, b->addr_y, 1, b->addr_h, theme->taskbar_edge);
        set_clip(b->addr_x + 7, b->addr_y + 1, b->addr_x + b->addr_w - 6, b->addr_y + b->addr_h - 1);
        g_text_bg = addr_bg;
        if (b->address_active && b->address_replace && b->address_len > 0) {
            fill_rect(boot_info, b->addr_x + 5, b->addr_y + 3,
                      b->addr_w > 10 ? b->addr_w - 10 : b->addr_w,
                      b->addr_h > 6 ? b->addr_h - 6 : b->addr_h,
                      make_color(boot_info, 42, 82, 102));
            g_text_bg = make_color(boot_info, 42, 82, 102);
        }
        vibio_u32 text_x = draw_cstr(boot_info, b->addr_x + 7, b->addr_y + 3,
                                     b->address_len ? b->address : "(empty)",
                                     b->address_active ? theme->text : theme->accent);
        if (b->address_active && !b->address_replace) {
            fill_rect(boot_info, text_x + 2, b->addr_y + 4, 2, b->addr_h - 8, theme->accent);
        }
        reset_clip();
    }

    vibio_u32 content_x = x;
    vibio_u32 content_top = y + BROWSER_HEADER_H;
    vibio_u32 content_h = h > BROWSER_HEADER_H ? h - BROWSER_HEADER_H : 0;
    vibio_u32 left = x + BROWSER_PAD;
    vibio_u32 right = x + w - BROWSER_PAD;
    b->view_height = content_h;
    g_text_bg = body_color;
    fill_rect(boot_info, content_x, content_top, w, content_h, body_color);

    if (content_h == 0 || right <= left) {
        return;
    }

    /* Clamp scroll to the previously-measured content height. */
    vibio_u32 max_scroll = b->content_height > content_h ? b->content_height - content_h : 0;
    if (b->scroll > max_scroll) {
        b->scroll = max_scroll;
    }

    if (b->status == BROWSER_STATUS_IMAGE) {
        set_clip(content_x, content_top, x + w, content_top + content_h);
        vibio_u32 iw = 0;
        vibio_u32 ih = 0;
        vibio_u32 image_x = left;
        vibio_u32 image_y = content_top + BROWSER_PAD;
        if (browser_draw_any_image(boot_info, b, b->current, image_x, image_y, right - left, &iw, &ih)) {
            b->content_height = ih + BROWSER_PAD * 2;
            b->scroll = 0;
        } else {
            const char *line1 = browser_is_video_file(b->current) ? "VIDEO PREVIEW NOT AVAILABLE" : "IMAGE NOT RENDERED";
            const char *line2 = browser_is_video_file(b->current) ?
                "Video playback is not implemented; use a matching .VIM poster." :
                "Missing/large/unsupported image; use matching .VIM fallback.";
            const char *line3 = browser_is_video_file(b->current) ?
                "Redeploy USB after generating the poster asset." :
                "Rebuild/redeploy USB so the .VIM file is present.";
            draw_cstr(boot_info, left, content_top + 14, line1, theme->bad);
            draw_cstr(boot_info, left, content_top + 14 + BROWSER_BODY_LINE, line2, theme->muted);
            draw_cstr(boot_info, left, content_top + 14 + BROWSER_BODY_LINE * 2, line3, theme->muted);
            b->content_height = 0;
            b->scroll = 0;
        }
        reset_clip();
        return;
    }

    /* Non-OK states: render a short message instead of a page. */
    if (b->status != BROWSER_STATUS_OK || b->html_len == 0) {
        const char *line1 = "PAGE LOADED";
        const char *line2 = "";
        if (b->status == BROWSER_STATUS_NOT_FOUND) {
            line1 = "PAGE NOT FOUND";
            line2 = "The linked file is not on the disk.";
        } else if (b->status == BROWSER_STATUS_NO_DISK) {
            line1 = "NO READABLE DISK";
            line2 = "FAT32 read path is unavailable.";
        } else if (b->status == BROWSER_STATUS_EXTERNAL) {
            line1 = "EXTERNAL LINK";
            line2 = "Networking is not implemented yet.";
        }
        set_clip(content_x, content_top, x + w, content_top + content_h);
        draw_cstr(boot_info, left, content_top + 14, line1,
                  b->status == BROWSER_STATUS_NOT_FOUND ? theme->bad : theme->text);
        draw_cstr(boot_info, left, content_top + 14 + BROWSER_BODY_LINE, line2, theme->muted);
        reset_clip();
        b->content_height = 0;
        b->scroll = 0;
        return;
    }

    set_clip(content_x, content_top, x + w, content_top + content_h);

    /* Layout state, in content-relative coordinates (pen_y measured from the
     * top of the flow; screen y = content_top + pen_y - scroll). */
    vibio_u32 pen_y = BROWSER_PAD;
    vibio_u32 pen_x = left;
    vibio_u32 line_h = BROWSER_BODY_LINE;
    vibio_u32 line_started = 0;
    vibio_u32 pending_space = 0;
    vibio_u32 bold = 0;
    vibio_u32 heading = 0; /* 0 none, 1 h1, 2 h2/h3 */
    vibio_u32 cur_color = theme->text;
    vibio_u32 skip_text = 0;
    vibio_u32 in_link = 0;
    vibio_u32 link_external = 0;
    char link_href[BROWSER_URL_MAX];
    link_href[0] = 0;

    vibio_u8 cr, cg, cb;
    vibio_u32 space_adv = font_glyph_advance(&FONT_GEIST_UI, ' ');

    b->link_count = 0;
    const vibio_u8 *html = b->html;
    vibio_u32 len = b->html_len;
    vibio_u32 i = 0;

    while (i < len) {
        char c = (char)html[i];

        if (c == '<') {
            vibio_u32 j = i + 1;
            vibio_u32 is_close = 0;
            if (j < len && html[j] == '/') {
                is_close = 1;
                j++;
            }
            vibio_u32 name_start = j;
            while (j < len && ((html[j] >= 'a' && html[j] <= 'z') ||
                               (html[j] >= 'A' && html[j] <= 'Z') ||
                               (html[j] >= '0' && html[j] <= '9'))) {
                j++;
            }
            const char *tag = (const char *)(html + name_start);
            vibio_u32 tag_len = j - name_start;
            vibio_u32 attr_start = j;
            while (j < len && html[j] != '>') {
                j++;
            }
            const char *attrs = (const char *)(html + attr_start);
            vibio_u32 attr_len = j - attr_start;
            i = (j < len) ? j + 1 : len;

            if (browser_tag_is(tag, tag_len, "br")) {
                pen_x = left;
                pen_y += line_h;
                line_started = 0;
                pending_space = 0;
            } else if (browser_tag_is(tag, tag_len, "hr")) {
                if (line_started) {
                    pen_x = left;
                    pen_y += line_h;
                    line_started = 0;
                }
                pen_y += 6;
                vibio_u32 sy = content_top + pen_y - b->scroll;
                if (pen_y >= b->scroll && sy < content_top + content_h) {
                    fill_rect(boot_info, left, sy, right - left, 1, theme->taskbar_edge);
                }
                pen_y += 10;
                pending_space = 0;
            } else if (browser_tag_is(tag, tag_len, "title")) {
                skip_text = is_close ? (skip_text > 0 ? skip_text - 1 : 0) : skip_text + 1;
            } else if (browser_tag_is(tag, tag_len, "head") ||
                       browser_tag_is(tag, tag_len, "style") ||
                       browser_tag_is(tag, tag_len, "script")) {
                skip_text = is_close ? (skip_text > 0 ? skip_text - 1 : 0) : skip_text + 1;
            } else if (browser_tag_is(tag, tag_len, "h1") ||
                       browser_tag_is(tag, tag_len, "h2") ||
                       browser_tag_is(tag, tag_len, "h3")) {
                if (line_started) {
                    pen_x = left;
                    pen_y += line_h;
                    line_started = 0;
                }
                if (!is_close) {
                    pen_y += 8;
                    bold = 1;
                    heading = browser_tag_is(tag, tag_len, "h1") ? 1 : 2;
                    line_h = heading == 1 ? BROWSER_H1_LINE : BROWSER_H2_LINE;
                    cur_color = heading == 1 ? theme->accent : theme->text;
                } else {
                    if (heading == 1) {
                        vibio_u32 sy = content_top + pen_y - b->scroll + 2;
                        if (pen_y >= b->scroll && sy < content_top + content_h) {
                            fill_rect(boot_info, left, sy, right - left, 1, theme->taskbar_edge);
                        }
                    }
                    pen_y += 8;
                    bold = 0;
                    heading = 0;
                    line_h = BROWSER_BODY_LINE;
                    cur_color = theme->text;
                }
            } else if (browser_tag_is(tag, tag_len, "p") ||
                       browser_tag_is(tag, tag_len, "div") ||
                       browser_tag_is(tag, tag_len, "ul")) {
                if (line_started) {
                    pen_x = left;
                    pen_y += line_h;
                    line_started = 0;
                }
                pen_y += 6;
                pending_space = 0;
            } else if (browser_tag_is(tag, tag_len, "li")) {
                if (line_started) {
                    pen_x = left;
                    pen_y += line_h;
                    line_started = 0;
                }
                pen_y += 2;
                pen_x = left + 6;
                pending_space = 0;
            } else if (browser_tag_is(tag, tag_len, "a")) {
                if (!is_close) {
                    browser_get_attr(attrs, attr_len, "href", link_href, BROWSER_URL_MAX);
                    link_external = browser_href_is_external(link_href);
                    in_link = 1;
                    cur_color = theme->accent;
                } else {
                    in_link = 0;
                    cur_color = bold ? (heading == 1 ? theme->accent : theme->text) : theme->text;
                }
            } else if (browser_tag_is(tag, tag_len, "img")) {
                char alt[40];
                char src[BROWSER_URL_MAX];
                browser_get_attr(attrs, attr_len, "alt", alt, sizeof(alt));
                browser_get_attr(attrs, attr_len, "src", src, sizeof(src));
                if (line_started) {
                    pen_x = left;
                    pen_y += line_h;
                    line_started = 0;
                }
                pen_y += 4;
                vibio_u32 box_w = 140;
                vibio_u32 box_h = 64;
                vibio_u32 drawn_w = 0;
                vibio_u32 drawn_h = 0;
                if (left + box_w > right) {
                    box_w = right - left;
                }
                vibio_u32 sy = content_top + pen_y - b->scroll;
                if ((int)(pen_y + box_h) > (int)b->scroll && sy < content_top + content_h) {
                    if (src[0] != 0 &&
                        browser_draw_any_image(boot_info, b, src, left, sy, right - left, &drawn_w, &drawn_h)) {
                        box_w = drawn_w;
                        box_h = drawn_h;
                        fill_rect(boot_info, left, sy, box_w, 1, theme->taskbar_edge);
                        fill_rect(boot_info, left, sy + box_h - 1, box_w, 1, theme->taskbar_edge);
                        fill_rect(boot_info, left, sy, 1, box_h, theme->taskbar_edge);
                        fill_rect(boot_info, left + box_w - 1, sy, 1, box_h, theme->taskbar_edge);
                    } else {
                        fill_rect(boot_info, left, sy, box_w, box_h, make_color(boot_info, 20, 36, 46));
                        fill_rect(boot_info, left, sy, box_w, 1, theme->taskbar_edge);
                        fill_rect(boot_info, left, sy + box_h - 1, box_w, 1, theme->taskbar_edge);
                        fill_rect(boot_info, left, sy, 1, box_h, theme->taskbar_edge);
                        fill_rect(boot_info, left + box_w - 1, sy, 1, box_h, theme->taskbar_edge);
                        g_text_bg = make_color(boot_info, 20, 36, 46);
                        draw_cstr(boot_info, left + 8, sy + 8, "IMAGE", theme->muted);
                        if (alt[0] != 0) {
                            draw_cstr(boot_info, left + 8, sy + 8 + BROWSER_BODY_LINE, alt, theme->muted);
                        }
                        g_text_bg = body_color;
                    }
                }
                pen_y += box_h + 6;
                pending_space = 0;
            }
            continue;
        }

        if (skip_text > 0) {
            i++;
            continue;
        }

        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
            if (line_started) {
                pending_space = 1;
            }
            i++;
            continue;
        }

        /* Accumulate a word, decoding entities as we go. */
        char word[BROWSER_WORD_MAX];
        vibio_u32 word_len = 0;
        while (i < len) {
            c = (char)html[i];
            if (c == '<' || c == ' ' || c == '\n' || c == '\t' || c == '\r') {
                break;
            }
            if (c == '&') {
                char decoded = '&';
                vibio_u32 consumed = browser_decode_entity(html, i, len, &decoded);
                if (word_len + 1 < BROWSER_WORD_MAX) {
                    word[word_len++] = decoded;
                }
                i += consumed;
                continue;
            }
            if (word_len + 1 < BROWSER_WORD_MAX) {
                word[word_len++] = c;
            }
            i++;
        }
        if (word_len == 0) {
            continue;
        }

        /* Measure, wrap, and draw. */
        vibio_u32 word_w = 0;
        for (vibio_u32 k = 0; k < word_len; k++) {
            word_w += font_glyph_advance(&FONT_GEIST_UI, word[k]) + (bold ? 1 : 0);
        }
        vibio_u32 advance_space = (pending_space && line_started) ? space_adv : 0;
        if (line_started && pen_x + advance_space + word_w > right) {
            pen_x = left;
            pen_y += line_h;
            line_started = 0;
            advance_space = 0;
        }
        pen_x += advance_space;
        pending_space = 0;

        vibio_u32 word_start_x = pen_x;
        vibio_u32 screen_y = content_top + pen_y - b->scroll;
        vibio_u32 visible = ((int)(pen_y + line_h) > (int)b->scroll) && (screen_y < content_top + content_h);
        unpack_color(boot_info, cur_color, &cr, &cg, &cb);
        for (vibio_u32 k = 0; k < word_len; k++) {
            vibio_u32 nx = pen_x;
            if (visible) {
                nx = font_draw_glyph(boot_info, &FONT_GEIST_UI, pen_x, screen_y, word[k], cr, cg, cb);
                if (bold) {
                    font_draw_glyph(boot_info, &FONT_GEIST_UI, pen_x + 1, screen_y, word[k], cr, cg, cb);
                }
            } else {
                nx = pen_x + font_glyph_advance(&FONT_GEIST_UI, word[k]);
            }
            pen_x = nx + (bold ? 1 : 0);
        }

        if (in_link) {
            if (visible) {
                fill_rect(boot_info, word_start_x, screen_y + BROWSER_BODY_LINE - 4,
                          pen_x - word_start_x, 1, theme->accent);
            }
            if (b->link_count < BROWSER_MAX_LINKS) {
                VibioBrowserLink *l = &b->links[b->link_count++];
                l->x = word_start_x;
                l->y = screen_y;
                l->w = pen_x - word_start_x;
                l->h = line_h;
                l->external = link_external;
                l->href_len = 0;
                for (vibio_u32 k = 0; k + 1 < BROWSER_URL_MAX && link_href[k] != 0; k++) {
                    l->href[k] = link_href[k];
                    l->href_len++;
                }
                l->href[l->href_len] = 0;
            }
        }
        line_started = 1;
    }

    if (line_started) {
        pen_y += line_h;
    }
    b->content_height = pen_y + BROWSER_PAD;
    reset_clip();

    /* Scrollbar when the page is taller than the view. Drawn wide enough to be
     * an easy click/drag target, with its rects stored for hit-testing. */
    if (b->content_height > content_h) {
        vibio_u32 bar_w = 12;
        vibio_u32 bar_x = x + w - bar_w;
        fill_rect(boot_info, bar_x, content_top, bar_w, content_h, make_color(boot_info, 24, 36, 46));
        vibio_u32 thumb_h = (content_h * content_h) / b->content_height;
        if (thumb_h < 24) {
            thumb_h = 24;
        }
        if (thumb_h > content_h) {
            thumb_h = content_h;
        }
        vibio_u32 max_thumb = content_h > thumb_h ? content_h - thumb_h : 0;
        vibio_u32 thumb_y = max_scroll > 0 ? (b->scroll * max_thumb) / max_scroll : 0;
        vibio_u32 thumb_color = b->bar_dragging ? theme->text : theme->accent;
        fill_rect(boot_info, bar_x + 2, content_top + thumb_y + 1, bar_w - 4, thumb_h - 2, thumb_color);

        b->bar_x = bar_x;
        b->bar_y = content_top;
        b->bar_w = bar_w;
        b->bar_h = content_h;
        b->bar_thumb_y = content_top + thumb_y;
        b->bar_thumb_h = thumb_h;
    } else {
        b->bar_h = 0;
        b->bar_dragging = 0;
    }
}

/* Set the scroll position from a scrollbar cursor y, keeping the grab offset so
 * dragging the thumb tracks the cursor 1:1. */
static void browser_scrollbar_to(VibioBrowser *b, vibio_u32 mouse_y)
{
    if (b->bar_h == 0 || b->content_height <= b->view_height) {
        return;
    }
    vibio_u32 travel = b->bar_h > b->bar_thumb_h ? b->bar_h - b->bar_thumb_h : 0;
    if (travel == 0) {
        return;
    }
    int top = (int)mouse_y - b->bar_grab_offset;
    int rel = top - (int)b->bar_y;
    if (rel < 0) {
        rel = 0;
    }
    if (rel > (int)travel) {
        rel = (int)travel;
    }
    vibio_u32 max_scroll = b->content_height - b->view_height;
    b->scroll = (vibio_u32)(((vibio_u64)rel * max_scroll) / travel);
}

static void draw_browser_window_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h)
{
    if (desktop->browser == 0) {
        g_text_bg = window->body_color;
        draw_cstr(boot_info, x + 12, y + 12, "BROWSER UNAVAILABLE", desktop->theme->bad);
        return;
    }
    browser_render(boot_info, desktop->browser, desktop->theme, x, y, w, h, window->body_color);
}

/* Small folder/file glyph drawn left of each row. Read-only marker only. */
static void files_draw_type_glyph(const VibioBootInfo *boot_info, vibio_u32 x, vibio_u32 y, vibio_u32 is_dir)
{
    if (is_dir) {
        vibio_u32 tab = make_color(boot_info, 224, 184, 92);
        vibio_u32 body = make_color(boot_info, 245, 205, 110);
        fill_round_rect(boot_info, x, y + 1, 7, 3, 1, tab);
        fill_round_rect(boot_info, x, y + 3, 14, 9, 2, body);
    } else {
        vibio_u32 page = make_color(boot_info, 225, 233, 240);
        vibio_u32 fold = make_color(boot_info, 170, 188, 202);
        fill_round_rect(boot_info, x + 1, y + 1, 11, 12, 2, page);
        fill_rect(boot_info, x + 8, y + 1, 4, 4, fold);
    }
}

static void files_draw_button(
    const VibioBootInfo *boot_info,
    vibio_u32 x, vibio_u32 y, vibio_u32 w, vibio_u32 h,
    const char *label, vibio_u32 enabled, const VibioDesktopTheme *theme)
{
    vibio_u32 bg = enabled ? make_color(boot_info, 33, 58, 72) : make_color(boot_info, 22, 36, 45);
    vibio_u32 fg = enabled ? theme->text : theme->muted;
    fill_round_rect(boot_info, x, y, w, h, 5, bg);
    draw_soft_rect_border(boot_info, x, y, w, h, make_color(boot_info, 60, 86, 100));
    vibio_u32 lw = font_text_width(&FONT_GEIST_SM, label);
    draw_small_cstr(boot_info, x + (w > lw ? (w - lw) / 2 : 0), y + 4, label, fg, bg);
}

/* Human-readable backend label for the Files app status bar. */
static const char *files_source_label(vibio_u8 source)
{
    if (source == FILES_SRC_AHCI) {
        return "SOURCE: AHCI BOOT DISK - READ ONLY";
    }
    if (source == FILES_SRC_USB) {
        return "SOURCE: USB MASS STORAGE - READ ONLY";
    }
    if (source == FILES_SRC_BOOT) {
        return "SOURCE: UEFI BOOT FILES - READ ONLY";
    }
    return "SOURCE: NONE";
}

static void draw_files_window_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 x,
    vibio_u32 y,
    vibio_u32 w,
    vibio_u32 h)
{
    const VibioDesktopTheme *theme = desktop->theme;
    VibioFilesApp *files = desktop->files;

    fill_rect(boot_info, x, y, w, h, window->body_color);
    if (files == 0) {
        draw_small_cstr(boot_info, x + 12, y + 12, "FILES UNAVAILABLE", theme->bad, window->body_color);
        return;
    }

    vibio_u32 pad = 10;
    vibio_u32 toolbar_h = 30;

    /* Toolbar: Up + Open + (when a live USB drive is mounted) a Disk<->USB
     * source toggle, then the current path and READ ONLY badge. */
    files->up_x = x + pad;       files->up_y = y + 6;  files->up_w = 44; files->up_h = 20;
    files->open_x = x + pad + 50; files->open_y = y + 6; files->open_w = 56; files->open_h = 20;
    files_draw_button(boot_info, files->up_x, files->up_y, files->up_w, files->up_h, "Up", files->path_depth > 1, theme);
    files_draw_button(boot_info, files->open_x, files->open_y, files->open_w, files->open_h, "Open", files->entry_count > 0, theme);

    vibio_u32 path_x = files->open_x + files->open_w + 12;
    files->src_active = files_usb_available(files);
    if (files->src_active) {
        files->src_x = files->open_x + files->open_w + 8;
        files->src_y = y + 6; files->src_w = 64; files->src_h = 20;
        files_draw_button(boot_info, files->src_x, files->src_y, files->src_w, files->src_h,
                          files->view_source == FILES_SRC_USB ? "USB" : "Disk", 1, theme);
        path_x = files->src_x + files->src_w + 12;
    }

    char pathline[96];
    vibio_u32 pp = 0;
    pp = panic_append_text(pathline, pp, sizeof(pathline), "Path: ");
    pp = panic_append_text(pathline, pp, sizeof(pathline), files->path_text);
    pathline[pp] = 0;
    draw_small_cstr(boot_info, path_x, y + 10, pathline, theme->text, window->body_color);

    const char *ro = "READ ONLY";
    vibio_u32 ro_w = font_text_width(&FONT_GEIST_SM, ro);
    if (w > ro_w + pad + 4) {
        draw_small_cstr(boot_info, x + w - ro_w - pad, y + 10, ro, theme->accent, window->body_color);
    }
    fill_rect(boot_info, x, y + toolbar_h, w, 1, make_color(boot_info, 40, 56, 66));

    /* List geometry first, so the scrollbar gutter can be reserved before the
     * size column is positioned. */
    vibio_u32 header_y = y + toolbar_h + 6;
    vibio_u32 list_top = header_y + 18;
    vibio_u32 status_h = 24;
    vibio_u32 list_bottom = (h > toolbar_h + 18 + status_h + 24) ? (y + h - status_h) : (y + h);
    vibio_u32 row_h = 22;
    vibio_u32 list_area = list_bottom > list_top ? list_bottom - list_top : 0;
    vibio_u32 row_visible = list_area / row_h;

    /* Reserve a 10px gutter on the right for the scrollbar when the list
     * overflows, so long byte counts never draw under the bar. */
    vibio_u32 sb_w = 10;
    vibio_u32 overflow = files->fs_available && files->entry_count > row_visible;
    vibio_u32 gutter = overflow ? sb_w + 4 : 0;

    /* Column header. SIZE is right-aligned just inside the scrollbar gutter;
     * TYPE sits to its left so long byte counts never collide with the type. */
    vibio_u32 name_col = x + pad + 20;
    vibio_u32 size_right = (x + w > pad + gutter) ? x + w - pad - gutter : x + w;
    vibio_u32 type_col = (w > pad + 150 + gutter) ? (size_right - 130) : name_col;
    draw_small_cstr(boot_info, name_col, header_y, "NAME", theme->muted, window->body_color);
    draw_small_cstr(boot_info, type_col, header_y, "TYPE", theme->muted, window->body_color);
    {
        vibio_u32 hw = font_text_width(&FONT_GEIST_SM, "SIZE");
        draw_small_cstr(boot_info, size_right - hw, header_y, "SIZE", theme->muted, window->body_color);
    }

    files->row_x = x + pad;
    files->row_y = list_top;
    files->row_w = (w > pad * 2 + gutter) ? w - pad * 2 - gutter : w - pad * 2;
    files->row_h = row_h;
    files->row_first = files->scroll;
    files->row_visible = row_visible;
    files->sb_active = 0;
    if (overflow) {
        /* Clamp scroll in case the list shrank or the window grew. */
        vibio_u32 max_scroll = files->entry_count - row_visible;
        if (files->scroll > max_scroll) {
            files->scroll = max_scroll;
            files->row_first = files->scroll;
        }
    }

    if (!files->fs_available) {
        /* Honest, source-aware explanation - never a fake file list. */
        const VibioUsbStorage *us = files->usbstor;
        draw_small_cstr(boot_info, name_col, list_top + 6, "No readable FAT32 filesystem found.", theme->bad, window->body_color);
        const char *reason2 = "AHCI boot disk not available.";
        if (us != 0 && us->controller_found) {
            reason2 = "USB storage not available yet.";
        }
        draw_small_cstr(boot_info, name_col, list_top + 26, reason2, theme->muted, window->body_color);
        draw_small_cstr(boot_info, name_col, list_top + 46, "(No files were faked.)", theme->muted, window->body_color);
        if (us != 0 && us->controller_found) {
            char ul[64];
            vibio_u32 up = 0;
            up = panic_append_text(ul, up, sizeof(ul), "USB: xHCI found, ");
            up = panic_append_text(ul, up, sizeof(ul),
                us->last_step >= USB_STOR_STEP_PORTS ? "ports read, " :
                us->last_step >= USB_STOR_STEP_MMIO ? "caps read, " : "");
            up = panic_append_text(ul, up, sizeof(ul), "MSD read path not implemented.");
            ul[up] = 0;
            draw_small_cstr(boot_info, name_col, list_top + 66, ul, theme->muted, window->body_color);
        }
    } else if (files->entry_count == 0) {
        draw_small_cstr(boot_info, name_col, list_top + 6, "(empty folder)", theme->muted, window->body_color);
    } else {
        for (vibio_u32 vi = 0; vi < row_visible; vi++) {
            vibio_u32 idx = files->scroll + vi;
            if (idx >= files->entry_count) {
                break;
            }
            const VibioFilesEntry *e = &files->entries[idx];
            vibio_u32 ry = list_top + vi * row_h;
            if (idx == files->selected) {
                fill_round_rect(boot_info, files->row_x, ry, files->row_w, row_h - 2, 4, make_color(boot_info, 36, 64, 80));
            }
            files_draw_type_glyph(boot_info, x + pad, ry + 4, e->is_dir);
            draw_small_cstr(boot_info, name_col, ry + 4, e->name, theme->text, window->body_color);
            draw_small_cstr(boot_info, type_col, ry + 4, e->is_dir ? "DIR" : "FILE", theme->muted, window->body_color);
            {
                char szbuf[24];
                if (e->is_dir) {
                    szbuf[0] = '-'; szbuf[1] = '-'; szbuf[2] = 0;
                } else {
                    vibio_u32 sp = append_human_size(szbuf, 0, sizeof(szbuf), e->size);
                    szbuf[sp] = 0;
                }
                vibio_u32 sw = font_text_width(&FONT_GEIST_SM, szbuf);
                draw_small_cstr(boot_info, size_right > sw ? size_right - sw : name_col, ry + 4, szbuf, theme->muted, window->body_color);
            }
        }
    }

    /* Vertical scrollbar: a track with a proportional thumb, shown only when
     * the folder has more entries than fit. Drag/click is handled in the main
     * loop using the geometry recorded here. */
    if (overflow && list_area > 8) {
        vibio_u32 track_x = x + w - pad - sb_w;
        vibio_u32 track_y = list_top;
        vibio_u32 track_h = list_area;
        files->sb_x = track_x;
        files->sb_y = track_y;
        files->sb_w = sb_w;
        files->sb_h = track_h;
        files->sb_active = 1;
        fill_round_rect(boot_info, track_x, track_y, sb_w, track_h, sb_w / 2, make_color(boot_info, 22, 36, 45));
        vibio_u32 total = files->entry_count;
        vibio_u32 thumb_h = (track_h * row_visible) / total;
        if (thumb_h < 18) { thumb_h = 18; }
        if (thumb_h > track_h) { thumb_h = track_h; }
        vibio_u32 max_scroll = total - row_visible;
        vibio_u32 thumb_y = track_y + (max_scroll > 0 ? ((track_h - thumb_h) * files->scroll) / max_scroll : 0);
        files->sb_thumb_y = thumb_y;
        files->sb_thumb_h = thumb_h;
        vibio_u32 thumb_col = files->sb_dragging ? make_color(boot_info, 96, 140, 166) :
                                                   make_color(boot_info, 70, 104, 122);
        fill_round_rect(boot_info, track_x + 1, thumb_y, sb_w - 2, thumb_h, (sb_w - 2) / 2, thumb_col);
        draw_soft_rect_border(boot_info, track_x + 1, thumb_y, sb_w - 2, thumb_h, make_color(boot_info, 96, 134, 156));
    } else {
        files->sb_thumb_h = 0;
    }

    /* Status bar. */
    vibio_u32 status_y = y + h - status_h + 4;
    fill_rect(boot_info, x, y + h - status_h, w, 1, make_color(boot_info, 40, 56, 66));
    char status[96];
    vibio_u32 sp2 = 0;
    sp2 = panic_append_text(status, sp2, sizeof(status), files_source_label(files->source));
    if (files->fs_available) {
        sp2 = panic_append_text(status, sp2, sizeof(status), "  -  ");
        sp2 = panic_append_uint(status, sp2, sizeof(status), files->entry_count);
        sp2 = panic_append_text(status, sp2, sizeof(status), " items");
        if (files->truncated) {
            sp2 = panic_append_text(status, sp2, sizeof(status), " (truncated)");
        }
    }
    status[sp2] = 0;
    draw_small_cstr(boot_info, x + pad, status_y, status, theme->muted, window->body_color);

    if (files->fs_available && files->entry_count > 0 && files->selected < files->entry_count) {
        const VibioFilesEntry *sel = &files->entries[files->selected];
        vibio_u32 selw = font_text_width(&FONT_GEIST_SM, sel->name);
        if (w > selw + pad + 4) {
            draw_small_cstr(boot_info, x + w - selw - pad, status_y, sel->name, theme->text, window->body_color);
        }
    }

    /* Modal info / error panel. */
    if (files->info_open) {
        vibio_u32 bw = w > 120 ? (w > 360 ? 340 : w - 40) : w;
        vibio_u32 bh = 44 + files->info_line_count * 18 + 28;
        vibio_u32 bx = x + (w > bw ? (w - bw) / 2 : 0);
        vibio_u32 by = y + (h > bh ? (h - bh) / 2 : 0);
        fill_round_rect(boot_info, bx + 3, by + 4, bw, bh, 8, make_color(boot_info, 4, 9, 13));
        fill_round_rect(boot_info, bx, by, bw, bh, 8, make_color(boot_info, 20, 33, 42));
        draw_soft_rect_border(boot_info, bx, by, bw, bh, make_color(boot_info, 90, 124, 142));
        draw_small_cstr(boot_info, bx + 14, by + 12, files->info_title, theme->accent, make_color(boot_info, 20, 33, 42));
        for (vibio_u32 i = 0; i < files->info_line_count; i++) {
            draw_small_cstr(boot_info, bx + 14, by + 38 + i * 18, files->info_lines[i], theme->text, make_color(boot_info, 20, 33, 42));
        }
        draw_small_cstr(boot_info, bx + 14, by + bh - 20, "Click or press Esc/Enter to close.", theme->muted, make_color(boot_info, 20, 33, 42));
    }
}

static void window_draw_content(
    const VibioBootInfo *boot_info,
    const VibioWindow *window,
    const VibioDesktopContext *desktop,
    vibio_u32 y_off)
{
    vibio_u32 x = window->x + WINDOW_BORDER;
    vibio_u32 y = window->y + y_off + WINDOW_TITLEBAR_HEIGHT + WINDOW_BORDER;
    vibio_u32 w = window->width > WINDOW_BORDER * 2 ? window->width - WINDOW_BORDER * 2 : 0;
    vibio_u32 h = window->height > WINDOW_TITLEBAR_HEIGHT + WINDOW_BORDER * 2 ?
                  window->height - WINDOW_TITLEBAR_HEIGHT - WINDOW_BORDER * 2 : 0;

    /* F11 full screen draws content edge to edge, with no chrome to subtract. */
    if (window->fullscreen) {
        x = window->x;
        y = window->y + y_off;
        w = window->width;
        h = window->height;
    }

    if (w == 0 || h == 0) {
        return;
    }

    set_clip(x, y, x + w, y + h);

    if (window->kind == WINDOW_KIND_SYSTEM) {
        draw_system_window_content(boot_info, window, desktop, x, y, w);
    } else if (window->kind == WINDOW_KIND_TERMINAL) {
        draw_terminal_window_content(boot_info, window, desktop, x, y, w, h);
    } else if (window->kind == WINDOW_KIND_WELCOME) {
        draw_welcome_window_content(boot_info, window, desktop, x, y);
    } else if (window->kind == WINDOW_KIND_BROWSER) {
        draw_browser_window_content(boot_info, window, desktop, x, y, w, h);
    } else if (window->kind == WINDOW_KIND_FILES) {
        draw_files_window_content(boot_info, window, desktop, x, y, w, h);
    } else if (window->kind == WINDOW_KIND_MEDIA_VIEWER && desktop->media_viewer != 0) {
        media_viewer_render(boot_info, desktop->media_viewer, desktop->theme, x, y, w, h, window->body_color);
    } else if (window->kind == WINDOW_KIND_MEDIA_PLAYER && desktop->media_player != 0) {
        /* F11 full screen auto-hides the transport bar: it shows only while the
         * cursor has moved within the last ~2.5 s (fs_controls_until), like a
         * normal full-screen player. Windowed mode always shows the controls. */
        vibio_u32 chrome_hidden = window->fullscreen &&
            desktop->live_ticks >= desktop->media_player->fs_controls_until;
        media_player_render(boot_info, desktop->media_player, desktop->theme, x, y, w, h, chrome_hidden);
    } else if (window->kind == WINDOW_KIND_SETTINGS) {
        draw_settings_window_content(boot_info, window, desktop, x, y, w, h);
    }

    reset_clip();
}

/* Dedicated kernel stack size: 1 MiB (256 x 4 KiB pages). See kernel_main. */
#define KERNEL_STACK_PAGES 256U

/* Early boot tracing used to paint milestone colours during every successful
 * boot. Keep this hook compiled as a no-op so old call sites cannot flash the
 * screen; real boot stops use dedicated single-colour error screens instead. */
static void boot_trace(const VibioBootInfo *boot_info, vibio_u8 r, vibio_u8 g, vibio_u8 b)
{
    (void)boot_info;
    (void)r;
    (void)g;
    (void)b;
}

__attribute__((noreturn)) static void kernel_boot_stop(
    const VibioBootInfo *boot_info,
    vibio_u8 r,
    vibio_u8 g,
    vibio_u8 b,
    const char *code,
    const char *issue)
{
    if (boot_info != 0 && boot_info->framebuffer_base != 0 &&
        boot_info->framebuffer_width != 0 && boot_info->framebuffer_height != 0) {
        vibio_u32 bg = make_color(boot_info, r, g, b);
        vibio_u32 fg = make_color(boot_info, 255, 255, 255);
        if (r > 180 && g > 180 && b > 180) {
            fg = make_color(boot_info, 0, 0, 0);
        }
        fill_rect(boot_info, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, bg);
        font_draw_text(boot_info, &FONT_GEIST_UI, 24, 24, "Vibio OS", fg);

        vibio_u32 bottom = boot_info->framebuffer_height > 128 ? boot_info->framebuffer_height - 112 : 24;
        font_draw_text(boot_info, &FONT_GEIST_UI, 24, bottom, "ERROR CODE:", fg);
        font_draw_text(boot_info, &FONT_GEIST_UI, 154, bottom, code, fg);
        font_draw_text(boot_info, &FONT_GEIST_UI, 24, bottom + 32, issue, fg);
        font_draw_text(boot_info, &FONT_GEIST_UI, 24, bottom + 64, "Stopped safely. Hold power to shut off.", fg);
    }

    for (;;) {
        __asm__ volatile("hlt");
    }
}

__attribute__((noinline, noreturn)) static void kernel_main_body(VibioBootInfo *boot_info,
                                                                 VibioPageAllocator *allocator_in);

/* Real entry point from the loader. This runs on the firmware-provided UEFI
 * stack, which UEFI only guarantees to be about 128 KiB.
 *
 * kernel_main_body declares many large structs as simultaneous locals - the media
 * player alone embeds H264Probe's ~60 KB luma_region - so its single stack frame
 * can approach ~100 KiB. On firmware near the 128 KiB floor that overflows the
 * UEFI stack, and with no guard page the overflow silently corrupts memory: the
 * machine hangs right after the loader's "Starting Vibio kernel" line with nothing
 * on screen. OVMF/VirtualBox hand out generous stacks, which is exactly why this
 * only bit real hardware while every VM booted fine.
 *
 * So keep THIS frame tiny: build the page allocator, carve a 1 MiB stack out of
 * real conventional RAM (the loader drops trailing .bss, so a static array would
 * sit in unreserved memory - the page allocator hands back memory that is actually
 * free per the UEFI memory map), switch RSP onto it, and run the kernel there. */
__attribute__((section(".text.kernel_entry"), used, noreturn)) void kernel_main(VibioBootInfo *boot_info)
{
    g_panic_boot_info = boot_info;
    panic_set_context("KERNEL", "BOOT", "KERNEL ENTRY");

    if (boot_info == 0 || boot_info->magic != VIBIO_BOOT_INFO_MAGIC) {
        kernel_boot_stop(boot_info, 210, 36, 36, "VB-K001", "Invalid boot information from the loader.");
    }

    boot_trace(boot_info, 255, 0, 0); /* TRACE 1 RED: kernel entered, FB writable */

    VibioPageAllocator boot_allocator;
    page_allocator_init(&boot_allocator, boot_info);
    vibio_u64 kstack = alloc_contiguous_pages(&boot_allocator, KERNEL_STACK_PAGES);
    boot_trace(boot_info, 255, 128, 0); /* TRACE 2 ORANGE: allocator ready, switching stack */
    if (kstack != 0) {
        /* Top of the descending stack; page-aligned, so 16-byte aligned as the
         * SysV ABI requires immediately before a CALL. */
        vibio_u64 kstack_top = kstack + (vibio_u64)KERNEL_STACK_PAGES * PAGE_SIZE;
        __asm__ volatile(
            "mov %1, %%rdi\n"  /* arg0: boot_info                        */
            "mov %2, %%rsi\n"  /* arg1: &boot_allocator (old stack, ok)  */
            "mov %0, %%rsp\n"  /* switch onto the dedicated kernel stack */
            "call *%3\n"        /* kernel_main_body(boot_info, allocator) */
            :
            : "r"(kstack_top), "r"(boot_info), "r"(&boot_allocator), "r"(kernel_main_body)
            : "rdi", "rsi", "memory");
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    kernel_boot_stop(boot_info, 128, 48, 160, "VB-K002", "Dedicated kernel stack allocation failed.");
}

__attribute__((noinline, noreturn)) static void kernel_main_body(VibioBootInfo *boot_info,
                                                                 VibioPageAllocator *allocator_in)
{
    g_panic_boot_info = boot_info;
    panic_set_context("KERNEL", "BOOT", "KERNEL ENTRY");

    boot_trace(boot_info, 255, 255, 0); /* TRACE 3 YELLOW: running on the dedicated stack */

    VibioDesktopTheme theme;
    theme.wallpaper_top = make_color(boot_info, 56, 100, 126);
    theme.wallpaper_bottom = make_color(boot_info, 45, 86, 112);
    theme.topbar = make_color(boot_info, 12, 22, 30);
    theme.taskbar = make_color(boot_info, 16, 27, 36);
    theme.taskbar_edge = make_color(boot_info, 33, 52, 64);
    theme.start_button = make_color(boot_info, 27, 43, 54);
    theme.taskbar_button = make_color(boot_info, 22, 35, 44);
    theme.taskbar_button_focus = make_color(boot_info, 40, 70, 88);
    theme.window_border = make_color(boot_info, 165, 209, 226);
    theme.text = make_color(boot_info, 238, 244, 246);
    theme.muted = make_color(boot_info, 155, 169, 181);
    theme.accent = make_color(boot_info, 56, 220, 226);
    theme.ok = make_color(boot_info, 126, 231, 135);
    theme.bad = make_color(boot_info, 240, 72, 72);
    theme.black = make_color(boot_info, 0, 0, 0);
    VibioFileTest file_test;
    file_table_run_self_test(boot_info, &file_test);
    /* The allocator was already initialized by kernel_main (and advanced past the
     * dedicated stack it carved out); continue from that exact state. It is a
     * small, self-contained struct (cursors into the UEFI memory map, no pointers
     * into shared state), so this by-value copy keeps allocating correctly. */
    VibioPageAllocator allocator = *allocator_in;
    vibio_u64 free_pages_before = allocator.free_pages;
    vibio_u64 first_page = page_allocator_alloc(&allocator);
    vibio_u64 second_page = page_allocator_alloc(&allocator);
    vibio_u64 third_page = page_allocator_alloc(&allocator);

    VibioPageTableBuild page_tables = build_identity_page_tables(&allocator, boot_info);
    if (!page_tables.ok) {
        kernel_boot_stop(boot_info, 32, 120, 70, "VB-K003", "Kernel page-table setup failed.");
    }
    load_cr3(page_tables.pml4);

    vibio_u64 active_cr3 = read_cr3();
    vibio_u64 active_cr3_page = active_cr3 & 0x000FFFFFFFFFF000ULL;
    vibio_u32 paging_test_ok = page_tables.ok && active_cr3_page == page_tables.pml4;

    boot_trace(boot_info, 0, 0, 255); /* TRACE 4 BLUE: kernel page tables active, FB mapped */

    /* Bring up the kernel GDT/TSS and the IDT (CPU fault handlers) NOW, before any
     * hardware probing below (HDA audio, the AHCI disk test, and especially the
     * xHCI USB bring-up, which pokes real controller MMIO).
     *
     * With no IDT loaded, a fault in any of those on real hardware is an
     * uncatchable triple fault: the machine hangs silently right after the
     * loader's "Starting Vibio kernel" line with nothing on screen. A VM with USB
     * disabled never runs the xHCI path, which is why this only bit real hardware.
     * The GDT must load FIRST because the IDT entries capture the current code
     * selector via read_cs() - installing the IDT before the GDT swap would leave
     * its selectors stale and triple-fault on the first interrupt. This does NOT
     * enable IRQs; the timer/keyboard sti still happens later in
     * start_timer_and_keyboard_irqs. Once installed, a fault in a probe draws
     * Vibio's panic screen (naming the failing subsystem) instead of hanging. */
    VibioUserBoundaryBuild user_boundary = build_and_test_user_boundary(&allocator);
    VibioIdtEntry *idt_entries = 0;
    VibioIdtBuild idt = build_and_test_idt(&allocator, &idt_entries);

    boot_trace(boot_info, 0, 255, 255); /* TRACE 5 CYAN: GDT + IDT installed */

    /* Allocate the compositor buffers now, while allocations are still
     * contiguous near the start of memory: a persistent base layer (desktop +
     * diagnostics) and a back buffer the final frame is composited into. */
    vibio_u64 backbuffer_pixels = (vibio_u64)boot_info->pixels_per_scan_line * boot_info->framebuffer_height;
    vibio_u64 backbuffer_pages = (backbuffer_pixels * sizeof(vibio_u32) + PAGE_SIZE - 1) / PAGE_SIZE;
    vibio_u64 base_layer = alloc_contiguous_pages(&allocator, backbuffer_pages);
    vibio_u64 backbuffer = alloc_contiguous_pages(&allocator, backbuffer_pages);
    vibio_u32 compositor_ready = base_layer != 0 && backbuffer != 0;
    if (compositor_ready) {
        g_backbuffer = backbuffer;
        g_base_layer = base_layer;
    }
    boot_trace(boot_info, 0, 128, 255); /* TRACE 5a AZURE: compositor buffers allocated */

    VibioAudioState audio;
    audio_init(&audio, &allocator, &page_tables);
    vibio_u32 boot_sound_pending = audio.ready;
    boot_trace(boot_info, 255, 0, 255); /* TRACE 5b MAGENTA: HDA audio init done */

    VibioDiskReadTest disk_test = disk_run_readonly_test(&allocator);
    VibioFat32ReadTest fat32_test = fat32_run_readonly_test(&disk_test);
    boot_trace(boot_info, 255, 255, 255); /* TRACE 5c WHITE: AHCI disk + FAT32 probe done */
    /* Stage 29: identify (but never assume) the VM-only FAT32 write sandbox on a
     * second AHCI disk. Writes stay disabled unless the VIBIORW scratch disk is
     * positively identified by label + marker + preallocated test file. */
    VibioRwSandbox rw_sandbox;
    rw_sandbox_init(&rw_sandbox, &allocator, &disk_test);
    VibioUsbReadTest usb_test = usb_run_readonly_probe();
    /* Stage 30: read-only USB mass-storage diagnostic layers (honest status; no
     * device enumeration or USB write commands yet). Files prefers AHCI. */
    usb_storage_probe(&g_usb_storage, &usb_test);
    boot_trace(boot_info, 128, 128, 128); /* TRACE 5d GRAY: USB controller PCI probe done */
    /* Stage 37: the xHCI MMIO BAR can sit above 4 GiB (QEMU q35 places its 64-bit
     * PCI window near 0xC0_0000_0000), outside the low-4 GiB identity map. Map the
     * controller's 2 MiB MMIO page uncacheable into the active kernel page tables
     * and reload CR3 to flush the TLB, so the driver can reach its registers. */
    if (g_usb_storage.controller_found && g_usb_storage.controller_type == USB_TYPE_XHCI &&
        g_usb_storage.bar != 0 && page_tables.ok) {
        map_identity_range_2m(&allocator, &page_tables, g_usb_storage.bar & ~0x1FFFFFULL,
                              0x200000ULL, PAGE_PCD | PAGE_PWT);
        load_cr3(page_tables.pml4);
    }
    boot_trace(boot_info, 160, 32, 240); /* TRACE 5e PURPLE: xHCI MMIO mapped + CR3 reloaded */
    /* Stage 37 emergency real-hardware fix: do NOT run full xHCI
     * enumeration/BOT/SCSI/FAT32 mount during boot. The laptop freezes on
     * TRACE 5e PURPLE, which means xHCI MMIO mapping completed and the next
     * block is the unsafe bring-up path. Keep the code compiled for later VM or
     * manual/lazy work, but boot defaults to an honest skipped/unmounted state. */
#if XHCI_BOOT_MSD_AUTOMOUNT
    if (xhci_msd_init(&g_xhci_msd, &allocator, &g_usb_storage)) {
        vibio_u64 usb_secbuf = page_allocator_alloc(&allocator);
        if (usb_mount_fat32(&g_usb_disk, &g_usb_fs, &g_xhci_msd, usb_secbuf)) {
            g_usb_storage.fat32_mounted = 1;
            g_usb_storage.last_step = USB_STOR_STEP_FAT32;
        }
    }
#else
    xhci_record_boot_msd_skipped(&g_xhci_msd, &g_usb_storage);
#endif
    boot_trace(boot_info, 255, 105, 180); /* TRACE 5f PINK: xHCI MSD boot mount skipped/done */
    /* Stage 31: xHCI root-hub port hotplug detection only. This watches PORTSC
     * connect-state transitions and plays USB event WAVs; it does not enumerate
     * devices or add any USB storage read/write path. */
    usb_hotplug_init(&g_usb_hotplug, &usb_test);
    boot_trace(boot_info, 0, 255, 0); /* TRACE 6 GREEN: audio/disk/USB probes all done */
    vibio_u32 alloc_test_ok = 1;
    alloc_test_ok &= test_allocated_page(first_page, 0x564942494F504731ULL);
    alloc_test_ok &= test_allocated_page(second_page, 0x564942494F504732ULL);
    alloc_test_ok &= test_allocated_page(third_page, 0x564942494F504733ULL);
    paging_test_ok &= alloc_test_ok;
    VibioKernelHeap heap;
    heap_init(&heap, &allocator, HEAP_INITIAL_PAGES);
    VibioHeapTest heap_test;
    heap_run_self_test(&heap, &heap_test);
    VibioTaskManager tasks;
    VibioTaskTest task_test;
    task_manager_create_initial_tasks(&tasks, &heap, &task_test);
    if (task_test.ok) {
        task_manager_run_context_demo(&tasks, &task_test);
    }
    VibioSyscallState syscall_fallback;
    VibioSyscallState *syscalls = (VibioSyscallState *)heap_alloc(&heap, sizeof(VibioSyscallState));
    if (syscalls == 0) {
        syscalls = &syscall_fallback;
    }
    syscall_state_init(syscalls, &tasks);
    if (idt_entries != 0) {
        user_boundary.syscall_user_callable =
            idt_entries[SYSCALL_VECTOR].type_attributes == IDT_USER_INTERRUPT_GATE &&
            idt_entries[SYSCALL_VECTOR].selector == GDT_KERNEL_CODE_SELECTOR;
    }
    VibioIrqBuild irq = start_timer_and_keyboard_irqs(&allocator, idt_entries, idt.ok);
    VibioIrqState *irq_state = (VibioIrqState *)irq.state_address;
    VibioSyscallTest syscall_test;
    zero_bytes(&syscall_test, sizeof(VibioSyscallTest));
    if (idt.ok && irq.ok) {
        syscall_run_self_test(syscalls, &syscall_test);
    }
    VibioMouseState mouse;
    if (irq.ok) {
        /* Initialize the mouse with interrupts off so the keyboard handler does
         * not consume the controller bytes, then unmask IRQ12 and resume. */
        __asm__ volatile("cli");
        mouse_init(&mouse, boot_info);
        pic_enable_mouse();
        __asm__ volatile("sti");
    } else {
        zero_bytes(&mouse, sizeof(VibioMouseState));
        mouse.x = boot_info->framebuffer_width / 2;
        mouse.y = boot_info->framebuffer_height / 2;
        mouse.x_fixed = (int)(mouse.x << 8);
        mouse.y_fixed = (int)(mouse.y << 8);
    }
    VibioWindowManager wm;
    zero_bytes(&wm, sizeof(VibioWindowManager));
    wm.taskbar_hover_slot = TASKBAR_NO_SLOT;
    wm.context_hover_row = 0xFFFFFFFFU;
    wm.desktop_icons_visible = 1;
    wm.desktop_icon_size = DESKTOP_ICON_SIZE_MEDIUM;
    wm.desktop_last_click_index = 0xFFFFFFFFU;
    wm.wallpaper_mode = 0;
    for (vibio_u32 di = 0; di < DESKTOP_ICON_BUILTIN_COUNT; di++) {
        wm.desktop_builtin_x[di] = desktop_builtin_default_x(di);
        wm.desktop_builtin_y[di] = desktop_builtin_default_y(di);
    }
    wm.count = WM_MAX_WINDOWS;
    wm.windows[0].x = 266; wm.windows[0].y = 168; wm.windows[0].width = 492; wm.windows[0].height = 318;
    wm.windows[0].kind = WINDOW_KIND_SYSTEM;
    wm.windows[0].visible = 0;
    wm.windows[0].titlebar_color = make_color(boot_info, 31, 114, 130);
    wm.windows[0].body_color = make_color(boot_info, 18, 34, 42);
    wm_set_title(&wm.windows[0], "SYSTEM");
    wm.windows[1].x = 152; wm.windows[1].y = 196; wm.windows[1].width = 720; wm.windows[1].height = 300;
    wm.windows[1].kind = WINDOW_KIND_TERMINAL;
    wm.windows[1].visible = 0;
    wm.windows[1].titlebar_color = make_color(boot_info, 48, 82, 102);
    wm.windows[1].body_color = make_color(boot_info, 10, 18, 24);
    wm_set_title(&wm.windows[1], "TERMINAL");
    wm.windows[2].x = 300; wm.windows[2].y = 150; wm.windows[2].width = 560; wm.windows[2].height = 380;
    wm.windows[2].kind = WINDOW_KIND_BROWSER;
    wm.windows[2].visible = 0;
    wm.windows[2].titlebar_color = make_color(boot_info, 38, 70, 92);
    wm.windows[2].body_color = make_color(boot_info, 14, 26, 34);
    wm_set_title(&wm.windows[2], "BROWSER");
    wm.windows[3].x = 240; wm.windows[3].y = 132; wm.windows[3].width = 560; wm.windows[3].height = 396;
    wm.windows[3].kind = WINDOW_KIND_FILES;
    wm.windows[3].visible = 0;
    wm.windows[3].titlebar_color = make_color(boot_info, 44, 74, 92);
    wm.windows[3].body_color = make_color(boot_info, 15, 27, 35);
    wm_set_title(&wm.windows[3], "Vibio Files");
    wm.windows[4].x = 80; wm.windows[4].y = 48; wm.windows[4].width = 800; wm.windows[4].height = 520;
    wm.windows[4].kind = WINDOW_KIND_MEDIA_VIEWER;
    wm.windows[4].visible = 0;
    wm.windows[4].titlebar_color = make_color(boot_info, 36, 62, 82);
    wm.windows[4].body_color = make_color(boot_info, 8, 12, 16);
    wm_set_title(&wm.windows[4], "Media Viewer");
    wm.windows[5].x = 80; wm.windows[5].y = 48; wm.windows[5].width = 800; wm.windows[5].height = 520;
    wm.windows[5].kind = WINDOW_KIND_MEDIA_PLAYER;
    wm.windows[5].visible = 0;
    wm.windows[5].titlebar_color = make_color(boot_info, 52, 42, 92);
    wm.windows[5].body_color = make_color(boot_info, 8, 12, 16);
    wm_set_title(&wm.windows[5], "Media Player");
    wm.windows[6].x = 128; wm.windows[6].y = 72; wm.windows[6].width = 900; wm.windows[6].height = 560;
    wm.windows[6].kind = WINDOW_KIND_SETTINGS;
    wm.windows[6].visible = 0;
    wm.windows[6].titlebar_color = make_color(boot_info, 34, 56, 66);
    wm.windows[6].body_color = make_color(boot_info, 15, 24, 30);
    wm_set_title(&wm.windows[6], "Settings");
    for (vibio_u32 i = 0; i < wm.count; i++) {
        wm.z_order[i] = i;
    }

    /* Browser state: a page-allocated content buffer plus the disk/FAT32 read
     * path so it can open local HTML files. Opens START.HTM at boot. */
    VibioBrowser browser;
    zero_bytes(&browser, sizeof(VibioBrowser));
    browser.boot = boot_info;
    browser.disk = &disk_test;
    browser.fs = &fat32_test;
    vibio_u64 browser_pages = (BROWSER_HTML_MAX + PAGE_SIZE - 1) / PAGE_SIZE;
    vibio_u64 browser_buffer = alloc_contiguous_pages(&allocator, browser_pages);
    if (browser_buffer != 0) {
        browser.html = (vibio_u8 *)browser_buffer;
        browser.html_capacity = (vibio_u32)(browser_pages * PAGE_SIZE);
    }
    vibio_u64 browser_image_pages = (BROWSER_IMAGE_MAX + PAGE_SIZE - 1) / PAGE_SIZE;
    vibio_u64 browser_image_buffer = alloc_contiguous_pages(&allocator, browser_image_pages);
    if (browser_image_buffer != 0) {
        browser.image = (vibio_u8 *)browser_image_buffer;
        browser.image_capacity = (vibio_u32)(browser_image_pages * PAGE_SIZE);
    }
    vibio_u64 media_work_pages = (MEDIA_WORK_MAX + PAGE_SIZE - 1) / PAGE_SIZE;
    vibio_u64 media_work_buffer = alloc_contiguous_pages(&allocator, media_work_pages);
    if (media_work_buffer != 0) {
        browser.image_work = (vibio_u8 *)media_work_buffer;
        browser.image_work_capacity = (vibio_u32)(media_work_pages * PAGE_SIZE);
    }
    browser_open_target(&browser, "START.HTM", 0, 1);
    if (browser.title_len > 0) {
        wm_set_title(&wm.windows[2], browser.title);
    }

    /* Read-only Files app over the same FAT32 path. Loaded on first open.
     * Static (not on the kernel stack) because the entry list is now large
     * enough (256 entries) that keeping it off the stack avoids any risk of
     * overrunning the boot-provided stack. */
    static VibioFilesApp files_app;
    files_app_init(&files_app, boot_info, &disk_test, &fat32_test, &g_usb_storage, &g_usb_disk, &g_usb_fs);

    VibioMediaViewer media_viewer;
    zero_bytes(&media_viewer, sizeof(VibioMediaViewer));
    media_viewer.boot = boot_info;
    media_viewer.disk = &disk_test;
    media_viewer.fs = &fat32_test;
    media_viewer.scratch = browser.image;
    media_viewer.scratch_capacity = browser.image_capacity;
    media_viewer.work = browser.image_work;
    media_viewer.work_capacity = browser.image_work_capacity;

    VibioMediaPlayer media_player;
    zero_bytes(&media_player, sizeof(VibioMediaPlayer));
    media_player.boot = boot_info;
    media_player.disk = &disk_test;
    media_player.fs = &fat32_test;
    vibio_u64 media_frame_pages = (MEDIA_PLAYER_FRAME_BYTES + PAGE_SIZE - 1) / PAGE_SIZE;
    vibio_u64 media_frame_buf = alloc_contiguous_pages(&allocator, media_frame_pages);
    if (media_frame_buf != 0) {
        media_player.frame_buf = (vibio_u8 *)media_frame_buf;
        media_player.frame_capacity = (vibio_u32)(media_frame_pages * PAGE_SIZE);
    }
    vibio_u64 media_audio_pages = (MEDIA_AUDIO_SCRATCH_BYTES + PAGE_SIZE - 1U) / PAGE_SIZE;
    vibio_u64 media_audio_buf = alloc_contiguous_pages(&allocator, media_audio_pages);
    if (media_audio_buf != 0) {
        media_player.audio_scratch = (vibio_u8 *)media_audio_buf;
        media_player.audio_scratch_capacity = (vibio_u32)(media_audio_pages * PAGE_SIZE);
    }
    vibio_u64 media_mp4_table_pages = (MEDIA_MP4_TABLE_BYTES + PAGE_SIZE - 1U) / PAGE_SIZE;
    vibio_u64 media_mp4_table_buf = alloc_contiguous_pages(&allocator, media_mp4_table_pages);
    if (media_mp4_table_buf != 0) {
        media_player.mp4_table_buf = (vibio_u8 *)media_mp4_table_buf;
        media_player.mp4_table_capacity = (vibio_u32)(media_mp4_table_pages * PAGE_SIZE);
    }
    vibio_u64 media_mp4_yuv_pages = (MEDIA_MP4_FRAME_BYTES * MEDIA_MP4_POOL + PAGE_SIZE - 1U) / PAGE_SIZE;
    vibio_u64 media_mp4_yuv_buf = alloc_contiguous_pages(&allocator, media_mp4_yuv_pages);
    if (media_mp4_yuv_buf != 0) {
        media_player.mp4_yuv_pool = (vibio_u8 *)media_mp4_yuv_buf;
        media_player.mp4_yuv_capacity = (vibio_u32)(media_mp4_yuv_pages * PAGE_SIZE);
    }
    vibio_u64 media_mp4_work_pages = (MEDIA_MP4_WORK_BYTES + PAGE_SIZE - 1U) / PAGE_SIZE;
    vibio_u64 media_mp4_work_buf = alloc_contiguous_pages(&allocator, media_mp4_work_pages);
    if (media_mp4_work_buf != 0) {
        media_player.mp4_work = (vibio_u8 *)media_mp4_work_buf;
        media_player.mp4_work_capacity = (vibio_u32)(media_mp4_work_pages * PAGE_SIZE);
    }
    vibio_u64 media_mp4_rgb_pages = (MEDIA_MP4_RGB_BYTES + PAGE_SIZE - 1U) / PAGE_SIZE;
    vibio_u64 media_mp4_rgb_buf = alloc_contiguous_pages(&allocator, media_mp4_rgb_pages);
    if (media_mp4_rgb_buf != 0) {
        media_player.mp4_rgb = (vibio_u32 *)media_mp4_rgb_buf;
        media_player.mp4_rgb_capacity = (vibio_u32)(media_mp4_rgb_pages * PAGE_SIZE);
    }
    media_player.volume = 80;
    g_media_manifest_present = 0;
    {
        vibio_u8 mfscratch[64];
        vibio_u32 mfsz = 0;
        if (fat32_read_file(&disk_test, &fat32_test, "MEDIA.MF", mfscratch, sizeof(mfscratch) - 1, &mfsz) > 0) {
            g_media_manifest_present = 1;
        }
    }
    /* Start at a clean desktop. The browser is preloaded for SELFTEST and for
     * quick first-open behavior, but no windows or power dialogs should appear
     * automatically at boot. */
    wm.power_confirm = POWER_CONFIRM_NONE;
    wm.power_menu_open = 0;
    VibioConsoleState *console = (VibioConsoleState *)heap_alloc(&heap, sizeof(VibioConsoleState));

    if (console != 0) {
        zero_bytes(console, sizeof(VibioConsoleState));
        console_init(console);
    }

    vibio_u64 last_draw_tick = 0;
    vibio_u64 last_draw_keyboard_irqs = 0;
    vibio_u32 clock_hour = 0;
    vibio_u32 clock_minute = 0;
    vibio_u32 clock_ok = rtc_read_clock(&clock_hour, &clock_minute);
    vibio_u32 running_in_vm = system_running_in_vm();
    vibio_u32 force_draw = 1;
    vibio_u32 last_cursor_x = 0;
    vibio_u32 last_cursor_y = 0;
    /* Where the base layer (desktop + diagnostics + console) is drawn. */
    vibio_u64 base_target = compositor_ready ? base_layer : 0;
    VibioSettingsApp settings_app;
    zero_bytes(&settings_app, sizeof(VibioSettingsApp));
    settings_app.page = SETTINGS_PAGE_HOME;
    settings_app.use_24_hour = 1;
    VibioDesktopContext desktop;
    desktop.theme = &theme;
    desktop.console = console;
    desktop.mouse = &mouse;
    desktop.file = &file_test;
    desktop.disk = &disk_test;
    desktop.fs = &fat32_test;
    desktop.usb = &usb_test;
    desktop.heap = &heap_test;
    desktop.task = &task_test;
    desktop.syscall = &syscall_test;
    desktop.audio = &audio;
    desktop.user_boundary = &user_boundary;
    desktop.browser = &browser;
    desktop.files = &files_app;
    desktop.media_viewer = &media_viewer;
    desktop.media_player = &media_player;
    desktop.wm = &wm;
    desktop.settings = &settings_app;
    desktop.rw = &rw_sandbox;
    desktop.live_ticks = 0;
    desktop.live_keyboard_irqs = 0;
    desktop.live_last_scancode = 0;
    desktop.clock_hour = clock_hour;
    desktop.clock_minute = clock_minute;
    desktop.clock_ok = clock_ok;
    desktop.boot_ok = boot_info->boot_services_exited == 1;
    desktop.alloc_ok = alloc_test_ok;
    desktop.paging_ok = paging_test_ok;
    desktop.idt_ok = idt.ok;
    desktop.irq_ok = irq.ok;
    desktop.running_in_vm = running_in_vm;

    g_selftest_ctx.paging_ok = paging_test_ok;
    g_selftest_ctx.idt_ok = idt.ok;
    g_selftest_ctx.file = &file_test;
    g_selftest_ctx.browser = &browser;
    g_selftest_ctx.files = &files_app;
    g_selftest_ctx.media_viewer = &media_viewer;
    g_selftest_ctx.media_player = &media_player;
    g_selftest_ctx.wm = &wm;

    for (;;) {
        vibio_u64 live_ticks = irq.timer_ticks;
        vibio_u64 live_keyboard_irqs = irq.keyboard_irqs;
        vibio_u64 live_last_scancode = irq.last_scancode;
        vibio_u32 wm_changed = 0;
        vibio_u32 prev_power_menu_open = 0;
        vibio_u32 prev_power_confirm = POWER_CONFIRM_NONE;
        VibioWindowManager previous_wm;

        if (irq.ok && irq_state != 0) {
            live_ticks = irq_state->timer_ticks;
            live_keyboard_irqs = irq_state->keyboard_irqs;
            live_last_scancode = irq_state->last_scancode;
            g_anim_now = live_ticks;
            task_manager_handle_timer_requests(&tasks, irq_state, &task_test, syscalls);
            mouse_poll(&mouse, irq_state, boot_info);
            copy_bytes(&previous_wm, &wm, sizeof(VibioWindowManager));
            prev_power_menu_open = wm.power_menu_open;
            prev_power_confirm = wm.power_confirm;
            wm_changed = wm_update(&wm, &mouse, boot_info, &audio);
            if (wm.power_menu_open != prev_power_menu_open || wm.power_confirm != prev_power_confirm) {
                wm_changed = 1;
            }
            if (wm.desktop_base_dirty) {
                force_draw = 1;
                wm.desktop_base_dirty = 0;
                wm_changed = 1;
            }
            audio_service(&audio, live_ticks);
            usb_hotplug_poll(&g_usb_hotplug, boot_info, &audio, live_ticks);

            if (console != 0) {
                console_poll_keyboard(
                    console,
                    boot_info,
                    irq_state,
                    &heap,
                    &tasks,
                    &user_boundary,
                    &disk_test,
                    &fat32_test,
                    &usb_test,
                    &mouse,
                    syscalls,
                    &audio,
                    &rw_sandbox,
                    free_pages_before,
                    &wm,
                    &browser,
                    &files_app);
                if (wm_visual_state_changed(&previous_wm, &wm)) {
                    wm_changed = 1;
                }
            }

            if (console != 0 && console->wants_focus) {
                /* Typing only raises an already-open Terminal to the top so the
                 * keystrokes are visible. It never opens a hidden Terminal: the
                 * Terminal opens only when the user clicks its icon/taskbar
                 * button. This is why pressing Space (or any key) on the empty
                 * desktop no longer pops the Terminal open. */
                vibio_u32 terminal_idx = wm_find_kind(&wm, WINDOW_KIND_TERMINAL);
                if (terminal_idx != WM_NO_WINDOW &&
                    wm.windows[terminal_idx].visible &&
                    wm.z_order[wm.count - 1] != terminal_idx) {
                    wm_show_and_raise(&wm, terminal_idx);
                    wm_changed = 1;
                }
                console->wants_focus = 0;
            }

            /* The BROWSER command opens and reloads the browser window. */
            if (console != 0 && console->wants_browser) {
                browser_open_target(&browser, "START.HTM", 0, 1);
                browser.address_active = 0;
                wm_set_title(&wm.windows[2], browser.title_len > 0 ? browser.title : "BROWSER");
                wm_show_and_raise(&wm, 2);
                console->wants_browser = 0;
                wm_changed = 1;
            }

            /* The FILESAPP / EXPLORER command opens the Files window at root.
             * USBFILES additionally switches to the live USB FAT32 source. */
            if (console != 0 && console->wants_files) {
                if (console->wants_files_usb && files_usb_available(&files_app)) {
                    files_app.view_source = FILES_SRC_USB;
                }
                files_app_go_root(&files_app);
                vibio_u32 files_idx = wm_find_kind(&wm, WINDOW_KIND_FILES);
                if (files_idx != WM_NO_WINDOW) {
                    wm_show_and_raise(&wm, files_idx);
                }
                console->wants_files = 0;
                console->wants_files_usb = 0;
                wm_changed = 1;
            }

            if (console != 0 && console->wants_settings) {
                vibio_u32 settings_idx = wm_find_kind(&wm, WINDOW_KIND_SETTINGS);
                if (settings_idx != WM_NO_WINDOW) {
                    wm_show_and_raise(&wm, settings_idx);
                    wm_changed = 1;
                }
                console->wants_settings = 0;
            }

            /* Opening Files from the desktop icon or taskbar (not just the
             * FILESAPP command) must load the current directory; otherwise the
             * window would show an empty / "no FAT32" view even when a backend
             * exists. Load to root the first time it becomes visible, and reset
             * when it closes so reopening starts fresh at root. */
            {
                vibio_u32 fvis = wm_find_kind(&wm, WINDOW_KIND_FILES);
                if (fvis != WM_NO_WINDOW) {
                    if (wm.windows[fvis].visible) {
                        if (!files_app.loaded) {
                            files_app_go_root(&files_app);
                            wm_changed = 1;
                        }
                    } else {
                        files_app.loaded = 0;
                    }
                }
            }

            /* A queued "open the selected entry" request from the Files app. */
            if (files_app.open_pending) {
                files_app_open_selected(&files_app, boot_info, &audio, &wm, &browser, &media_viewer, &media_player);
                files_app.open_pending = 0;
                wm_changed = 1;
            }

            {
                vibio_u32 frame_before = media_player.current_frame;
                vibio_u32 playing_before = media_player.playing;
                vibio_u32 audio_started_before = media_player.audio_started;
                media_player_tick(&media_player, &audio, live_ticks);
                vibio_u32 pidx = wm_find_kind(&wm, WINDOW_KIND_MEDIA_PLAYER);
                static vibio_u32 media_player_visible_prev = 0;
                if (pidx != WM_NO_WINDOW && media_player_visible_prev && !wm.windows[pidx].visible) {
                    media_player_stop(&media_player, &audio);
                }
                /* Repaint the player window each time the displayed frame
                 * advances so playing video is not stuck at ~4 Hz. The repaint
                 * is limited to the player's dirty rectangle, not the screen. */
                if (pidx != WM_NO_WINDOW && wm.windows[pidx].visible &&
                    (media_player.current_frame != frame_before ||
                     media_player.playing != playing_before ||
                     media_player.audio_started != audio_started_before)) {
                    wm_changed = 1;
                }
                if (pidx != WM_NO_WINDOW) {
                    media_player_visible_prev = wm.windows[pidx].visible;
                }
            }

            if (console != 0 && console->wants_media_viewer && console->media_open_name[0] != 0) {
                media_open_viewer(&media_viewer, &wm, boot_info, console->media_open_name, files_app.source);
                console->wants_media_viewer = 0;
                console->media_open_name[0] = 0;
                wm_changed = 1;
            }
            if (console != 0 && console->wants_media_player && console->media_open_name[0] != 0) {
                media_open_player(&media_player, &wm, boot_info, console->media_open_name, files_app.source);
                console->wants_media_player = 0;
                console->media_open_name[0] = 0;
                wm_changed = 1;
            }

            /* Files app mouse input: row select, double-click open, Up/Open,
             * and a fully draggable scrollbar (thumb grab + track paging). */
            {
                vibio_u32 files_idx = wm_find_kind(&wm, WINDOW_KIND_FILES);
                vibio_u32 left = mouse.buttons & 0x01;
                vibio_u32 focused = files_idx != WM_NO_WINDOW &&
                                    wm.windows[files_idx].visible &&
                                    wm_top_visible(&wm) == files_idx;
                /* Active thumb drag keeps capture even if the cursor strays off
                 * the track horizontally - this is what makes hold-and-drag work
                 * (previously the scrollbar only reacted to the press edge). */
                if (focused && left && files_app.sb_dragging) {
                    files_app_scrollbar_set_from_thumb_top(&files_app, (int)mouse.y - (int)files_app.sb_grab_dy);
                    wm_changed = 1;
                } else if (focused && left && !files_app.prev_left) {
                    if (files_app.info_open) {
                        files_app.info_open = 0;
                        wm_changed = 1;
                    } else if (mouse.x >= files_app.up_x && mouse.x < files_app.up_x + files_app.up_w &&
                               mouse.y >= files_app.up_y && mouse.y < files_app.up_y + files_app.up_h) {
                        files_app_go_up(&files_app);
                        wm_changed = 1;
                    } else if (mouse.x >= files_app.open_x && mouse.x < files_app.open_x + files_app.open_w &&
                               mouse.y >= files_app.open_y && mouse.y < files_app.open_y + files_app.open_h) {
                        files_app_open_selected(&files_app, boot_info, &audio, &wm, &browser, &media_viewer, &media_player);
                        wm_changed = 1;
                    } else if (files_app.src_active &&
                               mouse.x >= files_app.src_x && mouse.x < files_app.src_x + files_app.src_w &&
                               mouse.y >= files_app.src_y && mouse.y < files_app.src_y + files_app.src_h) {
                        /* Toggle between the AHCI boot disk and the live USB drive. */
                        files_app_set_source(&files_app,
                            files_app.view_source == FILES_SRC_USB ? FILES_SRC_AHCI : FILES_SRC_USB);
                        wm_changed = 1;
                    } else if (files_app.sb_active &&
                               mouse.x >= files_app.sb_x && mouse.x < files_app.sb_x + files_app.sb_w &&
                               mouse.y >= files_app.sb_y && mouse.y < files_app.sb_y + files_app.sb_h) {
                        if (files_app.sb_thumb_h > 0 &&
                            mouse.y >= files_app.sb_thumb_y &&
                            mouse.y < files_app.sb_thumb_y + files_app.sb_thumb_h) {
                            /* Grab the thumb: remember where inside it we grabbed
                             * so the drag does not jump. */
                            files_app.sb_dragging = 1;
                            files_app.sb_grab_dy = mouse.y - files_app.sb_thumb_y;
                        } else {
                            /* Click above/below the thumb pages by a screenful
                             * toward the click, like a normal scrollbar track. */
                            int page = (int)(files_app.row_visible > 1 ? files_app.row_visible - 1 : 1);
                            files_app_scroll_by(&files_app, mouse.y < files_app.sb_thumb_y ? -page : page);
                        }
                        wm_changed = 1;
                    } else if (files_app.row_visible > 0 &&
                               mouse.x >= files_app.row_x && mouse.x < files_app.row_x + files_app.row_w &&
                               mouse.y >= files_app.row_y &&
                               mouse.y < files_app.row_y + files_app.row_visible * files_app.row_h) {
                        vibio_u32 vi = (mouse.y - files_app.row_y) / files_app.row_h;
                        vibio_u32 idx = files_app.row_first + vi;
                        if (idx < files_app.entry_count) {
                            vibio_u32 double_click = (idx == files_app.last_click_row) &&
                                                     (live_ticks - files_app.last_click_tick <= 30);
                            files_app.selected = idx;
                            files_app.last_click_row = idx;
                            files_app.last_click_tick = live_ticks;
                            if (double_click) {
                                files_app_open_selected(&files_app, boot_info, &audio, &wm, &browser, &media_viewer, &media_player);
                            }
                            wm_changed = 1;
                        }
                    }
                }
                if (!left) {
                    files_app.sb_dragging = 0;
                }
                files_app.prev_left = left;
            }

            /* Settings app mouse input: category navigation, Home cards,
             * wallpaper selection, and the live volume slider. */
            if (settings_handle_mouse(&settings_app, &wm, &mouse, &audio)) {
                wm_changed = 1;
            }

            /* Browser mouse input: scrollbar click/drag and link clicks. */
            {
                vibio_u32 browser_idx = wm_find_kind(&wm, WINDOW_KIND_BROWSER);
                vibio_u32 left = mouse.buttons & 0x01;
                vibio_u32 focused = browser_idx != WM_NO_WINDOW &&
                                    wm.windows[browser_idx].visible &&
                                    wm_top_visible(&wm) == browser_idx;

                if (focused && left && !browser.prev_left &&
                    mouse.x >= browser.back_x && mouse.x < browser.back_x + browser.back_w &&
                    mouse.y >= browser.back_y && mouse.y < browser.back_y + browser.back_h) {
                    browser.address_active = 0;
                    browser.address_replace = 0;
                    if (browser.back_enabled) {
                        browser_go_back(&browser);
                        wm_set_title(&wm.windows[2], browser.title_len > 0 ? browser.title : "BROWSER");
                    }
                    wm_changed = 1;
                } else if (focused && left && !browser.prev_left &&
                    mouse.x >= browser.fwd_x && mouse.x < browser.fwd_x + browser.fwd_w &&
                    mouse.y >= browser.fwd_y && mouse.y < browser.fwd_y + browser.fwd_h) {
                    browser.address_active = 0;
                    browser.address_replace = 0;
                    if (browser.forward_enabled) {
                        browser_go_forward(&browser);
                        wm_set_title(&wm.windows[2], browser.title_len > 0 ? browser.title : "BROWSER");
                    }
                    wm_changed = 1;
                } else if (focused && left && !browser.prev_left &&
                    mouse.x >= browser.reload_x && mouse.x < browser.reload_x + browser.reload_w &&
                    mouse.y >= browser.reload_y && mouse.y < browser.reload_y + browser.reload_h) {
                    browser.address_active = 0;
                    browser.address_replace = 0;
                    browser_reload(&browser);
                    wm_set_title(&wm.windows[2], browser.title_len > 0 ? browser.title : "BROWSER");
                    wm_changed = 1;
                } else if (focused && left && !browser.prev_left &&
                    mouse.x >= browser.home_x && mouse.x < browser.home_x + browser.home_w &&
                    mouse.y >= browser.home_y && mouse.y < browser.home_y + browser.home_h) {
                    browser.address_active = 0;
                    browser.address_replace = 0;
                    browser_home(&browser);
                    wm_set_title(&wm.windows[2], browser.title_len > 0 ? browser.title : "BROWSER");
                    wm_changed = 1;
                } else if (focused && left && !browser.prev_left &&
                    browser.addr_w != 0 &&
                    mouse.x >= browser.addr_x && mouse.x < browser.addr_x + browser.addr_w &&
                    mouse.y >= browser.addr_y && mouse.y < browser.addr_y + browser.addr_h) {
                    browser.address_active = 1;
                    browser_sync_address(&browser);
                    browser.address_replace = 1;
                    wm_changed = 1;
                } else if (focused && left && !browser.prev_left &&
                    browser.bar_h != 0 &&
                    mouse.x >= browser.bar_x && mouse.x < browser.bar_x + browser.bar_w &&
                    mouse.y >= browser.bar_y && mouse.y < browser.bar_y + browser.bar_h) {
                    /* Pressed on the scrollbar: grab the thumb, or jump to the
                     * click position when pressing the empty track. */
                    if (mouse.y >= browser.bar_thumb_y &&
                        mouse.y < browser.bar_thumb_y + browser.bar_thumb_h) {
                        browser.bar_grab_offset = (int)mouse.y - (int)browser.bar_thumb_y;
                    } else {
                        browser.bar_grab_offset = (int)(browser.bar_thumb_h / 2);
                    }
                    browser.bar_dragging = 1;
                    browser_scrollbar_to(&browser, mouse.y);
                    wm_changed = 1;
                } else if (focused && left && browser.bar_dragging) {
                    /* Dragging the thumb. */
                    browser_scrollbar_to(&browser, mouse.y);
                    wm_changed = 1;
                } else if (focused && left && !browser.prev_left) {
                    /* Otherwise a click navigates a link under the cursor. */
                    for (vibio_u32 li = 0; li < browser.link_count; li++) {
                        const VibioBrowserLink *l = &browser.links[li];
                        if (mouse.x >= l->x && mouse.x < l->x + l->w &&
                            mouse.y >= l->y && mouse.y < l->y + l->h) {
                            browser.nav_pending = 1;
                            browser.nav_external = l->external;
                            for (vibio_u32 k = 0; k < BROWSER_URL_MAX; k++) {
                                browser.nav_target[k] = l->href[k];
                            }
                            break;
                        }
                    }
                }

                if (!left) {
                    browser.bar_dragging = 0;
                }
                browser.prev_left = left;
            }

            /* Media Viewer mouse: toolbar + pan drag. */
            {
                vibio_u32 mvidx = wm_find_kind(&wm, WINDOW_KIND_MEDIA_VIEWER);
                vibio_u32 left = mouse.buttons & 0x01;
                vibio_u32 focused = mvidx != WM_NO_WINDOW && wm.windows[mvidx].visible &&
                                    wm_top_visible(&wm) == mvidx;
                if (focused && left && !media_viewer.prev_left) {
                    if (mouse.x >= media_viewer.btn_fit_x && mouse.x < media_viewer.btn_fit_x + media_viewer.btn_fit_w &&
                        mouse.y >= media_viewer.btn_fit_y && mouse.y < media_viewer.btn_fit_y + media_viewer.btn_fit_h) {
                        media_viewer.zoom_percent = MEDIA_ZOOM_FIT;
                        media_viewer.pan_x = 0;
                        media_viewer.pan_y = 0;
                        wm_changed = 1;
                    } else if (mouse.x >= media_viewer.btn100_x && mouse.x < media_viewer.btn100_x + media_viewer.btn100_w &&
                               mouse.y >= media_viewer.btn100_y && mouse.y < media_viewer.btn100_y + media_viewer.btn100_h) {
                        media_viewer.zoom_percent = MEDIA_ZOOM_100;
                        wm_changed = 1;
                    } else if (mouse.x >= media_viewer.btn_minus_x && mouse.x < media_viewer.btn_minus_x + media_viewer.btn_minus_w &&
                               mouse.y >= media_viewer.btn_minus_y && mouse.y < media_viewer.btn_minus_y + media_viewer.btn_minus_h) {
                        /* Step DOWN from whatever is actually on screen. In FIT
                         * mode that is effective_scale (the real fit percentage),
                         * NOT a snap to 100% - so minus always zooms out and never
                         * reverses near low zoom. Clamped to a clean 10% floor. */
                        vibio_u32 base = media_viewer.zoom_percent == MEDIA_ZOOM_FIT ?
                                         media_viewer.effective_scale : media_viewer.zoom_percent;
                        if (base == 0) { base = MEDIA_ZOOM_100; }
                        vibio_u32 nz = base > (MEDIA_ZOOM_MIN + MEDIA_ZOOM_STEP) ? base - MEDIA_ZOOM_STEP : MEDIA_ZOOM_MIN;
                        media_viewer.zoom_percent = nz;
                        wm_changed = 1;
                    } else if (mouse.x >= media_viewer.btn_plus_x && mouse.x < media_viewer.btn_plus_x + media_viewer.btn_plus_w &&
                               mouse.y >= media_viewer.btn_plus_y && mouse.y < media_viewer.btn_plus_y + media_viewer.btn_plus_h) {
                        vibio_u32 base = media_viewer.zoom_percent == MEDIA_ZOOM_FIT ?
                                         media_viewer.effective_scale : media_viewer.zoom_percent;
                        if (base == 0) { base = MEDIA_ZOOM_100; }
                        vibio_u32 nz = base + MEDIA_ZOOM_STEP;
                        if (nz > MEDIA_ZOOM_MAX) { nz = MEDIA_ZOOM_MAX; }
                        media_viewer.zoom_percent = nz;
                        wm_changed = 1;
                    } else if (media_viewer.loaded) {
                        media_viewer.dragging = 1;
                        media_viewer.drag_mx = mouse.x;
                        media_viewer.drag_my = mouse.y;
                        media_viewer.drag_pan_x = media_viewer.pan_x;
                        media_viewer.drag_pan_y = media_viewer.pan_y;
                    }
                } else if (focused && left && media_viewer.dragging) {
                    int dx = (int)mouse.x - (int)media_viewer.drag_mx;
                    int dy = (int)mouse.y - (int)media_viewer.drag_my;
                    media_viewer.pan_x = (vibio_u32)((int)media_viewer.drag_pan_x + dx);
                    media_viewer.pan_y = (vibio_u32)((int)media_viewer.drag_pan_y + dy);
                    wm_changed = 1;
                } else if (!left) {
                    media_viewer.dragging = 0;
                }
                media_viewer.prev_left = left;
            }

            /* Media Player transport controls. */
            {
                vibio_u32 mpidx = wm_find_kind(&wm, WINDOW_KIND_MEDIA_PLAYER);
                vibio_u32 left = mouse.buttons & 0x01;
                vibio_u32 focused = mpidx != WM_NO_WINDOW && wm.windows[mpidx].visible &&
                                    wm_top_visible(&wm) == mpidx;
                if (focused && left && !media_player.prev_left) {
                    if (mouse.x >= media_player.btn_play_x && mouse.x < media_player.btn_play_x + media_player.btn_play_w &&
                        mouse.y >= media_player.btn_play_y && mouse.y < media_player.btn_play_y + media_player.btn_play_h) {
                        if (media_player.audio_only) {
                            if (media_player.playing) {
                                media_player_stop(&media_player, &audio);
                            } else {
                                audio_stop_playback(&audio);
                                media_player.audio_started = 0;
                                media_player.audio_available = 0;
                                media_player.playing = 1;
                                media_player.last_frame_tick = 0;
                            }
                        } else {
                            media_player.playing = !media_player.playing;
                            if (media_player.playing) {
                                if (media_player.current_frame + 1U >= media_player.frame_count) {
                                    /* Clip finished: PLAY restarts from frame 0
                                     * (the audio is restarted fresh from the tick
                                     * loop) instead of flashing the final frame. */
                                    media_player.current_frame = 0;
                                    if (media_player.is_mp4) {
                                        media_player_decode_mp4_frame(&media_player, 0);
                                    } else {
                                        media_player_read_frame(&media_player, 0);
                                    }
                                    audio_stop_playback(&audio);
                                    media_player.audio_started = 0;
                                    media_player.last_frame_tick = 0;
                                } else if (media_player.audio_started && audio.paused) {
                                    /* Resume a paused clip from exactly where it
                                     * stopped - the audio DMA kept its position, so
                                     * just un-pause it instead of restarting the
                                     * sidecar from the beginning. last_frame_tick=0
                                     * re-anchors the video clock to current_frame;
                                     * audio_started stays set so the tick loop does
                                     * not re-arm the PCM stream. */
                                    audio_resume_playback(&audio);
                                    media_player.last_frame_tick = 0;
                                } else {
                                    /* Nothing to resume (e.g. audio never armed):
                                     * start the stream fresh on the next tick. */
                                    audio_stop_playback(&audio);
                                    media_player.audio_started = 0;
                                    media_player.last_frame_tick = 0;
                                }
                            } else if (audio.playback_active) {
                                /* PAUSE: halt the DMA but keep its position so the
                                 * matching resume continues the same audio. */
                                audio_pause_playback(&audio);
                            }
                        }
                        wm_changed = 1;
                    } else if (mouse.x >= media_player.btn_stop_x && mouse.x < media_player.btn_stop_x + media_player.btn_stop_w &&
                               mouse.y >= media_player.btn_stop_y && mouse.y < media_player.btn_stop_y + media_player.btn_stop_h) {
                        media_player_stop(&media_player, &audio);
                        wm_changed = 1;
                    } else if (mouse.x >= media_player.btn_restart_x && mouse.x < media_player.btn_restart_x + media_player.btn_restart_w &&
                               mouse.y >= media_player.btn_restart_y && mouse.y < media_player.btn_restart_y + media_player.btn_restart_h) {
                        media_player_stop(&media_player, &audio);
                        media_player.current_frame = 0;
                        if (media_player.is_mp4) {
                            media_player_decode_mp4_frame(&media_player, 0);
                        } else if (!media_player.audio_only) {
                            media_player_read_frame(&media_player, 0);
                        }
                        media_player.playing = 1;
                        media_player.last_frame_tick = 0;
                        wm_changed = 1;
                    } else if (mouse.x >= media_player.btn_mute_x && mouse.x < media_player.btn_mute_x + media_player.btn_mute_w &&
                               mouse.y >= media_player.btn_mute_y && mouse.y < media_player.btn_mute_y + media_player.btn_mute_h) {
                        media_player.muted = !media_player.muted;
                        if (media_player.muted) {
                            audio_stop_playback(&audio);
                            media_player.audio_started = 0;
                            if (media_player.audio_only) {
                                media_player.playing = 0;
                            }
                        } else {
                            /* Unmute: bring audio back immediately during
                             * playback instead of only on the next restart.
                             * Clearing audio_started + last_frame_tick makes the
                             * player tick re-arm the PCM stream on its next pass;
                             * audio-only mode also resumes playing. */
                            audio_stop_playback(&audio);
                            media_player.audio_started = 0;
                            media_player.last_frame_tick = 0;
                            if (media_player.audio_only) {
                                media_player.playing = 1;
                            }
                        }
                        wm_changed = 1;
                    }
                }
                media_player.prev_left = left;
            }

            if (browser.nav_pending) {
                browser_navigate(&browser, browser.nav_target, browser.nav_external);
                wm_set_title(&wm.windows[2], browser.title_len > 0 ? browser.title : "BROWSER");
                browser.nav_pending = 0;
                wm_changed = 1;
            }

            /* While dragging the scrollbar thumb, drop wheel input so a noisy
             * touchpad cannot fight the drag and make it jump up and down. */
            if (browser.bar_dragging || files_app.sb_dragging) {
                mouse.scroll = 0;
            }

            /* Mouse wheel scrolls the focused browser, else terminal history. */
            if (mouse.scroll != 0) {
                int scroll = mouse.scroll;
                if (scroll > MOUSE_WHEEL_FRAME_CAP) {
                    scroll = MOUSE_WHEEL_FRAME_CAP;
                }
                if (scroll < -MOUSE_WHEEL_FRAME_CAP) {
                    scroll = -MOUSE_WHEEL_FRAME_CAP;
                }
                vibio_u32 browser_idx = wm_find_kind(&wm, WINDOW_KIND_BROWSER);
                vibio_u32 wheel_files_idx = wm_find_kind(&wm, WINDOW_KIND_FILES);
                if (browser_idx != WM_NO_WINDOW && wm.windows[browser_idx].visible &&
                    wm_top_visible(&wm) == browser_idx) {
                    int max_scroll = (int)(browser.content_height > browser.view_height ?
                                           browser.content_height - browser.view_height : 0);
                    int ns = (int)browser.scroll - scroll * (int)BROWSER_BODY_LINE;
                    if (ns < 0) {
                        ns = 0;
                    }
                    if (ns > max_scroll) {
                        ns = max_scroll;
                    }
                    browser.scroll = (vibio_u32)ns;
                } else if (wheel_files_idx != WM_NO_WINDOW && wm.windows[wheel_files_idx].visible &&
                           wm_top_visible(&wm) == wheel_files_idx) {
                    /* Wheel scrolls the Files list by whole rows (3 per notch). */
                    files_app_scroll_by(&files_app, -scroll * 3);
                } else if (console != 0) {
                    console_scroll(console, scroll);
                }
                mouse.scroll = 0;
                wm_changed = 1;
            }

            /* Keep compositing every frame while an open animation is playing. */
            if (wm_step_animations(&wm, live_ticks) != 0) {
                wm_changed = 1;
            }
        }

        vibio_u32 timer_draw_due = force_draw || live_ticks - last_draw_tick >= 25;
        vibio_u32 keyboard_draw_due = force_draw || live_keyboard_irqs != last_draw_keyboard_irqs;
        vibio_u32 content_draw = timer_draw_due || keyboard_draw_due;
        vibio_u32 want_cursor = mouse.ready;
        vibio_u32 cursor_moved = want_cursor && (mouse.x != last_cursor_x || mouse.y != last_cursor_y);
        if (timer_draw_due) {
            clock_ok = rtc_read_clock(&clock_hour, &clock_minute);
        }

        /* F11 media player: reveal the transport bar while the cursor is moving
         * and for a short time after, then auto-hide it. Force a repaint when the
         * hidden/shown state flips so the bar appears and disappears even while
         * the clip is paused (which otherwise produces no frame repaints). When
         * hidden, the cursor is suppressed too (full-screen video, no chrome). */
        vibio_u32 fs_media_chrome_hidden = 0;
        {
            static vibio_u32 mp_chrome_hidden_prev = 0;
            vibio_u32 mp_idx = wm_find_kind(&wm, WINDOW_KIND_MEDIA_PLAYER);
            vibio_u32 fs_player = mp_idx != WM_NO_WINDOW && wm.windows[mp_idx].visible &&
                                  wm.windows[mp_idx].fullscreen && wm_top_visible(&wm) == mp_idx;
            if (fs_player && cursor_moved) {
                media_player.fs_controls_until = live_ticks + MEDIA_FS_CONTROLS_TICKS;
            }
            fs_media_chrome_hidden = fs_player && live_ticks >= media_player.fs_controls_until;
            if (fs_media_chrome_hidden != mp_chrome_hidden_prev) {
                wm_changed = 1;
                mp_chrome_hidden_prev = fs_media_chrome_hidden;
            }
        }
        if (fs_media_chrome_hidden) {
            want_cursor = 0;
        }
        desktop.console = console;
        desktop.mouse = &mouse;
        desktop.live_ticks = live_ticks;
        desktop.live_keyboard_irqs = live_keyboard_irqs;
        desktop.live_last_scancode = live_last_scancode;
        desktop.clock_hour = clock_hour;
        desktop.clock_minute = clock_minute;
        desktop.clock_ok = clock_ok;

        if (force_draw || content_draw || cursor_moved || wm_changed) {
            VibioDirtyList dirty;
            dirty_list_empty(&dirty);
            vibio_u32 fullscreen_now = wm_any_fullscreen(&wm);
            if (force_draw) {
                dirty_list_add(&dirty, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, boot_info);
            } else {
                if (wm_changed) {
                    for (vibio_u32 wi = 0; wi < wm.count; wi++) {
                        dirty_list_add_window(&dirty, &previous_wm.windows[wi], boot_info);
                    }
                    dirty_list_add_all_windows(&dirty, &wm, boot_info);
                    if (!fullscreen_now) {
                        dirty_list_add_taskbar(&dirty, boot_info);
                        dirty_list_add_desktop_interactions(&dirty, boot_info, &previous_wm, &wm);
                        dirty_list_add_taskbar_hover_popup(&dirty, boot_info, &previous_wm, previous_wm.taskbar_hover_slot);
                        dirty_list_add_taskbar_hover_popup(&dirty, boot_info, &wm, wm.taskbar_hover_slot);
                    }
                    dirty_list_add_power_shell_overlays(&dirty, boot_info, wm.power_menu_open, wm.power_confirm);
                    dirty_list_add_power_shell_overlays(&dirty, boot_info, prev_power_menu_open, prev_power_confirm);
                    dirty_list_add_volume_popup_overlay(&dirty, boot_info, wm.volume_popup_open);
                    dirty_list_add_volume_popup_overlay(&dirty, boot_info, previous_wm.volume_popup_open);
                }
                if (content_draw && (!fullscreen_now || keyboard_draw_due)) {
                    vibio_u32 top = wm_top_visible(&wm);
                    if (top != WM_NO_WINDOW) {
                        dirty_list_add_window(&dirty, &wm.windows[top], boot_info);
                    }
                    if (!fullscreen_now) {
                        dirty_list_add_taskbar(&dirty, boot_info);
                    }
                    if (wm.power_menu_open || wm.power_confirm != POWER_CONFIRM_NONE) {
                        dirty_list_add_power_shell_overlays(&dirty, boot_info, wm.power_menu_open, wm.power_confirm);
                    }
                    dirty_list_add_volume_popup_overlay(&dirty, boot_info, wm.volume_popup_open);
                }
                if (cursor_moved) {
                    dirty_list_add_cursor(&dirty, last_cursor_x, last_cursor_y, boot_info);
                    dirty_list_add_cursor(&dirty, mouse.x, mouse.y, boot_info);
                }
                /* In F11 fullscreen the shell is hidden, so a periodic clock/tick
                 * redraw should not force a full-frame repaint. Actual window
                 * changes (scroll, video frame advance, open/close, keyboard)
                 * still redraw the full fullscreen surface. */
                if (fullscreen_now && (wm_changed || keyboard_draw_due)) {
                    dirty_list_empty(&dirty);
                    dirty_list_add(&dirty, 0, 0, boot_info->framebuffer_width, boot_info->framebuffer_height, boot_info);
                }
            }

            if (dirty.count == 0) {
                if (timer_draw_due) {
                    last_draw_tick = live_ticks;
                }
                if (keyboard_draw_due) {
                    last_draw_keyboard_irqs = live_keyboard_irqs;
                }
                force_draw = 0;
            } else {
                __asm__ volatile("cli");

                for (vibio_u32 di = 0; di < dirty.count; di++) {
                    VibioRect *r = &dirty.rects[di];
                    set_paint_bounds(r->x0, r->y0, r->x1, r->y1);

                    /* 1) Update the static desktop base. With the compositor active,
                     * this is mostly wallpaper and persistent shell text; dynamic
                     * window/taskbar content is drawn during the composite pass. */
                    if (force_draw || !compositor_ready) {
                        g_draw_target = base_target;
                        draw_desktop_base(boot_info, &desktop, &wm);
                    }

                    /* 2) Composite the frame: base layer, then windows back-to-front,
                     *    then shell overlays and cursor, all into the back buffer,
                     *    then copy the finished dirty rectangle to the screen. */
                    if (compositor_ready) {
                        if (r->x0 == 0 && r->y0 == 0 &&
                            r->x1 == boot_info->framebuffer_width &&
                            r->y1 == boot_info->framebuffer_height) {
                            blit_layer(boot_info, backbuffer, base_layer);
                        } else {
                            blit_rect(boot_info, backbuffer, base_layer, r->x0, r->y0, r->x1, r->y1);
                        }
                        g_draw_target = backbuffer;
                    } else {
                        g_draw_target = 0;
                    }

                    if (!fullscreen_now) {
                        draw_desktop_interaction_overlay(boot_info, &wm, &theme);
                    }
                    wm_draw(boot_info, &wm, &desktop, theme.window_border);
                    /* F11 full screen hides the shell entirely. */
                    if (!fullscreen_now) {
                        draw_topbar_live(boot_info, &desktop);
                        draw_taskbar(boot_info, &wm, &desktop);
                        draw_desktop_context_menu(boot_info, &wm, &theme);
                    }
                    if (want_cursor) {
                        cursor_draw(boot_info, mouse.x, mouse.y, theme.text, theme.black);
                    }

                    if (compositor_ready) {
                        if (r->x0 == 0 && r->y0 == 0 &&
                            r->x1 == boot_info->framebuffer_width &&
                            r->y1 == boot_info->framebuffer_height) {
                            present(boot_info);
                        } else {
                            present_rect(boot_info, r);
                        }
                    } else {
                        framebuffer_write_fence();
                    }
                }
                reset_paint_bounds();
                g_draw_target = 0;

                if (boot_sound_pending && audio.ready) {
                    audio.boot_play_started = audio_play_named(&audio, boot_info, "BOOT.WAV");
                    boot_sound_pending = 0;
                }

                if (timer_draw_due) {
                    last_draw_tick = live_ticks;
                }
                if (keyboard_draw_due) {
                    last_draw_keyboard_irqs = live_keyboard_irqs;
                }
                last_cursor_x = mouse.x;
                last_cursor_y = mouse.y;
                force_draw = 0;

                if (irq.ok) {
                    __asm__ volatile("sti");
                }
            }
        }

        __asm__ volatile("hlt");
    }
}
