FROM debian:bullseye

USER root
WORKDIR /build

RUN apt-get -qq update -y && \
    apt-get -qq install -y --no-install-recommends \
      build-essential acl git cmake clang-9 libboost-date-time-dev python ca-certificates docker.io && \
    apt-get -y clean && \
    apt-get -y autoclean && \
    apt-get -y autoremove && \
    rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    ./emsdk install latest

COPY tools/docker/entrypoint.sh /build
RUN chmod +x /build/entrypoint.sh
ENTRYPOINT ["/build/entrypoint.sh"]
