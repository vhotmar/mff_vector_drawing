#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::DefaultError <Input>>
struct separated_pair_fn {
    template <typename Parser1, typename SeparatorParser, typename Parser3>
    auto operator()(Parser1 first, SeparatorParser sep, Parser3 second) const {
        using Parser1_Output = utils::parser_output_t<Parser1, Input>;
        using Parser3_Output = utils::parser_output_t<Parser3, Input>;
        using Output = std::pair<Parser1_Output, Parser3_Output>;

        return [first, sep, second](const Input& input) -> ParserResult <Input, Output, Error> {
            auto first_result = first(input);
            if (!first_result) return tl::make_unexpected(first_result.error());

            auto sep_result = sep(first_result->next_input);
            if (!sep_result) return tl::make_unexpected(sep_result.error());

            auto second_result = second(sep_result->next_input);
            if (!second_result) return tl::make_unexpected(second_result.error());

            return make_parser_result<Input, Output, Error>(
                second_result->next_input,
                std::make_pair(
                    first_result->output,
                    second_result->output
                )
            );
        };
    }
};

}