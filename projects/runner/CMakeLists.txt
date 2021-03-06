project(mff_runner)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
find_package(leaf CONFIG REQUIRED)
find_package(Boost REQUIRED program_options)

# add_executable(${PROJECT_NAME} main.cpp renderer.cpp)
add_executable(${PROJECT_NAME} main.cpp)

# add_subdirectory(internal)
add_subdirectory(renderer)
add_subdirectory(canvas)
add_subdirectory(utils)

target_link_libraries(${PROJECT_NAME}
    mff::parser_combinator
    mff::core
    mff::graphics
    Vulkan::Vulkan
    glfw
    spdlog::spdlog
    Eigen3::Eigen
    fmt::fmt
    unofficial::vulkan-memory-allocator::vulkan-memory-allocator
    zajo::leaf
    ${Boost_LIBRARIES}
    )

target_include_directories(${PROJECT_NAME} INTERFACE
    ${Boost_INCLUDE_DIRS}
    )


add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:${PROJECT_NAME}>)
