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
    typename Error = error::DefaultError<Input>
>
auto alpha0(
    const Input& input
) {
    using value_type = traits::iterator::value_type_t<Input>;

    return traits::input::split_at_position_complete<Input, Error>(
        input,
        [](value_type c) { return !traits::as_char::is_alpha(c); }
    );
}

template <
    typename Input,
    typename Error = error::DefaultError<Input>
>
auto alpha1(
    const Input& input
) {
    using value_type = traits::iterator::value_type_t<Input>;

    return traits::input::split_at_position1_complete<Input, Error>(
        input,
        [](value_type c) { return !traits::as_char::is_alpha(c); },
        error::ErrorKind::Alpha
    );
}

}