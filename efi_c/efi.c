#include <stdarg.h>
#include "efi.h"

// -----------------
// Global macros
// -----------------
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x)[0])

// -----------------
// Global constants
// -----------------
#define SCANCODE_UP_ARROW   0x1
#define SCANCODE_DOWN_ARROW 0x2
#define SCANCODE_ESC        0x17

#define DEFAULT_FG_COLOR EFI_YELLOW
#define DEFAULT_BG_COLOR EFI_BLUE

#define HIGHLIGHT_FG_COLOR EFI_BLUE
#define HIGHLIGHT_BG_COLOR EFI_CYAN

// EFI_GRAPHICS_OUTPUT_BLT_PIXEL values, BGRr8888
#define px_LGRAY {0xEE,0xEE,0xEE,0x00}
#define px_BLACK {0x00,0x00,0x00,0x00}
#define px_BLUE  {0x98,0x00,0x00,0x00}  // EFI_BLUE

#ifdef __clang__
int _fltused = 0;   // If using floating point code & lld-link, need to define this
#endif

// -----------------
// Global variables
// -----------------
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;  // Console output
EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin  = NULL;  // Console input
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr = NULL;  // Console output - stderr
EFI_BOOT_SERVICES    *bs;   // Boot services
EFI_RUNTIME_SERVICES *rs;   // Runtime services
EFI_HANDLE image = NULL;    // Image handle

// Mouse cursor buffer 8x8
EFI_GRAPHICS_OUTPUT_BLT_PIXEL cursor_buffer[] = {
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, // Line 1
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, // Line 2
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE, px_BLUE,     // Line 3
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE,    // Line 4
    px_LGRAY, px_LGRAY, px_BLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE, px_BLUE,    // Line 5
    px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE,    // Line 6
    px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE, px_LGRAY, px_LGRAY, px_LGRAY,    // Line 7
    px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE, px_BLUE, px_LGRAY, px_LGRAY,     // Line 8
};

// Buffer to save Framebuffer data at cursor position
EFI_GRAPHICS_OUTPUT_BLT_PIXEL save_buffer[8*8] = {0};

// ====================
// Set global vars
// ====================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
    //cerr = systable->StdErr; // Stderr can be set to a serial output or other non-display device.
    cerr = cout;  // Use stdout for error printing 
    bs = systable->BootServices;
    rs = systable->RuntimeServices;
    image = handle;
}

// ====================================
// memset for compiling with clang/gcc:
// Sets len bytes of dst memory with int c
// Returns dst buffer
// ================================
VOID *memset(VOID *dst, UINT8 c, UINTN len) {
    UINT8 *p = dst;
    for (UINTN i = 0; i < len; i++)
        p[i] = c;

    return dst;
}

// ====================================
// memcpy for compiling with clang/gcc:
// Sets len bytes of dst memory from src.
// Assumes memory does not overlap!
// Returns dst buffer
// ================================
VOID *memcpy(VOID *dst, VOID *src, UINTN len) {
    UINT8 *p = dst;
    UINT8 *q = src;
    for (UINTN i = 0; i < len; i++)
        p[i] = q[i];

    return dst;
}

// =============================================================================
// memcmp:
// Compare up to len bytes of m1 and m2, stop at first
//   point that they don't equal.
// Returns 0 if equal, >0 if m1 is greater than m2, <0 if m2 is greater than m1
// =============================================================================
INTN memcmp(VOID *m1, VOID *m2, UINTN len) {
    UINT8 *p = m1;
    UINT8 *q = m2;
    for (UINTN i = 0; i < len; i++)
        if (p[i] != q[i]) return (INTN)p[1] - (INTN)p[2];

    return 0;
}

// ================================
// CHAR16 strcpy:
//   Copy src string into dst 
//   Returns dst
// ================================
CHAR16 *strcpy_u16(CHAR16 *dst, CHAR16 *src) {
    if (!dst) return NULL;
    if (!src) return dst;

    CHAR16 *result = dst;
    while (*src) *dst++ = *src++;

    *dst = u'\0';   // Null terminate

    return result;
}

// ================================
// CHAR16 strncmp:
//   Compare 2 strings, each character, up to at most len bytes
//   Returns difference in strings at last point of comparison:
//   0 if strings are equal, <0 if s2 is greater, >0 if s1 is greater
// ================================
INTN strncmp_u16(CHAR16 *s1, CHAR16 *s2, UINTN len) {
    if (len == 0) return 0;

    while (len > 0 && *s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
        len--;
    }

    return *s1 - *s2;
}

// ================================
// CHAR16 strrchr:
//   Return last occurrence of C in string
// ================================
CHAR16 *strrchr_u16(CHAR16 *str, CHAR16 c) {
    CHAR16 *result = NULL;

    while (*str) {
        if (*str == c) result = str;
        str++;
    }

    return result;
}

// ================================
// CHAR16 strcat:
//   Concatenate src string onto the end of dst string, at
//   dst's original null terminator position.
//  Returns dst
// ================================
CHAR16 *strcat_u16(CHAR16 *dst, CHAR16 *src) {
    CHAR16 *s = dst;

    while (*s) s++; // Go until null terminator

    while (*src) *s++ = *src++;

    *s = u'\0'; // I guess this isn't normal libc behavior? But seems better to null terminate

    return dst; 
}

// ================================
// Print a number to stderr
// ================================
BOOLEAN eprint_number(UINTN number, UINT8 base, BOOLEAN is_signed) {
    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[24];  // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
    UINTN i = 0;
    BOOLEAN negative = FALSE;

    if (base > 16) {
        cerr->OutputString(cerr, u"Invalid base specified!\r\n");
        return FALSE;    // Invalid base
    }

    // Only use and print negative numbers if decimal and signed True
    if (base == 10 && is_signed && (INTN)number < 0) {
       number = -(INTN)number;  // Get absolute value of correct signed value to get digits to print
       negative = TRUE;
    }

    do {
       buffer[i++] = digits[number % base];
       number /= base;
    } while (number > 0);

    switch (base) {
        case 2:
            // Binary
            buffer[i++] = u'b';
            buffer[i++] = u'0';
            break;

        case 8:
            // Octal
            buffer[i++] = u'o';
            buffer[i++] = u'0';
            break;

        case 10:
            // Decimal
            if (negative) buffer[i++] = u'-';
            break;

        case 16:
            // Hexadecimal
            buffer[i++] = u'x';
            buffer[i++] = u'0';
            break;

        default:
            // Maybe invalid base, but we'll go with it (no special processing)
            break;
    }

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Print number string
    cerr->OutputString(cerr, buffer);

    return TRUE;
}

// ================================
// Print a number to stdout
// ================================
BOOLEAN print_number(UINTN number, UINT8 base, BOOLEAN is_signed) {
    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[24];  // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
    UINTN i = 0;
    BOOLEAN negative = FALSE;

    if (base > 16) {
        cerr->OutputString(cerr, u"Invalid base specified!\r\n");
        return FALSE;    // Invalid base
    }

    // Only use and print negative numbers if decimal and signed True
    if (base == 10 && is_signed && (INTN)number < 0) {
       number = -(INTN)number;  // Get absolute value of correct signed value to get digits to print
       negative = TRUE;
    }

    do {
       buffer[i++] = digits[number % base];
       number /= base;
    } while (number > 0);

    switch (base) {
        case 2:
            // Binary
            buffer[i++] = u'b';
            buffer[i++] = u'0';
            break;

        case 8:
            // Octal
            buffer[i++] = u'o';
            buffer[i++] = u'0';
            break;

        case 10:
            // Decimal
            if (negative) buffer[i++] = u'-';
            break;

        case 16:
            // Hexadecimal
            buffer[i++] = u'x';
            buffer[i++] = u'0';
            break;

        default:
            // Maybe invalid base, but we'll go with it (no special processing)
            break;
    }

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Print number string
    cout->OutputString(cout, buffer);

    return TRUE;
}

// ====================================
// Print formatted strings to stderr
// ====================================
bool eprintf(CHAR16 *fmt, va_list args) {
    bool result = true;
    CHAR16 charstr[2];    // TODO: Replace initializing this with memset and use = { } initializer

    // Initialize buffers
    charstr[0] = u'\0', charstr[1] = u'\0';

    // Print formatted string values
    for (UINTN i = 0; fmt[i] != u'\0'; i++) {
        if (fmt[i] == u'%') {
            i++;

            // Grab next argument type from input args, and print it
            switch (fmt[i]) {
                case u'c': {
                    // Print CHAR16 value; printf("%c", char)
                    charstr[0] = va_arg(args, int); // Compiler warning says to do this
                    cerr->OutputString(cerr, charstr);
                }
                break;

                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    CHAR16 *string = va_arg(args, CHAR16*);
                    cerr->OutputString(cerr, string);
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    INT32 number = va_arg(args, INT32);
                    eprint_number(number, 10, TRUE);
                }
                break;

                case u'u': {
                    // Print UINT32; printf("%u", number_uint32)
                    UINT32 number = va_arg(args, UINT32);
                    eprint_number(number, 10, FALSE);
                }
                break;

                case u'b': {
                    // Print UINTN as binary; printf("%b", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    eprint_number(number, 2, FALSE);
                }
                break;

                case u'o': {
                    // Print UINTN as octal; printf("%o", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    eprint_number(number, 8, FALSE);
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    eprint_number(number, 16, FALSE);
                }
                break;

                default:
                    cerr->OutputString(cerr, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    cerr->OutputString(cerr, charstr);
                    cerr->OutputString(cerr, u"\r\n");
                    result = false;
                    goto end;
                    break;
            }
        } else {
            // Not formatted string, print next character
            charstr[0] = fmt[i];
            cerr->OutputString(cerr, charstr);
        }
    }

end:
    return result;
}

// ===================================
// Print formatted strings to stdout
// ===================================
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
                case u'c': {
                    // Print CHAR16 value; printf("%c", char)
                    charstr[0] = va_arg(args, int); // Compiler warning says to do this
                    cout->OutputString(cout, charstr);
                }
                break;

                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    CHAR16 *string = va_arg(args, CHAR16*);
                    cout->OutputString(cout, string);
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    INT32 number = va_arg(args, INT32);
                    //print_int(number);
                    print_number(number, 10, TRUE);
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    //print_hex(number);
                    print_number(number, 16, FALSE);
                }
                break;

                case u'u': {
                    // Print UINT32; printf("%u", number_uint32)
                    UINT32 number = va_arg(args, UINT32);
                    print_number(number, 10, FALSE);
                }
                break;

                case u'b': {
                    // Print UINTN as binary; printf("%b", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    eprint_number(number, 2, FALSE);
                }
                break;

                case u'o': {
                    // Print UINTN as octal; printf("%o", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    eprint_number(number, 8, FALSE);
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

// ======================================================================
// Print error message and get a key from user,
//  so they can acknowledge the error and it doesn't go on immediately.
// ======================================================================
bool error(CHAR16 *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    bool result = eprintf(fmt, args); // Printf the error message to stderr
    va_end(args);

    get_key();  // User will respond with input before going on

    return result;
}

// ====================
// Set Text Mode
// ====================
EFI_STATUS set_text_mode(void) {
    // Store found Text mode info
    typedef struct {
        UINTN cols;
        UINTN rows;
    } Text_Mode_Info;

    Text_Mode_Info text_modes[20];

    UINTN mode_index = 0;   // Current mode within entire menu of GOP mode choices;

    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Write String
        cout->OutputString(cout, u"Text mode information:\r\n");
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

        cout->OutputString(cout, u"Available text modes:\r\n");

        UINTN menu_top = cout->Mode->CursorRow, menu_bottom = max_rows;

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, menu_bottom-3);
        printf(u"Up/Down Arrow = Move Cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Go Back");

        cout->SetCursorPosition(cout, 0, menu_top);
        menu_bottom -= 5;   // Bottom of menu will be 2 rows above keybinds
        UINTN menu_len = menu_bottom - menu_top;

        // Get all available Text modes' info
        const UINT32 max = cout->Mode->MaxMode;
        if (max < menu_len) {
            // Bound menu by actual # of available modes
            menu_bottom = menu_top + max-1;
            menu_len = menu_bottom - menu_top;  // Limit # of modes in menu to max mode - 1
        }

        for (UINT32 i = 0; i < ARRAY_SIZE(text_modes) && i < max; i++) 
            cout->QueryMode(cout, i, &text_modes[i].cols, &text_modes[i].rows);

        // Highlight top menu row to start off
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"Mode %d: %dx%d", 0, text_modes[0].cols, text_modes[0].rows);

        // Print other text mode infos
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINT32 i = 1; i < menu_len + 1; i++) 
            printf(u"\r\nMode %d: %dx%d", i, text_modes[i].cols, text_modes[i].rows);

        // Get input from user
        cout->SetCursorPosition(cout, 0, menu_top);
        bool getting_input = true;
        while (getting_input) {
            UINTN current_row = cout->Mode->CursorRow;

            EFI_INPUT_KEY key = get_key();
            switch (key.ScanCode) {
                case SCANCODE_ESC: return EFI_SUCCESS;  // ESC Key: Go back to main menu

                case SCANCODE_UP_ARROW:
                    if (current_row == menu_top && mode_index > 0) {
                        // Scroll menu up by decrementing all modes by 1
                        printf(u"                    \r");  // Blank out mode text first

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        mode_index--;
                        printf(u"Mode %d: %dx%d", 
                               mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        UINTN temp_mode = mode_index + 1;
                        for (UINT32 i = 0; i < menu_len; i++, temp_mode++) {
                            printf(u"\r\n                    \r"  // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
                                   temp_mode, text_modes[temp_mode].cols, text_modes[temp_mode].rows);
                        }

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                    } else if (current_row-1 >= menu_top) {
                        // De-highlight current row, move up 1 row, highlight new row
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);

                        mode_index--;
                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                case SCANCODE_DOWN_ARROW:
                    // NOTE: Max valid GOP mode is ModeMax-1 per UEFI spec
                    if (current_row == menu_bottom && mode_index < max-1) {
                        // Scroll menu down by incrementing all modes by 1
                        mode_index -= menu_len - 1;

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                        // Print modes up until the last menu row
                        for (UINT32 i = 0; i < menu_len; i++, mode_index++) {
                            printf(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r\n", 
                                   mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);
                        }

                        // Highlight last row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);

                    } else if (current_row+1 <= menu_bottom) {
                        // De-highlight current row, move down 1 row, highlight new row
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r\n", 
                               mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);

                        mode_index++;
                        current_row++;
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, text_modes[mode_index].cols, text_modes[mode_index].rows);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                default:
                    if (key.UnicodeChar == u'\r' && text_modes[mode_index].cols != 0) {	// Qemu can have invalid text modes
                        // Enter key, set Text mode
                        cout->SetMode(cout, mode_index);
                        cout->QueryMode(cout, mode_index, &text_modes[mode_index].cols, &text_modes[mode_index].rows);

                        // Clear text screen
			cout->ClearScreen(cout);

                        getting_input = false;  // Will leave input loop and redraw screen
                        mode_index = 0;         // Reset last selected mode in menu
                    }
                    break;
            }
        }
    }

    return EFI_SUCCESS;
}

// ====================
// Set Graphics Mode
// ====================
EFI_STATUS set_graphics_mode(void) {
    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    EFI_STATUS status = 0;
    UINTN mode_index = 0;   // Current mode within entire menu of GOP mode choices;

    // Store found GOP mode info
    typedef struct {
        UINT32 width;
        UINT32 height;
    } Gop_Mode_Info;

    Gop_Mode_Info gop_modes[50];

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not locate GOP! :(\r\n", status);
        return status;
    }

    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Write String
        printf(u"Graphics mode information:\r\n");

        // Get current GOP mode information
        status = gop->QueryMode(gop, 
                                gop->Mode->Mode, 
                                &mode_info_size, 
                                &mode_info);

        if (EFI_ERROR(status)) {
            error(u"\r\nERROR: %x; Could not Query GOP Mode %u\r\n", status, gop->Mode->Mode);
            return status;
        }

        // Print info
        printf(u"Max Mode: %d\r\n"
               u"Current Mode: %d\r\n"
               u"WidthxHeight: %ux%u\r\n"
               u"Framebuffer address: %x\r\n"
               u"Framebuffer size: %x\r\n"
               u"PixelFormat: %d\r\n"
               u"PixelsPerScanLine: %u\r\n",
               gop->Mode->MaxMode,
               gop->Mode->Mode,
               mode_info->HorizontalResolution, mode_info->VerticalResolution,
               gop->Mode->FrameBufferBase,
               gop->Mode->FrameBufferSize,
               mode_info->PixelFormat,
               mode_info->PixelsPerScanLine);

        cout->OutputString(cout, u"\r\nAvailable GOP modes:\r\n");

        // Get current text mode ColsxRows values
        UINTN menu_top = cout->Mode->CursorRow, menu_bottom = 0, max_cols;
        cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &menu_bottom);

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, menu_bottom-3);
        printf(u"Up/Down Arrow = Move Cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Go Back");

        cout->SetCursorPosition(cout, 0, menu_top);
        menu_bottom -= 5;   // Bottom of menu will be 2 rows above keybinds
        UINTN menu_len = menu_bottom - menu_top;

        // Get all available GOP modes' info
        const UINT32 max = gop->Mode->MaxMode;
        if (max < menu_len) {
            // Bound menu by actual # of available modes
            menu_bottom = menu_top + max-1;
            menu_len = menu_bottom - menu_top;  // Limit # of modes in menu to max mode - 1
        }

        for (UINT32 i = 0; i < ARRAY_SIZE(gop_modes) && i < max; i++) {
            gop->QueryMode(gop, i, &mode_info_size, &mode_info);

            gop_modes[i].width = mode_info->HorizontalResolution;
            gop_modes[i].height = mode_info->VerticalResolution;
        }

        // Highlight top menu row to start off
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"Mode %d: %dx%d", 0, gop_modes[0].width, gop_modes[0].height);

        // Print other text mode infos
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINT32 i = 1; i < menu_len + 1; i++) 
            printf(u"\r\nMode %d: %dx%d", i, gop_modes[i].width, gop_modes[i].height);

        // Get input from user 
        cout->SetCursorPosition(cout, 0, menu_top);
        bool getting_input = true;
        while (getting_input) {
            UINTN current_row = cout->Mode->CursorRow;

            EFI_INPUT_KEY key = get_key();
            switch (key.ScanCode) {
                case SCANCODE_ESC: return EFI_SUCCESS;  // ESC Key: Go back to main menu

                case SCANCODE_UP_ARROW:
                    if (current_row == menu_top && mode_index > 0) {
                        // Scroll menu up by decrementing all modes by 1
                        printf(u"                    \r");  // Blank out mode text first

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        mode_index--;
                        printf(u"Mode %d: %dx%d", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        UINTN temp_mode = mode_index + 1;
                        for (UINT32 i = 0; i < menu_len; i++, temp_mode++) {
                            printf(u"\r\n                    \r"  // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
                                   temp_mode, gop_modes[temp_mode].width, gop_modes[temp_mode].height);
                        }

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                    } else if (current_row-1 >= menu_top) {
                        // De-highlight current row, move up 1 row, highlight new row
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        mode_index--;
                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                case SCANCODE_DOWN_ARROW:
                    // NOTE: Max valid GOP mode is ModeMax-1 per UEFI spec
                    if (current_row == menu_bottom && mode_index < max-1) {
                        // Scroll menu down by incrementing all modes by 1
                        mode_index -= menu_len - 1;

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                        // Print modes up until the last menu row
                        for (UINT32 i = 0; i < menu_len; i++, mode_index++) {
                            printf(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r\n", 
                                   mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                        }

                        // Highlight last row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                    } else if (current_row+1 <= menu_bottom) {
                        // De-highlight current row, move down 1 row, highlight new row
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r\n", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        mode_index++;
                        current_row++;
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                default:
                    if (key.UnicodeChar == u'\r') {
                        // Enter key, set GOP mode
                        gop->SetMode(gop, mode_index);
                        gop->QueryMode(gop, mode_index, &mode_info_size, &mode_info);

                        // Clear GOP screen - EFI_BLUE seems to have a hex value of 0x98
                        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px = { 0x98, 0x00, 0x00, 0x00 };  // BGR_8888
                        gop->Blt(gop, &px, EfiBltVideoFill, 
                                 0, 0,  // Origin BLT BUFFER X,Y
                                 0, 0,  // Destination screen X,Y
                                 mode_info->HorizontalResolution, mode_info->VerticalResolution,
                                 0);

                        getting_input = false;  // Will leave input loop and redraw screen
                        mode_index = 0;         // Reset last selected mode in menu
                    }
                    break;
            }
        }
    }

    return EFI_SUCCESS;
}

// ===================================================================
// Test mouse, touchscreen & various cursor/pointer support using 
//   Simple Pointer Protocol (SPP) & Absolute Pointer Protocol (APP)
// ===================================================================
EFI_STATUS test_mouse(void) {
    // Get SPP protocol via LocateHandleBuffer()
    EFI_GUID spp_guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
    EFI_SIMPLE_POINTER_PROTOCOL *spp[5];
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;
    EFI_STATUS status = 0;
    INTN cursor_size = 8;               // Size in pixels    
    INTN cursor_x = 0, cursor_y = 0;    // Mouse cursor position

    // Get APP protocol via LocateHandleBuffer()
    EFI_GUID app_guid = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;
    EFI_ABSOLUTE_POINTER_PROTOCOL *app[5];

    typedef enum {
        CIN = 0,    // ConIn (keyboard)
        SPP = 1,    // Simple Pointer Protocol (mouse/touchpad)
        APP = 2,    // Absolute Pointer Protocol (touchscreen/digitizer)
    } INPUT_TYPE;

    typedef struct {
        EFI_EVENT wait_event;   // This will be used in WaitForEvent()
        INPUT_TYPE type;
        union {
            EFI_SIMPLE_POINTER_PROTOCOL   *spp;
            EFI_ABSOLUTE_POINTER_PROTOCOL *app;
        };
    } INPUT_PROTOCOL;

    INPUT_PROTOCOL input_protocols[11]; // 11 = Max of 5 spp + 5 app + 1 conin
    UINTN num_protocols = 0;

    // First input will be ConIn
    input_protocols[num_protocols++] = (INPUT_PROTOCOL){ 
        .wait_event = cin->WaitForKey,
        .type = CIN,
        .spp = NULL,
    };

    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    UINTN mode_index = 0;   // Current mode within entire menu of GOP mode choices;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not locate GOP! :(\r\n", status);
        return status;
    }

    gop->QueryMode(gop, mode_index, &mode_info_size, &mode_info);

    // Use LocateHandleBuffer() to find all SPPs 
    status = bs->LocateHandleBuffer(ByProtocol, &spp_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not locate Simple Pointer Protocol handle buffer.\r\n", status);
    }

    cout->ClearScreen(cout);

    BOOLEAN found_mode = FALSE;

    // Open all SPP protocols for each handle
    for (UINTN i = 0; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &spp_guid,
                                  (VOID **)&spp[i],
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

        if (EFI_ERROR(status)) {
            error(u"\r\nERROR: %x; Could not Open Simple Pointer Protocol on handle.\r\n", status);
            continue;
        }

        // Reset device
        spp[i]->Reset(spp[i], TRUE);

        // Print initial SPP mode info
        printf(u"SPP %u; Resolution X: %u, Y: %u, Z: %u, LButton: %u, RButton: %u\r\n",
               i,
               spp[i]->Mode->ResolutionX, 
               spp[i]->Mode->ResolutionY,
               spp[i]->Mode->ResolutionZ,
               spp[i]->Mode->LeftButton, 
               spp[i]->Mode->RightButton);

        if (spp[i]->Mode->ResolutionX < 65536) {
            found_mode = TRUE;
            // Add valid protocol to array
            input_protocols[num_protocols++] = (INPUT_PROTOCOL){ 
                .wait_event = spp[i]->WaitForInput,
                .type = SPP,
                .spp = spp[i]
            };
        }
    }
    
    if (!found_mode) error(u"\r\nERROR: Could not find any valid SPP Mode.\r\n");

    // Free memory pool allocated by LocateHandleBuffer()
    bs->FreePool(handle_buffer);

    // Use LocateHandleBuffer() to find all APPs 
    num_handles = 0;
    handle_buffer = NULL;
    found_mode = FALSE;

    status = bs->LocateHandleBuffer(ByProtocol, &app_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) 
        error(u"\r\nERROR: %x; Could not locate Absolute Pointer Protocol handle buffer.\r\n", status);

    printf(u"\r\n");    // Separate SPP and APP info visually

    // Open all APP protocols for each handle
    for (UINTN i = 0; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &app_guid,
                                  (VOID **)&app[i],
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

        if (EFI_ERROR(status)) {
            error(u"\r\nERROR: %x; Could not Open Simple Pointer Protocol on handle.\r\n", status);
            continue;
        }

        // Reset device
        app[i]->Reset(app[i], TRUE);

        // Print initial APP mode info
        printf(u"APP %u; Min X: %u, Y: %u, Z: %u, Max X: %u, Y: %u, Z: %u, Attributes: %b\r\n",
               i,
               app[i]->Mode->AbsoluteMinX, 
               app[i]->Mode->AbsoluteMinY,
               app[i]->Mode->AbsoluteMinZ,
               app[i]->Mode->AbsoluteMaxX, 
               app[i]->Mode->AbsoluteMaxY,
               app[i]->Mode->AbsoluteMaxZ,
               app[i]->Mode->Attributes);

        if (app[i]->Mode->AbsoluteMaxX < 65536) {
            found_mode = TRUE;
            // Add valid protocol to array
            input_protocols[num_protocols++] = (INPUT_PROTOCOL){ 
                .wait_event = app[i]->WaitForInput,
                .type = APP,
                .app = app[i]
            };
        }
    }
    
    if (!found_mode) error(u"\r\nERROR: Could not find any valid APP Mode.\r\n");

    if (num_protocols == 0) {
        error(u"\r\nERROR: Could not find any Simple or Absolute Pointer Protocols.\r\n");
        return 1;
    }

    // Found valid SPP mode, get mouse input
    // Start off in middle of screen
    INT32 xres = mode_info->HorizontalResolution, yres = mode_info->VerticalResolution;
    cursor_x = (xres / 2) - (cursor_size / 2);
    cursor_y = (yres / 2) - (cursor_size / 2);

    // Print initial mouse state & draw initial cursor
    printf(u"\r\nMouse Xpos: %d, Ypos: %d, Xmm: %d, Ymm: %d, LB: %u, RB: %u\r",
           cursor_x, cursor_y, 0, 0, 0);

    // Draw mouse cursor, and also save underlying FB data first
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb = 
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gop->Mode->FrameBufferBase;

    for (INTN y = 0; y < cursor_size; y++) {
        for (INTN x = 0; x < cursor_size; x++) {
            save_buffer[(y * cursor_size) + x] = 
                fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)];

            EFI_GRAPHICS_OUTPUT_BLT_PIXEL csr_px = cursor_buffer[(y * cursor_size) + x];
            fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)] = csr_px;
        }
    }

    // Input loop
    // Fill out event queue first
    EFI_EVENT events[11]; // Same max # of elems as input_protocols
    for (UINTN i = 0; i < num_protocols; i++) events[i] = input_protocols[i].wait_event;

    while (TRUE) {
        UINTN index = 0;

        bs->WaitForEvent(num_protocols, events, &index);
        if (input_protocols[index].type == CIN) {
            // Keypress
            EFI_INPUT_KEY key;
            cin->ReadKeyStroke(cin, &key);

            if (key.ScanCode == SCANCODE_ESC) {
                // ESC Key, leave and go back to main menu
                break;
            }

        } else if (input_protocols[index].type == SPP) {
            // Simple Pointer Protocol; Mouse event
            // Get mouse state
            EFI_SIMPLE_POINTER_STATE state;
            EFI_SIMPLE_POINTER_PROTOCOL *active_spp = input_protocols[index].spp;
            active_spp->GetState(active_spp, &state);

            // Print current info
            // Movement is spp state's RelativeMovement / spp mode's Resolution
            //   movement amount is in mm; 1mm = 2% of horizontal or vertical resolution
            float xmm_float = (float)state.RelativeMovementX / (float)active_spp->Mode->ResolutionX;
            float ymm_float = (float)state.RelativeMovementY / (float)active_spp->Mode->ResolutionY;

            // If moved a tiny bit, show that on screen for a small minimum amount
            if (state.RelativeMovementX > 0 && xmm_float == 0.0) xmm_float = 1.0;
            if (state.RelativeMovementY > 0 && ymm_float == 0.0) ymm_float = 1.0;

            // Erase text first before reprinting
            printf(u"                                                                      \r");
            printf(u"Mouse Xpos: %d, Ypos: %d, Xmm: %d, Ymm: %d, LB: %u, RB: %u\r",
                  cursor_x, cursor_y, (INTN)xmm_float, (INTN)ymm_float, 
                  state.LeftButton, state.RightButton);

            // Draw cursor: Get pixel amount to move per mm
            float xres_mm_px = mode_info->HorizontalResolution * 0.02;
            float yres_mm_px = mode_info->VerticalResolution   * 0.02;

            // Save framebuffer data at mouse position first, then redraw that data
            //   instead of just overwriting with background color e.g. with a blt buffer and
            //   EfiVideoToBltBuffer and EfiBltBufferToVideo
            for (INTN y = 0; y < cursor_size; y++) {
                for (INTN x = 0; x < cursor_size; x++) {
                    fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)] = 
                        save_buffer[(y * cursor_size) + x];
                }
            }

            cursor_x += (INTN)(xres_mm_px * xmm_float);
            cursor_y += (INTN)(yres_mm_px * ymm_float);

            // Keep cursor in screen bounds
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x > xres - cursor_size) cursor_x = xres - cursor_size;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_y > yres - cursor_size) cursor_y = yres - cursor_size;

            // Save FB data at new cursor position before drawing over it
            for (INTN y = 0; y < cursor_size; y++) {
                for (INTN x = 0; x < cursor_size; x++) {
                    save_buffer[(y * cursor_size) + x] = 
                        fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)];

                    EFI_GRAPHICS_OUTPUT_BLT_PIXEL csr_px = cursor_buffer[(y * cursor_size) + x];
                    fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)] = csr_px;
                }
            }

        } else if (input_protocols[index].type == APP) {
            // Handle absolute pointer protocol
            // Get state
            EFI_ABSOLUTE_POINTER_STATE state;
            EFI_ABSOLUTE_POINTER_PROTOCOL *active_app = input_protocols[index].app;
            active_app->GetState(active_app, &state);

            // Print state values
            // Erase text first before reprinting
            printf(u"                                                                      \r");
            printf(u"Ptr Xpos: %u, Ypos: %u, Zpos: %u, Buttons: %b\r",
                  state.CurrentX, state.CurrentY, state.CurrentZ,
                  state.ActiveButtons);

            // Save framebuffer data at mouse position first, then redraw that data
            //   instead of just overwriting with background color e.g. with a blt buffer and
            //   EfiVideoToBltBuffer and EfiBltBufferToVideo
            for (INTN y = 0; y < cursor_size; y++) {
                for (INTN x = 0; x < cursor_size; x++) {
                    fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)] = 
                        save_buffer[(y * cursor_size) + x];
                }
            }

            // Get ratio of GOP screen resolution to APP max values, to translate the APP
            //   position to the correct on screen GOP position
            float x_app_ratio = (float)mode_info->HorizontalResolution / 
                                (float)active_app->Mode->AbsoluteMaxX;

            float y_app_ratio = (float)mode_info->VerticalResolution / 
                                (float)active_app->Mode->AbsoluteMaxY;

            cursor_x = (INTN)((float)state.CurrentX * x_app_ratio);
            cursor_y = (INTN)((float)state.CurrentY * y_app_ratio);

            // Keep cursor in screen bounds
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x > xres - cursor_size) cursor_x = xres - cursor_size;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_y > yres - cursor_size) cursor_y = yres - cursor_size;

            // Save FB data at new cursor position before drawing over it
            for (INTN y = 0; y < cursor_size; y++) {
                for (INTN x = 0; x < cursor_size; x++) {
                    save_buffer[(y * cursor_size) + x] = 
                        fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)];

                    EFI_GRAPHICS_OUTPUT_BLT_PIXEL csr_px = cursor_buffer[(y * cursor_size) + x];
                    fb[(mode_info->PixelsPerScanLine * (cursor_y + y)) + (cursor_x + x)] = csr_px;
                }
            }
        }
    }

    return EFI_SUCCESS;
}

// ===========================================================
// Timer function to print current date/time every 1 second
// ===========================================================
VOID EFIAPI print_datetime(IN EFI_EVENT event, IN VOID *Context) {
    (VOID)event; // Suppress compiler warning

    // Timer context will be the text mode screen bounds
    typedef struct {
        UINT32 rows; 
        UINT32 cols;
    } Timer_Context;

    Timer_Context context = *(Timer_Context *)Context;

    // Save current cursor position before printing date/time
    UINT32 save_col = cout->Mode->CursorColumn, save_row = cout->Mode->CursorRow;

    // Get current date/time
    EFI_TIME time;
    EFI_TIME_CAPABILITIES capabilities;
    rs->GetTime(&time, &capabilities);

    // Move cursor to print in lower right corner
    cout->SetCursorPosition(cout, context.cols-20, context.rows-1);

    // Print current date/time
    printf(u"%u-%c%u-%c%u %c%u:%c%u:%c%u",
            time.Year, 
            time.Month  < 10 ? u'0' : u'\0', time.Month,
            time.Day    < 10 ? u'0' : u'\0', time.Day,
            time.Hour   < 10 ? u'0' : u'\0', time.Hour,
            time.Minute < 10 ? u'0' : u'\0', time.Minute,
            time.Second < 10 ? u'0' : u'\0', time.Second);

    // Restore cursor position
    cout->SetCursorPosition(cout, save_col, save_row);
}

// ================================================
// Read & print files in the EFI System Partition
// ================================================
EFI_STATUS read_esp_files(void) {
    // Get the Loaded Image protocol for this EFI image/application itself,
    //   in order to get the device handle to use for the Simple File System Protocol
    EFI_STATUS status = EFI_SUCCESS;
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    status = bs->OpenProtocol(image,
                              &lip_guid,
                              (VOID **)&lip,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not open Loaded Image Protocol\r\n", status);
        return status;
    }

    // Get Simple File System Protocol for device handle for this loaded
    //   image, to open the root directory for the ESP
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = NULL;
    status = bs->OpenProtocol(lip->DeviceHandle,
                              &sfsp_guid,
                              (VOID **)&sfsp,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not open Simple File System Protocol\r\n", status);
        return status;
    }

    // Open root directory via OpenVolume()
    EFI_FILE_PROTOCOL *dirp = NULL;
    status = sfsp->OpenVolume(sfsp, &dirp);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not Open Volume for root directory\r\n", status);
        goto cleanup;
    }

    // Start at root directory
    CHAR16 current_directory[256];
    strcpy_u16(current_directory, u"/");    

    // Print dir entries for currently opened directory
    // Overall input loop
    INT32 csr_row = 1;
    while (true) {
        cout->ClearScreen(cout);
        printf(u"%s:\r\n", current_directory);

        INT32 num_entries = 0;
        EFI_FILE_INFO file_info;

        dirp->SetPosition(dirp, 0);                 // Reset to start of directory entries
        UINTN buf_size = sizeof file_info;
        dirp->Read(dirp, &buf_size, &file_info);
        while (buf_size > 0) {
            num_entries++;

            // Got next dir entry, print info
            if (csr_row == cout->Mode->CursorRow) {
                // Highlight row cursor/user is on
                cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
            }

            printf(u"%s %s\r\n", 
                   (file_info.Attribute & EFI_FILE_DIRECTORY) ? u"[DIR] " : u"[FILE]",
                   file_info.FileName);

            if (csr_row+1 == cout->Mode->CursorRow) {
                // De-highlight rows after cursor
                cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
            }

            buf_size = sizeof file_info;
            dirp->Read(dirp, &buf_size, &file_info);
        }

        EFI_INPUT_KEY key = get_key();
        switch (key.ScanCode) {
            case SCANCODE_ESC:
                // ESC Key, exit and go back to main menu
                dirp->Close(dirp);  // Close last open directory 
                goto cleanup;
                break;

            case SCANCODE_UP_ARROW:
                if (csr_row > 1) csr_row--;
                break;

            case SCANCODE_DOWN_ARROW:
                if (csr_row < num_entries) csr_row++;
                break;

            default:
                if (key.UnicodeChar == u'\r') {
                    // Enter key; 
                    //   for a directory, enter that directory and iterate the loop
                    //   for a file, print the file contents to screen

                    // Get directory entry under user cursor row
                    dirp->SetPosition(dirp, 0);  // Reset to start of directory entries
                    INT32 i = 0;  
                    do {
                        buf_size = sizeof file_info;
                        dirp->Read(dirp, &buf_size, &file_info);
                        i++;
                    } while (i < csr_row);

                    if (file_info.Attribute & EFI_FILE_DIRECTORY) {
                        // Directory, open and enter this new directory
                        EFI_FILE_PROTOCOL *new_dir;
                        status = dirp->Open(dirp, 
                                            &new_dir, 
                                            file_info.FileName, 
                                            EFI_FILE_MODE_READ,
                                            0);

                        if (EFI_ERROR(status)) {
                            error(u"Error %x; Could not open new directory %s\r\n", 
                                  status, file_info.FileName);
                            goto cleanup;
                        }

                        dirp->Close(dirp);  // Close last opened dir
                        dirp = new_dir;     // Set new opened dir
                        csr_row = 1;        // Reset user row to first entry in new directory

                        // Set new path for current directory
                        if (!strncmp_u16(file_info.FileName, u".", 2)) {
                            // Current directory, do nothing

                        } else if (!strncmp_u16(file_info.FileName, u"..", 3)) {
                            // Parent directory, go back up and remove dir name from path
                            CHAR16 *pos = strrchr_u16(current_directory, u'/');
                            if (pos == current_directory) pos++;    // Move past initial root dir '/'

                            *pos = u'\0';

                        } else {
                            // Go into nested directory, add on to current string
                            if (current_directory[1] != u'\0') {
                                strcat_u16(current_directory, u"/"); 
                            }
                            strcat_u16(current_directory, file_info.FileName);
                        }
                        continue;   // Continue overall loop and print new directory entries
                    } 

                    // Else this is a file, print contents:
                    // Allocate buffer for file
                    VOID *buffer = NULL;
                    buf_size = file_info.FileSize;
                    status = bs->AllocatePool(EfiLoaderData, buf_size, &buffer);
                    if (EFI_ERROR(status)) {
                        error(u"Error %x; Could not allocate memory for file %s\r\n", 
                              status, file_info.FileName);
                        goto cleanup;
                    }

                    // Open file
                    EFI_FILE_PROTOCOL *file = NULL;
                    status = dirp->Open(dirp, 
                                        &file, 
                                        file_info.FileName, 
                                        EFI_FILE_MODE_READ,
                                        0);

                    if (EFI_ERROR(status)) {
                        error(u"Error %x; Could not open file %s\r\n", 
                              status, file_info.FileName);
                        goto cleanup;
                    }

                    // Read file into buffer
                    status = dirp->Read(file, &buf_size, buffer);
                    if (EFI_ERROR(status)) {
                        error(u"Error %x; Could not read file %s into buffer.\r\n", 
                              status, file_info.FileName);
                        goto cleanup;
                    } 

                    if (buf_size != file_info.FileSize) {
                        error(u"Error: Could not read all of file %s into buffer.\r\n" 
                              u"Bytes read: %u, Expected: %u\r\n",
                              file_info.FileName, buf_size, file_info.FileSize);
                        goto cleanup;
                    }

                    // Print buffer contents
                    printf(u"\r\nFile Contents:\r\n");

                    char *pos = (char *)buffer;
                    for (UINTN bytes = buf_size; bytes > 0; bytes--) {
                        CHAR16 str[2];
                        str[0] = *pos;
                        str[1] = u'\0';
                        if (*pos == '\n') {
                            // Convert LF newline to CRLF
                            printf(u"\r\n");
                        } else {
                            printf(u"%s", str);
                        }

                        pos++;
                    }

                    printf(u"\r\n\r\nPress any key to continue...\r\n");
                    get_key();

                    // Free memory for file when done
                    bs->FreePool(buffer);

                    // Close file handle
                    dirp->Close(file);
                }
                break;
        }
    }

    cleanup:
    // Close open protocols
    bs->CloseProtocol(lip->DeviceHandle,
                      &sfsp_guid,
                      image,
                      NULL);

    bs->CloseProtocol(image,
                      &lip_guid,
                      image,
                      NULL);

    return status;
}
// ======================================================================
// Print Block IO Partitions using Block IO and Parition Info Protocols
// ======================================================================
EFI_STATUS print_block_io_partitions(void) {
    EFI_STATUS status = EFI_SUCCESS;

    cout->ClearScreen(cout);

    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;

    // Get media ID for this disk image first, to compare to others in output
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    status = bs->OpenProtocol(image,
                              &lip_guid,
                              (VOID **)&lip,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not open Loaded Image Protocol\r\n", status);
        return status;
    }

    status = bs->OpenProtocol(lip->DeviceHandle,
                              &bio_guid,
                              (VOID **)&biop,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not open Block IO Protocol for this loaded image.\r\n", status);
        return status;
    }

    UINT32 this_image_media_id = biop->Media->MediaId;  // Media ID for this running disk image itself

    // Close open protocols when done
    bs->CloseProtocol(lip->DeviceHandle,
                      &bio_guid,
                      image,
                      NULL);
    bs->CloseProtocol(image,
                      &lip_guid,
                      image,
                      NULL);

    // Loop through and print all partition information found
    status = bs->LocateHandleBuffer(ByProtocol, &bio_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not locate any Block IO Protocols.\r\n", status);
        return status;
    }

    UINT32 last_media_id = -1;  // Keep track of currently opened Media info
    for (UINTN i = 0; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &bio_guid,
                                  (VOID **)&biop,
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

        if (EFI_ERROR(status)) {
            error(u"\r\nERROR: %x; Could not Open Block IO protocol on handle %u.\r\n", status, i);
            continue;
        }

        // Print Block IO Media Info for this Disk/partition
        if (last_media_id != biop->Media->MediaId) {
            last_media_id = biop->Media->MediaId;   
            printf(u"Media ID: %u %s\r\n", 
                   last_media_id, 
                   (last_media_id == this_image_media_id ? u"(Disk Image)" : u""));
        }

        if (biop->Media->LastBlock == 0) {
            // Only really care about partitions/disks above 1 block in size
            bs->CloseProtocol(handle_buffer[i],
                              &bio_guid,
                              image,
                              NULL);
            continue;
        }

        printf(u"Rmv: %s, Pr: %s, LglPrt: %s, RdOnly: %s, Wrt$: %s\r\n"
               u"BlkSz: %u, IoAln: %u, LstBlk: %u, LwLBA: %u, LglBlkPerPhys: %u\r\n"
               u"OptTrnLenGran: %u\r\n",
               biop->Media->RemovableMedia   ? u"Y" : u"N",
               biop->Media->MediaPresent     ? u"Y" : u"N",
               biop->Media->LogicalPartition ? u"Y" : u"N",
               biop->Media->ReadOnly         ? u"Y" : u"N",
               biop->Media->WriteCaching     ? u"Y" : u"N",

               biop->Media->BlockSize,
               biop->Media->IoAlign,
               biop->Media->LastBlock,
               biop->Media->LowestAlignedLba,                   
               biop->Media->LogicalBlocksPerPhysicalBlock,     
               biop->Media->OptimalTransferLengthGranularity);

        // Print type of partition e.g. ESP or Data or Other
        if (!biop->Media->LogicalPartition) printf(u"<Entire Disk>\r\n");
        else {
            // Get partition info protocol for this partition
            EFI_GUID pi_guid = EFI_PARTITION_INFO_PROTOCOL_GUID;
            EFI_PARTITION_INFO_PROTOCOL *pip = NULL;
            status = bs->OpenProtocol(handle_buffer[i], 
                                      &pi_guid,
                                      (VOID **)&pip,
                                      image,
                                      NULL,
                                      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

            if (EFI_ERROR(status)) {
                error(u"\r\nERROR: %x; Could not Open Partition Info protocol on handle %u.\r\n", 
                      status, i);
            } else {
                if      (pip->Type == PARTITION_TYPE_OTHER) printf(u"<Other Type>\r\n");
                else if (pip->Type == PARTITION_TYPE_MBR)   printf(u"<MBR>\r\n");
                else if (pip->Type == PARTITION_TYPE_GPT) {
                    if (pip->System == 1) printf(u"<EFI System Partition>\r\n");
                    else {
                        // Compare Gpt.PartitionTypeGUID with known values
                        EFI_GUID data_guid = BASIC_DATA_GUID;
                        if (!memcmp(&pip->Info.Gpt.PartitionTypeGUID, &data_guid, sizeof(EFI_GUID))) 
                            printf(u"<Basic Data>\r\n");
                        else
                            printf(u"<Other GPT Type>\r\n");
                    }
                }
            }
        }

        // Close open protocol when done
        bs->CloseProtocol(handle_buffer[i],
                          &bio_guid,
                          image,
                          NULL);

        printf(u"\r\n");    // Separate each block of text visually 
    }

    printf(u"Press any key to go back..\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ====================
// Entry Point
// ====================
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize global variables
    init_global_variables(ImageHandle, SystemTable);

    // Reset Console Inputs/Outputs
    cin->Reset(cin, FALSE);
    cout->Reset(cout, FALSE);
    cout->Reset(cerr, FALSE);

    // Set text colors - foreground, background
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    // Disable Watchdog Timer
    bs->SetWatchdogTimer(0, 0x10000, 0, NULL);

    // Menu text on screen
    const CHAR16 *menu_choices[] = {
        u"Set Text Mode",
        u"Set Graphics Mode",
        u"Test Mouse",
        u"Read ESP Files",
        u"Print Block IO Partitions",
    };

    // Functions to call for each menu option
    EFI_STATUS (*menu_funcs[])(void) = {
        set_text_mode,
        set_graphics_mode,
        test_mouse,
        read_esp_files,
        print_block_io_partitions,
    };

    // Screen loop
    bool running = true;
    while (running) {
        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(cout);

        // Get current text mode ColsxRows values
        UINTN cols = 0, rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

        // Timer context will be the text mode screen bounds
        typedef struct {
            UINT32 rows; 
            UINT32 cols;
        } Timer_Context;

        Timer_Context context;
        context.rows = rows;
        context.cols = cols;

        EFI_EVENT timer_event;

        // Create timer event, to print date/time on screen every ~1second
        bs->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL,
                        TPL_CALLBACK, 
                        print_datetime,
                        (VOID *)&context,
                        &timer_event);

        // Set Timer for the timer event to run every 1 second (in 100ns units)
        bs->SetTimer(timer_event, TimerPeriodic, 10000000);

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, rows-3);
        printf(u"Up/Down Arrow = Move cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Shutdown");

        // Print menu choices
        // Highlight first choice as initial choice
        cout->SetCursorPosition(cout, 0, 0);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"%s", menu_choices[0]);

        // Print rest of choices
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINTN i = 1; i < ARRAY_SIZE(menu_choices); i++)
            printf(u"\r\n%s", menu_choices[i]);

        // Get cursor row boundaries
        INTN min_row = 0, max_row = cout->Mode->CursorRow;

        // Input loop
        cout->SetCursorPosition(cout, 0, 0);
        bool getting_input = true;
        while (getting_input) {
            INTN current_row = cout->Mode->CursorRow;
            EFI_INPUT_KEY key = get_key();

            // Process input
            switch (key.ScanCode) {
                case SCANCODE_UP_ARROW:
                    if (current_row-1 >= min_row) {
                        // De-highlight current row, move up 1 row, highlight new row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        // Reset colors
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    }
                    break;

                case SCANCODE_DOWN_ARROW:
                    if (current_row+1 <= max_row) {
                        // De-highlight current row, move down 1 row, highlight new row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        current_row++;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        // Reset colors
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    }
                    break;

                case SCANCODE_ESC:
                    // Close Timer Event for cleanup
                    bs->CloseEvent(timer_event);

                    // Escape key: power off
                    rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

                    // !NOTE!: This should not return, system should power off
                    break;

                default:
                    if (key.UnicodeChar == u'\r') {
                        // Enter key, select choice
                        EFI_STATUS return_status = menu_funcs[current_row]();
                        if (EFI_ERROR(return_status)) {
                            error(u"ERROR %x\r\n; Press any key to go back...", return_status);
                        }

                        // Will leave input loop and reprint main menu
                        getting_input = false; 
                    }
                    break;
            }
        }
    }

    // End program
    return EFI_SUCCESS;
}
