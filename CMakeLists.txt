cmake_minimum_required(VERSION 3.1)
project(satin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/Users/alanstewart/projects/c/satin-c")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c90 -ansi -pedantic")

set(SOURCE_FILES src/satin.h src/satin.c)
add_executable(satin ${SOURCE_FILES})