#pragma once

#include <utility>

#include <mff/parser_combinator/parser_result.h>
#include <mff/parser_combinator/utils.h>

namespace mff::parser_combinator::parsers {

namespace detail {

template <typename, typename>
struct prepend_type_to_tuple;

template <typename T, typename... Args>
struct prepend_type_to_tuple<T, std::tuple<Args...>> {
    using type = std::tuple<T, Args...>;
};

template <typename Input, typename Error = error::DefaultError <Input>, typename Parser>
auto tuple_impl(Parser parser) {
    using Output = std::tuple<utils::parser_output_t < Parser, Input>>;

    return [parser](const Input& input) -> ParserResult <Input, Output, Error> {
        auto result = parser(input);
        if (!result) return tl::make_unexpected(result.error());

        // if we got only one parser redirect the output
        return make_parser_result(result->next_input, std::make_tuple(result->output));
    };
}

template <typename Input, typename Error = error::DefaultError <Input>, typename Parser, typename... Others>
auto tuple_impl(Parser parser, Others... others) {
    auto base_parser = tuple_impl<Input, Error, Others...>(others...);

    using CurrentOutput = utils::parser_output_t<Parser, Input>;
    using BaseOutput = utils::parser_output_t<decltype(base_parser), Input>;

    using Output = typename prepend_type_to_tuple<CurrentOutput, BaseOutput>::type;

    return [parser, base_parser](const Input& input) -> ParserResult <Input, Output, Error> {
        auto parser_result = parser(input);
        if (!parser_result) return tl::make_unexpected(parser_result.error());

        auto base_result = base_parser(parser_result->next_input);
        if (!base_result) return tl::make_unexpected(base_result.error());

        return make_parser_result(
            base_result->next_input,
            std::tuple_cat(std::make_tuple(parser_result->output), base_result->output));
    };
}

}

/**
 * tries list of parsers one by one until one succeeds
 */
template <typename Input, typename Error = error::DefaultError <Input>>
struct tuple_fn {
    template <typename... Parsers>
    auto operator()(Parsers... parsers) const {
        return detail::tuple_impl<Input, Error, Parsers...>(parsers...);
    }
};

}