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

/////////////////////////
/// Parse coordinates ///
/////////////////////////



///////////////////////
/// Command parsers ///
///////////////////////

    template <typename Output>
    using parser_fn = std::function<mff::parser_combinator::ParserResult<std::string_view, Output>(const std::string_view&)>;

    using ignore_parser_fn = parser_fn<mff::parser_combinator::parsers::combinator::Ignore>;

    ignore_parser_fn parse_space() {
        using parsers = Parser;

        return parsers::ignore(parsers::complete::take_while1(is_path_space));
    }


    ignore_parser_fn parse_space_optional() {
        using parsers = Parser;

        return parsers::ignore(parsers::complete::take_while1(is_path_space));
    }

    ignore_parser_fn parse_comma_separator() {
        using parsers = Parser;
        auto parse_comma_symbol = parsers::ignore(parsers::complete::char_p(','));

        return parsers::ignore(parsers::tuple(
                parsers::alt(
                        // required space and optional comma
                        parsers::ignore(parsers::tuple(parse_space(), parsers::opt(parse_comma_symbol))),
                        // required comma
                        parse_comma_symbol
                ),
                // optional following space
                parse_space_optional()
        ));
    }

    ignore_parser_fn parse_comma_separator_optional() {
        using parsers = Parser;

        return parsers::ignore(parsers::opt(parse_comma_separator()));
    }

    template <typename Output>
    parser_fn<Output> preceded_with_comma(const parser_fn<Output>& parser) {
        using parsers = Parser;

        return parsers::preceded(parse_comma_separator_optional(), parser);
    }

    parser_fn<mff::Vector2f> parse_coordinate() {
        using parsers = Parser;

        return parsers::map(
                parsers::pair(parse_float{}, preceded_with_comma<std::float_t>(parse_float{})),
                [](auto i) -> mff::Vector2f { return { i.first, i.second }; }
        );
    }

    parser_fn<std::vector<mff::Vector2f>> parse_coordinate_sequence() {
        using parsers = Parser;

        return parsers::many1(preceded_with_comma(parse_coordinate()));
    }

    boost::leaf::result<std::vector<mff::Vector2f>> parse_coordinates(const std::string& input) {
        auto result = parse_coordinate_sequence()(input);

        if (!result)
            return LEAF_NEW_ERROR();

        return std::move(result->output);
    }

    parser_fn<std::vector<std::float_t>> parse_number_sequence() {
        using parsers = Parser;

        //return parsers::many1(parse_coordinate())(input);
        return parsers::many1(preceded_with_comma<std::float_t>(parse_float{}));
    }

    parser_fn<std::tuple<mff::Vector2f, mff::Vector2f>> parse_coordinate_double() {
        using parsers = Parser;

        return parsers::tuple(
                parse_coordinate(),
                preceded_with_comma<mff::Vector2f>(parse_coordinate())
        );
    }

    parser_fn<std::vector<std::tuple<mff::Vector2f, mff::Vector2f>>> parse_coordinate_double_sequence() {
        using parsers = Parser;

        return parsers::many1(preceded_with_comma<std::tuple<mff::Vector2f, mff::Vector2f>>(parse_coordinate_double()));
    }


    parser_fn<std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f>> parse_coordinate_triplet() {
        using parsers = Parser;

        return parsers::tuple(
                parse_coordinate(),
                preceded_with_comma<mff::Vector2f>(parse_coordinate()),
                preceded_with_comma<mff::Vector2f>(parse_coordinate())
        );
    }

    parser_fn<std::vector<std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f>>> parse_coordinate_triplet_sequence() {
        using parsers = Parser;

        return parsers::many1(preceded_with_comma<std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f>>(parse_coordinate_triplet()));
    }

    auto parse_path_internal(const std::string_view& input) {
        using parsers = Parser;

        // Create parser which will parse the specified symbol - if it is upper case we should use
        // absolute positioning and if it is lower case we should use relative positioning
        auto command_position_parser = [&](char symbol) {
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
        auto parse_moveto = parsers::map(
                command_with_sequence_parser('M', parse_coordinate_sequence),
                [](const auto& i) -> Command {
                    return Commands_::Moveto{std::get<0>(i), std::get<1>(i)};
                }
        );

        // Parse closepath command "Z"
        auto parse_closepath = parsers::value(Command{Commands_::Closepath{}}, command_position_parser('Z'));

        // Parse lineto command "L" followed by coordinates
        auto parse_lineto = parsers::map(
                command_with_sequence_parser('L', parse_coordinate_sequence),
                [](const auto& i) -> Command {
                    return Commands_::Lineto{std::get<0>(i), std::get<1>(i)};
                }
        );

        // Parse horizontal lineto command "H" followed by numbers
        auto parse_horizontal_lineto = parsers::map(
                command_with_sequence_parser('H', parse_number_sequence),
                [](const auto& i) -> Command {
                    return Commands_::HorizontalLineto{std::get<0>(i), std::get<1>(i)};
                }
        );

        // Parse vertical lineto command "v" followed by numbers
        auto parse_vertical_lineto = parsers::map(
                command_with_sequence_parser('V', parse_number_sequence),
                [](const auto& i) -> Command {
                    return Commands_::VerticalLineto{std::get<0>(i), std::get<1>(i)};
                }
        );

        // Parse curveto command "C" followed by coordinates triplets (C1 C2 P)
        auto parse_curveto = parsers::map(
                command_with_sequence_parser('C', parse_coordinate_triplet_sequence),
                [](const auto& i) -> Command {
                    return Commands_::Curveto{std::get<0>(i), std::get<1>(i)};
                }
        );

        // Parse smooth curveto command "C" followed by coordinates doubles (C1 P)
        auto parse_smooth_curveto = parsers::map(
                command_with_sequence_parser('S', parse_coordinate_double_sequence),
                [](const auto& i) -> Command {
                    return Commands_::SmoothCurveto{std::get<0>(i), std::get<1>(i)};
                }
        );

        // Commands parser
        auto parse_command = parsers::alt(
                parse_lineto,
                parse_closepath,
                parse_horizontal_lineto,
                parse_vertical_lineto,
                parse_curveto,
                parse_smooth_curveto,
                parse_moveto
        );

        auto parse_first_then_rest = parsers::tuple(
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
        )(input);*/
    }

//template <typename Input, typename Error=mff::parser_combinator::error::DefaultError<Input>>
    boost::leaf::result<std::vector<Command>> parse_path(const std::string& input) {
        auto result = parse_path_internal<std::string_view>(input);

        if (!result)
            return LEAF_NEW_ERROR();

        return std::move(result->output);
    }

}