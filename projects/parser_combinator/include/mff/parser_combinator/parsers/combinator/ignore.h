#pragma once

#include <optional>

#include <mff/parser_combinator/parsers/combinator/map.h>
#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers::combinator {

struct Ignore {};

template <typename Input, typename Error = error::DefaultError<Input>>
struct ignore_fn {
    template <typename Parser>
    auto operator()(Parser parser) const {
        return map_fn<Input, Error>{}(parser, [](auto i) -> Ignore { return Ignore{}; });
    }
};

}