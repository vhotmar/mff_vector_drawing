#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::default_error <Input>, typename ValueType, typename Parser>
auto value(ValueType val, Parser parser) {
    return [val, parser](const Input& input) -> parser_result <Input, ValueType, Error> {
        auto result = TRY(parser(input));

        return make_parser_result(result.next_input, val);
    };
}

}