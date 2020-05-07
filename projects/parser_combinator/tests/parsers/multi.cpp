#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>
#include <mff/parser_combinator/utils.h>


using namespace std::string_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

SCENARIO("there exists a many parser") {
    GIVEN("a many parser of \"abc\"") {
        auto parser = parsers::many0_fn<std::string>{}(
            parsers::complete::tag_fn<std::string>{}("abc"s)
        );

        WHEN("we try to parse \"abcabc\"") {
            auto result = parser("abcabc");

            THEN("it should succeed and return empty next input and vector of two \"abc\" as output ") {
                REQUIRE(
                    result == mff::parser_combinator::make_parser_result(""s, std::vector<std::string>{"abc", "abc"}));
            }
        }

        WHEN("we try to parse \"abc123\"") {
            auto result = parser("abc123");

            THEN("it should succeed and return \"123\" as next input and vector of one \"abc\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("123"s, std::vector<std::string>{"abc"}));
            }
        }

        WHEN("we try to parse \"123123\"") {
            auto result = parser("123123");

            THEN("it should succeed and return \"123123\" as next input and empty vector as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("123123"s, std::vector<std::string>{}));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should succeed and return empty next input and empty vector as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, std::vector<std::string>{}));
            }
        }
    }
}

SCENARIO("there exists a separated_list parser") {
    GIVEN("A separated list parser of string \"abc\" separated by \"|\"") {
        auto parser = parsers::separated_list_fn<std::string>{}(
            parsers::complete::tag_fn<std::string>{}("|"s),
            parsers::complete::tag_fn<std::string>{}("abc"s)
        );

        WHEN("we try to parse \"abc|abc|abc\"") {
            auto result = parser("abc|abc|abc");

            THEN("it should succeed and return empty next input and vector containing 3 \"abc\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    std::vector<std::string>{"abc", "abc", "abc"}
                ));
            }
        }

        WHEN("we try to parse \"abc123abc\"") {
            auto result = parser("abc123abc");

            THEN("it should succeed and return \"123abc\" as next input and vector containing \"abc\" as output ") {
                REQUIRE(
                    result == mff::parser_combinator::make_parser_result("123abc"s, std::vector<std::string>{"abc"}));
            }
        }

        WHEN("we try to parse \"abc|def\"") {
            auto result = parser("abc|def");

            // Do not consume separator not followed by valid item
            THEN("it should succeed and return \"|def\" as next input and vector containing \"abc\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("|def"s, std::vector<std::string>{"abc"}));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should succeed and return \"\" as next input and empty vector as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, std::vector<std::string>{}));
            }
        }

        WHEN("we try to parse \"def|abc\"") {
            auto result = parser("def|abc");

            THEN("it should succeed and return \"def|abc\" as next input and empty vector as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("def|abc"s, std::vector<std::string>{}));
            }
        }
    }

    GIVEN("a separated nonempty list parser of string \"abc\" separated by \"|\"") {
        auto parser = parsers::separated_nonempty_list_fn<std::string>{}(
            parsers::complete::tag_fn<std::string>{}("|"s),
            parsers::complete::tag_fn<std::string>{}("abc"s)
        );

        WHEN("we try to parse \"abc|abc|abc\"") {
            auto result = parser("abc|abc|abc");

            THEN("it should succeed and return empty next input and vector containing 3 \"abc\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    std::vector<std::string>{"abc", "abc", "abc"}
                ));
            }
        }

        WHEN("we try to parse \"abc123abc\"") {
            auto result = parser("abc123abc");

            THEN("it should succeed and return \"123abc\" as next input and vector containing \"abc\" as output ") {
                REQUIRE(
                    result == mff::parser_combinator::make_parser_result("123abc"s, std::vector<std::string>{"abc"}));
            }
        }

        WHEN("we try to parse \"abc|def\"") {
            auto result = parser("abc|def");

            // Do not consume separator not followed by valid item
            THEN("it should succeed and return \"|def\" as next input and vector containing \"abc\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("|def"s, std::vector<std::string>{"abc"}));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should fail") {
                REQUIRE(result
                            == mff::parser_combinator::make_parser_result_error<std::string, std::vector<std::string>>(
                                "",
                                error::ErrorKind::Tag
                            ));
            }
        }

        WHEN("we try to parse \"def|abc\"") {
            auto result = parser("def|abc");

            THEN("it should fail") {
                REQUIRE(result
                            == mff::parser_combinator::make_parser_result_error<std::string, std::vector<std::string>>(
                                "def|abc",
                                error::ErrorKind::Tag
                            ));
            }
        }
    }
}

SCENARIO("there exists a tuple parser") {
    GIVEN("a tuple parser of (alpha1, digit1, alpha1)") {
        auto parser = parsers::tuple_fn<std::string>{}(
            parsers::complete::alpha1_fn<std::string>{},
            parsers::complete::digit1_fn<std::string>{},
            parsers::complete::alpha1_fn<std::string>{}
        );

        using parser_result_t = mff::parser_combinator::utils::parser_output_t<decltype(parser), std::string>;

        WHEN("we try to parse \"abc123def\"") {
            auto result = parser("abc123def");

            THEN("it should succeed and return empty next input and vector of two \"abc\" as output ") {
                REQUIRE(
                    result == mff::parser_combinator::make_parser_result(""s, std::make_tuple("abc"s, "123"s, "def"s)));
            }
        }


        WHEN("we try to parse \"123def\"") {
            auto result = parser("123def");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, parser_result_t>(
                    "123def",
                    error::ErrorKind::Alpha
                ));
            }
        }
    }
}