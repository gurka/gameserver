CXX      = g++
CXXFLAGS = -c -Wall -Werror -std=c++11 -MMD
LDFLAGS  = -lboost_system -lpthread

# Loginserver
SOURCE_LOGIN  = src/loginserver/loginserver.cc \
                src/utils/logger.cc \
                src/common/accountmanager/accountmgr.cc \
                src/common/network/connection.cc \
                src/common/network/incomingpacket.cc \
                src/common/network/outgoingpacket.cc \
                src/common/network/acceptor.cc \
                src/common/network/server.cc
OBJECT_LOGIN  = $(patsubst src/%.cc, obj/%.o, $(SOURCE_LOGIN))
DEPEND_LOGIN  = $(OBJECT_LOGIN:.o=.d)
INCLUDE_LOGIN = -Isrc/utils -Ilib/rapidxml-1.13
TARGET_LOGIN  = bin/loginserver

# Worldserver
SOURCE_WORLD  = src/worldserver/worldserver.cc \
                src/worldserver/gameengine.cc \
                src/worldserver/player.cc \
                src/worldserver/playerctrl.cc \
                src/utils/logger.cc \
                src/common/accountmanager/accountmgr.cc \
                src/common/world/creature.cc \
                src/common/world/item.cc \
                src/common/world/position.cc \
                src/common/world/tile.cc \
                src/common/world/world.cc \
                src/common/network/connection.cc \
                src/common/network/incomingpacket.cc \
                src/common/network/outgoingpacket.cc \
                src/common/network/acceptor.cc \
                src/common/network/server.cc
OBJECT_WORLD  = $(patsubst src/%.cc, obj/%.o, $(SOURCE_WORLD))
DEPEND_WORLD  = $(OBJECT_WORLD:.o=.d)
INCLUDE_WORLD = -Isrc/utils -Ilib/rapidxml-1.13
TARGET_WORLD  = bin/worldserver

.PHONY: all login login-debug world world-debug test clean

all: login
all: world

debug: login-debug
debug: world-debug

login: CXXFLAGS += $(INCLUDE_LOGIN)
login: $(TARGET_LOGIN)
login-debug: CXXFLAGS += -DDEBUG -g -O0
login-debug: login

world: CXXFLAGS += $(INCLUDE_WORLD)
world: $(TARGET_WORLD)
world-debug: CXXFLAGS += -DDEBUG -g -O0
world-debug: world

dir_guard = @mkdir -p $(@D)

# Loginserver
$(TARGET_LOGIN): $(OBJECT_LOGIN)
	$(dir_guard)
	$(CXX) $(LDFLAGS) -o $@ $^

# Worldserver
$(TARGET_WORLD): $(OBJECT_WORLD)
	$(dir_guard)
	$(CXX) $(LDFLAGS) -o $@ $^

# Common files
obj/common/%.o: src/common/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Utility files
obj/utils/%.o: src/utils/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Compile specific files
obj/%.o: src/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) -Isrc/common -o $@ $<

test: lib/googletest/make/gtest_main.a configparser_test position_test

lib/googletest/make/gtest_main.a:
	make -C lib/googletest/make

configparser_test: lib/googletest/make/gtest_main.a test/utils/configparser_test.cc src/utils/logger.cc
	g++ -std=c++11 -isystem lib/googletest/include -Isrc/utils -g -Wall -Werror -pthread -lpthread $^ -o $@

position_test: lib/googletest/make/gtest_main.a test/common/world/position_test.cc src/common/world/position.cc
	g++ -std=c++11 -isystem lib/googletest/include -Isrc/common/world -g -Wall -Werror -pthread -lpthread $^ -o $@

clean:
	rm -rf obj/ $(TARGET_LOGIN) $(TARGET_WORLD)

-include $(DEPEND_LOGIN)
-include $(DEPEND_WORLD)

