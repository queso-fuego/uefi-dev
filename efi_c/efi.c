#include <stdarg.h>
#include "efi.h"

// -----------------
// Global variables
// -----------------
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;  // Console output
//EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin = NULL;   // Console input
void  *cin = NULL;   // Console input
//void *bs; // Boot services
//void *rs; // Runtime services

// ====================
// Set global vars
// ====================
void init_global_variables(EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
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
    bool result = false;
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

    result = true;
    return result;
}

// ====================
// Entry Point
// ====================
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // TODO: Remove this line when using input parms
    (void)ImageHandle, (void)SystemTable;   // Prevent compiler warnings

    // Initialize global variables
    init_global_variables(SystemTable);

    // Reset Console Output
    cout->Reset(SystemTable->ConOut, false);

    // Set text colors - foreground, background
    cout->SetAttribute(SystemTable->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLUE));

    // Clear console output; clear screen to background color and
    //   set cursor to 0,0
    cout->ClearScreen(SystemTable->ConOut);

    // Printf Hex test
    printf(u"Testing hex: %x\r\n", (UINTN)0x11223344AABBCCDD);

    // Printf negative int test
    printf(u"Testing negative int: %d\r\n\r\n", (INT32)-54321);

    // Write String
    cout->OutputString(SystemTable->ConOut, u"Current text mode:\r\n");
    UINTN max_cols = 0, max_rows = 0;

    // Get current text mode's column and row counts
    cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);

    printf(u"Max Mode: %d\r\n"
           u"Current Mode: %d\r\n"
           u"Attribute: %d\r\n" // TODO: Change to %x and print hex instead
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

    // Infinite Loop
    while (1) ;

    return EFI_SUCCESS;
}
