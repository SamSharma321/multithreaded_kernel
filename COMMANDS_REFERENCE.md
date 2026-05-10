# SAMOS Kernel - Command Reference Cheat Sheet

Quick reference for common commands used in SAMOS kernel development.

## Installation Commands

### macOS
```bash
brew install nasm qemu binutils i686-elf-gcc make gdb
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install nasm qemu-system-x86 build-essential gcc-i686-linux-gnu binutils-i686-linux-gnu gdb
```

### Linux (Fedora)
```bash
sudo dnf install nasm qemu-system-x86 gcc binutils make gdb
```

### WSL2
```bash
sudo apt-get update
sudo apt-get install nasm qemu-system-x86 build-essential gcc-i686-linux-gnu gdb
```

## Environment Setup

```bash
# Add to ~/.bashrc or ~/.zshrc
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Reload profile
source ~/.bashrc
# or for macOS
source ~/.zshrc
```

## Build Commands

```bash
# Full build (clean + compile)
./build.sh

# Manual build process
make clean                 # Remove build artifacts
make all                   # Build all components
make help                  # Show make targets

# Rebuild specific component
make clean && make build/kernel.o

# Check compiler
which i686-elf-gcc
gcc -m32 --version
```

## Running

```bash
# Basic execution
qemu-system-x86_64 -hda ./bin/os.bin

# With 1 GB RAM
qemu-system-x86_64 -hda ./bin/os.bin -m 1G

# With 4 CPU cores
qemu-system-x86_64 -hda ./bin/os.bin -smp 4

# With debug output
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset

# With serial console
qemu-system-x86_64 -hda ./bin/os.bin -serial stdio

# With VNC display (headless)
qemu-system-x86_64 -hda ./bin/os.bin -vnc :0

# Save debug log
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset -D qemu.log

# For GDB debugging
qemu-system-x86_64 -hda ./bin/os.bin -gdb stdio -S

# Exit QEMU
Ctrl+Alt+Q  # or Ctrl+C
```

## Debugging with GDB

### Terminal 1: Start QEMU
```bash
qemu-system-x86_64 -hda ./bin/os.bin -gdb stdio -S
```

### Terminal 2: Connect GDB
```bash
cd bin
gdb
```

### GDB Commands
```gdb
# Connect to QEMU
(gdb) target remote | qemu-system-x86_64 -hda os.bin -gdb stdio -S

# Load kernel symbols at 1MB
(gdb) add-symbol-file ../build/kernelfull.o 0x100000

# Set breakpoint
(gdb) break kernel_main
(gdb) break kernel.c:80
(gdb) break src/kernel.asm:50

# Execute
(gdb) run
(gdb) continue (c)

# Stepping
(gdb) step (s)          # Step source line
(gdb) stepi (si)        # Step CPU instruction
(gdb) next (n)          # Step over function
(gdb) nexti (ni)        # Next instruction

# View code
(gdb) layout asm        # Assembly view
(gdb) layout src        # Source code view
(gdb) layout split      # Both source and asm
(gdb) list              # List source around breakpoint

# Examine data
(gdb) print $eax        # Print register
(gdb) print/x $eax      # Print in hex
(gdb) x/10x $esp        # Examine 10 hex words
(gdb) x/s 0xb8000       # String at VGA buffer

# Stack/calls
(gdb) backtrace (bt)    # Show call stack
(gdb) frame 0           # Select frame
(gdb) info registers    # Show all registers
(gdb) disassemble       # Show disassembly

# Delete breakpoint
(gdb) delete 1

# Quit GDB
(gdb) quit (q)
```

## Inspection Commands

### Disk/Binary Files
```bash
# Check image
file bin/os.bin
ls -lah bin/

# Examine boot sector
hexdump -C bin/boot.bin | head -20
od -x bin/boot.bin | head

# Compare disk image size
du -sh bin/os.bin
```

### Object Files & Symbols
```bash
# List symbols
objdump -t build/kernel.asm.o
objdump -t build/kernel.o

# Disassembly
objdump -d build/kernel.o | head -50
objdump -d build/kernel.asm.o

# Section info
objdump -h build/kernel.o
readelf -S build/kernel.o

# Symbol names
nm build/kernel.o
nm -S build/kernelfull.o

# Full analysis
objdump -x build/kernel.o
```

### Assembly Code
```bash
# Assemble with listing
nasm -f bin ./src/boot/boot.asm -o ./boot.bin -l boot.lst

# View assembly listing
cat boot.lst | head -50

# Check syntax only
nasm -f bin ./src/boot/boot.asm -o /dev/null
```

### Source Code
```bash
# Find symbols in code
grep -r "kernel_main" src/
grep -r "handle_zero" src/
grep -n "function_name" src/file.c

# Count lines
wc -l src/**/*.c
find src -name "*.c" -exec wc -l {} +

# Find TODO/FIXME comments
grep -r "TODO\|FIXME" src/
```

## Development Workflow

### Quick Edit-Build-Run Cycle
```bash
# Edit file
vim src/kernel.c

# Rebuild
make clean && make all

# Run
qemu-system-x86_64 -hda ./bin/os.bin
```

### Watch-Style Rebuild
```bash
# Rebuild automatically on changes
while true; do clear; make all 2>&1 | tail -20; sleep 2; done
```

### Continuous Build & Test
```bash
# In one terminal
while true; do make clean && make all && sleep 1; done

# In another terminal
qemu-system-x86_64 -hda ./bin/os.bin
```

### Build with Warnings Only
```bash
make all 2>&1 | grep -i warning
```

## Common Issues & Fixes

### Can't Find NASM
```bash
which nasm
brew install nasm          # macOS
sudo apt install nasm      # Linux
```

### Can't Find Compiler
```bash
which i686-elf-gcc
which gcc
gcc -m32 --version

# Install if missing
brew install i686-elf-gcc  # macOS
sudo apt install gcc-i686-linux-gnu  # Linux
```

### Build Fails with Linking Error
```bash
# Clean and try again
make clean
make all

# Check linker script
cat src/linker.ld
```

### QEMU Won't Start Kernel
```bash
# Verify image
ls -la bin/os.bin

# Check image integrity
file bin/os.bin

# Verbose output
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset -D qemu.log
cat qemu.log
```

### GDB Can't Connect
```bash
# Make sure debug symbols are present
ls -la build/kernelfull.o

# Rebuild if missing
make clean && make all

# Try adding symbol file again in GDB
(gdb) add-symbol-file ../build/kernelfull.o 0x100000
```

## Performance Analysis

```bash
# Measure build time
time make all

# Check executable size
ls -lh bin/kernel.bin
size build/kernelfull.o

# Memory usage
free -h
top -l 1 | head

# Disk usage
du -sh bin/
du -sh build/
```

## Repository Cleanup

```bash
# Remove all build artifacts
make clean

# Remove build directory completely
rm -rf build/

# Remove binaries
rm -rf bin/

# Remove only object files
find build -name "*.o" -delete

# Git cleanup
git clean -fd           # Remove untracked files
git clean -fdx          # Also remove ignored files
```

## Useful Info Commands

```bash
# Check CPU flags (need x86 bits)
grep flags /proc/cpuinfo | head -1

# Check QEMU version
qemu-system-x86_64 --version

# List QEMU machines
qemu-system-x86_64 -M help

# Check available QEMU CPUs
qemu-system-x86_64 -cpu help

# GDB version
gdb --version

# NASM version
nasm -version

# GCC version
gcc --version
i686-elf-gcc --version
```

## Documentation

```bash
# Read project README
less README.md

# Read setup guide
less SETUP_GUIDE.md

# Check kernel configuration
cat src/config.h

# Check linker script
cat src/linker.ld

# View Makefile
cat Makefile

# Check notes
ls -la Notes/
cat Notes/DISKS.md
```

## Tips & Tricks

```bash
# Rebuild just the boot sector
nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

# Create minimal test
echo "mov ax, 0x7c0" | nasm -f bin -

# Count .c and .h files
find src -name "*.c" -o -name "*.h" | wc -l

# Check function count
grep -r "^[a-zA-Z_].*(.*).*{" src/ --include="*.c" | wc -l

# Quick syntax check
for f in src/**/*.c; do gcc -fsyntax-only "$f" && echo "✓ $f"; done

# Create backup
tar czf samos-kernel-backup.tar.gz src/ build.sh Makefile

# Compare files
diff -u old_file new_file
diff -r src/ ../other_kernel/src/
```

---

**Legend:**
- `$` = Shell prompt (don't type)
- `(gdb)` = GDB prompt (don't type)
- `#` = Command comment
- `...` = Omitted for brevity

**Last Updated:** April 2026
**SAMOS Kernel Project**
