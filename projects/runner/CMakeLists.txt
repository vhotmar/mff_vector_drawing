project(mff_runner)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

add_subdirectory(internal)
add_subdirectory(utils)

target_link_libraries(${PROJECT_NAME}
    tl::expected
    mff::parser_combinator
    mff::core
    Vulkan::Vulkan
    glfw
    spdlog
    shaderc
    glm
    fmt
    vma
    leaf
)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:${PROJECT_NAME}>)
