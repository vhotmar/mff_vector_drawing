project(mff_core)

set(CMAKE_CXX_STANDARD 20)

find_package(leaf CONFIG REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
add_library(mff::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} INTERFACE
    zajo::leaf
)