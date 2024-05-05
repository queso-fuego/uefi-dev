ENTRY(kmain);

SECTIONS {
    . = 0;
    .text : ALIGN(0x1000) {
        *(.kernel*);
        *(.text*);
    } 
    .data : ALIGN(0x1000) {
        *(.data*);
    }
    .rodata : ALIGN(0x1000) {
        *(.rodata*);
    }
    .bss : ALIGN(0x1000) {
        *(.bss*);
    }
}
