#
# Copyright 2019 Kamil Rusin
#
# This file is a part of Fusion Server project.
#
FROM archlinux/base:latest

LABEL Name=FusionServer Version=0.1

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm cmake \
                          clang \
                          boost \
                          boost-libs \
                          make

RUN mkdir -p /app/src \
 && mkdir -p /app/build \
 && mkdir -p /app/log

COPY . /app/src

WORKDIR /app/build
RUN cmake -DFUSION_DOCS=OFF -DFUSION_TEST=OFF /app/src \
 && cmake --build . --config Release --target FusionServerExe -j 10

RUN mv /app/src/docker-config.json /config.json \
 && mv /app/build/executable/FusionServerExe /Server

EXPOSE 80/tcp
VOLUME /app/log

CMD ["/Server", "/config.json"]
