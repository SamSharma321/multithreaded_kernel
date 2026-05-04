PREFIX = $(HOME)/opt/cross
TARGET = i686-elf
CC = $(shell if command -v $(TARGET)-gcc >/dev/null 2>&1; then echo $(TARGET)-gcc; else echo gcc -m32; fi)
LD = $(PREFIX)/bin/$(TARGET)-ld

FILES = ./build/kernel.asm.o ./build/kernel.o ./build/idt/idt.asm.o \
			./build/memory/memory.o ./build/idt/idt.o ./build/io/io.asm.o \
			./build/io/io.o ./build/memory/heap/heap.o ./build/memory/heap/kheap.o \
			./build/memory/paging/paging.o ./build/memory/paging/paging.asm.o\
			./build/disk/disk.o ./build/string/string.o ./build/fs/pparser.o \
			./build/disk/streamer.o ./build/fs/file.o

INCLUDES = -I./src
CFLAGS = -g -ffreestanding -fno-pic -fno-pie -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

all: ./bin/boot.bin ./bin/kernel.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin # allocating 512 * 100 bytes for the kernel
	# Copy a file over
	sudo cp ./hello.txt /mnt/d
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo umount /mnt/d
	
./bin/kernel.bin: $(FILES)
	mkdir -p $(@D)
	$(LD) -g --relocatable $(FILES) -o ./build/kernelfull.o
	$(LD) -T ./src/linker.ld -o ./bin/kernel.bin ./build/kernelfull.o

./bin/boot.bin: ./src/boot/boot.asm
	mkdir -p $(@D)
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

./build/kernel.asm.o: ./src/kernel.asm
	mkdir -p $(@D)
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/idt/idt.asm.o: ./src/idt/idt.asm
	mkdir -p $(@D)
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o

./build/io/io.asm.o: ./src/io/io.asm
	mkdir -p $(@D)
	nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.o

./build/io/io.o: ./src/io/io.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/io $(CFLAGS) -std=gnu99 -c ./src/io/io.c -o ./build/io/io.o

./build/idt/idt.o: ./src/idt/idt.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/idt $(CFLAGS) -std=gnu99 -c ./src/idt/idt.c -o ./build/idt/idt.o

./build/kernel.o: ./src/kernel.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) $(CFLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o

./build/memory/memory.o: ./src/memory/memory.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/memory $(CFLAGS) -std=gnu99 -c ./src/memory/memory.c -o ./build/memory/memory.o

./build/memory/heap/heap.o: ./src/memory/heap/heap.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/memory/heap $(CFLAGS) -std=gnu99 -c ./src/memory/heap/heap.c -o ./build/memory/heap/heap.o

./build/memory/heap/kheap.o: ./src/memory/heap/kheap.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/memory/heap $(CFLAGS) -std=gnu99 -c ./src/memory/heap/kheap.c -o ./build/memory/heap/kheap.o

./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/memory/paging $(CFLAGS) -std=gnu99 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	mkdir -p $(@D)
	nasm -f elf -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o

./build/disk/disk.o: ./src/disk/disk.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/disk $(CFLAGS) -std=gnu99 -c ./src/disk/disk.c -o ./build/disk/disk.o

./build/string/string.o: ./src/string/string.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/string $(CFLAGS) -std=gnu99 -c ./src/string/string.c -o ./build/string/string.o

./build/fs/pparser.o: ./src/fs/pparser.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/fs $(CFLAGS) -std=gnu99 -c ./src/fs/pparser.c -o ./build/fs/pparser.o

./build/disk/streamer.o: ./src/disk/streamer.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/disk $(CFLAGS) -std=gnu99 -c ./src/disk/streamer.c -o ./build/disk/streamer.o

./build/fs/file.o: ./src/fs/file.c
	mkdir -p $(@D)
	$(CC) $(INCLUDES) -I./src/fs $(CFLAGS) -std=gnu99 -c ./src/fs/file.c -o ./build/fs/file.o


clean:
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf ./bin/os.bin
	rm -rf ./build
