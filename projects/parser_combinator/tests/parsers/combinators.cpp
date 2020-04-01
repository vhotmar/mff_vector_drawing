#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>

using namespace std::string_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

SCENARIO("there exists combinators") {
    GIVEN("map combinator ") {
        auto parser = parsers::combinator::map<std::string>(
            parsers::complete::digit1<std::string>,
            [](std::string res) -> unsigned long {
                return res.length();
            }
        );

        WHEN("we try to parse \"123456\"") {
            mff::parser_combinator::ParserResult<std::string, unsigned long> result = parser("123456");

            THEN("it should return 6 as output") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, (unsigned long) 6));
            }
        }

        WHEN("we try to parse \"abc\"") {
            auto result = parser("abc");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, unsigned long>(
                    "abc"s,
                    error::ErrorKind::Digit
                ));
            }
        }
    }

    GIVEN("opt combinator") {
        auto parser = parsers::combinator::opt<std::string>(
            parsers::complete::alpha1<std::string>
        );

        WHEN("we try to parse \"abcd;\"") {
            auto result = parser("abcd;");

            THEN("it should return optional<\"abcd\"> as output") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(";"s, std::make_optional("abcd"s)));
            }
        }

        WHEN("we try to parse \"123;\"") {
            auto result = parser("123;");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result<std::string, std::optional<std::string>>(
                    "123;"s,
                    std::nullopt
                ));
            }
        }
    }

    GIVEN("value mapper of 1234 of alpha1 parser") {
        auto parser = parsers::combinator::value<std::string>(
            1234,
            parsers::complete::alpha1<std::string>
        );

        WHEN("we try to parse \"abcd\"") {
            auto result = parser("abcd");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, 1234));
            }
        }

        WHEN("we try to parse \"123abcd;\"") {
            auto result = parser("123abcd;");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, int>(
                    "123abcd;"s,
                    error::ErrorKind::Alpha
                ));
            }
        }
    }
}