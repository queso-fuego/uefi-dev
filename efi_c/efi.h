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
typedef uint_least16_t char16_t;
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


// EFI_MEMORY_TYPE: UEFI Spec 2.10 section 7.2.1
typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiUnacceptedMemoryType,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

// EFI_GUID values - various/misc./NOT all inclusive
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
{0x9042a9de,0x23dc,0x4a38,\
 0x96,0xfb,{0x7a,0xde,0xd0,0x80,0x51,0x6a}}

#define EFI_SIMPLE_POINTER_PROTOCOL_GUID \
{0x31878c87,0xb75,0x11d5,\
0x9a,0x4f,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define EFI_ABSOLUTE_POINTER_PROTOCOL_GUID \
{0x8D59D32B,0xC655,0x4AE9,\
0x9B,0x15,{0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43}}

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
{0x5B1B31A1,0x9562,0x11d2,\
0x8E,0x3F,{0x00,0xA0,0xC9,0x69,0x72,0x3B}}

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
{0x0964e5b22,0x6459,0x11d2,\
0x8e,0x39,{0x00,0xa0,0xc9,0x69,0x72,0x3b}}

#define EFI_FILE_INFO_ID \
{0x09576e92,0x6d3f,0x11d2,\
0x8e,0x39,{0x00,0xa0,0xc9,0x69,0x72,0x3b}}

#define EFI_FILE_SYSTEM_INFO_ID \
{0x09576e93,0x6d3f,0x11d2,\
0x8e,0x39,{0x00,0xa0,0xc9,0x69,0x72, 0x3b}}

#define EFI_BLOCK_IO_PROTOCOL_GUID \
{0x964e5b21,0x6459,0x11d2,\
0x8e,0x39,{0x00,0xa0,0xc9,0x69,0x72,0x3b}}

#define EFI_DISK_IO_PROTOCOL_GUID \
{0xCE345171,0xBA0B,0x11d2,\
0x8e,0x4F,{0x00,0xa0,0xc9,0x69,0x72,0x3b}}

#define EFI_PARTITION_INFO_PROTOCOL_GUID \
{0x8cf2f62c, 0xbc9b, 0x4821,\
0x80, 0x8d, {0xec, 0x9e, 0xc4, 0x21, 0xa1, 0xa0}}

// Partition Type GUID values:
// EFI System Partition GUID
#define ESP_GUID \
{0xC12A7328, 0xF81F, 0x11D2, \
0xBA, 0x4B, {0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}}

// (Microsoft) Basic Data GUID
#define BASIC_DATA_GUID \
{0xEBD0A0A2, 0xB9E5, 0x4433, \
0x87, 0xC0, {0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}}

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
    UINT32 RedMask; UINT32 GreenMask;
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

// EFI_SIMPLE_POINTER_PROTOCOL
typedef struct EFI_SIMPLE_POINTER_PROTOCOL EFI_SIMPLE_POINTER_PROTOCOL;

// EFI_SIMPLE_POINTER_RESET: UEFI Spec 2.10 section 12.5.2
typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_POINTER_RESET) (
    IN EFI_SIMPLE_POINTER_PROTOCOL *This,
    IN BOOLEAN                     ExtendedVerification
);

// EFI_SIMPLE_POINTER_STATE
typedef struct {
    INT32 RelativeMovementX;
    INT32 RelativeMovementY;
    INT32 RelativeMovementZ;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_STATE;

// EFI_SIMPLE_POINTER_GET_STATE: UEFI Spec 2.10 section 12.5.3
typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_POINTER_GET_STATE) (
    IN EFI_SIMPLE_POINTER_PROTOCOL *This,
    OUT EFI_SIMPLE_POINTER_STATE   *State
);

// EFI_SIMPLE_POINTER_MODE
typedef struct {
    UINT64  ResolutionX;
    UINT64  ResolutionY;
    UINT64  ResolutionZ;
    BOOLEAN LeftButton;
    BOOLEAN RightButton;
} EFI_SIMPLE_POINTER_MODE;

// EFI_SIMPLE_POINTER_PROTOCOL: UEFI Spec 2.10 section 12.5
typedef struct EFI_SIMPLE_POINTER_PROTOCOL {
    EFI_SIMPLE_POINTER_RESET     Reset;
    EFI_SIMPLE_POINTER_GET_STATE GetState;
    EFI_EVENT                    WaitForInput;
    EFI_SIMPLE_POINTER_MODE      *Mode;
} EFI_SIMPLE_POINTER_PROTOCOL;

// EFI_ABSOLUTE_POINTER_PROTOCOL
typedef struct EFI_ABSOLUTE_POINTER_PROTOCOL EFI_ABSOLUTE_POINTER_PROTOCOL; 

// EFI_ABSOLUTE_POINTER_MODE
typedef struct {
    UINT64 AbsoluteMinX;
    UINT64 AbsoluteMinY;
    UINT64 AbsoluteMinZ;
    UINT64 AbsoluteMaxX;
    UINT64 AbsoluteMaxY;
    UINT64 AbsoluteMaxZ;
    UINT32 Attributes;
} EFI_ABSOLUTE_POINTER_MODE;

// Attributes bit values
#define EFI_ABSP_SupportsAltActive   0x00000001
#define EFI_ABSP_SupportsPressureAsZ 0x00000002

// EFI_ABSOLUTE_POINTER_RESET: UEFI Spec 2.10 section 12.7.2
typedef
EFI_STATUS
(EFIAPI *EFI_ABSOLUTE_POINTER_RESET) (
    IN EFI_ABSOLUTE_POINTER_PROTOCOL *This,
    IN BOOLEAN                       ExtendedVerification
);

// EFI_ABSOLUTE_POINTER_STATE
typedef struct {
    UINT64 CurrentX;
    UINT64 CurrentY;
    UINT64 CurrentZ;
    UINT32 ActiveButtons;
} EFI_ABSOLUTE_POINTER_STATE;

// ActiveButtons bit values
#define EFI_ABSP_TouchActive 0x00000001
#define EFI_ABS_AltActive    0x00000002

// EFI_ABSOLUTE_POINTER_GET_STATE: UEFI Spec 2.10 section 12.7.3
typedef
EFI_STATUS
(EFIAPI *EFI_ABSOLUTE_POINTER_GET_STATE) (
    IN EFI_ABSOLUTE_POINTER_PROTOCOL *This,
    OUT EFI_ABSOLUTE_POINTER_STATE   *State
);

// EFI_ABSOLUTE_POINTER_PROTOCOL: UEFI Spec 2.10 section 12.7.1
typedef struct EFI_ABSOLUTE_POINTER_PROTOCOL {
    EFI_ABSOLUTE_POINTER_RESET     Reset;
    EFI_ABSOLUTE_POINTER_GET_STATE GetState;
    EFI_EVENT                      WaitForInput;
    EFI_ABSOLUTE_POINTER_MODE      *Mode;
} EFI_ABSOLUTE_POINTER_PROTOCOL;

// EFI_ALLOCATE_POOL: UEFI Spec 2.10 section 7.2.4
typedef
EFI_STATUS
(EFIAPI
*EFI_ALLOCATE_POOL) (
    IN EFI_MEMORY_TYPE PoolType,
    IN UINTN           Size,
    OUT VOID           **Buffer
);

// EFI_FREE_POOL: UEFI Spec 2.10 section 7.2.5
typedef
EFI_STATUS
(EFIAPI *EFI_FREE_POOL) (
    IN VOID *Buffer
);

// EFI_LOCATE_SEARCH_TYPE 
typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

// EFI_LOCATE_HANDLE_BUFFER: UEFI Spec 2.10 section 7.3.15
typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE_BUFFER) (
    IN EFI_LOCATE_SEARCH_TYPE SearchType,
    IN EFI_GUID               *Protocol OPTIONAL,
    IN VOID                   *SearchKey OPTIONAL,
    OUT UINTN                 *NoHandles,
    OUT EFI_HANDLE            **Buffer
);

// EFI_EVENT_NOTIFY
typedef
VOID
(EFIAPI *EFI_EVENT_NOTIFY) (
    IN EFI_EVENT Event,
    IN VOID      *Context
);

// EFI_CREATE_EVENT: UEFI Spec 2.10 section 7.1.1
typedef
EFI_STATUS
(EFIAPI *EFI_CREATE_EVENT) (
    IN UINT32           Type,
    IN EFI_TPL          NotifyTpl,
    IN EFI_EVENT_NOTIFY NotifyFunction OPTIONAL,
    IN VOID             *NotifyContext OPTIONAL,
    OUT EFI_EVENT       *Event
);

// EFI_TPL Levels (Task priority levels)
#define TPL_APPLICATION 4   // 0b00000100
#define TPL_CALLBACK    8   // 0b00001000
#define TPL_NOTIFY      16  // 0b00010000
#define TPL_HIGH_LEVEL  31  // 0b00011111

// EFI_EVENT types 
// These types can be "ORed" together as needed - for example,
// EVT_TIMER might be "ORed" with EVT_NOTIFY_WAIT or EVT_NOTIFY_SIGNAL.
#define EVT_TIMER                         0x80000000
#define EVT_RUNTIME                       0x40000000
#define EVT_NOTIFY_WAIT                   0x00000100
#define EVT_NOTIFY_SIGNAL                 0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES     0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

// EFI_TIMER_DELAY
typedef enum {
    TimerCancel,
    TimerPeriodic,
    TimerRelative
} EFI_TIMER_DELAY;

// EFI_SET_TIMER: UEFI Spec 2.10 section 7.1.7
typedef
EFI_STATUS
(EFIAPI *EFI_SET_TIMER) (
    IN EFI_EVENT       Event,
    IN EFI_TIMER_DELAY Type,
    IN UINT64          TriggerTime
);

// EFI_CLOSE_EVENT: UEFI Spec 2.10 section 7.1.3
typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_EVENT) (
    IN EFI_EVENT Event
);

// EFI_SET_WATCHDOG_TIMER: UEFI Spec 2.10 7.5.1
typedef
EFI_STATUS
(EFIAPI *EFI_SET_WATCHDOG_TIMER) (
    IN UINTN  Timeout,
    IN UINT64 WatchdogCode,
    IN UINTN  DataSize,
    IN CHAR16 *WatchdogData OPTIONAL
);

// EFI_WAIT_FOR_EVENT: UEFI Spec 2.10 section 7.1.5
typedef 
EFI_STATUS
(EFIAPI *EFI_WAIT_FOR_EVENT) (
    IN UINTN     NumberOfEvents,
    IN EFI_EVENT *Event,
    OUT UINTN    *Index
);

// EFI_OPEN_PROTOCOL: UEFI Spec 2.10 section 7.3.9
typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL) (
    IN EFI_HANDLE Handle,
    IN EFI_GUID *Protocol,
    OUT VOID **Interface OPTIONAL,
    IN EFI_HANDLE AgentHandle,
    IN EFI_HANDLE ControllerHandle,
    IN UINT32 Attributes
);

// EFI_CLOSE_PROTOCOL: UEFI Spec 2.10 section 7.3.10
typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_PROTOCOL) (
    IN EFI_HANDLE Handle,
    IN EFI_GUID   *Protocol,
    IN EFI_HANDLE AgentHandle,
    IN EFI_HANDLE ControllerHandle
);

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

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

// EFI_TIME
typedef struct {
    UINT16 Year;       // 1900 - 9999
    UINT8  Month;      // 1 - 12
    UINT8  Day;        // 1 - 31
    UINT8  Hour;       // 0 - 23
    UINT8  Minute;     // 0 - 59
    UINT8  Second;     // 0 - 59
    UINT8  Pad1; 
    UINT32 Nanosecond; // 0 - 999,999,999
    INT16  TimeZone;   // --1440 to 1440 or 2047
    UINT8  Daylight;
    UINT8  Pad2;
} EFI_TIME;

// Bit Definitions for EFI_TIME.Daylight
#define EFI_TIME_ADJUST_DAYLIGHT 0x01
#define EFI_TIME_IN_DAYLIGHT     0x02

// Value Definition for EFI_TIME.TimeZone
#define EFI_UNSPECIFIED_TIMEZONE 0x07FF

// EFI_TIME_CAPABILITIES
typedef struct {
    UINT32 Resolution;  // Resolution in counts/second e.g. 1hz = 1
    UINT32 Accuracy;    // Error rate of parts per million (1e-6)
    BOOLEAN SetsToZero; // TRUE = time set clears the time below the resolution level
} EFI_TIME_CAPABILITIES;

// EFI_GET_TIME: UEFI Spec 2.10 section 8.3.1
typedef
EFI_STATUS
(EFIAPI *EFI_GET_TIME) (
    OUT EFI_TIME              *Time,
    OUT EFI_TIME_CAPABILITIES *Capabilities OPTIONAL
);

// EFI_FILE_PROTOCOL: UEFI Spec 2.10 section 13.5.1
typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

// EFI_FILE_OPEN: UEFI Spec 2.10 section 13.5.2
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN) (
    IN EFI_FILE_PROTOCOL  *This,
    OUT EFI_FILE_PROTOCOL **NewHandle,
    IN CHAR16             *FileName,
    IN UINT64             OpenMode,
    IN UINT64             Attributes
);

// Open Modes
#define EFI_FILE_MODE_READ   0x0000000000000001
#define EFI_FILE_MODE_WRITE  0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000

// File Attributes
#define EFI_FILE_READ_ONLY  0x0000000000000001
#define EFI_FILE_HIDDEN     0x0000000000000002
#define EFI_FILE_SYSTEM     0x0000000000000004
#define EFI_FILE_RESERVED   0x0000000000000008
#define EFI_FILE_DIRECTORY  0x0000000000000010
#define EFI_FILE_ARCHIVE    0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037

// EFI_FILE_CLOSE: UEFI Spec 2.10 section 13.5.3
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_CLOSE) (
    IN EFI_FILE_PROTOCOL *This
);

// EFI_FILE_DELETE: UEFI Spec 2.10 section 13.5.4
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_DELETE) (
IN EFI_FILE_PROTOCOL *This
);

// EFI_FILE_READ: UEFI Spec 2.10 section 13.5.5
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ) (
    IN EFI_FILE_PROTOCOL *This,
    IN OUT UINTN         *BufferSize,
    OUT VOID             *Buffer
);

// EFI_FILE_WRITE: UEFI Spec 2.10 section 13.5.6
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE) (
    IN EFI_FILE_PROTOCOL *This,
    IN OUT UINTN         *BufferSize,
    IN VOID              *Buffer
);

// EFI_FILE_SET_POSITION: UEFI Spec 2.10 section 13.5.11
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_POSITION) (
    IN EFI_FILE_PROTOCOL *This,
    IN UINT64            Position
);

// EFI_FILE_GET_POSITION: UEFI Spec 2.10 section 13.5.12
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_POSITION) (
    IN EFI_FILE_PROTOCOL *This,
    OUT UINT64           *Position
);

// EFI_FILE_GET_INFO: UEFI Spec 2.10 section 13.5.13
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_INFO) (
    IN EFI_FILE_PROTOCOL *This,
    IN EFI_GUID          *InformationType,
    IN OUT UINTN         *BufferSize,
    OUT VOID             *Buffer
);

// EFI_FILE_SET_INFO: UEFI Spec 2.10 section 13.5.14
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_INFO) (
    IN EFI_FILE_PROTOCOL *This,
    IN EFI_GUID          *InformationType,
    IN UINTN             BufferSize,
    IN VOID              *Buffer
);

// EFI_FILE_FLUSH: UEFI Spec 2.10 section 13.5.15
typedef
EFI_STATUS
(EFIAPI *EFI_FILE_FLUSH) (
    IN EFI_FILE_PROTOCOL *This
);

// EFI_FILE_INFO: UEFI Spec 2.10 section 13.5.16
typedef struct {
    UINT64   Size;
    UINT64   FileSize;
    UINT64   PhysicalSize;
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    UINT64   Attribute;
    //CHAR16   FileName [];
    CHAR16 FileName [256];  // Maybe TODO: change to dynamically allocate memory for these?
} EFI_FILE_INFO;

// File Attribute Bits
#define EFI_FILE_READ_ONLY  0x0000000000000001
#define EFI_FILE_HIDDEN     0x0000000000000002
#define EFI_FILE_SYSTEM     0x0000000000000004
#define EFI_FILE_RESERVED   0x0000000000000008
#define EFI_FILE_DIRECTORY  0x0000000000000010
#define EFI_FILE_ARCHIVE    0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037

#define EFI_FILE_PROTOCOL_REVISION  0x00010000
#define EFI_FILE_PROTOCOL_REVISION2 0x00020000
#define EFI_FILE_PROTOCOL_LATEST_REVISION EFI_FILE_PROTOCOL_REVISION2
typedef struct EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN         Open;
    EFI_FILE_CLOSE        Close;
    EFI_FILE_DELETE       Delete;
    EFI_FILE_READ         Read;
    EFI_FILE_WRITE        Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO     GetInfo;
    EFI_FILE_SET_INFO     SetInfo;
    EFI_FILE_FLUSH        Flush;

    //EFI_FILE_OPEN_EX  OpenEx;  // Added for revision 2
    void *OpenEx;  
    //EFI_FILE_READ_EX  ReadEx;  // Added for revision 2
    void *ReadEx; 
    //EFI_FILE_WRITE_EX WriteEx; // Added for revision 2
    void *WriteEx;
    //EFI_FILE_FLUSH_EX FlushEx; // Added for revision 2
    void *FlushEx;
} EFI_FILE_PROTOCOL;

// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL: UEFI Spec 2.10 section 13.4.1
typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000

typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME) (
    IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    OUT EFI_FILE_PROTOCOL              **Root
);

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64                                      Revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

// EFI_BLOCK_IO_PROTOCOL: UEFI Spec 2.10 section 13.9
#define EFI_BLOCK_IO_PROTOCOL_REVISION2 0x00020001
#define EFI_BLOCK_IO_PROTOCOL_REVISION3 ((2<<16) | (31))

typedef struct EFI_BLOCK_IO_PROTOCOL EFI_BLOCK_IO_PROTOCOL;

// EFI_BLOCK_IO_MEDIA
typedef struct {
    UINT32  MediaId;
    BOOLEAN RemovableMedia;
    BOOLEAN MediaPresent;
    BOOLEAN LogicalPartition;
    BOOLEAN ReadOnly;
    BOOLEAN WriteCaching;
    UINT32  BlockSize;
    UINT32  IoAlign;
    EFI_LBA LastBlock;
    EFI_LBA LowestAlignedLba;                   // added in Revision 2
    UINT32  LogicalBlocksPerPhysicalBlock;      // added in Revision 2
    UINT32  OptimalTransferLengthGranularity;   // added in Revision 3
} EFI_BLOCK_IO_MEDIA;

// EFI_BLOCK_RESET: UEFI Spec 2.10 section 13.9.2
typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_RESET) (
    IN EFI_BLOCK_IO_PROTOCOL *This,
    IN BOOLEAN               ExtendedVerification
);

// EFI_BLOCK_READ: UEFI Spec 2.10 section 13.9.3
typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_READ) (
    IN EFI_BLOCK_IO_PROTOCOL *This,
    IN UINT32                MediaId,
    IN EFI_LBA               LBA,
    IN UINTN                 BufferSize,
    OUT VOID                 *Buffer
);

// EFI_BLOCK_WRITE: UEFI Spec 2.10 section 13.9.4
typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_WRITE) (
    IN EFI_BLOCK_IO_PROTOCOL *This,
    IN UINT32                MediaId,
    IN EFI_LBA               LBA,
    IN UINTN                 BufferSize,
    IN VOID                  *Buffer
);

// EFI_BLOCK_FLUSH: UEFI Spec 2.10 section 13.9.5
typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_FLUSH) (
    IN EFI_BLOCK_IO_PROTOCOL *This
);

typedef struct EFI_BLOCK_IO_PROTOCOL {
    UINT64             Revision;
    EFI_BLOCK_IO_MEDIA *Media;
    EFI_BLOCK_RESET    Reset;
    EFI_BLOCK_READ     ReadBlocks;
    EFI_BLOCK_WRITE    WriteBlocks;
    EFI_BLOCK_FLUSH    FlushBlocks;
} EFI_BLOCK_IO_PROTOCOL;

// EFI_DISK_IO_PROTOCOL: UEFI Spec 2.10 section 13.7.1
#define EFI_DISK_IO_PROTOCOL_REVISION 0x00010000

typedef struct EFI_DISK_IO_PROTOCOL EFI_DISK_IO_PROTOCOL; 

// EFI_DISK_READ: UEFI Spec 2.10 section 13.7.2
typedef
EFI_STATUS
(EFIAPI *EFI_DISK_READ) (
    IN EFI_DISK_IO_PROTOCOL *This,
    IN UINT32               MediaId,
    IN UINT64               Offset,
    IN UINTN                BufferSize,
    OUT VOID                *Buffer
);

// EFI_DISK_WRITE: UEFI Spec 2.10 section 13.7.3
typedef
EFI_STATUS
(EFIAPI *EFI_DISK_WRITE) (
    IN EFI_DISK_IO_PROTOCOL *This,
    IN UINT32               MediaId,
    IN UINT64               Offset,
    IN UINTN                BufferSize,
    IN VOID                 *Buffer
);

typedef struct EFI_DISK_IO_PROTOCOL {
    UINT64         Revision;
    EFI_DISK_READ  ReadDisk;
    EFI_DISK_WRITE WriteDisk;
} EFI_DISK_IO_PROTOCOL;

// EFI_PARTITION_INFO_PROTOCOL: UEFI Spec 2.10 section 13.18
#define EFI_PARTITION_INFO_PROTOCOL_REVISION 0x0001000
#define PARTITION_TYPE_OTHER                 0x00
#define PARTITION_TYPE_MBR                   0x01
#define PARTITION_TYPE_GPT                   0x02

// MBR Partition Entry: UEFI Spec 2.10 section 5.2.1
typedef struct {
    UINT8 BootIndicator;
    UINT8 StartHead;
    UINT8 StartSector;
    UINT8 StartTrack;
    UINT8 OSIndicator;
    UINT8 EndHead;
    UINT8 EndSector;
    UINT8 EndTrack;
    UINT8 StartingLBA[4];
    UINT8 SizeInLBA[4];
} __attribute__ ((packed)) MBR_PARTITION_RECORD;

// MBR Partition Table: UEFI Spec 2.10 section 5.2.1
typedef struct {
    UINT8                BootStrapCode[440];
    UINT8                UniqueMbrSignature[4];
    UINT8                Unknown[2];
    MBR_PARTITION_RECORD Partition[4];
    UINT16               Signature;
} __attribute__ ((packed)) MASTER_BOOT_RECORD;

// GPT Partition Entry: UEFI Spec 2.10 section 5.3.3
typedef struct {
    EFI_GUID PartitionTypeGUID;
    EFI_GUID UniquePartitionGUID;
    EFI_LBA  StartingLBA;
    EFI_LBA  EndingLBA;
    UINT64   Attributes;
    CHAR16   PartitionName[36];
} __attribute__ ((packed)) EFI_PARTITION_ENTRY;

typedef struct {
    UINT32 Revision;
    UINT32 Type;
    UINT8 System;
    UINT8 Reserved[7];
    union {
        // MBR data
        MBR_PARTITION_RECORD Mbr;

        // GPT data
        EFI_PARTITION_ENTRY Gpt;
    } Info;
} __attribute__ ((packed)) EFI_PARTITION_INFO_PROTOCOL;

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
    EFI_GET_TIME GetTime;
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
    void*         AllocatePages;
    void*         FreePages;
    void*         GetMemoryMap;
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL     FreePool;

    //
    // Event & Timer Services
    //
    EFI_CREATE_EVENT   CreateEvent;
    EFI_SET_TIMER      SetTimer;
    EFI_WAIT_FOR_EVENT WaitForEvent;
    void*              SignalEvent;
    EFI_CLOSE_EVENT    CloseEvent;
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
    void*                  GetNextMonotonicCount;
    void*                  Stall;
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;

    //
    // DriverSupport Services
    //
    void* ConnectController;
    void* DisconnectController;

    //
    // Open and Close Protocol Services
    //
    EFI_OPEN_PROTOCOL  OpenProtocol;
    EFI_CLOSE_PROTOCOL CloseProtocol;
    void* OpenProtocolInformation;

    //
    // Library Services
    //
    void*                    ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL      LocateProtocol;
    void*                    InstallMultipleProtocolInterfaces;
    void*                    UninstallMultipleProtocolInterfaces;

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

// EFI_LOADED_IMAGE_PROTOCOL: UEFI Spec 2.10 section 9.1.1
#define EFI_LOADED_IMAGE_PROTOCOL_REVISION 0x1000

typedef struct {
    UINT32           Revision;
    EFI_HANDLE       ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;

    // Source location of the image
    EFI_HANDLE               DeviceHandle;
    //EFI_DEVICE_PATH_PROTOCOL *FilePath;
    void                     *FilePath;
    VOID                     *Reserved;

    // Image’s load options
    UINT32 LoadOptionsSize;
    VOID   *LoadOptions;

    // Location where image was loaded
    VOID             *ImageBase;
    UINT64           ImageSize;
    EFI_MEMORY_TYPE  ImageCodeType;
    EFI_MEMORY_TYPE  ImageDataType;
    //EFI_IMAGE_UNLOAD Unload;
    void            *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;


