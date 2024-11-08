// kernel.c: Sample "kernel" file for testing
#include <stdint.h>
#include "efi.h"
#include "efi_lib.h"

// ------------------------------
// Global variables / Constants
// ------------------------------
enum {
    LIGHT_GRAY = 0,
    RED,
    DARK_GRAY,

    COLOR_MAX,
} COLOR_NAMES;

// All colors are assumed ARGB8888
const uint32_t colors[COLOR_MAX] = {
    [LIGHT_GRAY] = 0xFFDDDDDD, 
    [RED]        = 0xFFCC2222, 
    [DARK_GRAY]  = 0xFF222222, 
};

uint32_t *fb = NULL;  // Framebuffer
uint32_t xres = 0;    // X/Horizontal resolution of framebuffer
uint32_t yres = 0;    // Y/Vertical resolution of framebuffer
uint32_t x = 0;       // X offset into framebuffer
uint32_t y = 0;       // Y offset into framebuffer

const uint32_t text_fg_color = colors[LIGHT_GRAY];
const uint32_t text_bg_color = colors[DARK_GRAY];

void print_string(char *string, Bitmap_Font *font);

// ==============
// MAIN
// ==============
__attribute__((section(".kernel"), aligned(0x1000))) void EFIAPI kmain(Kernel_Parms *kargs) {
    // Grab Framebuffer/GOP info
    fb = (UINT32 *)kargs->gop_mode.FrameBufferBase;  // BGRA8888
    xres = kargs->gop_mode.Info->PixelsPerScanLine;
    yres = kargs->gop_mode.Info->VerticalResolution;

    // Clear screen to solid color
    UINTN color = colors[DARK_GRAY];
    for (y = 0; y < yres; y++) 
        for (x = 0; x < xres; x++) 
            fb[y*xres + x] = color;

    // Print test string(s)
    x = y = 0;  // Reset to 0,0 position
    Bitmap_Font *font1 = &kargs->fonts[0];
    Bitmap_Font *font2 = &kargs->fonts[1];
    print_string("Hello, kernel bitmap font world!", font1);
    print_string("\r\nFont 1 Name: ", font1);
    print_string(font1->name, font1);
    print_string("\r\nFont 2 Name: ", font2);
    print_string(font2->name, font2);

    //print_string("\r\nTest long wrapping line: ", font);
    //for (uint64_t i = 0; i < 200; i++)
    //  print_string("@", font);

    //const uint32_t char_lines = yres / font->height;
    //y = (char_lines - 1) * font->height; // Last character line on screen    
    //print_string("\rTest scrolling up 1+ lines:", font);                          
    //for (uint64_t i = 0; i < char_lines-1; i++)
    //    print_string("\r\n", font);

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

    // Uncomment if qemu/hardware works fine with shutdown
    //kargs->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // Uncomment if qemu/hardware works does not work with shutdown;
    // Infinite loop, do not return back to UEFI,
    //   this is in case my hardware (laptop) doesn't shut off from ResetSystem
    //   Can still use power button manually to shut down fine
    while (true) __asm__ __volatile__ ("cli; hlt");

    // Should not return after shutting down
    __builtin_unreachable();
}

// ======================================================================
// Print a line feed visually (go down 1 line and/or scroll the screen)
// ======================================================================
void line_feed(Bitmap_Font *font) {
    // Can we draw another line of characters below current line?
    if (y + font->height < yres - font->height) y += font->height; // Yes, go down 1 line 
    else {
        // No more room, move all lines on screen 1 row up by overwriting 1st line with lines 2+
        // NOTE: This is probably slow due to reading and writing to framebuffer
        uint32_t char_line_px    = xres * font->height;
        uint32_t char_line_bytes = char_line_px * 4;    // Assuming ARGB8888
        uint32_t char_lines      = yres / font->height;

        memcpy(fb, fb + char_line_px, char_line_bytes * (char_lines-1)); 

        // Blank out last row by making all pixels the background color 
        // Get X,Y start of last character line 
        uint32_t px = ((yres / font->height) - 1) * font->height * xres; 
        for (uint32_t i = 0; i < char_line_px; i++)
            fb[px++] = text_bg_color;
    }
}

// ===========================================================
// Print a bitmapped font string to the screen (framebuffer)
// ===========================================================
void print_string(char *string, Bitmap_Font *font) {
    uint32_t glyph_size = ((font->width + 7) / 8) * font->height;   // Size of all glyph lines
    uint32_t glyph_width_bytes = (font->width + 7) / 8;             // Size of 1 line of a glyph
    for (char c = *string++; c != '\0'; c = *string++) {
        if (c == '\r') { x = 0; continue; }             // Carriage return (CR)
        if (c == '\n') { line_feed(font); continue; }   // Line Feed (LF) 

        uint8_t *glyph = &font->glyphs[c * glyph_size];

        // Draw each line of glyph
        for (uint32_t i = 0; i < font->height; i++) {
            // e.g. for glyph 'F':
            // Left to right 1 bits:      Right to left 1 bits:
            // mask starts here ------v   mask starts here ------v
            // data starts here ->11111   data starts here ->11111
            //                    1                              1
            //                    111                          111
            //                    1                              1
            //                    1                              1
            // The bitmask starts at the font width and goes down to 0, which ends up 
            //   drawing the bits mirrored from how they are stored in memory.
            //   We want to byteswap the bits if it is already stored left to right, so that 
            //   drawing it mirrored will be left to right visually, not right to left.
            // NOTE: This only works for fonts <= 64 pixels in width, and assumes font data
            //    is padded with 0s to not overflow, if last line of character data is less than 8 bytes.
            uint64_t mask = 1 << (font->width-1);
            uint64_t bytes = font->left_col_first ? 
                             ((uint64_t)glyph[0] << 56) | // Byteswap character data; can 
                             ((uint64_t)glyph[1] << 48) | //   also use __builtin_bswap64(*(uint64_t *)glyph)
                             ((uint64_t)glyph[2] << 40) | 
                             ((uint64_t)glyph[3] << 32) |
                             ((uint64_t)glyph[4] << 24) |
                             ((uint64_t)glyph[5] << 16) | 
                             ((uint64_t)glyph[6] <<  8) |
                             ((uint64_t)glyph[7] <<  0) 
                             : *(uint64_t *)glyph;   // Else pixels are stored right to left
            for (uint32_t px = 0; px < font->width; px++) {
                fb[y*xres + x] = bytes & mask ? text_fg_color : text_bg_color;
                mask >>= 1;
                x++;            // Next pixel of character
            }
            y++;                // Next line of character
            x -= font->width;   // Back up to start of character 
            glyph += glyph_width_bytes;
        }

        // Go to start of next character, top left pixel
        y -= font->height;      
        if (x + font->width < xres - font->width) x += font->width; 
        else {
            // Wrap text to next line with a CR/LF
            x = 0;
            line_feed(font);
        }
    }
}








