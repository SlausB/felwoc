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
    cmake \
    ninja-build \
    libboost-all-dev \
    zlib1g-dev \
    libminizip-dev

WORKDIR /root/app
CMD /bin/bash

