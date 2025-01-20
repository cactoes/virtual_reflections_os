FROM randomdude/gcc-cross-x86_64-elf

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y binutils mtools
RUN apt-get install -y nasm
RUN apt-get install -y xorriso
RUN apt-get install -y grub-pc-bin
RUN apt-get install -y grub-common
# RUN apt-get install -y cmake
# RUN apt-get install -y qemu-utils

VOLUME /root/env
WORKDIR /root/env

# COPY build.sh /root/env/
# RUN chmod +x /root/env/build.sh
# CMD ["./build.sh"]