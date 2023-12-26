cmake_minimum_required(VERSION 3.27)
project(command_line_parsing)

set(CMAKE_CXX_STANDARD 23)

add_executable(command_line_parsing main.cpp
        CommandLineParser.hpp
        Finally.hpp)
