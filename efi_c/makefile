# Aiming for POSIX.1-2024 compliance
.POSIX:

# Uncomment disk image program & shell script for Windows
#DISK_IMG_PGM    ::= write_gpt.exe
#QEMU_SCRIPT     ::= qemu.bat

# Uncomment disk image program & shell script for Linux
DISK_IMG_PGM    ::= write_gpt
QEMU_SCRIPT     ::= qemu.sh

DISK_IMG_FOLDER ::= ../UEFI-GPT-image-creator/

# Uncomment CC/LDFLAGS for EFI object - gcc
#EFICC       ::= x86_64-w64-mingw32-gcc
#EFI_LDFLAGS ::= \
	-nostdlib \
	-Wl,--subsystem,10 \
	-e efi_main 

# Uncomment CC/LDFLAGS for EFI object - clang
EFICC       ::= clang -target x86_64-unknown-windows 
EFI_LDFLAGS ::= \
	-nostdlib \
	-fuse-ld=lld-link \
	-Wl,-subsystem:efi_application \
	-Wl,-entry:efi_main

ELFCC ::= clang 
ELFLD ::= ld

PECC  ::= x86_64-w64-mingw32-gcc
PELD  ::= x86_64-w64-mingw32-ld

# Common CFLAGS
CFLAGS ::= \
	-std=c17 \
	-MMD \
	-Wall \
	-Wextra \
	-Wpedantic \
	-mno-red-zone \
	-ffreestanding \
	-fno-stack-protector	# Freestanding programs do not have libc stack protector functions

KERNEL_SRC     ::= kernel.c
KERNEL_CFLAGS  ::= $(CFLAGS) -fPIE
KERNEL_LDFLAGS ::= -e kmain -nostdlib -pie

EFISRC  ::= efi.c
EFIOBJ  ::= $(EFISRC:.c=.o)
DEPENDS ::= $(EFIOBJ:.o=.d) $(KERNEL_SRC:.c=.d)
EFI_APP ::= BOOTX64.EFI

# Uncomment kernel binary format to build
KERNEL ::= kernel.elf    # ELF64 PIE kernel binary
#KERNEL ::= kernel.pe     # PE32+ PIE kernel binary
#KERNEL ::= kernel.binelf # Flat binary PIE kernel from ELF file
#KERNEL ::= kernel.binpe  # Flat binary PIE kernel from PE file

FONT ::= ter-132n.psf	# PSF2 Bitmapped Font: Terminus 16x32 ISO8859-1

# Add kernel binary to new disk image
ADD_KERNEL = \
	cp $(KERNEL) $(DISK_IMG_FOLDER); \
	cp $(FONT) $(DISK_IMG_FOLDER); \
	cd $(DISK_IMG_FOLDER); \
	./$(DISK_IMG_PGM) -ad $(KERNEL) $(FONT);

all: $(DISK_IMG_FOLDER)/$(DISK_IMG_PGM) $(KERNEL) $(EFI_APP)
	cd $(DISK_IMG_FOLDER); \
	./$(QEMU_SCRIPT)

$(DISK_IMG_FOLDER)/$(DISK_IMG_PGM):
	cd $(DISK_IMG_FOLDER) && $(MAKE) 

$(EFI_APP): $(EFIOBJ)
	$(EFICC) $(EFI_LDFLAGS) -o $@ $<
	cp $@ $(DISK_IMG_FOLDER)
	$(ADD_KERNEL)

$(EFIOBJ): $(EFISRC)
	$(EFICC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(KERNEL_SRC)
	$(ELFCC) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ $<
	$(ADD_KERNEL)

kernel.pe: $(KERNEL_SRC)
	$(PECC) $(KERNEL_CFLAGS) $(KERNEL_LDFLAGS) -o $@ $<
	$(ADD_KERNEL)

kernel.binelf: $(KERNEL_SRC)
	$(ELFCC) -c $(KERNEL_CFLAGS) -o kernel.o $<
	$(ELFLD) $(KERNEL_LDFLAGS) -Tkernel.ld --oformat binary -o $@ kernel.o 
	$(ADD_KERNEL)

kernel.binpe: $(KERNEL_SRC)
	$(PECC) -c $(KERNEL_CFLAGS) -o kernel.o $<
	$(PELD) $(KERNEL_LDFLAGS) -Tkernel.ld --image-base=0 -o kernel.obj kernel.o
	objcopy -O binary kernel.obj $@
	$(ADD_KERNEL)

-include $(DEPENDS)

clean:
	rm -rf $(EFI_APP) $(KERNEL) *.bin* *.d *.efi *.elf *.o *.obj *.pe

