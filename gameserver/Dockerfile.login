FROM debian:bullseye

WORKDIR /gameserver
COPY bin/loginserver /gameserver/

ENTRYPOINT ["/gameserver/loginserver"]
