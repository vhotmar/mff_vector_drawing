target_sources(${PROJECT_NAME} PRIVATE
    logger.cpp
)


add_subdirectory(vulkan)
add_subdirectory(window)