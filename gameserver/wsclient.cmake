project(wsclient)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g --bind -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1 -s USE_SDL=2")

add_subdirectory(wsclient)

set_target_properties(wsclient PROPERTIES OUTPUT_NAME "index.html")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/data.dat" "${CMAKE_BINARY_DIR}/files/data.dat" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/sprite.dat" "${CMAKE_BINARY_DIR}/files/sprite.dat" COPYONLY)

target_link_options(wsclient PRIVATE --preload-file "files/")

# Hack to build subset of the modules that gameserver use

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
  utils
  world
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
  "protocol/export/protocol.h"
  "protocol/export/protocol_types.h"
  "protocol/src/protocol.cc"
)
target_include_directories(protocol PUBLIC "protocol/export")
target_link_libraries(protocol PRIVATE
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

add_library(world
  "world/export/creature.h"
  "world/export/direction.h"
  "world/export/item.h"
  "world/export/position.h"
  "world/src/creature.cc"
  "world/src/position.cc"
)
target_include_directories(world PUBLIC "world/export")
target_link_libraries(world PRIVATE
  utils
)

add_library(rapidxml INTERFACE)
target_include_directories(rapidxml SYSTEM INTERFACE "../external/rapidxml")
