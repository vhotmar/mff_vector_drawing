#include <string>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/traits/input_take_at_position.h>

namespace traits = mff::parser_combinator::traits;

SCENARIO("we can split an input based on predicate") {
    GIVEN("A string \"Hello world!\"") {
        std::string a = "Hello world!";

        WHEN("we split the the string at space") {
            THEN("we should get them") {
                auto res = traits::input::split_at_position(a, [](char c) { return isspace(c); });

                REQUIRE_THAT(res->next_input, Catch::Equals(" world!"));
                REQUIRE_THAT(res->output, Catch::Equals("Hello"));
            }
        }
    }
}