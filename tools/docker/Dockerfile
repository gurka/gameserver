FROM debian:bullseye

RUN apt-get -qq update -y && \
    apt-get -qq install -y --no-install-recommends \
      build-essential acl git cmake clang-9 libboost-date-time-dev python ca-certificates && \
    apt-get -y clean && \
    apt-get -y autoclean && \
    apt-get -y autoremove && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build

RUN git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

COPY tools/docker/entrypoint.sh /build
RUN chmod +x /build/entrypoint.sh
ENTRYPOINT ["/build/entrypoint.sh"]
