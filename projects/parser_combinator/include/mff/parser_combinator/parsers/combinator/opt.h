#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::DefaultError <Input>>
struct opt_fn {
    template <typename Parser>
    auto operator()(Parser parser) const {
        using Output = std::optional<utils::parser_output_t < Parser, Input>>;

        return [parser](const Input& input) -> ParserResult <Input, Output, Error> {
            auto result = parser(input);

            if (!result) {
                auto error = result.error();

                if (error.is_error()) {
                    return make_parser_result<Input, Output, Error>(input, std::nullopt);
                }

                return tl::make_unexpected(error);
            }

            return make_parser_result<Input, Output, Error>(result->next_input, std::make_optional(result->output));
        };
    }
};

}