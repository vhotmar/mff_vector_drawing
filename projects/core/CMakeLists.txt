project(mff_core)

set(CMAKE_CXX_STANDARD 20)

find_package(leaf CONFIG REQUIRED)

add_library(${PROJECT_NAME})
add_library(mff::core ALIAS ${PROJECT_NAME})

add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    zajo::leaf
)