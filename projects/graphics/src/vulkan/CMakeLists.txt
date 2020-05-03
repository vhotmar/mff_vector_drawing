target_sources(${PROJECT_NAME} PRIVATE
    debug.cpp
    device.cpp
    dispatcher.cpp
    format.cpp
    instance.cpp
    render_pass.cpp
    swapchain.cpp
    sync.cpp
    version.cpp
)

add_subdirectory(command_buffer)
add_subdirectory(framebuffer)
add_subdirectory(image)
# add_subdirectory(graphics_pipeline)
# add_subdirectory(pipeline)
# add_subdirectory(descriptor)
