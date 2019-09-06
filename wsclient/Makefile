.PHONY: all clean

all: index.html

index.html: wsclient.cc library.js
	emcc -std=c++14 --bind -I../gameserver/network/export -I../gameserver/utils/export wsclient.cc ../gameserver/network/src/incoming_packet.cc ../gameserver/network/src/outgoing_packet.cc ../gameserver/utils/src/logger.cc -o index.html

clean:
	rm -f index.html index.js index.wasm
