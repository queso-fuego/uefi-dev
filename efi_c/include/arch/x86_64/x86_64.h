//
// x86_64.h: Arch specific definitions
//
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "../../../efi_lib.h"

// GDT = Global Descriptor Table
// LDT = Local Descriptor Table
// TSS = Task State Segment

// Use this for GDTR with assembly "LGDT" instruction
typedef struct {
    uint16_t limit;
    uint64_t base; 
} __attribute__((packed)) Descriptor_Register;

// Descriptor e.g. an array of these is used for the GDT/LDT
typedef struct {
    union {
        uint64_t value;
        struct {
            uint64_t limit_15_0:  16;
            uint64_t base_15_0:   16;
            uint64_t base_23_16:  8;
            uint64_t type:        4;
            uint64_t s:           1;  // System descriptor (0), Code/Data segment (1)
            uint64_t dpl:         2;  // Descriptor Privelege Level
            uint64_t p:           1;  // Present flag
            uint64_t limit_19_16: 4;
            uint64_t avl:         1;  // Available for use
            uint64_t l:           1;  // 64 bit (Long mode) code segment
            uint64_t d_b:         1;  // Default op size/stack pointer size/upper bound flag
            uint64_t g:           1;  // Granularity; 1 byte (0), 4KiB (1)
            uint64_t base_31_24:  8;
        };
    };
} X86_64_Descriptor;

// TSS/LDT descriptor - 64 bit
typedef struct {
    X86_64_Descriptor descriptor;
    uint32_t base_63_32;
    uint32_t zero;
} TSS_LDT_Descriptor;

// TSS structure - TSS descriptor points to this structure in the GDT
typedef struct {
    uint32_t reserved_1;
    uint32_t RSP0_lower;
    uint32_t RSP0_upper;
    uint32_t RSP1_lower;
    uint32_t RSP1_upper;
    uint32_t RSP2_lower;
    uint32_t RSP2_upper;
    uint32_t reserved_2;
    uint32_t reserved_3;
    uint32_t IST1_lower;
    uint32_t IST1_upper;
    uint32_t IST2_lower;
    uint32_t IST2_upper;
    uint32_t IST3_lower;
    uint32_t IST3_upper;
    uint32_t IST4_lower;
    uint32_t IST4_upper;
    uint32_t IST5_lower;
    uint32_t IST5_upper;
    uint32_t IST6_lower;
    uint32_t IST6_upper;
    uint32_t IST7_lower;
    uint32_t IST7_upper;
    uint32_t reserved_4;
    uint32_t reserved_5;
    uint16_t reserved_6;
    uint16_t io_map_base;
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

// Page table structure: 512 64bit entries per table/level
typedef struct {
    uint64_t entries[512];
} Page_Table;

// Page flags: bits 11-0
enum {
    PRESENT    = (1 << 0),
    READWRITE  = (1 << 1),
    USER       = (1 << 2),
};

#define ARCH_COFF_MACHINE 0x8664    // Machine type bytes for PE Coff Header

#define PHYS_PAGE_ADDR_MASK 0x000FFFFFFFFFF000  // 52 bit physical address limit, lowest 12 bits are for flags only

// ---------------------
// Global variables
// ---------------------
Page_Table *pml4 = NULL;        // Top level 4 page table for x86_64 long mode paging

// ---------------------
// Functions
// ---------------------
extern void *mmap_allocate_pages(Memory_Map_Info *mmap, uint64_t pages);
extern void *memset(void *dst, uint8_t c, uint64_t len);

// Clear interrupts and halt CPU
void arch_cpu_halt(void) {
    __asm__ ("cli; hlt");
}

// ===================================
// Return example Task State Segment
// ===================================
TSS example_tss(void) {
    // All bits after limit (in TSS descriptor) assumed to be '1'
    return (TSS){.io_map_base = sizeof(TSS)}; 
}

// ========================================
// Return example Global Descriptor Table
// ========================================
GDT example_gdt(TSS tss, uint64_t tss_address) {
    return (GDT){
        .null.value           = 0x0000000000000000, // Null descriptor

        .kernel_code_64.value = 0x00AF9A000000FFFF,
        .kernel_data_64.value = 0x00CF92000000FFFF,

        .user_code_64.value   = 0x00AFFA000000FFFF,
        .user_data_64.value   = 0x00CFF2000000FFFF,

        .kernel_code_32.value = 0x00CF9A000000FFFF,
        .kernel_data_32.value = 0x00CF92000000FFFF,

        .user_code_32.value   = 0x00CFFA000000FFFF,
        .user_data_32.value   = 0x00CFF2000000FFFF,

        .tss = {
            .descriptor = {
                .limit_15_0 = sizeof tss - 1,
                .base_15_0  = tss_address & 0xFFFF, 
                .base_23_16 = (tss_address >> 16) & 0xFF, 
                .type       = 9,    // 0b1001 64 bit TSS (available)
                .p          = 1,    // Present
                .base_31_24 = (tss_address >> 24) & 0xFF,
            },
            .base_63_32 = (tss_address >> 32) & 0xFFFFFFFF,
        }
    };
}

// ============================================================================
// Set page tables & paging, do other arch specific settings, and call kernel
// ============================================================================
void arch_setup_and_call_kernel(Entry_Point entry, void *kernel_stack, uint32_t stack_size, 
                                Kernel_Parms *kparms) {
    TSS tss = example_tss();
    uint64_t tss_address = (UINTN)&tss;
    GDT gdt = example_gdt(tss, tss_address);
    Descriptor_Register gdtr = {.limit = sizeof gdt - 1, .base = (uint64_t)&gdt}; 

    // Set new page tables (CR3 = PML4) and GDT (lgdt && ltr), and call entry point with parms
    __asm__ __volatile__(
        "cli\n"                     // Clear interrupts before setting new GDT/TSS, etc.
        "movq %[pml4], %%CR3\n"     // Load new page tables
        "lgdt %[gdt]\n"             // Load new GDT from gdtr register
        "ltr %[tss]\n"              // Load new task register with new TSS value (byte offset into GDT)

        // Jump to new code segment in GDT (offset in GDT of 64 bit kernel/system code segment)
        "pushq $0x8\n"
        "leaq 1f(%%RIP), %%RAX\n"
        "pushq %%RAX\n"
        "lretq\n"

        // Executing code with new Code segment now, set up remaining segment registers
        "1:\n"
        "movq $0x10, %%RAX\n"   // Data segment to use (64 bit kernel data segment, offset in GDT)
        "movq %%RAX, %%DS\n"    // Data segment
        "movq %%RAX, %%ES\n"    // Extra segment
        "movq %%RAX, %%FS\n"    // Extra segment (2), these also have different uses in Long Mode
        "movq %%RAX, %%GS\n"    // Extra segment (3), these also have different uses in Long Mode
        "movq %%RAX, %%SS\n"    // Stack segment

        // Set new stack value to use (for SP/stack pointer, etc.)
        "movq %[stack], %%RSP\n"

        // Call new entry point in higher memory
        "callq *%[entry]\n" // First parameter is kparms in RCX in input constraints below, for MS ABI
      :
      : [pml4]"r"(pml4), [gdt]"m"(gdtr), [tss]"r"((uint16_t)offsetof(GDT, tss)),
        [stack]"gm"((uint64_t)kernel_stack + stack_size),    // Top of newly allocated stack
        [entry]"r"(entry), "c"(kparms)
      : "rax", "memory");
}

// ==================================================================
// Map a virtual address to a physical address for a page of memory
// ==================================================================
void arch_map_page(uint64_t physical_address, uint64_t virtual_address, Memory_Map_Info *mmap) {
    int flags = PRESENT | READWRITE | USER;   // 0b111

    uint64_t pml4_index = ((virtual_address) >> 39) & 0x1FF;   // 0-511
    uint64_t pdpt_index = ((virtual_address) >> 30) & 0x1FF;   // 0-511
    uint64_t pdt_index  = ((virtual_address) >> 21) & 0x1FF;   // 0-511
    uint64_t pt_index   = ((virtual_address) >> 12) & 0x1FF;   // 0-511

    // Make sure pdpt exists, if not then allocate it
    if (!(pml4->entries[pml4_index] & PRESENT)) {
        void *pdpt_address = mmap_allocate_pages(mmap, 1);

        memset(pdpt_address, 0, sizeof(Page_Table));
        pml4->entries[pml4_index] = (uint64_t)pdpt_address | flags;  
    }

    // Make sure pdt exists, if not then allocate it
    Page_Table *pdpt = (Page_Table *)(pml4->entries[pml4_index] & PHYS_PAGE_ADDR_MASK);
    if (!(pdpt->entries[pdpt_index] & PRESENT)) {
        void *pdt_address = mmap_allocate_pages(mmap, 1);

        memset(pdt_address, 0, sizeof(Page_Table));
        pdpt->entries[pdpt_index] = (uint64_t)pdt_address | flags;  
    }

    // Make sure pt exists, if not then allocate it
    Page_Table *pdt = (Page_Table *)(pdpt->entries[pdpt_index] & PHYS_PAGE_ADDR_MASK);
    if (!(pdt->entries[pdt_index] & PRESENT)) {
        void *pt_address = mmap_allocate_pages(mmap, 1);

        memset(pt_address, 0, sizeof(Page_Table));
        pdt->entries[pdt_index] = (uint64_t)pt_address | flags;  
    }

    // Map new page physical address if not present
    Page_Table *pt = (Page_Table *)(pdt->entries[pdt_index] & PHYS_PAGE_ADDR_MASK);
    if (!(pt->entries[pt_index] & PRESENT)) 
        pt->entries[pt_index] = (physical_address & PHYS_PAGE_ADDR_MASK) | flags;
}

// ==============================
// Unmap a page/virtual address 
// ==============================
void arch_unmap_page(UINTN virtual_address) {
    uint64_t pml4_index = ((virtual_address) >> 39) & 0x1FF;   // 0-511
    uint64_t pdpt_index = ((virtual_address) >> 30) & 0x1FF;   // 0-511
    uint64_t pdt_index  = ((virtual_address) >> 21) & 0x1FF;   // 0-511
    uint64_t pt_index   = ((virtual_address) >> 12) & 0x1FF;   // 0-511

    Page_Table *pdpt = (Page_Table *)(pml4->entries[pml4_index] & PHYS_PAGE_ADDR_MASK);
    Page_Table *pdt = (Page_Table *)(pdpt->entries[pdpt_index] & PHYS_PAGE_ADDR_MASK);
    Page_Table *pt = (Page_Table *)(pdt->entries[pdt_index] & PHYS_PAGE_ADDR_MASK);

    pt->entries[pt_index] = 0;  // Clear page in page table to unmap the physical address there

    // Flush the TLB cache for this page
    __asm__ ("invlpg (%0)\n" : : "r"(virtual_address));
}

// =============================================================
// Initialize page tables by setting up new level 4 page table
// =============================================================
void arch_init_page_tables(Memory_Map_Info *mmap) {
    pml4 = mmap_allocate_pages(mmap, 1);
    memset(pml4, 0, sizeof *pml4);  
}

