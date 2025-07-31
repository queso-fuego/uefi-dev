#include <stdarg.h>

#include "efi.h"
#include "efi_lib.h"

#define arch_header <arch/ARCH/ARCH.h>
#include arch_header

// -----------------
// Global constants
// -----------------
#define DEFAULT_FG_COLOR EFI_YELLOW
#define DEFAULT_BG_COLOR EFI_BLUE

#define HIGHLIGHT_FG_COLOR EFI_BLUE
#define HIGHLIGHT_BG_COLOR EFI_CYAN

// EFI_GRAPHICS_OUTPUT_BLT_PIXEL values, BGRr8888
#define px_LGRAY {0xEE,0xEE,0xEE,0x00}
#define px_BLACK {0x00,0x00,0x00,0x00}
#define px_BLUE  {0x98,0x00,0x00,0x00}  // EFI_BLUE

#define PAGE_SIZE 4096  // 4KiB

// Kernel start address in higher memory (64-bit) - last 2 GiBs of virtual memory
#define KERNEL_START_ADDRESS 0xFFFFFFFF80000000

#ifdef __clang__
int _fltused = 0;   // If using floating point code & lld-link, need to define this
#endif

// -----------------
// Global variables
// -----------------
// These external global vars are defined in efi_lib.h
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout;   // Console output
extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin;    // Console input
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr;   // Console output - stderr
extern EFI_BOOT_SERVICES    *bs;                // Boot services
extern EFI_RUNTIME_SERVICES *rs;                // Runtime services
extern EFI_SYSTEM_TABLE     *st;                // System Table
extern EFI_HANDLE image;                        // Image handle

extern INT32 text_rows, text_cols;              // Current text screen size rows & columns

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

bool autoload_kernel = false;   // Autoload kernel instead of main menu?

// ====================
// Set Text Mode
// ====================
EFI_STATUS set_text_mode(void) {
    // Store found Text mode info
    typedef struct {
	INTN  mode;
        UINTN cols;
        UINTN rows;
    } Text_Mode_Info;

    Text_Mode_Info text_modes[20];

    UINTN mode_index = 0;   // Current mode within entire menu of text mode choices 

    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Get current text mode info
        UINTN max_cols = 0, max_rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);

        printf_c16(u"Text mode information:\r\n"
                   u"Max Mode: %d\r\n"
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

        printf_c16(u"Available text modes:\r\n");

        UINTN menu_top = cout->Mode->CursorRow;

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, max_rows-3);
        printf_c16(u"Up/Down Arrow = Move Cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Go Back");

        UINTN menu_bottom = max_rows-5;	// Stop above keybind text (0-based offset)

        // Get all valid text modes' info
	// NOTE: Max valid GOP mode is ModeMax-1 per UEFI spec
        UINT32 max = cout->Mode->MaxMode;
	if (max-1 < menu_bottom - menu_top) menu_bottom = menu_top + max-1;

	UINT32 num_modes = 0;
        for (UINT32 i = 0; i < ARRAY_SIZE(text_modes) && i < max; i++) {
	    // If mode is bad or rows/cols are invalid, go on
            if (cout->QueryMode(cout, i, &text_modes[num_modes].cols, &text_modes[num_modes].rows) != EFI_SUCCESS || 
	        ((text_modes[num_modes].cols < 10 || text_modes[num_modes].cols > 999) || 
	         (text_modes[num_modes].rows < 10 || text_modes[num_modes].rows > 999))) {
		continue;
	    }
	    text_modes[num_modes++].mode = i;
	}

	if (num_modes-1 < menu_bottom - menu_top) menu_bottom = menu_top + num_modes-1;
	UINTN menu_len = menu_bottom - menu_top + 1;	// 1-based offset

        // Highlight top menu row to start off
        cout->SetCursorPosition(cout, 0, menu_top);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf_c16(u"Mode %d: %llux%llu", 
		   text_modes[0].mode, text_modes[0].cols, text_modes[0].rows);

        // Print other text mode infos
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINT32 i = 1; i < menu_len; i++) 
            printf_c16(u"\r\nMode %d: %llux%llu", 
		       text_modes[i].mode, text_modes[i].cols, text_modes[i].rows);

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
                        printf_c16(u"                    \r");  // Blank out mode text first

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        mode_index--;
                        printf_c16(u"Mode %d: %dx%d", 
                                   text_modes[mode_index].mode, 
				   text_modes[mode_index].cols, text_modes[mode_index].rows);

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        UINTN temp_mode = mode_index + 1;
                        for (UINT32 i = 0; i < menu_len; i++, temp_mode++) {
                            printf_c16(u"\r\n                    \r"  // Blank out mode text first
                                       u"Mode %d: %dx%d\r", 
                                       text_modes[temp_mode].mode, 
				       text_modes[temp_mode].cols, text_modes[temp_mode].rows);
                        }

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                    } else if (current_row-1 >= menu_top) {
                        // De-highlight current row, move up 1 row, highlight new row
                        printf_c16(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
                                   text_modes[mode_index].mode, 
			           text_modes[mode_index].cols, text_modes[mode_index].rows);

                        mode_index--;
                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf_c16(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
				   text_modes[mode_index].mode, 
				   text_modes[mode_index].cols, text_modes[mode_index].rows);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                case SCANCODE_DOWN_ARROW:
                    if (current_row == menu_bottom && mode_index < num_modes-1) {
                        // Not at bottom of modes yet, scroll menu down by incrementing all modes by 1
                        mode_index -= menu_len - 1;

                        // Print modes up until the last menu row
                        cout->SetCursorPosition(cout, 0, menu_top);
                        for (UINT32 i = 0; i < menu_len; i++, mode_index++) {
                            printf_c16(u"                    \r"    // Blank out mode text first
                                       u"Mode %d: %dx%d\r\n", 
				       text_modes[mode_index].mode, 
				       text_modes[mode_index].cols, text_modes[mode_index].rows);
                        }

                        // Highlight last row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf_c16(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
				   text_modes[mode_index].mode, 
				   text_modes[mode_index].cols, text_modes[mode_index].rows);

                    } else if (current_row+1 <= menu_bottom) {
                        // De-highlight current row, move down 1 row, highlight new row
                        printf_c16(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r\n", 
				   text_modes[mode_index].mode, 
				   text_modes[mode_index].cols, text_modes[mode_index].rows);

                        mode_index++;
                        current_row++;
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf_c16(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
				   text_modes[mode_index].mode, 
				   text_modes[mode_index].cols, text_modes[mode_index].rows);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                default:
                    if (key.UnicodeChar == u'\r' && text_modes[mode_index].cols != 0) {	// Qemu can have invalid text modes
                        // Enter key, set Text mode
                        cout->SetMode(cout, text_modes[mode_index].mode);
                        cout->QueryMode(cout, text_modes[mode_index].mode, 
					&text_modes[mode_index].cols, &text_modes[mode_index].rows);

                        // Set global rows/cols values
                        text_rows = text_modes[mode_index].rows;
                        text_cols = text_modes[mode_index].cols;

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

        // Get current GOP mode information
        printf_c16(u"Graphics mode information:\r\n");
        status = gop->QueryMode(gop, 
                                gop->Mode->Mode, 
                                &mode_info_size, 
                                &mode_info);

        if (EFI_ERROR(status)) {
            error(status, u"Could not Query GOP Mode %u\r\n", gop->Mode->Mode);
            return status;
        }

        printf_c16(u"Max Mode: %d\r\n"
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
        printf_c16(u"Up/Down Arrow = Move Cursor\r\n"
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
        printf_c16(u"Mode %d: %dx%d", 0, gop_modes[0].width, gop_modes[0].height);

        // Print other text mode infos
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINT32 i = 1; i < menu_len + 1; i++) 
            printf_c16(u"\r\nMode %d: %dx%d", i, gop_modes[i].width, gop_modes[i].height);

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
                        printf_c16(u"                    \r");  // Blank out mode text first

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        mode_index--;
                        printf_c16(u"Mode %d: %dx%d", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        UINTN temp_mode = mode_index + 1;
                        for (UINT32 i = 0; i < menu_len; i++, temp_mode++) {
                            printf_c16(u"\r\n                    \r"  // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
                                   temp_mode, gop_modes[temp_mode].width, gop_modes[temp_mode].height);
                        }

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                    } else if (current_row-1 >= menu_top) {
                        // De-highlight current row, move up 1 row, highlight new row
                        printf_c16(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        mode_index--;
                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf_c16(u"                    \r"    // Blank out mode text first
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
                            printf_c16(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r\n", 
                                   mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                        }

                        // Highlight last row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf_c16(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                    } else if (current_row+1 <= menu_bottom) {
                        // De-highlight current row, move down 1 row, highlight new row
                        printf_c16(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r\n", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        mode_index++;
                        current_row++;
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf_c16(u"                    \r"    // Blank out mode text first
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

                        // Clear GOP screen 
                        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px = px_BLUE;
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
    UINTN spp_handles = 0, app_handles = 0;
    EFI_HANDLE *spp_handle_buf = NULL, *app_handle_buf = NULL;
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

    cout->ClearScreen(cout);
    BOOLEAN found_mode = FALSE;

    // Use LocateHandleBuffer() to find all SPPs 
    status = bs->LocateHandleBuffer(ByProtocol, &spp_guid, NULL, &spp_handles, &spp_handle_buf);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate Simple Pointer Protocol handle buffer.\r\n");
        goto get_app;
    }

    // Open all SPP protocols for each handle
    for (UINTN i = 0; i < spp_handles; i++) {
        status = bs->OpenProtocol(spp_handle_buf[i], 
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
        printf_c16(u"SPP %u; Resolution X: %u, Y: %u, Z: %u, LButton: %u, RButton: %u\r\n",
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

    // Use LocateHandleBuffer() to find all APPs 
    get_app:
    found_mode = FALSE;

    status = bs->LocateHandleBuffer(ByProtocol, &app_guid, NULL, &app_handles, &app_handle_buf);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate Absolute Pointer Protocol handle buffer.\r\n");
        goto after_app;
    }

    printf_c16(u"\r\n");    // Separate SPP and APP info visually

    // Open all APP protocols for each handle
    for (UINTN i = 0; i < app_handles; i++) {
        status = bs->OpenProtocol(app_handle_buf[i], 
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
        printf_c16(u"APP %u; Min X: %u, Y: %u, Z: %u, Max X: %u, Y: %u, Z: %u, Attributes: %b\r\n",
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

    after_app:
    if (num_protocols == 0) {
        error(0, u"Could not find any Simple or Absolute Pointer Protocols.\r\n");
        goto done;
    }

    // Found valid SPP mode, get mouse input
    // Start off in middle of screen
    INT32 xres = mode_info->HorizontalResolution, yres = mode_info->VerticalResolution;
    cursor_x = (xres / 2) - (cursor_size / 2);
    cursor_y = (yres / 2) - (cursor_size / 2);

    // Print initial mouse state & draw initial cursor
    printf_c16(u"\r\nMouse Xpos: %d, Ypos: %d, Xmm: %d, Ymm: %d, LB: %u, RB: %u\r",
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
            double xmm_float = (double)state.RelativeMovementX / (double)active_spp->Mode->ResolutionX;
            double ymm_float = (double)state.RelativeMovementY / (double)active_spp->Mode->ResolutionY;

            // Erase text first before reprinting
            printf_c16(u"                                                                      \r");
            printf_c16(u"Mouse Xpos: %d, Ypos: %d, Xmm: %.4f, Ymm: %.4f, LB: %b, RB: %b\r",
                  cursor_x, cursor_y, xmm_float, ymm_float, state.LeftButton, state.RightButton);

            // Draw cursor: Get pixel amount to move per mm
            const double xres_mm_px = mode_info->HorizontalResolution * 0.02;
            const double yres_mm_px = mode_info->VerticalResolution   * 0.02;

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
            printf_c16(u"                                                                      \r");
            printf_c16(u"Ptr Xpos: %u, Ypos: %u, Zpos: %u, Buttons: %b\r",
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

    done:
    // Free mouse spp/app handle buffers memory & close open protocols
    if (spp_handle_buf) {
        for (UINTN i = 0; i < spp_handles; i++)
            bs->CloseProtocol(spp_handle_buf[i], &spp_guid, image, NULL);

        bs->FreePool(spp_handle_buf);
    }

    if (app_handle_buf) {
        for (UINTN i = 0; i < spp_handles; i++)
            bs->CloseProtocol(app_handle_buf[i], &app_guid, image, NULL);

        bs->FreePool(app_handle_buf);
    }

    return EFI_SUCCESS;
}

// =====================================================
// Test if EFI_SIMPLE_NETWORK_PROTOCOL is found or not
// =====================================================
EFI_STATUS test_network(void) {
    cout->ClearScreen(cout);

    EFI_GUID netGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
    EFI_SIMPLE_NETWORK_PROTOCOL* netProtocol;
    EFI_STATUS status = bs->LocateProtocol(&netGuid, NULL, (VOID**)&netProtocol);
    if(EFI_ERROR(status)) {
        printf_c16(u"ERROR: Network protocol(s) not found.\r\n");
    }
    else {
        printf_c16(u"Network protocol(s) found.\r\n");
    }
    get_key();
    return status;
}

// ===========================================================
// Timer function to print current date/time every 1 second
// ===========================================================
VOID EFIAPI print_datetime(__attribute__((unused)) IN EFI_EVENT event, IN VOID *Context) {
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
    printf_c16(u"%u-%c%u-%c%u %c%u:%c%u:%c%u",
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
    EFI_STATUS status = EFI_SUCCESS;

    // Get ESP root directory
    EFI_FILE_PROTOCOL *dirp = esp_root_dir();
    if (!dirp) {
        error(0, u"Could not get ESP root directory.\r\n");
        goto done;
    }

    // Start at root directory
    CHAR16 current_directory[256];
    strcpy_c16(current_directory, u"/");    

    // Print dir entries for currently opened directory
    // Overall input loop
    INT32 csr_row = 1;
    while (true) {
        cout->ClearScreen(cout);
        printf_c16(u"%s:\r\n", current_directory);

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

            printf_c16(u"%s %s\r\n", 
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
                goto done;
                break;

            case SCANCODE_UP_ARROW:
            case SCANCODE_DOWN_ARROW:
                // Go up or down 1 row in range [1:num_entries] (circular buffer)
                csr_row = (key.ScanCode == SCANCODE_UP_ARROW) 
                          ? ((csr_row-1 + num_entries-1) % num_entries) + 1
                          : (csr_row % num_entries) + 1;
                break;

            default:
                if (key.UnicodeChar == u'\r') {
                    // Enter key: 
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
                            goto done;
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
                                strcat_c16(current_directory, u"/"); 
                            }
                            strcat_c16(current_directory, file_info.FileName);
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
                        goto done;
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
                        goto done;
                    }

                    // Read file into buffer
                    status = dirp->Read(file, &buf_size, buffer);
                    if (EFI_ERROR(status)) {
                        error(status, u"Could not read file %s into buffer.\r\n", file_info.FileName);
                        goto done;
                    } 

                    if (buf_size != file_info.FileSize) {
                        error(0, u"Could not read all of file %s into buffer.\r\n" 
                              u"Bytes read: %u, Expected: %u\r\n",
                              file_info.FileName, buf_size, file_info.FileSize);
                        goto done;
                    }

                    // Print buffer contents
                    printf_c16(u"\r\nFile Contents:\r\n");

                    char *pos = (char *)buffer;
                    for (UINTN bytes = buf_size; bytes > 0; bytes--) {
                        CHAR16 str[2];
                        str[0] = *pos;
                        str[1] = u'\0';
                        if (*pos == '\n') {
                            // Convert LF newline to CRLF
                            printf_c16(u"\r\n");
                        } else {
                            printf_c16(u"%s", str);
                        }

                        pos++;
                    }

                    printf_c16(u"\r\n\r\nPress any key to continue...\r\n");
                    get_key();

                    // Free memory for file when done
                    bs->FreePool(buffer);

                    // Close file handle
                    dirp->Close(file);
                }
                break;
        }
    }

    done:
    if (dirp) dirp->Close(dirp);    // Cleanup directory pointer
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
    UINT32 this_image_media_id = 0;
    status = get_disk_image_mediaID(&this_image_media_id);
    if (EFI_ERROR(status)) {
        error(status, u"Could not get disk image media ID.\r\n");
        return status;
    }

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
                                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);

        if (EFI_ERROR(status)) {
            error(status, u"Could not Open Block IO protocol on handle %u.\r\n", i);
            continue;
        }

        // Print Block IO Media Info for this Disk/partition
        if (last_media_id != biop->Media->MediaId) {
            last_media_id = biop->Media->MediaId;   
            printf_c16(u"Media ID: %u %s\r\n", 
                   last_media_id, 
                   (last_media_id == this_image_media_id ? u"(Disk Image)" : u""));
        }

        if (biop->Media->LastBlock == 0) {
            // Only really care about partitions/disks above 1 block in size
            continue;
        }

        printf_c16(u"Rmv: %s, Pr: %s, LglPrt: %s, RdOnly: %s, Wrt$: %s\r\n"
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
        if (!biop->Media->LogicalPartition) printf_c16(u"<Entire Disk>\r\n");
        else {
            // Get partition info protocol for this partition
            EFI_GUID pi_guid = EFI_PARTITION_INFO_PROTOCOL_GUID;
            EFI_PARTITION_INFO_PROTOCOL *pip = NULL;
            status = bs->OpenProtocol(handle_buffer[i], 
                                      &pi_guid,
                                      (VOID **)&pip,
                                      image,
                                      NULL,
                                      EFI_OPEN_PROTOCOL_GET_PROTOCOL);

            if (EFI_ERROR(status)) {
                error(status, u"Could not Open Partition Info protocol on handle %u.\r\n", i);
            } else {
                if      (pip->Type == PARTITION_TYPE_OTHER) printf_c16(u"<Other Type>\r\n");
                else if (pip->Type == PARTITION_TYPE_MBR)   printf_c16(u"<MBR>\r\n");
                else if (pip->Type == PARTITION_TYPE_GPT) {
                    if (pip->System == 1) printf_c16(u"<EFI System Partition>\r\n");
                    else {
                        // Compare Gpt.PartitionTypeGUID with known values
                        EFI_GUID data_guid = BASIC_DATA_GUID;
                        if (!memcmp(&pip->Info.Gpt.PartitionTypeGUID, &data_guid, sizeof(EFI_GUID))) 
                            printf_c16(u"<Basic Data>\r\n");
                        else
                            printf_c16(u"<Other GPT Type>\r\n");
                    }
                }
            }
        }

        printf_c16(u"\r\n");    // Separate each block of text visually 
    }

    printf_c16(u"Press any key to go back..\r\n");
    get_key();
    return EFI_SUCCESS;
}

// ==========================================================
// Load an ELF64 PIE file into a new buffer, and return the 
//   entry point for the loaded ELF program
// ==========================================================
VOID *load_elf(VOID *elf_buffer, EFI_PHYSICAL_ADDRESS *file_buffer, UINTN *file_size) {
    ELF_Header_64 *ehdr = elf_buffer;

    // Only allow PIE ELF files
    if (ehdr->e_type != ET_DYN) {
        error(0, u"ELF is not a PIE file; e_type is not ETDYN/0x03\r\n");
        return NULL;
    }

    // Get loadable program header measurements for loading
    ELF_Program_Header_64 *phdr = (ELF_Program_Header_64 *)((UINT8 *)ehdr + ehdr->e_phoff);

    UINTN max_alignment = PAGE_SIZE;    
    UINTN mem_min = UINT64_MAX, mem_max = 0;

    for (UINT16 i = 0; i < ehdr->e_phnum; i++, phdr++) {
        // Only interested in loadable program headers
        if (phdr->p_type != PT_LOAD) continue;

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

    // Allocate buffer for program headers
    EFI_STATUS status = 0;
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
    // Get COFF header
    UINT8 pe_sig_offset = 0x3C; // From PE file format
    UINT32 pe_sig_pos = *(UINT32 *)((UINT8 *)pe_buffer + pe_sig_offset);
    UINT8 *pe_sig = (UINT8 *)pe_buffer + pe_sig_pos;

    PE_Coff_File_Header_64 *coff_hdr = (PE_Coff_File_Header_64 *)(pe_sig + 4);

    // Validate header values
    if (coff_hdr->Machine != ARCH_COFF_MACHINE) {
        error(0, u"Machine type not ARCH.\r\n");    // Uses ARCH from makefile
        return NULL;
    }

    if (!(coff_hdr->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        error(0, u"PE file not an executable image.\r\n");
        return NULL;
    }

    // Get Optional Header
    PE_Optional_Header_64 *opt_hdr = 
        (PE_Optional_Header_64 *)((UINT8 *)coff_hdr + sizeof(PE_Coff_File_Header_64));

    // Validate Optional header info
    if (opt_hdr->Magic != 0x20B) {
        error(0, u"PE file is not a PE32+ file.\r\n");
        return NULL;
    }

    if (!(opt_hdr->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) {
        error(0, u"PE file is not a PIE file.\r\n");
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

    // Get and load section headers into new buffer, from original "physical" data/addresses into
    //   new "virtual" addresses
    PE_Section_Header_64 *shdr = 
        (PE_Section_Header_64 *)((UINT8 *)opt_hdr + coff_hdr->SizeOfOptionalHeader);

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

// ==========================================
// Read a file from the basic data partition
// ==========================================
EFI_STATUS load_kernel(void) {
    EFI_HII_PACKAGE_LIST_HEADER *pkg_list = NULL;   
    EFI_STATUS status = EFI_SUCCESS;

    // Defined in efi_lib.h
    Kernel_Parms kparms = {     
        .mmap                 = {0},
        .gop_mode             = {0},
        .RuntimeServices      = rs,
        .NumberOfTableEntries = st->NumberOfTableEntries,
        .ConfigurationTable   = st->ConfigurationTable,
        .num_fonts            = 0,
        .fonts                = NULL,
    };

    cout->ClearScreen(cout);

    // Get kernel file from data partition on disk 
    UINTN file_size = 0;
    VOID *disk_buffer = read_data_partition_file_to_buffer("kernel", false, &file_size);
    if (!disk_buffer) {
        error(0, u"Could not find or read kernel file to buffer\r\n");
        goto cleanup;
    }

    // Load Kernel binary depending on format (initial header bytes)
    UINT8 *hdr = disk_buffer;
    printf_c16(u"Header bytes: [%hhx][%hhx][%hhx][%hhx]\r\n", 
           hdr[0], hdr[1], hdr[2], hdr[3]);

    EFI_PHYSICAL_ADDRESS kernel_buffer = 0;
    UINTN kernel_size = 0;

    // Load kernel binary and get the entry point
    // Get around compiler warning about function vs void pointer
    //   with a cast to (void **)
    Entry_Point entry_point = NULL;

    printf_c16(u"File Format: ");
    if (!memcmp(hdr, (UINT8[4]){0x7F, 'E', 'L', 'F'}, 4)) {
        printf_c16(u"ELF\r\n");
        print_elf_info(disk_buffer); // Print ELF header and loadable program header information
        *(void **)&entry_point = load_elf(disk_buffer, &kernel_buffer, &kernel_size);   

    } else if (!memcmp(hdr, (UINT8[2]){'M', 'Z'}, 2)) {
        printf_c16(u"PE\r\n");
        print_pe_info(disk_buffer); // Print PE header and loadable section header information
        *(void **)&entry_point = load_pe(disk_buffer, &kernel_buffer, &kernel_size); 

    } else {
        printf_c16(u"No format found, assuming flat binary file\r\n");
        // Flat binary executable code assumed to start at the beginning of the loaded buffer
        *(void **)&entry_point = disk_buffer;   
        kernel_buffer = (EFI_PHYSICAL_ADDRESS)disk_buffer;
        kernel_size = file_size;
    }

    // Get new higher address kernel entry point to use
    UINTN entry_offset = (UINTN)entry_point - kernel_buffer;
    Entry_Point higher_entry_point = (Entry_Point)(KERNEL_START_ADDRESS + entry_offset);

    // Print info for loaded kernel 
    printf_c16(u"\r\nOriginal Kernel address: %llx, size: %u, entry point: %llx\r\n"
           u"Higher address entry point: %llx\r\n",
            kernel_buffer, kernel_size, (UINTN)entry_point, higher_entry_point);

    if (!entry_point) {   
        // Clean up/free pages for allocated kernel buffer
        bs->FreePages(kernel_buffer, kernel_size / PAGE_SIZE);
        goto cleanup;     
    }

    if (!autoload_kernel) {
        printf_c16(u"\r\nPress ESC to abort, or another key to load kernel...\r\n");
        EFI_INPUT_KEY key = get_key();
        if (key.ScanCode == SCANCODE_ESC)
            goto cleanup;
    }

    // Close Timer Event so that it does not continue to fire off
    bs->CloseEvent(timer_event);

    // Initialize Kernel Parameters
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    bool set_mode = false;
    if (autoload_kernel) {
        // Set gop mode from width & height values in install data file
        CHAR16 *install_file = u"\\EFI\\BOOT\\INSTALL.DAT";
        UINTN buf_size = 0;
        VOID *inst_file_buf = read_esp_file_to_buffer(install_file, &buf_size);
        if (!inst_file_buf) goto gop_done;

        char *str_pos = stpstr(inst_file_buf, "XRES=");
        if (!str_pos) goto gop_done;
        UINT32 xres = atoi(str_pos);

        str_pos = stpstr(str_pos, "YRES=");
        if (!str_pos) goto gop_done;
        UINT32 yres = atoi(str_pos);

        bs->FreePool(inst_file_buf);

        status = set_gop_mode(&gop, xres, yres); 
        set_mode = !EFI_ERROR(status);
    } 

    gop_done:
    if (!set_mode) {
        status = set_gop_mode(&gop, 1920, 1080);    // Try 1080p as default
        if (EFI_ERROR(status))
            status = set_largest_gop_mode(&gop);    // Try largest WxH mode found
    }

    if (EFI_ERROR(status)) {
        error(status, u"Could not set GOP mode for kernel.\r\n");
        goto cleanup;
    }

    kparms.gop_mode = *gop->Mode;

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

    // Get PSF font file for another bitmap font to use;
    //   this one should be stored in the disk image's data partition
    char *psf_name = "ter-132n.psf";
    UINTN psf_size = 0;
    VOID *psf_font = read_data_partition_file_to_buffer(psf_name, false, &psf_size);
    if (psf_font) {
        PSF2_Header *psf2_hdr = psf_font;
        kparms.fonts[1] = (Bitmap_Font){
            .name            = psf_name,
            .width           = psf2_hdr->width,
            .height          = psf2_hdr->height,
            .left_col_first  = true,                   // Pixels in memory are stored left to right
            .num_glyphs      = psf2_hdr->num_glyphs,
            .glyphs          = (uint8_t *)(psf2_hdr+1),
        };
    }

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

    // Initialize page tables
    arch_init_page_tables(&kparms.mmap);

    // Identity mapping all available memory 
    identity_map_efi_mmap(&kparms.mmap);

    // Identity map runtime services memory & set new runtime address map
    set_runtime_address_map(&kparms.mmap);

    // Remap kernel to higher addresses
    for (UINTN i = 0; i < (kernel_size + (PAGE_SIZE-1)) / PAGE_SIZE; i++) 
        arch_map_page(kernel_buffer + (i*PAGE_SIZE), 
                      KERNEL_START_ADDRESS + (i*PAGE_SIZE), 
                      &kparms.mmap); 

    // NOTE: TODO: Remap kparms to higher address?

    // Identity map framebuffer
    for (UINTN i = 0; i < (kparms.gop_mode.FrameBufferSize + (PAGE_SIZE-1)) / PAGE_SIZE; i++) 
        identity_map_page(kparms.gop_mode.FrameBufferBase + (i*PAGE_SIZE), &kparms.mmap); 

    // Identity map new stack for kernel
    const UINTN STACK_PAGES = 16;   
    void *kernel_stack = mmap_allocate_pages(&kparms.mmap, STACK_PAGES);   // 64KiB stack
    uint32_t stack_size = STACK_PAGES * PAGE_SIZE;
    memset(kernel_stack, 0, stack_size); // Initialize stack memory

    for (UINTN i = 0; i < STACK_PAGES; i++) 
        identity_map_page((UINTN)kernel_stack + (i*PAGE_SIZE), &kparms.mmap); 

    // Set page tables & paging, do other arch specific settings, and call kernel
    arch_setup_and_call_kernel(higher_entry_point, kernel_stack, stack_size, &kparms);

    // Final cleanup
    cleanup:
    if (disk_buffer) bs->FreePool(disk_buffer); // Free memory for data partition file
    if (pkg_list)    bs->FreePool(pkg_list);    // Free memory for simple font package list

    if (kparms.fonts) {
        // Free memory for kparms font glyphs
        for (UINTN i = 0; i < kparms.num_fonts; i++)    
            bs->FreePool(kparms.fonts[i].glyphs);

        bs->FreePool(kparms.fonts);   // Free memory for kparms fonts array
    }

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
    printf_c16(u"Memory map: Size %u, Descriptor size: %u, # of descriptors: %u, key: %x\r\n",
            mmap.size, mmap.desc_size, mmap.size / mmap.desc_size, mmap.key);

    UINTN usable_bytes = 0; // "Usable" memory for an OS or similar, not firmware/device reserved
    for (UINTN i = 0; i < mmap.size / mmap.desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = 
            (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)mmap.map + (i * mmap.desc_size));

        printf_c16(u"%u: Typ: %u, Phy: %x, Vrt: %x, Pgs: %u, Att: %x\r\n",
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
            printf_c16(u"Press any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }

    printf_c16(u"\r\nUsable memory: %u / %u MiB / %u GiB\r\n",
            usable_bytes, usable_bytes / (1024 * 1024), usable_bytes / (1024 * 1024 * 1024));

    // Free allocated buffer for memory map
    bs->FreePool(mmap.map);

    printf_c16(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
}

// =======================================
// Print configuration table GUID values
// =======================================
EFI_STATUS print_config_tables(void) { 
    cout->ClearScreen(cout);

    // Close Timer Event for cleanup
    bs->CloseEvent(timer_event);

    printf_c16(u"Configuration Table GUIDs:\r\n");
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
        printf_c16(u"(%s)\r\n\r\n", 
               found ? config_table_guids_and_strings[j].string : u"Unknown GUID Value");

        // Pause at bottom of screen
        if (cout->Mode->CursorRow >= text_rows-2) {
            printf_c16(u"Press any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
    }

    printf_c16(u"\r\nPress any key to go back...\r\n");
    get_key();
    return EFI_SUCCESS;
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
    if (rsdp_ptr) {
        printf_c16(u"ACPI 2.0 Table found at %#x\r\n", rsdp_ptr);
        acpi_20 = true;
    } else {
        // Check for ACPI 1.0 table as fallback
        acpi_guid = (EFI_GUID)ACPI_TABLE_GUID;
        rsdp_ptr = get_config_table_by_guid(acpi_guid);
        if (rsdp_ptr) {
            printf_c16(u"ACPI 1.0 Table found at %#x\r\n", rsdp_ptr);
        } else {
            error(0, u"Could not find ACPI configuration table\r\n");
            return 1;
        }
    }

    // Print RSDP
    UINT8 *rsdp = rsdp_ptr;
    printf_c16(u"RSDP:\r\n"
           u"Signature: %.8hhs\r\n"
           u"Checksum: %hhu\r\n"
           u"OEMID: %.6hhs\r\n"
           u"Revision: %hhu\r\n"
           u"RSDT Address: %x\r\n",
           &rsdp[0], 
           rsdp[8],
           &rsdp[9],
           rsdp[15],
           *(UINT32 *)&rsdp[16]);

    if (acpi_20) {
        printf_c16(u"Length: %u\r\n"
               u"XSDT Address: %x\r\n"
               u"Extended Checksum: %hhu\r\n",
               *(UINT32 *)&rsdp[20],
               *(UINT64 *)&rsdp[24],
               rsdp[32]);
    } 

    printf_c16(u"\r\nPress any key to print RSDT/XSDT...\r\n");
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
        printf_c16(u"\r\nPress any key to print entries...\r\n");
        get_key();

        cout->ClearScreen(cout);
        printf_c16(u"Entries:\r\n");
        UINT64 *entry = (UINT64 *)((UINT8 *)header + sizeof *header); 
        for (UINTN i = 0; i < (header->length - sizeof *header) / 8; i++) {
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)entry[i];
            printf_c16(u"%.4hhs\r\n", &table_header.signature[0]);

            if (cout->Mode->CursorRow >= text_rows-2) {
                printf_c16(u"Press any key to continue...\r\n");
                get_key();
                cout->ClearScreen(cout);
            }
        }

        printf_c16(u"\r\nPress any key to print next table...\r\n");
        get_key();

        // Loop and print each ACPI table
        for (UINTN i = 0; i < (header->length - sizeof *header) / 8; i++) {
            cout->ClearScreen(cout);

            // Print header
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)entry[i];
            print_acpi_table_header(table_header);

            // TODO: Print specific table info ?

            printf_c16(u"\r\nPress any key to print next table...\r\n");
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
        printf_c16(u"\r\nPress any key to print entries...\r\n");
        get_key();

        cout->ClearScreen(cout);
        printf_c16(u"Entries:\r\n");
        UINT32 *entry = (UINT32 *)((UINT8 *)header + sizeof *header); 
        for (UINTN i = 0; i < (header->length - sizeof *header) / 4; i++) {
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)(UINTN)entry[i];
            printf_c16(u"%.4hhs\r\n", &table_header.signature[0]);

            if (cout->Mode->CursorRow >= text_rows-2) {
                printf_c16(u"Press any key to continue...\r\n");
                get_key();
                cout->ClearScreen(cout);
            }
        }

        printf_c16(u"\r\nPress any key to print next table...\r\n");
        get_key();

        // Loop and print each ACPI table
        for (UINTN i = 0; i < (header->length - sizeof *header) / 4; i++) {
            cout->ClearScreen(cout);

            // Print header
            ACPI_TABLE_HEADER table_header = *(ACPI_TABLE_HEADER *)(UINTN)entry[i];
            print_acpi_table_header(table_header);

            // TODO: Print specific table info ?

            printf_c16(u"\r\nPress any key to print next table...\r\n");
            get_key();
        }
    }

    printf_c16(u"\r\nPress any key to go back...\r\n");
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
        error(status, u"Could not allocate 2 bytes?!\r\n");
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
            
            strcpy_c16(temp_buf, var_name_buf);  // Copy old buffer to new buffer
            bs->FreePool(var_name_buf);          // Free old buffer
            var_name_buf = temp_buf;             // Set new buffer

            status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
            continue;
        }

        // Print variable name
        printf_c16(u"%.*s\r\n", var_name_size, var_name_buf);

        // Pause at bottom of screen
        if (cout->Mode->CursorRow >= text_rows-2) {
            printf_c16(u"Press any key to continue...\r\n");
            get_key();
            cout->ClearScreen(cout);
        }
        status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
    }

    // Free buffer when done
    bs->FreePool(var_name_buf);

    printf_c16(u"\r\nPress any key to go back...\r\n");
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
                
                strcpy_c16(temp_buf, var_name_buf);  // Copy old buffer to new buffer
                bs->FreePool(var_name_buf);          // Free old buffer
                var_name_buf = temp_buf;             // Set new buffer

                status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
                continue;
            }

            // Print variable name and their value(s)
            if (!memcmp(var_name_buf, u"Boot", 8)) {
                printf_c16(u"\r\n%.*s: ", var_name_size, var_name_buf);

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
                        printf_c16(u"%#.4x,", *p++);   

                    printf_c16(u"\r\n");
                    goto next;
                }

                if (!memcmp(var_name_buf, u"BootOptionSupport", 34)) {
                    // Single UINT32 value
                    UINT32 *p = data;
                    printf_c16(u"%#.8x\r\n", *p);  
                    goto next;
                }

                if (!memcmp(var_name_buf, u"BootNext",    18) || 
                    !memcmp(var_name_buf, u"BootCurrent", 22)) {

                    // Single UINT16 value
                    UINT16 *p = data;
                    printf_c16(u"%#.4hx\r\n", *p); 
                    goto next;
                }

                if (isxdigit_c16(var_name_buf[4]) && var_name_size == 18) {  
                    // Boot#### load option: Name size = 8 CHAR16 chars * 2 bytes + CHAR16 null bytes
                    EFI_LOAD_OPTION *load_option = (EFI_LOAD_OPTION *)data;
                    CHAR16 *description = (CHAR16 *)((UINT8 *)data + sizeof(UINT32) + sizeof(UINT16));
                    printf_c16(u"%s\r\n", description);    

                    CHAR16 *p = description;
                    UINTN strlen =  0;
                    while (p[strlen]) strlen++;  
                    strlen++;                    // Skip null byte

                    EFI_DEVICE_PATH_PROTOCOL *file_path_list = 
                        (EFI_DEVICE_PATH_PROTOCOL *)(description + strlen); 

                    CHAR16 *device_path_text = 
                        dpttp->ConvertDevicePathToText(file_path_list, FALSE, FALSE);

                    printf_c16(u"Device Path: %s\r\n", device_path_text ? device_path_text : u"(null)");

                    UINT8 *optional_data = (UINT8 *)file_path_list + load_option->FilePathListLength;
                    UINTN optional_data_size = data_size - (optional_data - (UINT8 *)data);
                    if (optional_data_size > 0) {
                        printf_c16(u"Optional Data: 0x");
                        for (UINTN i = 0; i < optional_data_size; i++)
                            printf_c16(u"%.2hhx", optional_data[i]);

                        printf_c16(u"\r\n"); 
                    }
                    
                    goto next; 
                }

                printf_c16(u"\r\n");  // Unhandled Boot* variable, go on with space before next one

                next:
                bs->FreePool(data);
            }

            // Pause at bottom of screen
            if (cout->Mode->CursorRow >= text_rows-2) {
                printf_c16(u"Press any key to continue...\r\n");
                get_key();
                cout->ClearScreen(cout);
            }
            status = rs->GetNextVariableName(&var_name_size, var_name_buf, &vendor_guid);
        }

        // Allow user to change values
        printf_c16(u"Press '1' to change BootOrder, '2' to change BootNext, or other to go back...");
        EFI_INPUT_KEY key = get_key();
        if (key.UnicodeChar == u'1') {
            // Change BootOrder - set new array of UINT16 values
            #define MAX_BOOT_OPTIONS 10
            UINT16 option_array[MAX_BOOT_OPTIONS] = {0};
            UINTN new_option = 0;
            UINT16 num_options = 0;
            for (UINTN i = 0; i < MAX_BOOT_OPTIONS; i++) {
                printf_c16(u"\r\nBoot Option %u (0000-FFFF): ", i+1);
                if (!get_num(&new_option, 16)) break;    // Stop processing
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
            printf_c16(u"\r\nBootNext value (0000-FFFF): ");
            UINTN value = 0;
            if (get_num(&value, 16)) {
                EFI_GUID guid = EFI_GLOBAL_VARIABLE_GUID;
                UINT32 attr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                              EFI_VARIABLE_RUNTIME_ACCESS;

                status = rs->SetVariable(u"BootNext", &guid, attr, 2, &value);
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

// =================================================================
// "Install" this disk image/bootloader, by creating a new 
//    file marking it as installed. This file existing on
//    boot will go on to load the kernel instead of the main menu
// =================================================================
EFI_STATUS install_to_disk(void) { 
    EFI_STATUS status = EFI_SUCCESS;

    CHAR16 *path = u"\\EFI\\BOOT\\INSTALL.DAT";
    printf_c16(u"\r\nInstall to disk by writing file '%s' (Y/N)?  ", path);

    bool yes = false, no = false;
    EFI_INPUT_KEY key = get_key();
    while (key.UnicodeChar != u'\r' && key.ScanCode != SCANCODE_ESC) {
        yes = no = false;
        yes = (key.UnicodeChar == 'Y' || key.UnicodeChar == 'y');
        no  = (key.UnicodeChar == 'N' || key.UnicodeChar == 'n');
        if (yes || no) printf_c16(u"\b%c", key.UnicodeChar);    // Overwrite character at same position
        key = get_key();
    }
    printf_c16(u"\r\n");

    if (yes) {
        EFI_FILE_PROTOCOL *root = esp_root_dir();
        if (!root) {
            error(0, u"Could not get ESP root directory.\r\n");
            return EFI_UNSUPPORTED;
        }

        // Create new empty file, flags must be Create+Read+Write
        EFI_FILE_PROTOCOL *file = NULL;
        status = root->Open(root, 
                            &file, 
                            path, 
                            EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                            0);

        if (EFI_ERROR(status)) {
            error(status, u"Could not create new file '%s'\r\n", path);
            goto cleanup;
        }

        // Get info for desired GOP Framebuffer WidthxHeight values
        UINTN fb_width = 0, fb_height = 0;
        UINT32 mode_num = 0;
        while (true) {
            printf_c16(u"Desired Framebuffer Width (number, default = 1920)? ");
            get_num(&fb_width, 10);
            if (fb_width == 0) fb_width = 1920;

            printf_c16(u"\r\nFramebuffer Height (number, default = 1080)? ");
            get_num(&fb_height, 10);
            if (fb_height == 0) fb_height = 1080;
            
            if (!EFI_ERROR(check_gop_mode(&mode_num, fb_width, fb_height)))
                break;

            printf_c16(u"\r\nGOP Mode not found for WidthxHeight.\r\n");
        }

        // Write GOP mode & values to file
        char buf[512];
        sprintf(buf, "GOP_MODE=%u\r\n"
                     "XRES=%u\r\n"
                     "YRES=%u\r\n",
                     mode_num,
                     fb_width,
                     fb_height);
                
        UINTN buf_size = strlen(buf);
        status = file->Write(file, &buf_size, buf);
        if (EFI_ERROR(status)) {
            error(status, u"Issue writing GOP info to file '%s'.\r\n", path);
            goto cleanup;
        }

        printf_c16(u"\r\n\r\nFile '%s' written.\r\n"
               u"Next boot will automatically load the kernel and not the main menu.\r\n"
               u"Press any key to go on...\r\n\r\n",
               path);
        get_key();

        // Cleanup file/directory pointers
        cleanup:
        if (file) file->Close(file);
        if (root) root->Close(root);
    }

    return status;
}

// ===================================================
// Write disk image to other disk (blockIO media ID)
// ===================================================
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
    CHAR16 *file_name = u"\\EFI\\BOOT\\FILE.TXT";
    UINTN buf_size = 0;
    VOID *file_buffer = NULL;
    file_buffer = read_esp_file_to_buffer(file_name, &buf_size);
    if (!file_buffer) {
        error(0, u"Could not find or read file '%s' to buffer\r\n", file_name);
        return 1;
    }

    char *str_pos = strstr(file_buffer, "DISK_SIZE=");
    if (!str_pos) {
        error(0, u"Could not find disk image size in FILE.TXT\r\n");
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
            printf_c16(u"Media ID: %u %s\r\n", 
                   last_media_id, 
                   (last_media_id == disk_image_media_id ? u"(Disk Image)" : u""));

            if (last_media_id == disk_image_media_id) 
                disk_image_bio = biop; // Save for later
        }

        // Get disk size in bytes, add 1 block for 0-based indexing fun
        UINTN size = (biop->Media->LastBlock+1) * biop->Media->BlockSize; 

        printf_c16(u"Rmv: %s, Size: %llu/%llu MiB/%llu GiB\r\n",
               biop->Media->RemovableMedia ? u"Yes" : u"No",
               size, size / (1024 * 1024), size / (1024 * 1024 * 1024));

        if (biop->Media->MediaId == disk_image_media_id) {
            printf_c16(u"Disk image size: %llu/%llu MiB/%llu GiB\r\n",
                   disk_image_size, 
                   disk_image_size / (1024 * 1024), 
                   disk_image_size / (1024 * 1024 * 1024));
        }
        printf_c16(u"\r\n");
    }

    // Take in a number from the user for the media to write the disk image to
    printf_c16(u"Input Media ID number to write to and press enter: ");
    UINTN chosen_media = 0;
    get_num(&chosen_media, 10);
    printf_c16(u"\r\n");

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

    // Ask user to install bootloader yes/no. If yes, will autoload kernel on next
    //   boot from new disk from existence of new "install" file.
    install_to_disk();

    // Print info about chosen disk and disk image
    // block size for from and to disks
    UINTN from_block_size = disk_image_bio->Media->BlockSize, 
          to_block_size = chosen_disk_bio->Media->BlockSize;

    UINTN from_blocks = (disk_image_size + (from_block_size-1)) / from_block_size;
    UINTN to_blocks = (disk_image_size + (to_block_size-1)) / to_block_size;

    printf_c16(u"From block size: %u, To block size: %u\r\n"
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
    printf_c16(u"Reading %u blocks from disk image disk to buffer...\r\n", from_blocks);
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
    printf_c16(u"Writing %u blocks from buffer to chosen disk...\r\n", to_blocks);
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

    printf_c16(u"\r\nDisk Image written to chosen disk.\r\n"
           u"Reboot and choose new boot option when able.\r\n");

    printf_c16(u"\r\nPress any key to go back...\r\n");
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

    // Get current text mode ColsxRows values
    UINTN cols = 0, rows = 0;
    cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

    // Set global text rows/cols values
    text_rows = rows; 
    text_cols = cols;

    // Check for "installed" file to autoload kernel instead of main menu, or not
    EFI_FILE_PROTOCOL *root = esp_root_dir();
    if (root) {
        EFI_STATUS status = EFI_SUCCESS;
        CHAR16 *path = u"\\EFI\\BOOT\\INSTALL.DAT";
        EFI_FILE_PROTOCOL *file = NULL;
        status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
        autoload_kernel = !EFI_ERROR(status);
        if (file) file->Close(file);
        if (root) root->Close(root);
    }

    if (autoload_kernel) load_kernel(); // Load kernel; Should not return!

    // Menu text on screen
    const CHAR16 *menu_choices[] = {
        u"Set Text Mode",
        u"Set Graphics Mode",
        u"Test Mouse",
        u"Test Network",
        u"Read ESP Files",
        u"Print Block IO Partitions",
        u"Print Memory Map",
        u"Print Configuration Tables",
        u"Print ACPI Tables",
        u"Print EFI Global Variables",
        u"Load Kernel",
        u"Change Boot Variables",
        u"Write Disk Image Image To Other Disk",
        u"Install Bootloader & Autoload Kernel",
    };

    // Functions to call for each menu option
    EFI_STATUS (*menu_funcs[])(void) = {
        set_text_mode,
        set_graphics_mode,
        test_mouse,
        test_network,
        read_esp_files,
        print_block_io_partitions,
        print_memory_map,
        print_config_tables,
        print_acpi_tables,
        print_efi_global_variables,
        load_kernel,
        change_boot_variables,
        write_to_another_disk,
        install_to_disk
    };

    // Connect all controllers found for all handles, to hopefully fix
    //   any bugs related to not initializing device drivers from firmware
    connect_all_controllers();

    // Timer function context will be the text mode screen bounds
    typedef struct {
        UINT32 rows; 
        UINT32 cols;
    } Timer_Context;

    Timer_Context context = { .rows = rows, .cols = cols };

    // Screen loop
    bool running = true;
    while (running) {
        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(cout);

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
        printf_c16(u"Up/Down Arrow = Move cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Shutdown");

        // Print menu choices
        // Highlight first choice as initial choice
        cout->SetCursorPosition(cout, 0, 0);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf_c16(u"%s", menu_choices[0]);

        // Print rest of choices
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINTN i = 1; i < ARRAY_SIZE(menu_choices); i++)
            printf_c16(u"\r\n%s", menu_choices[i]);

        // Get cursor row boundaries
        INTN max_row = cout->Mode->CursorRow;

        // Input loop
        cout->SetCursorPosition(cout, 0, 0);
        bool getting_input = true;
        while (getting_input) {
            INTN current_row = cout->Mode->CursorRow;
            EFI_INPUT_KEY key = get_key();

            // Process input
            switch (key.ScanCode) {
                case SCANCODE_UP_ARROW:
                case SCANCODE_DOWN_ARROW:
                    // De-highlight current row 
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    printf_c16(u"%s\r", menu_choices[current_row]);

                    // Go up or down 1 row in range [0:max_row] (circular buffer)
                    current_row = (key.ScanCode == SCANCODE_UP_ARROW) 
                                  ? (current_row + max_row) % (max_row+1)
                                  : (current_row+1) % (max_row+1);  

                    // Highlight new current row
                    cout->SetCursorPosition(cout, 0, current_row);
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                    printf_c16(u"%s\r", menu_choices[current_row]);

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                case SCANCODE_ESC:
                    // Escape key: power off
                    rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

                    // !NOTE!: This should not return, system should power off
                    __builtin_unreachable();
                    break;

                default:
                    if (key.UnicodeChar == u'\r') {
                        cout->ClearScreen(cout);

                        // Enter key, select choice
                        EFI_STATUS return_status = menu_funcs[current_row]();
                        if (EFI_ERROR(return_status)) 
                            error(return_status, u"Press any key to go back...");

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
