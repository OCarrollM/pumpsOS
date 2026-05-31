#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/pumpsos.kernel isodir/boot/pumpsos.kernel


if [ -d user ]; then
    echo "Building user programs..."
    mkdir -p initrd
    for src in user/*.c; do
        [ -e "$src" ] || continue
        name=$(basename "$src" .c)
        echo "  $src -> initrd/$name.elf"
        i686-elf-gcc -ffreestanding -nostdlib -fno-pie \
            -fno-stack-protector -O2 -Wall -Wextra \
            -c "$src" -o "user/$name.o"
        i686-elf-gcc -ffreestanding -nostdlib -fno-pie -static \
            -T user/user.ld \
            "user/$name.o" -o "initrd/$name.elf"
    done
fi

if [ -d initrd ] && [ "$(ls -A initrd)" ]; then
    echo "Building initrd.tar..."
    (cd initrd && tar -cf ../isodir/boot/initrd.tar *)
else
    echo "Warning: initrd/ folder is empty or missing"
fi

cat > isodir/boot/grub/grub.cfg << EOF
menuentry "pumpsOS" {
    multiboot /boot/pumpsos.kernel
    module /boot/initrd.tar
    boot
}
EOF
GRUB_MKRESCUE=$(command -v grub-mkrescue || command -v i686-elf-grub-mkrescue)
$GRUB_MKRESCUE -o pumpsos.iso isodir