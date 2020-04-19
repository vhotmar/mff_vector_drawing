cmake_minimum_required(VERSION 3.15)

project(mff_vector_drawing
    VERSION 0.1
    DESCRIPTION "Project for MFF class NPRG...")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)


##
# Packages we need in CMake
##
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Git REQUIRED)


##
# Functions to help us manage dependencies
##
function(add_package packageName)
    string(TOLOWER ${packageName} packageNameLower)

    message(STATUS "Preparing ${packageName}")

    FetchContent_Declare(${packageName} ${ARGN} GIT_PROGRESS ON)

    FetchContent_GetProperties(${packageName})

    if (NOT ${${packageNameLower}_POPULATED})
        message(STATUS "Populating ${packageName}")

        FetchContent_Populate(${packageName})

        if (EXISTS ${${packageNameLower}_SOURCE_DIR}/CMakeLists.txt)
            add_subdirectory(${${packageNameLower}_SOURCE_DIR} ${${packageNameLower}_BINARY_DIR})
        endif ()
    endif ()
endfunction()

##
# Required libraries
##
add_subdirectory(third_party)


##
# Add projects
##
add_subdirectory(projects/core)
add_subdirectory(projects/parser_combinator)
add_subdirectory(projects/runner)
