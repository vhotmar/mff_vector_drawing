#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>
#include <mff/parser_combinator/utils.h>

using namespace std::string_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

SCENARIO("there exists a delimited parser") {
    GIVEN("A delimited parser of string \"abc|efg\" (where \"abc\" and \"efg\" are left and right parens)") {
        auto parser = parsers::delimited<std::string>(
            parsers::complete::tag("abc"s),
            parsers::complete::tag("|"s),
            parsers::complete::tag("efg"s)
        );

        WHEN("we try to parse \"abc|efg\"") {
            auto result = parser("abc|efg");

            THEN("it should succeed and return empty next input and \"|\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "|"s));
            }
        }

        WHEN("we try to parse \"abc|efghij\"") {
            auto result = parser("abc|efghij");

            THEN("it should succeed and return \"hij\" as next input and \"|\" as output") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("hij"s, "|"s));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "",
                    error::ErrorKind::Tag
                ));
            }
        }


        WHEN("we try to parse \"123\"") {
            auto result = parser("123");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "123",
                    error::ErrorKind::Tag
                ));
            }
        }
    }
}

SCENARIO("there exists a preceded parser") {
    GIVEN("A preceded parser of string \"abc\" and \"efg\"") {
        auto parser = parsers::preceded<std::string>(
            parsers::complete::tag("abc"s),
            parsers::complete::tag("efg"s)
        );

        WHEN("we try to parse \"abcefg\"") {
            auto result = parser("abcefg");

            THEN("it should succeed and return empty next input and \"efg\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "efg"s));
            }
        }

        WHEN("we try to parse \"abcefghij\"") {
            auto result = parser("abcefghij");

            THEN("it should succeed and return \"hij\" as next input and \"efg\" as output") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("hij"s, "efg"s));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "",
                    error::ErrorKind::Tag
                ));
            }
        }


        WHEN("we try to parse \"123\"") {
            auto result = parser("123");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "123",
                    error::ErrorKind::Tag
                ));
            }
        }
    }
}

SCENARIO("there exists a pair parser") {
    GIVEN("Pair parser of string \"abc\" and \"def\"") {
        auto parser = parsers::pair<std::string>(
            parsers::complete::tag("abc"s),
            parsers::complete::tag("def"s)
        );

        WHEN("we try to parse \"abcdef\"") {
            auto result = parser("abcdef");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, std::make_pair("abc"s, "def"s)));
            }
        }

        WHEN("we try to parse \"ciao\"") {
            auto result = parser("ciao");

            THEN("it should fail") {
                REQUIRE(result
                            == mff::parser_combinator::make_parser_result_error<std::string, std::pair<std::string, std::string>>(
                                "ciao"s,
                                error::ErrorKind::Tag
                            ));
            }
        }
    }
}

SCENARIO("there exists a separated_pair parser") {
    GIVEN("A separated_pair parser of string \"abc|efg\" (where \"abc\" and \"efg\" are left and right parens)") {
        auto parser = parsers::separated_pair<std::string>(
            parsers::complete::tag("abc"s),
            parsers::complete::tag("|"s),
            parsers::complete::tag("efg"s)
        );

        WHEN("we try to parse \"abc|efg\"") {
            auto result = parser("abc|efg");

            THEN("it should succeed and return empty next input and pair (\"abc\", \"efg\") as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, std::make_pair("abc"s, "efg"s)));
            }
        }

        WHEN("we try to parse \"abc|efghij\"") {
            auto result = parser("abc|efghij");

            THEN("it should succeed and return \"hij\" as next input and pair (\"abc\", \"efg\") as output") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("hij"s, std::make_pair("abc"s, "efg"s)));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should fail") {
                REQUIRE(result
                            == mff::parser_combinator::make_parser_result_error<std::string, std::pair<std::string, std::string>>(
                                "",
                                error::ErrorKind::Tag
                            ));
            }
        }


        WHEN("we try to parse \"123\"") {
            auto result = parser("123");

            THEN("it should fail") {
                REQUIRE(result
                            == mff::parser_combinator::make_parser_result_error<std::string, std::pair<std::string, std::string>>(
                                "123",
                                error::ErrorKind::Tag
                            ));
            }
        }
    }
}