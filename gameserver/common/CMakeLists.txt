cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_library(common INTERFACE)

target_include_directories(common INTERFACE "export")
