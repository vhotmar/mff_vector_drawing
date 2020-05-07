#pragma once

#include <optional>

#include <mff/parser_combinator/parsers/combinator/map.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::DefaultError <Input>>
struct verify_fn {
    template <typename Parser, typename Validator>
    auto operator()(Parser parser, Validator validator) const {
        using Output = utils::parser_output_t<Parser, Input>;

        return [parser, validator](const Input& input) -> ParserResult <Input, Output, Error> {
            auto res = parser(input);
            if (!res) return tl::make_unexpected(res.error());

            if (!validator(res->output))
                return make_parser_result_error<Input, Output, Error>(
                    input,
                    error::ErrorKind::Verify
                );

            return res;
        };
    }
};

}