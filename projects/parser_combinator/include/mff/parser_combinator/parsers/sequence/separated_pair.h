#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <
    typename Input,
    typename Error = error::default_error <Input>,
    typename Parser1,
    typename SeparatorParser,
    typename Parser3
>
auto separated_pair(Parser1 first, SeparatorParser sep, Parser3 second) {
    using Parser1_Output = utils::parser_output_t<Parser1, Input>;
    using Parser3_Output = utils::parser_output_t<Parser3, Input>;
    using Output = std::pair<Parser1_Output, Parser3_Output>;

    return [first, sep, second](const Input& input) -> parser_result <Input, Output, Error> {
        auto first_result = TRY(first(input));
        auto sep_result = TRY(sep(first_result.next_input));
        auto second_result = TRY(second(sep_result.next_input));

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