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
    typename Error = error::DefaultError <Input>,
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

    return [first, second](const Input& input) -> ParserResult <Input, Output, Error> {
        auto first_result = first(input);
        if (!first_result) return tl::make_unexpected(first_result.error());

        auto second_result = second(first_result->next_input);
        if (!second_result) return tl::make_unexpected(second_result.error());

        return make_parser_result<Input, Output, Error>(
            second_result->next_input,
            std::make_pair(
                std::move(first_result->output),
                std::move(second_result->output)
            )
        );
    };
}

}