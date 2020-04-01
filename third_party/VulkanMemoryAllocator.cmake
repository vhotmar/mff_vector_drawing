set(packageName VulkanMemoryAllocator)
string(TOLOWER ${packageName} packageNameLower)

message(STATUS "Preparing ${packageName}")

FetchContent_Declare(${packageName}
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG 3d1ce4ebb843e6d078c93f2f94f677fa89c9f874
)

FetchContent_GetProperties(${packageName})

if (NOT ${${packageNameLower}_POPULATED})
    message(STATUS "Populating ${packageName}")

    FetchContent_Populate(${packageName})

    if(NOT EXISTS ${${packageNameLower}_BINARY_DIR}/vk_mem_alloc.cc)
        file(WRITE ${${packageNameLower}_BINARY_DIR}/vk_mem_alloc.cc
            "#define VMA_IMPLEMENTATION\n#include \"vk_mem_alloc.h\"")
    endif()

    add_library(vma ${${packageNameLower}_BINARY_DIR}/vk_mem_alloc.cc)
    target_include_directories(vma
        PUBLIC
        ${${packageNameLower}_SOURCE_DIR}/src
    )

    target_link_libraries(vma PUBLIC Vulkan::Vulkan)
endif()
