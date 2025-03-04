#!/bin/sh
set -e
. ./headers.sh

for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install)
done

gcc -o initrd_builder initrd_builder.c -g -O2 -Wall -Wextra
./initrd_builder shigure.initrd
mv shigure.initrd $SYSROOT/boot
