# vulkan-memory-allocator eigen3 tl-expected catch2 fmt glfw3 glm leaf spdlog boost-program-options range-v3


find_package(Boost 1.72.0 REQUIRED)

find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
# target_link_libraries(main PRIVATE unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

find_package(Eigen3 CONFIG REQUIRED)
# target_link_libraries(main PRIVATE Eigen3::Eigen)

find_package(tl-expected CONFIG REQUIRED)
# target_link_libraries(main PRIVATE tl::expected)

find_package(Catch2 REQUIRED)
# target_link_libraries(main PRIVATE Catch2::Catch2)

find_package(fmt CONFIG REQUIRED)
# target_link_libraries(main PRIVATE fmt::fmt fmt::fmt-header-only)

find_package(glfw3 CONFIG REQUIRED)
# target_link_libraries(main PRIVATE glfw)

find_package(glm CONFIG REQUIRED)
# target_link_libraries(main PRIVATE glm)

find_package(leaf CONFIG REQUIRED)
# target_link_libraries(main PRIVATE zajo::leaf)

find_package(range-v3 CONFIG REQUIRED)
# Note: 2 target(s) were omitted.
# target_link_libraries(main PRIVATE range-v3 range-v3-meta range-v3::meta range-v3-concepts)


IF (APPLE)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF ()

find_package(spdlog CONFIG REQUIRED)
# target_link_libraries(main PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

###
# Vulkan this dependency needs to be supported by user
###
set(Vulkan_VERSION 1.2.131.2)

find_package(Vulkan ${Vulkan_VERSION} REQUIRED)
set_target_properties(Vulkan::Vulkan PROPERTIES IMPORTED_GLOBAL TRUE)

get_filename_component(Vulkan_LIBRARY_DIR ${Vulkan_LIBRARY} DIRECTORY CACHE)
get_filename_component(Vulkan_SDK_DIR ${Vulkan_LIBRARY_DIR}/.. ABSOLUTE CACHE)
