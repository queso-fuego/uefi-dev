# uefi-dev
This repo was originally to go along with the UEFI Development Series: 
https://www.youtube.com/playlist?list=PLT7NbkyNWaqZYHNLtOZ1MNxOt8myP5K0p

But can act as a basis for an efi application/bootloader example.
Those videos are done now for an intro series, and most commits before late 2024 roughly 
corresponded to a given video in that playlist. 
Future videos could be done on an as wanted or requested basis.

The goal was to learn EFI development as from-scratch as possible, for freestanding bare metal
programs, without using gnu-efi or posix-efi or other 3rd party premade libraries. 
Only using the UEFI specification and related documents, as feasible.

The current end result is headers for general UEFI definitions and helper functions,
and an EFI OS loader application to load an ELF/PE/flat binary file as a starter "kernel",
as well as print other various information discoverable/available via UEFI.
It builds, runs, and boots in both a QEMU emulated environment and on real hardware for
a dell xps 13 laptop ca. 2019.

Supported architectures are currently only x86_64, with stubs for Aarch64.

## repo layout
`efi_c` is the main directory; it should hold the lastest developments and is used for building 
EFI applications and test kernels.

`hello_efi` is a directory for what was shown in the first UEFI intro/overview video. It has a
basic "hello world" test EFI application.

## dependencies
- C compiler supporting `-std=c17` or later, and ability to output PE32+ object files. 
This repo assumes `clang` or `x86_64-w64-mingw32-gcc` (MinGW) for the default x86_64 target. 
Windows users can use the same MinGW gcc compiler. Clang, through LLVM, supports other targets for 
cross compiling with `-target`, while gcc has to use a different compiler for different target
architectures.
- `make`; gnu make preferably, unless posix defines ifeq() or other niceties.
- `qemu` with `ovmf` firmware, x86_64 is provided as `bios64.bin` in the "UEFI-GPT-image-creator" 
directory, aarch64 as QEMU_EFI_AARCH64.raw and VARS raw files.

## building 
1. *NOTE: This git repo uses submodules*. To initialize everything use 
`git clone --recurse-submodules https://github.com/queso-fuego/uefi-dev` or after cloning 
use `git submodule update --remote` or `git pull --recurse-submodules`. 
2. `cd /efi_c/ && make` (this should build the x86_64 target and launch qemu automatically).

Running/testing thereafter should only need `make` in the main `efi_c` directory.

## running/testing
- Emulation: Use `make` in the `efi_c` directory to run using qemu/ovmf.
- Bare metal: On linux, I recommend using `dd` to write the disk image to e.g. a USB drive. 
Use `lsblk` or another way to find your USB's block device. 
An example using the default disk image name and assuming /dev/sdB as a USB drive could be 
`dd if=uefi-dev/UEFI-GPT-image-creator/test.hdd of=/dev/sdb bs=1M`.
Note the 1 megabyte block size to reduce write time, and writing to the drive directly with `sdb`, 
_not_ using a partition e.g. sdb1.
On Windows, `rufus` works well to write raw image files directly to a USB drive: 
https://rufus.ie

