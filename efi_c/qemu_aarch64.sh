#!/bin/sh

# Sendin' Out a TEST O S
qemu-system-aarch64 \
-drive format=raw,file=../UEFI-GPT-image-creator/test.hdd \
-bios QEMU_EFI_AARCH64.raw \
-name TESTOS \
-machine virt \
-cpu max \
-device virtio-gpu-pci \
-display gtk,gl=on,zoom-to-fit=off,window-close=on \
-usb \
-device qemu-xhci \
-device usb-kbd \
-device usb-mouse \
-rtc base=localtime \
-net none 

# If using both EFI.fd and VARS.fd, use these in place of the "-bios ..." line above
#-drive if=pflash,format=raw,file=QEMU_EFI_AARCH64.raw \
#-drive if=pflash,format=raw,file=QEMU_VARS_AARCH64.raw \
