## Compiler, compiler flags, linker flags and common includes
CXX            = g++
CXXFLAGS       = -c -Wall -Werror -std=c++11 -MMD
CXXFLAGS_DEBUG = -DDEBUG -g -O0
LDFLAGS        = -lboost_system -lpthread
INCLUDES       = -Isrc/utils -Ilib/rapidxml-1.13

## Source code
# Common
SOURCE_ACCMGR  = $(wildcard src/common/accountmanager/*.cc)
SOURCE_NETWORK = $(wildcard src/common/network/*.cc)
SOURCE_WORLD   = $(wildcard src/common/world/*.cc)

# Utils
SOURCE_UTILS   = $(wildcard src/utils/*.cc)

# Loginserver
SOURCE_LOGINSERVER  = $(wildcard src/loginserver/*.cc) \
                      $(SOURCE_UTILS) \
                      $(SOURCE_ACCMGR) \
                      $(SOURCE_NETWORK)

# Worldserver
SOURCE_WORLDSERVER  = $(wildcard src/worldserver/*.cc) \
                      $(SOURCE_UTILS) \
                      $(SOURCE_ACCMGR) \
                      $(SOURCE_NETWORK) \
                      $(SOURCE_WORLD)

# Unit test
SOURCE_ACCMGR_TEST      = $(wildcard test/common/accountmanager/*.cc) $(SOURCE_ACCMGR)
SOURCE_NETWORK_TEST     = $(wildcard test/common/network/*.cc)        $(SOURCE_NETWORK)
SOURCE_WORLD_TEST       = $(wildcard test/common/world/*.cc)          $(SOURCE_WORLD)
SOURCE_UTILS_TEST       = $(wildcard test/utils/*.cc)                 $(SOURCE_UTILS)
SOURCE_LOGINSERVER_TEST = $(wildcard test/loginserver/*.cc)           $(SOURCE_LOGINSERVER)
SOURCE_WORLDSERVER_TEST = $(wildcard test/worldserver/*.cc)           $(SOURCE_WORLDSERVER)

SOURCE_TEST = $(SOURCE_ACCMGR_TEST) \
              $(SOURCE_NETWORK_TEST) \
              $(SOURCE_WORLD_TEST) \
              $(SOURCE_UTILS_TEST) \
              $(SOURCE_LOGINSERVER_TEST) \
              $(SOURCE_WORLDSERVER_TEST)

SOURCE_TEST := $(filter-out src/loginserver/loginserver.cc src/worldserver/worldserver.cc, $(SOURCE_TEST))  # Contains main()


## Binary targets
TARGET_LOGINSERVER  = bin/loginserver
TARGET_WORLDSERVER  = bin/worldserver
TARGET_UNITTEST     = bin/unittest

## Makefile targets
.PHONY: all login world debug login-debug world-debug test clean

all: login world
login: $(TARGET_LOGINSERVER)
world: $(TARGET_WORLDSERVER)

debug: login-debug world-debug
login-debug: CXXFLAGS += $(CXXFLAGS_DEBUG)
login-debug: login
world-debug: CXXFLAGS += $(CXXFLAGS_DEBUG)
world-debug: world

test: $(TARGET_UNITTEST)
	@$(TARGET_UNITTEST)

clean:
	rm -rf obj/ $(TARGET_LOGINSERVER) $(TARGET_WORLDSERVER)

dir_guard = @mkdir -p $(@D)

## Rules
# Loginserver binary
$(TARGET_LOGINSERVER): $(addprefix obj/, $(SOURCE_LOGINSERVER:.cc=.o))
	$(dir_guard)
	$(CXX) $(LDFLAGS) -o $@ $^

# Worldserver binary
$(TARGET_WORLDSERVER): $(addprefix obj/, $(SOURCE_WORLDSERVER:.cc=.o))
	$(dir_guard)
	$(CXX) $(LDFLAGS) -o $@ $^

# Unit test binary
$(TARGET_UNITTEST): $(addprefix obj/, $(SOURCE_TEST:.cc=.o))
	@make -C lib/googletest/make
	$(dir_guard)
	$(CXX) $(LDFLAGS) -o $@ lib/googletest/make/gtest_main.a $^

# Common files
obj/src/common/%.o: src/common/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Utility files
obj/src/utils/%.o: src/utils/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Other (non-common/non-utility) files
obj/src/%.o: src/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Isrc/common -o $@ $^

# Unit test files
obj/test/%.o: test/%.cc
	$(dir_guard)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Isrc/common -I$(patsubst test/%, src/%, $(dir $^)) -Ilib/googletest/include -o $@ $^

## Dependencies
-include $(addprefix obj/, $(SOURCE_LOGINSERVER:.cc=.d))
-include $(addprefix obj/, $(SOURCE_WORLDSERVER:.cc=.d))
-include $(addprefix obj/, $(SOURCE_UNITTEST:.cc=.d))
