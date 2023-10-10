//
// NOTE: "void *" fields in structs = not implemented!!
//   They are defined in the UEFI spec, but I have not used
//   them or implemented them yet. Using void pointers 
//   ensures they take up the same amount of space so that
//   the actually defined functions work correctly and are 
//   at the correct offsets.
//

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // NULL

#if __has_include(<uchar.h>)
  #include <uchar.h>
#endif

// UEFI Spec 2.10 section 2.4
#define IN
#define OUT
#define OPTIONAL
#define CONST const

// EFIAPI defines the calling convention for EFI defined functions
// Taken from gnu-efi at
// https://github.com/vathpela/gnu-efi/blob/master/inc/x86_64/efibind.h
#define EFIAPI __attribute__((ms_abi))  // x86_64 Microsoft calling convention

// Data types: UEFI Spec 2.10 section 2.3
#define FALSE 0
#define TRUE 1
typedef uint8_t  BOOLEAN;  // 0 = False, 1 = True
typedef int64_t  INTN;
typedef uint64_t UINTN;
typedef int8_t   INT8;
typedef uint8_t  UINT8;
typedef int16_t  INT16;
typedef uint16_t UINT16;
typedef int32_t  INT32;
typedef uint32_t UINT32;
typedef int64_t  INT64;
typedef uint64_t UINT64;
typedef char     CHAR8;

// UTF-16 equivalent-ish type, for UCS-2 characters
//   codepoints <= 0xFFFF
#ifndef _UCHAR_H
    typedef uint_least16_t char16_t;
#endif
typedef char16_t CHAR16;

typedef void VOID;

typedef struct {
    UINT32 TimeLow;
    UINT16 TimeMid;
    UINT16 TimeHighAndVersion;
    UINT8  ClockSeqHighAndReserved;
    UINT8  ClockSeqLow;
    UINT8  Node[6];
} __attribute__ ((packed)) EFI_GUID;

typedef UINTN EFI_STATUS;
typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;
typedef UINT64 EFI_LBA;
typedef UINTN EFI_TPL;

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

// EFI_GUID values - various/misc./NOT all inclusive
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
{0x9042a9de,0x23dc,0x4a38,\
 0x96,0xfb,{0x7a,0xde,0xd0,0x80,0x51,0x6a}}

// EFI_STATUS Codes - UEFI Spec 2.10 Appendix D
#define EFI_SUCCESS 0ULL

#define TOP_BIT 0x8000000000000000
#define ENCODE_ERROR(x) (TOP_BIT | (x))
#define EFI_ERROR(x) ((INTN)((UINTN)(x)) < 0)

#define EFI_UNSUPPORTED  ENCODE_ERROR(3)
#define EFI_DEVICE_ERROR ENCODE_ERROR(7)


// EFI_GRAPHICS_OUTPUT_PROTOCOL
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

// EFI_PIXEL_BITMASK
typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

// EFI_GRAPHICS_PIXEL_FORMAT
typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

// EFI_GRAPHICS_OUTPUT_MODE_INFORMATION
typedef struct {
    UINT32                    Version;
    UINT32                    HorizontalResolution;
    UINT32                    VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat; 
    EFI_PIXEL_BITMASK         PixelInformation;
    UINT32                    PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;


// EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE: UEFI spec 2.10 section 12.9.2.1
typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE) (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
    IN UINT32                                ModeNumber,
    OUT UINTN                                *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);

// EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE: UEFI spec 2.10 section 12.9.2.2
typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE) (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32                       ModeNumber
);

// EFI_GRAPHICS_OUTPUT_BLT_PIXEL
typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

// EFI_GRAPHICS_OUTPUT_BLT_OPERATION
typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

// EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT: UEFI spec 2.10 section 12.9.2.3
typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT) (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
    IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer OPTIONAL,
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN                             SourceX,
    IN UINTN                             SourceY,
    IN UINTN                             DestinationX,
    IN UINTN                             DestinationY,
    IN UINTN                             Width,
    IN UINTN                             Height,
    IN UINTN                             Delta OPTIONAL
);

// EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE
typedef struct {
    UINT32                               MaxMode;
    UINT32                               Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN                                SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                 FrameBufferBase;
    UINTN                                FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

// EFI_GRAPHICS_OUTPUT_PROTOCOL: UEFI spec 2.10 section 12.9.2
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT        Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE       *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

// EFI_LOCATE_PROTOCOL: UEFI spec 2.10 section 7.3.16
typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL) (
    IN EFI_GUID *Protocol,
    IN VOID     *Registration OPTIONAL,
    OUT VOID    **Interface
);

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// EFI_TEXT_RESET: UEFI Spec 2.10 section 12.4.2
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_RESET) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN                         ExtendedVerification
);

// EFI_TEXT_STRING: UEFI Spec 2.10 section 12.4.3
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_STRING) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16                          *String
);

// EFI_TEXT_QUERY_MODE: UEFI Spec 2.10 section 12.4.5
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_QUERY_MODE) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           ModeNumber,
    OUT UINTN                          *Columns,
    OUT UINTN                          *Rows
);

// EFI_TEXT_SET_MODE: UEFI Spec 2.10 section 12.4.6
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_MODE) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           ModeNumber
);

// EFI_TEXT_SET_ATTRIBUTE: UEFI Spec 2.10 section 12.4.7
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_ATTRIBUTE) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           Attribute
);

// Attributes (text colors)
#define EFI_BLACK                             0x00
#define EFI_BLUE                              0x01
#define EFI_GREEN                             0x02
#define EFI_CYAN                              0x03
#define EFI_RED                               0x04
#define EFI_MAGENTA                           0x05
#define EFI_BROWN                             0x06
#define EFI_LIGHTGRAY                         0x07
#define EFI_BRIGHT                            0x08
#define EFI_DARKGRAY                          0x08 // (EFI_BLACK | EFI_BRIGHT)
#define EFI_LIGHTBLUE                         0x09
#define EFI_LIGHTGREEN                        0x0A
#define EFI_LIGHTCYAN                         0x0B
#define EFI_LIGHTRED                          0x0C
#define EFI_LIGHTMAGENTA                      0x0D
#define EFI_YELLOW                            0x0E
#define EFI_WHITE                             0x0F

// Background colors
#define EFI_BACKGROUND_BLACK     0x00
#define EFI_BACKGROUND_BLUE      0x10
#define EFI_BACKGROUND_GREEN     0x20
#define EFI_BACKGROUND_CYAN      0x30
#define EFI_BACKGROUND_RED       0x40
#define EFI_BACKGROUND_MAGENTA   0x50
#define EFI_BACKGROUND_BROWN     0x60
#define EFI_BACKGROUND_LIGHTGRAY 0x70

#define EFI_TEXT_ATTR(Foreground, Background) \
    ((Foreground) | ((Background) << 4))

// EFI_TEXT_CLEAR_SCREEN: UEFI Spec 2.10 section 12.4.8
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This 
);

// EFI_TEXT_SET_CURSOR_POSITION: UEFI Spec 2.10 section 12.4.9
typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_CURSOR_POSITION) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN                           Column,
    IN UINTN                           Row
);

// SIMPLE_TEXT_OUTPUT_MODE
typedef struct {
    INT32   MaxMode;

    // Current settings
    INT32   Mode;
    INT32   Attribute;
    INT32   CursorColumn;
    INT32   CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL: UEFI Spec 2.10 section 12.4.1
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET               Reset;
    EFI_TEXT_STRING              OutputString;
    void                         *TestString;
    EFI_TEXT_QUERY_MODE          QueryMode;
    EFI_TEXT_SET_MODE            SetMode;
    EFI_TEXT_SET_ATTRIBUTE       SetAttribute;
    EFI_TEXT_CLEAR_SCREEN        ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    void                         *EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE      *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// EFI_SIMPLE_TEXT_INPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// EFI_INPUT_RESET: UEFI Spec 2.10 section 12.3.2
typedef 
EFI_STATUS
(EFIAPI *EFI_INPUT_RESET) (
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    IN BOOLEAN                        ExtendedVerification
);

// EFI_INPUT_KEY
typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

// EFI_INPUT_RESET: UEFI Spec 2.10 section 12.3.2
typedef 
EFI_STATUS
(EFIAPI *EFI_INPUT_READ_KEY) (
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    OUT EFI_INPUT_KEY                 *Key
);

// EFI_SIMPLE_TEXT_INPUT_PROTOCOL: UEFI Spec 2.10 section 12.3.1
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET    Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    EFI_EVENT          WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// EFI_WAIT_FOR_EVENT: UEFI Spec 2.10 section 7.1.5
typedef 
EFI_STATUS
(EFIAPI *EFI_WAIT_FOR_EVENT) (
    IN UINTN     NumberOfEvents,
    IN EFI_EVENT *Event,
    OUT UINTN    *Index
);

// EFI_RESET_TYPE: UEFI Spec 2.10 section 8.5.1
typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

// EFI_RESET_SYSTEM: UEFI Spec 2.10 section 8.5.1
typedef
VOID
(EFIAPI *EFI_RESET_SYSTEM) (
    IN EFI_RESET_TYPE ResetType,
    IN EFI_STATUS     ResetStatus,
    IN UINTN          DataSize,
    IN VOID           *ResetData OPTIONAL
);

// EFI_TABLE_HEADER: UEFI Spec 2.10 section 4.2.1
typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

// EFI_RUNTIME_SERVICES: UEFI Spec 2.10 section 4.5.1
typedef struct {
    EFI_TABLE_HEADER Hdr;
    
    //
    // Time Services
    //
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    
    //
    // Virtual Memory Services
    //
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    
    //
    // Variable Services
    //
    void *GetVariable;
    void *GetNextVariableName;
    void *SetVariable;
    
    //
    // Miscellaneous Services
    //
    void *GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
    
    //
    // UEFI 2.0 Capsule Services
    //
    void *UpdateCapsule;
    void *QueryCapsuleCapabilities;
    
    //
    // Miscellaneous UEFI 2.0 Service
    //
    void *QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

// EFI_BOOT_SERVICES: UEFI Spec 2.10 section 4.4.1
typedef struct {
    EFI_TABLE_HEADER Hdr;

    //
    // Task Priority Services
    //
    void* RaiseTPL;
    void* RestoreTPL;

    //
    // Memory Services
    //
    void* AllocatePages;
    void* FreePages;
    void* GetMemoryMap;
    void* AllocatePool;
    void* FreePool;

    //
    // Event & Timer Services
    //
    void*              CreateEvent;
    void*              SetTimer;
    EFI_WAIT_FOR_EVENT WaitForEvent;
    void*              SignalEvent;
    void*              CloseEvent;
    void*              CheckEvent;

    //
    // Protocol Handler Services
    //
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    void* HandleProtocol;
    VOID* Reserved;
    void* RegisterProtocolNotify;
    void* LocateHandle;
    void* LocateDevicePath;
    void* InstallConfigurationTable;

    //
    // Image Services
    //
    void* LoadImage;
    void* StartImage;
    void* Exit;
    void* UnloadImage;
    void* ExitBootServices;

    //
    // Miscellaneous Services
    //
    void* GetNextMonotonicCount;
    void* Stall;
    void* SetWatchdogTimer;

    //
    // DriverSupport Services
    //
    void* ConnectController;
    void* DisconnectController;

    //
    // Open and Close Protocol Services
    //
    void* OpenProtocol;
    void* CloseProtocol;
    void* OpenProtocolInformation;

    //
    // Library Services
    //
    void*               ProtocolsPerHandle;
    void*               LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    void*               InstallMultipleProtocolInterfaces;
    void*               UninstallMultipleProtocolInterfaces;

    //
    // 32-bit CRC Services
    //
    void* CalculateCrc32;

    //
    // Miscellaneous Services
    //
    void* CopyMem;
    void* SetMem;
    void* CreateEventEx;
} EFI_BOOT_SERVICES;

// EFI_SYSTEM_TABLE: UEFI Spec 2.10 section 4.3.1
typedef struct {
    EFI_TABLE_HEADER                Hdr;
    CHAR16                          *FirmwareVendor;
    UINT32                          FirmwareRevision;
    EFI_HANDLE                      ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *ConIn;
    EFI_HANDLE                      ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE                      StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES            *RuntimeServices;
    EFI_BOOT_SERVICES               *BootServices;
    UINTN                           NumberOfTableEntries;
    //EFI_CONFIGURATION_TABLE         *ConfigurationTable;
    void                            *ConfigurationTable;
} EFI_SYSTEM_TABLE;

// EFI_IMAGE_ENTRY_POINT: UEFI Spec 2.10 section 4.1.1
typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_ENTRY_POINT) (
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
);


