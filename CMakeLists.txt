cmake_minimum_required(VERSION 3.28)
project(command_line_parsing)

set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)

add_library(std_module OBJECT IMPORTED)

add_executable(command_line_parsing main.cpp CommandLineParser.hpp)
