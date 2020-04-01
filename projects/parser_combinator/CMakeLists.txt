project(mff_parser_combinator)

set(CMAKE_CXX_STANDARD 20)

enable_testing()

add_library(${PROJECT_NAME} INTERFACE)
add_library(mff::parser_combinator ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} INTERFACE
    tl::expected
)

target_include_directories(${PROJECT_NAME} INTERFACE
    ${PROJECT_SOURCE_DIR}/include
)

### Tests

file(GLOB_RECURSE test-sources CONFIGURE_DEPENDS tests/*.cpp)
list(FILTER test-sources EXCLUDE REGEX "tests/test.cpp")
add_executable(${PROJECT_NAME}-tests "${test-sources}")

# Can add benchmarks in the future
target_compile_definitions(${PROJECT_NAME}-tests
    PRIVATE
     -DCATCH_CONFIG_ENABLE_BENCHMARKING
)

target_link_libraries(${PROJECT_NAME}-tests
    PRIVATE
    Catch2::Catch2
    mff::parser_combinator
    expected
)

add_test(NAME mff::parser_combinator::tests COMMAND ${PROJECT_NAME}-tests)