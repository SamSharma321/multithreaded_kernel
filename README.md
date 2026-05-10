# Multithreaded Kernel - SAMOS

SAMOS is a linux-based x86 multithreaded kernel project. It boots from a custom boot
sector, switches the CPU into 32-bit protected mode, loads a freestanding C
kernel, initializes basic kernel services, enables paging, and begins handling
hardware interrupts.

The project is intentionally low-level: most of the interesting work happens by
talking directly to x86 CPU registers, VGA memory, ATA disk ports, and the
Programmable Interrupt Controller.

**Table of Contents:**
- [Requirements](#requirements)
- [Getting Started](#getting-started)
- [Building](#building)
- [Running](#running)
- [Debugging](#debugging)
- [Current Features](#current-features)

## Requirements

### System Requirements

- **Operating System:** Linux, macOS, or Windows (with WSL2)
- **Architecture:** x86_64 (to compile for x86/i686 target)
- **RAM:** Minimum 2 GB recommended
- **Disk Space:** ~500 MB for toolchain and build artifacts

### Required Software

1. **NASM (Netwide Assembler)** - for assembling boot sector and assembly code
   - Version: 2.13 or later
   - Website: https://www.nasm.us/

2. **GCC (with i686-elf support)** or **i686-elf-gcc cross-compiler**
   - For 32-bit x86 freestanding kernel compilation
   - Recommended: Use i686-elf cross-compiler toolchain

3. **GNU Binutils (ld, objdump, etc.)** 
   - Required for linking object files

4. **GNU Make**
   - For build automation

5. **QEMU** (for running and testing)
   - Version: 4.0 or later
   - Specifically: `qemu-system-x86_64` command
   - Website: https://www.qemu.org/

6. **GDB** (optional, for debugging)
   - GNU Debugger for kernel debugging with QEMU

7. **WSL2 or Docker** (optional, for Windows users)
   - For running Linux development environment on Windows

### Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| **Linux** | ✅ Full Support | Recommended for development |
| **macOS** | ✅ Full Support | Use Homebrew for package management |
| **Windows (WSL2)** | ✅ Full Support | Recommended setup for Windows |
| **Windows (Native)** | ⚠️ Limited | Not recommended; use WSL2 instead |

## Getting Started

### Installation

#### macOS (Using Homebrew)

```bash
# Install required tools
brew install nasm qemu
brew install binutils i686-elf-gcc

# Verify installations
nasm -version
qemu-system-x86_64 --version
i686-elf-gcc --version
```

#### Linux (Ubuntu/Debian)

```bash
# Install required tools
sudo apt-get update
sudo apt-get install -y nasm qemu-system-x86 build-essential

# Install i686-elf toolchain (if not available via package manager)
sudo apt-get install -y gcc-i686-linux-gnu binutils-i686-linux-gnu
# OR build from source (see notes below)
```

#### Linux (Fedora/RHEL)

```bash
# Install required tools
sudo dnf install -y nasm qemu-system-x86 gcc binutils

# For i686-elf cross-compiler
sudo dnf install -y mingw32-gcc mingw32-binutils
```

#### Windows (WSL2)

```bash
# In WSL2 terminal (Ubuntu):
sudo apt-get update
sudo apt-get install -y nasm qemu-system-x86 build-essential gcc-i686-linux-gnu

# Verify QEMU installation
qemu-system-x86_64 --version
```

### Cross-Compiler Setup (Optional but Recommended)

For best results, set up the i686-elf cross-compiler toolchain:

**Option 1: Pre-built binaries**

Visit: https://github.com/lordmilko/i686-elf-tools/releases

**Option 2: Build from source**

```bash
# Create installation directory
mkdir -p $HOME/opt/cross
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Download and build binutils and GCC
# (See Notes/Assembler_Installation_Guide.txt for detailed steps)
```

### Configure Environment

Add the following to your shell profile (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```

Then reload:

```bash
source ~/.bashrc  # or ~/.zshrc for macOS
```

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

### Prerequisites Check

Before building, verify all tools are installed:

```bash
# Check NASM
nasm -version

# Check GCC (either native or cross-compiler)
gcc -m32 --version  # or i686-elf-gcc --version

# Check LD (linker)
ld --version

# Check Make
make --version
```

### Quick Build

Run the build script:

```bash
./build.sh
```

This script:
1. Sets cross-compiler environment variables
2. Cleans previous build artifacts
3. Builds boot sector, kernel objects, and disk image
4. Output: `bin/os.bin` (bootable disk image)

### Manual Build Steps

If you prefer to build step-by-step:

```bash
# Set environment (if not already set)
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Clean previous builds
make clean

# Build all components
make all

# Output files
ls -la bin/os.bin      # Final bootable image
ls -la bin/boot.bin    # Boot sector (512 bytes)
ls -la bin/kernel.bin  # Kernel binary
```

### Build System Details

The Makefile:
- Prefers `i686-elf-gcc` when available
- Falls back to `gcc -m32` if cross-compiler not found
- Uses NASM for assembly files (`.asm`)
- Uses GCC for C files (`.c`)
- Uses GNU LD for linking
- Outputs final image: `bin/os.bin`

### Build Configuration

Edit these files to customize the build:

- `Makefile` - Build rules and compiler flags
- `src/config.h` - Kernel constants (heap size, interrupts, etc.)
- `src/linker.ld` - Kernel memory layout
- `build.sh` - Build script and environment setup

The Makefile prefers `i686-elf-gcc` when available. If it is not found, it falls
back to `gcc -m32`.

## Running

### Quick Start

With QEMU installed and kernel built, run:

```bash
qemu-system-x86_64 -hda ./bin/os.bin
```

This launches the kernel in QEMU emulator. You should see:
- Boot sector initialization
- Kernel mode setup
- VGA text output: "BIOS Execution start..."
- Interrupt handling demonstrations

### QEMU Options

For more control, use these flags:

```bash
# Basic execution (512 MB RAM, 1 CPU)
qemu-system-x86_64 -hda ./bin/os.bin

# With more RAM
qemu-system-x86_64 -hda ./bin/os.bin -m 1G

# With multiple CPU cores
qemu-system-x86_64 -hda ./bin/os.bin -smp 4

# With VNC display (for headless systems)
qemu-system-x86_64 -hda ./bin/os.bin -vnc :0

# Verbose output
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset

# Monitor console (Ctrl+Alt+2 to access)
qemu-system-x86_64 -hda ./bin/os.bin -monitor stdio
```

### Exit QEMU

- Press `Ctrl+Alt+Q` to quit
- Or use `Ctrl+C` in the terminal
- Or type `quit` in the monitor console (if using `-monitor stdio`)

### Disk Image Details

The boot image (`bin/os.bin`) is built by concatenating:

1. Boot sector (`bin/boot.bin`) - 512 bytes
2. Kernel image (`bin/kernel.bin`) - variable size
3. Zero padding - extra disk sectors (16 MB)

## Debugging

### Prerequisites for Debugging

Ensure you have GDB installed:

```bash
# macOS
brew install gdb

# Linux (Ubuntu/Debian)
sudo apt-get install -y gdb

# Linux (Fedora)
sudo dnf install -y gdb
```

### GDB + QEMU Debugging

**Terminal 1 - Start QEMU in Debug Mode:**

```bash
qemu-system-x86_64 -hda ./bin/os.bin -gdb stdio -S
```

Flags explained:
- `-gdb stdio` - Accept GDB connections on stdio
- `-S` - Pause CPU at startup (don't auto-run)

**Terminal 2 - Connect with GDB:**

```bash
cd bin
gdb

# In GDB shell:
(gdb) add-symbol-file ../build/kernelfull.o 0x100000
(gdb) target remote | qemu-system-x86_64 -hda os.bin -gdb stdio -S
(gdb) break kernel_main
(gdb) c
```

### Useful GDB Commands

```gdb
# Breakpoints
break kernel.c:80        # Break at line 80 in kernel.c
break kernel_main        # Break at kernel_main function
continue (c)             # Continue execution after break
delete 1                 # Delete breakpoint 1

# Viewing code
layout asm               # Show disassembly view
layout src               # Show source code view
layout split             # Show both source and asm

# Examining memory/registers
info registers           # Show all CPU registers
x/10x $esp              # Examine 10 hex words at ESP
x/s 0xb8000             # Examine string at VGA buffer
print/x $eax            # Print EAX in hex

# Single stepping
stepi (si)              # Step one instruction
step (s)                # Step one source line
nexti (ni)              # Next instruction (skip functions)
next (n)                # Next source line

# Stack traces
backtrace (bt)          # Show call stack
frame 0                 # Select frame 0

# Kernel dumps
info mem                # Show memory layout
```

### Common Debugging Scenarios

**Debugging boot process:**

```gdb
break src/boot/boot.asm:1  # Early breakpoint in boot
c                          # Continue to breakpoint
layout asm                 # View assembly
si                        # Step through boot code
```

**Debugging kernel initialization:**

```gdb
break kernel_main
c
list                  # Show source around breakpoint
step                  # Step into kernel_main
```

**Inspecting VGA output:**

```gdb
# VGA buffer starts at 0xB8000
x/80x 0xb8000       # Show first 80 characters
x/s 0xb8000         # Show as string
```

**Inspecting paging structures:**

```gdb
# Page directory at 0x01000000
x/1024x 0x01000000  # Show page directory entries
```

### Debugging Tips

1. **Rebuild with debug symbols** before debugging:
   ```bash
   make clean
   make all
   # Debug symbols are included in kernelfull.o
   ```

2. **Use conditional breakpoints** for loops:
   ```gdb
   break kernel.c:50 if i > 100
   ```

3. **Disable/enable breakpoints** for cleanup:
   ```gdb
   disable 1     # Disable breakpoint 1
   enable 1      # Re-enable it
   ```

4. **Watch for segment register changes** when debugging memory:
   ```gdb
   break src/kernel.asm:50  # Early segment setup
   ```

### Kernel-Specific Debugging Challenges

- **Paging enabled?** Virtual addresses may differ from physical
- **Interrupts disabled?** Debugging may seem frozen; check IF flag
- **Stack issues?** Check SS and SP in `src/kernel.asm`
- **IDT not loaded?** Interrupts will triple-fault; check `idt_init()`

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

## Troubleshooting

### Build Issues

**Problem:** `command not found: nasm`
```bash
# Solution: Install NASM
brew install nasm          # macOS
sudo apt-get install nasm  # Linux (Debian/Ubuntu)
```

**Problem:** `i686-elf-gcc: command not found`
```bash
# Solution 1: Install cross-compiler
brew install i686-elf-gcc  # macOS

# Solution 2: Use system GCC with -m32 flag
gcc -m32 --version

# Solution 3: Build from source
# See: Notes/Assembler_Installation_Guide.txt
```

**Problem:** `error: conflicting types for 'function_name'`
```bash
# Solution: Check header includes and function declarations
grep -r "function_name" src/
# Ensure consistent declarations in .h and .c files
```

**Problem:** Linker errors or missing symbols
```bash
# Solution: Rebuild from scratch
make clean
make all

# If still failing, check linker script
cat src/linker.ld
```

### Runtime Issues

**Problem:** QEMU starts but kernel doesn't load
```bash
# Check disk image was built
ls -la bin/os.bin

# Try verbose QEMU output
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset
```

**Problem:** Triple fault or immediate crash
```bash
# Check:
# 1. IDT is properly initialized (idt_init in kernel.c)
# 2. Interrupt handlers are defined (src/idt/idt.asm)
# 3. Segment registers set correctly (src/kernel.asm)

# Debug with GDB:
gdb
add-symbol-file build/kernelfull.o 0x100000
target remote | qemu-system-x86_64 -hda bin/os.bin -gdb stdio -S
break kernel_main
c
```

**Problem:** No output on screen
```bash
# Check VGA buffer isn't overwritten
# Add debug statements in kernel_main()
# Verify terminal_initialize() is called first in kernel_main()

# Try different QEMU video options:
qemu-system-x86_64 -hda bin/os.bin -vga std
qemu-system-x86_64 -hda bin/os.bin -vga cirrus
```

**Problem:** Keyboard input not working
```bash
# Check:
# 1. PIC is remapped (IRQ1 -> 0x21)
# 2. Keyboard handler reads port 0x60
# 3. EOI is sent to PIC after interrupt

# Add debug output in keyboard handler
# Check src/idt/idt.c for keyboard setup
```

## Useful Commands Reference

### Build & Clean

```bash
make              # Full rebuild
make clean        # Remove build artifacts
./build.sh        # Clean + build with env setup
```

### Inspection

```bash
# Check object files
objdump -t build/kernel.asm.o    # Symbol table
objdump -d build/kernel.o         # Disassembly
objdump -s build/kernel.o         # Section contents

# Check disk image
file bin/os.bin
ls -lah bin/
hexdump -C bin/boot.bin | head -20

# Check generated assembly
nasm -f bin src/boot/boot.asm -o /tmp/test.bin -l /tmp/test.lst
cat /tmp/test.lst
```

### Testing

```bash
# Run kernel in QEMU
qemu-system-x86_64 -hda ./bin/os.bin

# Run with GDB
qemu-system-x86_64 -hda ./bin/os.bin -gdb stdio -S

# Run with extended debugging
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset -D qemu.log

# Run with serial output
qemu-system-x86_64 -hda ./bin/os.bin -serial stdio
```

### Development Workflow

```bash
# Watch-style development loop (repeat in new terminal)
while true; do clear; make all 2>&1 | head -50; sleep 2; done

# Rebuild specific module
make clean
make ./build/disk/disk.o

# Check for unused variables/functions
make clean && make all 2>&1 | grep -i "warning"
```

## Documentation References

### Kernel Development Resources

- **OSDev.org:** https://wiki.osdev.org/
  - Excellent tutorials on bootloaders, kernels, and x86 architecture
  
- **Intel x86 Manuals:** https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
  - Official CPU reference (Volume 2: Instruction Set Reference)
  - Volume 3: System Programming
  
- **NASM Manual:** https://www.nasm.us/doc/
  - Assembly language syntax and directives
  
- **GCC for i686-elf:** https://gcc.gnu.org/
  - Cross-compiler documentation
  
- **QEMU Documentation:** https://www.qemu.org/docs/
  - Emulator options and debugging

### Project-Specific Documentation

- `Notes/DISKS.md` - Disk and filesystem architecture
- `Notes/Assembler_Installation_Guide.txt` - Toolchain setup
- `Notes/Notes.txt` - Development notes and findings
- `src/config.h` - Kernel configuration constants
- `src/linker.ld` - Memory layout and linking details

## Contributing & Next Steps

### Areas for Enhancement

1. **Scheduler & Multithreading**
   - Implement process/thread scheduling
   - Context switching on timer interrupt

2. **Filesystem**
   - Complete FAT16 filesystem implementation
   - File read/write operations

3. **User Space**
   - Ring 3 privilege level support
   - System calls interface

4. **Device Support**
   - SATA/AHCI disk interface
   - USB device handling
   - Network stack (IP/TCP)

### Development Tips

- Keep assembly code minimal and well-commented
- Test incrementally: add one feature, test thoroughly
- Use GDB extensively for complex debugging
- Document changes and architectural decisions
- Reference OSDev.org for best practices
- Study real kernel sources (Linux, xv6) for inspiration

## License & Attribution

This is an educational project inspired by x86 kernel development practices.
For learning purposes. See Notes/ for resources and references.
