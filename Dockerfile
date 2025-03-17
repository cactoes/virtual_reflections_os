# basic image for cross compilation
FROM randomdude/gcc-cross-x86_64-elf

# required tools for building the ISO
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y \
    binutils \
    mtools \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-common

# linking with the project
VOLUME /root/env
WORKDIR /root/env

# copying the build script
COPY build.sh /root/env/
RUN chmod +x /root/env/build.sh
CMD [ "/bin/bash", "/root/env/build.sh" ]