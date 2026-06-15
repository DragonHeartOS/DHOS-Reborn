#!/usr/bin/env bash

SYSTEM='QEMU'

# Configuration
SMP_COUNT="${SMP_COUNT:-4}"
HEADLESS="${HEADLESS:-0}"

if [ "$1" = 'Bochs' ] ; then
    echo 'Bochs not yet implemented!'
    exit 1
fi

BOOT_MODE='DISK'
if [ -f 'dhos.iso' ] ; then
    BOOT_MODE='CDROM'
fi

has_kvm() {
    [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]
}

is_headless() {
    case "${HEADLESS,,}" in
        1|true|yes|on)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

if [ "$SYSTEM" = 'QEMU' ] ; then
    QEMU_BIN='qemu-system-x86_64'
    QEMU_ARGS="-m 1G -serial stdio -smp ${SMP_COUNT}"

    if is_headless ; then
        echo 'Running headless'
        QEMU_ARGS="$QEMU_ARGS -display none -monitor none"
    fi

    if has_kvm ; then
        echo 'Using KVM acceleration'
        QEMU_ARGS="$QEMU_ARGS -enable-kvm -cpu host,+x2apic"
    else
        echo 'KVM not available'
        QEMU_ARGS="$QEMU_ARGS -cpu max,+x2apic"
    fi

    if [ "$BOOT_MODE" = 'DISK' ] ; then
        QEMU_ARGS="$QEMU_ARGS -drive format=raw,file=_disk_image"
    elif [ "$BOOT_MODE" = 'CDROM' ] ; then
        QEMU_ARGS="$QEMU_ARGS -drive format=raw,media=cdrom,readonly=on,file=dhos.iso"
    else
        echo "Unknown boot mode: $BOOT_MODE"
        exit 1
    fi

    exec "$QEMU_BIN" $QEMU_ARGS
fi
