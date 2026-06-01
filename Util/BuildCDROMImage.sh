#!/usr/bin/env bash

if [ "$(basename $PWD)" != "Build" ] ; then
    echo 'Not in build directory. Please go into Build/'
    exit 1
fi

if [ ! -f "Katline/Katline" ] ; then
    echo 'Kernel not built. Please build the kernel.'
    exit 1
fi

if ! command -v xorriso &>/dev/null; then
    echo 'Xorisso not found. Please install.'
    exit 1
fi

set -e

echo 'Building limine-install'
make -C ../limine

mkdir -p iso_root
cp -v Katline/Katline ../limine.conf ../limine/limine-bios.sys ../limine/limine-bios-cd.bin ../limine/limine-uefi-cd.bin iso_root/
mkdir -p iso_root/EFI/BOOT
cp -v ../limine/BOOTX64.EFI iso_root/EFI/BOOT/BOOTX64.EFI

xorriso -as mkisofs -b limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot limine-uefi-cd.bin \
        --efi-boot-part --efi-boot-image --protective-msdos-label \
        iso_root -o dhos.iso

../limine/limine bios-install dhos.iso
