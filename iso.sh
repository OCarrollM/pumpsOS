#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/pumpsos.kernel isodir/boot/pumpsos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "pumpsOS" {
    multiboot /boot/pumpsos.kernel
}
EOF
grub-mkrescue -o pumpsos.iso isodir
