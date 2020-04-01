set(packageName glm)
string(TOLOWER ${packageName} packageNameLower)

message(STATUS "Preparing ${packageName}")

FetchContent_Declare(${packageName}
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG efbfecab63da2adb2dc197f91270ff6221900740
)

FetchContent_GetProperties(${packageName})

if (NOT ${${packageNameLower}_POPULATED})
    message(STATUS "Populating ${packageName}")

    FetchContent_Populate(${packageName})

    if(EXISTS ${${packageNameLower}_SOURCE_DIR}/CMakeLists.txt)
        add_subdirectory(${${packageNameLower}_SOURCE_DIR} ${${packageNameLower}_BINARY_DIR})
    endif()
endif()
