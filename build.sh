#!/bin/bash

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

make clean
make all

# qemu-system-x86_64 -hda ./bin/os.bin
# cd bin -> gdb -> add-symbol-file ../build/kernelfull.o 0x100000 -> 
# target remote | qemu-system-x86_64 -hda os.bin -gdb stdio -S -> break kernel.c:80 -> c -> layout asm
