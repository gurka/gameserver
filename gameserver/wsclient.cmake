project(wsclient)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g --bind -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1 -s USE_SDL=2")

add_subdirectory(wsclient)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/data.dat" "${CMAKE_BINARY_DIR}/files/data.dat" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/sprite.dat" "${CMAKE_BINARY_DIR}/files/sprite.dat" COPYONLY)

# Hack to build subset of the modules that gameserver use

add_subdirectory(common)

add_library(gameengine INTERFACE)
target_include_directories(gameengine INTERFACE "gameengine/export")

add_library(io
  "io/export/data_loader.h"
  "io/export/sprite_loader.h"
  "io/src/data_loader.cc"
  "io/src/file_reader.h"
  "io/src/sprite_loader.cc"
)
target_include_directories(io PUBLIC "io/export")
target_link_libraries(io PRIVATE
  common
  utils
  rapidxml
)

add_library(network
  "network/export/incoming_packet.h"
  "network/export/outgoing_packet.h"
  "network/src/incoming_packet.cc"
  "network/src/outgoing_packet.cc"
)
target_include_directories(network PUBLIC "network/export")
target_link_libraries(network PRIVATE
  utils
)

add_library(protocol
  "protocol/export/protocol_client.h"
  "protocol/export/protocol_common.h"
  "protocol/src/protocol_client.cc"
  "protocol/src/protocol_common.cc"
)
target_include_directories(protocol PUBLIC "protocol/export")
target_link_libraries(protocol PRIVATE
  common
  gameengine
  network
  world
  utils
)

add_library(utils
  "utils/export/logger.h"
  "utils/src/logger.cc"
)
target_include_directories(utils PUBLIC "utils/export")

add_library(world INTERFACE)
target_include_directories(world INTERFACE "world/export")

add_library(rapidxml INTERFACE)
target_include_directories(rapidxml SYSTEM INTERFACE "../external/rapidxml")

# -- Docker targets --
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.wsclient" "${CMAKE_BINARY_DIR}/Dockerfile.wsclient" COPYONLY)
add_custom_target(image
  docker build -t gameserver/wsclient -f "${CMAKE_BINARY_DIR}/Dockerfile.wsclient" "${CMAKE_BINARY_DIR}"
)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/run_wsclient.sh" "${CMAKE_BINARY_DIR}/run_wsclient.sh" COPYONLY)
add_dependencies(image wsclient)
