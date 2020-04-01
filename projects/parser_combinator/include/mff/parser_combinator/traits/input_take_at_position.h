#pragma once

#include <algorithm>

#include <mff/parser_combinator/error/error_kind.h>
#include <mff/parser_combinator/traits/input_take.h>
#include <mff/parser_combinator/traits/input_iterator.h>
#include <mff/parser_combinator/parser_result.h>

namespace mff::parser_combinator::traits {

template <typename T>
class InputTakeAtPositionTrait {
public:
    using value_type = typename InputIteratorTrait<T>::value_type;

    template <typename Error, typename P>
    ParserResult<T, T, Error> split_at_position(const T& from, P predicate) {
        auto pos = iterator::position(from, predicate);

        if (pos == std::nullopt) {
            return make_parser_result_incomplete<T, T, Error>();
        }

        return input::take_length(from, *pos);
    };

    template <typename Error, typename P>
    ParserResult<T, T, Error> split_at_position1(const T& from, P predicate, error::ErrorKind kind) {
        auto pos = iterator::position(from, predicate);

        if (pos == std::nullopt) {
            return make_parser_result_incomplete<T, T, Error>();
        }

        if (*pos == 0) {
            return make_parser_result_error<T, T, Error>(from, kind);
        }

        return input::take_length(from, *pos);
    };


    template <typename Error, typename P>
    ParserResult<T, T, Error> split_at_position_complete(const T& from, P predicate) {
        auto result = split_at_position<Error, P>(from, predicate);
        auto length = iterator::length(from);

        if (result.has_value()) return *result;

        auto error = result.error();

        if (error.is_incomplete()) return input::take_length(from, length);

        return tl::make_unexpected(error);
    }

    template <typename Error, typename P>
    ParserResult<T, T, Error> split_at_position1_complete(const T& from, P predicate, error::ErrorKind kind) {
        auto result = split_at_position1<Error, P>(from, predicate, kind);
        auto length = iterator::length(from);

        if (result.has_value()) return *result;

        auto error = result.error();

        if (error.is_incomplete()) {
            if (length == 0) {
                return make_parser_result_error<T, T, Error>(from, kind);
            }

            return input::take_length(from, length);
        }

        return tl::make_unexpected(error);
    }
};

namespace input {

template <typename T, typename Error = error::DefaultError<T>, typename P>
ParserResult<T, T, Error> split_at_position(const T& from, P predicate) {
    InputTakeAtPositionTrait<T> traits;

    return traits.template split_at_position<Error, P>(from, predicate);
}

template <typename T, typename Error = error::DefaultError<T>, typename P>
ParserResult<T, T, Error> split_at_position1(const T& from, P predicate, error::ErrorKind kind) {
    InputTakeAtPositionTrait<T> traits;

    return traits.template split_at_position1<Error, P>(from, predicate, kind);
}

template <typename T, typename Error = error::DefaultError<T>, typename P>
ParserResult<T, T, Error> split_at_position_complete(const T& from, P predicate) {
    InputTakeAtPositionTrait<T> traits;

    return traits.template split_at_position_complete<Error, P>(from, predicate);
}

template <typename T, typename Error = error::DefaultError<T>, typename P>
ParserResult<T, T, Error> split_at_position1_complete(const T& from, P predicate, error::ErrorKind kind) {
    InputTakeAtPositionTrait<T> traits;

    return traits.template split_at_position1_complete<Error, P>(from, predicate, kind);
}

}

}