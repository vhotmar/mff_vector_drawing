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
auto char_p(
    char c
) {
    using value_type = traits::iterator::value_type_t<Input>;

    return [c](const Input& input) -> ParserResult<Input, char, Error> {
        auto begin = traits::iterator::begin(input);
        auto end = traits::iterator::end(input);

        if (begin == end || traits::as_char::as_char<value_type>(*begin) != c)
            return make_parser_result_error<Input, char, Error>(input, c);

        char copy = c;

        return make_parser_result(traits::input::slice(input, 1, traits::iterator::length(input)), std::move(copy));
    };
}

}