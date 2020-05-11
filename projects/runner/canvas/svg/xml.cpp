#include "./xml.h"

#include <string_view>
#include <stack>

#include <range/v3/all.hpp>
#include <mff/algorithms.h>
#include <mff/utils.h>
#include <mff/parser_combinator/parsers.h>

#include "./path.h"

namespace canvas::svg {

template <typename Input, typename Error = mff::parser_combinator::error::DefaultError<Input>>
using Parser = mff::parser_combinator::parsers::Parsers<Input>;

bool is_xml_space(char c) {
    return c == 0x20 || c == 0x9 || c == 0xA || c == 0xD;
}

// Char ::=  #x9 | #xA | #xD | [#x20-#xD7FF]
bool is_xml_char(char c) {
    return c == 0x9 || c == 0xA || c == 0xD || c >= 0x20;
}

// Digit ::=  [#x30-#x39]
bool is_xml_digit(char c) {
    return isdigit(c);
}

// BaseChar ::=  [#x41-#x5A] | [#x61-#x7A]
bool is_xml_base_char(char c) {
    return (c >= 0x41 && c <= 0x51) || (c >= 0x61 && c <= 0x7A);
}

// Letter ::=  BaseChar
bool is_xml_letter(char c) {
    return is_xml_base_char(c);
}

// NameChar ::=  Letter | Digit
//           |  '.' | '-' | '_' | ':'
bool is_name_char(char c) {
    return is_xml_letter(c) || is_xml_digit(c) || c == '.' || c == '-' || c == '_' || c == ':';
}

template <typename Input, typename Error = mff::parser_combinator::error::DefaultError<Input>>
auto parse_xml_internal(const Input& input) {
    using parsers = Parser<Input, Error>;

    auto parse_space = parsers::complete::take_while1(is_xml_space);
    auto parse_space_optional = parsers::complete::take_while(is_xml_space);

    auto parse_name = parsers::recognize(
        parsers::tuple(
            parsers::verify(
                parsers::complete::take(1),
                [](const auto& chars) {
                    char first_char = chars[0];
                    return is_xml_letter(first_char) || first_char == '_' || first_char == ':';
                }
            ),
            parsers::complete::take_while(is_name_char)
        ));

    auto parse_eq = parsers::between(parse_space_optional, parsers::complete::char_p('='));

    auto is_not_char = [](char c) { return [c](const auto& i) { return c != i; }; };
    auto any_surrounded_by = [&](char c) {
        return parsers::between(
            parsers::complete::char_p(c),
            parsers::complete::take_while(is_not_char(c)));
    };

    auto parse_attribute_value = parsers::alt(any_surrounded_by('\''), any_surrounded_by('"'));
    auto parse_attribute = parsers::separated_pair(parse_name, parse_eq, parse_attribute_value);
    auto parse_attributes = parsers::map(
        parsers::many0(parsers::preceded(parse_space_optional, parse_attribute)),
        [](const auto& attributes) {
            std::unordered_map<std::string, std::string> result;

            for (const auto& attribute: attributes) {
                result[std::string(std::get<0>(attribute))] = std::string(std::get<1>(attribute));
            }

            return result;
        }
    );
    auto parse_tag_content = parsers::terminated(
        parsers::pair(parse_name, parse_attributes),
        parse_space_optional
    );
    auto in_braces = [](Input start_brace, Input end_brace, auto parser) {
        return parsers::delimited(parsers::complete::tag(start_brace), parser, parsers::complete::tag(end_brace));
    };

    auto parse_empty_element_tag = parsers::map(
        in_braces("<", "/>", parse_tag_content),
        [](const auto& contents) -> XmlContent {
            return XmlContent_::EmptyElementTag{std::string(std::get<0>(contents)), std::get<1>(contents)};
        }
    );
    auto parse_start_element_tag = parsers::map(
        in_braces("<", ">", parse_tag_content),
        [](const auto& contents) -> XmlContent {
            return XmlContent_::StartTag{std::string(std::get<0>(contents)), std::get<1>(contents)};
        }
    );
    auto parse_end_element_tag = parsers::map(
        in_braces("</", ">", parsers::terminated(parse_name, parse_space_optional)),
        [](const auto& name) -> XmlContent {
            return XmlContent_::EndTag{std::string(name)};
        }
    );
    auto parse_xml_char_data = parsers::map(
        parsers::complete::take_while1([](auto c) { return c != '<' && c != '>'; }),
        [](const auto& data) -> XmlContent {
            return XmlContent_::CharData{};
        }
    );

    auto parse_some = parsers::alt(
        parse_empty_element_tag,
        parse_start_element_tag,
        parse_end_element_tag,
        parse_xml_char_data
    );

    return parse_some(input);
}

mff::Vector4f parse_color(const std::string& input) {
    using parsers = Parser<std::string>;

    auto is_hex_digit = [](char c) {
        return isxdigit(c);
    };

    auto digits = parsers::verify(
        parsers::complete::take_while(is_hex_digit),
        [](const auto& value) {
            return value.size() == 6 || value.size() == 3;
        }
    );

    auto whole = parsers::preceded(parsers::complete::char_p('#'), digits);

    auto hex_to_num = [](const std::string& str, int from, int n = 2) -> std::float_t {
        return (std::float_t) std::stoi(str.substr(from, n), 0, 16);
    };

    return parsers::map(
        whole,
        [&](const auto& value) {
            if (value.size() == 3) {
                return mff::Vector4f{
                    hex_to_num(value, 0, 1) / 15.0f,
                    hex_to_num(value, 1, 1) / 15.0f,
                    hex_to_num(value, 2, 1) / 15.0f,
                    1.0f};
            }

            return mff::Vector4f{
                hex_to_num(value, 0) / 255.0f,
                hex_to_num(value, 2) / 255.0f,
                hex_to_num(value, 4) / 255.0f,
                1.0f};
        }
    )(input)->output;
}

std::vector<std::tuple<Path2D, DrawState>> to_paths(const std::string& data) {
    std::vector<std::tuple<Path2D, DrawState>> result;

    auto next_input = std::string_view(data);
    auto parsed_result = parse_xml_internal<std::string_view>(next_input);

    std::stack<DrawState> states;
    states.push(DrawState{});

    auto to_lower = [](const std::string& s) -> std::string {
        return s | ranges::views::transform([](auto c) { return std::tolower(c); }) | ranges::to<std::string>();
    };

    auto apply_info = [](DrawState& state, const std::unordered_map<std::string, std::string>& attributes) {
        if (mff::has(attributes, "fill")) {
            if (attributes.at("fill") == "none") {
                state.fill = false;
            } else {
                state.fill_color = parse_color(attributes.at("fill"));
                state.fill = true;
            }
        }

        if (mff::has(attributes, "stroke")) {
            if (attributes.at("stroke") == "none") {
                state.stroke = false;
            } else {
                state.stroke_color = parse_color(attributes.at("stroke"));
                state.stroke = true;
            }
        }

        if (mff::has(attributes, "stroke-width")) {
            state.stroke_width = std::stof(attributes.at("stroke-width"));
        }
    };

    while (parsed_result.has_value() && next_input != "" && next_input != parsed_result->next_input) {
        std::visit(
            mff::overloaded{
                [](const XmlContent_::CharData& empty) {},
                [&](const XmlContent_::EndTag& end) {
                    states.pop();
                },
                [&](const XmlContent_::StartTag& start) {
                    if (to_lower(start.name) == "g") {
                        DrawState new_state = states.top();
                        apply_info(new_state, start.attributes);
                        states.push(new_state);
                    }
                },
                [&](const XmlContent_::EmptyElementTag& empty) {
                    if (to_lower(empty.name) == "path" && mff::has(empty.attributes, "d")) {
                        DrawState curr_state = states.top();
                        apply_info(curr_state, empty.attributes);

                        auto path_string = empty.attributes.at("d");
                        auto path = Path2D::from_svg_commands(parse_path(path_string).value());

                        result.push_back(std::make_tuple(path, curr_state));
                    }
                },
            },
            parsed_result->output
        );

        next_input = parsed_result->next_input;
        parsed_result = parse_xml_internal<std::string_view>(next_input);
    }

    return result;
}

}
