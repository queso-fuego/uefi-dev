// kernel.c: Sample "kernel" file for testing
#include <stdint.h>
#include "efi.h"
#include "efi_lib.h"

UINTN get_color(UINTN choice);

// MAIN
__attribute__((section(".kernel"))) void EFIAPI kmain(Kernel_Parms *kargs) {
    // Grab Framebuffer/GOP info
    UINT32 *fb = (UINT32 *)kargs->gop_mode.FrameBufferBase;  // ARGB8
    UINT32 xres = kargs->gop_mode.Info->PixelsPerScanLine;
    UINT32 yres = kargs->gop_mode.Info->VerticalResolution;

    // Clear screen to solid color
    UINTN color = get_color(1);
    for (UINT32 y = 0; y < yres; y++) 
        for (UINT32 x = 0; x < xres; x++) 
            fb[y*xres + x] = color;

    // Wait 5 seconds and then shutdown
    EFI_TIME old_time = {0}, new_time = {0};
    EFI_TIME_CAPABILITIES time_cap = {0};
    UINTN second = 0;
    while (second < 5) {
        kargs->RuntimeServices->GetTime(&new_time, &time_cap);
        if (old_time.Second != new_time.Second) {
            // Darken screen every second (skip Alpha channel)
            if (color > 0) color -= 0x00003300;
            for (UINT32 y = 0; y < yres; y++) 
                for (UINT32 x = 0; x < xres; x++) 
                    fb[y*xres + x] = color;

            old_time = new_time;
            second++;
        }
    }

    // Doesn't work at this point on my laptop, simply hangs
    //kargs.RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // This lets me use the power button on laptop to shutdown.
    // Probably need some sort of better ACPI set up, as ResetSystem() does work
    //   before ExitBootServices()/SetVirtualAddressMap()/paging/gdt setup.
    // GetTime() works above, so it doesn't seem to be a RuntimeServices() issue, 
    //   but more of an ACPI issue, which ResetSystem() uses.
    // May end up using the FADT Reset Register for the ACPI table "FACP"?
    while (1) __asm__ __volatile__ ("cli; hlt"); 
}

UINTN get_color(UINTN choice) {
    switch (choice) {
        case 1:
            return 0xFF00FF00; // Green ARGB8
            break;

        case 2:
            return 0xFFFF00FF; // Magenta ARGB8
            break;

        default:
            break;
    }

    return choice;
}

