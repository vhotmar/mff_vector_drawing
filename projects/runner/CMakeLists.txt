project(mff_runner)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp ../parser_combinator/include/mff/parser_combinator/stream.h)
target_link_libraries(${PROJECT_NAME}
    tl::expected
    mff::parser_combinator
)
