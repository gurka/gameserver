cmake_minimum_required(VERSION 3.12)

project(common)

add_library(common INTERFACE)

target_include_directories(common INTERFACE "export")
