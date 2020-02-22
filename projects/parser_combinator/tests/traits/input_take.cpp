#include <string>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/traits/input_take.h>

namespace traits = mff::parser_combinator::traits;

SCENARIO("we can take from input type") {
    GIVEN("A string \"Hello world!\"") {
        std::string a = "Hello world!";

        WHEN("we take 5 chars") {
            THEN("we should get them") {
                REQUIRE_THAT(traits::input::take(a, 5), Catch::Equals("Hello"));
            }
        }

        WHEN("we split at 5 chars") {
            THEN("we should get first 5 chars and the rest") {
                auto r = traits::input::take_length(a, 5);

                REQUIRE_THAT(r.next_input, Catch::Equals(" world!"));
                REQUIRE_THAT(r.output, Catch::Equals("Hello"));
            }
        }
    }
}