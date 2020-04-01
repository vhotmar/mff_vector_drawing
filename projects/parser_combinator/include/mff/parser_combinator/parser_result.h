#pragma once

#include <variant>
#include <tl/expected.hpp>

#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_kind.h>
#include <mff/parser_combinator/error/error_traits.h>
#include <mff/parser_combinator/error/ParserError.h>
#include <mff/parser_combinator/ParserResultInner.h>

namespace mff::parser_combinator {

template <typename Input, typename Output, typename Error = error::DefaultError<Input>>
using ParserResult = tl::expected<ParserResultInner<Input, Output>, error::ParserError<Error>>;


template <typename Input, typename Error = error::DefaultError<Input>>
error::ParserError<Error> make_parser_error(const Input& input, error::ErrorKind kind) {
    error::ParserErrorTraits<Input, Error> traits;

    return error::ParserError<Error>(error::types::Err(traits.from_error_kind(input, kind)));
}

template <typename Input, typename Error = error::DefaultError<Input>>
error::ParserError<Error> make_parser_error(const Input& input, char c) {
    error::ParserErrorTraits<Input, Error> traits;

    return error::ParserError<Error>(error::types::Err(traits.from_char(input, c)));
}

template <typename Input, typename Output, typename Error = error::DefaultError<Input>>
ParserResult<Input, Output, Error> make_parser_result_error(const Input& input, error::ErrorKind kind) {
    return tl::make_unexpected(make_parser_error<Input, Error>(input, kind));
}

template <typename Input, typename Output, typename Error = error::DefaultError<Input>>
ParserResult<Input, Output, Error> make_parser_result_error(const Input& input, char c) {
    return tl::make_unexpected(make_parser_error<Input, Error>(input, c));
}

template <typename Input, typename Output, typename Error = error::DefaultError<Input>>
ParserResult<Input, Output, Error> make_parser_result_incomplete() {
    return tl::make_unexpected(error::ParserError<Error>(error::types::Incomplete{}));
}

template <typename Input, typename Output, typename Error = error::DefaultError<Input>>
ParserResult<Input, Output, Error> make_parser_result(const Input& input, Output&& output) {
    return ParserResultInner(input, std::move(output));
}

}