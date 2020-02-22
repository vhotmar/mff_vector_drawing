#include <iostream>
#include <sstream>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/buffered_stream.h>

SCENARIO("stream_buffer can be read from")
{
    GIVEN("A buffered stream with some content")
    {
        std::stringstream ss;

        ss << "Hello";

        mff::parser_combinator::buffered_stream stream(ss);

        REQUIRE(stream.position() == 0);
        REQUIRE(!stream.ended());

        WHEN("we peek at first character") {
            auto peek_result = stream.peek();

            THEN("the result is not erroneous and a character is returned") {
                REQUIRE(peek_result.has_value());

                char c = peek_result.value();

                REQUIRE(c == 'H');
            }

            THEN("position does not change") {
                REQUIRE(stream.position() == 0);
            }
        }

        WHEN("we get a character") {
            auto get_result = stream.get();

            THEN("the result is not erroneous and a character is returned") {
                REQUIRE(get_result.has_value());

                char c = get_result.value();

                REQUIRE(c == 'H');
            }

            THEN("a position is changed") {
                REQUIRE(stream.position() == 1);
            }

            AND_WHEN("we peek at next character") {
                auto peek_result = stream.peek();

                THEN("the result is not erroneous and a character is returned") {
                    REQUIRE(peek_result.has_value());

                    char c = peek_result.value();

                    REQUIRE(c == 'e');
                }

                THEN("a position is not changed") {
                    REQUIRE(stream.position() == 1);
                }
            }
        }
    }
}