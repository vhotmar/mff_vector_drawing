#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::DefaultError <Input>>
struct terminated_fn {
    template <typename Parser1, typename Parser2>
    auto operator()(Parser1 first, Parser2 second) const {
        using Output = utils::parser_output_t<Parser1, Input>;

        return [first, second](const Input& input) -> ParserResult <Input, Output, Error> {
            auto first_result = first(input);
            if (!first_result) return tl::make_unexpected(first_result.error());

            auto second_result = second(first_result->next_input);
            if (!second_result) return tl::make_unexpected(second_result.error());

            return make_parser_result(second_result->next_input, std::move(first_result->output));
        };
    }
};

}