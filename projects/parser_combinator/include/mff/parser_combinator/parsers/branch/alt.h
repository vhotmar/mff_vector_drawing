#pragma once

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

namespace detail {

template <typename Input, typename Error = error::DefaultError<Input>, typename Parser>
auto alt_implementation(Parser parser) {
    using Output = utils::parser_output_t<Parser, Input>;

    return [parser](const Input& input) -> ParserResult<Input, Output, Error> {
        // if we got only one parser redirect the output
        return parser(input);
    };
}

template <typename Input, typename Error = error::DefaultError<Input>, typename Parser, typename... Others>
auto alt_implementation(Parser parser, Others... others) {
    auto base = alt_implementation<Input, Error>(others...);

    using Output = utils::parser_output_t<Parser, Input>;
    using OthersOutput = utils::parser_output_t<decltype(base), Input>;

    static_assert(std::is_same_v<Output, OthersOutput>, "Outputs have to be same");

    return [parser, base](const Input& input) -> ParserResult<Input, Output, Error> {
        // try the first parser
        auto first_result = parser(input);

        if (!first_result) {
            auto first_error = first_result.error();

            // if the first parser failed with recoverable error, try the other parsers
            if (first_error.is_error()) {
                auto base_result = base(input);

                if (!base_result) {
                    auto base_error = base_result.error();

                    // if the other parsers failed with recoverable error, then just return it
                    if (base_error.is_error()) {
                        return tl::make_unexpected(
                            error::ParserError<Error>(
                                error::types::Err(
                                    error::or_error < Input,
                                    Error > (*first_error.error(), *base_error.error()))));
                    }
                }

                return std::move(base_result);
            }
        }

        return std::move(first_result);
    };
}

}

/**
 * tries list of parsers one by one until one succeeds
 */
template <typename Input, typename Error = error::DefaultError<Input>, typename... Others>
auto alt(Others... others) {
    auto base = detail::alt_implementation<Input, Error, Others...>(others...);

    using Output = utils::parser_output_t<decltype(base), Input>;

    return [base](const Input& input) -> ParserResult<Input, Output, Error> {
        auto result = base(input);

        if (!result) {
            auto error = result.error();

            // if the parsers failed with recoverable error, then append alt error
            if (error.is_error()) {
                return tl::make_unexpected(
                    error::ParserError<Error>(
                        error::types::Err(
                            error::append_error(
                                input,
                                error::ErrorKind::Alt,
                                *error.error()))));
            }
        }

        return std::move(result);
    };
}

}