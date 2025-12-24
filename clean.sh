#!/bin/sh
set -e
. ./config.sh

for PROJECT in $PROJECTS; do
    (cd $PROJECT && $MAKE clean)
done

rm -rf SYSROOT
rm -rf isodir
rm -rf pumpsos.iso
