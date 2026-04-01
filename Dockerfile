# basic image for cross compilation
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# required tools for building the iso & compiling
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y \
    build-essential \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    make \
    git

# linking with the project
VOLUME /root/env
WORKDIR /root/env

# copying the build script
COPY build_kernel.sh /root/env/
RUN chmod +x /root/env/build_kernel.sh
CMD [ "/bin/bash", "/root/env/build_kernel.sh" ]