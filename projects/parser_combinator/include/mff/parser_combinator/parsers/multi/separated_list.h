#pragma once

#include <vector>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::default_error <Input>, typename Parser, typename SeparatorParser>
auto separated_list(SeparatorParser separator_parser, Parser item_parser) {
    using POutput = utils::parser_output_t<Parser, Input>;
    using Output = std::vector<POutput>;

    return [separator_parser, item_parser](const Input& input) -> parser_result <Input, Output, Error> {
        Output result;

        // try to parse first item
        auto first_result = item_parser(input);

        // if parser returned error:
        // - is it recoverable? return empty list
        // - otherwise return error (failure / incomplete)
        if (!first_result) {
            auto error = first_result.error();

            if (error.is_error()) {
                return make_parser_result<Input, Output, Error>(input, result);
            }

            return tl::make_unexpected(first_result.error());
        }

        // if the parser did not consume anything then return error
        if (input == first_result->next_input) {
            return make_parser_result_error<Input, Output, Error>(input, error::ErrorKind::SeparatedList);
        }

        // otherwise we parsed first item
        result.push_back(first_result->output);

        Input i = first_result->next_input;

        // then we will consume as much (separator, item) pairs as we can
        while (true) {
            // try to parse current input
            auto separator_parser_result = separator_parser(i);

            // if separator parser returned error:
            // - is it recoverable? return current result
            // - otherwise return error (failure / incomplete)
            if (!separator_parser_result) {
                auto error = separator_parser_result.error();

                if (error.is_error()) {
                    return make_parser_result<Input, Output, Error>(i, result);
                }

                return tl::make_unexpected(error);
            }

            // if separator parser did not consume anything return error
            if (i == separator_parser_result->next_input) {
                return make_parser_result_error<Input, Output, Error>(i, error::ErrorKind::SeparatedList);
            }

            // else parse the item
            auto item_parser_result = item_parser(separator_parser_result->next_input);


            // if item parser returned error:
            // - is it recoverable? return current result
            // - otherwise return error (failure / incomplete)
            if (!item_parser_result) {
                auto error = item_parser_result.error();

                if (error.is_error()) {
                    return make_parser_result<Input, Output, Error>(i, result);
                }

                return tl::make_unexpected(error);
            }

            // if item parser did not consume anything return error
            if (i == item_parser_result->next_input) {
                return make_parser_result_error<Input, Output, Error>(i, error::ErrorKind::SeparatedList);
            }

            // ad item to results
            result.push_back(item_parser_result->output);

            // move the input
            i = item_parser_result->next_input;
        }
    };
}

template <typename Input, typename Error = error::default_error <Input>, typename Parser, typename SeparatorParser>
auto separated_nonempty_list(SeparatorParser separator_parser, Parser item_parser) {
    using POutput = utils::parser_output_t<Parser, Input>;
    using Output = std::vector<POutput>;

    return [separator_parser, item_parser](const Input& input) -> parser_result <Input, Output, Error> {
        Output result;

        // try to parse first item
        auto first_result = item_parser(input);

        // if parser returned error, then return the error
        if (!first_result) {
            return tl::make_unexpected(first_result.error());
        }

        // if the parser did not consume anything then return error
        if (input == first_result->next_input) {
            return make_parser_result_error<Input, Output, Error>(input, error::ErrorKind::SeparatedList);
        }

        // otherwise we parsed first item
        result.push_back(first_result->output);

        Input i = first_result->next_input;

        // then we will consume as much (separator, item) pairs as we can
        while (true) {
            // try to parse current input
            auto separator_parser_result = separator_parser(i);

            // if separator parser returned error:
            // - is it recoverable? return current result
            // - otherwise return error (failure / incomplete)
            if (!separator_parser_result) {
                auto error = separator_parser_result.error();

                if (error.is_error()) {
                    return make_parser_result<Input, Output, Error>(i, result);
                }

                return tl::make_unexpected(error);
            }

            // if separator parser did not consume anything return error
            if (i == separator_parser_result->next_input) {
                return make_parser_result_error<Input, Output, Error>(i, error::ErrorKind::SeparatedList);
            }

            // else parse the item
            auto item_parser_result = item_parser(separator_parser_result->next_input);


            // if item parser returned error:
            // - is it recoverable? return current result
            // - otherwise return error (failure / incomplete)
            if (!item_parser_result) {
                auto error = item_parser_result.error();

                if (error.is_error()) {
                    return make_parser_result<Input, Output, Error>(i, result);
                }

                return tl::make_unexpected(error);
            }

            // if item parser did not consume anything return error
            if (i == item_parser_result->next_input) {
                return make_parser_result_error<Input, Output, Error>(i, error::ErrorKind::SeparatedList);
            }

            // ad item to results
            result.push_back(item_parser_result->output);

            // move the input
            i = item_parser_result->next_input;
        }
    };
}

}