cmake_minimum_required(VERSION 3.12)

project(gameserver)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -- Options --
if (NOT EMSCRIPTEN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wmost -Werror -std=c++17 -pedantic -pthread")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g --bind -s ALLOW_MEMORY_GROWTH=1 -s EXCEPTION_DEBUG=1 -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1 -s USE_SDL=2")
endif()

option(GAMESERVER_DEBUG_FULL "Compile with ASAN and clang-tidy" OFF)
if(GAMESERVER_DEBUG_FULL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*;-warnings-as-errors=*")
endif()

# Check for boost date_time
if (NOT EMSCRIPTEN)
  find_package(Boost REQUIRED COMPONENTS date_time)
endif()

# -- Binaries --
if (NOT EMSCRIPTEN)
  add_subdirectory("loginserver")
  add_subdirectory("worldserver")
  add_subdirectory("replayserver")
endif()
add_subdirectory("wsclient")

# -- Data files --
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../data")
  if (NOT EMSCRIPTEN)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/loginserver.cfg"  "${CMAKE_BINARY_DIR}/data/loginserver.cfg"  COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/worldserver.cfg"  "${CMAKE_BINARY_DIR}/data/worldserver.cfg"  COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/replayserver.cfg" "${CMAKE_BINARY_DIR}/data/replayserver.cfg" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/accounts.xml"     "${CMAKE_BINARY_DIR}/data/accounts.xml"     COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/data.dat"         "${CMAKE_BINARY_DIR}/data/data.dat"         COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/items.xml"        "${CMAKE_BINARY_DIR}/data/items.xml"        COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/world.xml"        "${CMAKE_BINARY_DIR}/data/world.xml"        COPYONLY)
  endif()
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/data.dat"         "${CMAKE_BINARY_DIR}/files/data.dat"   COPYONLY)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/sprite.dat"       "${CMAKE_BINARY_DIR}/files/sprite.dat" COPYONLY)
endif()

# -- Modules --
add_subdirectory("common"     EXCLUDE_FROM_ALL)
add_subdirectory("protocol"   EXCLUDE_FROM_ALL)
add_subdirectory("utils"      EXCLUDE_FROM_ALL)
add_subdirectory("world"      EXCLUDE_FROM_ALL)
add_subdirectory("gameengine" EXCLUDE_FROM_ALL)
add_subdirectory("network"    EXCLUDE_FROM_ALL)
add_subdirectory("account"    EXCLUDE_FROM_ALL)

set(CMAKE_CXX_CLANG_TIDY "")

# -- External --
add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE "../external/asio/asio/include")
add_library(websocketpp INTERFACE)
target_include_directories(websocketpp SYSTEM INTERFACE "../external/websocketpp")
add_library(rapidxml INTERFACE)
target_include_directories(rapidxml SYSTEM INTERFACE "../external/rapidxml")

# -- Unit tests --
add_subdirectory("../external/googletest" "external/googletest" EXCLUDE_FROM_ALL)
add_subdirectory("account/test" EXCLUDE_FROM_ALL)
add_subdirectory("common/test" EXCLUDE_FROM_ALL)
add_subdirectory("gameengine/test" EXCLUDE_FROM_ALL)
add_subdirectory("network/test" EXCLUDE_FROM_ALL)
add_subdirectory("utils/test" EXCLUDE_FROM_ALL)
add_subdirectory("world/test" EXCLUDE_FROM_ALL)
add_subdirectory("wsclient/test" EXCLUDE_FROM_ALL)
add_custom_target(unittest DEPENDS
  account_test
  common_test
  gameengine_test
  network_test
  utils_test
  world_test
  wsclient_test
)

# -- Docker targets --
if (NOT EMSCRIPTEN)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../Dockerfile.login" "${CMAKE_BINARY_DIR}/Dockerfile.login" COPYONLY)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../Dockerfile.world" "${CMAKE_BINARY_DIR}/Dockerfile.world" COPYONLY)
  add_custom_target(docker
    COMMAND docker build -t gameserver/login -f "${CMAKE_BINARY_DIR}/Dockerfile.login" "${CMAKE_BINARY_DIR}"
    COMMAND docker build -t gameserver/world -f "${CMAKE_BINARY_DIR}/Dockerfile.world" "${CMAKE_BINARY_DIR}"
  )
  add_dependencies(docker loginserver worldserver)
else()
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../Dockerfile.wsclient" "${CMAKE_BINARY_DIR}/Dockerfile.wsclient" COPYONLY)
  add_custom_target(docker
    docker build -t gameserver/wsclient -f "${CMAKE_BINARY_DIR}/Dockerfile.wsclient" "${CMAKE_BINARY_DIR}"
  )
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../run_wsclient.sh" "${CMAKE_BINARY_DIR}/run_wsclient.sh" COPYONLY)
  add_dependencies(docker wsclient)
endif()
