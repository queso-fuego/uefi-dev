//
// NOTE: "void *" fields in structs = not implemented!!
//   They are defined in the UEFI spec, but I have not used
//   them or implemented them yet
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

// EFI_STATUS Codes - UEFI Spec 2.10 Appendix D
#define EFI_SUCCESS 0ULL

// TODO: Add EFI_ERROR() macro/other for checking if EFI_STATUS
//   high bit is set, if so it is an ERROR status

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
#define EFI_DARKGRAY (EFI_BLACK | EFI_BRIGHT) 0x08
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
    EFI_TEXT_RESET          Reset;
    EFI_TEXT_STRING         OutputString;
    void                    *TestString;
    EFI_TEXT_QUERY_MODE     QueryMode;
    EFI_TEXT_SET_MODE       SetMode;
    EFI_TEXT_SET_ATTRIBUTE  SetAttribute;
    EFI_TEXT_CLEAR_SCREEN   ClearScreen;
    void                    *SetCursorPosition;
    void                    *EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// EFI_TABLE_HEADER: UEFI Spec 2.10 section 4.2.1
typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

// EFI_SYSTEM_TABLE: UEFI Spec 2.10 section 4.3.1
typedef struct {
    EFI_TABLE_HEADER                Hdr;
    CHAR16                          *FirmwareVendor;
    UINT32                          FirmwareRevision;
    EFI_HANDLE                      ConsoleInHandle;
    //EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *ConIn;
    void                            *ConIn;
    EFI_HANDLE                      ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE                      StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    //EFI_RUNTIME_SERVICES            *RuntimeServices;
    void                            *RuntimeServices;
    //EFI_BOOT_SERVICES               *BootServices;
    void                            *BootServices;
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


