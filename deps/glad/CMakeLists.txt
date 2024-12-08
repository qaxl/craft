cmake_minimum_required(VERSION 3.20)
project(GLAD)

add_library(glad src/gl.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
