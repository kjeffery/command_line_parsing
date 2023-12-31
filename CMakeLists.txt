cmake_minimum_required(VERSION 3.28)
project(command_line_parsing)

set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)

add_library(CommandLineParser)

target_sources(CommandLineParser
        PUBLIC
        FILE_SET CXX_MODULES FILES
        CommandLineParser.ixx
)

add_executable(command_line_parsing main.cpp
        CommandLineParser.hpp
)

target_link_libraries(command_line_parsing CommandLineParser)
