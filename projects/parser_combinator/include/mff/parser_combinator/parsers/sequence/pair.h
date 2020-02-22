#pragma once

#include <mff/parser_combinator/traits/comparer.h>
#include <mff/parser_combinator/traits/input_take.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_traits.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <
    typename Input,
    typename Error = error::default_error <Input>,
    typename Parser1,
    typename Parser2
>
auto pair(
    Parser1 first,
    Parser2 second
) {
    using Parser1Output = utils::parser_output_t<Parser1, Input>;
    using Parser2Output = utils::parser_output_t<Parser2, Input>;
    using Output = std::pair<Parser1Output, Parser2Output>;

    return [first, second](const Input& input) -> parser_result <Input, Output, Error> {
        auto first_result = TRY(first(input));
        auto second_result = TRY(second(first_result.next_input));

        return make_parser_result<Input, Output, Error>(
            second_result.next_input,
            std::make_pair(
                first_result.output,
                second_result.output
            )
        );
    };
}

}