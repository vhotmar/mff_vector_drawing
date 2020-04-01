set(packageName glslang)
string(TOLOWER ${packageName} packageNameLower)

message(STATUS "Preparing ${packageName}")

FetchContent_Declare(${packageName}
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG 9659831766f1be87a6aaf6df8302ec61be0ca05a
)

FetchContent_GetProperties(${packageName})

if (NOT ${${packageNameLower}_POPULATED})
    message(STATUS "Populating ${packageName}")

    FetchContent_Populate(${packageName})

    if(EXISTS ${${packageNameLower}_SOURCE_DIR}/CMakeLists.txt)
        add_subdirectory(${${packageNameLower}_SOURCE_DIR} ${${packageNameLower}_BINARY_DIR})
    endif()
endif()
