#include "path.h"

#include <string_view>

#include <mff/utils.h>
#include <mff/parser_combinator/parsers.h>

namespace canvas::svg {

///////////////////////////////////
/// Helpers (per specitifaiton) ///
///////////////////////////////////

bool is_path_space(char c) {
    return c == 0x20 || c == 0x9 || c == 0xA || c == 0xD || c == 0xC;
}

bool is_path_digit(char c) {
    return isdigit(c);
}

using Parser = mff::parser_combinator::parsers::Parsers<std::string_view>;

/////////////////////
/// Parse numbers ///
/////////////////////


struct recognize_float {
    auto operator()(const std::string_view& input) const {
        using parsers = Parser;

        return parsers::recognize(
            parsers::tuple(
                // there is optional sign
                parsers::opt(parsers::alt(parsers::complete::char_p('+'), parsers::complete::char_p('-'))),
                // then there are two options
                parsers::alt(
                    // either the number starts with number and follows with optional decimal part
                    parsers::ignore(
                        parsers::tuple(
                            parsers::complete::digit1,
                            parsers::opt(
                                parsers::pair(
                                    parsers::complete::char_p('.'),
                                    parsers::opt(parsers::complete::digit1))))),
                    // or we get dot followed by digits (.001)
                    parsers::ignore(
                        parsers::tuple(
                            parsers::complete::char_p('.'),
                            parsers::complete::digit1
                        )
                    )
                )
            )
        )(input);
    }
};

struct parse_float {
    auto operator()(const std::string_view& input) const {
        using parsers = Parser;

        return parsers::map(
            recognize_float{},
            [](const auto& number) {
                return std::stof(std::string(number));
            }
        )(input);
    }
};


    template <typename Output>
    using parser_fn = std::function<mff::parser_combinator::ParserResult<std::string_view, Output>(const std::string_view&)>;

    using ignore_parser_fn = parser_fn<mff::parser_combinator::parsers::combinator::Ignore>;
///////////////////////
/// Command parsers ///
///////////////////////

auto parse_coordinates_internal(const std::string_view& input) {
    using parsers = Parser;
    ignore_parser_fn parse_space = parsers::ignore(parsers::complete::take_while1(is_path_space));
    ignore_parser_fn parse_space_optional = parsers::ignore(parsers::complete::take_while(is_path_space));
    ignore_parser_fn parse_comma_symbol = parsers::ignore(parsers::complete::char_p(','));

    // space or comma with some space
    ignore_parser_fn parse_comma_separator = parsers::ignore(parsers::tuple(
        parsers::alt(
            // required space and optional comma
            parsers::ignore(parsers::tuple(parse_space, parsers::opt(parse_comma_symbol))),
            // required comma
            parse_comma_symbol
        ),
        // optional following space
        parse_space_optional
    ));
    ignore_parser_fn parse_comma_separator_optional = parsers::ignore(parsers::opt(parse_comma_separator));

    // factory which indicates that specified parser should be preceded by comma or space
    auto preceded_with_comma = [&](const auto& parser) {
        return parsers::preceded(parse_comma_separator_optional, parser);
    };

    // number parser
    parser_fn<std::float_t> parse_number = parse_float{};

    // two numbers with separators between them
    parser_fn<mff::Vector2f> parse_coordinate = parsers::map(
        parsers::pair(parse_number, preceded_with_comma(parse_number)),
        [](auto i) -> mff::Vector2f { return {i.first, i.second}; }
    );
    parser_fn<std::vector<mff::Vector2f>> parse_coordinate_sequence = parsers::many1(preceded_with_comma(parse_coordinate));

    return parse_coordinate_sequence(input);
}

boost::leaf::result<std::vector<mff::Vector2f>> parse_coordinates(const std::string& input) {
    auto result = parse_coordinates_internal(input);

    if (!result)
        return LEAF_NEW_ERROR();

    return std::move(result->output);
}

auto parse_path_internal(const std::string_view& input) {
    using parsers = Parser;

    // at least one space
    ignore_parser_fn parse_space = parsers::ignore(parsers::complete::take_while1(is_path_space));
    ignore_parser_fn parse_space_optional = parsers::ignore(parsers::complete::take_while(is_path_space));
    ignore_parser_fn parse_comma_symbol = parsers::ignore(parsers::complete::char_p(','));

    // space or comma with some space
    ignore_parser_fn parse_comma_separator = parsers::ignore(parsers::tuple(
        parsers::alt(
            // required space and optional comma
            parsers::ignore(parsers::tuple(parse_space, parsers::opt(parse_comma_symbol))),
            // required comma
            parse_comma_symbol
        ),
        // optional following space
        parse_space_optional
    ));
    ignore_parser_fn parse_comma_separator_optional = parsers::ignore(parsers::opt(parse_comma_separator));

    // factory which indicates that specified parser should be preceded by comma or space
    auto preceded_with_comma = [&](const auto& parser) {
        return parsers::preceded(parse_comma_separator_optional, parser);
    };

    // number parser
    parser_fn<std::float_t> parse_number = parse_float{};
    parser_fn<std::vector<std::float_t>> parse_number_sequence = parsers::many1(preceded_with_comma(parse_number));

    // two numbers with separators between them
    parser_fn<mff::Vector2f> parse_coordinate = parsers::map(
        parsers::pair(parse_number, preceded_with_comma(parse_number)),
        [](auto i) -> mff::Vector2f { return {i.first, i.second}; }
    );
    parser_fn<std::vector<mff::Vector2f>> parse_coordinate_sequence = parsers::many1(preceded_with_comma(parse_coordinate));

    // two coordinates with separators between them
    parser_fn<std::tuple<mff::Vector2f, mff::Vector2f>> parse_coordinate_double = parsers::tuple(
        parse_coordinate,
        preceded_with_comma(parse_coordinate));
    parser_fn<std::vector<std::tuple<mff::Vector2f, mff::Vector2f>>> parse_coordinate_double_sequence = parsers::many1(preceded_with_comma(parse_coordinate_double));

    // three coordinates with spaces between them
    parser_fn<std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f>> parse_coordinate_triplet = parsers::tuple(
        parse_coordinate,
        preceded_with_comma(parse_coordinate),
        preceded_with_comma(parse_coordinate)
    );
    parser_fn<std::vector<std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f>>> parse_coordinate_triplet_sequence = parsers::many1(preceded_with_comma(parse_coordinate_triplet));

    // Create parser which will parse the specified symbol - if it is upper case we should use
    // absolute positioning and if it is lower case we should use relative positioning
    auto command_position_parser = [&](char symbol) -> parser_fn<Position> {
        return parsers::alt(
            parsers::value(Position::Absolute, parsers::complete::char_p(std::toupper(symbol))),
            parsers::value(Position::Relative, parsers::complete::char_p(std::tolower(symbol)))
        );
    };

    // Create parser which will start with symbol (which indicates positioning) followed by
    auto command_with_sequence_parser = [&](char symbol, const auto& sequence_parser) {
        return parsers::map(
            parsers::tuple(
                command_position_parser(symbol),
                parse_comma_separator_optional,
                sequence_parser
            ),
            [](const auto& i) {
                return std::make_tuple(std::get<0>(i), std::get<2>(i));
            }
        );
    };

    // Parse moveto command "M" followed by coordinates
    parser_fn<Command> parse_moveto = parsers::map(
        command_with_sequence_parser('M', parse_coordinate_sequence),
        [](const auto& i) -> Command {
            return Commands_::Moveto{std::get<0>(i), std::get<1>(i)};
        }
    );

    // Parse closepath command "Z"
    parser_fn<Command> parse_closepath = parsers::value(Command{Commands_::Closepath{}}, command_position_parser('Z'));

    // Parse lineto command "L" followed by coordinates
    parser_fn<Command> parse_lineto = parsers::map(
        command_with_sequence_parser('L', parse_coordinate_sequence),
        [](const auto& i) -> Command {
            return Commands_::Lineto{std::get<0>(i), std::get<1>(i)};
        }
    );

    // Parse horizontal lineto command "H" followed by numbers
    parser_fn<Command> parse_horizontal_lineto = parsers::map(
        command_with_sequence_parser('H', parse_number_sequence),
        [](const auto& i) -> Command {
            return Commands_::HorizontalLineto{std::get<0>(i), std::get<1>(i)};
        }
    );

    // Parse vertical lineto command "v" followed by numbers
    parser_fn<Command> parse_vertical_lineto = parsers::map(
        command_with_sequence_parser('V', parse_number_sequence),
        [](const auto& i) -> Command {
            return Commands_::VerticalLineto{std::get<0>(i), std::get<1>(i)};
        }
    );

    // Parse curveto command "C" followed by coordinates triplets (C1 C2 P)
    parser_fn<Command> parse_curveto = parsers::map(
        command_with_sequence_parser('C', parse_coordinate_triplet_sequence),
        [](const auto& i) -> Command {
            return Commands_::Curveto{std::get<0>(i), std::get<1>(i)};
        }
    );

    // Parse smooth curveto command "C" followed by coordinates doubles (C1 P)
    parser_fn<Command> parse_smooth_curveto = parsers::map(
        command_with_sequence_parser('S', parse_coordinate_double_sequence),
        [](const auto& i) -> Command {
            return Commands_::SmoothCurveto{std::get<0>(i), std::get<1>(i)};
        }
    );

    // Commands parser
    parser_fn<Command> parse_command = parsers::alt(
        parse_lineto,
        parse_closepath,
        parse_horizontal_lineto,
        parse_vertical_lineto,
        parse_curveto,
        parse_smooth_curveto,
        parse_moveto
    );

    parser_fn<std::tuple<Command, std::vector<Command>>> parse_first_then_rest = parsers::tuple(
        parsers::preceded(parse_space_optional, parse_moveto),
        parsers::many0(parsers::preceded(parse_space_optional, parse_command))
    );

    return parsers::map(
        parse_first_then_rest,
        [](auto i) {
            auto[first, rest] = std::move(i);

            rest.insert(rest.begin(), first);

            return rest;
        }
    )(input);
}

//template <typename Input, typename Error=mff::parser_combinator::error::DefaultError<Input>>
boost::leaf::result<std::vector<Command>> parse_path(const std::string& input) {
    auto result = parse_path_internal(input);

    if (!result)
        return LEAF_NEW_ERROR();

    return std::move(result->output);
}

}