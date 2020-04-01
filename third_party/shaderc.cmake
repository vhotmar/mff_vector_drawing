set(packageName shaderc)
string(TOLOWER ${packageName} packageNameLower)

message(STATUS "Preparing ${packageName}")

FetchContent_Declare(${packageName}
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG eb7bd643ef43ffb9ba091e39a389d0a2a923b1aa
)

FetchContent_GetProperties(${packageName})

if (NOT ${${packageNameLower}_POPULATED})
    message(STATUS "Populating ${packageName}")

    FetchContent_Populate(${packageName})

    if(EXISTS ${${packageNameLower}_SOURCE_DIR}/CMakeLists.txt)
        add_subdirectory(${${packageNameLower}_SOURCE_DIR} ${${packageNameLower}_BINARY_DIR})
    endif()
endif()
