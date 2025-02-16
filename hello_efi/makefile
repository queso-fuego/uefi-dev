.POSIX:
.PHONY: all clean

SOURCE = efi.c
TARGET = BOOTX64.EFI

# Uncomment for gcc
#CC = x86_64-w64-mingw32-gcc \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment for clang
CC = clang \
	-target x86_64-unknown-windows \
	-fuse-ld=lld-link \
	-Wl,-subsystem:efi_application \
	-Wl,-entry:efi_main

CFLAGS = \
	-std=c17 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-mno-red-zone \
	-ffreestanding \
	-nostdlib 

DISK_IMG_FOLDER = ../UEFI-GPT-image-creator
DISK_IMG_PGM    = write_gpt

all: $(DISK_IMG_FOLDER)/$(DISK_IMG_PGM) $(TARGET)
	cd $(DISK_IMG_FOLDER) && ./qemu.sh

$(DISK_IMG_FOLDER)/$(DISK_IMG_PGM):
	cd $(DISK_IMG_FOLDER) && $(MAKE)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $<
	cp $(TARGET) $(DISK_IMG_FOLDER); \
	cd $(DISK_IMG_FOLDER) && ./$(DISK_IMG_PGM)

clean:
	rm -rf $(TARGET)

