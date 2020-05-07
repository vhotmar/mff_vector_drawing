#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::DefaultError <Input>>
struct value_fn {
    template <typename ValueType, typename Parser>
    auto operator()(ValueType val, Parser parser) const {
        return [val, parser](const Input& input) -> ParserResult <Input, ValueType, Error> {
            auto result = parser(input);
            if (!result) return tl::make_unexpected(result.error());

            ValueType copy(val);

            return make_parser_result(result->next_input, std::move(copy));
        };
    }
};

}