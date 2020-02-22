#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::default_error <Input>, typename Parser1, typename Parser2>
auto terminated(Parser1 first, Parser2 second) {
    using Output = utils::parser_output_t<Parser1, Input>;

    return [first, second](const Input& input) -> parser_result <Input, Output, Error> {
        auto first_result = TRY(first(input));
        auto second_result = TRY(second(first_result.next_input));

        return make_parser_result(second_result.next_input, first_result.output);
    };
}

}