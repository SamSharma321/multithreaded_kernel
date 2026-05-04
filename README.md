# Multithreaded Kernel - SAMOS

SAMOS is a small educational x86 kernel project. It boots from a custom boot
sector, switches the CPU into 32-bit protected mode, loads a freestanding C
kernel, initializes basic kernel services, enables paging, and begins handling
hardware interrupts.

The project is intentionally low-level: most of the interesting work happens by
talking directly to x86 CPU registers, VGA memory, ATA disk ports, and the
Programmable Interrupt Controller.

## Current Features

- 16-bit boot sector that switches into 32-bit protected mode.
- Flat GDT setup for kernel code and data segments.
- Kernel entry point written in assembly, then handoff to C.
- VGA text-mode output through memory at `0xB8000`.
- IDT setup with interrupt stubs in assembly and C handlers.
- Keyboard IRQ handling through the remapped PIC.
- Basic heap allocator with 4096-byte blocks.
- Kernel allocation wrappers: `kmalloc`, `kfree`, and `kzalloc`.
- 4 GB paging table creation with identity mapping.
- Example virtual address remap using `paging_set`.
- Basic ATA PIO disk read abstraction.
- Freestanding build flow using NASM, GCC/i686-elf-gcc, and LD.

## Boot Flow

1. BIOS loads `src/boot/boot.asm` at physical address `0x7C00`.
2. The boot sector sets up segment registers and a stack.
3. It installs a GDT and switches from real mode to protected mode.
4. In 32-bit mode, it reads kernel sectors from disk using ATA LBA PIO.
5. The kernel image is loaded at `0x00100000` or 1 MB.
6. Execution jumps to the linked kernel entry point in `src/kernel.asm`.
7. `src/kernel.asm` sets data segments, stack, A20, and PIC state.
8. `kernel_main()` in `src/kernel.c` initializes kernel subsystems.

## Kernel Initialization

`kernel_main()` currently initializes things in this order:

```c
terminal_initialize();
kheap_init();
disk_search_and_init();
idt_init();
paging_new_4gb(...);
paging_switch(...);
enable_paging();
enable_virtual_addressing();
enable_interrupts();
```

The order matters. Heap setup is needed before paging because the paging code
allocates page directories and page tables dynamically. The IDT must be loaded
before interrupts are enabled, or a hardware interrupt can cause a CPU fault and
reset the machine.

## Repository Structure

```text
.
|-- Makefile
|-- build.sh
|-- README.md
|-- Notes/
|-- old_files/
|-- bin/
|-- build/
`-- src/
    |-- boot/
    |-- disk/
    |-- idt/
    |-- io/
    |-- memory/
    |   |-- heap/
    |   `-- paging/
    |-- config.h
    |-- kernel.asm
    |-- kernel.c
    |-- kernel.h
    |-- linker.ld
    `-- status.h
```

## Top-Level Files

- `Makefile`: Builds the boot sector, kernel objects, linked kernel binary, and
  final disk image.
- `build.sh`: Convenience wrapper that sets cross-compiler environment variables,
  runs `make clean`, then runs `make all`.
- `README.md`: This project overview.
- `Notes/`: Learning notes for disks, assemblers, and kernel-development topics.
- `old_files/`: Older experiments and archived boot/build files.
- `bin/`: Generated boot/kernel/disk-image binaries.
- `build/`: Generated object files and intermediate linker output.

`bin/` and `build/` are build outputs, not source-of-truth files.

## Source Layout

### `src/boot/`

Contains the boot sector.

- `boot.asm`: 512-byte boot sector loaded by BIOS. It starts in 16-bit real mode,
  builds a GDT, enters protected mode, reads kernel sectors from ATA disk, then
  jumps to the kernel at 1 MB.

### `src/kernel.asm`

The first code executed after the bootloader jumps to the kernel. It:

- sets `ds`, `es`, `fs`, `gs`, and `ss`;
- sets up the kernel stack;
- enables the A20 line;
- configures the PIC interrupt mapping;
- calls `kernel_main()`.

### `src/kernel.c`

The main C kernel file. It contains:

- VGA text terminal functions;
- `print()`;
- the high-level kernel initialization sequence;
- the example virtual-address remap after paging is enabled.

### `src/idt/`

Interrupt Descriptor Table support.

- `idt.h`: IDT descriptor structures and interrupt function declarations.
- `idt.c`: Builds IDT entries, loads the IDTR descriptor, and defines C interrupt
  handlers.
- `idt.asm`: Assembly wrappers for interrupt handlers. These save registers,
  call C handlers, and return with `iret`.

Keyboard IRQ1 is remapped to interrupt vector `0x21`. Timer IRQ0 is remapped to
interrupt vector `0x20`.

### `src/io/`

Port I/O helpers.

- `io.h`: Function declarations for byte/word port input and output.
- `io.asm`: Assembly implementations of `insb`, `insw`, `outb`, and `outw`.
- `io.c`: C translation unit for the I/O module.

These functions are used by the PIC, keyboard controller, and ATA disk code.

### `src/memory/`

General memory helpers.

- `memory.h`: Declarations for kernel `memset` and `memcpy`.
- `memory.c`: Freestanding implementations of memory functions.

### `src/memory/heap/`

Kernel heap allocator.

- `heap.h`: Heap table structures and block metadata flags.
- `heap.c`: Block allocator implementation. It tracks free/taken blocks in a
  table and allocates contiguous 4096-byte blocks.
- `kheap.h`: Kernel-facing allocation API.
- `kheap.c`: Initializes the global kernel heap and exposes `kmalloc`, `kfree`,
  and `kzalloc`.

Heap configuration comes from `src/config.h`:

```c
#define SAMOS_HEAP_SIZE_BYTES  (100 * 1024 * 1024)
#define SAMOS_HEAP_BLOCK_SIZE  4096
#define SAMOS_HEAP_START_ADDR  0x01000000
#define SAMOS_HEAP_TABLE_ADDR  0x00007E00
```

The heap table stores allocator metadata. The heap start address is the usable
allocation region.

### `src/memory/paging/`

Paging support.

- `paging.h`: Paging constants, flags, and function declarations.
- `paging.c`: Creates a 4 GB identity-mapped paging structure and supports
  changing individual virtual-to-physical mappings.
- `paging.asm`: Loads `cr3` and enables paging by setting the paging bit in
  `cr0`.

The current setup creates 1024 page directory entries, each pointing to a page
table with 1024 entries. With 4096-byte pages, this covers the 32-bit 4 GB
address space.

### `src/disk/`

Basic ATA disk support.

- `disk.h`: Disk structure and public disk API.
- `disk.c`: Initializes a single real disk abstraction and reads sectors using
  ATA PIO ports.

Important ATA primary-bus ports used by the disk code:

```text
0x1F0  data
0x1F2  sector count
0x1F3  LBA low
0x1F4  LBA mid
0x1F5  LBA high
0x1F6  drive/head
0x1F7  command when writing, status when reading
```

Writing `0x20` to `0x1F7` issues the ATA `READ SECTORS` command.

### Shared Headers

- `src/config.h`: Global kernel constants such as heap addresses, sector size,
  segment selectors, and interrupt count.
- `src/status.h`: Shared status/error codes.
- `src/kernel.h`: Kernel-level declarations and VGA dimensions.
- `src/linker.ld`: Linker script that places the kernel at 1 MB and emits a flat
  binary.

## Building

Run:

```sh
./build.sh
```

The script sets:

```sh
PREFIX="$HOME/opt/cross"
TARGET=i686-elf
PATH="$PREFIX/bin:$PATH"
```

Then it runs:

```sh
make clean
make all
```

The Makefile prefers `i686-elf-gcc` when available. If it is not found, it falls
back to `gcc -m32`.

The final disk image is:

```text
bin/os.bin
```

## Running

With QEMU installed, run:

```sh
qemu-system-x86_64 -hda ./bin/os.bin
```

The boot image is built by concatenating:

1. `bin/boot.bin`
2. `bin/kernel.bin`
3. zero padding for extra disk sectors

## Debugging

The comments in `build.sh` include a GDB/QEMU flow:

```text
cd bin
gdb
add-symbol-file ../build/kernelfull.o 0x100000
target remote | qemu-system-x86_64 -hda os.bin -gdb stdio -S
```

Useful early breakpoints include:

```gdb
break kernel_main
break kernel.c:80
```

## Notes and Learning Material

- `Notes/DISKS.md`: ATA disks, sectors, LBA, and filesystem notes.
- `Notes/Assembler_Installation_Guide.txt`: Assembler/tooling setup notes.
- `Notes/Notes.txt`: General project notes.
- `old_files/`: Historical files kept for reference while learning.

## Current Limitations

This is still an early educational kernel, so several systems are intentionally
minimal:

- no scheduler or real multithreading implementation yet;
- no filesystem parser yet;
- disk access is ATA PIO only;
- interrupt handling is basic;
- keyboard input currently works at scan-code level;
- paging is mostly identity-mapped;
- heap allocation is simple block allocation;
- no userspace, processes, syscalls, or privilege separation yet.

## Development Notes

- This is freestanding code, so normal C library assumptions do not apply.
- Hardware interrupts should only be enabled after a valid IDT is loaded.
- IRQ handlers must acknowledge the PIC with EOI.
- Keyboard IRQ handlers must read port `0x60` to drain the keyboard controller.
- Paging structures must be page-aligned and visible through the active address
  mapping.
- Generated files in `bin/` and `build/` can be recreated with `./build.sh`.
