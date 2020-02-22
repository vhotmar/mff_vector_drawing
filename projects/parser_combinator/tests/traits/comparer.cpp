#include <string>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/traits/comparer.h>

namespace traits = mff::parser_combinator::traits;

SCENARIO("we can compare") {
    GIVEN("A string \"Hello!\"") {
        std::string a = "Hello!";

        WHEN("we compare it with a string \"Hello\"") {
            std::string b = "Hello";

            THEN("they should match") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::ok);
            }
        }

        WHEN("we compare it with a string \"Ciao\"") {
            std::string b = "Ciao";

            THEN("they should not match") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::error);
            }
        }

        WHEN("we compare it with string \"Hallo! Lieblings\"") {
            std::string b = "Hallo! Lieblings";

            THEN("they should not match") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::error);
            }
        }

        WHEN("we compare it with string \"Hello! Lieblings\"") {
            std::string b = "Hello! Lieblings";

            THEN("they should be incomplete") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::incomplete);
            }
        }
    }

    GIVEN("A vector of chars containing a \"Hello!\"") {
        std::string a_s = "Hello!";
        std::vector<char> a(a_s.begin(), a_s.end());

        WHEN("we compare it with a vector of chars containing a \"Hello\"") {
            std::string b_s = "Hello";
            std::vector<char> b(b_s.begin(), b_s.end());

            THEN("they should match") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::ok);
            }
        }

        WHEN("we compare it with a vector of chars containing a \"Ciao\"") {
            std::string b_s = "Ciao";
            std::vector<char> b(b_s.begin(), b_s.end());

            THEN("they should not match") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::error);
            }
        }

        WHEN("we compare it with vector of chars containing a \"Hallo! Lieblings\"") {
            std::string b_s = "Hallo! Lieblings";
            std::vector<char> b(b_s.begin(), b_s.end());

            THEN("they should not match") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::error);
            }
        }

        WHEN("we compare it with vector of chars containing a \"Hello! Lieblings\"") {
            std::string b_s = "Hello! Lieblings";
            std::vector<char> b(b_s.begin(), b_s.end());

            THEN("they should be incomplete") {
                REQUIRE(traits::compare(a, b) == traits::compare_result::incomplete);
            }
        }
    }
}