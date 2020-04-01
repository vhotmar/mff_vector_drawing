
set(packageName effcee)
string(TOLOWER ${packageName} packageNameLower)

message(STATUS "Preparing ${packageName}")

FetchContent_Declare(${packageName}
    GIT_REPOSITORY https://github.com/google/effcee
    GIT_TAG 98980e2b785403b5f43c23ed5a81e1a22e7297e8
)

FetchContent_GetProperties(${packageName})

if (NOT ${${packageNameLower}_POPULATED})
    message(STATUS "Populating ${packageName}")

    FetchContent_Populate(${packageName})

    if(EXISTS ${${packageNameLower}_SOURCE_DIR}/CMakeLists.txt)
        add_subdirectory(${${packageNameLower}_SOURCE_DIR} ${${packageNameLower}_BINARY_DIR})
    endif()
endif()
