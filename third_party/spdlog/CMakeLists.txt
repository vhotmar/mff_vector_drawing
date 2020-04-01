
message(STATUS "Preparing spdlog")

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG 2c5c96e1595b01dee19353dac846091e20371429
)

FetchContent_GetProperties(spdlog)

if (NOT ${spdlog_POPULATED})
    message(STATUS "Populating spdlog")

    FetchContent_Populate(spdlog)

    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})

    target_include_directories(spdlog PRIVATE $<TARGET_PROPERTY:fmt,INCLUDE_DIRECTORIES>)


endif()

file(REMOVE ${spdlog_SOURCE_DIR}/include/spdlog/tweakme.h)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tweakme.h
    DESTINATION ${spdlog_SOURCE_DIR}/include/spdlog)
