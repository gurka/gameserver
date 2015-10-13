cmake_minimum_required(VERSION 3.0)

option(gameserver_test "Unit tests" OFF)
project(gameserver)


## Settings
# Common compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# Required libraries (Boost)
set(LIBRARIES boost_system pthread)


## Executables
# Loginserver
set(loginserver_src
  "src/loginserver/loginserver.cc"
)
set(loginserver_inc
  "src/utils"
  "src/common"
)
set(loginserver_lib
  "utils"
  "common_accmgr"
  "common_network"
)
add_executable(loginserver ${loginserver_src})
target_include_directories(loginserver PUBLIC ${loginserver_inc})
target_link_libraries(loginserver ${loginserver_lib} ${LIBRARIES})

# Worldserver
set(worldserver_src
  "src/worldserver/gameengine.cc"
  "src/worldserver/gameengine.h"
  "src/worldserver/player.cc"
  "src/worldserver/playerctrl.cc"
  "src/worldserver/playerctrl.h"
  "src/worldserver/player.h"
  "src/worldserver/taskqueue.h"
  "src/worldserver/worldserver.cc"
)
set(worldserver_inc
  "src/utils"
  "src/common"
)
set(worldserver_lib
  "utils"
  "common_accmgr"
  "common_network"
  "common_world"
)
add_executable(worldserver ${worldserver_src})
target_include_directories(worldserver PUBLIC ${worldserver_inc})
target_link_libraries(worldserver ${worldserver_lib} ${LIBRARIES})


## Modules
# Utils
set(utils_src
  "src/utils/configparser.h"
  "src/utils/logger.cc"
  "src/utils/logger.h"
)
add_library(utils ${utils_src})

# Accountmanager
set(common_accmgr_src
  "src/common/accountmanager/accountmgr.cc"
  "src/common/accountmanager/accountmgr.h"
)
set(common_accmgr_inc
  "src/utils"
  "lib/rapidxml"
)
add_library(common_accmgr ${common_accmgr_src})
target_include_directories(common_accmgr PUBLIC ${common_accmgr_inc})

# Network
set(common_network_src
  "src/common/network/acceptor.cc"
  "src/common/network/acceptor.h"
  "src/common/network/connection.cc"
  "src/common/network/connection.h"
  "src/common/network/incomingpacket.cc"
  "src/common/network/incomingpacket.h"
  "src/common/network/outgoingpacket.cc"
  "src/common/network/outgoingpacket.h"
  "src/common/network/server.cc"
  "src/common/network/server.h"
)
set(common_network_inc
  "src/utils"
)
add_library(common_network ${common_network_src})
target_include_directories(common_network PUBLIC ${common_network_inc})

# World
set(common_world_src
  "src/common/world/creature.cc"
  "src/common/world/creaturectrl.h"
  "src/common/world/creature.h"
  "src/common/world/direction.h"
  "src/common/world/item.cc"
  "src/common/world/item.h"
  "src/common/world/npcctrl.h"
  "src/common/world/position.cc"
  "src/common/world/position.h"
  "src/common/world/tile.cc"
  "src/common/world/tile.h"
  "src/common/world/world.cc"
  "src/common/world/world.h"
  "src/common/world/worldinterface.h"
)
set(common_world_inc
  "src/utils"
  "lib/rapidxml"
)
add_library(common_world ${common_world_src})
target_include_directories(common_world PUBLIC ${common_world_inc})

## Unit tests
if (gameserver_test)
  set(unittest_src
    "test/utils/configparser_test.cc"
    "test/common/world/position_test.cc"
  )

  set(unittest_inc
    "src/utils"
    "src/common/accountmanager"
    "src/common/network"
    "src/common/world"
    "lib/rapidxml"
  )

  set(unittest_lib
    "utils"
    "common_accmgr"
    "common_network"
    "common_world"
  )

  add_subdirectory(lib/googletest)

  enable_testing()
  
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

  add_executable(unittest ${unittest_src})
  target_include_directories(unittest PUBLIC ${unittest_inc})

  target_link_libraries(unittest gtest_main)
  target_link_libraries(unittest ${unittest_lib})
endif()
