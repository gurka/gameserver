.PHONY: all clean

SRC=wsclient.cc \
		network.cc \
		../gameserver/network/src/incoming_packet.cc \
		../gameserver/network/src/outgoing_packet.cc \
		../gameserver/utils/src/logger.cc \
		../gameserver/world/src/position.cc

INC=../gameserver/network/export \
		../gameserver/utils/export \
		../gameserver/world/export

all: index.html

index.html: $(SRC)
	emcc -std=c++14 --bind $(addprefix -I,$(INC)) -o $@ $^

clean:
	rm -f index.html index.js index.wasm
