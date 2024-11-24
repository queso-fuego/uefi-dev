//
// aarch64.h: Arch specific definitions
//
#pragma once

#include <stdint.h>

#include "../../../efi_lib.h"

#define ARCH_COFF_MACHINE 0xaa64    // Machine type bytes for PE Coff Header

// TODO:
void arch_setup_and_call_kernel(Entry_Point entry, void *kernel_stack, uint32_t stack_size, 
                                Kernel_Parms *kparms) {
    (void)entry, (void)kernel_stack, (void)stack_size, (void)kparms;
}

// TODO:
void arch_cpu_halt(void) {
}

// TODO:
void arch_map_page(uint64_t physical_address, uint64_t virtual_address, Memory_Map_Info *mmap) {
    (void)physical_address, (void)virtual_address, (void)mmap;
}

// TODO:
void arch_unmap_page(UINTN virtual_address) {
    (void)virtual_address;
}

// TODO:
void arch_init_page_tables(Memory_Map_Info *mmap) {
    void *page_table = mmap_allocate_pages(mmap, 1);
    memset(page_table, 0, PAGE_SIZE);  
}

