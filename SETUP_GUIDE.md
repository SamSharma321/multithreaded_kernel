# SAMOS Kernel - Quick Setup Guide

This guide provides step-by-step instructions to set up, build, and run the SAMOS multithreaded kernel project.

## 1. System Requirements

### Minimum Requirements
- **OS:** Linux, macOS, or Windows (WSL2)
- **RAM:** 2 GB
- **Disk Space:** 500 MB

### Supported Platforms
- ✅ **Linux** (Recommended)
- ✅ **macOS** (via Homebrew)
- ✅ **Windows WSL2** (Recommended)
- ⚠️ **Windows Native** (Not recommended)

## 2. Required Software

### Core Tools
1. **NASM** - Assembler (v2.13+)
2. **GCC/i686-elf-gcc** - C Compiler for x86
3. **GNU Binutils** - Linker and tools
4. **GNU Make** - Build system
5. **QEMU** - Emulator (v4.0+)
6. **GDB** - Debugger (optional, for debugging)

## 3. Installation by Platform

### macOS

```bash
# Install via Homebrew
brew install nasm qemu binutils i686-elf-gcc

# Verify
nasm -version
qemu-system-x86_64 --version
i686-elf-gcc --version
```

### Linux (Ubuntu/Debian)

```bash
# Install packages
sudo apt-get update
sudo apt-get install -y nasm qemu-system-x86 build-essential

# For i686-elf cross-compiler
sudo apt-get install -y gcc-i686-linux-gnu binutils-i686-linux-gnu

# OR build cross-compiler from source (see README.md)
```

### Linux (Fedora/RHEL)

```bash
sudo dnf install -y nasm qemu-system-x86 gcc binutils
sudo dnf install -y mingw32-gcc mingw32-binutils
```

### Windows (WSL2)

```bash
# In WSL2 terminal (Ubuntu):
sudo apt-get update
sudo apt-get install -y nasm qemu-system-x86 build-essential gcc-i686-linux-gnu

# Verify
qemu-system-x86_64 --version
```

## 4. Environment Setup (Optional but Recommended)

### Create Cross-Compiler Directory

```bash
# Download pre-built i686-elf toolchain
# From: https://github.com/lordmilko/i686-elf-tools/releases

# OR create directory for custom build
mkdir -p $HOME/opt/cross
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```

### Add to Shell Profile

Add to `~/.bashrc` or `~/.zshrc`:

```bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```

Then reload:
```bash
source ~/.bashrc  # or ~/.zshrc
```

## 5. Clone & Navigate to Repository

```bash
# Clone the repository
git clone https://github.com/SamSharma321/multithreaded_kernel.git
cd multithreaded_kernel

# Verify structure
ls -la
# Should see: bin/, build/, src/, Makefile, build.sh, README.md
```

## 6. Build the Kernel

### Quick Build

```bash
./build.sh
```

### Manual Build

```bash
# Set environment (if not in profile)
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Clean previous builds
make clean

# Build everything
make all

# Check outputs
ls -la bin/os.bin
```

### Build Verification

After successful build, you should have:
- `bin/boot.bin` - Boot sector (512 bytes)
- `bin/kernel.bin` - Kernel binary
- `bin/os.bin` - Complete disk image (~16 MB)

## 7. Run the Kernel

### Basic Execution

```bash
qemu-system-x86_64 -hda ./bin/os.bin
```

### With Options

```bash
# More RAM
qemu-system-x86_64 -hda ./bin/os.bin -m 1G

# Multiple CPUs
qemu-system-x86_64 -hda ./bin/os.bin -smp 4

# With verbose output
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset
```

### Exit QEMU
- Press `Ctrl+Alt+Q`
- Or `Ctrl+C` in terminal
- Or `quit` in monitor console

## 8. Debug the Kernel

### Terminal 1: Start QEMU with GDB Support

```bash
qemu-system-x86_64 -hda ./bin/os.bin -gdb stdio -S
```

### Terminal 2: Connect GDB

```bash
cd bin
gdb

# In GDB prompt:
(gdb) add-symbol-file ../build/kernelfull.o 0x100000
(gdb) target remote | qemu-system-x86_64 -hda os.bin -gdb stdio -S
(gdb) break kernel_main
(gdb) c
(gdb) layout asm
```

### Useful GDB Commands

```gdb
break kernel.c:80         # Set breakpoint
continue (c)              # Continue execution
stepi (si)                # Step one instruction
layout asm                # Show assembly
info registers            # Show CPU registers
x/10x $esp               # Examine memory
```

## 9. Troubleshooting

### Build Fails: "nasm not found"
```bash
# Install NASM
brew install nasm           # macOS
sudo apt install nasm       # Linux
```

### Build Fails: "i686-elf-gcc not found"
```bash
# Option 1: Use system GCC with -m32
export CC="gcc -m32"

# Option 2: Install cross-compiler
brew install i686-elf-gcc   # macOS

# Option 3: Build from source
# See: Notes/Assembler_Installation_Guide.txt
```

### Kernel Won't Boot in QEMU
```bash
# Verify disk image exists
ls -la bin/os.bin

# Check with verbose output
qemu-system-x86_64 -hda ./bin/os.bin -d int,cpu_reset -D qemu.log
cat qemu.log | head -100
```

### Debug with GDB Not Working
```bash
# Rebuild with debug symbols
make clean
make all

# Ensure kernelfull.o exists
ls -la build/kernelfull.o
```

## 10. Next Steps

1. **Read the full README.md** - Comprehensive documentation
2. **Explore the source code** - Start with `src/kernel.c`
3. **Study the Notes** - See `Notes/` directory for learning materials
4. **Use GDB** - Debug and trace kernel execution
5. **Modify and experiment** - Change code and rebuild to understand

## 10. Project Resources

### Documentation
- **Full README:** `README.md` - Complete project documentation
- **Disk Info:** `Notes/DISKS.md` - Filesystem and disk architecture
- **Setup Guide:** `Notes/Assembler_Installation_Guide.txt` - Toolchain building
- **Notes:** `Notes/Notes.txt` - Development insights

### External Resources
- **OSDev Wiki:** https://wiki.osdev.org/ (Excellent OS dev tutorials)
- **Intel x86 Manuals:** https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- **NASM Manual:** https://www.nasm.us/doc/
- **QEMU Docs:** https://www.qemu.org/docs/

## Common Commands Reference

```bash
# Build and run
./build.sh && qemu-system-x86_64 -hda ./bin/os.bin

# Clean and rebuild
make clean && make all

# Inspect disk image
file bin/os.bin
hexdump -C bin/boot.bin | head -20

# Debug with GDB
qemu-system-x86_64 -hda ./bin/os.bin -gdb stdio -S
# (in another terminal) gdb, then add-symbol-file ...

# Check build artifacts
ls -la build/
objdump -d build/kernel.o | head -50

# Watch rebuild loop (for development)
while true; do clear; make all 2>&1 | tail -20; sleep 2; done
```

## Getting Help

- **Check README.md** for detailed documentation
- **Consult Notes/** directory for learning materials
- **Enable GDB debugging** for complex issues
- **Check QEMU logs** with `-D qemu.log` option
- **Reference OSDev.org** for kernel development best practices

---

**Last Updated:** April 2026
**Project:** SAMOS Multithreaded Kernel
**Repository:** https://github.com/SamSharma321/multithreaded_kernel
