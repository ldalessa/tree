# syntax=docker/dockerfile:1-labs
FROM debian:trixie

ARG USER_NAME
ARG LLVM_VERSION=19

RUN apt update
RUN apt -y full-upgrade
RUN apt -y install wget sudo make ninja-build valgrind git locales cmake g++ # basic tools
RUN apt -y install clang-tidy-$LLVM_VERSION clang-format-$LLVM_VERSION clang-tools-$LLVM_VERSION llvm-$LLVM_VERSION-dev lld-$LLVM_VERSION lldb-$LLVM_VERSION llvm-$LLVM_VERSION-tools libomp-$LLVM_VERSION-dev libc++-$LLVM_VERSION-dev libc++abi-$LLVM_VERSION-dev libclang-common-$LLVM_VERSION-dev libclang-$LLVM_VERSION-dev libclang-cpp$LLVM_VERSION-dev libunwind-$LLVM_VERSION-dev libclang-rt-$LLVM_VERSION-dev libpolly-$LLVM_VERSION-dev
RUN apt -y autoremove

# add a user
RUN useradd -rms /bin/bash -G sudo -p "$(openssl passwd -6 devcontainer)" $USER_NAME
RUN passwd -d $USER_NAME # passwordless login
USER $USER_NAME

# # set the environment
ENV PATH="/usr/lib/llvm-19/bin:$PATH"
ENV TERM="xterm-256color"
ENV EDITOR=nano
ENV GIT_EDITOR=nano
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8
