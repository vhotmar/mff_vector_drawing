#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::DefaultError <Input>, typename Value>
auto constant(Value val) {
    return [val](const Input& input) -> ParserResult <Input, Value, Error> {
        return make_parser_result(input, val);
    };
}

}