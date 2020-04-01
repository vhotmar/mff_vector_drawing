#pragma once

#include <vector>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::DefaultError <Input>, typename Parser>
auto many0(Parser parser) {
    using POutput = utils::parser_output_t<Parser, Input>;
    using Output = std::vector<POutput>;

    return [parser](const Input& input) -> ParserResult <Input, Output, Error> {
        Output result;
        Input i(input);

        while (true) {
            auto parser_result = parser(i);

            if (!parser_result) {
                auto error = parser_result.error();

                if (error.is_error()) {
                    return make_parser_result<Input, Output, Error>(i, std::move(result));
                }

                return tl::make_unexpected(error);
            }

            if (i == parser_result->next_input) {
                return make_parser_result_error<Input, Output, Error>(i, error::ErrorKind::Many0);
            }

            result.push_back(parser_result->output);
            i = parser_result->next_input;
        }
    };
}

}