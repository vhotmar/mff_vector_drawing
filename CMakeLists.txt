cmake_minimum_required(VERSION 3.15)

project(mff_vector_drawing_master)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.11.1
        )

FetchContent_Declare(expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected.git
        GIT_TAG master
        )

FetchContent_MakeAvailable(Catch2 expected)

add_subdirectory(projects/parser_combinator)
add_subdirectory(projects/runner)
