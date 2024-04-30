.POSIX:
.PHONY: all clean

SOURCES = efi.c
OBJS = $(SOURCES:.c=.o)
DEPENDS = $(OBJS:.o=.d)
TARGET = BOOTX64.EFI

# Flat binary PIE kernel binary
#KERNEL = kernel.bin # ELF64 PIE kernel binary
#KERNEL = kernel.elf
# PE32+ PIE kernel binary
KERNEL = kernel.pe

# Uncomment disk image exe & shell script for Windows
#DISK_IMG_FOLDER = ..\UEFI-GPT-image-creator\
#DISK_IMG_PGM = write_gpt.exe
#QEMU_SCRIPT = qemu.bat

# Uncomment disk image exe & shell script for Linux
DISK_IMG_FOLDER = ../UEFI-GPT-image-creator/
DISK_IMG_PGM = write_gpt
QEMU_SCRIPT = qemu.sh

# Uncomment CC/LDFLAGS for gcc
CC = x86_64-w64-mingw32-gcc 
LDFLAGS = \
	-nostdlib \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment CC/LDFLAGS for clang
#CC = clang \
	-target x86_64-unknown-windows 
#LDFLAGS = \
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

all: $(TARGET) $(KERNEL)
	cd $(DISK_IMG_FOLDER); \
	./$(DISK_IMG_PGM) -ad $(KERNEL); \
	./$(QEMU_SCRIPT)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $<
	cp $(TARGET) $(DISK_IMG_FOLDER)

$(TEST_FILE):
	echo TESTING > $@
	cp $@ $(DISK_IMG_FOLDER)

# Flat binary - ELF/Clang
kernel.bin: kernel.c
	clang -c $(CFLAGS) -fPIE -o kernel.o $<
	ld -nostdlib -Tkernel.ld --oformat binary -pie -o $@ kernel.o 
	cp $@ $(DISK_IMG_FOLDER)

# Flat binary - MINGW GCC (PE)
#kernel.bin: kernel.c
#	x86_64-w64-mingw32-gcc -c $(CFLAGS) -fPIE -o kernel.o $<
#	x86_64-w64-mingw32-ld -nostdlib -Tkernel.ld --image-base=0 -pie -o kernel.obj kernel.o
#	objcopy -O binary kernel.obj $@
#	cp $@ $(DISK_IMG_FOLDER)

kernel.elf: kernel.c
	clang $(CFLAGS) -fPIE -e kmain -nostdlib -o $@ $<
	cp $@ $(DISK_IMG_FOLDER)

kernel.pe: kernel.c
	x86_64-w64-mingw32-gcc $(CFLAGS) -fPIE -e kmain -nostdlib -o $@ $<
	cp $@ $(DISK_IMG_FOLDER)

-include $(DEPENDS)

clean:
	rm -rf $(TARGET) *.efi *.o *.d *.bin *.elf *.pe

