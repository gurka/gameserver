project(wsclient)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g --bind -s ALLOW_MEMORY_GROWTH=1 -s EXCEPTION_DEBUG=1 -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1 -s USE_SDL=2")

# -- Binary --
add_subdirectory(wsclient)

# -- Data files --
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/data.dat" "${CMAKE_BINARY_DIR}/files/data.dat" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../data/sprite.dat" "${CMAKE_BINARY_DIR}/files/sprite.dat" COPYONLY)

# -- Modules --
add_subdirectory(common EXCLUDE_FROM_ALL)
add_subdirectory(network EXCLUDE_FROM_ALL)
add_subdirectory(protocol EXCLUDE_FROM_ALL)
add_subdirectory(utils EXCLUDE_FROM_ALL)

# -- External --
add_library(rapidxml INTERFACE)
target_include_directories(rapidxml SYSTEM INTERFACE "../external/rapidxml")

# -- Docker targets --
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.wsclient" "${CMAKE_BINARY_DIR}/Dockerfile.wsclient" COPYONLY)
add_custom_target(image
  docker build -t gameserver/wsclient -f "${CMAKE_BINARY_DIR}/Dockerfile.wsclient" "${CMAKE_BINARY_DIR}"
)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/run_wsclient.sh" "${CMAKE_BINARY_DIR}/run_wsclient.sh" COPYONLY)
add_dependencies(image wsclient)
