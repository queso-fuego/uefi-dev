// kernel.c: Sample "kernel" file for testing
#include <stdint.h>
#include "efi.h"
#include "efi_lib.h"

UINTN get_color(UINTN choice);

// MAIN
__attribute__((section(".kernel"), aligned(0x1000))) void EFIAPI kmain(Kernel_Parms *kargs) {
    // Grab Framebuffer/GOP info
    UINT32 *fb = (UINT32 *)kargs->gop_mode.FrameBufferBase;  // BGRA8888
    UINT32 xres = kargs->gop_mode.Info->PixelsPerScanLine;
    UINT32 yres = kargs->gop_mode.Info->VerticalResolution;

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

    // Test runtime services by waiting a few seconds and then shutting down
    EFI_TIME old_time = {0}, new_time = {0};
    EFI_TIME_CAPABILITIES time_cap = {0};
    UINTN i = 0;
    while (i < 3) {
        kargs->RuntimeServices->GetTime(&new_time, &time_cap);
        if (old_time.Second != new_time.Second) {
            i++;
            old_time.Second = new_time.Second;
        }
    }
    kargs->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // Infinite loop, do not return back to UEFI,
    //   this is in case my hardware (laptop) doesn't shut off from ResetSystem
    //   Can still use power button manually to shut down fine
    while (true) __asm__ __volatile__ ("cli; hlt");

    // Should not return after shutting down
    __builtin_unreachable();

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

