#pragma once

#include <variant>
#include <tl/expected.hpp>

#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_kind.h>
#include <mff/parser_combinator/error/error_traits.h>
#include <mff/parser_combinator/error/parser_error.h>
#include <mff/parser_combinator/parser_result_inner.h>

namespace mff::parser_combinator {

template <typename Input, typename Output, typename Error = error::default_error<Input>>
using parser_result = tl::expected<parser_result_inner<Input, Output>, error::parser_error<Error>>;


template <typename Input, typename Error = error::default_error<Input>>
error::parser_error<Error> make_parser_error(const Input& input, error::ErrorKind kind) {
    error::parser_error_traits<Input, Error> traits;

    return error::parser_error<Error>(error::types::err(traits.from_error_kind(input, kind)));
}

template <typename Input, typename Error = error::default_error<Input>>
error::parser_error<Error> make_parser_error(const Input& input, char c) {
    error::parser_error_traits<Input, Error> traits;

    return error::parser_error<Error>(error::types::err(traits.from_char(input, c)));
}

template <typename Input, typename Output, typename Error = error::default_error<Input>>
parser_result<Input, Output, Error> make_parser_result_error(const Input& input, error::ErrorKind kind) {
    return tl::make_unexpected(make_parser_error<Input, Error>(input, kind));
}

template <typename Input, typename Output, typename Error = error::default_error<Input>>
parser_result<Input, Output, Error> make_parser_result_error(const Input& input, char c) {
    return tl::make_unexpected(make_parser_error<Input, Error>(input, c));
}

template <typename Input, typename Output, typename Error = error::default_error<Input>>
parser_result<Input, Output, Error> make_parser_result_incomplete() {
    return tl::make_unexpected(error::parser_error<Error>(error::types::incomplete{}));
}

template <typename Input, typename Output, typename Error = error::default_error<Input>>
parser_result<Input, Output, Error> make_parser_result(const Input& input, Output output) {
    return parser_result_inner(input, output);
}

}