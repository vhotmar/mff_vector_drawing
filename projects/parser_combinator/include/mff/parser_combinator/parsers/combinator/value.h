#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::DefaultError <Input>, typename ValueType, typename Parser>
auto value(ValueType val, Parser parser) {
    return [val, parser](const Input& input) -> ParserResult <Input, ValueType, Error> {
        auto result = TRY(parser(input));

        ValueType copy(val);

        return make_parser_result(result.next_input, std::move(copy));
    };
}

}