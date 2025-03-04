#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/shigure.kernel isodir/boot/shigure.kernel
cp sysroot/boot/shigure.initrd isodir/boot/shigure.initrd
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "ShigureOS (dev)" {
	multiboot /boot/shigure.kernel
	module /boot/shigure.initrd
}
EOF
grub-mkrescue -o shigure.iso isodir
