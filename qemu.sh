#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) \
    -d int -D qemu.log -no-reboot -no-shutdown \
    -serial file:serial.log \
    -cdrom pumpsos.iso \
    -drive file=pumpsos-disk.img,format=raw,if=ide,index=0,media=disk
