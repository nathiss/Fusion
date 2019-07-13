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

RUN git clone https://github.com/nathiss/Fusion.git .
RUN git submodule init .
RUN git submodule update --recursive .

EXPOSE 80/tcp

CMD ["/bin/bash"]
