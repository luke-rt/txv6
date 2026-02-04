# VS Code Dev Container for xv6-riscv

This project includes a VS Code Dev Container configuration for building and developing xv6-riscv.

## Quick Start

### Using VS Code Dev Containers (Recommended):

1. Install the "Dev Containers" extension in VS Code
2. Open this folder in VS Code
3. When prompted, click "Reopen in Container" (or use Command Palette: `Dev Containers: Reopen in Container`)
4. Wait for the container to build (first time takes 30-60 minutes)
5. Once ready, you'll have a fully configured development environment!

### Manual Docker Setup (Alternative):

If you prefer not to use VS Code Dev Containers:

```bash
cd .devcontainer
docker-compose up -d
docker-compose exec xv6-dev bash
```

## Building and Running xv6

Once inside the container:

```bash
# Clean previous builds
make clean

# Build xv6
make

# Run xv6 in QEMU (note: QEMU graphical output won't work in Docker)
make qemu

# Run xv6 with QEMU in non-graphical mode
make qemu-nox
```

To exit QEMU, press `Ctrl-A` then `X`.

## Development Workflow

With VS Code Dev Containers:
1. Edit files directly in VS Code with full IntelliSense support
2. Use the integrated terminal to build: `make`
3. Run xv6: `make qemu-nox`
4. Debug using VS Code's debugging features with the RISC-V toolchain

Manual Docker workflow:
1. Edit files on your host machine
2. Changes are immediately available in the container via volume mount
3. Build and test inside the container
4. Use `make qemu-gdb` in one terminal and `riscv64-unknown-elf-gdb` in another for debugging

## What's Included

- Ubuntu 22.04 base image
- RISC-V GNU toolchain (built from source)
- QEMU 8.1.0 with RISC-V support
- All necessary build tools (make, gcc, etc.)
- Python 3 for running test scripts
- VS Code C/C++ extensions for IntelliSense and debugging
- Makefile Tools extension

## Notes

- The initial build takes significant time (~30-60 minutes) due to compiling the RISC-V toolchain
- The Docker image is quite large (~5-6 GB) due to the toolchain
- QEMU graphical output is not available in Docker; use `make qemu-nox` for console-only mode
- Your workspace files are mounted into the container, so changes persist on your host machine
