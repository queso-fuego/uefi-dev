//
// efi_lib.h: Helper definitions that aren't in efi.h or the UEFI Spec
//
#pragma once

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

// List of known configuration table or other GUID values
typedef struct {
    EFI_GUID guid;
    CHAR16   *string;
} Guid_String;

Guid_String STANDARD_GUIDS[] = {
    {EFI_ACPI_TABLE_GUID,             u"EFI_ACPI_TABLE_GUID"},
    {ACPI_TABLE_GUID,                 u"ACPI_TABLE_GUID"},
    {SAL_SYSTEM_TABLE_GUID,           u"SAL_SYSTEM_TABLE_GUID"},
    {SMBIOS_TABLE_GUID,               u"SMBIOS_TABLE_GUID"},
    {SMBIOS3_TABLE_GUID,              u"SMBIOS3_TABLE_GUID"},
    {MPS_TABLE_GUID,                  u"MPS_TABLE_GUID"},
    {EFI_PROPERTIES_TABLE_GUID,       u"EFI_PROPERTIES_TABLE_GUID"},
    {EFI_SYSTEM_RESOURCES_TABLE_GUID, u"EFI_SYSTEM_RESOURCES_TABLE_GUID"},
    {EFI_SECTION_TIANO_COMPRESS_GUID, u"EFI_SECTION_TIANO_COMPRESS_GUID"},
    {EFI_SECTION_LZMA_COMPRESS_GUID,  u"EFI_SECTION_LZMA_COMPRESS_GUID"},
    {EFI_DXE_SERVICES_TABLE_GUID,     u"EFI_DXE_SERVICES_TABLE_GUID"},
    {EFI_HOB_LIST_GUID,               u"EFI_HOB_LIST_GUID"},
    {MEMORY_TYPE_INFORMATION_GUID,    u"MEMORY_TYPE_INFORMATION_GUID"},
    {EFI_DEBUG_IMAGE_INFO_TABLE_GUID, u"EFI_DEBUG_IMAGE_INFO_TABLE_GUID"},
};

// Memory map and associated meta data
typedef struct {
    UINTN                 size;
    EFI_MEMORY_DESCRIPTOR *map;
    UINTN                 key;
    UINTN                 desc_size;
    UINT32                desc_version;
} Memory_Map_Info;

// Example Kernel Parameters
typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE gop_mode;             // Current Graphics mode
    Memory_Map_Info                   mmap;                 // EFI Memory Map
    EFI_RUNTIME_SERVICES              *RuntimeServices;     // Pointer to system table runtime services
    UINTN                             NumberOfTableEntries; // Number of configuration tables
    EFI_CONFIGURATION_TABLE           *ConfigurationTable;  // Pointer to system table config tables
} Kernel_Parms;

// ====================================
// memset for compiling with clang/gcc:
// Sets len bytes of dst memory with int c
// Returns dst buffer
// ================================
VOID *memset(VOID *dst, UINT8 c, UINTN len) {
    UINT8 *p = dst;
    for (UINTN i = 0; i < len; i++)
        *p++ = c;

    return dst;
}

// ====================================
// memcpy for compiling with clang/gcc:
// Sets len bytes of dst memory from src.
// Assumes memory does not overlap!
// Returns dst buffer
// ================================
VOID *memcpy(VOID *dst, VOID *src, UINTN len) {
    UINT8 *p = dst;
    UINT8 *q = src;
    for (UINTN i = 0; i < len; i++)
        *p++ = *q++;

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
    while (*s) {
        len++;
        s++;
    }

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
        if (*p == *needle) {
            if (!memcmp(p, needle, strlen(needle))) return p;
        }
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

// =============================================
// (ASCII) atoi: 
// Converts intial value of input string to 
//   int.
// Returns converted value or 0 on error
// =============================================
INTN atoi(char *s) {
    INTN result = 0;
    while (isdigit(*s)) {
        result = (result * 10) + (*s - '0');
        s++;
    }

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

    while (*s) s++; // Go until null terminator

    while (*src) *s++ = *src++;

    *s = u'\0'; // I guess this isn't normal libc behavior? But seems better to null terminate

    return dst; 
}

