#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) \
    -no-reboot -no-shutdown \
    -serial file:serial.log \
    -cdrom pumpsos.iso \
    -drive file=pumpsos-disk.img,format=raw,if=ide,index=0,media=disk \
    -netdev user,id=n0,hostfwd=udp::7777-:7777 \
    -device e1000,netdev=n0 \
    -object filter-dump,id=f0,netdev=n0,file=net.pcap