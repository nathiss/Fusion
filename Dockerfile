#
# Copyright 2019 Kamil Rusin
#
# This file is a part of Fusion Server project.
#
# Build:
# $ docker build --tag nathiss/fusion_server:0.1 .
#
# Run:
# $ docker run -it -p 8080:80 nathiss/fusion_server:0.1
#
FROM archlinux/base:latest

LABEL Name=FusionServer Version=0.1

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm cmake \
                          git \
                          clang \
                          boost \
                          boost-libs \
                          make

RUN [ -d /usr/src ] || mkdir /usr/src
WORKDIR /usr/src

RUN git clone "https://github.com/nathiss/Fusion.git" . \
 && git submodule init . \
 && git submodule update --recursive

RUN mkdir build
WORKDIR /usr/src/build

RUN cmake .. \
 && make -j 10

EXPOSE 80/tcp

CMD ["/usr/src/build/executable/FusionServerExecutable"]
