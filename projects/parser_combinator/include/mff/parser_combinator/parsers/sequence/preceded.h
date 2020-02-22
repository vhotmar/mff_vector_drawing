#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <
    typename Input,
    typename Error = error::default_error <Input>,
    typename Parser1,
    typename Parser2
>
auto preceded(Parser1 first, Parser2 second) {
    using Output = utils::parser_output_t<Parser2, Input>;

    return [first, second](const Input& input) -> parser_result <Input, Output, Error> {
        auto first_result = TRY(first(input));

        return second(first_result.next_input);
    };
}

}