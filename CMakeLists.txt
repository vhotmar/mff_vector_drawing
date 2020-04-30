cmake_minimum_required(VERSION 3.15)

project(mff_vector_drawing
    VERSION 0.1
    DESCRIPTION "Project for MFF class NPRG...")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

##
# Packages we need in CMake
##
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Git REQUIRED)

##
# Required libraries
##
add_subdirectory(third_party)


##
# Add projects
##
add_subdirectory(projects/core)
add_subdirectory(projects/graphics)
add_subdirectory(projects/parser_combinator)
add_subdirectory(projects/runner)
