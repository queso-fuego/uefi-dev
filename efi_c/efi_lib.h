//
// efi_lib.h: Helper definitions that aren't in efi.h or the UEFI Spec
//
#pragma once

#include "efi.h"

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

// Example Kernel Parameters
typedef struct {
    Memory_Map_Info                   mmap; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE gop_mode;
    EFI_RUNTIME_SERVICES              *RuntimeServices;
    UINTN                             NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE           *ConfigurationTable;
} Kernel_Parms;

// Kernel entry point typedef
typedef void EFIAPI (*Entry_Point)(Kernel_Parms);

// EFI Configuration Table GUIDs and string names
typedef struct {
    EFI_GUID guid;
    char16_t *string;
} Efi_Guid_With_String;

// Default 
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

// ====================================
// memset for compiling with clang/gcc:
// Sets len bytes of dst memory with int c
// Returns dst buffer
// ================================
VOID *memset(VOID *dst, UINT8 c, UINTN len) {
    UINT8 *p = dst;
    for (UINTN i = 0; i < len; i++)
        p[i] = c;

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
        p[i] = q[i];

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
















