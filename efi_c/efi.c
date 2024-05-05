#include <stdarg.h>
#include <stdalign.h>
#include "efi.h"
#include "efi_lib.h"

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

#define PAGE_PHYS_ADDR_MASK 0x000FFFFFFFFFF000  // Page aligned 52 bit address
#define PAGE_SIZE 4096                          // 4KiB

#ifdef __clang__
int _fltused = 0;   // If using floating point code & lld-link, need to define this
#endif

// -----------------
// Global variables
// -----------------
EFI_SYSTEM_TABLE *st = NULL;                    // System Table 
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;   // Console output
EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin  = NULL;   // Console input
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr = NULL;   // Console output - stderr
EFI_BOOT_SERVICES    *bs;                       // Boot services
EFI_RUNTIME_SERVICES *rs;                       // Runtime services
EFI_HANDLE image = NULL;                        // Image handle

EFI_EVENT timer_event;  // Global timer event

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

// Paging tables & variables
#define KERNEL_ADDRESS 0xFFFFFFFF80000000

typedef struct {
    uint64_t entries[512];
} __attribute__((packed)) Paging_Table;

Paging_Table *pml4;  // Level 4 page table 
UINTN current_descriptor = 0;
UINTN available_pages = 0;
UINTN next_page_address = 0;

EFI_MEMORY_DESCRIPTOR *runtime_map = NULL;

// ====================
// Set global vars
// ====================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    st = systable;
    cout = systable->ConOut;
    cin = systable->ConIn;
    //cerr = systable->StdErr; // Stderr can be set to a serial output or other non-display device.
    cerr = cout;  // Use stdout for error printing 
    bs = systable->BootServices;
    rs = systable->RuntimeServices;
    image = handle;
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

// ================================
// Print a GUID value
// ================================
void print_guid(EFI_GUID guid) {
    UINT8 *p = (UINT8 *)&guid;
    printf(u"{%x,%x,%x,%x,%x,{%x,%x,%x,%x,%x,%x}\r\n",
            *(UINT32 *)&p[0], *(UINT16 *)&p[4], *(UINT16 *)&p[6],
            p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
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

// =================================================================
// Read a fully qualified file path in the EFI System Partition into 
//   an output buffer. File path must start with root '\',
//   escaped as needed by the caller with '\\'.
//
// Returns: non-null pointer to allocated buffer with file data, 
//  allocated with Boot Services AllocatePool(), or NULL if not 
//  found or error.
//
//  NOTE: Caller will have to use FreePool() on returned buffer to 
//    free allocated memory.
// =================================================================
VOID *read_esp_file_to_buffer(CHAR16 *path, UINTN *file_size) {
    VOID *file_buffer = NULL;
    EFI_STATUS status;

    // Get loaded image protocol first to grab device handle to use 
    //   simple file system protocol on
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
        goto cleanup;
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
        goto cleanup;
    }

    // Open root directory via OpenVolume()
    EFI_FILE_PROTOCOL *root = NULL;
    status = sfsp->OpenVolume(sfsp, &root);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not Open Volume for root directory in ESP\r\n", status);
        goto cleanup;
    }

    // Open file in input path (qualified from root directory)
    EFI_FILE_PROTOCOL *file = NULL;
    status = root->Open(root, 
                        &file, 
                        path,
                        EFI_FILE_MODE_READ,
                        0);

    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not open file '%s'\r\n", status, path);
        goto cleanup;
    }

    // Get info for file, to grab file size
    EFI_FILE_INFO file_info;
    EFI_GUID fi_guid = EFI_FILE_INFO_ID;
    UINTN buf_size = sizeof(EFI_FILE_INFO);
    status = file->GetInfo(file, &fi_guid, &buf_size, &file_info);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not get file info for file '%s'\r\n", status, path);
        goto file_cleanup;
    }

    // Allocate buffer for file
    buf_size = file_info.FileSize;
    status = bs->AllocatePool(EfiLoaderData, buf_size, &file_buffer);
    if (EFI_ERROR(status) || buf_size != file_info.FileSize) {
        error(u"Error %x; Could not allocate memory for file '%s'\r\n", status, path);
        goto file_cleanup;
    }

    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not get file info for file '%s'\r\n", status, path);
        goto file_cleanup;
    }

    // Read file into buffer
    status = file->Read(file, &buf_size, file_buffer);
    if (EFI_ERROR(status) || buf_size != file_info.FileSize) {
        error(u"Error %x; Could not read file '%s' into buffer\r\n", status, path);
        goto file_cleanup;
    }

    // Set output file size in buffer
    *file_size = buf_size;

    // Close open file/dir pointers
    file_cleanup:
    root->Close(root);
    file->Close(file);

    // Final cleanup before returning
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

    // Will return buffer with file data or NULL on errors
    return file_buffer; 
}

// =================================================================
// Read a file from a given disk (from input media ID), into an
//   output buffer. 

// Returns: non-null pointer to allocated buffer with data, 
//  allocated with Boot Services AllocatePool(), or NULL if not 
//  found or error.
//
//  NOTE: Caller will have to use FreePool() on returned buffer to 
//    free allocated memory.
// =================================================================
VOID *read_disk_lbas_to_buffer(EFI_LBA disk_lba, UINTN data_size, UINT32 disk_mediaID) {
    VOID *buffer = NULL;
    EFI_STATUS status = EFI_SUCCESS;

    // Loop through and get Block IO protocol for input media ID, for entire disk
    //   NOTE: This assumes the first Block IO found with logical partition false is the entire disk
    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;

    status = bs->LocateHandleBuffer(ByProtocol, &bio_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not locate any Block IO Protocols.\r\n", status);
        return buffer;
    }

    BOOLEAN found = false;
    UINTN i = 0;
    for (; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &bio_guid,
                                  (VOID **)&biop,
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

        if (EFI_ERROR(status)) {
            error(u"\r\nERROR: %x; Could not Open Block IO protocol on handle %u.\r\n", status, i);
            // Close open protocol 
            bs->CloseProtocol(handle_buffer[i],
                              &bio_guid,
                              image,
                              NULL);
            continue;
        }

        if (biop->Media->MediaId == disk_mediaID && !biop->Media->LogicalPartition) {
            found = true;
            break;
        }

        // Close open protocol when done
        bs->CloseProtocol(handle_buffer[i],
                          &bio_guid,
                          image,
                          NULL);
    }

    if (!found) {
        error(u"\r\nERROR: Could not find Block IO protocol for disk with ID %u.\r\n", 
              disk_mediaID);
        return buffer;
    }

    // Get Disk IO Protocol on same handle as Block IO protocol
    EFI_GUID dio_guid = EFI_DISK_IO_PROTOCOL_GUID;
    EFI_DISK_IO_PROTOCOL *diop;
    status = bs->OpenProtocol(handle_buffer[i], 
                              &dio_guid,
                              (VOID **)&diop,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not Open Disk IO protocol on handle.\r\n", status, i);
        goto cleanup;
    }

    // Allocate buffer for data
    status = bs->AllocatePool(EfiLoaderData, data_size, &buffer);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not Allocate buffer for disk data.\r\n", status);
        bs->CloseProtocol(handle_buffer[i],
                          &dio_guid,
                          image,
                          NULL);
        goto cleanup;
    }

    // Use Disk IO Read to read into allocated buffer
    status = diop->ReadDisk(diop, disk_mediaID, disk_lba * biop->Media->BlockSize, data_size, buffer);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not read Disk LBAs into buffer.\r\n", status);
    }

    // Close disk IO protocol when done
    bs->CloseProtocol(handle_buffer[i],
                      &dio_guid,
                      image,
                      NULL);

    cleanup:
    // Close open protocol when done
    bs->CloseProtocol(handle_buffer[i],
                      &bio_guid,
                      image,
                      NULL);

    return buffer;
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

// ================================================
// Get Media ID value for this running disk image
// ================================================
EFI_STATUS get_disk_image_mediaID(UINT32 *mediaID) {
    EFI_STATUS status = EFI_SUCCESS;

    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;

    // Get media ID for this disk image 
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

    // Get Block IO protocol for loaded image's device handle
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

    *mediaID = biop->Media->MediaId;  // Media ID for this running disk image itself

    // Close open protocols when done
    bs->CloseProtocol(lip->DeviceHandle,
                      &bio_guid,
                      image,
                      NULL);

    bs->CloseProtocol(image,
                      &lip_guid,
                      image,
                      NULL);

    return EFI_SUCCESS;
}

// ==========================================================
// Load an ELF64 PIE file into a new buffer, and return the 
//   entry point for the loaded ELF program
// ==========================================================
VOID *load_elf(VOID *elf_buffer, EFI_PHYSICAL_ADDRESS *pgm_buffer, UINTN *pgm_size) {
    ELF_Header_64 *ehdr = elf_buffer;

    // Print elf header info for user
    printf(u"Type: %u, Machine: %x, Entry: %x\r\n"
           u"Pgm headers offset: %u, Elf Header Size: %u\r\n"
           u"Pgm entry size: %u, # of Pgm headers: %u\r\n",
           ehdr->e_type, ehdr->e_machine, ehdr->e_entry,
           ehdr->e_phoff, ehdr->e_ehsize, 
           ehdr->e_phentsize, ehdr->e_phnum);

    // Only allow PIE ELF files
    if (ehdr->e_type != ET_DYN) {
        error(u"ELF is not a PIE file; e_type is not ETDYN/0x03\r\n");
        return NULL;
    }

    // Print Loadable program header info for user, and get measurements for loading
    ELF_Program_Header_64 *phdr = (ELF_Program_Header_64 *)((UINT8 *)ehdr + ehdr->e_phoff);

    UINTN max_alignment = PAGE_SIZE;    
    UINTN mem_min = UINT64_MAX, mem_max = 0;

    printf(u"\r\nLoadable Program Headers:\r\n");
    for (UINT16 i = 0; i < ehdr->e_phnum; i++, phdr++) {
        // Only interested in loadable program headers
        if (phdr->p_type != PT_LOAD) continue;

        printf(u"%u: Offset: %x, Vaddr: %x, Paddr: %x\r\n"
               u"FileSize: %x, MemSize: %x, Alignment: %x\r\n",
               (UINTN)i, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
               phdr->p_filesz, phdr->p_memsz, phdr->p_align);

        // Update max alignment as needed
        if (max_alignment < phdr->p_align) max_alignment = phdr->p_align;

        UINTN hdr_begin = phdr->p_vaddr;        
        UINTN hdr_end   = phdr->p_vaddr + phdr->p_memsz + max_alignment-1;

        // Limit memory range to aligned values
        //   e.g. 4096-1 = 4095 or 0x00000FFF (32 bit); ~4095 = 0xFFFFF000
        hdr_begin &= ~(max_alignment-1);    
        hdr_end   &= ~(max_alignment-1);   

        // Get new minimum & maximum memory bounds for all program sections
        if (hdr_begin < mem_min) mem_min = hdr_begin;
        if (hdr_end   > mem_max) mem_max = hdr_end;
    }

    UINTN max_memory_needed = mem_max - mem_min;   
    printf(u"\r\nMemory needed for file: %x\r\n", max_memory_needed);

    // Allocate buffer for program headers
    EFI_STATUS status = 0;
    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    status = bs->AllocatePages(AllocateAnyPages, EfiLoaderData, max_memory_needed / PAGE_SIZE, &program_buffer);

    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not allocate memory for ELF program\r\n", status);
        return NULL;
    }

    // Zero init buffer, to ensure 0 padding for all program sections
    memset((VOID *)program_buffer, 0, max_memory_needed);
    *pgm_size = max_memory_needed;

    // Load program headers into buffer
    phdr = (ELF_Program_Header_64 *)((UINT8 *)ehdr + ehdr->e_phoff);
    for (UINT16 i = 0; i < ehdr->e_phnum; i++, phdr++) {
        // Only interested in loadable program headers
        if (phdr->p_type != PT_LOAD) continue;

        // Use relative position of program section in file, to ensure it still works correctly.
        //   With PIE executables, this means we can use any entry point or addresses, as long as
        //   we use the same relative addresses.
        UINTN relative_offset = phdr->p_vaddr - mem_min;

        // Read p_filesz amount of data from p_offset in original file buffer,
        //   to the same relative offset of p_vaddr in new buffer
        UINT8 *dst = (UINT8 *)(program_buffer + relative_offset); 
        UINT8 *src = (UINT8 *)elf_buffer + phdr->p_offset;
        UINT32 len = phdr->p_filesz;
        memcpy(dst, src, len);
    }

    // Return entry point in new buffer, with same relative offset as in the original buffer 
    VOID *entry_point = (VOID *)(program_buffer + (ehdr->e_entry - mem_min));
    
    *pgm_buffer = program_buffer;
    return entry_point;
}

// ==========================================================
// Load an PE32+ PIE file into a new buffer, and return the 
//   entry point for the loaded PE program
// ==========================================================
VOID *load_pe(VOID *pe_buffer, EFI_PHYSICAL_ADDRESS *pgm_buffer, UINTN *pgm_size) {
    // Print PE Signature
    UINT8 pe_sig_offset = 0x3C; // From PE file format
    UINT32 pe_sig_pos = *(UINT32 *)((UINT8 *)pe_buffer + pe_sig_offset);
    UINT8 *pe_sig = (UINT8 *)pe_buffer + pe_sig_pos;

    printf(u"\r\nPE Signature: [%x][%x][%x][%x]\r\n",
           (UINTN)pe_sig[0], (UINTN)pe_sig[1], (UINTN)pe_sig[2], (UINTN)pe_sig[3]);

    // Print Coff File Header Info
    PE_Coff_File_Header_64 *coff_hdr = (PE_Coff_File_Header_64 *)(pe_sig + 4);
    printf(u"Coff File Header:\r\n");
    printf(u"Machine: %x, # of sections: %u, Size of Opt Hdr: %x\r\n"
           u"Characteristics: %x\r\n",
           coff_hdr->Machine, coff_hdr->NumberOfSections, coff_hdr->SizeOfOptionalHeader,
           coff_hdr->Characteristics);

    // Validate some data
    if (coff_hdr->Machine != 0x8664) {
        error(u"Error: Machine type not AMD64.\r\n");
        return NULL;
    }

    if (!(coff_hdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        error(u"Error: File not an executable image.\r\n");
        return NULL;
    }

    // Print Optional header info
    PE_Optional_Header_64 *opt_hdr = 
        (PE_Optional_Header_64 *)((UINT8 *)coff_hdr + sizeof(PE_Coff_File_Header_64));

    printf(u"\r\nOptional Header:\r\n");
    printf(u"Magic: %x, Entry Point: %x\r\n" 
           u"Sect Align: %x, File Align: %x, Size of Image: %x\r\n"
           u"Subsystem: %x, DLL Characteristics: %x\r\n",
           opt_hdr->Magic, opt_hdr->AddressOfEntryPoint,
           opt_hdr->SectionAlignment, opt_hdr->FileAlignment, opt_hdr->SizeOfImage,
           (UINTN)opt_hdr->Subsystem, (UINTN)opt_hdr->DllCharacteristics);

    // Validate info
    if (opt_hdr->Magic != 0x20B) {
        error(u"Error: File not a PE32+ file.\r\n");
        return NULL;
    }

    if (!(opt_hdr->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) {
        error(u"Error: File not a PIE file.\r\n");
        return NULL;
    }

    // Allocate buffer to load sections into
    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    EFI_STATUS status = bs->AllocatePages(AllocateAnyPages, EfiLoaderData, opt_hdr->SizeOfImage / PAGE_SIZE, &program_buffer);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not allocate memory for PE file.\r\n", status);
        return NULL;
    }

    // Initialize buffer to 0, which should also take care of needing to 0-pad sections between
    //   Raw Data and Virtual Size
    memset((VOID *)program_buffer, 0, opt_hdr->SizeOfImage);
    *pgm_size = opt_hdr->SizeOfImage;

    // Print section header info
    PE_Section_Header_64 *shdr = 
        (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);

    printf(u"\r\nSection Headers:\r\n");
    for (UINT16 i = 0; i < coff_hdr->NumberOfSections; i++, shdr++) {
        printf(u"Name: ");
        char *pos = (char *)&shdr->Name;
        for (UINT8 j = 0; j < 8; j++) {
            CHAR16 str[2];
            str[0] = *pos;
            str[1] = u'\0';
            if (*pos == '\0') break;
            printf(u"%s", str);
            pos++;
        }

        printf(u" VSize: %x, Vaddr: %x, DataSize: %x, DataPtr: %x\r\n",
               shdr->VirtualSize, shdr->VirtualAddress, 
               shdr->SizeOfRawData, shdr->PointerToRawData);
    }

    // Load sections into new buffer
    shdr = (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);
    for (UINT16 i = 0; i < coff_hdr->NumberOfSections; i++, shdr++) {
        if (shdr->SizeOfRawData == 0) continue;

        VOID *dst = (VOID *)(program_buffer + shdr->VirtualAddress);
        VOID *src = (UINT8 *)pe_buffer + shdr->PointerToRawData;
        UINTN len = shdr->SizeOfRawData;
        memcpy(dst, src, len);
    }

    // Return entry point
    VOID *entry_point = (VOID *)(program_buffer + opt_hdr->AddressOfEntryPoint);

    *pgm_buffer = program_buffer;
    return entry_point;
}

// ==========================
// Get Memory Map from UEFI
// ==========================
EFI_STATUS get_memory_map(Memory_Map_Info *mmap) { 
    memset(mmap, 0, sizeof *mmap);  // Ensure input parm is initialized

    // Get initial memory map size (send 0 for map size)
    EFI_STATUS status = EFI_SUCCESS;
    status = bs->GetMemoryMap(&mmap->size,
                              mmap->map,
                              &mmap->key,
                              &mmap->desc_size,
                              &mmap->desc_version);

    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL) {
        error(u"Could not get initial memory map size.\r\n");
        return status;
    }

    // Allocate buffer for actual memory map for size in mmap->size;
    //   need to allocate enough space for an additional memory descriptor or 2 in the map due to
    //   this allocation itself.
    mmap->size += mmap->desc_size * 2;  
    status = bs->AllocatePool(EfiLoaderData, mmap->size,(VOID **)&mmap->map);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not allocate buffer for memory map '%s'\r\n", status);
        return status;
    }

    // Call get memory map again to get the actual memory map now that the buffer is the correct
    //   size
    status = bs->GetMemoryMap(&mmap->size,
                              mmap->map,
                              &mmap->key,
                              &mmap->desc_size,
                              &mmap->desc_version);
    if (EFI_ERROR(status)) {
        error(u"Error %x; Could not get UEFI memory map! :(\r\n", status);
        return status;
    }

    return EFI_SUCCESS;
}

// ==================================================================
// Return pointer to configuration table given a GUID to search for
// ==================================================================
VOID *get_config_table(EFI_GUID guid) { 
    for (UINTN i = 0; i < st->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE cfg = st->ConfigurationTable[i];
        if (!memcmp(&cfg.VendorGuid, &guid, sizeof(EFI_GUID)))  
            return cfg.VendorTable;
    }
    return NULL;
}

// ==========================================================
// "Allocate" (find) next page of physical/available memory
// ==========================================================
void *allocate_pages(Memory_Map_Info *mmap, UINTN pages) {
    if (next_page_address == 0) {
        // Get first available area for allocations
        for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
            EFI_MEMORY_DESCRIPTOR *desc = 
                (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

            if (desc->Type == EfiConventionalMemory && desc->NumberOfPages >= pages) {
                current_descriptor = i;
                next_page_address = desc->PhysicalStart;
                available_pages = desc->NumberOfPages;
                break;
            }
        }
    }

    if (available_pages > 0) {
        void *page = (void *)next_page_address;
        available_pages -= pages;
        next_page_address += PAGE_SIZE * pages;
        return page;
    }

    // Available pages in current descriptor is 0,
    //   Find next available page of memory in EFI memory map
    UINTN i = current_descriptor + 1;
    for (; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        if (desc->Type == EfiConventionalMemory && desc->NumberOfPages >= pages) {
            current_descriptor = i;
            next_page_address = desc->PhysicalStart;
            available_pages = desc->NumberOfPages;
            break;
        }
    }

    if (i >= mmap->size / mmap->desc_size) return NULL;

    void *page = (void *)next_page_address;
    available_pages -= pages;
    next_page_address += PAGE_SIZE * pages;
    return page;
}

// =========================================================
// Map a 4KiB page of virtual memory to a physical address
// =========================================================
bool map_page(UINTN physical_address, UINTN virtual_address, Memory_Map_Info *mmap) {
    #define PRESENT  (1 << 0)
    #define WRITABLE (1 << 1)
    #define USER     (1 << 2)
    int flags = PRESENT | WRITABLE | USER;

    UINTN pml4_idx = (virtual_address >> 39) & 0x1FF;   // 0-511
    UINTN pdpt_idx = (virtual_address >> 30) & 0x1FF;   // 0-511
    UINTN pdt_idx  = (virtual_address >> 21) & 0x1FF;   // 0-511
    UINTN pt_idx   = (virtual_address >> 12) & 0x1FF;   // 0-511

    // Allocate new pdpt if not in pml4
    if (!(pml4->entries[pml4_idx] & PRESENT)) {
        UINTN pdpt_address = (UINTN)allocate_pages(mmap, 1);

        memset((void *)pdpt_address, 0, PAGE_SIZE);
        pml4->entries[pml4_idx] = pdpt_address | flags;
    }

    // Allocate new pdt if not in pdpt
    Paging_Table *pdpt = (Paging_Table *)(pml4->entries[pml4_idx] & PAGE_PHYS_ADDR_MASK);
    if (!(pdpt->entries[pdpt_idx] & PRESENT)) {
        UINTN pdt_address = (UINTN)allocate_pages(mmap, 1);

        memset((void *)pdt_address, 0, PAGE_SIZE);
        pdpt->entries[pdpt_idx] = pdt_address | flags;
    }

    // Allocate new pt if not in pdt
    Paging_Table *pdt = (Paging_Table *)(pdpt->entries[pdpt_idx] & PAGE_PHYS_ADDR_MASK);
    if (!(pdt->entries[pdt_idx] & PRESENT)) {
        UINTN pt_address = (UINTN)allocate_pages(mmap, 1);

        memset((void *)pt_address, 0, PAGE_SIZE);
        pdt->entries[pdt_idx] = pt_address | flags;
    }

    // Set page in page table with input physical address
    Paging_Table *pt = (Paging_Table *)(pdt->entries[pdt_idx] & PAGE_PHYS_ADDR_MASK);
    if (!(pt->entries[pt_idx] & PRESENT)) 
        pt->entries[pt_idx] = (physical_address & PAGE_PHYS_ADDR_MASK) | flags;

    return true;
}

// =====================================
// Unmap a 4KiB page of virtual memory 
// =====================================
bool unmap_page(UINTN virtual_address) {
    UINTN pml4_idx = (virtual_address >> 39) & 0x1FF;   // 0-511
    UINTN pdpt_idx = (virtual_address >> 30) & 0x1FF;   // 0-511
    UINTN pdt_idx  = (virtual_address >> 21) & 0x1FF;   // 0-511
    UINTN pt_idx   = (virtual_address >> 12) & 0x1FF;   // 0-511

    Paging_Table *pdpt = (Paging_Table *)(pml4->entries[pml4_idx] & PAGE_PHYS_ADDR_MASK);
    Paging_Table *pdt  = (Paging_Table *)(pdpt->entries[pdpt_idx] & PAGE_PHYS_ADDR_MASK);
    Paging_Table *pt   = (Paging_Table *)(pdt->entries[pdt_idx]   & PAGE_PHYS_ADDR_MASK);

    pt->entries[pt_idx] = 0;    // Wipe out physical address and flags

    // Flush unmapped page in TLB
    __asm__ __volatile__ ("invlpg (%0)" : : "r"(virtual_address));
    return true;
}

// ===============================================================================
// Identity map a 4KiB page of virtual memory; virtual address= physical address
// ===============================================================================
bool identity_map_page(UINTN virtual_address, Memory_Map_Info *mmap) {
    return map_page(virtual_address, virtual_address, mmap);
}

// ===========================================
// Initialize physical memory in Page Tables
// ===========================================
void init_physical_memory(Memory_Map_Info *mmap) {
    // Allocate top level page table
    pml4 = allocate_pages(mmap, 1);  
    memset(pml4, 0, sizeof *pml4);

    // Initialize all EFI identity mapped memory
    UINTN runtime_descriptors = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        for (UINTN j = 0; j < desc->NumberOfPages; j++) 
            identity_map_page(desc->PhysicalStart + (j*PAGE_SIZE), mmap);

        if (desc->Attribute & EFI_MEMORY_RUNTIME) runtime_descriptors++;
    }

    // Set runtime memory descriptor virtual addresses, to be able to call runtime 
    //   services from the loaded OS. The runtime descriptors will be identity mapped 
    //   from setting virtual start = physical start
    UINTN runtime_pages = (runtime_descriptors * mmap->desc_size) % PAGE_SIZE;
    runtime_pages++;    // In case of partial page amount of memory
    runtime_map = allocate_pages(mmap, runtime_pages);

    // Add all runtime descriptors to runtime only memory map
    UINTN curr_rt_desc = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        if (desc->Attribute & EFI_MEMORY_RUNTIME) {
            EFI_MEMORY_DESCRIPTOR *runtime_desc = 
                (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)runtime_map + (curr_rt_desc * mmap->desc_size));

            memcpy(runtime_desc, desc, mmap->desc_size);
            runtime_desc->VirtualStart = runtime_desc->PhysicalStart;   // Identity map
            curr_rt_desc++;
        }
    }

    // Set virtual address map for runtime services using runtime memory map
    rs->SetVirtualAddressMap(runtime_pages * PAGE_SIZE, mmap->desc_size, mmap->desc_version, runtime_map);
}

// ===========================================
// Load kernel binary to buffer and call it;
//   this will load the OS and not return!
// ===========================================
EFI_STATUS load_kernel(void) {
    VOID *file_buffer = NULL;
    VOID *disk_buffer = NULL;
    EFI_STATUS status = EFI_SUCCESS;

    // Print file info for DATAFLS.INF file from path "/EFI/BOOT/DATAFLS.INF"
    CHAR16 *file_name = u"\\EFI\\BOOT\\DATAFLS.INF";

    cout->ClearScreen(cout);

    UINTN buf_size = 0;
    file_buffer = read_esp_file_to_buffer(file_name, &buf_size);
    if (!file_buffer) {
        error(u"Could not find or read file '%s' to buffer\r\n", file_name);
        goto exit;
    }

    // Parse data from DATAFLS.INF file to get disk LBA and file size
    char *str_pos = NULL;
    str_pos = strstr(file_buffer, "kernel");
    if (!str_pos) {
        error(u"Could not find kernel file in data partition\r\n");
        goto cleanup;
    }

    printf(u"Found kernel file\r\n");

    str_pos = strstr(file_buffer, "FILE_SIZE=");
    if (!str_pos) {
        error(u"Could not find file size from buffer for '%s'\r\n", file_name);
        goto cleanup;
    }

    str_pos += strlen("FILE_SIZE=");
    UINTN file_size = atoi(str_pos);

    str_pos = strstr(file_buffer, "DISK_LBA=");
    if (!str_pos) {
        error(u"Could not find disk lba value from buffer for '%s'\r\n", file_name);
        goto cleanup;
    }

    str_pos += strlen("DISK_LBA=");
    UINTN disk_lba = atoi(str_pos);

    printf(u"File Size: %u, Disk LBA: %u\r\n", file_size, disk_lba);

    // Get media ID (disk number for Block IO protocol Media) for this running disk image
    UINT32 image_mediaID = 0;
    status = get_disk_image_mediaID(&image_mediaID);
    if (EFI_ERROR(status)) {
        error(u"Error: %x; Could not find or get MediaID value for disk image\r\n", status);
        bs->FreePool(file_buffer);  // Free memory allocated for ESP file
        goto exit;
    }

    // Read disk lbas for file into buffer
    disk_buffer = read_disk_lbas_to_buffer(disk_lba, file_size, image_mediaID);
    if (!disk_buffer) {
        error(u"Could not find or read data partition file to buffer\r\n");
        bs->FreePool(file_buffer);  // Free memory allocated for ESP file
        goto exit;
    }

    // Get GOP info for kernel parms
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(u"\r\nERROR: %x; Could not locate GOP! :(\r\n", status);
        return status;
    }

    // Initialize Kernel Parameters
    Kernel_Parms kparms = {0};  // Defined in efi_lib.h
    kparms.gop_mode = *gop->Mode;

    // Load Kernel File depending on format (initial header bytes)
    UINT8 *hdr = disk_buffer;
    printf(u"File Format: ");
    printf(u"Header bytes: [%x][%x][%x][%x]\r\n", 
           (UINTN)hdr[0], (UINTN)hdr[1], (UINTN)hdr[2], (UINTN)hdr[3]);

    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    UINTN program_size = 0;

    typedef void EFIAPI (*Entry_Point)(Kernel_Parms);
    Entry_Point entry = NULL, kernel_entry = NULL;
    if (!memcmp(hdr, (UINT8[4]){0x7F, 'E', 'L', 'F'}, 4)) {
        // Get around compiler warning about function vs void pointer
        *(void **)&entry = load_elf(disk_buffer, &program_buffer, &program_size);   

    } else if (!memcmp(hdr, (UINT8[2]){'M', 'Z'}, 2)) {
        // Get around compiler warning about function vs void pointer
        *(void **)&entry = load_pe(disk_buffer, &program_buffer, &program_size);   

    } else {
        printf(u"No format found, Assuming it's a flat binary file\r\n");
        // Get around compiler warning about function vs void pointer
        *(void **)&entry = disk_buffer;   
        program_buffer = (EFI_PHYSICAL_ADDRESS)disk_buffer;
        program_size = file_size;
    }

    if (!entry) {
        if (program_buffer > 0)
            bs->FreePages(program_buffer, program_size / PAGE_SIZE); 
        goto cleanup; 
    }

    kernel_entry = (Entry_Point)((UINTN)entry + KERNEL_ADDRESS);

    // Get ACPI table
    EFI_GUID acpi_20_guid = EFI_ACPI_TABLE_GUID, acpi_guid = ACPI_TABLE_GUID;
    VOID *ACPI_Table = get_config_table(acpi_20_guid);     // Get ACPI 2.0 table by default
    if (ACPI_Table) printf(u"\r\nFound ACPI 2.0 Table, ");
    else {
        ACPI_Table = get_config_table(acpi_guid);    // Use ACPI 1.0 table as fallback
        if (ACPI_Table) printf(u"\r\nFound ACPI 1.0 Table, ");
        else {
            error(u"\r\nERROR: Could not find ACPI Table.\r\n");
            goto cleanup;
        }
    }

    UINT8 *p = (UINT8 *)ACPI_Table;
    printf(u"Signature: \"%c%c%c%c%c%c%c%c\"\r\n", 
            p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);

    // Can't print after this point, getting memory map and exiting boot services!
    printf(u"\r\nPress any key to load kernel...\r\n");
    get_key();

    // Close Timer Event so that it does not continue to fire off
    bs->CloseEvent(timer_event);

    Gdt gdt = {0};
    // E.g. 64 bit Kernel Code segment:
    // uint64_t limit_15_0  : 16;  // 0xFFFF
    // uint64_t base_15_0   : 16;  // 0x0000
    // uint64_t base_23_16  : 8;   // 0x00
    // uint64_t type        : 4;   // 0b1010 (Code segment = Execute/Read)
    // uint64_t s_flag      : 1;   // 0b1  // 1 = Code or Data segment
    // uint64_t dpl         : 2;   // 0b00 // Ring level 0
    // uint64_t p_flag      : 1;   // 0b1  // Present flag
    // uint64_t limit_19_16 : 4;   // 0xF
    // uint64_t avl         : 1;   // 0b0  // Free to use
    // uint64_t l_flag      : 1;   // 0b1  // 64-bit code segment
    // uint64_t db_flag     : 1;   // 0b0  
    // uint64_t g_flag      : 1;   // 0b1  // Granularity, 1 = 4KiB
    // uint64_t base_31_24  : 8;   // 0x00
    gdt.kernel_code.value = 0x00AF9A000000FFFF;
    gdt.kernel_data.value = 0x00CF92000000FFFF; // db flag = 1, l flag = 0, type = 0b0010 (Data segment = Read/Write)

    gdt.user_code.value = 0x00AFFA000000FFFF;   // DPL = (ring) 3
    gdt.user_data.value = 0x00CFF2000000FFFF;   // db = 1, l = 0, dpl = 3, type = 2

    Task_State_Segment tss = {0};    // Won't be used, but need valid data in GDT
    tss.io_map_base_address = sizeof tss;

    UINT64 tss_address = (UINT64)&tss;
    gdt.tss = (Tss_Descriptor){
        .limit_15_0  = sizeof tss - 1,
        .base_15_0   = tss_address & 0xFFFF,
        .base_23_16  = (tss_address >> 16) & 0xFF,
        .type        = 9,   // 0b1001 = 64 bit TSS (available)
        .p_flag      = 1,
        .base_31_24  = (tss_address >> 24) & 0xFF,
        .base_63_32  = (tss_address >> 32) & 0xFFFFFFFF,
    };

    Descriptor_Table_Register gdtr = {.limit = sizeof gdt - 1, .base = (UINT64)&gdt};

    // Set rest of kernel parms
    kparms.RuntimeServices      = rs;
    kparms.NumberOfTableEntries = st->NumberOfTableEntries;
    kparms.ConfigurationTable   = st->ConfigurationTable;

    // Get Memory Map
    if (EFI_ERROR(get_memory_map(&kparms.mmap))) goto cleanup;

    // Exit boot services before calling kernel
    UINTN retries = 0;
    while (EFI_ERROR(bs->ExitBootServices(image, kparms.mmap.key)) && retries < 10) {
        // Firmware could have done a partial shutdown, get memory map again
        bs->FreePool(&kparms.mmap.map); 
        if (EFI_ERROR(get_memory_map(&kparms.mmap))) goto cleanup;
        retries++;
    }

    // TODO: Pass more parameters to kernel:
    //    - Physical memory map for physical memory manager (pass to kernel)
    //    - Virtual memory map for virtual memory manager (if not using EFI memory map for this? & pass to kernel)

    // Initialize page tables with EFI identity mapped memory, and set runtime services 
    //   virtual address map
    init_physical_memory(&kparms.mmap);

    // Map kernel to higher address
    for (UINTN i = 0; i < program_size; i += PAGE_SIZE)
        map_page(program_buffer + i, program_buffer + i + KERNEL_ADDRESS, &kparms.mmap);

    // Identity map framebuffer
    for (UINTN i = 0; i < (kparms.gop_mode.FrameBufferSize / PAGE_SIZE) + 1; i++)
        identity_map_page(kparms.gop_mode.FrameBufferBase + (i*PAGE_SIZE), &kparms.mmap);

    // Identity map stack
    #define STACK_PAGES 4   // 16KiB stack
    UINT8 *kernel_stack = allocate_pages(&kparms.mmap, STACK_PAGES);  
    memset(kernel_stack, 0, STACK_PAGES * PAGE_SIZE);

    for (UINTN i = 0; i < STACK_PAGES; i++)
        identity_map_page((UINTN)kernel_stack + (i*PAGE_SIZE), &kparms.mmap);

    Kernel_Parms *kparms_ptr = &kparms;

    // Set new page tables, TSS & GDT 
    __asm__ __volatile__ ("cli\n"                           // Clear interrupt flag before gdt/etc.
                          "mov %[kernel_entry], %%rbx\n"    // Kernel entry point
                          "mov %[kparms_ptr], %%rcx\n"      // MS ABI first register parameter

                          "movq %[pml4], %%rax\n"    
                          "movq %%rax, %%cr3\n"     // Set new page table mappings, will flush TLB

                          "lgdt %[gdt]\n"           // Load new Global Descriptor Table

                          "movw $0x30, %%ax\n"      // 0x30 = tss segment offset in gdt
                          "ltr %%ax\n"              // Load task register, with tss offset

                          "movq $0x08, %%rax\n"
                          "pushq %%rax\n"           // Push kernel code segment
                          "leaq 1f(%%rip), %%rax\n"    // Use relative offset for label
                          "pushq %%rax\n"           // Push return address
                          "lretq\n"                 // Far return, pop return address into IP, 
                                                    //   and pop code segment into CS
                          "1:\n"
                          "movw $0x10, %%ax\n"      // 0x10 = kernel data segment
                          "movw %%ax, %%ds\n"       // Load data segment registers
                          "movw %%ax, %%es\n"
                          "movw %%ax, %%fs\n"
                          "movw %%ax, %%gs\n"

                          "movw %%ax, %%ss\n"       // Set new stack, will grow down
                          "movq %[stack], %%rsp\n"        

                          "call *%%rbx\n"           // Call kernel entry point, for MS ABI the 1st 
                                                    //   parm kparms_ptr is in RCX from above.
                                                    // Also pushes current address (8 bytes) onto
                                                    //   stack, for stack alignment.

                          : 
                          : [pml4]"gm"(pml4), [gdt]"gm"(gdtr), 
                            [stack]"gm"(kernel_stack + (STACK_PAGES * PAGE_SIZE)), 
                            [kernel_entry]"gm"(kernel_entry), [kparms_ptr]"gm"(kparms_ptr)
                          : "memory");

    // Should not reach this point!
    __builtin_unreachable();

    // Final cleanup
    cleanup:
    bs->FreePool(file_buffer);      // Free memory allocated for ESP file
    bs->FreePool(disk_buffer);      // Free memory allocated for data partition file
    if (kparms.mmap.map)
        bs->FreePool(kparms.mmap.map);  // Free memory allocated for memory map

    exit:
    printf(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ====================
// Print Memory Map
// ====================
EFI_STATUS print_memory_map(void) { 
    cout->ClearScreen(cout);

    // Close Timer Event for cleanup
    bs->CloseEvent(timer_event);

    Memory_Map_Info mmap = {0};
    get_memory_map(&mmap);

    // Print memory map descriptor values
    printf(u"Memory map: Size %u, Descriptor size: %u, # of descriptors: %u, key: %x\r\n",
            mmap.size, mmap.desc_size, mmap.size / mmap.desc_size, mmap.key);

    UINTN usable_bytes = 0; // "Usable" memory for an OS or similar, not firmware/device reserved
    for (UINTN i = 0; i < mmap.size / mmap.desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap.map + (i * mmap.desc_size));

        printf(u"%u: Typ: %u, Phy: %x, Vrt: %x, Pgs: %u, Att: %x\r\n",
                i,
                desc->Type, 
                desc->PhysicalStart, 
                desc->VirtualStart, 
                desc->NumberOfPages, 
                desc->Attribute);

        // Add to usable memory count depending on type
        if (desc->Type == EfiLoaderCode         || 
            desc->Type == EfiLoaderData         || 
            desc->Type == EfiBootServicesCode   || 
            desc->Type == EfiBootServicesData   || 
            desc->Type == EfiConventionalMemory || 
            desc->Type == EfiPersistentMemory) {

            usable_bytes += desc->NumberOfPages * 4096;
        }

        // Pause every ~20 lines
        if (i > 0 && i % 20 == 0) 
            get_key();
    }

    printf(u"\r\nUsable memory: %u / %u MiB / %u GiB\r\n",
            usable_bytes, usable_bytes / (1024 * 1024), usable_bytes / (1024 * 1024 * 1024));

    // Free allocated buffer for memory map
    bs->FreePool(mmap.map);

    printf(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// =================================
// Print Configuration Table GUIDs
// =================================
EFI_STATUS print_config_tables(void) { 
    cout->ClearScreen(cout);

    // Close Timer Event 
    bs->CloseEvent(timer_event);

    for (UINTN i = 0; i < st->NumberOfTableEntries; i++) {
        EFI_GUID guid = st->ConfigurationTable[i].VendorGuid;
        print_guid(guid);

        bool found = false;
        for (UINTN j = 0; j < ARRAY_SIZE(STANDARD_GUIDS); j++) 
            if (!memcmp(&STANDARD_GUIDS[j].guid, &guid, sizeof(EFI_GUID))) { 
                printf(u"(%s)\r\n\r\n", STANDARD_GUIDS[j].string);
                found = true;
                break;
            }

        if (!found) printf(u"(Unknown)\r\n\r\n");

        // Pause every so often
        if (i > 0 && i % 7 == 0) get_key();
    }

    printf(u"\r\nPress any key to go back...\r\n");
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
        u"Print Memory Map",
        u"Print Config Table GUIDs",
        u"Load Kernel",
    };

    // Functions to call for each menu option
    EFI_STATUS (*menu_funcs[])(void) = {
        set_text_mode,
        set_graphics_mode,
        test_mouse,
        read_esp_files,
        print_block_io_partitions,
        print_memory_map,
        print_config_tables,
        load_kernel,
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

        // Close Timer Event for cleanup
        bs->CloseEvent(timer_event);

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
