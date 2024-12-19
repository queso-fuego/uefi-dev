//
// efi_lib.h: EFI related helper definitions and functions, outside of the scope and definitions 
//   in the UEFI specification or efi.h header.
//
#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "efi.h"

// -----------------
// Global macros
// -----------------
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x)[0])
#define max(x, y) ((x) > (y) ? (x) : (y))

// -----------------
// Global constants
// -----------------
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
// (CHAR16) strlen:
// Returns: length of string not including NULL terminator
// =====================================================================
UINTN strlen_c16(CHAR16 *s) {
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

// ======================================================================
// (ASCII) stpstr:
// Return a pointer to the terminating NULL byte of found substring,
//   or NULL if the substring is not found.
//   If needle is the empty string, the return value is always haystack
//   itself.
// ======================================================================
char *stpstr(char *haystack, char *needle) {
    if (!needle) return haystack;

    char *p = haystack;
    while (*p) {
        if (*p == *needle && !memcmp(p, needle, strlen(needle))) 
            return p + strlen(needle);
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

// ======================================================
// (ASCII) strrev:
//  Reverse a string.
//  Returns reversed string.
// ======================================================
char *strrev(char *s) {
    if (!s) return s;

    char *start = s, *end = s + strlen(s)-1;
    while (start < end) {
        char temp = *end;  // Swap
        *end-- = *start;
        *start++ = temp;
    }

    return s;
}

// ======================================================
// (CHAR16) strrev:
//  Reverse a string.
//  Returns reversed string.
// ======================================================
CHAR16 *strrev_c16(CHAR16 *s) {
    if (!s) return s;

    CHAR16 *start = s, *end = s + strlen_c16(s)-1;
    while (start < end) {
        CHAR16 temp = *end;  // Swap
        *end-- = *start;
        *start++ = temp;
    }

    return s;
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

// ======================================================
// (ASCII) itoa:
//  Convert integer to string representation.
//  Returns input string.
// ======================================================
char *itoa(int32_t number, char *s, uint8_t base) {
    char *p = s;
    uint8_t digit = 0;
    do {
        digit = number % base;
        if (digit < 10) *s++ = digit + '0';
        else            *s++ = 'A' + digit - 10;
        number /= base;
    } while (number > 0);

    *s = '\0';
    strrev(p);

    return p;
}

// ============================
// (CHAR16) strcpy:
//   Copy src string into dst 
//   Returns dst
// ============================
CHAR16 *strcpy_c16(CHAR16 *dst, CHAR16 *src) {
    if (!dst || !src) return dst;

    CHAR16 *result = dst;
    while (*src) *dst++ = *src++;

    *dst = u'\0';   // Null terminate

    return result;
}

// ============================
// (ASCII) strcpy:
//   Copy src string into dst 
//   Returns dst
// ============================
char *strcpy(char *dst, char *src) {
    if (!dst || !src) return dst;

    char *result = dst;
    while (*src) *dst++ = *src++;

    *dst = u'\0';   // Null terminate

    return result;
}

// ======================================================
// (ASCII) stpcpy:
//  Copy src string into dst, end with NULL terminator.
//  Returns pointer to null terminator of dst
// ======================================================
char *stpcpy(char *dst, char *src) {
    if (!dst || !src) return dst;

    while (*src) *dst++ = *src++;

    *dst = u'\0';  
    return dst;
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

// ==============================================
// CHAR16 strrchr:
//   Return last occurrence of char c in string
// ==============================================
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
CHAR16 *strcat_c16(CHAR16 *dst, CHAR16 *src) {
    CHAR16 *s = dst;

    while (*s) s++;             // Go until null terminator

    while (*src) *s++ = *src++; // Copy src to dst at null position

    *s = u'\0';                 // Null terminate new string

    return dst; 
}

// ================================
// (ASCII) strcat:
//   Concatenate src string onto the end of dst string, at
//   dst's original null terminator position.
//  Returns dst
// ================================
char *strcat(char *dst, char *src) {
    char *s = dst + strlen(dst);

    while (*src) *s++ = *src++; // Copy src to dst at null position
    *s = '\0';                  // Null terminate 

    return dst; 
}

// =========================================================
// (ASCII) stpcat:
//   Concatenate src string onto the end of dst string, at
//   dst's original null terminator position.
//  Returns pointer to null terminator of dst.
// =========================================================
char *stpcat(char *dst, char *src) {
    char *s = dst + strlen(dst);

    while (*src) *s++ = *src++; // Copy src to dst at null position
    *s = '\0';                  // Null terminate 

    return s;
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

// ==========================================
// (CHAR16) Add integer as string to buffer
// ==========================================
BOOLEAN
add_int_to_buf_c16(UINTN number, UINT8 base, BOOLEAN signed_num, UINTN min_digits, CHAR16 *buf, 
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
    strrev_c16(buffer);

    // Add number string to input buffer for printing
    for (CHAR16 *p = buffer; *p; p++) {
        buf[*buf_idx] = *p;
        *buf_idx += 1;
    }
    return TRUE;
}

// ========================================================================
// (CHAR16) Fill formatted string buffer with printf() format conversions
// ========================================================================
bool format_string_c16(CHAR16 *buf, CHAR16 *fmt, va_list args) {
    bool result = true;
    CHAR16 charstr[2] = {0};    
    UINTN buf_idx = 0;

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

                    // Only add non-null characters, to not end string early
                    if (charstr[0]) buf[buf_idx++] = charstr[0];    
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
                    strcpy_c16(buf, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    strcat_c16(buf, charstr);
                    strcat_c16(buf, u"\r\n");
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
                if (space_flag && signed_num && (INTN)number >= 0) buf[buf_idx++] = u' ';    

                // Add sign +/- before signed number for '+' flag
                if (plus_flag && signed_num) buf[buf_idx++] = (INTN)number >= 0 ? u'+' : u'-';

                add_int_to_buf_c16(number, base, signed_num, precision, buf, &buf_idx);
            }

            if (double_num) {
                // Number conversion: Float/Double
                double number = va_arg(args, double);
                INTN whole_num = 0;

                // Get digits before decimal point
                whole_num = (INTN)number;
                if (whole_num < 0) whole_num = -whole_num;

                UINTN num_digits = 0;
                do {
                   num_digits++; 
                   whole_num /= 10;
                } while (whole_num > 0);

                // Add digits to write buffer
                add_int_to_buf_c16(number, base, signed_num, num_digits, buf, &buf_idx);

                // Print decimal digits equal to precision value, 
                //   if precision is explicitly 0 then do not print
                if (!input_precision || precision != 0) {
                    buf[buf_idx++] = u'.';      // Add decimal point

                    if (number < 0.0) number = -number; // Ensure number is positive
                    whole_num = (INTN)number;
                    number -= whole_num;                // Get only decimal digits
                    signed_num = FALSE;                 // Don't print negative sign for decimals

                    // Move precision # of decimal digits before decimal point 
                    //   using base 10, number = number * 10^precision
                    for (UINTN i = 0; i < precision; i++)
                        number *= 10;

                    // Add digits to write buffer
                    add_int_to_buf_c16(number, base, signed_num, precision, buf, &buf_idx);
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

        } else {
            // Not formatted string, print next character
            buf[buf_idx++] = fmt[i];
        }
    }

    end:
    buf[buf_idx] = u'\0'; 
    va_end(args);
    return result;
}

// ==================================================================================
// (CHAR16) Print formatted strings to a file stream, using a va_list for arguments
// ==================================================================================
bool vfprintf_c16(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *stream, CHAR16 *fmt, va_list args) {
    CHAR16 buf[1024];   // Format string buffer for % strings
    if (!format_string_c16(buf, fmt, args)) 
        return false;
    
    return !EFI_ERROR(stream->OutputString(stream, buf));
}

// ============================================
// (CHAR16) Print formatted strings to stdout
// ============================================
bool printf_c16(CHAR16 *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return vfprintf_c16(cout, fmt, args);
}

// ===================================================
// (CHAR16) Print formatted strings to a file stream
// ===================================================
bool fprintf_c16(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *stream, CHAR16 *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return vfprintf_c16(stream, fmt, args);
}

// ==============================================
// (CHAR16) Print formatted strings to a string
// ==============================================
bool sprintf_c16(CHAR16 *s, CHAR16 *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return format_string_c16(s, fmt, args);
}

// ==========================================
// (ASCII) Add integer as string to buffer
// ==========================================
BOOLEAN
add_int_to_buf(UINTN number, UINT8 base, BOOLEAN signed_num, UINTN min_digits, char *buf, 
               UINTN *buf_idx) {
    const char *digits = "0123456789ABCDEF";
    char buffer[24];  // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
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
    strrev(buffer);

    // Add number string to input buffer for printing
    for (char *p = buffer; *p; p++) {
        buf[*buf_idx] = *p;
        *buf_idx += 1;
    }
    return TRUE;
}

// ========================================================================
// (ASCII) Fill formatted string buffer with printf() format conversions
// ========================================================================
bool format_string(char *buf, char *fmt, va_list args) {
    bool result = true;
    char charstr[2] = {0};    
    UINTN buf_idx = 0;

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
            char padding_char = ' ';  // '0' or ' ' depending on flags
            i++;

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
                    // Print char value; printf("%c", char)
                    charstr[0] = (char)va_arg(args, int);   // %hhc "ascii" or other 8 bit char

                    // Only add non-null characters, to not end string early
                    if (charstr[0]) buf[buf_idx++] = charstr[0];    
                }
                break;

                case u's': {
                    // Print string; printf("%s", string)
                    if (length_bits == 8) {
                        char *string = va_arg(args, char*);         // %hhs; Assuming 8 bit ascii chars
                        while (*string) {
                            buf[buf_idx++] = *string++;
                            if (++num_printed == precision) break;  // Stop printing at max characters
                        }

                    } else {
                        char *string = va_arg(args, char*);     // Assuming 16 bit char16_t
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
                    strcpy(buf, "Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    strcat(buf, charstr);
                    strcat(buf, "\r\n");
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
                if (space_flag && signed_num && (INTN)number >= 0) buf[buf_idx++] = u' ';    

                // Add sign +/- before signed number for '+' flag
                if (plus_flag && signed_num) buf[buf_idx++] = (INTN)number >= 0 ? u'+' : u'-';

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

                    whole_num = (UINTN)number;          // Get only digits before decimal
                    if (number < 0.0) number = -number; // Ensure number is positive
                    signed_num = FALSE;                 // Don't print negative decimal digits

                    number -= whole_num;                // Get only decimal digits

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

        } else {
            // Not formatted string, print next character
            buf[buf_idx++] = fmt[i];
        }
    }

    end:
    buf[buf_idx] = u'\0'; 
    va_end(args);
    return result;
}

// =============================================
// (ASCII) Print formatted strings to a string
// =============================================
bool sprintf(char *s, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return format_string(s, fmt, args);
}

// =======================================================================
// Print a formatted error message stderr and get a key from the user,
//   so they can acknowledge the error and it doesn't go on immediately.
// =======================================================================
bool error(char *file, int line, const char *func, EFI_STATUS status, CHAR16 *fmt, ...) {
    printf_c16(u"\r\nERROR: FILE %hhs, LINE %d, FUNCTION %hhs\r\n", file, line, func);

    // Print error code & string if applicable
    if (status > 0 && status - TOP_BIT < MAX_EFI_ERROR)
        printf_c16(u"STATUS: %#llx (%s)\r\n", status, EFI_ERROR_STRINGS[status-TOP_BIT]);

    va_list args;
    va_start(args, fmt);
    bool result = vfprintf_c16(cerr, fmt, args); // Printf the error message to stderr
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

    UINT8 pos = 0;
    *number = 0;
    do {
        key = get_key();
        if (key.ScanCode == SCANCODE_ESC) return false; // User wants to leave

        // Backspace
        if (key.UnicodeChar == u'\b' && pos > 0) {
            printf_c16(u"\b"); 
            pos--;
            *number /= base;    // Remove last digit
        }

        if (isdigit_c16(key.UnicodeChar)) {
            *number = (*number * base) + (key.UnicodeChar - u'0');
            printf_c16(u"%c", key.UnicodeChar);
            pos++;

        } else if (base == 16) {
            if (key.UnicodeChar >= u'a' && key.UnicodeChar <= u'f') {
                *number = (*number * base) + (key.UnicodeChar - u'a' + 10);
                printf_c16(u"%c", key.UnicodeChar);
                pos++;

            } else if (key.UnicodeChar >= u'A' && key.UnicodeChar <= u'F') {
                *number = (*number * base) + (key.UnicodeChar - u'A' + 10);
                printf_c16(u"%c", key.UnicodeChar);
                pos++;
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
    printf_c16(u"{%x,%x,%x,%x,%x,{%x,%x,%x,%x,%x,%x}}\r\n",
            *(UINT32 *)&p[0], *(UINT16 *)&p[4], *(UINT16 *)&p[6], 
            (UINTN)p[8], (UINTN)p[9], (UINTN)p[10], (UINTN)p[11], (UINTN)p[12], (UINTN)p[13], 
            (UINTN)p[14], (UINTN)p[15]);
}

// ============================
// Print an ACPI table header
// ============================
void print_acpi_table_header(ACPI_TABLE_HEADER header) { 
    printf_c16(u"Signature: %.4hhs\r\n"
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
    printf_c16(u"Type: %u, Machine: %x, Entry Point: %#llx\r\n"
           u"Pgm headers offset: %u, Elf Header Size: %u\r\n"
           u"Pgm entry size: %u, # of Pgm headers: %u\r\n",
           ehdr->e_type, ehdr->e_machine, ehdr->e_entry,
           ehdr->e_phoff, ehdr->e_ehsize, 
           ehdr->e_phentsize, ehdr->e_phnum);
    
    ELF_Program_Header_64 *phdr = (ELF_Program_Header_64 *)((UINT8 *)ehdr + ehdr->e_phoff);
    printf_c16(u"\r\nLoadable Program Headers:\r\n");

    UINTN max_alignment = PAGE_SIZE;    
    UINTN mem_min = UINT64_MAX, mem_max = 0;
    for (UINT16 i = 0; i < ehdr->e_phnum; i++, phdr++) {
        // Only interested in loadable program headers
        if (phdr->p_type != PT_LOAD) continue;

        printf_c16(u"%u: Offset: %x, Vaddr: %x, Paddr: %x, FileSize: %x\r\n"
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
            printf_c16(u"\r\nPress any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }

    UINTN max_memory_needed = mem_max - mem_min;   
    printf_c16(u"\r\nMemory needed for file: %#llx bytes\r\n", max_memory_needed);
}

// =============================================================
// Print information for a PE32+ file
// NOTE: Assumes file is PIE (Position Independent Executable)
// =============================================================
void print_pe_info(VOID *pe_buffer) {
    const UINT8 pe_sig_offset = 0x3C; // From PE file format
    UINT32 pe_sig_pos = *(UINT32 *)((UINT8 *)pe_buffer + pe_sig_offset);
    UINT8 *pe_sig = (UINT8 *)pe_buffer + pe_sig_pos;

    printf_c16(u"Signature: [%hhx][%hhx][%hhx][%hhx] ",
           pe_sig[0], pe_sig[1], pe_sig[2], pe_sig[3]);

    // COFF header
    PE_Coff_File_Header_64 *coff_hdr = (PE_Coff_File_Header_64 *)(pe_sig + 4);
    printf_c16(u"Coff Header:\r\n"
           u"Machine: %x, # of sections: %u, Size of Opt Hdr: %x\r\n"
           u"Characteristics: %x\r\n",
           coff_hdr->Machine, coff_hdr->NumberOfSections, coff_hdr->SizeOfOptionalHeader,
           coff_hdr->Characteristics);

    // Optional header - right after COFF header
    PE_Optional_Header_64 *opt_hdr = (PE_Optional_Header_64 *)(coff_hdr + 1);

    printf_c16(u"\r\nOptional Header:\r\n"
           u"Magic: %x, Entry Point: %#llx\r\n" 
           u"Sect Align: %x, File Align: %x, Size of Image: %x\r\n"
           u"Subsystem: %x, DLL Characteristics: %x\r\n",
           opt_hdr->Magic, opt_hdr->AddressOfEntryPoint,
           opt_hdr->SectionAlignment, opt_hdr->FileAlignment, opt_hdr->SizeOfImage,
           (UINTN)opt_hdr->Subsystem, (UINTN)opt_hdr->DllCharacteristics);

    // Section headers
    PE_Section_Header_64 *shdr = 
        (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);

    printf_c16(u"\r\nSection Headers:\r\n");
    for (UINT16 i = 0; i < coff_hdr->NumberOfSections; i++, shdr++) {
        printf_c16(u"Name: ");
        char *pos = (char *)&shdr->Name;
        for (UINT8 j = 0; j < 8; j++) {
            CHAR16 str[2];
            str[0] = *pos;
            str[1] = u'\0';
            if (*pos == '\0') break;
            printf_c16(u"%s", str);
            pos++;
        }

        printf_c16(u" VSize: %x, Vaddr: %x, DataSize: %x, DataPtr: %x\r\n",
               shdr->VirtualSize, shdr->VirtualAddress, 
               shdr->SizeOfRawData, shdr->PointerToRawData);

        if (cout->Mode->CursorRow >= text_rows-2) {
            printf_c16(u"\r\nPress any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }
}

// ============================================================================
// Get EFI_FILE_PROTOCOL* to root directory '/' of EFI System Partition (ESP)
// NOTE: Caller must close open root directory pointer with 
//   e.g. "root->Close(root);"
// ============================================================================
EFI_FILE_PROTOCOL *esp_root_dir(VOID) {
    EFI_FILE_PROTOCOL *root = NULL;
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = NULL;
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_STATUS status = EFI_SUCCESS;
    status = bs->OpenProtocol(image,
                              &lip_guid,
                              (VOID **)&lip,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Loaded Image Protocol\r\n");
        goto done;
    }

    // Get Simple File System Protocol for the device handle for this loaded
    //   image, to open the root directory for the ESP
    status = bs->OpenProtocol(lip->DeviceHandle,
                              &sfsp_guid,
                              (VOID **)&sfsp,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Simple File System Protocol\r\n");
        goto done;
    }

    // Open root directory via OpenVolume()
    status = sfsp->OpenVolume(sfsp, &root);
    if (EFI_ERROR(status)) 
        error(status, u"Could not Open Volume for root directory\r\n");

    done:
    return root;
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

    root = esp_root_dir();
    if (!root) {
        error(0, u"Could not get root directory of ESP.\r\n");
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
        goto done;
    }

    BOOLEAN found = false;
    UINTN i = 0;
    for (; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &bio_guid,
                                  (VOID **)&biop,
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);

        if (EFI_ERROR(status)) {
            fprintf_c16(cerr, u"Could not Open Block IO protocol on handle %u.\r\n", i);
            continue;
        }

        if (biop->Media->MediaId == disk_mediaID && !biop->Media->LogicalPartition) {
            found = true;
            break;
        }
    }

    if (!found) {
        error(0, u"Could not find Block IO protocol for disk with ID %u.\r\n", disk_mediaID);
        goto done;
    }

    // Get Disk IO Protocol on same handle as Block IO protocol
    EFI_GUID dio_guid = EFI_DISK_IO_PROTOCOL_GUID;
    EFI_DISK_IO_PROTOCOL *diop;
    status = bs->OpenProtocol(handle_buffer[i], 
                              &dio_guid,
                              (VOID **)&diop,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Open Disk IO protocol on handle %u.\r\n", i);
        goto done;
    }

    // Allocate buffer for data
    UINTN pages_needed = (data_size + (PAGE_SIZE-1)) / PAGE_SIZE;
    status = bs->AllocatePages(AllocateAnyPages, 
                               executable ? EfiLoaderCode : EfiLoaderData, 
                               pages_needed, 
                               &buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Allocate buffer for disk data.\r\n");
        goto done;
    }

    // Use Disk IO Read to read into allocated buffer
    status = diop->ReadDisk(diop, disk_mediaID, disk_lba * biop->Media->BlockSize, data_size, (VOID *)buffer);
    if (EFI_ERROR(status)) 
        error(status, u"Could not read Disk LBAs into buffer.\r\n");

    done:
    if (handle_buffer) bs->FreePool(handle_buffer);   // Free allocated handle buffer

    return buffer;
}

// =======================================================================
// Get and set the largest HorizontalxVertical resolution GOP mode found
// =======================================================================
EFI_STATUS set_largest_gop_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL **ret_gop) {
    EFI_STATUS status = EFI_SUCCESS;

    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof *mode_info;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate GOP.\r\n");
        return status;
    }

    // Get current GOP mode information
    status = gop->QueryMode(gop, gop->Mode->Mode, &mode_info_size, &mode_info);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Query GOP Mode %u.\r\n", gop->Mode->Mode);
        return status;
    }

    UINT32 max_mode = gop->Mode->MaxMode;
    UINT32 largest_res_mode = 0;
    UINTN largest_res = 0;
    bool found_mode = false;
    for (UINT32 i = 0; i < max_mode; i++) {
        status = gop->QueryMode(gop, i, &mode_info_size, &mode_info);
        if (EFI_ERROR(status)) continue;

        if (mode_info->PixelFormat == PixelBltOnly || 
            mode_info->PixelFormat == PixelBitMask) 
            continue;   // No linear framebuffer

        UINTN temp_res = mode_info->HorizontalResolution * mode_info->VerticalResolution;
        if (temp_res > largest_res) {
            largest_res = temp_res;
            largest_res_mode = i;
            found_mode = true;
        }
    }

    if (!found_mode) 
        status = EFI_UNSUPPORTED;
    else if (largest_res_mode > 0) {
        status = gop->SetMode(gop, largest_res_mode);
        if (EFI_ERROR(status)) {
            error(status, u"Could not set GOP mode %u.\r\n", largest_res_mode);
            return status;
        }
        status = gop->QueryMode(gop, largest_res_mode, &mode_info_size, &mode_info);
    }

    *ret_gop = gop;
    return status;
}

// ================================================================
// Set the GOP mode with a given Horizontal & Vertical resolution 
// ================================================================
EFI_STATUS set_gop_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL **ret_gop, UINT32 xres, UINT32 yres) {
    EFI_STATUS status = EFI_SUCCESS;

    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof *mode_info;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate GOP.\r\n");
        return status;
    }

    // Get current GOP mode information
    status = gop->QueryMode(gop, gop->Mode->Mode, &mode_info_size, &mode_info);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Query GOP Mode %u.\r\n", gop->Mode->Mode);
        return status;
    }

    // Get first GOP mode with xres/yres values
    UINT32 max_mode = gop->Mode->MaxMode;
    UINT32 save_mode = 0;
    bool found_mode = false;
    for (UINT32 i = 0; i < max_mode; i++) {
        status = gop->QueryMode(gop, i, &mode_info_size, &mode_info);
        if (EFI_ERROR(status)) continue;

        if (mode_info->PixelFormat == PixelBltOnly || 
            mode_info->PixelFormat == PixelBitMask) 
            continue;   // No linear framebuffer

        if (mode_info->HorizontalResolution == xres && 
            mode_info->VerticalResolution   == yres) {
            save_mode = i;
            found_mode = true;
            break;
        }
    }

    if (!found_mode) 
        status = EFI_UNSUPPORTED;
    else {
        status = gop->SetMode(gop, save_mode);
        if (EFI_ERROR(status)) {
            error(status, u"Could not set GOP mode %u.\r\n", save_mode);
            return status;
        }
        status = gop->QueryMode(gop, save_mode, &mode_info_size, &mode_info);
    }

    *ret_gop = gop;
    return status;
}

// =========================================================================
// Check if a GOP mode exists for a given Horizontal & Vertical resolution 
// =========================================================================
EFI_STATUS check_gop_mode(UINT32 *ret_mode, UINT32 xres, UINT32 yres) {
    EFI_STATUS status = EFI_SUCCESS;

    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof *mode_info;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate GOP.\r\n");
        return status;
    }

    // Get current GOP mode information
    status = gop->QueryMode(gop, gop->Mode->Mode, &mode_info_size, &mode_info);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Query GOP Mode %u.\r\n", gop->Mode->Mode);
        return status;
    }

    // Get first GOP mode with xres/yres values
    UINT32 max_mode = gop->Mode->MaxMode;
    UINT32 save_mode = 0;
    bool found_mode = false;
    for (UINT32 i = 0; i < max_mode; i++) {
        status = gop->QueryMode(gop, i, &mode_info_size, &mode_info);
        if (EFI_ERROR(status)) continue;

        if (mode_info->PixelFormat == PixelBltOnly || 
            mode_info->PixelFormat == PixelBitMask) 
            continue;   // No linear framebuffer

        if (mode_info->HorizontalResolution == xres && 
            mode_info->VerticalResolution   == yres) {
            save_mode = i;
            found_mode = true;
            break;
        }
    }

    if (!found_mode)
        status = EFI_UNSUPPORTED;
    else {
        status = gop->QueryMode(gop, save_mode, &mode_info_size, &mode_info);
        if (EFI_ERROR(status)) {
            error(status, u"Could not get GOP mode %u info.\r\n", save_mode);
            return status;
        }
    }

    *ret_mode = save_mode;
    return status;
}

// ================================================
// Get Media ID value for this running disk image
// ================================================
EFI_STATUS get_disk_image_mediaID(UINT32 *mediaID) {
    EFI_STATUS status = EFI_SUCCESS;

    // Get media ID for this disk image 
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    status = bs->OpenProtocol(image,
                              &lip_guid,
                              (VOID **)&lip,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Loaded Image Protocol\r\n");
        goto done;
    }

    // Get Block IO protocol for loaded image's device handle
    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop = NULL;
    status = bs->OpenProtocol(lip->DeviceHandle,
                              &bio_guid,
                              (VOID **)&biop,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Block IO Protocol for this loaded image.\r\n");
        goto done;
    }

    *mediaID = biop->Media->MediaId;  // Media ID for this running disk image itself

    done:
    return status;
}

// ===============================================================
// Read a file in the GPT disk image's raw data partition,
//   using information found in the FILE.TXT file in the ESP,
//   created when making the disk image.
//
// Returns: 
//  - non-null pointer to allocated buffer with file data, 
//      allocated with Boot Services AllocatePool(), or NULL if not 
//      found or error.
//  - Size of returned buffer, if not NULL.
//
//  NOTE: Caller will have to use FreePool() on returned buffer to 
//    free allocated memory.
// ===============================================================
VOID *read_data_partition_file_to_buffer(char *in_name, bool executable, UINTN *ret_size) {
    VOID *esp_file = NULL;
    VOID *data_file = NULL;
    EFI_STATUS status = EFI_SUCCESS;

    // Get media ID (disk number for Block IO protocol Media) for this running disk image
    UINT32 image_mediaID = 0;
    status = get_disk_image_mediaID(&image_mediaID);
    if (EFI_ERROR(status)) {
        error(status, u"Could not find or get MediaID value for disk image\r\n");
        goto cleanup;
    }

    // Get FILE.TXT file from path "/EFI/BOOT/FILE.TXT"
    CHAR16 *file_name = u"\\EFI\\BOOT\\FILE.TXT";
    UINTN buf_size = 0;
    esp_file = read_esp_file_to_buffer(file_name, &buf_size);
    if (!esp_file) {
        error(0, u"Could not find or read file '%s' to buffer\r\n", file_name);
        goto cleanup;
    }

    // Get disk LBA and file size from FILE.TXT for input file name 
    char *str_pos = stpstr(esp_file, in_name);
    if (!str_pos) {
        error(0, u"Could not find file '%s' in data partition\r\n", in_name);
        goto cleanup;
    }

    str_pos = stpstr(str_pos, "FILE_SIZE=");
    if (!str_pos) {
        error(0, u"Could not find file size for '%s'\r\n", in_name);
        goto cleanup;
    }

    UINTN file_size = atoi(str_pos);

    str_pos = stpstr(str_pos, "DISK_LBA=");
    if (!str_pos) {
        error(0, u"Could not find disk lba value for '%s'\r\n", in_name);
        goto cleanup;
    }

    UINTN disk_lba = atoi(str_pos);

    // Read disk lbas for file into buffer
    data_file = (VOID *)read_disk_lbas_to_buffer(disk_lba, file_size, image_mediaID, executable);
    *ret_size = file_size;
    if (!data_file) {
        error(0, u"Could not find or read data partition file '%s' to buffer\r\n", in_name);
        *ret_size = 0;
    } 

    cleanup:
    if (esp_file) bs->FreePool(esp_file);  
    return data_file;
}

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

// ==================================================
// Allocate pages from available UEFI Memory Map;
//   technically not allocating more, but returning
//   available pre-existing page addresses.
//   Using this as a sort of bump allocator.
// ==================================================
void *mmap_allocate_pages(Memory_Map_Info *mmap, UINTN pages) {
    static void *next_page_address = NULL;  // Next page/page range address to return to caller
    static UINTN current_descriptor = 0;    // Current descriptor number
    static UINTN remaining_pages = 0;       // Remaining pages in current descriptor

    if (remaining_pages < pages) {
        // Not enough remaining pages in current descriptor, find the next available one
        UINTN i = current_descriptor+1;
        for (; i < mmap->size / mmap->desc_size; i++) {
            EFI_MEMORY_DESCRIPTOR *desc = 
                (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

            if (desc->Type == EfiConventionalMemory && desc->NumberOfPages >= pages) {
                // Found enough memory to use at this descriptor, use it
                current_descriptor = i;
                remaining_pages = desc->NumberOfPages - pages;
                next_page_address = (void *)(desc->PhysicalStart + (pages * PAGE_SIZE));
                return (void *)desc->PhysicalStart;
            }
        }

        if (i >= mmap->size / mmap->desc_size) {
            // Ran out of descriptors to check in memory map
            error(0, u"\r\nCould not find any memory to allocate pages for.\r\n");
            return NULL;
        }
    }

    // Else we have at least enough pages for this allocation, return the current spot in 
    //   the memory map
    remaining_pages -= pages;
    void *page = next_page_address;
    next_page_address = (void *)((UINT8 *)page + (pages * PAGE_SIZE));
    return page;
}

// ===========================================================
// Identity map a page of memory, virtual = physical address
// ===========================================================
extern void arch_map_page(uint64_t physical_address, uint64_t virtual_address, Memory_Map_Info *mmap);

void identity_map_page(UINTN address, Memory_Map_Info *mmap) {
    arch_map_page(address, address, mmap);
}

// ======================================================================
// Initialize new paging setup by identity mapping all available memory 
//   from EFI memory map
// ======================================================================
void identity_map_efi_mmap(Memory_Map_Info *mmap) {
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        for (UINTN j = 0; j < desc->NumberOfPages; j++)
            identity_map_page(desc->PhysicalStart + (j * PAGE_SIZE), mmap);
    }
}

// ======================================================================
// Identity map runtime memory descriptors only, to use with
//   RuntimeServices->SetVirtualAddressMap()
// ======================================================================
void set_runtime_address_map(Memory_Map_Info *mmap) {
    // First get amount of memory to allocate for runtime memory map
    UINTN runtime_descriptors = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        if (desc->Attribute & EFI_MEMORY_RUNTIME)
            runtime_descriptors++;
    }

    // Allocate memory for runtime memory map
    UINTN runtime_mmap_pages = (runtime_descriptors * mmap->desc_size) + ((PAGE_SIZE-1) / PAGE_SIZE);
    EFI_MEMORY_DESCRIPTOR *runtime_mmap = mmap_allocate_pages(mmap, runtime_mmap_pages);
    if (!runtime_mmap) {
        error(0, u"Could not allocate runtime descriptors memory map\r\n");
        return;
    }

    // Identity map all runtime descriptors in each descriptor
    UINTN runtime_mmap_size = runtime_mmap_pages * PAGE_SIZE; 
    memset(runtime_mmap, 0, runtime_mmap_size);

    // Set all runtime descriptors in new runtime memory map, and identity map them
    UINTN curr_runtime_desc = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        if (desc->Attribute & EFI_MEMORY_RUNTIME) {
            EFI_MEMORY_DESCRIPTOR *runtime_desc = 
                (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)runtime_mmap + (curr_runtime_desc * mmap->desc_size));

            memcpy(runtime_desc, desc, mmap->desc_size);    
            runtime_desc->VirtualStart = runtime_desc->PhysicalStart;
            curr_runtime_desc++;
        }
    }

    // Set new virtual addresses for runtime memory via SetVirtualAddressMap()
    EFI_STATUS status = rs->SetVirtualAddressMap(runtime_mmap_size, 
                                                 mmap->desc_size, 
                                                 mmap->desc_version,
                                                 runtime_mmap);
    if (EFI_ERROR(status)) error(0, u"SetVirtualAddressMap()\r\n");
}

