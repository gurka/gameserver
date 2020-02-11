FROM debian:bullseye

WORKDIR /gameserver
COPY docker_build/bin/loginserver /gameserver/

EXPOSE 7171
EXPOSE 8171
ENTRYPOINT ["/gameserver/loginserver"]
