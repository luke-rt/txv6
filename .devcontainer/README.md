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

# Run txv6 in qemu (no graphical output)
make qemu

# Run txv6 in gdb mode
make qemu-gdb
```

## Exiting QEMU

Run `make qemu` → Press `Ctrl-a`, then `x`

`Ctrl-a` won't cause anything to happen until you press `x`

## What's Included

- Ubuntu 22.04 base image
- RISC-V GNU toolchain (built from source)
- QEMU 8.1.0 with RISC-V support
- All necessary build tools (make, gcc, etc.)
- Python 3 for running test scripts
- VS Code C/C++ extensions for IntelliSense and debugging
- Makefile Tools extension

## Notes

- The initial build takes significant time (~30 minutes) due to compiling the RISC-V toolchain
- The Docker image is quite large (~5-6 GB) due to the toolchain
- Your workspace files are mounted into the container, so changes persist on your host machine
