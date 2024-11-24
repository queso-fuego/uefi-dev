#!/bin/sh

# Sendin' Out a TEST O S
qemu-system-x86_64 \
-drive format=raw,file=../UEFI-GPT-image-creator/test.hdd \
-bios bios64.bin \
-m 256M \
-vga std \
-display gtk,gl=on,zoom-to-fit=off,window-close=on \
-name TESTOS \
-machine q35 \
-usb \
-device usb-mouse \
-rtc base=localtime \
-net none

