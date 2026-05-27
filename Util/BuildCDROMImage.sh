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
cp -v Katline/Katline ../limine.cfg ../limine/limine.sys ../limine/limine-cd.bin ../limine/limine-eltorito-efi.bin iso_root/

xorriso -as mkisofs -b limine-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot limine-eltorito-efi.bin \
        --efi-boot-part --efi-boot-image --protective-msdos-label \
        iso_root -o dhos.iso

../limine/limine-install dhos.iso
