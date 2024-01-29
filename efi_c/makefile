.POSIX:
.PHONY: all clean

SOURCES = efi.c
OBJS = $(SOURCES:.c=.o)
DEPENDS = $(OBJS:.o=.d)
TARGET = BOOTX64.EFI
TEST_FILE = test.txt

# Uncomment disk image exe & shell script for Windows
#DISK_IMG_PGM = write_gpt.exe
#QEMU_SCRIPT = qemu.bat

# Uncomment disk image exe & shell script for Linux
DISK_IMG_PGM = write_gpt
QEMU_SCRIPT = qemu.sh

# Uncomment CC/LDFLAGS for gcc
#CC = x86_64-w64-mingw32-gcc 
#LDFLAGS = \
	-nostdlib \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment CC/LDFLAGS for clang
CC = clang \
	-target x86_64-unknown-windows 
LDFLAGS = \
	-target x86_64-unknown-windows \
	-nostdlib \
	-fuse-ld=lld-link \
	-Wl,-subsystem:efi_application \
	-Wl,-entry:efi_main

CFLAGS = \
	-std=c17 \
	-MMD \
	-Wall \
	-Wextra \
	-Wpedantic \
	-mno-red-zone \
	-ffreestanding 

all: $(TARGET) $(TEST_FILE)
	cd ../UEFI-GPT-image-creator/; \
	./$(DISK_IMG_PGM) -ad $(TEST_FILE); \
	./$(QEMU_SCRIPT)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $<
	cp $(TARGET) ../UEFI-GPT-image-creator/

$(TEST_FILE):
	echo TESTING > $@
	cp $@ ../UEFI-GPT-image-creator/

-include $(DEPENDS)

clean:
	rm -rf $(TARGET) *.efi *.o *.d

