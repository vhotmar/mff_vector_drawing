#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>

using namespace std::string_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

SCENARIO("there exists an alt parser") {
    GIVEN("alt parser should parse alphanumeric characters or digits") {
        auto parser = parsers::alt_fn<std::string>{}(
            parsers::complete::alpha1_fn<std::string>{},
            parsers::complete::digit1_fn<std::string>{}
        );

        WHEN("we try to parse \"abc\"") {
            auto result = parser("abc");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "abc"s));
            }
        }

        WHEN("we try to parse \"123456\"") {
            auto result = parser("123456");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "123456"s));
            }
        }

        WHEN("we try to parse \" \"") {
            auto result = parser(" ");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    " "s,
                    error::ErrorKind::Digit
                ));
            }
        }
    }
}