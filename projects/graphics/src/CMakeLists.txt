target_sources(${PROJECT_NAME} PRIVATE
    logger.cpp
    math.cpp
    memory.cpp
    utils.cpp
    )

add_subdirectory(vulkan)
add_subdirectory(window)