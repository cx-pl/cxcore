cmake_minimum_required(VERSION 3.31)
project(cxcore)

add_library(cxcore SHARED)
add_subdirectory(inc)
add_subdirectory(platform)
add_subdirectory(src)
