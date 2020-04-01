set(packageName glfw)
string(TOLOWER ${packageName} packageNameLower)

message(STATUS "Preparing ${packageName}")

FetchContent_Declare(${packageName}
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 44b5d06583cd21ac237eb8f6263db03faf1726c2
)

FetchContent_GetProperties(${packageName})

if (NOT ${${packageNameLower}_POPULATED})
    message(STATUS "Populating ${packageName}")

    FetchContent_Populate(${packageName})

    if(EXISTS ${${packageNameLower}_SOURCE_DIR}/CMakeLists.txt)
        add_subdirectory(${${packageNameLower}_SOURCE_DIR} ${${packageNameLower}_BINARY_DIR})
    endif()
endif()
