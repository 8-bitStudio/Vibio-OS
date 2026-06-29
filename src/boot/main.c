#include "boot_info.h"
#include "efi.h"

static EFI_GUID loaded_image_protocol_guid = {
    0x5B1B31A1,
    0x9562,
    0x11D2,
    {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B},
};

static EFI_GUID simple_file_system_protocol_guid = {
    0x0964E5B22,
    0x6459,
    0x11D2,
    {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B},
};

static EFI_GUID graphics_output_protocol_guid = {
    0x9042A9DE,
    0x23DC,
    0x4A38,
    {0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A},
};

/* ACPI 2.0+ root pointer (RSDP) GUID; ACPI 1.0 uses the legacy GUID below. */
static EFI_GUID acpi_20_table_guid = {
    0x8868E871,
    0xE4F1,
    0x11D3,
    {0xBC, 0x22, 0x00, 0x80, 0xC7, 0x3C, 0x88, 0x81},
};

static EFI_GUID acpi_10_table_guid = {
    0xEB9D2D30,
    0x2D88,
    0x11D3,
    {0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D},
};

typedef struct {
    EFI_GUID VendorGuid;
    void *VendorTable;
} VibioEfiConfigurationTable;

static int guid_equal(const EFI_GUID *a, const EFI_GUID *b)
{
    if (a->Data1 != b->Data1 || a->Data2 != b->Data2 || a->Data3 != b->Data3) {
        return 0;
    }
    for (int i = 0; i < 8; i++) {
        if (a->Data4[i] != b->Data4[i]) {
            return 0;
        }
    }
    return 1;
}

/* Find the ACPI RSDP in the UEFI configuration table so the kernel can parse
 * the FADT later for a real ACPI power-off. Prefers ACPI 2.0+, falls back to
 * ACPI 1.0. Read-only lookup; nothing is modified. */
static void find_acpi_rsdp(EFI_SYSTEM_TABLE *system_table, VibioBootInfo *boot_info)
{
    boot_info->acpi_rsdp = 0;
    VibioEfiConfigurationTable *tables = (VibioEfiConfigurationTable *)system_table->ConfigurationTable;
    if (tables == 0) {
        return;
    }
    void *legacy = 0;
    for (UINTN i = 0; i < system_table->NumberOfTableEntries; i++) {
        if (guid_equal(&tables[i].VendorGuid, &acpi_20_table_guid)) {
            boot_info->acpi_rsdp = (UINT64)tables[i].VendorTable;
            return;
        }
        if (guid_equal(&tables[i].VendorGuid, &acpi_10_table_guid)) {
            legacy = tables[i].VendorTable;
        }
    }
    boot_info->acpi_rsdp = (UINT64)legacy;
}

typedef void(__attribute__((sysv_abi)) *KernelEntry)(VibioBootInfo *boot_info);

typedef struct {
    CHAR16 *path;
    char *name;
    UINT32 type;
} VibioBootFileRequest;

static VibioBootFileRequest boot_file_requests[] = {
    {L"\\SOUNDS\\BOOT.WAV", "BOOT.WAV", VIBIO_BOOT_FILE_TYPE_SOUND},
    {L"\\SOUNDS\\ERROR.WAV", "ERROR.WAV", VIBIO_BOOT_FILE_TYPE_SOUND},
    {L"\\SOUNDS\\NOTIFY.WAV", "NOTIFY.WAV", VIBIO_BOOT_FILE_TYPE_SOUND},
    {L"\\SOUNDS\\USBINS.WAV", "USBINS.WAV", VIBIO_BOOT_FILE_TYPE_SOUND},
    {L"\\SOUNDS\\USBRM.WAV", "USBRM.WAV", VIBIO_BOOT_FILE_TYPE_SOUND},
    /* The browser home page, loaded read-only through firmware so it works even
     * on real hardware where the kernel's own AHCI/FAT32 driver has no device
     * (NVMe or USB boot). The kernel browser prefers this in-memory copy. */
    {L"\\START.HTM", "START.HTM", VIBIO_BOOT_FILE_TYPE_HTML},
    {L"\\TEST.PNG", "TEST.PNG", VIBIO_BOOT_FILE_TYPE_IMAGE},
    {L"\\TEST.VIM", "TEST.VIM", VIBIO_BOOT_FILE_TYPE_IMAGE},
};

static void zero_memory(void *buffer, UINTN size)
{
    UINT8 *bytes = (UINT8 *)buffer;

    for (UINTN i = 0; i < size; i++) {
        bytes[i] = 0;
    }
}

static void print(EFI_SYSTEM_TABLE *system_table, CHAR16 *message)
{
    system_table->ConOut->OutputString(system_table->ConOut, message);
}

static void set_color(EFI_SYSTEM_TABLE *system_table, UINTN color)
{
    system_table->ConOut->SetAttribute(system_table->ConOut, color | EFI_BACKGROUND_BLACK);
}

static void set_text_attribute(EFI_SYSTEM_TABLE *system_table, UINTN foreground, UINTN background)
{
    system_table->ConOut->SetAttribute(system_table->ConOut, foreground | background);
}

static void print_hex(EFI_SYSTEM_TABLE *system_table, UINT64 value)
{
    print(system_table, L"0x");

    int started = 0;
    for (int shift = 60; shift >= 0; shift -= 4) {
        UINT8 nibble = (UINT8)((value >> shift) & 0xF);
        if (nibble != 0 || started || shift == 0) {
            started = 1;
            CHAR16 digit[2];
            digit[0] = nibble < 10 ? (CHAR16)(L'0' + nibble) : (CHAR16)(L'A' + (nibble - 10));
            digit[1] = 0;
            print(system_table, digit);
        }
    }
}

/*
 * Stop the machine safely instead of returning an error to the firmware.
 *
 * Returning an error from an EFI application makes the firmware move on to the
 * next boot option, which on a real laptop is usually the internal Windows
 * install. That is how an early failure here could drop the user into Windows
 * recovery. Halting in place keeps Vibio on screen and never touches any disk,
 * so a boot problem can never chain into the host operating system.
 */
__attribute__((noreturn)) static void boot_fail(
    EFI_SYSTEM_TABLE *system_table,
    UINTN foreground,
    UINTN background,
    CHAR16 *code,
    CHAR16 *message,
    EFI_STATUS status)
{
    UINTN columns = 80;
    UINTN rows = 25;
    if (system_table->ConOut->Mode != 0) {
        UINTN queried_columns = 0;
        UINTN queried_rows = 0;
        EFI_STATUS mode_status = system_table->ConOut->QueryMode(
            system_table->ConOut,
            (UINTN)system_table->ConOut->Mode->Mode,
            &queried_columns,
            &queried_rows);
        if (!EFI_ERROR(mode_status) && queried_columns > 0 && queried_rows > 0) {
            columns = queried_columns;
            rows = queried_rows;
        }
    }

    set_text_attribute(system_table, foreground, background);
    system_table->ConOut->ClearScreen(system_table->ConOut);
    system_table->ConOut->SetCursorPosition(system_table->ConOut, 0, 0);

    print(system_table, L"Vibio OS\r\n");

    UINTN bottom = rows > 4 ? rows - 4 : 0;
    system_table->ConOut->SetCursorPosition(system_table->ConOut, 0, bottom);
    print(system_table, L"ERROR CODE: ");
    print(system_table, code);
    print(system_table, L"\r\n");
    print(system_table, L"ISSUE: ");
    print(system_table, message);
    print(system_table, L"\r\n");
    print(system_table, L"EFI STATUS: ");
    print_hex(system_table, status);
    print(system_table, L"\r\n");
    if (columns >= 39) {
        print(system_table, L"Stopped safely. Hold power to shut off.\r\n");
    } else {
        print(system_table, L"Hold power to shut off.\r\n");
    }

    system_table->BootServices->SetWatchdogTimer(0, 0, 0, 0);
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static EFI_STATUS open_root_volume(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table,
    EFI_FILE_PROTOCOL **root)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_STATUS status = system_table->BootServices->HandleProtocol(
        image_handle,
        &loaded_image_protocol_guid,
        (void **)&loaded_image);

    if (EFI_ERROR(status)) {
        return status;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *file_system = NULL;
    status = system_table->BootServices->HandleProtocol(
        loaded_image->DeviceHandle,
        &simple_file_system_protocol_guid,
        (void **)&file_system);

    if (EFI_ERROR(status)) {
        return status;
    }

    return file_system->OpenVolume(file_system, root);
}

static UINT64 query_file_size(EFI_FILE_PROTOCOL *file);

static EFI_STATUS load_kernel(
    EFI_SYSTEM_TABLE *system_table,
    EFI_FILE_PROTOCOL *root,
    UINT64 *kernel_address,
    UINTN *kernel_size)
{
    EFI_FILE_PROTOCOL *kernel_file = NULL;
    EFI_STATUS status = root->Open(
        root,
        &kernel_file,
        L"\\KERNEL.BIN",
        EFI_FILE_MODE_READ,
        0);

    if (EFI_ERROR(status)) {
        return status;
    }

    /* The kernel must load at its linked address (0x100000), so the allocation
     * is AT a fixed address. Reserve only the kernel's actual size (rounded up,
     * plus a few guard pages) instead of a fixed multi-megabyte block: firmware
     * such as OVMF rejects a fixed 16 MiB reservation at 0x100000, while the
     * exact ~4 MiB fits. Fall back to a modest 6 MiB reservation if GetInfo is
     * unavailable. VIBIO_KERNEL_MAX_SIZE remains the upper safety cap. */
    *kernel_address = VIBIO_KERNEL_LOAD_ADDRESS;
    UINT64 exact = query_file_size(kernel_file);
    UINTN want_bytes;
    if (exact > 0 && exact <= VIBIO_KERNEL_MAX_SIZE) {
        want_bytes = (UINTN)exact + 0x4000; /* +16 KiB guard */
    } else {
        want_bytes = 6ULL * 1024ULL * 1024ULL;
        if (want_bytes > VIBIO_KERNEL_MAX_SIZE) {
            want_bytes = (UINTN)VIBIO_KERNEL_MAX_SIZE;
        }
    }
    UINTN pages = (want_bytes + 0xFFF) / 0x1000;
    UINTN read_cap = pages * 0x1000;
    status = system_table->BootServices->AllocatePages(
        EFI_ALLOCATE_ADDRESS,
        EFI_LOADER_DATA,
        pages,
        kernel_address);

    if (EFI_ERROR(status)) {
        kernel_file->Close(kernel_file);
        return status;
    }

    *kernel_size = 0;
    while (*kernel_size < read_cap) {
        UINTN requested = 64 * 1024;
        UINTN remaining = read_cap - *kernel_size;
        if (requested > remaining) {
            requested = remaining;
        }

        UINTN bytes_read = requested;
        status = kernel_file->Read(
            kernel_file,
            &bytes_read,
            (void *)(*kernel_address + *kernel_size));

        if (EFI_ERROR(status)) {
            kernel_file->Close(kernel_file);
            return status;
        }

        if (bytes_read == 0) {
            break;
        }

        *kernel_size += bytes_read;
    }

    kernel_file->Close(kernel_file);

    return status;
}

static EFI_STATUS allocate_loader_buffer(EFI_SYSTEM_TABLE *system_table, UINTN size, void **buffer)
{
    UINT64 address = 0xFFFFFFFFULL;
    UINTN pages = (size + 0xFFF) / 0x1000;
    EFI_STATUS status = system_table->BootServices->AllocatePages(
        EFI_ALLOCATE_MAX_ADDRESS,
        EFI_LOADER_DATA,
        pages,
        &address);

    if (EFI_ERROR(status)) {
        return status;
    }

    *buffer = (void *)address;
    zero_memory(*buffer, pages * 0x1000);

    return EFI_SUCCESS;
}

static void set_boot_file_name(VibioBootFile *file, char *name)
{
    file->name_length = 0;
    for (UINTN i = 0; i < VIBIO_BOOT_FILE_NAME_MAX; i++) {
        file->name[i] = 0;
    }

    while (name[file->name_length] != 0 && file->name_length < VIBIO_BOOT_FILE_NAME_MAX) {
        file->name[file->name_length] = name[file->name_length];
        file->name_length++;
    }
}

/* EFI_FILE_INFO GUID {09576E92-6D3F-11D2-8E39-00A0C969723B}. */
static EFI_GUID g_efi_file_info_guid = {
    0x09576E92, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
};

typedef EFI_STATUS(EFIAPI *EFI_FILE_GET_INFO)(
    EFI_FILE_PROTOCOL *This, EFI_GUID *InformationType, UINTN *BufferSize, void *Buffer);

/* Ask the firmware for the exact file size. Returns 0 if unavailable. */
static UINT64 query_file_size(EFI_FILE_PROTOCOL *file)
{
    if (file->GetInfo == 0) {
        return 0;
    }
    UINT8 info_buf[512];
    UINTN info_size = sizeof(info_buf);
    EFI_FILE_GET_INFO get_info = (EFI_FILE_GET_INFO)file->GetInfo;
    EFI_STATUS status = get_info(file, &g_efi_file_info_guid, &info_size, info_buf);
    if (EFI_ERROR(status)) {
        return 0;
    }
    EFI_FILE_INFO *info = (EFI_FILE_INFO *)info_buf;
    return info->FileSize;
}

static EFI_STATUS load_read_only_file(
    EFI_SYSTEM_TABLE *system_table,
    EFI_FILE_PROTOCOL *root,
    CHAR16 *path,
    UINT64 *file_address,
    UINT64 *file_size)
{
    EFI_FILE_PROTOCOL *file = NULL;
    EFI_STATUS status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        return status;
    }

    /* Preferred path: ask the firmware for the exact size and allocate exactly
     * that (rounded up to pages), so large media (video, full-res images) can be
     * preloaded on machines with no post-boot storage driver without wasting a
     * fixed buffer per file. VIBIO_BOOT_FILE_MAX_SIZE is now just an upper safety
     * limit. If GetInfo is unavailable or the size looks wrong, fall back to the
     * original fixed-buffer read so the loader can never regress below before. */
    UINT64 exact = query_file_size(file);
    if (exact > 0 && exact <= VIBIO_BOOT_FILE_MAX_SIZE) {
        void *buffer = NULL;
        status = allocate_loader_buffer(system_table, (UINTN)exact, &buffer);
        if (EFI_ERROR(status)) {
            file->Close(file);
            return status;
        }
        UINT64 total = 0;
        while (total < exact) {
            UINTN bytes_read = 1024 * 1024;
            UINT64 remaining = exact - total;
            if (bytes_read > remaining) {
                bytes_read = (UINTN)remaining;
            }
            status = file->Read(file, &bytes_read, (void *)((UINT64)buffer + total));
            if (EFI_ERROR(status)) {
                file->Close(file);
                return status;
            }
            if (bytes_read == 0) {
                break;
            }
            total += bytes_read;
        }
        file->Close(file);
        *file_address = (UINT64)buffer;
        *file_size = total;
        return EFI_SUCCESS;
    }

    void *buffer = NULL;
    status = allocate_loader_buffer(system_table, VIBIO_BOOT_FILE_MAX_SIZE, &buffer);
    if (EFI_ERROR(status)) {
        file->Close(file);
        return status;
    }

    UINT64 total = 0;
    while (total < VIBIO_BOOT_FILE_MAX_SIZE) {
        UINTN requested = 4096;
        UINT64 remaining = VIBIO_BOOT_FILE_MAX_SIZE - total;
        if (requested > remaining) {
            requested = (UINTN)remaining;
        }

        UINTN bytes_read = requested;
        status = file->Read(file, &bytes_read, (void *)((UINT64)buffer + total));
        if (EFI_ERROR(status)) {
            file->Close(file);
            return status;
        }

        if (bytes_read == 0) {
            break;
        }

        total += bytes_read;
    }

    if (total == VIBIO_BOOT_FILE_MAX_SIZE) {
        UINT8 extra_byte = 0;
        UINTN extra_read = 1;
        status = file->Read(file, &extra_read, &extra_byte);
        if (EFI_ERROR(status)) {
            file->Close(file);
            return status;
        }

        if (extra_read != 0) {
            file->Close(file);
            /* Over the preload cap: free the 32 MB scratch buffer instead of
             * leaking it. This matters now that EVERY unknown root file is
             * preload-attempted - a USB with several >32 MB videos would
             * otherwise leak 32 MB each and could exhaust firmware memory. */
            system_table->BootServices->FreePages((UINT64)buffer,
                (VIBIO_BOOT_FILE_MAX_SIZE + 0xFFFULL) / 0x1000ULL);
            return EFI_BUFFER_TOO_SMALL;
        }
    }

    file->Close(file);
    *file_address = (UINT64)buffer;
    *file_size = total;
    return EFI_SUCCESS;
}

/*
 * Load the optional packaged sound files. These are not required to boot, and
 * playback is not implemented yet, so a missing, misnamed, or oversized file
 * must never stop the boot. Any file that fails to load is simply skipped and
 * Vibio continues with however many it found.
 */
static void load_boot_files(EFI_SYSTEM_TABLE *system_table, EFI_FILE_PROTOCOL *root, VibioBootInfo *boot_info)
{
    boot_info->boot_file_count = 0;
    UINTN request_count = sizeof(boot_file_requests) / sizeof(boot_file_requests[0]);

    for (UINTN i = 0; i < request_count && boot_info->boot_file_count < VIBIO_BOOT_FILE_MAX; i++) {
        UINT64 file_address = 0;
        UINT64 file_size = 0;
        EFI_STATUS status = load_read_only_file(
            system_table,
            root,
            boot_file_requests[i].path,
            &file_address,
            &file_size);
        if (EFI_ERROR(status)) {
            continue;
        }

        VibioBootFile *file = &boot_info->boot_files[boot_info->boot_file_count];
        file->address = file_address;
        file->size = file_size;
        file->type = boot_file_requests[i].type;
        set_boot_file_name(file, boot_file_requests[i].name);
        boot_info->boot_file_count++;
    }
}

static char ascii_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

static UINT32 ascii_name_len(const char *s)
{
    UINT32 n = 0;
    while (s[n] != 0 && n < VIBIO_BOOT_FILE_NAME_MAX) {
        n++;
    }
    return n;
}

static int ascii_name_eq(const char *a, const char *b)
{
    UINT32 i = 0;
    while (a[i] != 0 && b[i] != 0 && i < VIBIO_BOOT_FILE_NAME_MAX) {
        if (ascii_upper(a[i]) != ascii_upper(b[i])) {
            return 0;
        }
        i++;
    }
    return a[i] == 0 && b[i] == 0;
}

static int boot_file_already_loaded(VibioBootInfo *boot_info, const char *name)
{
    for (UINT64 i = 0; i < boot_info->boot_file_count && i < VIBIO_BOOT_FILE_MAX; i++) {
        VibioBootFile *file = &boot_info->boot_files[i];
        if (file->name_length == ascii_name_len(name) && ascii_name_eq(file->name, name)) {
            return 1;
        }
    }
    return 0;
}

static int name_has_ext(const char *name, const char *ext)
{
    UINT32 n = 0;
    while (name[n] != 0 && n < VIBIO_BOOT_FILE_NAME_MAX) {
        n++;
    }
    UINT32 e = 0;
    while (ext[e] != 0) {
        e++;
    }
    if (e > n) {
        return 0;
    }
    UINT32 start = n - e;
    for (UINT32 i = 0; i < e; i++) {
        if (ascii_upper(name[start + i]) != ascii_upper(ext[i])) {
            return 0;
        }
    }
    return 1;
}

static UINT32 boot_file_type_for_name(const char *name)
{
    if (name_has_ext(name, ".WAV")) {
        return VIBIO_BOOT_FILE_TYPE_SOUND;
    }
    if (name_has_ext(name, ".HTM") || name_has_ext(name, ".HTML")) {
        return VIBIO_BOOT_FILE_TYPE_HTML;
    }
    if (name_has_ext(name, ".VIV") || name_has_ext(name, ".VVID")) {
        return VIBIO_BOOT_FILE_TYPE_VIDEO;
    }
    if (name_has_ext(name, ".PNG") || name_has_ext(name, ".BMP") ||
        name_has_ext(name, ".JPG") || name_has_ext(name, ".JPEG") ||
        name_has_ext(name, ".GIF") || name_has_ext(name, ".VIM") ||
        name_has_ext(name, ".VIMG")) {
        return VIBIO_BOOT_FILE_TYPE_IMAGE;
    }
    return 0;
}

static UINT32 char16_to_ascii_name(CHAR16 *src, char *dst)
{
    UINT32 n = 0;
    while (src[n] != 0 && n + 1 < VIBIO_BOOT_FILE_NAME_MAX) {
        CHAR16 ch = src[n];
        if (ch < 32 || ch > 126) {
            return 0;
        }
        dst[n] = (char)ch;
        n++;
    }
    dst[n] = 0;
    return n;
}

static void ascii_name_to_root_path(const char *name, CHAR16 *path, UINTN path_max)
{
    if (path_max == 0) {
        return;
    }
    UINTN p = 0;
    path[p++] = L'\\';
    for (UINTN i = 0; name[i] != 0 && p + 1 < path_max; i++) {
        path[p++] = (CHAR16)name[i];
    }
    path[p] = 0;
}

static void load_one_boot_file(
    EFI_SYSTEM_TABLE *system_table,
    EFI_FILE_PROTOCOL *root,
    VibioBootInfo *boot_info,
    CHAR16 *path,
    const char *name,
    UINT32 type)
{
    if (boot_info->boot_file_count >= VIBIO_BOOT_FILE_MAX || boot_file_already_loaded(boot_info, name)) {
        return;
    }
    UINT64 file_address = 0;
    UINT64 file_size = 0;
    EFI_STATUS status = load_read_only_file(system_table, root, path, &file_address, &file_size);
    if (EFI_ERROR(status)) {
        return;
    }
    VibioBootFile *file = &boot_info->boot_files[boot_info->boot_file_count];
    file->address = file_address;
    file->size = file_size;
    file->type = type;
    set_boot_file_name(file, (char *)name);
    boot_info->boot_file_count++;
}

static void add_boot_directory(VibioBootInfo *boot_info, const char *name)
{
    if (boot_info->boot_file_count >= VIBIO_BOOT_FILE_MAX || boot_file_already_loaded(boot_info, name)) {
        return;
    }
    VibioBootFile *file = &boot_info->boot_files[boot_info->boot_file_count];
    file->address = 0;
    file->size = 0;
    file->type = VIBIO_BOOT_FILE_TYPE_DIRECTORY;
    set_boot_file_name(file, (char *)name);
    boot_info->boot_file_count++;
}

/* Record a file in the boot table WITHOUT loading its bytes: an unknown or too
 * large file still appears in the Files listing (with its real size) instead of
 * being silently dropped. address == 0 marks it as "listed, not loaded". */
static void add_boot_listing_only(VibioBootInfo *boot_info, const char *name, UINT64 size)
{
    if (boot_info->boot_file_count >= VIBIO_BOOT_FILE_MAX || boot_file_already_loaded(boot_info, name)) {
        return;
    }
    VibioBootFile *file = &boot_info->boot_files[boot_info->boot_file_count];
    file->address = 0;
    file->size = size;
    file->type = VIBIO_BOOT_FILE_TYPE_OTHER;
    set_boot_file_name(file, (char *)name);
    boot_info->boot_file_count++;
}

static void load_root_boot_files(EFI_SYSTEM_TABLE *system_table, EFI_FILE_PROTOCOL *root, VibioBootInfo *boot_info)
{
    if (root->SetPosition != 0) {
        root->SetPosition(root, 0);
    }

    UINT8 buffer[2048];
    for (;;) {
        UINTN size = sizeof(buffer);
        EFI_STATUS status = root->Read(root, &size, buffer);
        if (EFI_ERROR(status) || size == 0) {
            break;
        }
        EFI_FILE_INFO *info = (EFI_FILE_INFO *)buffer;
        char name[VIBIO_BOOT_FILE_NAME_MAX];
        if (char16_to_ascii_name(info->FileName, name) == 0) {
            continue;
        }
        if (ascii_name_eq(name, ".") || ascii_name_eq(name, "..")) {
            continue;
        }
        if ((info->Attribute & EFI_FILE_DIRECTORY) != 0) {
            continue;
        }
        UINT32 type = boot_file_type_for_name(name);
        if (type == 0) {
            /* Unknown/unsupported extension (e.g. .mp4, .txt): preload it as a
             * generic file so it is not merely VISIBLE but actually READABLE on a
             * machine with no live post-boot storage driver (the laptop boots off
             * an xHCI USB and has an NVMe internal disk, so neither the AHCI nor
             * the live-USB backend serves these files). Without this the file
             * listed in Files but every read returned 0 -> "not found on this
             * source". Files over the preload cap fall back to listing-only. */
            type = VIBIO_BOOT_FILE_TYPE_OTHER;
        }
        CHAR16 path[80];
        ascii_name_to_root_path(name, path, sizeof(path) / sizeof(path[0]));
        UINT64 before = boot_info->boot_file_count;
        load_one_boot_file(system_table, root, boot_info, path, name, type);
        /* If the file could not be loaded (e.g. larger than the preload cap),
         * still list it by name so it is not hidden. */
        if (boot_info->boot_file_count == before) {
            add_boot_listing_only(boot_info, name, info->FileSize);
        }
        if (boot_info->boot_file_count >= VIBIO_BOOT_FILE_MAX) {
            break;
        }
    }

    add_boot_directory(boot_info, "EFI");
    add_boot_directory(boot_info, "SOUNDS");

    if (root->SetPosition != 0) {
        root->SetPosition(root, 0);
    }

    for (;;) {
        UINTN size = sizeof(buffer);
        EFI_STATUS status = root->Read(root, &size, buffer);
        if (EFI_ERROR(status) || size == 0) {
            break;
        }
        EFI_FILE_INFO *info = (EFI_FILE_INFO *)buffer;
        if ((info->Attribute & EFI_FILE_DIRECTORY) == 0) {
            continue;
        }
        char name[VIBIO_BOOT_FILE_NAME_MAX];
        if (char16_to_ascii_name(info->FileName, name) == 0) {
            continue;
        }
        if (ascii_name_eq(name, ".") || ascii_name_eq(name, "..")) {
            continue;
        }
        add_boot_directory(boot_info, name);
        if (boot_info->boot_file_count >= VIBIO_BOOT_FILE_MAX) {
            break;
        }
    }
}

static EFI_STATUS fill_boot_info(
    EFI_SYSTEM_TABLE *system_table,
    VibioBootInfo *boot_info,
    UINT64 kernel_address,
    UINTN kernel_size)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_STATUS status = system_table->BootServices->LocateProtocol(
        &graphics_output_protocol_guid,
        NULL,
        (void **)&gop);

    if (EFI_ERROR(status)) {
        return status;
    }

    /* Select the panel's native (largest) graphics mode instead of whatever the
     * firmware defaulted to, so the desktop is sharp rather than stretched. The
     * choice is capped at 1920x1080 on purpose: a bigger framebuffer means more
     * pixels to blit every frame, which would worsen the per-frame cost on real
     * hardware. Only 32-bit RGB/BGR modes are considered (the renderer's format).
     * If nothing qualifies, the current mode is kept. */
    {
        UINT32 best_mode = gop->Mode->Mode;
        UINT64 best_area = 0;
        UINT32 max_mode = gop->Mode->MaxMode;
        for (UINT32 m = 0; m < max_mode; m++) {
            UINTN info_size = 0;
            EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mi = NULL;
            if (EFI_ERROR(gop->QueryMode(gop, m, &info_size, &mi)) || mi == NULL) {
                continue;
            }
            /* PixelFormat 0 = RGB8, 1 = BGR8 (the two the renderer supports). */
            if (mi->PixelFormat != 0 && mi->PixelFormat != 1) {
                continue;
            }
            UINT32 w = mi->HorizontalResolution;
            UINT32 h = mi->VerticalResolution;
            if (w == 0 || h == 0 || w > 1920 || h > 1080) {
                continue;
            }
            UINT64 area = (UINT64)w * (UINT64)h;
            if (area > best_area) {
                best_area = area;
                best_mode = m;
            }
        }
        if (best_area > 0 && best_mode != gop->Mode->Mode) {
            gop->SetMode(gop, best_mode);
        }
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = gop->Mode->Info;

    boot_info->magic = VIBIO_BOOT_INFO_MAGIC;
    boot_info->framebuffer_base = gop->Mode->FrameBufferBase;
    boot_info->framebuffer_size = gop->Mode->FrameBufferSize;
    boot_info->framebuffer_width = mode_info->HorizontalResolution;
    boot_info->framebuffer_height = mode_info->VerticalResolution;
    boot_info->pixels_per_scan_line = mode_info->PixelsPerScanLine;
    boot_info->pixel_format = mode_info->PixelFormat;
    boot_info->kernel_load_address = kernel_address;
    boot_info->kernel_size = kernel_size;

    return EFI_SUCCESS;
}

static EFI_STATUS exit_boot_services(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table,
    VibioBootInfo *boot_info,
    EFI_MEMORY_DESCRIPTOR *memory_map)
{
    EFI_STATUS status = EFI_SUCCESS;

    for (UINTN attempt = 0; attempt < 5; attempt++) {
        UINTN memory_map_size = VIBIO_MEMORY_MAP_MAX_SIZE;
        UINTN map_key = 0;
        UINTN descriptor_size = 0;
        UINT32 descriptor_version = 0;

        status = system_table->BootServices->GetMemoryMap(
            &memory_map_size,
            memory_map,
            &map_key,
            &descriptor_size,
            &descriptor_version);

        if (EFI_ERROR(status)) {
            return status;
        }

        boot_info->memory_map_base = (UINT64)memory_map;
        boot_info->memory_map_size = memory_map_size;
        boot_info->memory_map_descriptor_size = descriptor_size;
        boot_info->memory_map_descriptor_version = descriptor_version;
        boot_info->memory_map_entry_count = descriptor_size == 0 ? 0 : memory_map_size / descriptor_size;
        boot_info->boot_services_exited = 0;

        status = system_table->BootServices->ExitBootServices(image_handle, map_key);
        if (!EFI_ERROR(status)) {
            boot_info->boot_services_exited = 1;
            return EFI_SUCCESS;
        }
    }

    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    system_table->BootServices->SetWatchdogTimer(0, 0, 0, 0);
    system_table->ConOut->EnableCursor(system_table->ConOut, 0);
    set_color(system_table, EFI_LIGHTGRAY);
    system_table->ConOut->ClearScreen(system_table->ConOut);

    EFI_FILE_PROTOCOL *root = NULL;
    EFI_STATUS status = open_root_volume(image_handle, system_table, &root);
    if (EFI_ERROR(status)) {
        boot_fail(system_table, EFI_WHITE, EFI_BACKGROUND_RED, L"VB-B001", L"Could not open boot volume.", status);
    }

    UINT64 kernel_address = 0;
    UINTN kernel_size = 0;
    status = load_kernel(system_table, root, &kernel_address, &kernel_size);
    if (EFI_ERROR(status)) {
        root->Close(root);
        boot_fail(system_table, EFI_BLACK, EFI_BACKGROUND_BROWN, L"VB-B002", L"Could not load KERNEL.BIN.", status);
    }

    VibioBootInfo *boot_info = NULL;
    status = allocate_loader_buffer(system_table, sizeof(VibioBootInfo), (void **)&boot_info);
    if (EFI_ERROR(status)) {
        root->Close(root);
        boot_fail(system_table, EFI_WHITE, EFI_BACKGROUND_MAGENTA, L"VB-B003", L"Could not allocate boot info.", status);
    }

    EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
    status = allocate_loader_buffer(system_table, VIBIO_MEMORY_MAP_MAX_SIZE, (void **)&memory_map);
    if (EFI_ERROR(status)) {
        root->Close(root);
        boot_fail(system_table, EFI_BLACK, EFI_BACKGROUND_LIGHTGRAY, L"VB-B004", L"Could not allocate memory map.", status);
    }

    status = fill_boot_info(system_table, boot_info, kernel_address, kernel_size);
    if (EFI_ERROR(status)) {
        root->Close(root);
        boot_fail(system_table, EFI_BLACK, EFI_BACKGROUND_GREEN, L"VB-B005", L"Could not locate framebuffer.", status);
    }

    load_boot_files(system_table, root, boot_info);
    load_root_boot_files(system_table, root, boot_info);
    root->Close(root);

    find_acpi_rsdp(system_table, boot_info);

    status = exit_boot_services(image_handle, system_table, boot_info, memory_map);
    if (EFI_ERROR(status)) {
        boot_fail(system_table, EFI_BLACK, EFI_BACKGROUND_BLUE, L"VB-B006", L"Could not exit boot services.", status);
    }

    KernelEntry kernel_entry = (KernelEntry)kernel_address;
    kernel_entry(boot_info);

    for (;;) {
        __asm__ volatile("hlt");
    }

    return EFI_SUCCESS;
}
