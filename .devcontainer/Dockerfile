FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    gdb \
    clangd \
    autoconf \
    automake \
    autotools-dev \
    curl \
    libmpc-dev \
    libmpfr-dev \
    libgmp-dev \
    gawk \
    bison \
    flex \
    texinfo \
    gperf \
    libtool \
    patchutils \
    bc \
    zlib1g-dev \
    libexpat-dev \
    ninja-build \
    wget \
    python3 \
    python3-pip \
    pkg-config \
    libglib2.0-dev \
    libpixman-1-dev \
    libslirp-dev \
    sudo \
    vim \
    nano \
    && rm -rf /var/lib/apt/lists/*

# Set working directory for toolchain installation
WORKDIR /opt

# Install RISC-V GNU toolchain
RUN git clone --recursive https://github.com/riscv/riscv-gnu-toolchain && \
    cd riscv-gnu-toolchain && \
    ./configure --prefix=/opt/riscv && \
    make && \
    cd .. && \
    rm -rf riscv-gnu-toolchain

# Install QEMU with RISC-V support
RUN wget https://download.qemu.org/qemu-8.1.0.tar.xz && \
    tar xJf qemu-8.1.0.tar.xz && \
    cd qemu-8.1.0 && \
    ./configure --target-list=riscv64-softmmu --prefix=/opt/qemu && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf qemu-8.1.0 qemu-8.1.0.tar.xz

# Add RISC-V toolchain and QEMU to PATH
ENV PATH="/opt/riscv/bin:/opt/qemu/bin:${PATH}"

# Set working directory for xv6
WORKDIR /txv6

# Default command
CMD ["/bin/bash"]
