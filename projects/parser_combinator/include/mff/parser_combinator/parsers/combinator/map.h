#pragma once

#include <optional>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

template <typename Input, typename Error = error::default_error <Input>, typename Parser, typename Map>
auto map(Parser parser, Map map_fn) {
    using ParserOutput = utils::parser_output_t<Parser, Input>;
    using Output = std::invoke_result_t<Map, ParserOutput>;

    return [map_fn, parser](const Input& input) -> parser_result <Input, Output, Error> {
        auto result = TRY(parser(input));

        return make_parser_result(result.next_input, map_fn(result.output));
    };
}

}