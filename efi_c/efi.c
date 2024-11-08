#include <stdarg.h>
#include "efi.h"
#include "efi_lib.h"

#include "ter_132n_psf_font.txt"

// -----------------
// Global macros
// -----------------
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x)[0])
#define max(x, y) ((x) > (y) ? (x) : (y))

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

#define PHYS_PAGE_ADDR_MASK 0x000FFFFFFFFFF000  // 52 bit physical address limit, lowest 12 bits are for flags only
#define PAGE_SIZE 4096  // 4KiB

// Kernel start address in higher memory (64-bit) - last 2 GiBs of virtual memory
#define KERNEL_START_ADDRESS 0xFFFFFFFF80000000

#ifdef __clang__
int _fltused = 0;   // If using floating point code & lld-link, need to define this
#endif

// Page flags: bits 11-0
enum {
    PRESENT    = (1 << 0),
    READWRITE  = (1 << 1),
    USER       = (1 << 2),
};

// Page table structure: 512 64bit entries per table/level
typedef struct {
    UINT64 entries[512];
} Page_Table;

// -----------------
// Global variables
// -----------------
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;  // Console output
EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin  = NULL;  // Console input
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr = NULL;  // Console output - stderr
EFI_BOOT_SERVICES    *bs;   // Boot services
EFI_RUNTIME_SERVICES *rs;   // Runtime services
EFI_SYSTEM_TABLE     *st;   // System Table
EFI_HANDLE image = NULL;    // Image handle

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

Page_Table *pml4 = NULL;   // Top level 4 page table for x86_64 long mode paging

INT32 text_rows = 0, text_cols = 0;

// ====================
// Set global vars
// ====================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
    //cerr = systable->StdErr;  // Stderr can be set to a serial output or other non-display device.
    cerr = cout;                // Use stdout for error printing 
    st = systable;
    bs = st->BootServices;
    rs = st->RuntimeServices;
    image = handle;
}

// ================================
// Print a number to stdout
// ================================
BOOLEAN print_number(UINTN number, UINT8 base, BOOLEAN is_signed, UINTN min_digits, CHAR16 *buf, 
                     UINTN *buf_idx) {
    
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

    while (i < min_digits) buffer[i++] = u'0'; // Pad with 0s

    // Print negative sign for decimal numbers
    if (base == 10 && negative) buffer[i++] = u'-';

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Add number string to input buffer for printing
    for (CHAR16 *p = buffer; *p; p++) {
        buf[*buf_idx] = *p;
        *buf_idx += 1;
    }
    return TRUE;
}

// ===================================================================
// Print formatted strings to stdout, using a va_list for arguments
// ===================================================================
bool vfprintf(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *stream, CHAR16 *fmt, va_list args) {
    bool result = true;
    CHAR16 charstr[2] = {0};    

    CHAR16 buf[1024];   // Format string buffer for % strings
    UINTN buf_idx = 0;
    
    // Initialize buffers
    charstr[0] = u'\0', charstr[1] = u'\0';

    // Print formatted string values
    for (UINTN i = 0; fmt[i] != u'\0'; i++) {
        if (fmt[i] == u'%') {
            bool alternate_form = false;
            UINTN min_field_width = 0;
            UINTN precision = 0;
            UINTN length_bits = 0;  
            UINTN num_printed = 0;      // # of digits/chars printed for numbers or strings
            UINT8 base = 0;
            bool signed_num = false;
            bool numeric = false;
            bool left_justify = false;  // Left justify text from '-' flag instead of default right justify
            CHAR16 padding_char = ' ';  // '0' or ' ' depending on flags
            i++;

            // Initialize format string buffer
            memset(buf, 0, sizeof buf);
            buf_idx = 0;

            // Check for flags
            while (true) {
                switch (fmt[i]) {
                    case u'#':
                        // Alternate form
                        alternate_form = true;
                        i++;
                        break;

                    case u'0':
                        // 0-pad numbers on the left, unless '-' or precision is also defined
                        padding_char = '0'; 
                        i++;
                        break;

                    case u' ':
                        // TODO:
                        i++;
                        break;

                    case u'+':
                        // TODO:
                        i++;
                        break;

                    case u'-':
                        left_justify = true;
                        i++;
                        break;

                    default:
                        break;
                }
                break; // No more flags
            }

            // Check for minimum field width e.g. in "8.2" this would be 8
            if (fmt[i] == u'*') {
                // Get int argument for min field width
                min_field_width = va_arg(args, int);
                i++;
            } else {
                // Get number literal from format string
                while (isdigit_c16(fmt[i])) 
                    min_field_width = (min_field_width * 10) + (fmt[i++] - u'0');
            }

            // Check for precision/maximum field width e.g. in "8.2" this would be 2
            if (fmt[i] == u'.') {
                i++;
                if (fmt[i] == u'*') {
                    // Get int argument for precision
                    precision = va_arg(args, int);
                    i++;
                } else {
                    // Get number literal from format string
                    while (isdigit_c16(fmt[i])) 
                        precision = (precision * 10) + (fmt[i++] - u'0');
                }
            }

            // Check for Length modifiers e.g. h/hh/l/ll
            if (fmt[i] == u'h') {
                i++;
                length_bits = 16;       // h
                if (fmt[i] == u'h') {
                    i++;
                    length_bits = 8;    // hh
                }
            } else if (fmt[i] == u'l') {
                i++;
                length_bits = 32;       // l
                if (fmt[i] == u'l') {
                    i++;
                    length_bits = 64;    // ll
                }
            }

            // Check for conversion specifier
            switch (fmt[i]) {
                case u'c': {
                    // Print CHAR16 value; printf("%c", char)
                    if (length_bits == 8)
                        charstr[0] = (char)va_arg(args, int);   // %hhc "ascii" or other 8 bit char
                    else
                        charstr[0] = (CHAR16)va_arg(args, int); // Assuming 16 bit char16_t

                    buf[buf_idx++] = charstr[0];
                }
                break;

                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    if (length_bits == 8) {
                        char *string = va_arg(args, char*);         // %hhs; Assuming 8 bit ascii chars
                        while (*string) {
                            buf[buf_idx++] = *string++;
                            if (++num_printed == precision) break;  // Stop printing at max characters
                        }

                    } else {
                        CHAR16 *string = va_arg(args, CHAR16*);     // Assuming 16 bit char16_t
                        while (*string) {
                            buf[buf_idx++] = *string++;
                            if (++num_printed == precision) break;  // Stop printing at max characters
                        }
                    }
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    numeric = true;
                    base = 10;
                    signed_num = true;
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    numeric = true;
                    base = 16;
                    signed_num = false;
                    if (alternate_form) {
                        buf[buf_idx++] = u'0';
                        buf[buf_idx++] = u'x';
                    }
                }
                break;

                case u'u': {
                    // Print UINT32; printf("%u", number_uint32)
                    numeric = true;
                    base = 10;
                    signed_num = false;
                }
                break;

                case u'b': {
                    // Print UINTN as binary; printf("%b", number_uintn)
                    numeric = true;
                    base = 2;
                    signed_num = false;
                    if (alternate_form) {
                        buf[buf_idx++] = u'0';
                        buf[buf_idx++] = u'b';
                    }
                }
                break;

                case u'o': {
                    // Print UINTN as octal; printf("%o", number_uintn)
                    numeric = true;
                    base = 8;
                    signed_num = false;
                    if (alternate_form) {
                        buf[buf_idx++] = u'0';
                        buf[buf_idx++] = u'o';
                    }
                }
                break;

                default:
                    stream->OutputString(stream, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    stream->OutputString(stream, charstr);
                    stream->OutputString(stream, u"\r\n");
                    result = false;
                    goto end;
                    break;
            }

            if (numeric) {
                // Printing a number
                UINT64 number = 0;
                switch (length_bits) {
                    case 0:
                    case 32:        // l
                    default:
                        number = va_arg(args, UINT32);
                        break;

                    case 8:
                        // hh
                        number = (UINT8)va_arg(args, int);
                        break;

                    case 16:
                        // h
                        number = (UINT16)va_arg(args, int);
                        break;

                    case 64:
                        // ll
                        number = va_arg(args, UINT64);
                        break;
                }
                print_number(number, base, signed_num, precision, buf, &buf_idx);  
            }

            // Print padding depending on flags (0 or space) and left/right justify, and 
            //   print buffer contents for % formatted conversion string
            buf[buf_idx] = u'\0';   // Null terminate buffer string

            // Flags are defined such that 0 is overruled by left justify and precision
            if (padding_char == u'0' && (left_justify || precision > 0))
                padding_char = u' ';

            charstr[0] = padding_char;
            charstr[1] = u'\0';

            if (left_justify) {
                // Print buffer contents and then blanks up until min field width for padding
                stream->OutputString(stream, buf);
                while (buf_idx < min_field_width) {
                    stream->OutputString(stream, charstr);
                    buf_idx++;
                }
            } else {
                // Default/right justified; Print padding first and then buffer contents
                while (buf_idx < min_field_width) {
                    stream->OutputString(stream, charstr);
                    buf_idx++;
                }
                stream->OutputString(stream, buf);
            }

        } else {
            // Not formatted string, print next character
            charstr[0] = fmt[i];
            stream->OutputString(stream, charstr);
        }
    }

end:
    va_end(args);

    return result;
}

// ===================================
// Print formatted strings to stdout
// ===================================
bool printf(CHAR16 *fmt, ...) {
    bool result = true;

    va_list args;
    va_start(args, fmt);
    result = vfprintf(cout, fmt, args);
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
bool error(char *file, int line, const char *func, EFI_STATUS status, CHAR16 *fmt, ...) {
    printf(u"\r\nERROR: FILE %hhs, LINE %d, FUNCTION %hhs\r\n", file, line, func);

    // Print error code & string if applicable
    if (status > 0 && status - TOP_BIT < MAX_EFI_ERROR)
        printf(u"STATUS: %#llx (%s)\r\n", status, EFI_ERROR_STRINGS[status-TOP_BIT]);

    va_list args;
    va_start(args, fmt);
    bool result = vfprintf(cerr, fmt, args); // Printf the error message to stderr
    va_end(args);

    get_key();  // User will respond with input before going on
    return result;
}
#define error(...) error(__FILE__, __LINE__, __func__, __VA_ARGS__)

// ==================================================
// Get an integer number from the user with a get_key() loop
//   and print to screen
// ==================================================
BOOLEAN get_int(INTN *number) {
    EFI_INPUT_KEY key = {0};

    if (!number) return false;  // Passed in NULL pointer

    *number = 0;
    do {
        key = get_key();
        if (key.ScanCode == SCANCODE_ESC) return false; // User wants to leave
        if (isdigit_c16(key.UnicodeChar)) {
            *number = (*number * 10) + (key.UnicodeChar - u'0');
            printf(u"%c", key.UnicodeChar);
        }
    } while (key.UnicodeChar != u'\r');

    return true;
}

// ==================================================
// Get a hexadecimal number from the user with a get_key() loop
//   and print to screen
// ==================================================
BOOLEAN get_hex(UINTN *number) {
    EFI_INPUT_KEY key = {0};

    if (!number) return false;  // Passed in NULL pointer

    *number = 0;
    do {
        key = get_key();
        if (key.ScanCode == SCANCODE_ESC) return false; // User wants to leave
        if (isdigit_c16(key.UnicodeChar)) {
            *number = (*number * 16) + (key.UnicodeChar - u'0');
            printf(u"%c", key.UnicodeChar);
        } else if (key.UnicodeChar >= u'a' && key.UnicodeChar <= u'f') {
            *number = (*number * 16) + (key.UnicodeChar - u'a' + 10);
            printf(u"%c", key.UnicodeChar);
        } else if (key.UnicodeChar >= u'A' && key.UnicodeChar <= u'F') {
            *number = (*number * 16) + (key.UnicodeChar - u'A' + 10);
            printf(u"%c", key.UnicodeChar);
        }
    } while (key.UnicodeChar != u'\r');

    return true;
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
        error(status, u"Could not open Loaded Image Protocol\r\n");
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
        error(status, u"Could not open Simple File System Protocol\r\n");
        goto cleanup;
    }

    // Open root directory via OpenVolume()
    EFI_FILE_PROTOCOL *root = NULL;
    status = sfsp->OpenVolume(sfsp, &root);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Open Volume for root directory in ESP\r\n");
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
        error(status, u"Could not open file '%s'\r\n", path);
        goto cleanup;
    }

    // Get info for file, to grab file size
    EFI_FILE_INFO file_info;
    EFI_GUID fi_guid = EFI_FILE_INFO_ID;
    UINTN buf_size = sizeof(EFI_FILE_INFO);
    status = file->GetInfo(file, &fi_guid, &buf_size, &file_info);
    if (EFI_ERROR(status)) {
        error(status, u"Could not get file info for file '%s'\r\n", path);
        goto file_cleanup;
    }

    // Allocate buffer for file
    buf_size = file_info.FileSize;
    status = bs->AllocatePool(EfiLoaderData, buf_size, &file_buffer);
    if (EFI_ERROR(status) || buf_size != file_info.FileSize) {
        error(status, u"Could not allocate memory for file '%s'\r\n", path);
        goto file_cleanup;
    }

    if (EFI_ERROR(status)) {
        error(status, u"Could not get file info for file '%s'\r\n", path);
        goto file_cleanup;
    }

    // Read file into buffer
    status = file->Read(file, &buf_size, file_buffer);
    if (EFI_ERROR(status) || buf_size != file_info.FileSize) {
        error(status, u"Could not read file '%s' into buffer\r\n", path);
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
//  found or error. If executable input parameter is true, then allocate 
//  EfiLoaderCode memory type, else use EfiLoaderData.
//
//  NOTE: Caller will have to use FreePool() on returned buffer to 
//    free allocated memory.
// =================================================================
EFI_PHYSICAL_ADDRESS read_disk_lbas_to_buffer(EFI_LBA disk_lba, UINTN data_size, UINT32 disk_mediaID, bool executable) {
    EFI_PHYSICAL_ADDRESS buffer = 0;
    EFI_STATUS status = EFI_SUCCESS;

    // Loop through and get Block IO protocol for input media ID, for entire disk
    //   NOTE: This assumes the first Block IO found with logical partition false is the entire disk
    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;

    status = bs->LocateHandleBuffer(ByProtocol, &bio_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate any Block IO Protocols.\r\n");
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
            error(status, u"Could not Open Block IO protocol on handle %u.\r\n", i);
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
        error(0, u"Could not find Block IO protocol for disk with ID %u.\r\n", disk_mediaID);
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
        error(status, u"Could not Open Disk IO protocol on handle %u.\r\n", i);
        goto cleanup;
    }

    // Allocate buffer for data
    UINTN pages_needed = (data_size + (PAGE_SIZE-1)) / PAGE_SIZE;
    if (executable) 
        status = bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages_needed, &buffer);
    else 
        status = bs->AllocatePages(AllocateAnyPages, EfiLoaderData, pages_needed, &buffer);

    if (EFI_ERROR(status)) {
        error(status, u"Could not Allocate buffer for disk data.\r\n");
        bs->CloseProtocol(handle_buffer[i],
                          &dio_guid,
                          image,
                          NULL);
        goto cleanup;
    }

    // Use Disk IO Read to read into allocated buffer
    status = diop->ReadDisk(diop, disk_mediaID, disk_lba * biop->Media->BlockSize, data_size, (VOID *)buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not read Disk LBAs into buffer.\r\n");
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

                        // Set global rows/cols values
                        text_rows = text_modes[mode_index].rows;
                        text_cols = text_modes[mode_index].cols;

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
        error(status, u"Could not locate GOP! :(\r\n");
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
            error(status, u"Could not Query GOP Mode %u\r\n", gop->Mode->Mode);
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
        error(status, u"Could not locate GOP! :(\r\n");
        return status;
    }
    if((*gop).Mode != NULL) {
    	mode_index = (*(*gop).Mode).Mode;
    }

    gop->QueryMode(gop, mode_index, &mode_info_size, &mode_info);

    // Use LocateHandleBuffer() to find all SPPs 
    status = bs->LocateHandleBuffer(ByProtocol, &spp_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate Simple Pointer Protocol handle buffer.\r\n");
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
            error(status, u"Could not Open Simple Pointer Protocol on handle.\r\n");
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
    
    if (!found_mode) error(0, u"\r\nCould not find any valid SPP Mode.\r\n");

    // Free memory pool allocated by LocateHandleBuffer()
    bs->FreePool(handle_buffer);

    // Use LocateHandleBuffer() to find all APPs 
    num_handles = 0;
    handle_buffer = NULL;
    found_mode = FALSE;

    status = bs->LocateHandleBuffer(ByProtocol, &app_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) 
        error(status, u"Could not locate Absolute Pointer Protocol handle buffer.\r\n");

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
            error(status, u"Could not Open Simple Pointer Protocol on handle.\r\n");
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
    
    if (!found_mode) error(0, u"Could not find any valid APP Mode.\r\n");

    if (num_protocols == 0) {
        error(0, u"Could not find any Simple or Absolute Pointer Protocols.\r\n");
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
        error(status, u"Could not open Loaded Image Protocol\r\n");
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
        error(status, u"Could not open Simple File System Protocol\r\n");
        return status;
    }

    // Open root directory via OpenVolume()
    EFI_FILE_PROTOCOL *dirp = NULL;
    status = sfsp->OpenVolume(sfsp, &dirp);
    if (EFI_ERROR(status)) {
        error(status, u"Could not Open Volume for root directory\r\n");
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
                            error(status, u"Could not open new directory %s\r\n", file_info.FileName);
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
                        error(status, u"Could not allocate memory for file %s\r\n", file_info.FileName);
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
                        error(status, u"Could not open file %s\r\n", file_info.FileName);
                        goto cleanup;
                    }

                    // Read file into buffer
                    status = dirp->Read(file, &buf_size, buffer);
                    if (EFI_ERROR(status)) {
                        error(status, u"Could not read file %s into buffer.\r\n", file_info.FileName);
                        goto cleanup;
                    } 

                    if (buf_size != file_info.FileSize) {
                        error(0, u"Could not read all of file %s into buffer.\r\n" 
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
        error(status, u"Could not open Loaded Image Protocol\r\n");
        return status;
    }

    status = bs->OpenProtocol(lip->DeviceHandle,
                              &bio_guid,
                              (VOID **)&biop,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        error(status, u"Could not open Block IO Protocol for this loaded image.\r\n");
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
        error(status, u"Could not locate any Block IO Protocols.\r\n");
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
            error(status, u"Could not Open Block IO protocol on handle %u.\r\n", i);
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
                error(status, u"Could not Open Partition Info protocol on handle %u.\r\n", i);
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
        error(status, u"Could not open Loaded Image Protocol\r\n");
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
        error(status, u"Could not open Block IO Protocol for this loaded image.\r\n");
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
VOID *load_elf(VOID *elf_buffer, EFI_PHYSICAL_ADDRESS *file_buffer, UINTN *file_size) {
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
        error(0, u"ELF is not a PIE file; e_type is not ETDYN/0x03\r\n");
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

    // NOTE: May want to switch this for allocating a kernel to use AllocateAddress to put the buffer
    //   starting at a specific address e.g. in higher half memory
    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    UINTN pages_needed = (max_memory_needed + (PAGE_SIZE-1)) / PAGE_SIZE;
    status = bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages_needed, &program_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate memory for ELF program\r\n");
        return NULL;
    }

    // Zero init buffer, to ensure 0 padding for all program sections
    memset((VOID *)program_buffer, 0, max_memory_needed);

    // Fill out input parms for caller
    *file_buffer = program_buffer;
    *file_size   = pages_needed * PAGE_SIZE;

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
        UINT8 *dst = (UINT8 *)program_buffer + relative_offset; 
        UINT8 *src = (UINT8 *)elf_buffer     + phdr->p_offset;
        UINT32 len = phdr->p_filesz;
        memcpy(dst, src, len);
    }

    // Return entry point in new buffer, with same relative offset as in the original buffer 
    VOID *entry_point = (VOID *)((UINT8 *)program_buffer + (ehdr->e_entry - mem_min));

    return entry_point;
}

// ==========================================================
// Load an PE32+ PIE file into a new buffer, and return the 
//   entry point for the loaded PE program
// ==========================================================
VOID *load_pe(VOID *pe_buffer, EFI_PHYSICAL_ADDRESS *file_buffer, UINTN *file_size) {
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
        error(0, u"Machine type not AMD64.\r\n");
        return NULL;
    }

    if (!(coff_hdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        error(0, u"File not an executable image.\r\n");
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
        error(0, u"File not a PE32+ file.\r\n");
        return NULL;
    }

    if (!(opt_hdr->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) {
        error(0, u"File not a PIE file.\r\n");
        return NULL;
    }

    // Allocate buffer to load sections into
    EFI_PHYSICAL_ADDRESS program_buffer = 0;
    EFI_STATUS status = 0;
    UINTN pages_needed = (opt_hdr->SizeOfImage + (PAGE_SIZE-1)) / PAGE_SIZE;
    status = bs->AllocatePages(AllocateAnyPages, EfiLoaderCode, pages_needed, &program_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate memory for PE file.\r\n");
        return NULL;
    }

    // Initialize buffer to 0, which should also take care of needing to 0-pad sections between
    //   Raw Data and Virtual Size
    memset((VOID *)program_buffer, 0, opt_hdr->SizeOfImage);
    *file_buffer = program_buffer;
    *file_size   = pages_needed * PAGE_SIZE;

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

        VOID *dst = (UINT8 *)program_buffer + shdr->VirtualAddress;
        VOID *src = (UINT8 *)pe_buffer + shdr->PointerToRawData;
        UINTN len = shdr->SizeOfRawData;
        memcpy(dst, src, len);
    }

    // Return entry point
    VOID *entry_point = (UINT8 *)program_buffer + opt_hdr->AddressOfEntryPoint;

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
        error(0, u"Could not get initial memory map size.\r\n");
        return status;
    }

    // Allocate buffer for actual memory map for size in mmap->size;
    //   need to allocate enough space for an additional memory descriptor or 2 in the map due to
    //   this allocation itself.
    mmap->size += mmap->desc_size * 2;  
    status = bs->AllocatePool(EfiLoaderData, mmap->size,(VOID **)&mmap->map);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for memory map '%s'\r\n");
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
        error(status, u"Could not get UEFI memory map! :(\r\n");
        return status;
    }

    return EFI_SUCCESS;
}

// ==================================================
// Allocate pages from available UEFI Memory Map;
//   technically not allocating more, but returning
//   available pre-existing page addresses.
//   Using this as a sort of bump allocator.
// ==================================================
void *mmap_allocate_pages(Memory_Map_Info *mmap, UINTN pages) {
    static void *next_page_address = NULL;  // Next page/page range address to return to caller
    static UINTN current_descriptor = 0;    // Current descriptor number
    static UINTN remaining_pages = 0;       // Remaining pages in current descriptor

    if (remaining_pages < pages) {
        // Not enough remaining pages in current descriptor, find the next available one
        UINTN i = current_descriptor+1;
        for (; i < mmap->size / mmap->desc_size; i++) {
            EFI_MEMORY_DESCRIPTOR *desc = 
                (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

            if (desc->Type == EfiConventionalMemory && desc->NumberOfPages >= pages) {
                // Found enough memory to use at this descriptor, use it
                current_descriptor = i;
                remaining_pages = desc->NumberOfPages - pages;
                next_page_address = (void *)(desc->PhysicalStart + (pages * PAGE_SIZE));
                return (void *)desc->PhysicalStart;
            }
        }

        if (i >= mmap->size / mmap->desc_size) {
            // Ran out of descriptors to check in memory map
            error(0, u"\r\nCould not find any memory to allocate pages for.\r\n");
            return NULL;
        }
    }

    // Else we have at least enough pages for this allocation, return the current spot in 
    //   the memory map
    remaining_pages -= pages;
    void *page = next_page_address;
    next_page_address = (void *)((UINT8 *)page + (pages * PAGE_SIZE));
    return page;
}

// ==================================================================
// Map a virtual address to a physical address for a page of memory
// ==================================================================
void map_page(UINTN physical_address, UINTN virtual_address, Memory_Map_Info *mmap) {
    int flags = PRESENT | READWRITE | USER;   // 0b111

    UINTN pml4_index = ((virtual_address) >> 39) & 0x1FF;   // 0-511
    UINTN pdpt_index = ((virtual_address) >> 30) & 0x1FF;   // 0-511
    UINTN pdt_index  = ((virtual_address) >> 21) & 0x1FF;   // 0-511
    UINTN pt_index   = ((virtual_address) >> 12) & 0x1FF;   // 0-511

    // Make sure pdpt exists, if not then allocate it
    if (!(pml4->entries[pml4_index] & PRESENT)) {
        void *pdpt_address = mmap_allocate_pages(mmap, 1);

        memset(pdpt_address, 0, sizeof(Page_Table));
        pml4->entries[pml4_index] = (UINTN)pdpt_address | flags;  
    }

    // Make sure pdt exists, if not then allocate it
    Page_Table *pdpt = (Page_Table *)(pml4->entries[pml4_index] & PHYS_PAGE_ADDR_MASK);
    if (!(pdpt->entries[pdpt_index] & PRESENT)) {
        void *pdt_address = mmap_allocate_pages(mmap, 1);

        memset(pdt_address, 0, sizeof(Page_Table));
        pdpt->entries[pdpt_index] = (UINTN)pdt_address | flags;  
    }

    // Make sure pt exists, if not then allocate it
    Page_Table *pdt = (Page_Table *)(pdpt->entries[pdpt_index] & PHYS_PAGE_ADDR_MASK);
    if (!(pdt->entries[pdt_index] & PRESENT)) {
        void *pt_address = mmap_allocate_pages(mmap, 1);

        memset(pt_address, 0, sizeof(Page_Table));
        pdt->entries[pdt_index] = (UINTN)pt_address | flags;  
    }

    // Map new page physical address if not present
    Page_Table *pt = (Page_Table *)(pdt->entries[pdt_index] & PHYS_PAGE_ADDR_MASK);
    if (!(pt->entries[pt_index] & PRESENT)) 
        pt->entries[pt_index] = (physical_address & PHYS_PAGE_ADDR_MASK) | flags;
}

// ==============================
// Unmap a page/virtual address 
// ==============================
void unmap_page(UINTN virtual_address) {
    UINTN pml4_index = ((virtual_address) >> 39) & 0x1FF;   // 0-511
    UINTN pdpt_index = ((virtual_address) >> 30) & 0x1FF;   // 0-511
    UINTN pdt_index  = ((virtual_address) >> 21) & 0x1FF;   // 0-511
    UINTN pt_index   = ((virtual_address) >> 12) & 0x1FF;   // 0-511

    Page_Table *pdpt = (Page_Table *)(pml4->entries[pml4_index] & PHYS_PAGE_ADDR_MASK);
    Page_Table *pdt = (Page_Table *)(pdpt->entries[pdpt_index] & PHYS_PAGE_ADDR_MASK);
    Page_Table *pt = (Page_Table *)(pdt->entries[pdt_index] & PHYS_PAGE_ADDR_MASK);

    pt->entries[pt_index] = 0;  // Clear page in page table to unmap the physical address there

    // Flush the TLB cache for this page
    __asm__ __volatile__("invlpg (%0)\n" : : "r"(virtual_address));
}

// ===========================================================
// Identity map a page of memory, virtual = physical address
// ===========================================================
void identity_map_page(UINTN address, Memory_Map_Info *mmap) {
    map_page(address, address, mmap);
}

// ======================================================================
// Initialize new paging setup by identity mapping all available memory 
//   from EFI memory map
// ======================================================================
void identity_map_efi_mmap(Memory_Map_Info *mmap) {
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        for (UINTN j = 0; j < desc->NumberOfPages; j++)
            identity_map_page(desc->PhysicalStart + (j * PAGE_SIZE), mmap);
    }
}

// ======================================================================
// Identity map runtime memory descriptors only, to use with
//   RuntimeServices->SetVirtualAddressMap()
// ======================================================================
void set_runtime_address_map(Memory_Map_Info *mmap) {
    // First get amount of memory to allocate for runtime memory map
    UINTN runtime_descriptors = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        if (desc->Attribute & EFI_MEMORY_RUNTIME)
            runtime_descriptors++;
    }

    // Allocate memory for runtime memory map
    UINTN runtime_mmap_pages = (runtime_descriptors * mmap->desc_size) + ((PAGE_SIZE-1) / PAGE_SIZE);
    EFI_MEMORY_DESCRIPTOR *runtime_mmap = mmap_allocate_pages(mmap, runtime_mmap_pages);
    if (!runtime_mmap) {
        error(0, u"Could not allocate runtime descriptors memory map\r\n");
        return;
    }

    // Identity map all runtime descriptors in each descriptor
    UINTN runtime_mmap_size = runtime_mmap_pages * PAGE_SIZE; 
    memset(runtime_mmap, 0, runtime_mmap_size);

    // Set all runtime descriptors in new runtime memory map, and identity map them
    UINTN curr_runtime_desc = 0;
    for (UINTN i = 0; i < mmap->size / mmap->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap->map + (i * mmap->desc_size));

        if (desc->Attribute & EFI_MEMORY_RUNTIME) {
            EFI_MEMORY_DESCRIPTOR *runtime_desc = 
                (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)runtime_mmap + (curr_runtime_desc * mmap->desc_size));

            memcpy(runtime_desc, desc, mmap->desc_size);    
            runtime_desc->VirtualStart = runtime_desc->PhysicalStart;
            curr_runtime_desc++;
        }
    }

    // Set new virtual addresses for runtime memory via SetVirtualAddressMap()
    EFI_STATUS status = rs->SetVirtualAddressMap(runtime_mmap_size, 
                                                 mmap->desc_size, 
                                                 mmap->desc_version,
                                                 runtime_mmap);
    if (EFI_ERROR(status)) error(0, u"SetVirtualAddressMap()\r\n");
}

// ===================================================
// Get a package list from the HII database
// NOTE: This allocates memory with AllocatePool(),
//   Caller should free returned address with e.g. 
//   if (result) bs->FreePool(result);
// TODO: Put in efi_lib.h
// ===================================================
EFI_HII_PACKAGE_LIST_HEADER *hii_database_package_list(UINT8 package_type) {
    EFI_HII_PACKAGE_LIST_HEADER *pkg_list = NULL;   // Return variable

    // Get HII databse protocol instance
    EFI_HII_DATABASE_PROTOCOL *dbp = NULL;
    EFI_GUID dbp_guid = EFI_HII_DATABASE_PROTOCOL_GUID;
    EFI_STATUS status = EFI_SUCCESS;

    status = bs->LocateProtocol(&dbp_guid, NULL, (VOID **)&dbp);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate HII Database Protocol!\r\n");
        return pkg_list;
    }

    // Get size of buffer needed for list of handles for package lists
    UINTN buf_len = 0;
    EFI_HII_HANDLE handle_buf = NULL;   
    status = dbp->ListPackageLists(dbp, package_type, NULL, &buf_len, &handle_buf); 
    if (status != EFI_BUFFER_TOO_SMALL && EFI_ERROR(status)) {
        error(status, u"Could not get size of list of handles for HII package lists for type %hhu.\r\n",
              package_type);
        return pkg_list;
    }

    // Allocate buffer for list of handles for package lists
    status = bs->AllocatePool(EfiLoaderData, buf_len, (VOID **)&handle_buf);  
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for handle list for package lists type %hhu.\r\n",
              package_type);
        return pkg_list;
    }

    // Get list of handles with package type into buffer
    status = dbp->ListPackageLists(dbp, package_type, NULL, &buf_len, &handle_buf); 
    if (EFI_ERROR(status)) {
        error(status, u"Could not get list of handles for HII package lists for type %hhu into buffer.\r\n",
              package_type);
        goto cleanup;
    }

    // Get size of buffer needed for package list on handle
    UINTN buf_len2 = 0;
    EFI_HII_HANDLE handle = handle_buf;    // Point to 1st handle in handle list    
    status = dbp->ExportPackageLists(dbp, handle, &buf_len2, pkg_list);  
    if (status != EFI_BUFFER_TOO_SMALL && EFI_ERROR(status)) {
        error(status, u"Could not get size of 1st package list for type %hhu.\r\n",
              package_type);
        goto cleanup;
    }

    // Allocate buffer for package list to export (1st package list on 1st handle)
    status = bs->AllocatePool(EfiLoaderData, buf_len2, (VOID **)&pkg_list);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for package list of type %hhu.\r\n",
              package_type);
        goto cleanup;
    }

    // Export package list to buffer
    status = dbp->ExportPackageLists(dbp, handle, &buf_len2, pkg_list); 
    if (EFI_ERROR(status)) {
        error(status, u"Could not export package list of type %hhu into buffer.\r\n",
              package_type);
        goto cleanup;
    }

    // Free memory when done and return result
    cleanup:
    if (handle_buf) bs->FreePool(handle_buf);
    return pkg_list;    // Caller needs to free this with bs->FreePool()
}

// ==========================================
// Read a file from the basic data partition
// ==========================================
EFI_STATUS load_kernel(void) {
    VOID *file_buffer = NULL;
    VOID *disk_buffer = NULL;
    EFI_HII_PACKAGE_LIST_HEADER *pkg_list = NULL;   
    EFI_STATUS status = EFI_SUCCESS;

    // Print file info for DATAFLS.INF file from path "/EFI/BOOT/DATAFLS.INF"
    CHAR16 *file_name = u"\\EFI\\BOOT\\DATAFLS.INF";

    cout->ClearScreen(cout);

    UINTN buf_size = 0;
    file_buffer = read_esp_file_to_buffer(file_name, &buf_size);
    if (!file_buffer) {
        error(0, u"Could not find or read file '%s' to buffer\r\n", file_name);
        goto exit;
    }

    // Parse data from DATAFLS.INF file to get disk LBA and file size
    char *str_pos = NULL;
    str_pos = strstr(file_buffer, "kernel");
    if (!str_pos) {
        error(0, u"Could not find kernel file in data partition\r\n");
        goto cleanup;
    }

    printf(u"Found kernel file\r\n");

    str_pos = strstr(file_buffer, "FILE_SIZE=");
    if (!str_pos) {
        error(0, u"Could not find file size from buffer for '%s'\r\n", file_name);
        goto cleanup;
    }

    str_pos += strlen("FILE_SIZE=");
    UINTN file_size = atoi(str_pos);

    str_pos = strstr(file_buffer, "DISK_LBA=");
    if (!str_pos) {
        error(0, u"Could not find disk lba value from buffer for '%s'\r\n", file_name);
        goto cleanup;
    }

    str_pos += strlen("DISK_LBA=");
    UINTN disk_lba = atoi(str_pos);

    printf(u"File Size: %u, Disk LBA: %u\r\n", file_size, disk_lba);

    // Get media ID (disk number for Block IO protocol Media) for this running disk image
    UINT32 image_mediaID = 0;
    status = get_disk_image_mediaID(&image_mediaID);
    if (EFI_ERROR(status)) {
        error(status, u"Could not find or get MediaID value for disk image\r\n");
        bs->FreePool(file_buffer);  // Free memory allocated for ESP file
        goto exit;
    }

    // Read disk lbas for file into buffer
    disk_buffer = (VOID *)read_disk_lbas_to_buffer(disk_lba, file_size, image_mediaID, true);
    if (!disk_buffer) {
        error(0, u"Could not find or read data partition file to buffer\r\n");
        bs->FreePool(file_buffer);  // Free memory allocated for ESP file
        goto exit;
    }

    // Get GOP info for kernel parms
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate GOP! :(\r\n");
        return status;
    }

    // Initialize Kernel Parameters
    Kernel_Parms kparms = {0};  // Defined in efi_lib.h

    kparms.gop_mode = *gop->Mode;
    Entry_Point entry_point = NULL;

    // Load Kernel File depending on format (initial header bytes)
    UINT8 *hdr = disk_buffer;

    printf(u"File Format: ");
    printf(u"Header bytes: [%x][%x][%x][%x]\r\n", 
           (UINTN)hdr[0], (UINTN)hdr[1], (UINTN)hdr[2], (UINTN)hdr[3]);

    EFI_PHYSICAL_ADDRESS kernel_buffer = 0;
    UINTN kernel_size = 0;

    // Get around compiler warning about function vs void pointer
    if (!memcmp(hdr, (UINT8[4]){0x7F, 'E', 'L', 'F'}, 4)) {
        *(void **)&entry_point = load_elf(disk_buffer, &kernel_buffer, &kernel_size);   

    } else if (!memcmp(hdr, (UINT8[2]){'M', 'Z'}, 2)) {
        *(void **)&entry_point = load_pe(disk_buffer, &kernel_buffer, &kernel_size); 

    } else {
        printf(u"No format found, Assuming it's a flat binary file\r\n");
        // Flat binary executable code assumed to start at the beginning of the loaded buffer
        *(void **)&entry_point = disk_buffer;   
        kernel_buffer = (EFI_PHYSICAL_ADDRESS)disk_buffer;
        kernel_size = file_size;
    }

    // Get new higher address kernel entry point to use
    UINTN entry_offset = (UINTN)entry_point - kernel_buffer;
    Entry_Point higher_entry_point = (Entry_Point)(KERNEL_START_ADDRESS + entry_offset);

    // Print info for loaded kernel 
    printf(u"\r\nOriginal Kernel address: %x, size: %u, entry point: %x\r\n"
           u"Higher address entry point: %x\r\n",
            kernel_buffer, kernel_size, (UINTN)entry_point, higher_entry_point);

    if (!entry_point) {   
        // Clean up/free pages for allocated kernel buffer
        bs->FreePages(kernel_buffer, kernel_size / PAGE_SIZE);
        goto cleanup;     
    }

    printf(u"\r\nPress any key to load kernel...\r\n");
    get_key();

    // Close Timer Event so that it does not continue to fire off
    bs->CloseEvent(timer_event);

    // Set rest of kernel parameters
    kparms.RuntimeServices = rs;
    kparms.NumberOfTableEntries = st->NumberOfTableEntries;
    kparms.ConfigurationTable = st->ConfigurationTable;

    // Allocate buffer for kernel bitmap fonts
    kparms.num_fonts = 2;
    status = bs->AllocatePool(EfiLoaderData, 
                              kparms.num_fonts * sizeof *kparms.fonts, 
                              (VOID **)&kparms.fonts);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for kernel bitmap font parms.\r\n");
        goto cleanup;
    }

    // Get simple font info & glyphs from HII database for kernel to use as a bitmap font 
    //   for printing
    pkg_list = hii_database_package_list(EFI_HII_PACKAGE_SIMPLE_FONTS);    
    if (pkg_list) {
        // Fill in kernel parm font with narrow glyph info from EFI HII simple font (8x19)
        EFI_HII_SIMPLE_FONT_PACKAGE_HDR *simple_font_hdr = 
          (EFI_HII_SIMPLE_FONT_PACKAGE_HDR *)(pkg_list + 1);

        // Fill out bitmap font info
        kparms.fonts[0] = (Bitmap_Font){
            .name = "efi_system_narrow_01",
            .width = EFI_GLYPH_WIDTH,
            .height = EFI_GLYPH_HEIGHT,
            .num_glyphs = simple_font_hdr->NumberOfNarrowGlyphs,
            .glyphs = NULL,
            .left_col_first = false,    // Bits in memory are laid out right to left
        };

        // Allocate buffer for glyph data, try to have at least full ASCII + code page range
        UINTN max_glyphs = max(256, simple_font_hdr->NumberOfNarrowGlyphs);
        Bitmap_Font font = kparms.fonts[0];
        UINTN glyph_size = ((font.width + 7) / 8) * font.height; 

        // Allocate extra 8 bytes for bitmap mask printing in kernel
        status = bs->AllocatePool(EfiLoaderData, 
                                  (max_glyphs * glyph_size) + 8,    
                                  (VOID **)&kparms.fonts[0].glyphs);
        if (EFI_ERROR(status)) {
            error(status, u"Could not allocate buffer for kernel parm font narrow glyphs bitmaps.\r\n");
            goto cleanup;
        }

        // Copy narrow glyphs into buffer, start at lowest/first glyph to 0-init or skip others
        memset(kparms.fonts[0].glyphs, 0, max_glyphs * glyph_size);

        EFI_NARROW_GLYPH *narrow_glyphs = (EFI_NARROW_GLYPH *)(simple_font_hdr + 1);
        CHAR16 lowest_glyph = narrow_glyphs[0].UnicodeWeight;

        UINT8 *buf = kparms.fonts[0].glyphs + (lowest_glyph * glyph_size);
        for (UINTN i = 0; i < simple_font_hdr->NumberOfNarrowGlyphs; i++) {
            EFI_NARROW_GLYPH glyph = narrow_glyphs[i];
            memcpy(buf + (i * glyph_size), glyph.GlyphCol1, glyph_size);
        }
    }

    // Get "embedded" PSF file for another bitmap font to use
    // Assuming psf font is #embed-ed or #include-ed from e.g. xxd -i and variables are 
    //   defined for e.g.
    //   "unsigned char ter_132n_psf[] = {};" and "unsigned int ter_132n_psf_len;"
    PSF2_Header *psf2_hdr = (PSF2_Header *)ter_132n_psf;
    kparms.fonts[1] = (Bitmap_Font){
        .name            = "ter-132n.psf",
        .width           = psf2_hdr->width,
        .height          = psf2_hdr->height,
        .left_col_first  = true,                   // Pixels in memory are stored left to right
        .num_glyphs      = psf2_hdr->num_glyphs,
        .glyphs          = (uint8_t *)(psf2_hdr+1),
    };

    // Get Memory Map
    if (EFI_ERROR(get_memory_map(&kparms.mmap))) goto cleanup;

    // Exit boot services before calling kernel
    UINTN retries = 0;
    const UINTN MAX_RETRIES = 5;
    while (EFI_ERROR(bs->ExitBootServices(image, kparms.mmap.key)) && retries < MAX_RETRIES) {
        // firmware could do a partial shutdown, need to get memory map again
        //   and try exit boot services again 
        bs->FreePool(kparms.mmap.map);
        if (EFI_ERROR(get_memory_map(&kparms.mmap))) goto cleanup;
        retries++;
    }
    if (retries == MAX_RETRIES) {
        error(0, u"Could not Exit Boot Services!\r\n");
        goto cleanup;
    }

    // Set up new level 4 page table
    pml4 = mmap_allocate_pages(&kparms.mmap, 1);
    memset(pml4, 0, sizeof *pml4);  

    // Initialize new paging setup by identity mapping all available memory 
    identity_map_efi_mmap(&kparms.mmap);

    // Identity map runtime services memory & set new runtime address map
    set_runtime_address_map(&kparms.mmap);

    // Remap kernel to higher addresses
    for (UINTN i = 0; i < (kernel_size + (PAGE_SIZE-1)) / PAGE_SIZE; i++) 
        map_page(kernel_buffer + (i*PAGE_SIZE), KERNEL_START_ADDRESS + (i*PAGE_SIZE), &kparms.mmap); 

    // NOTE: Remap kparms to higher address?

    // Identity map framebuffer
    for (UINTN i = 0; i < (kparms.gop_mode.FrameBufferSize + (PAGE_SIZE-1)) / PAGE_SIZE; i++) 
        identity_map_page(kparms.gop_mode.FrameBufferBase + (i*PAGE_SIZE), &kparms.mmap); 

    // Identity map new stack for kernel
    const UINTN STACK_PAGES = 16;   
    void *kernel_stack = mmap_allocate_pages(&kparms.mmap, STACK_PAGES);   // 64KiB stack
    memset(kernel_stack, 0, STACK_PAGES*PAGE_SIZE); // Initialize stack memory

    for (UINTN i = 0; i < STACK_PAGES; i++) 
        identity_map_page((UINTN)kernel_stack + (i*PAGE_SIZE), &kparms.mmap); 

    // Set up new GDT & TSS
    TSS tss = {.io_map_base = sizeof(TSS)}; // All bits after limit (in TSS descriptor) assumed to be '1'
    UINTN tss_address = (UINTN)&tss;

    GDT gdt = {
        .null.value           = 0x0000000000000000, // Null descriptor

        .kernel_code_64.value = 0x00AF9A000000FFFF,
        .kernel_data_64.value = 0x00CF92000000FFFF,

        .user_code_64.value   = 0x00AFFA000000FFFF,
        .user_data_64.value   = 0x00CFF2000000FFFF,

        .kernel_code_32.value = 0x00CF9A000000FFFF,
        .kernel_data_32.value = 0x00CF92000000FFFF,

        .user_code_32.value   = 0x00CFFA000000FFFF,
        .user_data_32.value   = 0x00CFF2000000FFFF,

        .tss = {
            .descriptor = {
                .limit_15_0 = sizeof tss - 1,
                .base_15_0  = tss_address & 0xFFFF, 
                .base_23_16 = (tss_address >> 16) & 0xFF, 
                .type       = 9,    // 0b1001 64 bit TSS (available)
                .p          = 1,    // Present
                .base_31_24 = (tss_address >> 24) & 0xFF,
            },
            .base_63_32 = (tss_address >> 32) & 0xFFFFFFFF,
        }
    };

    Descriptor_Register gdtr = {.limit = sizeof gdt - 1, .base = (UINT64)&gdt}; 

    // Get pointer to kernel for RCX as first parameter for x86_64 MS ABI
    Kernel_Parms *kparms_ptr = &kparms; 

    // Set new page tables (CR3 = PML4) and GDT (lgdt && ltr), and call entry point with parms
    __asm__ __volatile__(
        "cli\n"                     // Clear interrupts before setting new GDT/TSS, etc.
        "movq %[pml4], %%CR3\n"     // Load new page tables
        "lgdt %[gdt]\n"             // Load new GDT from gdtr register
        "ltr %[tss]\n"              // Load new task register with new TSS value (byte offset into GDT)

        // Jump to new code segment in GDT (offset in GDT of 64 bit kernel/system code segment)
        "pushq $0x8\n"
        "leaq 1f(%%RIP), %%RAX\n"
        "pushq %%RAX\n"
        "lretq\n"

        // Executing code with new Code segment now, set up remaining segment registers
        "1:\n"
        "movq $0x10, %%RAX\n"   // Data segment to use (64 bit kernel data segment, offset in GDT)
        "movq %%RAX, %%DS\n"    // Data segment
        "movq %%RAX, %%ES\n"    // Extra segment
        "movq %%RAX, %%FS\n"    // Extra segment (2), these also have different uses in Long Mode
        "movq %%RAX, %%GS\n"    // Extra segment (3), these also have different uses in Long Mode
        "movq %%RAX, %%SS\n"    // Stack segment

        // Set new stack value to use (for SP/stack pointer, etc.)
        "movq %[stack], %%RSP\n"

        // Call new entry point in higher memory
        "callq *%[entry]\n" // First parameter is kparms_ptr in RCX in input constraints below, for MS ABI
    :
    :   [pml4]"r"(pml4), [gdt]"m"(gdtr), [tss]"r"((UINT16)0x48),
        [stack]"gm"((UINTN)kernel_stack + (STACK_PAGES * PAGE_SIZE)),    // Top of newly allocated stack
        [entry]"r"(higher_entry_point), "c"(kparms_ptr)
    : "rax", "memory");

    // Should not return to this point!
    __builtin_unreachable();

    // Final cleanup
    cleanup:
    bs->FreePool(file_buffer);  // Free memory allocated for ESP file
    bs->FreePool(disk_buffer);  // Free memory allocated for data partition file

    if (pkg_list) bs->FreePool(pkg_list);   // Free memory for simple font package list

    for (UINTN i = 0; i < kparms.num_fonts; i++)    // Free memory for kparms font glyphs
        bs->FreePool(kparms.fonts[i].glyphs);

    if (kparms.fonts) bs->FreePool(kparms.fonts);   // Free memory for kparms fonts array

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

        // Pause if reached bottom of screen
        if (cout->Mode->CursorRow >= text_rows-2) {
            printf(u"Press any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }

    printf(u"\r\nUsable memory: %u / %u MiB / %u GiB\r\n",
            usable_bytes, usable_bytes / (1024 * 1024), usable_bytes / (1024 * 1024 * 1024));

    // Free allocated buffer for memory map
    bs->FreePool(mmap.map);

    printf(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ====================
// Print a GUID value
// ====================
void print_guid(EFI_GUID guid) { 
    UINT8 *p = (UINT8 *)&guid;
    printf(u"{%x,%x,%x,%x,%x,{%x,%x,%x,%x,%x,%x}}\r\n",
            *(UINT32 *)&p[0], *(UINT16 *)&p[4], *(UINT16 *)&p[6], 
            (UINTN)p[8], (UINTN)p[9], (UINTN)p[10], (UINTN)p[11], (UINTN)p[12], (UINTN)p[13], 
            (UINTN)p[14], (UINTN)p[15]);
}

// =======================================
// Print configuration table GUID values
// =======================================
EFI_STATUS print_config_tables(void) { 
    cout->ClearScreen(cout);

    // Close Timer Event for cleanup
    bs->CloseEvent(timer_event);

    printf(u"Configuration Table GUIDs:\r\n");
    for (UINTN i = 0; i < st->NumberOfTableEntries; i++) {
        EFI_GUID guid = st->ConfigurationTable[i].VendorGuid;
        print_guid(guid);

        // Print GUID name if available, else unknown
        UINTN j = 0;
        bool found = false;
        for (; j < ARRAY_SIZE(config_table_guids_and_strings); j++) {
            if (!memcmp(&guid, &config_table_guids_and_strings[j].guid, sizeof guid)) {
                found = true;
                break;
            }
        }
        printf(u"(%s)\r\n\r\n", 
               found ? config_table_guids_and_strings[j].string : u"Unknown GUID Value");

        // Pause every so often
        if (i > 0 && i % 6 == 0) get_key();
    }

    printf(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ===========================================
// Get specific config table pointer by GUID
// ===========================================
VOID *get_config_table_by_guid(EFI_GUID guid) { 
    for (UINTN i = 0; i < st->NumberOfTableEntries; i++) {
        EFI_GUID vendor_guid = st->ConfigurationTable[i].VendorGuid;

        if (!memcmp(&vendor_guid, &guid, sizeof guid)) 
            return st->ConfigurationTable[i].VendorTable;
    }
    return NULL;    // Did not find config table
}

// =========================
// Print ACPI table header
// =========================
void print_acpi_table_header(ACPI_TABLE_HEADER header) { 
    printf(u"Signature: %.4hhs\r\n"
           u"Length: %u\r\n"
           u"Revision: %#x\r\n"
           u"Checksum: %u\r\n"
           u"OEMID: %.6hhs\r\n"
           u"OEM Table ID: %.8hhs\r\n"
           u"OEM Revision: %#x\r\n"
           u"Creator ID: %.4hhs\r\n"
           u"Creator Revision: %#x\r\n",
           &header.signature[0],
           (UINTN)header.length,
           (UINTN)header.revision,
           (UINTN)header.checksum,
           &header.OEMID[0], 
           &header.OEM_table_id[0],
           (UINTN)header.OEM_revision,
           &header.creator_id[0], 
           (UINTN)header.creator_revision);
}

// =======================================
// Print configuration table GUID values
// =======================================
EFI_STATUS print_acpi_tables(void) { 
    cout->ClearScreen(cout);

    // Close Timer Event for cleanup
    bs->CloseEvent(timer_event);

    // Check for ACPI 2.0+ table
    EFI_GUID acpi_guid = EFI_ACPI_TABLE_GUID;
    VOID *rsdp_ptr = get_config_table_by_guid(acpi_guid);
    bool acpi_20 = false;
    if (!rsdp_ptr) {
        // Check for ACPI 1.0 table as fallback
        acpi_guid = (EFI_GUID)ACPI_TABLE_GUID;
        rsdp_ptr = get_config_table_by_guid(acpi_guid);
        if (!rsdp_ptr) {
            error(0, u"Could not find ACPI configuration table\r\n");
            return 1;
        } else {
            printf(u"ACPI 1.0 Table found at %#x\r\n", rsdp_ptr);
        }
    } else {
        printf(u"ACPI 2.0 Table found at %#x\r\n", rsdp_ptr);
        acpi_20 = true;
    }

    // Print RSDP
    UINT8 *rsdp = rsdp_ptr;
    if (acpi_20) {
        printf(u"RSDP:\r\n"
               u"Signature: %.8hhs\r\n"
               u"Checksum: %u\r\n"
               u"OEMID: %.6hhs\r\n"
               u"RSDT Address: %x\r\n"
               u"Length: %u\r\n"
               u"XSDT Address: %x\r\n"
               u"Extended Checksum: %u\r\n",
               &rsdp[0], 
               (UINTN)rsdp[8],
               &rsdp[9], 
               *(UINT32 *)&rsdp[16],
               *(UINT32 *)&rsdp[20],
               *(UINT64 *)&rsdp[24],
               (UINTN)rsdp[32]);
    } else {
        printf(u"RSDP:\r\n"
               u"Signature: %c%c%c%c%c%c%c%c\r\n"
               u"Checksum: %u\r\n"
               u"OEMID: %c%c%c%c%c%c\r\n"
               u"RSDT Address: %x\r\n",
               rsdp[0], rsdp[1], rsdp[2], rsdp[3], rsdp[4], rsdp[5], rsdp[6], rsdp[7],
               (UINTN)rsdp[8],
               rsdp[9], rsdp[10], rsdp[11], rsdp[12], rsdp[13], rsdp[14], 
               *(UINT32 *)&rsdp[16]);
    }

    printf(u"\r\nPress any key to print RSDT/XSDT...\r\n");
    get_key();

    // Uncomment this line to use RSDT instead of XSDT
    //acpi_20 = false;
    
    ACPI_TABLE_HEADER *header = NULL;
    if (acpi_20) {
        // Print XSDT header
        UINT64 xsdt_address = *(UINT64 *)&rsdp[24];
        header = (ACPI_TABLE_HEADER *)xsdt_address;
        print_acpi_table_header(*header);

        // Print XSDT entry signatures
        printf(u"\r\nPress any key to print entries...\r\n");
        get_key();

        cout->ClearScreen(cout);
        printf(u"Entries:\r\n");
        UINT64 *entry = (UINT64 *)((UINT8 *)header + sizeof *header); 
        for (UINTN i = 0; i < (header->length - sizeof *header) / 8; i++) {
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)entry[i];
            printf(u"%c%c%c%c\r\n",
                   table_header.signature[0], table_header.signature[1], table_header.signature[2], 
                       table_header.signature[3]);

            if (i > 0 && i % 23 == 0) get_key();
        }

        printf(u"\r\nPress any key to print next table...\r\n");
        get_key();

        // Loop and print each ACPI table
        for (UINTN i = 0; i < (header->length - sizeof *header) / 8; i++) {
            cout->ClearScreen(cout);

            // Print header
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)entry[i];
            print_acpi_table_header(table_header);

            // TODO: Print specific table info ?

            printf(u"\r\nPress any key to print next table...\r\n");
            get_key();
        }

    } else {
        // Print RSDT header
        UINT32 rsdt_address = *(UINT32 *)&rsdp[16];

        // The extra (UINTN) casts are to avoid compiler warnings about casting smaller 
        //   int types to pointer
        header = (ACPI_TABLE_HEADER *)(UINTN)rsdt_address;
        print_acpi_table_header(*header);

        // Print RSDT entry signatures
        printf(u"\r\nPress any key to print entries...\r\n");
        get_key();

        cout->ClearScreen(cout);
        printf(u"Entries:\r\n");
        UINT32 *entry = (UINT32 *)((UINT8 *)header + sizeof *header); 
        for (UINTN i = 0; i < (header->length - sizeof *header) / 4; i++) {
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)(UINTN)entry[i];
            printf(u"%c%c%c%c\r\n",
                   table_header.signature[0], table_header.signature[1], table_header.signature[2], 
                       table_header.signature[3]);

            if (i > 0 && i % 23 == 0) get_key();
        }

        printf(u"\r\nPress any key to print next table...\r\n");
        get_key();

        // Loop and print each ACPI table
        for (UINTN i = 0; i < (header->length - sizeof *header) / 4; i++) {
            cout->ClearScreen(cout);

            // Print header
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)(UINTN)entry[i];
            print_acpi_table_header(table_header);

            // TODO: Print specific table info ?

            printf(u"\r\nPress any key to print next table...\r\n");
            get_key();
        }
    }

    printf(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ================================
// Print all EFI Global Variables
// ================================
EFI_STATUS print_efi_global_variables(void) { 
    cout->ClearScreen(cout);

    // Close Timer Event for cleanup
    bs->CloseEvent(timer_event);

    UINTN var_name_size = 0;
    CHAR16 *var_name_buf = 0;
    EFI_GUID vendor_guid = {0};
    EFI_STATUS status = EFI_SUCCESS;

    var_name_size = 2;
    status = bs->AllocatePool(EfiLoaderData, var_name_size, (VOID **)&var_name_buf);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate 2 bytes...\r\n");
        return status;
    }

    // Set variable name to point to initial single null byte, to start off call to get list of
    //   variable names
    *var_name_buf = u'\0';

    status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
    while (status != EFI_NOT_FOUND) {   // End of list
        if (status == EFI_BUFFER_TOO_SMALL) {
            // Reallocate larger buffer for variable name
            CHAR16 *temp_buf = NULL;
            status = bs->AllocatePool(EfiLoaderData, var_name_size, (VOID **)&temp_buf);
            if (EFI_ERROR(status)) {
                error(status, u"Could not allocate %u bytes of memory for next variable name.\r\n",
                              var_name_size);
                return status;
            }
            
            strcpy_u16(temp_buf, var_name_buf);  // Copy old buffer to new buffer
            bs->FreePool(var_name_buf);          // Free old buffer
            var_name_buf = temp_buf;             // Set new buffer

            status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
            continue;
        }

        // Print variable name
        printf(u"%.*s\r\n", var_name_size, var_name_buf);

        // Pause at bottom of screen
        if (cout->Mode->CursorRow >= text_rows-2) {
            printf(u"Press any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
        status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
    }

    // Free buffer when done
    bs->FreePool(var_name_buf);

    printf(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ==========================================================
// Print Boot variable values and allow user to change them
// ==========================================================
EFI_STATUS change_boot_variables(void) { 
    // Close Timer Event for cleanup
    bs->CloseEvent(timer_event);

    // Get Device Path to Text protocol to print Load Option device/file paths
    EFI_STATUS status = EFI_SUCCESS;
    EFI_GUID dpttp_guid = EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID;
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *dpttp;
    status = bs->LocateProtocol(&dpttp_guid, NULL, (VOID **)&dpttp);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate Device Path To Text Protocol.\r\n");
        return status;
    }

    // Overall screen loop
    UINT32 boot_order_attributes = 0;
    while (true) {
        cout->ClearScreen(cout);

        UINTN var_name_size = 0;
        CHAR16 *var_name_buf = 0;
        EFI_GUID vendor_guid = {0};

        var_name_size = 2;
        status = bs->AllocatePool(EfiLoaderData, var_name_size, (VOID **)&var_name_buf);
        if (EFI_ERROR(status)) {
            error(status, u"Could not allocate 2 bytes...\r\n");
            return status;
        }

        // Set variable name to point to initial single null byte, to start off call to get list of
        //   variable names
        *var_name_buf = u'\0';

        status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
        while (status != EFI_NOT_FOUND) {   // End of list
            if (status == EFI_BUFFER_TOO_SMALL) {
                // Reallocate larger buffer for variable name
                CHAR16 *temp_buf = NULL;
                status = bs->AllocatePool(EfiLoaderData, var_name_size, (VOID **)&temp_buf);
                if (EFI_ERROR(status)) {
                    error(status, u"Could not allocate %u bytes of memory for next variable name.\r\n",
                                  var_name_size);
                    return status;
                }
                
                strcpy_u16(temp_buf, var_name_buf);  // Copy old buffer to new buffer
                bs->FreePool(var_name_buf);          // Free old buffer
                var_name_buf = temp_buf;             // Set new buffer

                status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
                continue;
            }

            // Print variable name and their value(s)
            if (!memcmp(var_name_buf, u"Boot", 8)) {
                printf(u"\r\n%.*s: ", var_name_size, var_name_buf);

                // Get variable value
                UINT32 attributes = 0;
                UINTN data_size = 0;
                VOID *data = NULL;

                // Call first with 0 data size to get actual size needed
                rs->GetVariable(var_name_buf, &vendor_guid, &attributes, &data_size, NULL);

                status = bs->AllocatePool(EfiLoaderData, data_size, (VOID **)&data);
                if (EFI_ERROR(status)) {
                    error(status, u"Could not allocate %u bytes of memory for GetVariable().\r\n",
                                  data_size);
                    goto cleanup;
                }

                // Get actual data now with correct size
                rs->GetVariable(var_name_buf, &vendor_guid, &attributes, &data_size, data);
                if (data_size == 0) goto next;  // Skip this one if no data

                if (!memcmp(var_name_buf, u"BootOrder", 18)) {
                    boot_order_attributes = attributes; // Use if user sets new BootOrder value

                    // Print array of UINT16 values
                    UINT16 *p = data;

                    for (UINTN i = 0; i < data_size / 2; i++)
                        printf(u"%#.4x,", *p++);   

                    printf(u"\r\n");
                    goto next;
                }

                if (!memcmp(var_name_buf, u"BootOptionSupport", 34)) {
                    // Single UINT32 value
                    UINT32 *p = data;
                    printf(u"%#.8x\r\n", *p);  
                    goto next;
                }

                if (!memcmp(var_name_buf, u"BootNext",    18) || 
                    !memcmp(var_name_buf, u"BootCurrent", 22)) {

                    // Single UINT16 value
                    UINT16 *p = data;
                    printf(u"%#.4hx\r\n", *p); 
                    goto next;
                }

                if (isxdigit_c16(var_name_buf[4]) && var_name_size == 18) {  
                    // Boot#### load option - Name size is 8 CHAR16 chars * 2 bytes + CHAR16 null byte
                    EFI_LOAD_OPTION *load_option = (EFI_LOAD_OPTION *)data;
                    CHAR16 *description = (CHAR16 *)((UINT8 *)data + sizeof(UINT32) + sizeof(UINT16));
                    printf(u"%s\r\n", description);    

                    CHAR16 *p = description;
                    UINTN strlen =  0;
                    while (p[strlen]) strlen++;  
                    strlen++;                    // Skip null byte

                    EFI_DEVICE_PATH_PROTOCOL *file_path_list = 
                        (EFI_DEVICE_PATH_PROTOCOL *)(description + strlen); 

                    CHAR16 *device_path_text = 
                        dpttp->ConvertDevicePathToText(file_path_list, FALSE, FALSE);

                    printf(u"Device Path: %s\r\n", device_path_text ? device_path_text : u"(null)");

                    UINT8 *optional_data = (UINT8 *)file_path_list + load_option->FilePathListLength;
                    UINTN optional_data_size = data_size - (optional_data - (UINT8 *)data);
                    if (optional_data_size > 0) {
                        printf(u"Optional Data: 0x");
                        for (UINTN i = 0; i < optional_data_size; i++)
                            printf(u"%.2hhx", optional_data[i]);

                        printf(u"\r\n"); 
                    }
                    
                    goto next; 
                }

                printf(u"\r\n");  // Unhandled Boot* variable, go on with space before next one

                next:
                bs->FreePool(data);
            }

            // Pause at bottom of screen
            if (cout->Mode->CursorRow >= text_rows-2) {
                printf(u"Press any key to continue...\r\n");
                get_key();
                cout->ClearScreen(cout);
            }
            status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
        }

        // Allow user to change values
        printf(u"Press '1' to change BootOrder, '2' to change BootNext, or other to go back...");
        EFI_INPUT_KEY key = get_key();
        if (key.UnicodeChar == u'1') {
            // Change BootOrder - set new array of UINT16 values
            #define MAX_BOOT_OPTIONS 10
            UINT16 option_array[MAX_BOOT_OPTIONS] = {0};
            UINTN new_option = 0;
            UINT16 num_options = 0;
            for (UINTN i = 0; i < MAX_BOOT_OPTIONS; i++) {
                printf(u"\r\nBoot Option %u (0000-FFFF): ", i+1);
                if (!get_hex(&new_option)) break;    // Stop processing
                option_array[num_options++] = new_option; 
            }

            EFI_GUID guid = EFI_GLOBAL_VARIABLE_GUID;
            status = rs->SetVariable(u"BootOrder", 
                                     &guid,
                                     boot_order_attributes, 
                                     num_options*2, 
                                     option_array);
            if (EFI_ERROR(status)) 
                error(status, u"Could not Set new value for BootOrder.\r\n");

        } else if (key.UnicodeChar == u'2') {
            // Change BootNext value - set new UINT16
            printf(u"\r\nBootNext value (0000-FFFF): ");
            UINTN value = 0;
            if (get_hex(&value)) {
                EFI_GUID guid = EFI_GLOBAL_VARIABLE_GUID;
                UINT32 attr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS;

                status = rs->SetVariable(u"BootNext", 
                                         &guid,
                                         attr, 
                                         2, 
                                         &value);
                if (EFI_ERROR(status)) 
                    error(status, u"Could not Set new value for BootNext.\r\n");
            }

        } else {
            bs->FreePool(var_name_buf);
            break;
        }

        cleanup:
        // Free buffers when done
        bs->FreePool(var_name_buf);
    }

    return EFI_SUCCESS;
}

// ==========================================================
// Print Boot variable values and allow user to change them
// ==========================================================
EFI_STATUS write_to_another_disk(void) { 
    EFI_STATUS status = EFI_SUCCESS;
    EFI_GUID bio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_BLOCK_IO_PROTOCOL *biop;
    UINTN num_handles = 0;
    EFI_HANDLE *handle_buffer = NULL;
    EFI_BLOCK_IO_PROTOCOL *disk_image_bio = NULL, *chosen_disk_bio = NULL;

    cout->ClearScreen(cout);

    // Get media ID for this disk image first, to compare to others in output
    UINT32 disk_image_media_id = 0;
    status = get_disk_image_mediaID(&disk_image_media_id);
    if (EFI_ERROR(status)) {
        error(status, u"Could not get Disk Image Media ID.\r\n");
        return status;
    }

    // Get size of disk image from file 
    CHAR16 *file_name = u"\\EFI\\BOOT\\DSKIMG.INF";
    UINTN buf_size = 0;
    VOID *file_buffer = NULL;
    file_buffer = read_esp_file_to_buffer(file_name, &buf_size);
    if (!file_buffer) {
        error(0, u"Could not find or read file '%s' to buffer\r\n", file_name);
        return 1;
    }

    char *str_pos = NULL;
    str_pos = strstr(file_buffer, "DISK_SIZE=");
    if (!str_pos) {
        error(0, u"Could not find disk image size in DSKIMG.INF\r\n");
        return 1;
    }

    str_pos += strlen("DISK_SIZE=");
    UINTN disk_image_size = atoi(str_pos);

    // Free file buffer when done
    bs->FreePool(file_buffer);

    // Loop through and print all full disk Block IO protocol Media 
    status = bs->LocateHandleBuffer(ByProtocol, &bio_guid, NULL, &num_handles, &handle_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate any Block IO Protocols.\r\n");
        return status;
    }

    UINT32 last_media_id = -1;  // Keep track of currently opened Media info
    for (UINTN i = 0; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &bio_guid,
                                  (VOID **)&biop,
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);  // Don't have to use CloseProtocol()
        if (EFI_ERROR(status)) {
            error(status, u"Could not Get Block IO protocol on handle %u.\r\n", i);
            continue;
        }

        if (biop->Media->LastBlock == 0 || biop->Media->LogicalPartition ||
            !biop->Media->MediaPresent  || biop->Media->ReadOnly) {
            // Only care about partitions/disks above 1 block in size, 
            // Block IOs for the "whole" disk (not a logical partition), 
            // Media that is currently present,
            // And media that can be written to
            continue;
        }

        // Print Block IO Media Info for this Disk/partition
        if (last_media_id != biop->Media->MediaId) {
            last_media_id = biop->Media->MediaId;   
            printf(u"Media ID: %u %s\r\n", 
                   last_media_id, 
                   (last_media_id == disk_image_media_id ? u"(Disk Image)" : u""));

            if (last_media_id == disk_image_media_id) 
                disk_image_bio = biop; // Save for later
        }

        // Get disk size in bytes, add 1 block for 0-based indexing fun
        UINTN size = (biop->Media->LastBlock+1) * biop->Media->BlockSize; 

        printf(u"Rmv: %s, Size: %llu/%llu MiB/%llu GiB\r\n",
               biop->Media->RemovableMedia ? u"Yes" : u"No",
               size, size / (1024 * 1024), size / (1024 * 1024 * 1024));

        if (biop->Media->MediaId == disk_image_media_id) {
            printf(u"Disk image size: %llu/%llu MiB/%llu GiB\r\n",
                   disk_image_size, 
                   disk_image_size / (1024 * 1024), 
                   disk_image_size / (1024 * 1024 * 1024));
        }
        printf(u"\r\n");
    }

    // Take in a number from the user for the media to write the disk image to
    printf(u"Input Media ID number to write to and press enter: ");
    INTN chosen_media = 0;
    get_int(&chosen_media);
    printf(u"\r\n");

    // Get Block IO for chosen disk media
    bool found = false;
    for (UINTN i = 0; i < num_handles; i++) {
        status = bs->OpenProtocol(handle_buffer[i], 
                                  &bio_guid,
                                  (VOID **)&biop,
                                  image,
                                  NULL,
                                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);  // Don't have to use CloseProtocol()
        if (EFI_ERROR(status)) {
            error(status, u"Could not Get Block IO protocol on handle %u.\r\n", i);
            continue;
        }

        if (biop->Media->MediaId == chosen_media) {
            chosen_disk_bio = biop;
            found = true;
            break;
        }
    }

    if (!found) {
        error(0, u"Could not find media with ID %u\r\n", chosen_media);
        return 1;
    }

    // Print info about chosen disk and disk image
    // block size for from and to disks
    UINTN from_block_size = disk_image_bio->Media->BlockSize, 
          to_block_size = chosen_disk_bio->Media->BlockSize;

    UINTN from_blocks = (disk_image_size + (from_block_size-1)) / from_block_size;
    UINTN to_blocks = (disk_image_size + (to_block_size-1)) / to_block_size;

    printf(u"From block size: %u, To block size: %u\r\n"
           u"From blocks: %u, To blocks: %u\r\n",
           from_block_size, to_block_size,
           from_blocks, to_blocks);

    // Allocate buffer to hold copy of disk image
    VOID *image_buffer = NULL;
    status = bs->AllocatePool(EfiLoaderData, disk_image_size, &image_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate memory for disk image.\r\n");
        return status;
    }

    // Read Blocks from disk image media to buffer
    printf(u"Reading %u blocks from disk image disk to buffer...\r\n", from_blocks);
    status = disk_image_bio->ReadBlocks(disk_image_bio,
                                        disk_image_media_id,
                                        0,
                                        from_blocks * from_block_size,
                                        image_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not read blocks from disk image media to buffer.\r\n");
        return status;
    }

    // Write Blocks from buffer to chosen media disk 
    printf(u"Writing %u blocks from buffer to chosen disk...\r\n", to_blocks);
    status = chosen_disk_bio->WriteBlocks(chosen_disk_bio,
                                          chosen_media,
                                          0,
                                          to_blocks * to_block_size,
                                          image_buffer);
    if (EFI_ERROR(status)) {
        error(status, u"Could not write blocks from buffer to chosen disk.\r\n");
        return status;
    }

    // Cleanup
    bs->FreePool(image_buffer);

    printf(u"\r\nDisk Image written to chosen disk.\r\n"
           u"Reboot and choose new boot option when able.\r\n");

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
        u"Print Configuration Tables",
        u"Print ACPI Tables",
        u"Print EFI Global Variables",
        u"Load Kernel",
        u"Change Boot Variables",
        u"Write Disk Image Image To Other Disk",
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
        print_acpi_tables,
        print_efi_global_variables,
        load_kernel,
        change_boot_variables,
        write_to_another_disk
        // install
    };

    // Connect all controllers found for all handles, to hopefully fix
    //   any bugs related to not initializing device drivers from firmware
    // * Code adapted from UEFI Spec 2.10 Errata A section 7.3.12 Examples
    //
    // Retrieve the list of all handles from the handle database
    EFI_STATUS Status;
    UINTN HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;
    UINTN HandleIndex = 0;

    Status = bs->LocateHandleBuffer(AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);
    if (!EFI_ERROR(Status)) {
        for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) 
            Status = bs->ConnectController(HandleBuffer[HandleIndex], NULL, NULL, TRUE);

        bs->FreePool(HandleBuffer);
    }

    // Screen loop
    bool running = true;
    while (running) {
        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(cout);

        // Get current text mode ColsxRows values
        UINTN cols = 0, rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

        // Set global rows/cols values
        text_rows = rows; 
        text_cols = cols;

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
                    // De-highlight current row, move up 1 row, highlight new row
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    printf(u"%s\r", menu_choices[current_row]);

                    if (current_row-1 >= min_row)  
                        current_row--;          // Go up one row
                    else
                        current_row = max_row;  // Wrap around to bottom of menu 

                    cout->SetCursorPosition(cout, 0, current_row);
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                    printf(u"%s\r", menu_choices[current_row]);

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                case SCANCODE_DOWN_ARROW:
                    // De-highlight current row, move down 1 row, highlight new row
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    printf(u"%s\r", menu_choices[current_row]);

                    if (current_row+1 <= max_row) 
                        current_row++;          // Go down one row
                    else
                        current_row = min_row;  // Wrap around to top of menu

                    cout->SetCursorPosition(cout, 0, current_row);
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                    printf(u"%s\r", menu_choices[current_row]);

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
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
                            error(return_status, u"Press any key to go back...");
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
