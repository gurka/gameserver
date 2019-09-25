cmake_minimum_required(VERSION 3.12)

project(worldtypes)

add_library(worldtypes INTERFACE)

target_include_directories(worldtypes INTERFACE "export")
