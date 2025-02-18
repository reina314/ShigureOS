# ShigureOS
ShigureOS is a Unix-like operating system that boasts its simplicity and high maintainability.<br>
This is my first personal OSDev project that is largely based on https://wiki.osdev.org/Meaty_Skeleton and its descendant https://github.com/itravers/PanicOS.<br>
It is currently developed for x86 systems (specifically i686-elf target), but in the future it might be translated to other architectures as well.

## Features
- GDT
- IDT (with ISR capability)

## Booting with QEMU
- To clean the development environment, you need to run the following command in the root project directory.
```bash
./clean.sh
```

- The following command will build all the necessary files and automatically boot into the OS with QEMU.
```bash
./run.sh
```