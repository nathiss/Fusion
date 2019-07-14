#
# Copyright 2019 Kamil Rusin
#
# This file is a part of Fusion Server project.
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

COPY . .

RUN mkdir build
WORKDIR /usr/src/build

RUN cmake ..
 && make -j 10

CMD ["/usr/src/build/test/FusionServerTests"]
