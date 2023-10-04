#include <stdarg.h>
#include "efi.h"

// -----------------
// Global constants
// -----------------
#define scancode_ESC 0x17

// -----------------
// Global variables
// -----------------
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;  // Console output
EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin = NULL;   // Console input
EFI_BOOT_SERVICES    *bs;   // Boot services
EFI_RUNTIME_SERVICES *rs;   // Runtime services
EFI_HANDLE image = NULL;    // Image handle

// ====================
// Set global vars
// ====================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
    bs = systable->BootServices;
    rs = systable->RuntimeServices;
    image = handle;
}

// ================================
// Print an interger (INT32 for now)
// ================================
bool print_int(INT32 number) {
    const CHAR16 *digits = u"0123456789";
    CHAR16 buffer[11];  // Enough for INT32_MAX + sign character
    UINTN i = 0;
    const bool negative = (number < 0);

    if (negative) number = -number;

    do {
       buffer[i++] = digits[number % 10];
       number /= 10;
    } while (number > 0);

    // Prepend minus sign if negative
    if (negative) buffer[i++] = u'-';

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse digits in buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Print number string
    cout->OutputString(cout, buffer);

    return true;
}

// ====================================
// Print a hexadecimal integer (UINTN)
// ====================================
bool print_hex(UINTN number) {
    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[20];  // Enough for UINTN_MAX, hopefully
    UINTN i = 0;

    do {
       buffer[i++] = digits[number % 16];
       number /= 16;
    } while (number > 0);

    // Prepend final string with 0x
    buffer[i++] = u'x';
    buffer[i++] = u'0';

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse digits in buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Print number string
    cout->OutputString(cout, buffer);

    return true;
}

// =========================
// Print formatted strings
// =========================
bool printf(CHAR16 *fmt, ...) {
    bool result = true;
    CHAR16 charstr[2];    // TODO: Replace initializing this with memset and use = { } initializer
    va_list args;

    va_start(args, fmt);

    // Initialize buffers
    charstr[0] = u'\0', charstr[1] = u'\0';

    // Print formatted string values
    for (UINTN i = 0; fmt[i] != u'\0'; i++) {
        if (fmt[i] == u'%') {
            i++;

            // Grab next argument type from input args, and print it
            switch (fmt[i]) {
                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    CHAR16 *string = va_arg(args, CHAR16*);
                    cout->OutputString(cout, string);
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    INT32 number = va_arg(args, INT32);
                    print_int(number);
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    print_hex(number);
                }
                break;

                default:
                    cout->OutputString(cout, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    cout->OutputString(cout, charstr);
                    cout->OutputString(cout, u"\r\n");
                    result = false;
                    goto end;
                    break;
            }
        } else {
            // Not formatted string, print next character
            charstr[0] = fmt[i];
            cout->OutputString(cout, charstr);
        }
    }

end:
    va_end(args);

    return result;
}

// ====================
// Get key from user
// ====================
EFI_INPUT_KEY get_key(void) {
    EFI_EVENT events[1];
    EFI_INPUT_KEY key;

    key.ScanCode = 0;
    key.UnicodeChar = u'\0';

    events[0] = cin->WaitForKey;
    UINTN index = 0;
    bs->WaitForEvent(1, events, &index);

    if (index == 0) cin->ReadKeyStroke(cin, &key);

    return key;
}

// ====================
// Entry Point
// ====================
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize global variables
    init_global_variables(ImageHandle, SystemTable);

    // Reset Console Output
    cout->Reset(SystemTable->ConOut, false);

    // Set text colors - foreground, background
    cout->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLUE));

    // Screen loop
    bool running = true;
    while (running) {
        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(SystemTable->ConOut);

        // Write String
        cout->OutputString(SystemTable->ConOut, u"Text mode information:\r\n");
        UINTN max_cols = 0, max_rows = 0;

        // Get current text mode's column and row counts
        cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);

        printf(u"Max Mode: %d\r\n"
               u"Current Mode: %d\r\n"
               u"Attribute: %x\r\n" 
               u"CursorColumn: %d\r\n"
               u"CursorRow: %d\r\n"
               u"CursorVisible: %d\r\n"
               u"Columns: %d\r\n"
               u"Rows: %d\r\n\r\n",
               cout->Mode->MaxMode,
               cout->Mode->Mode,
               cout->Mode->Attribute,
               cout->Mode->CursorColumn,
               cout->Mode->CursorRow,
               cout->Mode->CursorVisible,
               max_cols,
               max_rows);

        cout->OutputString(SystemTable->ConOut, u"Available text modes:\r\n");

        // Print other text mode infos
        const INT32 max = cout->Mode->MaxMode;
        for (INT32 i = 0; i < max; i++) {
            cout->QueryMode(cout, i, &max_cols, &max_rows);
            printf(u"Mode #: %d, %dx%d\r\n", i, max_cols, max_rows);
        }

        // Get number from user
        while (1) {
            static UINTN current_mode = 0;
            current_mode = cout->Mode->Mode;

            for (UINTN i = 0; i < 79; i++) printf(u" ");
            printf(u"\rSelect Text Mode # (0-%d): %d", max, current_mode);

            // Move cursor left by 1, to overwrite the mode #
            cout->SetCursorPosition(cout, cout->Mode->CursorColumn-1, cout->Mode->CursorRow);

            EFI_INPUT_KEY key = get_key();

            // Get key info
            CHAR16 cbuf[2];
            cbuf[0] = key.UnicodeChar;
            cbuf[1] = u'\0';
            //printf(u"Scancode: %x, Unicode Char: %s\r", key.ScanCode, cbuf);

            // Process keystroke
            printf(u"%s ", cbuf);

            if (key.ScanCode == scancode_ESC) {
                // ESC Key, Quit & Shutdown
                rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

                // !NOTE!: This should not return, system should power off
            }

            // Choose text mode & redraw screen
            current_mode = key.UnicodeChar - u'0';
            EFI_STATUS status = cout->SetMode(cout, current_mode);
            if (EFI_ERROR(status)) {
                // Handle error
                if (status == EFI_DEVICE_ERROR) { 
                    printf(u"ERROR: %x; Device Error\r", status);
                } else if (status == EFI_UNSUPPORTED) {
                    printf(u"ERROR: %x; Mode # is invalid\r", status);
                }
                get_key();
            } else {
                break; // Successfully set new text mode, redraw screen
            }
        }
    }

    return EFI_SUCCESS;
}
