#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <
    typename Input,
    typename Error = error::DefaultError <Input>,
    typename Parser1,
    typename Parser2
>
auto preceded(Parser1 first, Parser2 second) {
    using Output = utils::parser_output_t<Parser2, Input>;

    return [first, second](const Input& input) -> ParserResult <Input, Output, Error> {
        auto first_result = first(input);
        if (!first_result) return tl::make_unexpected(first_result.error());

        return second(first_result->next_input);
    };
}

}