#pragma once

#include <mff/parser_combinator/traits/input_take_at_position.h>
#include <mff/parser_combinator/traits/input_iterator.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_traits.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::complete {

template <typename Input, typename Error = error::DefaultError <Input>>
struct take_while_fn {
    template <typename Predicate>
    auto operator()(Predicate predicate) const {
        return [predicate](const Input& input) -> ParserResult <Input, Input, Error> {
            auto predicate_complement = [predicate](auto c) { return !predicate(c); };

            return traits::input::split_at_position_complete<Input, Error>(
                input,
                predicate_complement
            );
        };
    }
};

template <typename Input, typename Error = error::DefaultError <Input>>
struct take_while1_fn {
    template <typename Predicate>
    auto operator()(Predicate predicate) const {
        return [predicate](const Input& input) -> ParserResult <Input, Input, Error> {
            auto predicate_complement = [predicate](auto c) { return !predicate(c); };

            return traits::input::split_at_position1_complete<Input, Error>(
                input,
                predicate_complement,
                error::ErrorKind::TakeWhile1
            );
        };
    }
};

template <typename Input, typename Error = error::DefaultError <Input>>
struct take_while_m_n_fn {
    template <typename Predicate>
    auto operator()(size_t m, size_t n, Predicate predicate) const {
        return [predicate, m, n](const Input& input) -> ParserResult <Input, Input, Error> {
            auto predicate_complement = [predicate](auto c) { return !predicate(c); };

            auto position = traits::iterator::position(input, predicate_complement);
            auto end = position == std::nullopt ? traits::iterator::length(input) : *position;

            if (end < m) {
                return make_parser_result_error<Input, Input, Error>(input, error::ErrorKind::TakeWhileMN);
            }

            return traits::input::take_length(input, std::min(n, end));
        };
    }
};

template <typename Input, typename Error = error::DefaultError <Input>>
struct take_fn {
    auto operator()(size_t count) const {
        return [count](const Input& input) -> ParserResult <Input, Input, Error> {
            auto length = traits::iterator::length(input);

            if (length < count) return make_parser_result_error<Input, Input, Error>(input, error::ErrorKind::Eof);

            return traits::input::take_length(input, count);
        };
    }
};

}