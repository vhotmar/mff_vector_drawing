#pragma once

#include <mff/parser_combinator/traits/input_take_at_position.h>
#include <mff/parser_combinator/traits/find_token.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_traits.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

namespace complete {

template <
    typename Input,
    typename Error = error::DefaultError <Input>,
    typename T
>
auto is_not(
    const T& arr
) {
    return [arr](const Input& input) -> ParserResult <Input, Input, Error> {

        return traits::input::split_at_position1_complete<Input, Error>(
            input,
            [arr](auto c) {
                return traits::input::find_token(arr, c);
            },
            error::ErrorKind::IsA
        );
    };
}

}

namespace streaming {


}

}