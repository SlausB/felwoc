# useful for testing Haxe generated code

FROM docker.io/ubuntu:22.04

RUN apt-get -y update && apt-get -y install \
    build-essential \
    git \
    software-properties-common \
    lsb-release \
    wget \
    gnupg \
    ccrypt \
	sudo \
	curl \
    gdb \
    # provide libneko.so.2 for Haxe: \
    neko \
    # to install Lime: \
    libgl1-mesa-dev libglu1-mesa-dev g++ g++-multilib gcc-multilib libasound2-dev libx11-dev libxext-dev libxi-dev libxrandr-dev libxinerama-dev libpulse-dev

# Haxe:
RUN mkdir -p /root/haxe
WORKDIR /root/haxe
RUN wget -c https://github.com/HaxeFoundation/haxe/releases/download/4.3.2/haxe-4.3.2-linux64.tar.gz -O - | tar -xz --strip-components=1
RUN echo 'export PATH="$PATH:/root/haxe"' >> ~/.bashrc
ENV PATH="/root/haxe:${PATH}"
RUN haxelib setup /usr/lib/haxe/lib
# Lime:
RUN git clone --recursive https://github.com/openfl/lime
RUN haxelib dev lime lime && \
    haxelib install format && \
    haxelib install hxp && \
    haxelib install hxcpp && \
    haxelib install openfl && \
    haxelib install utest && \
    haxelib run lime rebuild linux

WORKDIR /root/app
CMD /bin/bash

RUN apt update && apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libboost-all-dev \
    zlib1g-dev \
    libminizip-dev \


