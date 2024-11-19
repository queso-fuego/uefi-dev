//
// efi_lib.h: EFI related helper definitions and functions, outside of the scope and definitions 
//   in the UEFI specification or efi.h header.
//
#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "efi.h"

#define PAGE_SIZE 4096  // 4KiB

// ELF Header - x86_64
typedef struct {
    struct {
        UINT8  ei_mag0;
        UINT8  ei_mag1;
        UINT8  ei_mag2;
        UINT8  ei_mag3;
        UINT8  ei_class;
        UINT8  ei_data;
        UINT8  ei_version;
        UINT8  ei_osabi;
        UINT8  ei_abiversion;
        UINT8  ei_pad[7];
    } e_ident;

    UINT16 e_type;
    UINT16 e_machine;
    UINT32 e_version;
    UINT64 e_entry;
    UINT64 e_phoff;
    UINT64 e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize;
    UINT16 e_phentsize;
    UINT16 e_phnum;
    UINT16 e_shentsize;
    UINT16 e_shnum;
    UINT16 e_shstrndx;
} __attribute__ ((packed)) ELF_Header_64;

// ELF Program Header - x86_64
typedef struct {
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
} __attribute__ ((packed)) ELF_Program_Header_64;

// Elf Header e_type values
typedef enum {
    ET_EXEC = 0x2,
    ET_DYN  = 0x3,
} ELF_EHEADER_TYPE;

// Elf Program header p_type values
typedef enum {
    PT_NULL = 0x0,
    PT_LOAD = 0x1,  // Loadable
} ELF_PHEADER_TYPE;

// PE Structs/types
// PE32+ COFF File Header
typedef struct {
    UINT16 Machine;             // 0x8664 = x86_64
    UINT16 NumberOfSections;    // # of sections to load for program
    UINT32 TimeDateStamp;
    UINT32 PointerToSymbolTable;
    UINT32 NumberOfSymbols;
    UINT16 SizeOfOptionalHeader;
    UINT16 Characteristics;
} __attribute__ ((packed)) PE_Coff_File_Header_64;

// COFF File Header Characteristics
typedef enum {
    IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002,
} PE_COFF_CHARACTERISTICS;

// PE32+ Optional Header
typedef struct {
    UINT16 Magic;                       // 0x10B = PE32, 0x20B = PE32+
    UINT8  MajorLinkerVersion;
    UINT8  MinorLinkerVersion;
    UINT32 SizeOfCode;
    UINT32 SizeOfInitializedData;
    UINT32 SizeOfUninitializedData;
    UINT32 AddressOfEntryPoint;         // Entry Point address (RVA from image base in memory)
    UINT32 BaseOfCode;
    UINT64 ImageBase;
    UINT32 SectionAlignment;
    UINT32 FileAlignment;
    UINT16 MajorOperatingSystemVersion;
    UINT16 MinorOperatingSystemVersion;
    UINT16 MajorImageVersion;
    UINT16 MinorImageVersion;
    UINT16 MajorSubsystemVersion;
    UINT16 MinorSubsystemVersion;
    UINT32 Win32VersionValue;
    UINT32 SizeOfImage;                 // Size in bytes of entire file (image) incl. headers
    UINT32 SizeOfHeaders;
    UINT32 CheckSum;
    UINT16 Subsystem;
    UINT16 DllCharacteristics;
    UINT64 SizeOfStackReserve;
    UINT64 SizeOfStackCommit;
    UINT64 SizeOfHeapReserve;
    UINT64 SizeOfHeapCommit;
    UINT32 LoaderFlags;
    UINT32 NumberOfRvaAndSizes;
} __attribute__ ((packed)) PE_Optional_Header_64;

// Optional Header DllCharacteristics
typedef enum {
    IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE = 0x0040, // PIE executable
} PE_OPTIONAL_HEADER_DLLCHARACTERISTICS;

// PE32+ Section Headers - Immediately following the Optional Header
typedef struct {
    UINT64 Name;                                // 8 byte, null padded UTF-8 string
    UINT32 VirtualSize;                         // Size in memory; If >SizeOfRawData, the difference is 0-padded
    UINT32 VirtualAddress;                      // RVA from image base in memory
    UINT32 SizeOfRawData;                       // Size of actual data (similar to ELF FileSize)
    UINT32 PointerToRawData;                    // Address of actual data
    UINT32 PointerToRelocations;
    UINT32 PointerToLinenumbers;
    UINT16 NumberOfRelocations;
    UINT16 NumberOfLinenumbers;
    UINT32 Characteristics;
}__attribute__ ((packed)) PE_Section_Header_64;

// Timer event context is the text mode screen bounds
typedef struct {
    UINT32 rows; 
    UINT32 cols;
} Timer_Context;

// Memory map and associated meta data
typedef struct {
    UINTN                 size;
    EFI_MEMORY_DESCRIPTOR *map;
    UINTN                 key;
    UINTN                 desc_size;
    UINT32                desc_version;
} Memory_Map_Info;

// Bitmapped font info (assuming monospaced)
typedef struct {
    char     *name;             // Font name
    uint32_t width;             // Glyph width in pixels
    uint32_t height;            // Glyph height in pixels
    uint32_t num_glyphs;        // Number of glyphs in array/font
    uint8_t  *glyphs;           // Glyph data/array
    bool     left_col_first;    // Are bits for glyphs stored in memory left->right 
                                //   e.g. PSF font, or right->left e.g. terminus?
} Bitmap_Font;

// Example Kernel Parameters
typedef struct {
    Memory_Map_Info                   mmap; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE gop_mode;
    EFI_RUNTIME_SERVICES              *RuntimeServices;
    UINTN                             NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE           *ConfigurationTable;
    UINTN                             num_fonts;
    Bitmap_Font                       *fonts;
} Kernel_Parms;

// Kernel entry point typedef
typedef void EFIAPI (*Entry_Point)(Kernel_Parms *);

// EFI Configuration Table GUIDs and string names
typedef struct {
    EFI_GUID guid;
    char16_t *string;
} Efi_Guid_With_String;

Efi_Guid_With_String config_table_guids_and_strings[] = {
    {EFI_ACPI_TABLE_GUID,   u"EFI_ACPI_TABLE_GUID"},
    {ACPI_TABLE_GUID,       u"ACPI_TABLE_GUID"},
    {SAL_SYSTEM_TABLE_GUID, u"SAL_SYSTEM_TABLE_GUID"},
    {SMBIOS_TABLE_GUID,     u"SMBIOS_TABLE_GUID"},
    {SMBIOS3_TABLE_GUID,    u"SMBIOS3_TABLE_GUID"},
    {MPS_TABLE_GUID,        u"MPS_TABLE_GUID"},
};

// General ACPI table header
//   taken from ACPI Spec 6.4 section 21.2.1
typedef struct {
    char   signature[4];
    UINT32 length;
    UINT8  revision;
    UINT8  checksum;
    char   OEMID[6];
    char   OEM_table_id[8];
    UINT32 OEM_revision;
    char   creator_id[4];
    UINT32 creator_revision;
} ACPI_TABLE_HEADER;

// KEY:
// GDT = Global Descriptor Table
// LDT = Local Descriptor Table
// TSS = Task State Segment

// Use this for GDTR with assembly "LGDT" instruction
typedef struct {
    UINT16 limit;
    UINT64 base; 
} __attribute__((packed)) Descriptor_Register;

// Descriptor e.g. an array of these is used for the GDT/LDT
typedef struct {
    union {
        UINT64 value;
        struct {
            UINT64 limit_15_0:  16;
            UINT64 base_15_0:   16;
            UINT64 base_23_16:  8;
            UINT64 type:        4;
            UINT64 s:           1;  // System descriptor (0), Code/Data segment (1)
            UINT64 dpl:         2;  // Descriptor Privelege Level
            UINT64 p:           1;  // Present flag
            UINT64 limit_19_16: 4;
            UINT64 avl:         1;  // Available for use
            UINT64 l:           1;  // 64 bit (Long mode) code segment
            UINT64 d_b:         1;  // Default op size/stack pointer size/upper bound flag
            UINT64 g:           1;  // Granularity; 1 byte (0), 4KiB (1)
            UINT64 base_31_24:  8;
        };
    };
} X86_64_Descriptor;

// TSS/LDT descriptor - 64 bit
typedef struct {
    X86_64_Descriptor descriptor;
    UINT32 base_63_32;
    UINT32 zero;
} TSS_LDT_Descriptor;

// TSS structure - TSS descriptor points to this structure in the GDT
typedef struct {
    UINT32 reserved_1;
    UINT32 RSP0_lower;
    UINT32 RSP0_upper;
    UINT32 RSP1_lower;
    UINT32 RSP1_upper;
    UINT32 RSP2_lower;
    UINT32 RSP2_upper;
    UINT32 reserved_2;
    UINT32 reserved_3;
    UINT32 IST1_lower;
    UINT32 IST1_upper;
    UINT32 IST2_lower;
    UINT32 IST2_upper;
    UINT32 IST3_lower;
    UINT32 IST3_upper;
    UINT32 IST4_lower;
    UINT32 IST4_upper;
    UINT32 IST5_lower;
    UINT32 IST5_upper;
    UINT32 IST6_lower;
    UINT32 IST6_upper;
    UINT32 IST7_lower;
    UINT32 IST7_upper;
    UINT32 reserved_4;
    UINT32 reserved_5;
    UINT16 reserved_6;
    UINT16 io_map_base;
} TSS;

// Example GDT
typedef struct {
    X86_64_Descriptor  null;                // Offset 0x00
    X86_64_Descriptor  kernel_code_64;      // Offset 0x08
    X86_64_Descriptor  kernel_data_64;      // Offset 0x10
    X86_64_Descriptor  user_code_64;        // Offset 0x18
    X86_64_Descriptor  user_data_64;        // Offset 0x20
    X86_64_Descriptor  kernel_code_32;      // Offset 0x28
    X86_64_Descriptor  kernel_data_32;      // Offset 0x30
    X86_64_Descriptor  user_code_32;        // Offset 0x38
    X86_64_Descriptor  user_data_32;        // Offset 0x40
    TSS_LDT_Descriptor tss;                 // Offset 0x48
} GDT;

// PSF Font types
// Adapted from https://wiki.osdev.org/PC_Screen_Font
#define PSF2_FONT_MAGIC 0x864ab572
typedef struct {
    uint32_t magic;             // Magic bytes to identify PSF file (PSF2_FONT_MAGIC)
    uint32_t version;           // Zero 
    uint32_t headersize;        // Offset of bitmaps in file, 32 
    uint32_t flags;             // 0 if no unicode table
    uint32_t num_glyphs;        // Number of glyphs 
    uint32_t bytes_per_glyph;   // Size of each glyph 
    uint32_t height;            // Height in pixels 
    uint32_t width;             // Width in pixels 
} PSF2_Header;

// -----------------
// Global variables
// -----------------
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;   // Console output
EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin  = NULL;   // Console input
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr = NULL;   // Console output - stderr

EFI_BOOT_SERVICES    *bs = NULL;                // Boot services
EFI_RUNTIME_SERVICES *rs = NULL;                // Runtime services
EFI_SYSTEM_TABLE     *st = NULL;                // System Table

EFI_HANDLE image = NULL;                        // Image handle

INT32 text_rows = 0, text_cols = 0;             // Current text mode screen rows & columns

// ======================
// Set global variables
// ======================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
    //cerr = systable->StdErr;  // Stderr can be set to a serial output or other non-display device.
    cerr = cout;                // Use stdout for error printing 
    st = systable;
    bs = st->BootServices;
    rs = st->RuntimeServices;
    image = handle;
}

// ====================
// Get key from user
// ====================
EFI_INPUT_KEY get_key(void) {
    EFI_EVENT events[1] = { cin->WaitForKey };
    EFI_INPUT_KEY key = {0};
    UINTN index = 0;

    bs->WaitForEvent(1, events, &index);
    cin->ReadKeyStroke(cin, &key);
    return key;
}

// ====================================
// memset for compiling with clang/gcc:
// Sets len bytes of dst memory with int c
// Returns dst buffer
// ================================
VOID *memset(VOID *dst, UINT8 c, UINTN len) {
    UINT8 *p = dst;
    while (len--) *p++ = c;
    return dst;
}

// ====================================
// memcpy for compiling with clang/gcc:
// Sets len bytes of dst memory from src.
// Assumes memory does not overlap!
// Returns dst buffer
// ================================
VOID *memcpy(VOID *dst, VOID *src, UINTN len) {
    UINT8 *p = dst, *q = src;
    while (len--) *p++ = *q++;
    return dst;
}

// =============================================================================
// memcmp:
// Compare up to len bytes of m1 and m2, stop at first
//   point that they don't equal.
// Returns 0 if equal, >0 if m1 is greater than m2, <0 if m2 is greater than m1
// =============================================================================
INTN memcmp(VOID *m1, VOID *m2, UINTN len) {
    UINT8 *p = m1;
    UINT8 *q = m2;
    for (UINTN i = 0; i < len; i++)
        if (p[i] != q[i]) return (INTN)(p[i]) - (INTN)(q[i]);

    return 0;
}

// =====================================================================
// (ASCII) strlen:
// Returns: length of string not including NULL terminator
// =====================================================================
UINTN strlen(char *s) {
    UINTN len = 0;
    while (*s++) len++;
    return len;
}

// =====================================================================
// (ASCII) strstr:
// Return a pointer to the beginning of the located
//   substring, or NULL if the substring is not found.
//   If needle is the empty string, the return value is always haystack
//   itself.
// =====================================================================
char *strstr(char *haystack, char *needle) {
    if (!needle) return haystack;

    char *p = haystack;
    while (*p) {
        if (*p == *needle && !memcmp(p, needle, strlen(needle))) 
            return p;
        p++;
    }

    return NULL;
}

// =====================================================================
// (ASCII) isdigit:
// Returns: true/1 if char c >= 0 and <= 9, else 0/false
// =====================================================================
BOOLEAN isdigit(char c) {
    return c >= '0' && c <= '9';
}

// =====================================================================
// CHAR16 isdigit:
// Returns: true/1 if char c >= 0 and <= 9, else 0/false
// =====================================================================
BOOLEAN isdigit_c16(CHAR16 c) {
    return c >= u'0' && c <= u'9';
}

// =====================================================================
// CHAR16 isxdigit:
// Returns: true/1 if char c >= 0   and <= 9   or 
//                           >= 'a' and <= 'f' or
//                           >= 'A' and <= 'F'
// =====================================================================
BOOLEAN isxdigit_c16(CHAR16 c) {
    return (c >= u'0' && c <= u'9') ||
           (c >= u'a' && c <= u'f') ||
           (c >= u'A' && c <= u'F');
}

// =============================================
// (ASCII) atoi: 
// Converts intial value of input string to 
//   int.
// Returns converted value or 0 on error
// =============================================
INTN atoi(char *s) {
    INTN result = 0;
    while (isdigit(*s)) 
        result = (result * 10) + (*s++ - '0');

    return result;
}

// ================================
// CHAR16 strcpy:
//   Copy src string into dst 
//   Returns dst
// ================================
CHAR16 *strcpy_u16(CHAR16 *dst, CHAR16 *src) {
    if (!dst) return NULL;
    if (!src) return dst;

    CHAR16 *result = dst;
    while (*src) *dst++ = *src++;

    *dst = u'\0';   // Null terminate

    return result;
}

// ================================
// CHAR16 strncmp:
//   Compare 2 strings, each character, up to at most len bytes
//   Returns difference in strings at last point of comparison:
//   0 if strings are equal, <0 if s2 is greater, >0 if s1 is greater
// ================================
INTN strncmp_u16(CHAR16 *s1, CHAR16 *s2, UINTN len) {
    if (len == 0) return 0;

    while (len > 0 && *s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
        len--;
    }

    return *s1 - *s2;
}

// ================================
// CHAR16 strrchr:
//   Return last occurrence of C in string
// ================================
CHAR16 *strrchr_u16(CHAR16 *str, CHAR16 c) {
    CHAR16 *result = NULL;

    while (*str) {
        if (*str == c) result = str;
        str++;
    }

    return result;
}

// ================================
// CHAR16 strcat:
//   Concatenate src string onto the end of dst string, at
//   dst's original null terminator position.
//  Returns dst
// ================================
CHAR16 *strcat_u16(CHAR16 *dst, CHAR16 *src) {
    CHAR16 *s = dst;

    while (*s) s++;             // Go until null terminator

    while (*src) *s++ = *src++; // Copy src to dst at null position

    *s = u'\0';                 // Null terminate new string

    return dst; 
}

// ===================================================================
// Connect all controllers to all handles,
// Code adapted from UEFI Spec 2.10 Errata A section 7.3.12 Examples
// ===================================================================
VOID connect_all_controllers(VOID) {
    // Retrieve the list of all handles from the handle database
    EFI_STATUS Status;
    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;
    UINTN HandleIndex = 0;
    Status = bs->LocateHandleBuffer(AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status) || !HandleBuffer) return;

    // Connect all controllers found on all handles
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) 
        Status = bs->ConnectController(HandleBuffer[HandleIndex], NULL, NULL, TRUE);

    // Free handle buffer when done
    bs->FreePool(HandleBuffer);
}

// ===========================================
// Get specific config table pointer by GUID
// ===========================================
VOID *get_config_table_by_guid(EFI_GUID guid) { 
    for (UINTN i = 0; i < st->NumberOfTableEntries; i++) {
        EFI_GUID vendor_guid = st->ConfigurationTable[i].VendorGuid;

        if (!memcmp(&vendor_guid, &guid, sizeof guid)) 
            return st->ConfigurationTable[i].VendorTable;
    }
    return NULL;    // Did not find config table
}

// =================================
// Add integer as string to buffer
// =================================
BOOLEAN
add_int_to_buf(UINTN number, UINT8 base, BOOLEAN signed_num, UINTN min_digits, CHAR16 *buf, 
               UINTN *buf_idx) {
    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[24];  // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
    UINTN i = 0;
    BOOLEAN negative = FALSE;

    if (base > 16) {
        cerr->OutputString(cerr, u"Invalid base specified!\r\n");
        return FALSE;    // Invalid base
    }

    // Only use and print negative numbers if decimal and signed True
    if (base == 10 && signed_num && (INTN)number < 0) {
       number = -(INTN)number;  // Get absolute value of correct signed value to get digits to print
       negative = TRUE;
    }

    do {
       buffer[i++] = digits[number % base];
       number /= base;
    } while (number > 0);

    while (i < min_digits) buffer[i++] = u'0'; // Pad with 0s

    // Add negative sign for decimal numbers
    if (base == 10 && negative) buffer[i++] = u'-';

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer to read left to right
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Add number string to input buffer for printing
    for (CHAR16 *p = buffer; *p; p++) {
        buf[*buf_idx] = *p;
        *buf_idx += 1;
    }
    return TRUE;
}

// ===================================================================
// Print formatted strings to stdout, using a va_list for arguments
// ===================================================================
bool vfprintf(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *stream, CHAR16 *fmt, va_list args) {
    bool result = true;
    CHAR16 charstr[2] = {0};    

    CHAR16 buf[1024];   // Format string buffer for % strings
    UINTN buf_idx = 0;
    
    // Print formatted string values
    for (UINTN i = 0; fmt[i] != u'\0'; i++) {
        if (fmt[i] == u'%') {
            bool alternate_form = false;
            UINTN min_field_width = 0;
            UINTN precision = 0;
            UINTN length_bits = 0;  
            UINTN num_printed = 0;      // # of digits/chars printed for numbers or strings
            UINT8 base = 0;
            bool input_precision = false;
            bool signed_num   = false;
            bool int_num      = false;
            bool double_num   = false;
            bool left_justify = false;  // Left justify text from '-' flag instead of default right justify
            bool space_flag   = false;
            bool plus_flag    = false;
            CHAR16 padding_char = ' ';  // '0' or ' ' depending on flags
            i++;

            // Initialize format string buffer
            memset(buf, 0, sizeof buf);
            buf_idx = 0;

            // Check for flags
            while (true) {
                switch (fmt[i]) {
                    case u'#':
                        // Alternate form
                        alternate_form = true;
                        i++;
                        continue;

                    case u'0':
                        // 0-pad numbers on the left, unless '-' or precision is also defined
                        padding_char = '0'; 
                        i++;
                        continue;

                    case u' ':
                        // Print a space before positive signed number conversion or empty string
                        //   number conversions
                        space_flag = true;
                        if (plus_flag) space_flag = false;  // Plus flag '+' overrides space flag
                        i++;
                        continue;

                    case u'+':
                        // Always print +/- before a signed number conversion
                        plus_flag = true;
                        if (space_flag) space_flag = false; // Plus flag '+' overrides space flag
                        i++;
                        continue;

                    case u'-':
                        left_justify = true;
                        i++;
                        continue;

                    default:
                        break;
                }
                break; // No more flags
            }

            // Check for minimum field width e.g. in "8.2" this would be 8
            if (fmt[i] == u'*') {
                // Get int argument for min field width
                min_field_width = va_arg(args, int);
                i++;
            } else {
                // Get number literal from format string
                while (isdigit_c16(fmt[i])) 
                    min_field_width = (min_field_width * 10) + (fmt[i++] - u'0');
            }

            // Check for precision/maximum field width e.g. in "8.2" this would be 2
            if (fmt[i] == u'.') {
                input_precision = true; 
                i++;
                if (fmt[i] == u'*') {
                    // Get int argument for precision
                    precision = va_arg(args, int);
                    i++;
                } else {
                    // Get number literal from format string
                    while (isdigit_c16(fmt[i])) 
                        precision = (precision * 10) + (fmt[i++] - u'0');
                }
            }

            // Check for Length modifiers e.g. h/hh/l/ll
            if (fmt[i] == u'h') {
                i++;
                length_bits = 16;       // h
                if (fmt[i] == u'h') {
                    i++;
                    length_bits = 8;    // hh
                }
            } else if (fmt[i] == u'l') {
                i++;
                length_bits = 32;       // l
                if (fmt[i] == u'l') {
                    i++;
                    length_bits = 64;    // ll
                }
            }

            // Check for conversion specifier
            switch (fmt[i]) {
                case u'c': {
                    // Print CHAR16 value; printf("%c", char)
                    if (length_bits == 8)
                        charstr[0] = (char)va_arg(args, int);   // %hhc "ascii" or other 8 bit char
                    else
                        charstr[0] = (CHAR16)va_arg(args, int); // Assuming 16 bit char16_t

                    buf[buf_idx++] = charstr[0];
                }
                break;

                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    if (length_bits == 8) {
                        char *string = va_arg(args, char*);         // %hhs; Assuming 8 bit ascii chars
                        while (*string) {
                            buf[buf_idx++] = *string++;
                            if (++num_printed == precision) break;  // Stop printing at max characters
                        }

                    } else {
                        CHAR16 *string = va_arg(args, CHAR16*);     // Assuming 16 bit char16_t
                        while (*string) {
                            buf[buf_idx++] = *string++;
                            if (++num_printed == precision) break;  // Stop printing at max characters
                        }
                    }
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    int_num = true;
                    base = 10;
                    signed_num = true;
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    int_num = true;
                    base = 16;
                    signed_num = false;
                    if (alternate_form) {
                        buf[buf_idx++] = u'0';
                        buf[buf_idx++] = u'x';
                    }
                }
                break;

                case u'u': {
                    // Print UINT32; printf("%u", number_uint32)
                    int_num = true;
                    base = 10;
                    signed_num = false;
                }
                break;

                case u'b': {
                    // Print UINTN as binary; printf("%b", number_uintn)
                    int_num = true;
                    base = 2;
                    signed_num = false;
                    if (alternate_form) {
                        buf[buf_idx++] = u'0';
                        buf[buf_idx++] = u'b';
                    }
                }
                break;

                case u'o': {
                    // Print UINTN as octal; printf("%o", number_uintn)
                    int_num = true;
                    base = 8;
                    signed_num = false;
                    if (alternate_form) {
                        buf[buf_idx++] = u'0';
                        buf[buf_idx++] = u'o';
                    }
                }
                break;

                case u'f': {
                    // Print INTN rounded float value
                    double_num = true;
                    signed_num = true;
                    base = 10;
                    if (!input_precision) precision = 6;    // Default decimal places to print
                }
                break;

                default:
                    stream->OutputString(stream, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    stream->OutputString(stream, charstr);
                    stream->OutputString(stream, u"\r\n");
                    result = false;
                    goto end;
                    break;
            }

            if (int_num) {
                // Number conversion: Integer
                UINT64 number = 0;
                switch (length_bits) {
                    case 0:
                    case 32: 
                    default:
                        // l
                        number = va_arg(args, UINT32);
                        if (signed_num) number = (INT32)number;
                        break;

                    case 8:
                        // hh
                        number = (UINT8)va_arg(args, int);
                        if (signed_num) number = (INT8)number;
                        break;

                    case 16:
                        // h
                        number = (UINT16)va_arg(args, int);
                        if (signed_num) number = (INT16)number;
                        break;

                    case 64:
                        // ll
                        number = va_arg(args, UINT64);
                        if (signed_num) number = (INT64)number;
                        break;
                }

                // Add space before positive number for ' ' flag
                if (space_flag && signed_num && number >= 0) buf[buf_idx++] = u' ';    

                // Add sign +/- before signed number for '+' flag
                if (plus_flag && signed_num) buf[buf_idx++] = number >= 0 ? u'+' : u'-';

                add_int_to_buf(number, base, signed_num, precision, buf, &buf_idx);
            }

            if (double_num) {
                // Number conversion: Float/Double
                double number = va_arg(args, double);
                UINTN whole_num = 0;

                // Get digits before decimal point
                whole_num = (UINTN)number;
                UINTN num_digits = 0;
                do {
                   num_digits++; 
                   whole_num /= 10;
                } while (whole_num > 0);

                // Add digits to write buffer
                add_int_to_buf(number, base, signed_num, num_digits, buf, &buf_idx);

                // Print decimal digits equal to precision value, 
                //   if precision is explicitly 0 then do not print
                if (!input_precision || precision != 0) {
                    buf[buf_idx++] = u'.';      // Add decimal point

                    whole_num = (UINTN)number;  // Get only digits before decimal
                    if (number < 0.0) number = -number; // Ensure number is positive
                    number -= whole_num;        // Get only decimal digits

                    // Move precision # of decimal digits before decimal point 
                    //   using base 10, number = number * 10^precision
                    for (UINTN i = 0; i < precision; i++)
                        number *= 10;

                    whole_num = (UINTN)number;  // Get only digits before decimal

                    // Add digits to write buffer
                    add_int_to_buf(number, base, signed_num, precision, buf, &buf_idx);
                }
            }

            // Flags are defined such that 0 is overruled by left justify and precision
            if (padding_char == u'0' && (left_justify || precision > 0))
                padding_char = u' ';

            // Add padding depending on flags (0 or space) and left/right justify
            INTN diff = min_field_width - buf_idx;
            if (diff > 0) {
                if (left_justify) {
                    // Append padding to minimum width, always spaces
                    while (diff--) buf[buf_idx++] = u' ';   
                } else {
                    // Right justify
                    // Copy buffer to end of buffer
                    INTN dst = min_field_width-1, src = buf_idx-1;
                    while (src >= 0)  buf[dst--] = buf[src--];  // e.g. "TEST\0\0" -> "TETEST"

                    // Overwrite beginning of buffer with padding
                    dst = (int_num && alternate_form) ? 2 : 0;  // Skip 0x/0b/0o/... prefix
                    while (diff--) buf[dst++] = padding_char;   // e.g. "TETEST" -> "  TEST"
                }
            }

            // Print buffer output for formatted string
            stream->OutputString(stream, buf);

        } else {
            // Not formatted string, print next character
            charstr[0] = fmt[i];
            stream->OutputString(stream, charstr);
        }
    }

    end:
    va_end(args);
    return result;
}

// ===================================
// Print formatted strings to stdout
// ===================================
bool printf(CHAR16 *fmt, ...) {
    bool result = true;

    va_list args;
    va_start(args, fmt);
    result = vfprintf(cout, fmt, args);
    va_end(args);

    return result;
}

// =======================================================================
// Print a formatted error message stderr and get a key from the user,
//   so they can acknowledge the error and it doesn't go on immediately.
// =======================================================================
bool error(char *file, int line, const char *func, EFI_STATUS status, CHAR16 *fmt, ...) {
    printf(u"\r\nERROR: FILE %hhs, LINE %d, FUNCTION %hhs\r\n", file, line, func);

    // Print error code & string if applicable
    if (status > 0 && status - TOP_BIT < MAX_EFI_ERROR)
        printf(u"STATUS: %#llx (%s)\r\n", status, EFI_ERROR_STRINGS[status-TOP_BIT]);

    va_list args;
    va_start(args, fmt);
    bool result = vfprintf(cerr, fmt, args); // Printf the error message to stderr
    va_end(args);

    get_key();  // User will respond with input before going on
    return result;
}
#define error(...) error(__FILE__, __LINE__, __func__, __VA_ARGS__)

// ================================================
// Get a number from the user and print to screen
// ================================================
BOOLEAN get_num(UINTN *number, UINT8 base) {
    EFI_INPUT_KEY key = {0};

    if (!number) return false;  // Passed in NULL pointer

    *number = 0;
    do {
        key = get_key();
        if (key.ScanCode == SCANCODE_ESC) return false; // User wants to leave

        if (isdigit_c16(key.UnicodeChar)) {
            *number = (*number * base) + (key.UnicodeChar - u'0');
            printf(u"%c", key.UnicodeChar);

        } else if (base == 16) {
            if (key.UnicodeChar >= u'a' && key.UnicodeChar <= u'f') {
                *number = (*number * base) + (key.UnicodeChar - u'a' + 10);
                printf(u"%c", key.UnicodeChar);

            } else if (key.UnicodeChar >= u'A' && key.UnicodeChar <= u'F') {
                *number = (*number * base) + (key.UnicodeChar - u'A' + 10);
                printf(u"%c", key.UnicodeChar);
            }
        }
    } while (key.UnicodeChar != u'\r');

    return true;
}

// ====================
// Print a GUID value
// ====================
void print_guid(EFI_GUID guid) { 
    UINT8 *p = (UINT8 *)&guid;
    printf(u"{%x,%x,%x,%x,%x,{%x,%x,%x,%x,%x,%x}}\r\n",
            *(UINT32 *)&p[0], *(UINT16 *)&p[4], *(UINT16 *)&p[6], 
            (UINTN)p[8], (UINTN)p[9], (UINTN)p[10], (UINTN)p[11], (UINTN)p[12], (UINTN)p[13], 
            (UINTN)p[14], (UINTN)p[15]);
}

// ============================
// Print an ACPI table header
// ============================
void print_acpi_table_header(ACPI_TABLE_HEADER header) { 
    printf(u"Signature: %.4hhs\r\n"
           u"Length: %u\r\n"
           u"Revision: %#x\r\n"
           u"Checksum: %u\r\n"
           u"OEMID: %.6hhs\r\n"
           u"OEM Table ID: %.8hhs\r\n"
           u"OEM Revision: %#x\r\n"
           u"Creator ID: %.4hhs\r\n"
           u"Creator Revision: %#x\r\n",
           &header.signature[0],
           (UINTN)header.length,
           (UINTN)header.revision,
           (UINTN)header.checksum,
           &header.OEMID[0], 
           &header.OEM_table_id[0],
           (UINTN)header.OEM_revision,
           &header.creator_id[0], 
           (UINTN)header.creator_revision);
}

// ===================================================================
// Print information for an ELF file
// NOTE: Assumes file is ELF64 PIE (Position Independent Executable)
// ===================================================================
void print_elf_info(VOID *elf_buffer) {
    ELF_Header_64 *ehdr = elf_buffer;

    // ELF header info
    printf(u"Type: %u, Machine: %x, Entry Point: %#llx\r\n"
           u"Pgm headers offset: %u, Elf Header Size: %u\r\n"
           u"Pgm entry size: %u, # of Pgm headers: %u\r\n",
           ehdr->e_type, ehdr->e_machine, ehdr->e_entry,
           ehdr->e_phoff, ehdr->e_ehsize, 
           ehdr->e_phentsize, ehdr->e_phnum);
    
    ELF_Program_Header_64 *phdr = (ELF_Program_Header_64 *)((UINT8 *)ehdr + ehdr->e_phoff);
    printf(u"\r\nLoadable Program Headers:\r\n");

    UINTN max_alignment = PAGE_SIZE;    
    UINTN mem_min = UINT64_MAX, mem_max = 0;
    for (UINT16 i = 0; i < ehdr->e_phnum; i++, phdr++) {
        // Only interested in loadable program headers
        if (phdr->p_type != PT_LOAD) continue;

        printf(u"%u: Offset: %x, Vaddr: %x, Paddr: %x, FileSize: %x\r\n"
               u"    MemSize: %x, Alignment: %x\r\n",
               (UINTN)i, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
               phdr->p_filesz, phdr->p_memsz, phdr->p_align);

        // Update max alignment as needed
        if (max_alignment < phdr->p_align) max_alignment = phdr->p_align;

        UINTN hdr_begin = phdr->p_vaddr;        
        UINTN hdr_end   = phdr->p_vaddr + phdr->p_memsz + max_alignment-1;

        // Limit memory range to aligned values
        //   e.g. 4096-1 = 4095 or 0x00000FFF (32 bit); ~4095 = 0xFFFFF000
        hdr_begin &= ~(max_alignment-1);    
        hdr_end   &= ~(max_alignment-1);   

        // Get new minimum & maximum memory bounds for all program sections
        if (hdr_begin < mem_min) mem_min = hdr_begin;
        if (hdr_end   > mem_max) mem_max = hdr_end;

        if (cout->Mode->CursorRow >= text_rows-2) {
            printf(u"\r\nPress any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }

    UINTN max_memory_needed = mem_max - mem_min;   
    printf(u"\r\nMemory needed for file: %#llx bytes\r\n", max_memory_needed);
}

// =============================================================
// Print information for a PE32+ file
// NOTE: Assumes file is PIE (Position Independent Executable)
// =============================================================
void print_pe_info(VOID *pe_buffer) {
    const UINT8 pe_sig_offset = 0x3C; // From PE file format
    UINT32 pe_sig_pos = *(UINT32 *)((UINT8 *)pe_buffer + pe_sig_offset);
    UINT8 *pe_sig = (UINT8 *)pe_buffer + pe_sig_pos;

    printf(u"Signature: [%hhx][%hhx][%hhx][%hhx] ",
           pe_sig[0], pe_sig[1], pe_sig[2], pe_sig[3]);

    // COFF header
    PE_Coff_File_Header_64 *coff_hdr = (PE_Coff_File_Header_64 *)(pe_sig + 4);
    printf(u"Coff Header:\r\n"
           u"Machine: %x, # of sections: %u, Size of Opt Hdr: %x\r\n"
           u"Characteristics: %x\r\n",
           coff_hdr->Machine, coff_hdr->NumberOfSections, coff_hdr->SizeOfOptionalHeader,
           coff_hdr->Characteristics);

    // Optional header - right after COFF header
    PE_Optional_Header_64 *opt_hdr = (PE_Optional_Header_64 *)(coff_hdr + 1);

    printf(u"\r\nOptional Header:\r\n"
           u"Magic: %x, Entry Point: %#llx\r\n" 
           u"Sect Align: %x, File Align: %x, Size of Image: %x\r\n"
           u"Subsystem: %x, DLL Characteristics: %x\r\n",
           opt_hdr->Magic, opt_hdr->AddressOfEntryPoint,
           opt_hdr->SectionAlignment, opt_hdr->FileAlignment, opt_hdr->SizeOfImage,
           (UINTN)opt_hdr->Subsystem, (UINTN)opt_hdr->DllCharacteristics);

    // Section headers
    PE_Section_Header_64 *shdr = 
        (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);

    printf(u"\r\nSection Headers:\r\n");
    for (UINT16 i = 0; i < coff_hdr->NumberOfSections; i++, shdr++) {
        printf(u"Name: ");
        char *pos = (char *)&shdr->Name;
        for (UINT8 j = 0; j < 8; j++) {
            CHAR16 str[2];
            str[0] = *pos;
            str[1] = u'\0';
            if (*pos == '\0') break;
            printf(u"%s", str);
            pos++;
        }

        printf(u" VSize: %x, Vaddr: %x, DataSize: %x, DataPtr: %x\r\n",
               shdr->VirtualSize, shdr->VirtualAddress, 
               shdr->SizeOfRawData, shdr->PointerToRawData);

        if (cout->Mode->CursorRow >= text_rows-2) {
            printf(u"\r\nPress any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }
}

// ===================================================================
// Read a fully qualified file path in the EFI System Partition into 
//   an output buffer. File path must start with root '\',
//   escaped as needed by the caller with '\\'.
//
// Returns: 
//  - non-null pointer to allocated buffer with file data, 
//      allocated with Boot Services AllocatePool(), or NULL if not 
//      found or error.
//  - Size of returned buffer, if not NULL
//
//  NOTE: Caller will have to use FreePool() on returned buffer to 
//    free allocated memory.
// ===================================================================
VOID *read_esp_file_to_buffer(CHAR16 *path, UINTN *file_size) {
    VOID *file_buffer = NULL;
    EFI_FILE_PROTOCOL *root = NULL, *file = NULL;
    EFI_STATUS status;

    *file_size = 0;

    // Get loaded image protocol first to grab device handle to use 
    //   simple file system protocol on
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    status = bs->OpenProtocol(image,
                              &lip_guid,
                              (VOID **)&lip,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Loaded Image Protocol\r\n");
        goto cleanup;
    }

    // Get Simple File System Protocol for device handle for this loaded
    //   image, to open the root directory for the ESP
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = NULL;
    status = bs->OpenProtocol(lip->DeviceHandle,
                              &sfsp_guid,
                              (VOID **)&sfsp,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Simple File System Protocol\r\n");
        goto cleanup;
    }

    // Open root directory via OpenVolume()
    status = sfsp->OpenVolume(sfsp, &root);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Open Volume for root directory in ESP\r\n");
        goto cleanup;
    }

    // Open file in input path (qualified from root directory)
    status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open file '%s'\r\n", path);
        goto cleanup;
    }

    // Get info for file, to grab file size
    EFI_FILE_INFO file_info;
    EFI_GUID fi_guid = EFI_FILE_INFO_ID;
    UINTN buf_size = sizeof(EFI_FILE_INFO);
    status = file->GetInfo(file, &fi_guid, &buf_size, &file_info);
    if (EFI_ERROR(status)) {
        error(status, u"Could not get file info for file '%s'\r\n", path);
        goto cleanup;
    }

    // Allocate buffer for file
    buf_size = file_info.FileSize;
    status = bs->AllocatePool(EfiLoaderData, buf_size, &file_buffer);
    if (EFI_ERROR(status) || buf_size != file_info.FileSize) {
        error(status, u"Could not allocate memory for file '%s'\r\n", path);
        goto cleanup;
    }

    // Read file into buffer
    status = file->Read(file, &buf_size, file_buffer);
    if (EFI_ERROR(status) || buf_size != file_info.FileSize) {
        error(status, u"Could not read file '%s' into buffer\r\n", path);
        goto cleanup;
    }

    // Set output file size in buffer
    *file_size = buf_size;

    cleanup:
    // Close open file/dir pointers
    if (file) file->Close(file);
    if (root) root->Close(root);

    // Close open protocols
    bs->CloseProtocol(lip->DeviceHandle,
                      &sfsp_guid,
                      image,
                      NULL);

    bs->CloseProtocol(image,
                      &lip_guid,
                      image,
                      NULL);

    // Will return buffer with file data or NULL on errors
    return file_buffer; 
}

// =================================================================
// Read a file from a given disk (from input media ID), into an
//   output buffer. 
//
// Returns: non-null pointer to allocated buffer with data, 
//  allocated with Boot Services AllocatePool(), or NULL if not 
//  found or error. If executable input parameter is true, then 
//  allocate EfiLoaderCode memory type, else use EfiLoaderData.
//
//  NOTE: Caller will have to use FreePool() on returned buffer to 
//    free allocated memory.
// =================================================================
EFI_PHYSICAL_ADDRESS 
read_disk_lbas_to_buffer(EFI_LBA disk_lba, UINTN data_size, UINT32 disk_mediaID, bool executable) {
    EFI_PHYSICAL_ADDRESS buffer = 0;
    EFI_STATUS status = EFI_SUCCESS;

    // Loop through and get Block IO protocol for input media ID, for entire disk
    //   NOTE: This assumes the first Block IO found with logical partition false is the entire disk
    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;

    status = bs->LocateHandleBuffer(ByProtocol, &bio_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate any Block IO Protocols.\r\n");
        return buffer;
    }

    BOOLEAN found = false;
    UINTN i = 0;
    for (; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &bio_guid,
                                  (VOID **)&biop,
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

        if (EFI_ERROR(status)) {
            error(status, u"Could not Open Block IO protocol on handle %u.\r\n", i);
            // Close open protocol 
            bs->CloseProtocol(handle_buffer[i],
                              &bio_guid,
                              image,
                              NULL);
            continue;
        }

        if (biop->Media->MediaId == disk_mediaID && !biop->Media->LogicalPartition) {
            found = true;
            break;
        }

        // Close open protocol when done
        bs->CloseProtocol(handle_buffer[i],
                          &bio_guid,
                          image,
                          NULL);
    }

    if (!found) {
        error(0, u"Could not find Block IO protocol for disk with ID %u.\r\n", disk_mediaID);
        return buffer;
    }

    // Get Disk IO Protocol on same handle as Block IO protocol
    EFI_GUID dio_guid = EFI_DISK_IO_PROTOCOL_GUID;
    EFI_DISK_IO_PROTOCOL *diop;
    status = bs->OpenProtocol(handle_buffer[i], 
                              &dio_guid,
                              (VOID **)&diop,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Open Disk IO protocol on handle %u.\r\n", i);
        goto cleanup;
    }

    // Allocate buffer for data
    UINTN pages_needed = (data_size + (PAGE_SIZE-1)) / PAGE_SIZE;
    status = bs->AllocatePages(AllocateAnyPages, 
                               executable ? EfiLoaderCode : EfiLoaderData, 
                               pages_needed, 
                               &buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Allocate buffer for disk data.\r\n");
        bs->CloseProtocol(handle_buffer[i],
                          &dio_guid,
                          image,
                          NULL);
        goto cleanup;
    }

    // Use Disk IO Read to read into allocated buffer
    status = diop->ReadDisk(diop, disk_mediaID, disk_lba * biop->Media->BlockSize, data_size, (VOID *)buffer);
    if (EFI_ERROR(status)) 
        error(status, u"Could not read Disk LBAs into buffer.\r\n");

    // Close disk IO protocol when done
    bs->CloseProtocol(handle_buffer[i],
                      &dio_guid,
                      image,
                      NULL);

    cleanup:
    // Close open block io protocol when done
    bs->CloseProtocol(handle_buffer[i],
                      &bio_guid,
                      image,
                      NULL);
    return buffer;
}

// TODO: Make helper function to read disk partition file to buffer,
//   similar to VOID *read_esp_file_to_buffer(CHAR16 *path, UINTN *file_size). 
//   Get info from file \\EFI\\BOOT\\DATAFLS.INF, call 
//     EFI_PHYSICAL_ADDRESS read_disk_lbas_to_buffer(EFI_LBA disk_lba, 
//                                                   UINTN data_size, 
//                                                   UINT32 disk_mediaID, 
//                                                   bool executable)
//     then return the filled buffer.
//
//   Can use this function in load_kernel() to abstract finding kernel file
//     info and loading it into a buffer, and for loading the PSF font 
//     instead of embedding it directly in source files, if wanted.

// ==================================================
// Get first package list found in the HII database
// NOTE: This allocates memory with AllocatePool(),
//   Caller should free returned address with e.g. 
//   if (result) bs->FreePool(result);
// ==================================================
EFI_HII_PACKAGE_LIST_HEADER *hii_database_package_list(UINT8 package_type) {
    EFI_HII_PACKAGE_LIST_HEADER *pkg_list = NULL;   // Return variable
    EFI_HII_HANDLE handle_buf = NULL;   

    // Get HII database protocol instance
    EFI_HII_DATABASE_PROTOCOL *dbp = NULL;
    EFI_GUID dbp_guid = EFI_HII_DATABASE_PROTOCOL_GUID;
    EFI_STATUS status = EFI_SUCCESS;

    status = bs->LocateProtocol(&dbp_guid, NULL, (VOID **)&dbp);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate HII Database Protocol!\r\n");
        goto cleanup;
    }

    // Get size of buffer needed for list of handles for package lists
    UINTN buf_len = 0;
    status = dbp->ListPackageLists(dbp, package_type, NULL, &buf_len, &handle_buf); 
    if (status != EFI_BUFFER_TOO_SMALL && EFI_ERROR(status)) {
        error(status, u"Could not get size of list of handles for HII package lists for type %hhu.\r\n",
              package_type);
        goto cleanup;
    }

    // Allocate buffer for list of handles for package lists
    status = bs->AllocatePool(EfiLoaderData, buf_len, (VOID **)&handle_buf);  
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for handle list for package lists type %hhu.\r\n",
              package_type);
        goto cleanup;
    }

    // Get list of handles with package type into buffer
    status = dbp->ListPackageLists(dbp, package_type, NULL, &buf_len, &handle_buf); 
    if (EFI_ERROR(status)) {
        error(status, u"Could not get list of handles for HII package lists for type %hhu into buffer.\r\n",
              package_type);
        goto cleanup;
    }

    // Get size of buffer needed for package list on handle
    UINTN buf_len2 = 0;
    EFI_HII_HANDLE handle = handle_buf;    // Point to 1st handle in handle list    
    status = dbp->ExportPackageLists(dbp, handle, &buf_len2, pkg_list);  
    if (status != EFI_BUFFER_TOO_SMALL && EFI_ERROR(status)) {
        error(status, u"Could not get size of 1st package list for type %hhu.\r\n",
              package_type);
        goto cleanup;
    }

    // Allocate buffer for package list to export (1st package list on 1st handle)
    status = bs->AllocatePool(EfiLoaderData, buf_len2, (VOID **)&pkg_list);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for package list of type %hhu.\r\n",
              package_type);
        goto cleanup;
    }

    // Export package list to buffer
    status = dbp->ExportPackageLists(dbp, handle, &buf_len2, pkg_list); 
    if (EFI_ERROR(status)) {
        error(status, u"Could not export package list of type %hhu into buffer.\r\n",
              package_type);
        goto cleanup;
    }

    // Free memory when done and return result
    cleanup:
    if (handle_buf) bs->FreePool(handle_buf);

    return pkg_list;    // Caller needs to free this with bs->FreePool()
}

// ---------------------------------------------------------------------
// Example: process all handles for all package lists:
// for (UINTN offset = 0; offset < handle_buf_len; offset += sizeof(EFI_HII_HANDLE)) {
//     // Get pointer to next handle in buffer
//     EFI_HII_HANDLE *handle = (EFI_HII_HANDLE *)((UINT8 *)handle_buf + offset);
//
//     // Get size of buffer needed for package list
//     UINTN buf_size = 0;
//     EFI_HII_PACKAGE_LIST_HEADER *pkg_list_hdr = NULL;
//     dbp->ExportPackageLists(dbp, *handle, &buf_size, pkg_list_hdr);
//
//     // Allocate buffer and export package list into it
//     bs->AllocatePool(EfiLoaderData, buf_size, (VOID **)&pkg_list_hdr);
//     dbp->ExportPackageLists(dbp, *handle, &buf_size, pkg_list_hdr);
//
//     // Process each package in package list; start after the package list header 
//     for (EFI_HII_PACKAGE_HEADER *pkg_hdr = (EFI_HII_PACKAGE_HEADER *)(pkg_list_hdr+1);
//         pkg_hdr->type != EFI_HII_PACKAGE_END;
//         pkg_hdr = (EFI_HII_PACKAGE_HEADER *)((UINT8 *)pkg_hdr + pkg_hdr->Length)) {
//         ...
//     }
//
//     // Free package list when done
//     bs->FreePool(pkg_list_hdr);
// }
//
// // Free handle buffer when done
// bs->FreePool(handle_buf);
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Example (may be buggy or not work): add all simple fonts in package list to kernel parms:
// Kernel_Parms kparms = {0};
// ...
// // Get simple font package list
// EFI_HII_PACKAGE_LIST_HEADER *simple_fonts = get_hii_package_list(EFI_HII_PACKAGE_SIMPLE_FONTS);

// // Get total number of fonts in package list
// UINT32 total_fonts = 0;
// for (EFI_HII_PACKAGE_HEADER *pkg_hdr = (EFI_HII_PACKAGE_HEADER *)(simple_fonts+1);
//      pkg_hdr->Type != EFI_HII_PACKAGE_END;
//      pkg_hdr = (EFI_HII_PACKAGE_HEADER *)((UINT8 *)pkg_hdr + pkg_hdr->Length)) {

//     total_fonts++;
// }

// // Allocate array for all fonts
// kparms.num_fonts = total_fonts;
// status = bs->AllocatePool(EfiLoaderData, 
//                           sizeof *kparms.fonts * kparms.num_fonts, 
//                           (VOID **)&kparms.fonts);

// // Loop through all font packages in list
// UINT32 current_font = 0;
// for (EFI_HII_PACKAGE_HEADER *pkg_hdr = (EFI_HII_PACKAGE_HEADER *)(simple_fonts+1);
//      pkg_hdr->Type != EFI_HII_PACKAGE_END;
//      pkg_hdr = (EFI_HII_PACKAGE_HEADER *)((UINT8 *)pkg_hdr + pkg_hdr->Length)) {

//     EFI_HII_SIMPLE_FONT_PACKAGE_HDR *simple_font =
//         (EFI_HII_SIMPLE_FONT_PACKAGE_HDR *)pkg_hdr;

//     // Allocate buffer for narrow glyphs, note adding 8 bytes for bitmap printing logic
//     //   in kernel later on.
//     VOID *font_glyphs = NULL;
//     const uint32_t bytes_per_glyph = EFI_GLYPH_HEIGHT;
//     const UINTN num_glyphs = max(256, simple_font->NumberOfNarrowGlyphs);
//     status = bs->AllocatePool(EfiLoaderData, 
//                               (num_glyphs * bytes_per_glyph) + 8,
//                               &font_glyphs);

//     memset(font_glyphs, 0, num_glyphs * bytes_per_glyph);   // 0-init buffer

//     // Copy starting at lowest glyph, skipping all characters below that 
//     EFI_NARROW_GLYPH *narrow_glyphs = (EFI_NARROW_GLYPH *)(simple_font+1);
//     const CHAR16 min_glyph = narrow_glyphs[0].UnicodeWeight;
//     memcpy((UINT8 *)font_glyphs + (min_glyph * bytes_per_glyph), 
//            narrow_glyphs,
//            simple_font->NumberOfNarrowGlyphs * sizeof *narrow_glyphs);

//     // Add font info and buffer to kernel font parms
//     char name[26] = "uefi_simple_font_narrow_00";
//     name[24] = ((current_font / 10) % 10) + '0';
//     name[25] = (current_font % 10) + '0';

//     kparms.fonts[current_font++] = (Bitmap_Font){
//         .name            = name,
//         .width           = EFI_GLYPH_WIDTH,
//         .height          = EFI_GLYPH_HEIGHT,
//         //.bytes_per_glyph = EFI_GLYPH_HEIGHT,    
//         .left_col_first  = false,           // Bits in memory are stored right to left
//         .num_glyphs      = simple_font->NumberOfNarrowGlyphs,
//         .glyphs          = font_glyphs,
//     };
// }

// // Free package list when done
// if (simple_fonts) bs->FreePool(simple_fonts);
// ---------------------------------------------------------------------


