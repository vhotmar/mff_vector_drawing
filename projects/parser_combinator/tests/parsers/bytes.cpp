#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

SCENARIO("there exists a is_a parser") {
    GIVEN("A delimited parser of string \"abc|efg\" (where \"abc\" and \"efg\" are left and right parens)") {
        auto parser = parsers::complete::is_a_fn<std::string>{}("123456789ABCDEF"s);

        WHEN("we try to parse \"123 and voila\"") {
            auto result = parser("123 and voila");

            THEN("it should succeed and return \" and voila\" as next input and \"123\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(" and voila"s, "123"s));
            }
        }

        WHEN("we try to parse \"DEADBEEF and others\"") {
            auto result = parser("DEADBEEF and others");

            THEN("it should succeed and return \" and others\" as next input and \"DEADBEEF\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(" and others"s, "DEADBEEF"s));
            }
        }

        WHEN("we try to parse \"D15EA5E\"") {
            auto result = parser("D15EA5E");

            THEN("it should succeed and return \"\" as next input and \"D15EA5E\" as output ") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "D15EA5E"s));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    ""s,
                    error::ErrorKind::IsA
                ));
            }
        }
    }
}

SCENARIO("there exists a tag parser") {
    GIVEN("Tag parser of string \"hello\"") {
        auto parser = parsers::complete::tag_fn<std::string>{}("hello"s);

        WHEN("we try to parse \"hello world\"") {
            auto result = parser("hello world");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(" world"s, "hello"s));
            }
        }

        WHEN("we try to parse \"ciao\"") {
            auto result = parser("ciao");

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "ciao"s,
                    error::ErrorKind::Tag
                ));
            }
        }
    }

    GIVEN("Tag parser of string_view \"hello\"") {
        auto parser = parsers::complete::tag_fn<std::string_view>{}("hello");

        WHEN("we try to parse \"hello world\"") {
            auto result = parser("hello world");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(" world"sv, "hello"sv));
            }
        }
    }
}

SCENARIO("there exists a take_while parser") {
    GIVEN("take_while is alpha") {
        auto parser = parsers::complete::take_while_fn<std::string>{}([](char c) { return std::isalpha(c); });

        WHEN("we try to parse \"latin123\"") {
            auto result = parser("latin123");

            THEN("it should succeed and return \"latin\" as output and \"123\" as next input") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("123"s, "latin"s));
            }
        }

        WHEN("we try to parse \"12345\" ") {
            auto result = parser("12345");

            THEN("it should succeed and return \"\" as output and \"12345\" as next input") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("12345"s, ""s));
            }
        }

        WHEN("we try to parse \"latin\"") {
            auto result = parser("latin");

            THEN("it should succeed and return \"latin\" as output \"\" as next input") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "latin"s));;
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should succeed and return \"\" as output \"\" as next input") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, ""s));
            }
        }
    }


    GIVEN("take_while_m_n 3, 6, isalpha") {
        auto parser = parsers::complete::take_while_m_n_fn<std::string>{}(3, 6, [](char c) { return std::isalpha(c); });

        WHEN("we try to parse \"latin123\"") {
            auto result = parser("latin123");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("123"s, "latin"s));
            }
        }

        WHEN("we try to parse \"lengthy\"") {
            auto result = parser("lengthy");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("y"s, "length"s));
            }
        }

        WHEN("we try to parse \"latin\"") {
            auto result = parser("latin");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "latin"s));
            }
        }

        WHEN("we try to parse \"ed\"") {
            auto result = parser("ed");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "ed"s,
                    error::ErrorKind::TakeWhileMN
                ));
            }
        }

        WHEN("we try to parse \"12345\"") {
            auto result = parser("12345");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "12345"s,
                    error::ErrorKind::TakeWhileMN
                ));
            }
        }
    }

    GIVEN("take 6 items") {
        auto parser = parsers::complete::take_fn<std::string>{}(6);

        WHEN("we try to parse \"1234567\"") {
            auto result = parser("1234567");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("7"s, "123456"s));
            }
        }

        WHEN("we try to parse \"things\"") {
            auto result = parser("things");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "things"s));
            }
        }

        WHEN("we try to parse \"short\"") {
            auto result = parser("short");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "short"s,
                    error::ErrorKind::Eof
                ));
            }
        }

        WHEN("we try to parse \"\"") {
            auto result = parser("");

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    ""s,
                    error::ErrorKind::Eof
                ));
            }
        }
    }
}