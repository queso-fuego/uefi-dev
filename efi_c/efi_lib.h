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
} Kernel_Parms;












