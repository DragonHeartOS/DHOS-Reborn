#!/usr/bin/env bash
set -e

if [ "$(basename "$PWD")" != "Build" ] && [ "$(basename "$PWD")" != "build" ]; then
    echo 'Not in build directory. Please go into Build/'
    exit 1
fi

if [ ! -f "Katline/Katline" ]; then
    echo 'Kernel not built. Please build the kernel.'
    exit 1
fi

OS="$(uname -s)"
IMAGE="_disk_image"
MOUNT_DIR="img_mount"

cleanup() {
    sync || true

    if [ "$OS" = "Linux" ] && [ -n "${USED_LOOPBACK:-}" ]; then
        sudo umount "$MOUNT_DIR" 2>/dev/null || true
        sudo losetup -d "$USED_LOOPBACK" 2>/dev/null || true
    fi

    if [ "$OS" = "Darwin" ] && [ -n "${MAC_DISK:-}" ]; then
        hdiutil detach "$MAC_DISK" 2>/dev/null || true
    fi

    rm -rf "$MOUNT_DIR"
}
trap cleanup EXIT

echo 'Building Limine'
make -C ../limine

echo 'Creating disk image'
rm -f "$IMAGE"
dd if=/dev/zero bs=1m count=64 of="$IMAGE"

mkdir -p "$MOUNT_DIR"

if [ "$OS" = "Linux" ]; then
    if ! command -v parted >/dev/null 2>&1; then
        echo 'parted not found. Please install.'
        exit 1
    fi

    echo 'Creating GPT partition table'
    parted -s "$IMAGE" mklabel gpt

    echo 'Creating ESP partition'
    parted -s "$IMAGE" mkpart ESP fat32 2048s 100%
    parted -s "$IMAGE" set 1 esp on

    echo 'Installing Limine BIOS stages onto the image'
    ../limine/limine bios-install "$IMAGE"

    echo 'Mounting loopback device'
    USED_LOOPBACK="$(sudo losetup -Pf --show "$IMAGE")"

    echo 'Formatting ESP as FAT32'
    sudo mkfs.fat -F 32 "${USED_LOOPBACK}p1"

    echo 'Mounting partition'
    sudo mount "${USED_LOOPBACK}p1" "$MOUNT_DIR"

elif [ "$OS" = "Darwin" ]; then
    echo 'Attaching image'
    MAC_DISK="$(hdiutil attach -nomount "$IMAGE" | awk 'NR==1 {print $1}')"

    echo 'Creating GPT + FAT32 ESP'
    diskutil partitionDisk "$MAC_DISK" GPT FAT32 ESP 100%

    echo 'Detaching before Limine BIOS install'
    hdiutil detach "$MAC_DISK"
    MAC_DISK=""

    echo 'Installing Limine BIOS stages onto the image'
    ../limine/limine bios-install "$IMAGE"

    echo 'Re-attaching image'
    MAC_DISK="$(hdiutil attach -nomount "$IMAGE" | awk 'NR==1 {print $1}')"

    echo 'Mounting ESP'
    diskutil mount -mountPoint "$PWD/$MOUNT_DIR" "${MAC_DISK}s1"

else
    echo "Unsupported OS: $OS"
    exit 1
fi

echo 'Copying files'
sudo mkdir -p "$MOUNT_DIR/EFI/BOOT"

sudo cp -v Katline/Katline "$MOUNT_DIR/"
sudo cp -v ../limine.conf "$MOUNT_DIR/limine.conf"

if [ -f ../limine/limine-bios.sys ]; then
    sudo cp -v ../limine/limine-bios.sys "$MOUNT_DIR/"
else
    sudo cp -v ../limine/limine.sys "$MOUNT_DIR/"
fi

sudo cp -v ../limine/BOOTX64.EFI "$MOUNT_DIR/EFI/BOOT/"

echo 'Done'
