#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>
#include <mff/parser_combinator/traits/input_offset.h>
#include <mff/parser_combinator/traits/input_take_at_position.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::DefaultError<Input>>
struct recognize_fn {
    template <typename Parser>
    auto operator()(Parser parser) const {
        return [parser](const Input& input) -> ParserResult<Input, Input, Error> {
            auto result = parser(input);
            if (!result) return tl::make_unexpected(result.error());

            auto length = traits::input::offset(input, result->next_input);

            Input copy(input);

            return traits::input::take_length(input, length);
        };
    }
};

}