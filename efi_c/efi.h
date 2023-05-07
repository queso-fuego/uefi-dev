/*
 * NOTE: void* fields in structs = not implemented!!
 */

#include <uchar.h>
#include <stdint.h>

// Common UEFI Data Types: UEFI Spec 2.10 section 2.3.1
typedef uint16_t    UINT16;
typedef uint32_t    UINT32;
typedef uint64_t    UINT64;
typedef uint64_t    UINTN;
typedef char16_t    CHAR16;	// UTF-16, but should use UCS-2 code points 0x0000-0xFFFF
typedef void        VOID;

typedef UINTN       EFI_STATUS;
typedef VOID*       EFI_HANDLE;

// Taken from EDKII at
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Base.h
#define IN
#define OUT
#define OPTIONAL
#define CONST const

// Taken from gnu-efi at
// https://github.com/vathpela/gnu-efi/blob/master/inc/x86_64/efibind.h
#define EFIAPI __attribute__((ms_abi))  // x86_64 Microsoft calling convention

// EFI_STATUS codes: UEFI Spec 2.10 Appendix D
#define EFI_SUCCESS 0

// EFI Simple Text Input Protocol: UEFI Spec 2.10 section 12.3
// Forward declare struct for this to work and compile
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
    UINT16  ScanCode;
    CHAR16  UnicodeChar;
} EFI_INPUT_KEY;

typedef 
EFI_STATUS 
(EFIAPI *EFI_INPUT_READ_KEY) (
 IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This, 
 OUT EFI_INPUT_KEY                   *Key
);

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    void*               Reset;
    EFI_INPUT_READ_KEY  ReadKeyStroke;
    void*               WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// EFI Simple Text Output Protocol: UEFI Spec 2.10 section 12.4
// Forward declare struct for this to work and compile
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// Text attributes: UEFI Spec 2.10 section 12.4.7
#define EFI_BLACK  0x00
#define EFI_BLUE   0x01
#define EFI_GREEN  0x02
#define EFI_CYAN   0x03
#define EFI_RED    0x04
#define EFI_YELLOW 0x0E
#define EFI_WHITE  0x0F

// Only use 0x00-0x07 for background with this macro!
#define EFI_TEXT_ATTR(Foreground,Background) \
    ((Foreground) | ((Background) << 4))

typedef 
EFI_STATUS 
(EFIAPI *EFI_TEXT_STRING) (
 IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
 IN CHAR16                          *String
);

typedef 
EFI_STATUS 
(EFIAPI *EFI_TEXT_SET_ATTRIBUTE) (
 IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
 IN UINTN                           Attribute
);

typedef 
EFI_STATUS 
(EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
 IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void*                           Reset;
    EFI_TEXT_STRING                 OutputString;
    void*                           TestString;
    void*                           QueryMode;
    void*                           SetMode;
    EFI_TEXT_SET_ATTRIBUTE          SetAttribute;
    EFI_TEXT_CLEAR_SCREEN           ClearScreen;
    void*                           SetCursorPosition;
    void*                           EnableCursor;
    void*                           Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef 
VOID 
(EFIAPI *EFI_RESET_SYSTEM) (
   IN EFI_RESET_TYPE ResetType,      
   IN EFI_STATUS     ResetStatus,   
   IN UINTN          DataSize,     
   IN VOID           *ResetData OPTIONAL
);

// EFI Table Header: UEFI Spec 2.10 section 4.2
typedef struct {
    UINT64  Signature;
    UINT32  Revision;
    UINT32  HeaderSize;
    UINT32  CRC32;
    UINT32  Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    EFI_TABLE_HEADER Hdr;

    // Time Services
    void*                           GetTime; 
    void*                           SetTime; 
    void*                           GetWakeupTime; 
    void*                           SetWakeupTime; 

    // Virtual Memory Services
    void*                           SetVirtualAddressMap;
    void*                           ConvertPointer;

    // Variable Services
    void*                           GetVariable;
    void*                           GetNextVariableName;
    void*                           SetVariable;

    // Miscellaneous Services
    void*                           GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM                ResetSystem;

    // UEFI 2.0 Capsule Services
    void*                           UpdateCapsule;
    void*                           QueryCapsuleCapabilities;

    // Miscellaneous UEFI 2.0 Service
    void*                           QueryVariableInfo; 
} EFI_RUNTIME_SERVICES;

// EFI System Table: UEFI Spec 2.10 section 4.3
typedef struct {
    EFI_TABLE_HEADER                Hdr;

    void*                           FirmwareVendor;
    UINT32                          FirmwareRevision;
    void*                           ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL 	*ConIn;
	void*                           ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
	void*                           StandardErrorHandle;
	void*                           StdErr;
	EFI_RUNTIME_SERVICES            *RuntimeServices;
	void*                           BootServices;
	UINTN                           NumberOfTableEntries;
	void*                           ConfigurationTable;
} EFI_SYSTEM_TABLE;

