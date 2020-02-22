#include <string>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/traits/input_iterator.h>

namespace traits = mff::parser_combinator::traits;

SCENARIO("we can iterate input type") {
    GIVEN("A string \"Hello world!\"") {
        std::string a = "Hello world!";

        WHEN("we try to get an iterator") {
            auto iter = traits::iterator::begin(a);

            THEN("it should reference to first character") {
                REQUIRE(*iter == 'H');
            }
        }
    }
}