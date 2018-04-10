cmake_minimum_required(VERSION 3.0)

project(gameserver)

# -- Options --

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused-parameter -std=c++14 -pedantic -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fsanitize=leak -fsanitize=undefined")

# -- Binaries --

add_subdirectory("loginserver")
target_include_directories(loginserver PUBLIC
  "account/export"
  "network/export"
  "utils/export"
)

add_subdirectory("worldserver")
target_include_directories(worldserver PUBLIC
  "account/export"
  "network/export"
  "utils/export"
  "world/export"
)

# -- Modules --

add_subdirectory("account")
target_include_directories(account PUBLIC
  "account/export"
  "utils/export"
)
target_include_directories(account SYSTEM PRIVATE
  "../external/rapidxml"
)

add_subdirectory("utils")
target_include_directories(utils PUBLIC
  "utils/export"
)

add_subdirectory("network")
target_include_directories(network PUBLIC
  "network/src"
  "network/export"
  "utils/export"
)

add_subdirectory("gameengine")
target_include_directories(gameengine PUBLIC
  "gameengine/src"
  "gameengine/export"
  "utils/export"
  "world/export"
)

add_subdirectory("world")
target_include_directories(world PUBLIC
  "world/src"
  "world/export"
  "utils/export"
)
target_include_directories(world SYSTEM PRIVATE
  "../external/rapidxml"
)

# -- Unit tests --

add_custom_target(tests DEPENDS
  account_test
  network_test
  utils_test
  world_test
)

add_subdirectory(../external/googletest external/googletest)
set_target_properties(gtest PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gmock PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gtest_main PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gmock_main PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_subdirectory("account/test")
target_compile_definitions(account_test PRIVATE UNITTEST)
set_target_properties(account_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_include_directories(account_test PUBLIC
  "account/export"
  "account/src"
)
target_include_directories(account_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

add_subdirectory("network/test")
target_compile_definitions(network_test PRIVATE UNITTEST)
set_target_properties(network_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_include_directories(network_test PUBLIC
  "network/export"
  "network/src"
)
target_include_directories(network_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

add_subdirectory("utils/test")
target_compile_definitions(utils_test PRIVATE UNITTEST)
set_target_properties(utils_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_include_directories(utils_test PUBLIC
  "utils/export"
  "utils/src"
)
target_include_directories(utils_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

add_subdirectory("world/test")
target_compile_definitions(world_test PRIVATE UNITTEST)
set_target_properties(world_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_include_directories(world_test PUBLIC
  "world/export"
  "world/src"
)
target_include_directories(world_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)
