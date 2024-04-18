// kernel.c: Sample "kernel" file for testing
#include <stdint.h>
#include "efi.h"
#include "efi_lib.h"

UINTN get_color(UINTN choice);

// MAIN
__attribute__((section(".kernel"))) void EFIAPI kmain(Kernel_Parms kargs) {
    // Grab Framebuffer/GOP info
    UINT32 *fb = (UINT32 *)kargs.gop_mode.FrameBufferBase;  // BGRA8888
    UINT32 xres = kargs.gop_mode.Info->PixelsPerScanLine;
    UINT32 yres = kargs.gop_mode.Info->VerticalResolution;

    // Clear screen to solid color
    UINTN color = get_color(1);
    for (UINT32 y = 0; y < yres; y++) 
        for (UINT32 x = 0; x < xres; x++) 
            fb[y*xres + x] = color;

    // Draw square in top left
    color = get_color(2);
    for (UINT32 y = 0; y < yres / 5; y++) 
        for (UINT32 x = 0; x < xres / 5; x++) 
            fb[y*xres + x] = color; 

    while (1); // Infinite loop, do not return back to UEFI
}

UINTN get_color(UINTN choice) {
    switch (choice) {
        case 1:
            return 0xFFDDDDDD; // Light Gray AARRGGBB 8888
            break;

        case 2:
            return 0xFFCC2222; // AARRGGBB 8888
            break;

        default:
            break;
    }

    return choice;
}

