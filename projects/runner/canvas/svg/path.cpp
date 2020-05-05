#include "path.h"

#include <string_view>

#include <mff/utils.h>
#include <mff/parser_combinator/parsers.h>

namespace canvas::svg {

bool is_path_space(char c) {
    return c == 0x20 || c == 0x9 || c == 0xA || c == 0xD || c == 0xC;
}

bool is_path_digit(char c) {
    return isdigit(c);
}

using input_t = std::string_view;
using error_t = mff::parser_combinator::error::DefaultError<input_t>;

namespace parsers = mff::parser_combinator::parsers;

struct Ignore {};

template <typename Input, typename Error = mff::parser_combinator::error::DefaultError<Input>, typename Parser>
auto ignore(Parser p) {
    return parsers::combinator::map<Input, Error, Parser>(p, [](auto i) -> Ignore { return Ignore{}; });
}

auto parse_space(const input_t& input) {
    return ignore<input_t, error_t>(
        parsers::complete::take_while1<input_t, error_t>(
            is_path_space
        ))(input);
}

auto parse_space_optional(const input_t& input) {
    return ignore<input_t, error_t>(
        parsers::complete::take_while<input_t, error_t>(
            is_path_space
        ))(input);
}

auto parse_comma_separator(const input_t& input) {
    auto symbol = parsers::complete::tag<input_t, error_t>(input_t(","));

    return parsers::alt<input_t, error_t>(
        ignore<input_t, error_t>(
            parsers::tuple<input_t, error_t>(
                ignore<input_t, error_t>(parse_space),
                ignore<input_t, error_t>(parsers::combinator::opt<input_t, error_t>(symbol)),
                ignore<input_t, error_t>(parse_space_optional)
            )),
        ignore<input_t, error_t>(
            parsers::tuple<input_t, error_t>(
                ignore<input_t, error_t>(symbol),
                ignore<input_t, error_t>(parse_space_optional))
        )
    )(input);
}

auto parse_comma_separator_optional(const input_t& input) {
    return parsers::combinator::opt<input_t, error_t>(parse_comma_separator)(input);
}

namespace Sign_ {

struct Positive {};
struct Negative {};

}

using Sign = std::variant<Sign_::Negative, Sign_::Positive>;

auto parse_sign(const input_t& input) {
    return parsers::alt<input_t, error_t>(
        parsers::combinator::map<input_t, error_t>(
            parsers::complete::char_p<input_t, error_t>('+'),
            [](auto i) -> Sign { return Sign_::Positive{}; }
        ),
        parsers::combinator::map<input_t, error_t>(
            parsers::complete::char_p<input_t, error_t>('-'),
            [](auto i) -> Sign { return Sign_::Negative{}; }
        )
    )(input);
}

auto parse_number(const input_t& input) {
    auto sign = parsers::combinator::opt<input_t, error_t>(parse_sign);
    auto numbers = parsers::complete::digit1<input_t, error_t>;
    auto dot = parsers::complete::char_p<input_t, error_t>('.');
    auto decimal = parsers::preceded<input_t, error_t>(dot, numbers);

    return parsers::combinator::map<input_t, error_t>(
        parsers::tuple<input_t, error_t>(
            sign,
            numbers,
            parsers::combinator::opt<input_t, error_t>(decimal)
        ),
        [](auto r) -> std::float_t {
            auto[sign, numbers, decimals] = r;

            auto coef = std::visit(
                mff::overloaded{
                    [](Sign_::Positive i) { return 1.0f; },
                    [](Sign_::Negative i) { return -1.0f; }
                },
                sign.value_or(Sign_::Positive{})
            );

            std::float_t dec = 0.0f;

            if (decimals) {
                dec = std::stof("0." + std::string(decimals.value()));
            }

            return (std::stof(std::string(numbers)) + dec) * coef;
        }
    )(input);
}

auto parse_coordinate_sequence(const input_t& input) {
    return parsers::many1<input_t, error_t>(
        parsers::preceded<input_t, error_t>(
            parse_comma_separator,
            parse_number
        )
    )(input);
}

auto parse_coordinate_pair(const input_t& input) {
    auto parse_pair = parsers::pair<input_t, error_t>(
        parse_number,
        parsers::preceded<input_t, error_t>(
            parse_comma_separator_optional,
            parse_number
        ));

    return parsers::combinator::map<input_t, error_t>(
        parse_pair,
        [](auto i) -> mff::Vector2f { return {i.first, i.second}; }
    )(input);
}

auto parse_coordinate_pair_sequence(const input_t& input) {
    return parsers::many1<input_t, error_t>(
        parsers::preceded<input_t, error_t>(
            parse_comma_separator_optional,
            parse_coordinate_pair
        )
    )(input);
}

auto parse_coordinate_pair_double(const input_t& input) {
    auto parse_triplet = parsers::tuple<input_t, error_t>(
        parse_coordinate_pair,
        parsers::preceded<input_t, error_t>(parse_comma_separator_optional, parse_coordinate_pair)
    );

    return parse_triplet(input);
}

auto parse_coordinate_pair_double_sequence(const input_t& input) {
    return parsers::many1<input_t, error_t>(
        parsers::preceded<input_t, error_t>(
            parse_comma_separator_optional,
            parse_coordinate_pair_double
        )
    )(input);
}

auto parse_coordinate_pair_triplet(const input_t& input) {
    auto parse_preceded_pair = parsers::preceded<input_t, error_t>(
        parse_comma_separator_optional,
        parse_coordinate_pair
    );
    auto parse_triplet = parsers::tuple<input_t, error_t>(
        parse_coordinate_pair,
        parse_preceded_pair,
        parse_preceded_pair
    );

    return parse_triplet(input);
}


auto parse_coordinate_pair_triplet_sequence(const input_t& input) {
    return parsers::many1<input_t, error_t>(
        parsers::preceded<input_t, error_t>(
            parse_comma_separator_optional,
            parse_coordinate_pair_triplet
        )
    )(input);
}


auto parse_command_letter_factory(char symbol) {
    return parsers::alt<input_t, error_t>(
        parsers::combinator::value<input_t, error_t>(
            Position::Absolute,
            parsers::complete::char_p<input_t, error_t>(std::toupper(symbol))),
        parsers::combinator::value<input_t, error_t>(
            Position::Relative,
            parsers::complete::char_p<input_t, error_t>(
                std::tolower(symbol))));
}


auto parse_moveto(const input_t& input) {
    auto command = parse_command_letter_factory('M');
    auto parser = parsers::tuple<input_t, error_t>(
        command,
        parse_comma_separator_optional,
        parse_coordinate_pair_sequence
    );

    return parsers::combinator::map<input_t, error_t>(
        parser, [](auto i) -> Command {
            auto[pos, ign, coordinates] = i;
            return Commands_::Moveto{pos, coordinates};
        }
    )(input);
}

auto parse_closepath(const input_t& input) {
    auto command = parse_command_letter_factory('Z');

    return parsers::combinator::value<input_t, error_t>(Command{Commands_::Closepath{}}, command)(input);
}

auto parse_lineto(const input_t& input) {
    auto command = parse_command_letter_factory('L');
    auto parser = parsers::tuple<input_t, error_t>(
        command,
        parse_comma_separator_optional,
        parse_coordinate_pair_sequence
    );

    return parsers::combinator::map<input_t, error_t>(
        parser, [](auto i) -> Command {
            auto[pos, ign, coordinates] = i;
            return Commands_::Lineto{pos, coordinates};
        }
    )(input);
}

auto parse_horizontal_lineto(const input_t& input) {
    auto command = parse_command_letter_factory('H');
    auto parser = parsers::tuple<input_t, error_t>(
        command,
        parse_comma_separator_optional,
        parse_coordinate_sequence
    );

    return parsers::combinator::map<input_t, error_t>(
        parser, [](auto i) -> Command {
            auto[pos, ign, coordinates] = i;
            return Commands_::HorizontalLineto{pos, coordinates};
        }
    )(input);
}

auto parse_vertical_lineto(const input_t& input) {
    auto command = parse_command_letter_factory('V');
    auto parser = parsers::tuple<input_t, error_t>(
        command,
        parse_comma_separator_optional,
        parse_coordinate_sequence
    );

    return parsers::combinator::map<input_t, error_t>(
        parser, [](auto i) -> Command {
            auto[pos, ign, coordinates] = i;
            return Commands_::VerticalLineto{pos, coordinates};
        }
    )(input);
}

auto parse_curveto(const input_t& input) {
    auto command = parse_command_letter_factory('C');
    auto parser = parsers::tuple<input_t, error_t>(
        command,
        parse_comma_separator_optional,
        parse_coordinate_pair_triplet_sequence
    );

    return parsers::combinator::map<input_t, error_t>(
        parser, [](auto i) -> Command {
            auto[pos, ign, coordinates] = i;
            return Commands_::Curveto{pos, coordinates};
        }
    )(input);
}

auto parse_smooth_curveto(const input_t& input) {
    auto command = parse_command_letter_factory('S');
    auto parser = parsers::tuple<input_t, error_t>(
        command,
        parse_comma_separator_optional,
        parse_coordinate_pair_double_sequence
    );

    return parsers::combinator::map<input_t, error_t>(
        parser, [](auto i) -> Command {
            auto[pos, ign, coordinates] = i;
            return Commands_::SmoothCurveto{pos, coordinates};
        }
    )(input);
}

auto parse_command(const input_t& input) {
    return parsers::alt<input_t, error_t>(
        parse_lineto,
        parse_closepath,
        parse_horizontal_lineto,
        parse_vertical_lineto,
        parse_curveto,
        parse_smooth_curveto,
        parse_moveto

    )(input);
}

auto parse_path_internal(const input_t& input) {
    auto parse_first_moveto = parsers::preceded<input_t, error_t>(parse_space_optional, parse_moveto);
    auto parse_commands = parsers::many0<input_t, error_t>(
        parsers::preceded<input_t, error_t>(
            parse_space_optional,
            parse_command
        ));

    auto parse_first_then_rest = parsers::tuple<input_t, error_t>(parse_first_moveto, parse_commands);

    return parsers::combinator::map<input_t, error_t>(
        parse_first_then_rest, [](auto i) {
            auto[first, rest] = i;

            rest.insert(rest.begin(), first);

            return rest;
        }
    )(input);
}

boost::leaf::result<std::vector<Command>> parse_path(const std::string& input) {
    auto result = parse_path_internal(input);

    if (!result) return LEAF_NEW_ERROR();

    return std::move(result->output);
}

}