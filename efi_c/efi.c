#include "efi.h"

// Entry Point
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // TODO: Remove this line when using input parms
    (void)ImageHandle, (void)SystemTable;   // Prevent compiler warnings

    // Reset Console Output
    SystemTable->ConOut->Reset(SystemTable->ConOut, false);

    // Clear console output; clear screen to background color and
    //   set cursor to 0,0
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    // Write String
    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"TESTING, Hello UEFI World!\r\n");
    
    // Infinite Loop
    while (1) ;

    return EFI_SUCCESS;
}
