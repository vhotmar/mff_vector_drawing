#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>

using namespace std::string_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

SCENARIO("there exists a alpha parser") {
    GIVEN("alpha0 parser") {
        WHEN("we try to parse \"ab1c\"") {
            auto result = parsers::complete::alpha0("ab1c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("1c"s, "ab"s));
            }
        }

        WHEN("we try to parse \"1c\"") {
            auto result = parsers::complete::alpha0("1c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("1c"s, ""s));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parsers::complete::alpha0(""s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, ""s));
            }
        }
    }

    GIVEN("alpha1 parser") {
        WHEN("we try to parse \"aB1c\"") {
            auto result = parsers::complete::alpha1("aB1c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("1c"s, "aB"s));
            }
        }

        WHEN("we try to parse \"1c\"") {
            auto result = parsers::complete::alpha1("1c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "1c"s,
                    error::ErrorKind::Alpha
                ));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parsers::complete::alpha1(""s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    ""s,
                    error::ErrorKind::Alpha
                ));
            }
        }
    }

    GIVEN("digit0 parser") {
        WHEN("we try to parse \"21c\"") {
            auto result = parsers::complete::digit0("21c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("c"s, "21"s));
            }
        }

        WHEN("we try to parse \"1c\"") {
            auto result = parsers::complete::digit0("21"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "21"s));
            }
        }

        WHEN("we try to parse \"a21c\"") {
            auto result = parsers::complete::digit0("a21c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("a21c"s, ""s));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parsers::complete::digit0(""s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, ""s));
            }
        }
    }

    GIVEN("digit1 parser") {
        WHEN("we try to parse \"21c\"") {
            auto result = parsers::complete::digit1("21c"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("c"s, "21"s));
            }
        }

        WHEN("we try to parse \"1c\"") {
            auto result = parsers::complete::digit1("c1"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "c1"s,
                    error::ErrorKind::Digit
                ));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parsers::complete::digit1(""s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    ""s,
                    error::ErrorKind::Digit
                ));
            }
        }
    }
}