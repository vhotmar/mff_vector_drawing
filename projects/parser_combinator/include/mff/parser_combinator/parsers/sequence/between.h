#pragma once

#include <mff/parser_combinator/parsers/sequence/delimited.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::DefaultError <Input>>
struct between_fn {
    template <typename Parser, typename SeparatorParser>
    auto operator()(Parser parser, SeparatorParser sep) const {
        using Output = utils::parser_output_t<SeparatorParser, Input>;

        return delimited_fn < Input, Error > {}(parser, sep, parser);
    }
};

}