#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/shigure.kernel isodir/boot/shigure.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "ShigureOS (0.0.0)" {
	multiboot /boot/shigure.kernel
}
EOF
grub-mkrescue -o shigure.iso isodir
