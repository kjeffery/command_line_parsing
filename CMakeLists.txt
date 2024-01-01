cmake_minimum_required(VERSION 3.28)
project(command_line_parsing)

set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)

add_library(CommandLineParser)
add_library(std_module OBJECT IMPORTED)

set_property(TARGET std_module PROPERTY IMPORTED_OBJECTS "${CMAKE_SOURCE_DIR}/std.obj")

target_sources(CommandLineParser
        PUBLIC
        FILE_SET CXX_MODULES FILES
        CommandLineParser.ixx
        std.ixx
)

add_executable(command_line_parsing main.cpp)

target_link_libraries(command_line_parsing CommandLineParser std_module)
