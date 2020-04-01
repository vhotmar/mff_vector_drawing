#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <
    typename Input,
    typename Error = error::DefaultError <Input>,
    typename Parser1,
    typename SeparatorParser,
    typename Parser2
>
auto delimited(Parser1 first, SeparatorParser sep, Parser2 second) {
    using Output = utils::parser_output_t<SeparatorParser, Input>;

    return [first, sep, second](const Input& input) -> ParserResult <Input, Output, Error> {
        auto first_result = TRY(first(input));
        auto sep_result = TRY(sep(first_result.next_input));
        auto second_result = TRY(second(sep_result.next_input));

        return make_parser_result<Input, Output, Error>(second_result.next_input, std::move(sep_result.output));
    };
}

}