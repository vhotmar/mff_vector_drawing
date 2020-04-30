project(mff_graphics)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(leaf CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)

add_library(${PROJECT_NAME})
add_library(mff::graphics ALIAS ${PROJECT_NAME})

add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    )

target_link_libraries(${PROJECT_NAME} PUBLIC
    mff::core
    Vulkan::Vulkan
    glfw
    spdlog::spdlog
    # shaderc
    Eigen3::Eigen
    fmt::fmt
    unofficial::vulkan-memory-allocator::vulkan-memory-allocator
    zajo::leaf
    range-v3
)
