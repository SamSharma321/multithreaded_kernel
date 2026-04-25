PREFIX = $(HOME)/opt/cross
TARGET = i686-elf
CC = $(shell if command -v $(TARGET)-gcc >/dev/null 2>&1; then echo $(TARGET)-gcc; else echo gcc -m32; fi)
LD = $(PREFIX)/bin/$(TARGET)-ld

FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o ./build/memory/memory.o ./build/idt/idt.o ./build/io/io.asm.o ./build/io/io.o
INCLUDES = -I./src
CFLAGS = -g -ffreestanding -fno-pic -fno-pie -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

all: ./bin/boot.bin ./bin/kernel.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin # allocating 512 * 100 bytes for the kernel
	# Kernel size is now 51200 Bytes
	
./bin/kernel.bin: $(FILES)
	$(LD) -g --relocatable $(FILES) -o ./build/kernelfull.o
	$(LD) -T ./src/linker.ld -o ./bin/kernel.bin ./build/kernelfull.o

./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o

./build/io/io.asm.o: ./src/io/io.asm
	nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.o

./build/io/io.o: ./src/io/io.c
	$(CC) $(INCLUDES) -I./src/io $(CFLAGS) -std=gnu99 -c ./src/io/io.c -o ./build/io/io.o

./build/idt/idt.o: ./src/idt/idt.c
	$(CC) $(INCLUDES) -I./src/idt $(CFLAGS) -std=gnu99 -c ./src/idt/idt.c -o ./build/idt/idt.o

./build/kernel.o: ./src/kernel.c
	$(CC) $(INCLUDES) $(CFLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o

./build/memory/memory.o: ./src/memory/memory.c
	$(CC) $(INCLUDES) -I./src/memory $(CFLAGS) -std=gnu99 -c ./src/memory/memory.c -o ./build/memory/memory.o

clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf ./bin/os.bin
	rm -rf ./build/kernelfull.o
	rm -rf ./build/kernel.asm.o
	rm -rf ./build/kernel.o
	rm -rf ./build/io/io.asm.o
	rm -rf ./build/io/io.o
