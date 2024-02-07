// kernel.c: Sample "kernel" file for testing
#include <stdint.h>
#include "efi.h"

typedef struct {
    void *memory_map;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE gop_mode;
} Kernel_Parms;

void EFIAPI kmain(Kernel_Parms kargs) {
    // Grab Framebuffer/GOP info
    UINT32 *fb = (UINT32 *)kargs.gop_mode.FrameBufferBase;  // BGRA8888
    UINT32 xres = kargs.gop_mode.Info->PixelsPerScanLine;
    UINT32 yres = kargs.gop_mode.Info->VerticalResolution;

    // Clear screen to solid color
    for (UINT32 y = 0; y < yres; y++) 
        for (UINT32 x = 0; x < xres; x++) 
            fb[y*xres + x] = 0xFFDDDDDD; // Light Gray AARRGGBB 8888

    // Draw square in top left
    for (UINT32 y = 0; y < yres / 5; y++) 
        for (UINT32 x = 0; x < xres / 5; x++) 
            fb[y*xres + x] = 0xFFCC2222; // AARRGGBB 8888

    while (1); // Infinite loop, do not return back to UEFI
}

