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
    gdb


RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER docker
CMD /bin/bash

# nvm environment variables
ENV NVM_DIR /home/docker/.nvm
ENV NODE_VERSION 20.5.1
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.1/install.sh | bash
# install node and npm
RUN chmod +x $NVM_DIR/nvm.sh
RUN $NVM_DIR/nvm.sh \
    && $NVM_DIR/nvm.sh install $NODE_VERSION \
    && $NVM_DIR/nvm.sh alias default $NODE_VERSION \
    && $NVM_DIR/nvm.sh use default
# add node and npm to path so the commands are available
ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
ENV PATH $NVM_DIR/versions/node/v$NODE_VERSION/bin:$PATH
# confirm installation
RUN node -v
RUN npm -v

RUN npm install -g @bazel/bazelisk

WORKDIR /home/docker/app
