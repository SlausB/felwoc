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
    libgl1-mesa-dev libglu1-mesa-dev g++ g++-multilib gcc-multilib libasound2-dev libx11-dev libxext-dev libxi-dev libxrandr-dev libxinerama-dev

# nvm environment variables
ENV NVM_DIR /root/.nvm
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

# Bazel
RUN npm install -g @bazel/bazelisk
RUN wget -O /root/buildifier https://github.com/bazelbuild/buildtools/releases/download/v6.3.3/buildifier-linux-amd64
RUN chmod +x /root/buildifier
RUN echo 'export PATH="$PATH:/root"' >> ~/.bashrc
ENV PATH="/root:${PATH}"

# persisting Bazel's cache:
RUN mkdir -p ~/.cache/bazel
RUN ln -s /root/app/bazel_cache ~/.cache/bazel

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
    haxelib run lime rebuild linux

WORKDIR /root/app
CMD /bin/bash
