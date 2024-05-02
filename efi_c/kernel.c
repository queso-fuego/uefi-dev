// kernel.c: Sample "kernel" file for testing
#include <stdint.h>
#include "efi.h"
#include "efi_lib.h"

UINTN get_color(UINTN choice);

// MAIN
__attribute__((section(".kernel"))) void EFIAPI kmain(Kernel_Parms kargs) {
    (void)kargs;
    // Grab Framebuffer/GOP info
    UINT32 *fb = (UINT32 *)kargs.gop_mode.FrameBufferBase;  // ARGB8
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

    // Wait 5 seconds then shutdown
    EFI_TIME old_time, new_time;
    EFI_TIME_CAPABILITIES time_cap;
    UINTN i = 0;
    kargs.RuntimeServices->GetTime(&old_time, &time_cap);
    while (i < 5) {
        kargs.RuntimeServices->GetTime(&new_time, &time_cap);
        if (old_time.Second != new_time.Second) {
            old_time.Second = new_time.Second;
            i++;
        }
    }
    
    kargs.RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // Infinite loop, do not return back to UEFI
    //while (1) __asm__ __volatile__ ("hlt"); 
}

UINTN get_color(UINTN choice) {
    switch (choice) {
        case 1:
            return 0xFFFFFF00; // Yellow ARGB8
            break;

        case 2:
            return 0xFFFF00FF; // Magenta ARGB8
            break;

        default:
            break;
    }

    return choice;
}

