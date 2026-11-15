# basic image for cross compilation
FROM randomdude/gcc-cross-x86_64-elf

# required tools for building the iso & compiling
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        binutils \
        grub-common \
        grub-efi-amd64-bin \
        grub-pc-bin \
        mtools \
        nasm \
        xorriso \
    && rm -rf /var/lib/apt/lists/*

# linking build scripts
WORKDIR /root/env
VOLUME /root/env

COPY --chmod=755 build_kernel.sh /root/env/build_kernel.sh

CMD ["/bin/bash", "/root/env/build_kernel.sh"]