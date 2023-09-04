# uefi-dev
Repo to go along with the UEFI Development Series: 
https://www.youtube.com/playlist?list=PLT7NbkyNWaqZYHNLtOZ1MNxOt8myP5K0p

Most commits will roughly correspond to a given video in that playlist, once the videos are uploaded.

## repo layout
`efi_c` is the main directory; it should hold the lastest developments and is used for building 
EFI applications and test kernels.

`hello_efi` is a directory for what was shown in the first UEFI intro/overview video. It has a
basic "hello world" test EFI application.

There may eventually be other examples, such as a `load_kernel_efi` directory with minimal code
needed to load a kernel and/or "install" the bootloader EFI application to a target machine.

## dependencies
- C compiler supporting `-std=C17` or later, this repo assumes `clang` or `gcc`. Windows users
can use MinGW for gcc.
- `make`
- `qemu` with `ovmf` firmware (provided as `bios64.bin` in the "UEFI-GPT-image-creator" directory)

## building 
1. *NOTE: This git repo uses submodules*. To initialize everything use 
`git clone --recurse-submodules https://github.com/queso-fuego/uefi-dev` or after cloning 
use `git submodule update --remote` or `git pull --recurse-submodules`. 
2. `cd uefi-dev/UEFI-GPT-image-creator/ && make`
3. `cd ../efi_c/ && make` (this should launch qemu automatically)

Running/testing thereafter only needs `make` in the main `efi_c` directory, there's no need to make
the disk image creator program again. Unless you want to.

## running/testing
- Emulation: Use `make` in the `efi_c` directory to run using qemu/ovmf.
- Bare metal: On linux, I recommend using `dd` to write the disk image to e.g. a USB drive. 
Use `lsblk` or another way to find your USB's block device. An example using the default disk 
image name and assuming /dev/sdB for a USB could be `dd if=uefi-dev/UEFI-GPT-image-creator/test.hdd of=/dev/sdb bs=1M`.
Note the 1 megabyte block size to reduce write time, and writing to the drive directly with `sdb`, 
_not_ using a partition e.g. sdb1.
On Windows, `rufus` works well to write raw image files directly to a USB drive: 
https://rufus.ie

