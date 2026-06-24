#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -d int,cpu_reset -no-reboot -no-shutdown -D qemu.log -serial file:serial.log -cdrom pumpsos.iso
