#pragma once

#include <mff/parser_combinator/traits/as_char.h>
#include <mff/parser_combinator/traits/input_take_at_position.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_traits.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::complete {

template <
    typename Input,
    typename Error = error::DefaultError <Input>
>
struct digit0_fn {
    auto operator()(
        const Input& input
    ) const {
        using value_type = traits::iterator::value_type_t<Input>;

        return traits::input::split_at_position_complete<Input, Error>(
            input,
            [](value_type c) { return !traits::as_char::is_dec_digit(c); }
        );
    }
};

template <
    typename Input,
    typename Error = error::DefaultError <Input>
>
struct digit1_fn {
    auto operator()(
        const Input& input
    ) const {
        using value_type = traits::iterator::value_type_t<Input>;

        return traits::input::split_at_position1_complete<Input, Error>(
            input,
            [](value_type c) { return !traits::as_char::is_dec_digit(c); },
            error::ErrorKind::Digit
        );
    }
};

}