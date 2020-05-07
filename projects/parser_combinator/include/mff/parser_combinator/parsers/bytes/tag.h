#pragma once

#include <mff/parser_combinator/traits/comparer.h>
#include <mff/parser_combinator/traits/input_take.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/error/default_error.h>
#include <mff/parser_combinator/error/error_traits.h>

namespace mff::parser_combinator::parsers {

namespace streaming {

template <typename Input, typename Error = error::DefaultError<Input>>
struct tag_fn {
    auto operator()(const Input& tag) const {
        return [tag](const Input& input) -> ParserResult<Input, Input, Error> {
            Input input_copy(input);

            auto size = tag.length();

            auto compare_result = traits::compare(input_copy, tag);

            if (compare_result == traits::CompareResult::ok) {
                return traits::input::take_length(input, size);
            }

            if (compare_result == traits::CompareResult::incomplete) {
                return make_parser_result_incomplete<Input, Input, Error>();
            }

            return make_parser_result_error<Input, Input, Error>(input, error::ErrorKind::Tag);
        };
    }
};

}

namespace complete {

template <typename Input, typename Error = error::DefaultError<Input>>
struct tag_fn {
    auto operator()(const Input& tag) const {
        return [tag](const Input& input) -> ParserResult<Input, Input, Error> {
            Input input_copy(input);

            auto size = tag.length();

            auto compare_result = traits::compare(input_copy, tag);

            if (compare_result == traits::CompareResult::ok) {
                return traits::input::take_length(input, size);
            }

            return make_parser_result_error<Input, Input, Error>(input, error::ErrorKind::Tag);
        };
    }
};

}

}
