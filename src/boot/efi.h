#ifndef VIBIO_EFI_H
#define VIBIO_EFI_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef int INT32;
typedef long long INT64;
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;
typedef void *EFI_EVENT;
typedef UINT16 CHAR16;

#define EFI_SUCCESS 0
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL
#define EFI_ERROR(Status) (((INT64)(Status)) < 0)
#define EFIAPI __attribute__((ms_abi))

#define NULL ((void *)0)

#define EFI_BLACK 0x00
#define EFI_BLUE 0x01
#define EFI_GREEN 0x02
#define EFI_CYAN 0x03
#define EFI_RED 0x04
#define EFI_MAGENTA 0x05
#define EFI_BROWN 0x06
#define EFI_LIGHTGRAY 0x07
#define EFI_DARKGRAY 0x08
#define EFI_LIGHTBLUE 0x09
#define EFI_LIGHTGREEN 0x0A
#define EFI_LIGHTCYAN 0x0B
#define EFI_LIGHTRED 0x0C
#define EFI_LIGHTMAGENTA 0x0D
#define EFI_YELLOW 0x0E
#define EFI_WHITE 0x0F
#define EFI_BACKGROUND_BLACK 0x00
#define EFI_BACKGROUND_BLUE 0x10
#define EFI_BACKGROUND_GREEN 0x20
#define EFI_BACKGROUND_CYAN 0x30
#define EFI_BACKGROUND_RED 0x40
#define EFI_BACKGROUND_MAGENTA 0x50
#define EFI_BACKGROUND_BROWN 0x60
#define EFI_BACKGROUND_LIGHTGRAY 0x70

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001
#define EFI_FILE_MODE_READ 0x0000000000000001ULL
#define EFI_FILE_DIRECTORY 0x0000000000000010ULL
#define EFI_ALLOCATE_ANY_PAGES 0
#define EFI_ALLOCATE_MAX_ADDRESS 1
#define EFI_ALLOCATE_ADDRESS 2
#define EFI_LOADER_CODE 1
#define EFI_LOADER_DATA 2

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *Reset)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, UINT8 ExtendedVerification);
    EFI_STATUS(EFIAPI *ReadKeyStroke)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key);
    EFI_EVENT WaitForKey;
};

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    INT32 MaxMode;
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    UINT8 CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *Reset)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINT8 ExtendedVerification);
    EFI_STATUS(EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
    EFI_STATUS(EFIAPI *TestString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
    EFI_STATUS(EFIAPI *QueryMode)(
        EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        UINTN ModeNumber,
        UINTN *Columns,
        UINTN *Rows);
    EFI_STATUS(EFIAPI *SetMode)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN ModeNumber);
    EFI_STATUS(EFIAPI *SetAttribute)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Attribute);
    EFI_STATUS(EFIAPI *ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
    EFI_STATUS(EFIAPI *SetCursorPosition)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Column, UINTN Row);
    EFI_STATUS(EFIAPI *EnableCursor)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINT8 Visible);
    EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
};

typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;

typedef struct {
    UINT32 Type;
    UINT32 Pad;
    UINT64 PhysicalStart;
    UINT64 VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

struct EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    EFI_STATUS(EFIAPI *AllocatePages)(UINT32 Type, UINT32 MemoryType, UINTN Pages, UINT64 *Memory);
    EFI_STATUS(EFIAPI *FreePages)(UINT64 Memory, UINTN Pages);
    EFI_STATUS(EFIAPI *GetMemoryMap)(
        UINTN *MemoryMapSize,
        EFI_MEMORY_DESCRIPTOR *MemoryMap,
        UINTN *MapKey,
        UINTN *DescriptorSize,
        UINT32 *DescriptorVersion);
    void *AllocatePool;
    void *FreePool;
    void *CreateEvent;
    void *SetTimer;
    EFI_STATUS(EFIAPI *WaitForEvent)(UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index);
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    EFI_STATUS(EFIAPI *HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, void **Interface);
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    void *LoadImage;
    void *StartImage;
    void *Exit;
    void *UnloadImage;
    EFI_STATUS(EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, UINTN MapKey);
    void *GetNextMonotonicCount;
    EFI_STATUS(EFIAPI *Stall)(UINTN Microseconds);
    EFI_STATUS(EFIAPI *SetWatchdogTimer)(UINTN Timeout, UINT64 WatchdogCode, UINTN DataSize, CHAR16 *WatchdogData);
    EFI_STATUS(EFIAPI *ConnectController)(
        EFI_HANDLE ControllerHandle,
        EFI_HANDLE *DriverImageHandle,
        void *RemainingDevicePath,
        UINT8 Recursive);
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    void *LocateHandleBuffer;
    EFI_STATUS(EFIAPI *LocateProtocol)(EFI_GUID *Protocol, void *Registration, void **Interface);
};

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    UINT32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    UINT64 ImageSize;
    UINT32 ImageCodeType;
    UINT32 ImageDataType;
    EFI_STATUS(EFIAPI *Unload)(EFI_HANDLE ImageHandle);
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    UINT64 CreateTime[2];
    UINT64 LastAccessTime[2];
    UINT64 ModificationTime[2];
    UINT64 Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

struct EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS(EFIAPI *Open)(
        EFI_FILE_PROTOCOL *This,
        EFI_FILE_PROTOCOL **NewHandle,
        CHAR16 *FileName,
        UINT64 OpenMode,
        UINT64 Attributes);
    EFI_STATUS(EFIAPI *Close)(EFI_FILE_PROTOCOL *This);
    EFI_STATUS(EFIAPI *Delete)(EFI_FILE_PROTOCOL *This);
    EFI_STATUS(EFIAPI *Read)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
    EFI_STATUS(EFIAPI *Write)(EFI_FILE_PROTOCOL *This, UINTN *BufferSize, void *Buffer);
    void *GetPosition;
    EFI_STATUS(EFIAPI *SetPosition)(EFI_FILE_PROTOCOL *This, UINT64 Position);
    void *GetInfo;
    void *SetInfo;
    void *Flush;
};

typedef struct {
    UINT64 Revision;
    EFI_STATUS(EFIAPI *OpenVolume)(void *This, EFI_FILE_PROTOCOL **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINT64 FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *QueryMode)(
        EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
        UINT32 ModeNumber,
        UINTN *SizeOfInfo,
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);
    EFI_STATUS(EFIAPI *SetMode)(EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber);
    void *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
};

#endif
