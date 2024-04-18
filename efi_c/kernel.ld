ENTRY(kmain);
SECTIONS {
    .text : {
        KEEP(*(.kernel*));
        *(.text*);
    }
    .data : {
        *(.data*);
    }
    .rodata : {
        *(.rodata*);
    }
    .bss : {
        *(.bss*);
    }
}
