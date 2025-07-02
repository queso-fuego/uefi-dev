//
// NOTE: "void *" fields in structs = not implemented!!
//   They are defined in the UEFI spec, but I have not used
//   them or implemented them yet. Using void pointers 
//   ensures they take up the same amount of space so that
//   the actually defined functions work correctly and are 
//   at the correct offsets.
//
#pragma once

#include <stdint.h>
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

// Scancodes for SIMPLE_TEXT_INPUT_PROTOCOL & EX protocol; 
//  UEFI Spec 2.10A Appendix B.1
#define SCANCODE_UP_ARROW   0x1
#define SCANCODE_DOWN_ARROW 0x2
#define SCANCODE_ESC        0x17

#define EFI_SIMPLE_NETWORK_PROTOCOL_GUID \
{0xA19832B9,0xAC25,0x11D3,\
0x9A,0x2D,{0x00,0x90,0x27,0x3F,0xC1,0x4D}}

// -----------------------------------
// Misc. EFI GUIDs
// -----------------------------------
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

#define EFI_GLOBAL_VARIABLE_GUID \
{0x8BE4DF61,0x93CA,0x11d2,\
0xAA,0x0D,{0x00,0xE0,0x98,0x03,0x2B,0x8C}}

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
{0x09576e91,0x6d3f,0x11d2,\
0x8e,0x39,{0x00,0xa0,0xc9,0x69,0x72,0x3b}}

#define EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID \
{0x8b843e20,0x8132,0x4852,\
0x90,0xcc,{0x55,0x1a,0x4e,0x4a,0x7f,0x1c}}

// 34.8.1 EFI_HII_DATABASE_PROTOCOL
#define EFI_HII_DATABASE_PROTOCOL_GUID \
{0xef9fc172, 0xa1b2, 0x4693,\
0xb3, 0x27, {0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42}}

// -----------------------------------
// EFI Configuration Table GUIDs
// -----------------------------------
#define EFI_ACPI_TABLE_GUID \
{0x8868e871,0xe4f1,0x11d3,\
0xbc,0x22,{0x00,0x80,0xc7,0x3c,0x88,0x81}}

// ACPI 2.0 or newer tables should use EFI_ACPI_TABLE_GUID
#define EFI_ACPI_20_TABLE_GUID EFI_ACPI_TABLE_GUID

#define ACPI_TABLE_GUID \
{0xeb9d2d30,0x2d88,0x11d3,\
0x9a,0x16,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define ACPI_10_TABLE_GUID ACPI_TABLE_GUID

#define SAL_SYSTEM_TABLE_GUID \
{0xeb9d2d32,0x2d88,0x11d3,\
0x9a,0x16,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define SMBIOS_TABLE_GUID \
{0xeb9d2d31,0x2d88,0x11d3,\
0x9a,0x16,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

#define SMBIOS3_TABLE_GUID \
{0xf2fd1544, 0x9794, 0x4a2c,\
0x99,0x2e,{0xe5,0xbb,0xcf,0x20,0xe3,0x94}}

#define MPS_TABLE_GUID \
{0xeb9d2d2f,0x2d88,0x11d3,\
0x9a,0x16,{0x00,0x90,0x27,0x3f,0xc1,0x4d}}

// -----------------------------------
// Partition Type GUID values
// -----------------------------------
// EFI System Partition GUID
#define ESP_GUID \
{0xC12A7328, 0xF81F, 0x11D2, \
0xBA, 0x4B, {0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}}

// (Microsoft) Basic Data GUID
#define BASIC_DATA_GUID \
{0xEBD0A0A2, 0xB9E5, 0x4433, \
0x87, 0xC0, {0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}}

#define EFI_PCI_IO_PROTOCOL_GUID \
{0x4cf5b200,0x68b8,0x4ca5, \
0x9e,0xec, {0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a}}

// EFI_STATUS Codes - UEFI Spec 2.10 Appendix D
#define EFI_SUCCESS 0ULL

#define TOP_BIT 0x8000000000000000
#define ENCODE_ERROR(x) (TOP_BIT | (x))
#define EFI_ERROR(x) ((INTN)((UINTN)(x)) < 0)

#define EFI_UNSUPPORTED      ENCODE_ERROR(3)
#define EFI_BUFFER_TOO_SMALL ENCODE_ERROR(5)
#define EFI_DEVICE_ERROR     ENCODE_ERROR(7)
#define EFI_NOT_FOUND        ENCODE_ERROR(14)
#define EFI_CRC_ERROR        ENCODE_ERROR(27)

#define MAX_EFI_ERROR 36
const CHAR16 *EFI_ERROR_STRINGS[MAX_EFI_ERROR] = {
    [3]  = u"EFI_UNSUPPORTED",
    [5]  = u"EFI_BUFFER_TOO_SMALL",
    [7]  = u"EFI_DEVICE_ERROR",
    [14] = u"EFI_NOT_FOUND",
    [27] = u"EFI_CRC_ERROR",
};

typedef struct _EFI_PCI_IO_PROTOCOL EFI_PCI_IO_PROTOCOL;

// EFI_SIMPLE_NETWORK_PROTOCOL
typedef struct EFI_SIMPLE_NETWORK_PROTOCOL EFI_SIMPLE_NETWORK_PROTOCOL;
typedef struct {
  UINT8                                     Addr[32];
} EFI_MAC_ADDRESS;
typedef struct {
  UINT8                                     Addr[4];
} EFI_IPv4_ADDRESS;
typedef struct {
  UINT8                                     Addr[16];
} EFI_IPv6_ADDRESS;
typedef union {
  UINT32                                    Addr[4];
  EFI_IPv4_ADDRESS                          v4;
  EFI_IPv6_ADDRESS                          v6;
} EFI_IP_ADDRESS;
typedef enum {
  EfiSimpleNetworkStopped,
  EfiSimpleNetworkStarted,
  EfiSimpleNetworkInitialized,
  EfiSimpleNetworkMaxState
} EFI_SIMPLE_NETWORK_STATE;

#define MAX_MCAST_FILTER_CNT                16

typedef struct {
  UINT32                                    State;
  UINT32                                    HwAddressSize;
  UINT32                                    MediaHeaderSize;
  UINT32                                    MaxPacketSize;
  UINT32                                    NvRamSize;
  UINT32                                    NvRamAccessSize;
  UINT32                                    ReceiveFilterMask;
  UINT32                                    ReceiveFilterSetting;
  UINT32                                    MaxMCastFilterCount;
  UINT32                                    MCastFilterCount;
  EFI_MAC_ADDRESS                           MCastFilter[MAX_MCAST_FILTER_CNT];
  EFI_MAC_ADDRESS                           CurrentAddress;
  EFI_MAC_ADDRESS                           BroadcastAddress;
  EFI_MAC_ADDRESS                           PermanentAddress;
  UINT8                                     IfType;
  BOOLEAN                                   MacAddressChangeable;
  BOOLEAN                                   MultipleTxSupported;
  BOOLEAN                                   MediaPresentSupported;
  BOOLEAN                                   MediaPresent;
} EFI_SIMPLE_NETWORK_MODE;

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_START) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_STOP) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_INITIALIZE) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN UINTN                                  ExtraRxBufferSize OPTIONAL,
  IN UINTN                                  ExtraTxBufferSize OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_RESET) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN BOOLEAN                                ExtendedVerification
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_SHUTDOWN) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_RECEIVE_FILTERS) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN UINT32                                 Enable,
  IN UINT32                                 Disable,
  IN BOOLEAN                                ResetMCastFilter,
  IN UINTN                                  MCastFilterCnt OPTIONAL,
  IN EFI_MAC_ADDRESS                        MCastFilter OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_STATION_ADDRESS) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN BOOLEAN                                Reset,
  IN EFI_MAC_ADDRESS*                       New OPTIONAL
);

#define EFI_SIMPLE_NETWORK_RECEIVE_UNICAST               0x01
#define EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST             0x02
#define EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST             0x04
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS           0x08
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST 0x10

typedef struct {
  UINT64    RxTotalFrames;
  UINT64    RxGoodFrames;
  UINT64    RxUndersizeFrames;
  UINT64    RxOversizeFrames;
  UINT64    RxDroppedFrames;
  UINT64    RxUnicastFrames;
  UINT64    RxBroadcastFrames;
  UINT64    RxMulticastFrames;
  UINT64    RxCrcErrorFrames;
  UINT64    RxTotalBytes;
  UINT64    TxTotalFrames;
  UINT64    TxGoodFrames;
  UINT64    TxUndersizeFrames;
  UINT64    TxOversizeFrames;
  UINT64    TxDroppedFrames;
  UINT64    TxUnicastFrames;
  UINT64    TxBroadcastFrames;
  UINT64    TxMulticastFrames;
  UINT64    TxCrcErrorFrames;
  UINT64    TxTotalBytes;
  UINT64    Collisions;
  UINT64    UnsupportedProtocol;
  UINT64    RxDuplicatedFrames;
  UINT64    RxDecryptErrorFrames;
  UINT64    TxErrorFrames;
  UINT64    TxRetryFrames;
} EFI_NETWORK_STATISTICS;

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_STATISTICS) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN BOOLEAN                                Reset,
  IN OUT UINTN*                             StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS*               StatisticsTable OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN BOOLEAN                                IPv6,
  IN EFI_IP_ADDRESS*                        IP,
  OUT EFI_MAC_ADDRESS*                      MAC
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_NVDATA) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN BOOLEAN                                ReadWrite,
  IN UINTN                                  Offset,
  IN UINTN                                  BufferSize,
  IN OUT VOID*                              Buffer
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_GET_STATUS) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  OUT UINT32*                               InterruptStatus OPTIONAL,
  OUT VOID**                                TxBuf OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_TRANSMIT) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  IN UINTN                                  HeaderSize,
  IN UINTN                                  BufferSize,
  IN VOID*                                  Buffer,
  IN EFI_MAC_ADDRESS*                       SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS*                       DestAddr OPTIONAL,
  IN UINT16*                                Protocol OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_SIMPLE_NETWORK_RECEIVE) (
  IN EFI_SIMPLE_NETWORK_PROTOCOL*           This,
  OUT UINTN*                                HeaderSize OPTIONAL,
  IN OUT UINTN*                             BufferSize,
  OUT VOID*                                 Buffer,
  OUT EFI_MAC_ADDRESS*                      SrcAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS*                      DestAddr OPTIONAL,
  OUT UINT16*                               Protocol OPTIONAL
);

typedef struct EFI_SIMPLE_NETWORK_PROTOCOL {
  UINT64                                    Revision;
  EFI_SIMPLE_NETWORK_START                  Start;
  EFI_SIMPLE_NETWORK_STOP                   Stop;
  EFI_SIMPLE_NETWORK_INITIALIZE             Initialize;
  EFI_SIMPLE_NETWORK_RESET                  Reset;
  EFI_SIMPLE_NETWORK_SHUTDOWN               Shutdown;
  EFI_SIMPLE_NETWORK_RECEIVE_FILTERS        ReceiveFilters;
  EFI_SIMPLE_NETWORK_STATION_ADDRESS        StationAddress;
  EFI_SIMPLE_NETWORK_STATISTICS             Statistics;
  EFI_SIMPLE_NETWORK_MCAST_IP_TO_MAC        MCastIpToMac;
  EFI_SIMPLE_NETWORK_NVDATA                 NvData;
  EFI_SIMPLE_NETWORK_GET_STATUS             GetStatus;
  EFI_SIMPLE_NETWORK_TRANSMIT               Transmit;
  EFI_SIMPLE_NETWORK_RECEIVE                Receive;
  EFI_EVENT                                 WaitForPacket;
  EFI_SIMPLE_NETWORK_MODE*                  Mode;
} EFI_SIMPLE_NETWORK_PROTOCOL;

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

//EFI_MEMORY_DESCRIPTOR
typedef struct {
    UINT32               Type;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS  VirtualStart;
    UINT64               NumberOfPages;
    UINT64               Attribute;
} EFI_MEMORY_DESCRIPTOR;

// Memory Attribute Definitions
// These types can be "ORed" together as needed.
#define EFI_MEMORY_UC            0x0000000000000001
#define EFI_MEMORY_WC            0x0000000000000002
#define EFI_MEMORY_WT            0x0000000000000004
#define EFI_MEMORY_WB            0x0000000000000008
#define EFI_MEMORY_UCE           0x0000000000000010
#define EFI_MEMORY_WP            0x0000000000001000
#define EFI_MEMORY_RP            0x0000000000002000
#define EFI_MEMORY_XP            0x0000000000004000
#define EFI_MEMORY_NV            0x0000000000008000
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000010000
#define EFI_MEMORY_RO            0x0000000000020000
#define EFI_MEMORY_SP            0x0000000000040000
#define EFI_MEMORY_CPU_CRYPTO    0x0000000000080000
#define EFI_MEMORY_RUNTIME       0x8000000000000000
#define EFI_MEMORY_ISA_VALID     0x4000000000000000
#define EFI_MEMORY_ISA_MASK      0x0FFFF00000000000

//EFI_ALLOCATE_TYPE
typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

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

// EFI_ALLOCATE_PAGES: UEFI Spec 2.10 section 7.2.1
typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES) (
    IN EFI_ALLOCATE_TYPE        Type,
    IN EFI_MEMORY_TYPE          MemoryType,
    IN UINTN                    Pages,
    IN OUT EFI_PHYSICAL_ADDRESS *Memory
); 

// EFI_FREE_PAGES: UEFI Spec 2.10 section 7.2.2
typedef
EFI_STATUS
(EFIAPI *EFI_FREE_PAGES) (
    IN EFI_PHYSICAL_ADDRESS Memory,
    IN UINTN                Pages
);

// EFI_GET_MEMORY_MAP: UEFI Spec 2.10 section 7.2.3
typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_MAP) (
    IN OUT UINTN              *MemoryMapSize,
    OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
    OUT UINTN                 *MapKey,
    OUT UINTN                 *DescriptorSize,
    OUT UINT32                *DescriptorVersion
);

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

typedef enum {
    EfiPciIoWidthUint8,
    EfiPciIoWidthUint16,
    EfiPciIoWidthUint32,
    EfiPciIoWidthUint64,
    EfiPciIoWidthFifoUint8,
    EfiPciIoWidthFifoUint16,
    EfiPciIoWidthFifoUint32,
    EfiPciIoWidthFifoUint64,
    EfiPciIoWidthFillUint8,
    EfiPciIoWidthFillUint16,
    EfiPciIoWidthFillUint32,
    EfiPciIoWidthFillUint64,
    EfiPciIoWidthMaximum
} EFI_PCI_IO_PROTOCOL_WIDTH;

#define EFI_PCI_IO_PASS_THROUGH_BAR 0xff

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_POLL_IO_MEM) (
  IN EFI_PCI_IO_PROTOCOL*       This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINT8                      BarIndex,
  IN UINT64                     Offset,
  IN UINT64                     Mask,
  IN UINT64                     Value,
  IN UINT64                     Delay,
  OUT UINT64*                   Result
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_IO_MEM) (
  IN EFI_PCI_IO_PROTOCOL*       This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINT8                      BarIndex,
  IN UINT64                     Offset,
  IN UINTN                      Count,
  IN OUT VOID*                  Buffer
);

typedef struct {
  EFI_PCI_IO_PROTOCOL_IO_MEM    Read;
  EFI_PCI_IO_PROTOCOL_IO_MEM    Write;
} EFI_PCI_IO_PROTOCOL_ACCESS;

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_CONFIG) (
  IN EFI_PCI_IO_PROTOCOL*       This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINT32                     Offset,
  IN UINTN                      Count,
  IN OUT VOID*                  Buffer
);

typedef struct {
  EFI_PCI_IO_PROTOCOL_CONFIG    Read;
  EFI_PCI_IO_PROTOCOL_CONFIG    Write;
} EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS;

#define EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO   0x0000
#define EFI_PCI_IO_ATTRIBUTE_ISA_IO               0x0001
#define EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO       0x0004
#define EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY           0x0008
#define EFI_PCI_IO_ATTRIBUTE_VGA_IO               0x0010
#define EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO       0x0020
#define EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO     0x0040
#define EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE 0x0080
#define EFI_PCI_IO_ATTRIBUTE_IO                   0x0100
#define EFI_PCI_IO_ATTRIBUTE_MEMORY               0x0200
#define EFI_PCI_IO_ATTRIBUTE_BUS_MASTER           0x0400
#define EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED        0x0800
#define EFI_PCI_IO_ATTRIBUTE_MEMORY_DISABLE       0x1000
#define EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE      0x2000
#define EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM         0x4000
#define EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE   0x8000
#define EFI_PCI_IO_ATTRIBUTE_ISA_IO_16            0x10000
#define EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16    0x20000
#define EFI_PCI_IO_ATTRIBUTE_VGA_IO_16            0x40000

typedef enum {
  EfiPciIoOperationBusMasterRead,
  EfiPciIoOperationBusMasterWrite,
  EfiPciIoOperationBusMasterCommonBuffer,
  EfiPciIoOperationBusMaximum
} EFI_PCI_IO_PROTOCOL_OPERATION;

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_POLL_IO_MEM) (
  IN EFI_PCI_IO_PROTOCOL*      This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH Width,
  UINT8                        BarIndex,
  UINT64                       Offset,
  UINT64                       Mask,
  UINT64                       Value,
  UINT64                       Delay,
  OUT UINT64*                  Result
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_MEM) (
  IN EFI_PCI_IO_PROTOCOL*      This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH Width,
  IN UINT8                     BarIndex,
  IN UINT64                    Offset,
  IN UINTN                     Count,
  IN OUT VOID*                 Buffer
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_CONFIG) (
  IN EFI_PCI_IO_PROTOCOL*      This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH Width,
  IN UINT32                    Offset,
  IN UINTN                     Count,
  IN OUT VOID*                 Buffer
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_COPY_MEM) (
  IN EFI_PCI_IO_PROTOCOL*      This,
  IN EFI_PCI_IO_PROTOCOL_WIDTH Width,
  IN UINT8                     DestBarIndex,
  IN UINT64                    DestOffset,
  IN UINT8                     SrcBarIndex,
  IN UINT64                    SrcOffset,
  IN UINTN                     Count
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_MAP) (
  IN EFI_PCI_IO_PROTOCOL*          This,
  IN EFI_PCI_IO_PROTOCOL_OPERATION Operation,
  IN VOID*                         HostAddress,
  IN OUT UINTN*                    NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS*        DeviceAddress,
  OUT VOID**                       Mapping
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_UNMAP) (
  IN EFI_PCI_IO_PROTOCOL*          This,
  IN VOID*                         Mapping
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER) (
  IN EFI_PCI_IO_PROTOCOL*          This,
  IN EFI_ALLOCATE_TYPE             Type,
  IN EFI_MEMORY_TYPE               MemoryType,
  IN UINTN                         Pages,
  OUT VOID**                       HostAddress,
  IN UINT64                        Attributes
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_FREE_BUFFER) (
  IN EFI_PCI_IO_PROTOCOL*          This,
  IN UINTN                         Pages,
  IN VOID*                         HostAddress
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_FLUSH) (
  IN EFI_PCI_IO_PROTOCOL*          This
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_GET_LOCATION) (
  IN EFI_PCI_IO_PROTOCOL*          This,
  OUT UINTN*                       SegmentNumber,
  OUT UINTN*                       BusNumber,
  OUT UINTN*                       DeviceNumber,
  OUT UINTN*                       FunctionNumber
);

typedef enum {
  EfiPciIoAttributeOperationGet,
  EfiPciIoAttributeOperationSet,
  EfiPciIoAttributeOperationEnable,
  EfiPciIoAttributeOperationDisable,
  EfiPciIoAttribtueOperationSupported,
  EfiPciIoAttributeOperationMaximum
} EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION;

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_ATTRIBUTES) (
  IN EFI_PCI_IO_PROTOCOL*                    This,
  IN EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION Operation,
  IN UINT64                                  Attributes,
  OUT UINT64*                                Result OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES) (
  IN EFI_PCI_IO_PROTOCOL*        This,
  IN UINT8                       BarIndex,
  OUT UINT64*                    Supports OPTIONAL,
  OUT VOID**                     Resources OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI* EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES) (
  IN EFI_PCI_IO_PROTOCOL*       This,
  IN UINT64                     Attributes,
  IN UINT8                      BarIndex,
  IN OUT UINT64*                Offset,
  IN OUT UINT64*                Length
);

typedef struct _EFI_PCI_IO_PROTOCOL {
    EFI_PCI_IO_PROTOCOL_POLL_IO_MEM        PollMem;
    EFI_PCI_IO_PROTOCOL_POLL_IO_MEM        PollIo;
    EFI_PCI_IO_PROTOCOL_ACCESS             Mem;
    EFI_PCI_IO_PROTOCOL_ACCESS             Io;
    EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS      Pci;
    EFI_PCI_IO_PROTOCOL_COPY_MEM           CopyMem;
    EFI_PCI_IO_PROTOCOL_MAP                Map;
    EFI_PCI_IO_PROTOCOL_UNMAP              Unmap;
    EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER    AllocateBuffer;
    EFI_PCI_IO_PROTOCOL_FREE_BUFFER        FreeBuffer;
    EFI_PCI_IO_PROTOCOL_FLUSH              Flush;
    EFI_PCI_IO_PROTOCOL_GET_LOCATION       GetLocation;
    EFI_PCI_IO_PROTOCOL_ATTRIBUTES         Attributes;
    EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES GetBarAttributes;
    EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES SetBarAttributes;
    UINT64                                 RomSize;
    VOID*                                  RomImage;
} EFI_PCI_IO_PROTOCOL;

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

// EFI_EXIT_BOOT_SERVICES: UEFI Spec 2.10 section 7.4.6
typedef
EFI_STATUS
(EFIAPI *EFI_EXIT_BOOT_SERVICES) (
    IN EFI_HANDLE ImageHandle,
    IN UINTN      MapKey
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

// EFI_DEVICE_PATH_PROTOCOL: UEFI Spec 2.10 Errata A section 10.2
typedef struct _EFI_DEVICE_PATH_PROTOCOL {
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

// EFI_CONNECT_CONTROLLER: UEFI Spec 2.10 Errata A section 7.3.12
typedef
EFI_STATUS
(EFIAPI *EFI_CONNECT_CONTROLLER) (
    IN EFI_HANDLE               ControllerHandle,
    IN EFI_HANDLE               *DriverImageHandle OPTIONAL,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL,
    IN BOOLEAN                  Recursive
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

// EFI_SET_VIRTUAL_ADDRESS_MAP: UEFI Spec 2.10 section 8.4.1
typedef
EFI_STATUS
(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP) (
    IN UINTN                 MemoryMapSize,
    IN UINTN                 DescriptorSize,
    IN UINT32                DescriptorVersion,
    IN EFI_MEMORY_DESCRIPTOR *VirtualMap
);

// EFI_LOAD_OPTION: UEFI Spec 2.10 Errata A section 3.1.3
// This is the data layout for a Boot#### variable, and maybe other ones
typedef struct _EFI_LOAD_OPTION {
    UINT32 Attributes;
    UINT16 FilePathListLength;
    // CHAR16 Description[];
    // EFI_DEVICE_PATH_PROTOCOL FilePathList[];
    // UINT8 OptionalData[];
} EFI_LOAD_OPTION;

// BootOptionSupport bitmasks
#define EFI_BOOT_OPTION_SUPPORT_KEY     0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP     0x00000002
#define EFI_BOOT_OPTION_SUPPORT_SYSPREP 0x00000010
#define EFI_BOOT_OPTION_SUPPORT_COUNT   0x00000300

// EFI_GET_VARIABLE: UEFI Spec 2.10 Errata A section 8.2.2
typedef
EFI_STATUS
(EFIAPI *EFI_GET_VARIABLE) (
    IN CHAR16    *VariableName,
    IN EFI_GUID  *VendorGuid,
    OUT UINT32   *Attributes OPTIONAL,
    IN OUT UINTN *DataSize,
    OUT VOID     *Data OPTIONAL
);

// Variable Attributes
#define EFI_VARIABLE_NON_VOLATILE           0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS     0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS         0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD  0x00000008 
//This attribute is identified by the mnemonic 'HR' elsewhere in the spec

// Reserved                                                0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE                          0x00000040
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS         0x00000080
//This attribute indicates that the variable payload begins
//with an EFI_VARIABLE_AUTHENTICATION_3 structure, and
//potentially more structures as indicated by fields of this
//structure. See definition in SetVariable().

// EFI_GET_NEXT_VARIABLE_NAME: UEFI Spec 2.10 Errata A section 8.2.2
typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME) (
    IN OUT UINTN    *VariableNameSize,
    IN OUT CHAR16   *VariableName,
    IN OUT EFI_GUID *VendorGuid
);

// EFI_SET_VARIABLE: UEFI Spec 2.10 Errata A section 8.2.3
typedef
EFI_STATUS
(EFIAPI *EFI_SET_VARIABLE) (
    IN CHAR16   *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32   Attributes,
    IN UINTN    DataSize,
    IN VOID     *Data
);

// EFI_DEVICE_PATH_TO_TEXT_NODE: UEFI Spec 2.10 Errata A section 10.6.3
typedef
CHAR16*
(EFIAPI *EFI_DEVICE_PATH_TO_TEXT_NODE) (
    IN CONST EFI_DEVICE_PATH_PROTOCOL *DeviceNode,
    IN BOOLEAN                         DisplayOnly,
    IN BOOLEAN                         AllowShortcuts
);

// EFI_DEVICE_PATH_TO_TEXT_PATH: UEFI Spec 2.10 Errata A section 10.6.4
typedef
CHAR16*
(EFIAPI *EFI_DEVICE_PATH_TO_TEXT_PATH) (
    IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    IN BOOLEAN                         DisplayOnly,
    IN BOOLEAN                         AllowShortcuts
);

// EFI_DEVICE_PATH_TO_TEXT_PROTOCOL: UEFI Spec 2.10 Errata A section 10.6.2
typedef struct _EFI_DEVICE_PATH_TO_TEXT_PROTOCOL {
    EFI_DEVICE_PATH_TO_TEXT_NODE ConvertDeviceNodeToText;
    EFI_DEVICE_PATH_TO_TEXT_PATH ConvertDevicePathToText;
} EFI_DEVICE_PATH_TO_TEXT_PROTOCOL;

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
    EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
    void                        *ConvertPointer;
    
    //
    // Variable Services
    //
    EFI_GET_VARIABLE           GetVariable;
    EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
    EFI_SET_VARIABLE           SetVariable;
    
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
    EFI_ALLOCATE_PAGES AllocatePages;
    EFI_FREE_PAGES     FreePages;
    EFI_GET_MEMORY_MAP GetMemoryMap;
    EFI_ALLOCATE_POOL  AllocatePool;
    EFI_FREE_POOL      FreePool;

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
    void*                  LoadImage;
    void*                  StartImage;
    void*                  Exit;
    void*                  UnloadImage;
    EFI_EXIT_BOOT_SERVICES ExitBootServices;

    //
    // Miscellaneous Services
    //
    void*                  GetNextMonotonicCount;
    void*                  Stall;
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;

    //
    // DriverSupport Services
    //
    EFI_CONNECT_CONTROLLER ConnectController;
    void                   *DisconnectController;

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

// EFI_CONFIGURATION_TABLE: UEFI_Spec 2.10 section 4.6.1
typedef struct {
    EFI_GUID VendorGuid; 
    VOID     *VendorTable; 
} EFI_CONFIGURATION_TABLE;

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
    EFI_CONFIGURATION_TABLE         *ConfigurationTable;
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

// HII PACKAGE HEADER: UEFI Spec 2.10A section 33.3.1.1
typedef struct {
    UINT32 Length:24;
    UINT32 Type:8;
    // UINT8 Data[...];
} EFI_HII_PACKAGE_HEADER;

// HII PACKAGE TYPES
#define EFI_HII_PACKAGE_TYPE_ALL          0x00
#define EFI_HII_PACKAGE_TYPE_GUID         0x01
#define EFI_HII_PACKAGE_FORMS             0x02
#define EFI_HII_PACKAGE_STRINGS           0x04
#define EFI_HII_PACKAGE_FONTS             0x05
#define EFI_HII_PACKAGE_IMAGES            0x06
#define EFI_HII_PACKAGE_SIMPLE_FONTS      0x07
#define EFI_HII_PACKAGE_DEVICE_PATH       0x08
#define EFI_HII_PACKAGE_KEYBOARD_LAYOUT   0x09
#define EFI_HII_PACKAGE_ANIMATIONS        0x0A
#define EFI_HII_PACKAGE_END               0xDF
#define EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN 0xE0
#define EFI_HII_PACKAGE_TYPE_SYSTEM_END   0xFF

// HII PACKAGE LIST HEADER: UEFI Spec 2.10A section 33.3.1.2
typedef struct {
    EFI_GUID PackageListGuid;
    UINT32   PackageLength;
} EFI_HII_PACKAGE_LIST_HEADER;

// HII SIMPLE FONT PACKAGE HEADER: UEFI Spec 2.10A section 33.3.2
typedef struct {
    EFI_HII_PACKAGE_HEADER Header;
    UINT16                 NumberOfNarrowGlyphs;
    UINT16                 NumberOfWideGlyphs;
    // EFI_NARROW_GLYPH NarrowGlyphs[];
    // EFI_WIDE_GLYPH WideGlyphs[];
} EFI_HII_SIMPLE_FONT_PACKAGE_HDR;

// Contents of EFI_NARROW_GLYPH.Attributes
#define EFI_GLYPH_NON_SPACING 0x01
#define EFI_GLYPH_WIDE        0x02
#define EFI_GLYPH_HEIGHT      19
#define EFI_GLYPH_WIDTH       8

// EFI NARROW GLYPH: UEFI Spec 2.10A section 33.3.2.2   
// should be 8x19 monospaced bitmap font
typedef struct {
    CHAR16 UnicodeWeight;
    UINT8  Attributes;
    UINT8  GlyphCol1[EFI_GLYPH_HEIGHT];
} EFI_NARROW_GLYPH;

// EFI WIDE GLYPH: UEFI Spec 2.10A section 33.3.2.3   
// should be 16x19 monospaced bitmap font
typedef struct {
    CHAR16 UnicodeWeight;
    UINT8  Attributes;
    UINT8  GlyphCol1[EFI_GLYPH_HEIGHT];
    UINT8  GlyphCol2[EFI_GLYPH_HEIGHT];
    UINT8  Pad[3];
} EFI_WIDE_GLYPH;

// NOTE: This is _not_ an EFI_HANDLE!
typedef void *EFI_HII_HANDLE;

// Forward declare for function declarations to work
typedef struct EFI_HII_DATABASE_PROTOCOL EFI_HII_DATABASE_PROTOCOL;

// EFI_HII_DATABASE_LIST_PACKS: UEFI Spec 2.10A section 34.8.5
typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_LIST_PACKS) (
    IN CONST EFI_HII_DATABASE_PROTOCOL *This,
    IN UINT8                           PackageType,
    IN CONST EFI_GUID                  *PackageGuid,
    IN OUT UINTN                       *HandleBufferLength,
    OUT EFI_HII_HANDLE                 *Handle
);

// EFI_HII_DATABASE_EXPORT_PACKS: UEFI Spec 2.10A section 34.8.6
typedef 
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_EXPORT_PACKS) (
    IN CONST EFI_HII_DATABASE_PROTOCOL *This,
    IN EFI_HII_HANDLE                  Handle,
    IN OUT UINTN                       *BufferSize,
    OUT EFI_HII_PACKAGE_LIST_HEADER    *Buffer
);

// HII DATABASE PROTOCOL: UEFI Spec 2.10A section 34.8.1
typedef struct EFI_HII_DATABASE_PROTOCOL {
    //EFI_HII_DATABASE_NEW_PACK          NewPackageList;
    //EFI_HII_DATABASE_REMOVE_PACK       RemovePackageList;
    //EFI_HII_DATABASE_UPDATE_PACK       UpdatePackageList;
    void                               *NewPackageList;
    void                               *RemovePackageList;
    void                               *UpdatePackageList;

    EFI_HII_DATABASE_LIST_PACKS        ListPackageLists;
    EFI_HII_DATABASE_EXPORT_PACKS      ExportPackageLists;

    //EFI_HII_DATABASE_REGISTER_NOTIFY   RegisterPackageNotify;
    //EFI_HII_DATABASE_UNREGISTER_NOTIFY UnregisterPackageNotify;
    //EFI_HII_FIND_KEYBOARD_LAYOUTS      FindKeyboardLayouts;
    //EFI_HII_GET_KEYBOARD_LAYOUT        GetKeyboardLayout;
    //EFI_HII_SET_KEYBOARD_LAYOUT        SetKeyboardLayout;
    //EFI_HII_DATABASE_GET_PACK_HANDLE   GetPackageListHandle;
    void                               *RegisterPackageNotify;
    void                               *UnregisterPackageNotify;
    void                               *FindKeyboardLayouts;
    void                               *GetKeyboardLayout;
    void                               *SetKeyboardLayout;
    void                               *GetPackageListHandle;
} EFI_HII_DATABASE_PROTOCOL;

