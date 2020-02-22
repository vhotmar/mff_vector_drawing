#include <string>
#include <string_view>
#include <sstream>

#include <catch2/catch.hpp>

#include <mff/parser_combinator/parsers.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
namespace parsers = mff::parser_combinator::parsers;
namespace error = mff::parser_combinator::error;

bool is_xml_space(char c) {
    return c == 0x20 || c == 0x9 || c == 0xA || c == 0xD;
}

// S  ::=  (#x20 | #x9 | #xD | #xA)+
template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, Input, Error> parse_space(const Input& input) {
    return parsers::complete::take_while1<Input, Error>(
        is_xml_space
    )(input);
}

template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, Input, Error> parse_space_optional(const Input& input) {
    return parsers::complete::take_while<Input, Error>(
        is_xml_space
    )(input);
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

// Name ::=  (Letter | '_' | ':') (NameChar)*
template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, Input, Error> parse_name(const Input& input) {
    auto first_char_result = TRY(parsers::complete::take<Input, Error>(1)(input));
    auto first_char = first_char_result.output[0];

    if (!(is_xml_letter(first_char) || first_char == '_' || first_char == ':')) {
        return mff::parser_combinator::make_parser_result_error<Input, Input, Error>(input, error::ErrorKind::User);
    }

    auto name_char_parser = parsers::complete::take_while<Input, Error>([](auto c) { return is_name_char(c); });

    return name_char_parser(input);
}

template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, char, Error> parse_eq(const Input& input) {
    return parsers::between<Input, Error>(
        parse_space_optional<Input, Error>,
        parsers::complete::char_p<Input, Error>('=')
    )(input);
}


template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, Input, Error> parse_attribute_value(const Input& input) {
    return parsers::alt<Input, Error>(
        parsers::between<Input, Error>(
            parsers::complete::char_p<Input, Error>('\''),
            parsers::complete::take_while<Input, Error>(
                [](auto c) { return c != '\''; }
            )
        ),
        parsers::between<Input, Error>(
            parsers::complete::char_p<Input, Error>('"'),
            parsers::complete::take_while<Input, Error>(
                [](auto c) { return c != '"'; }
            )
        )
    )(input);
}

template <class... Args>
struct variant_cast_proxy {
    std::variant<Args...> v;

    template <class... ToArgs>
    operator std::variant<ToArgs...>() const {
        return std::visit(
            [](auto&& arg) -> std::variant<ToArgs...> { return arg; },
            v
        );
    }
};

template <class... Args>
auto variant_cast(const std::variant<Args...>& v) -> variant_cast_proxy<Args...> {
    return {v};
}

template <typename T>
struct xml_attribute {
    T name;
    T value;

    bool operator==(const xml_attribute<T>& rhs) const {
        return name == rhs.name && value == rhs.value;
    }

    bool operator!=(const xml_attribute<T>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename T>
struct xml_empty_element {
    T name;
    std::vector<xml_attribute<T>> attributes;

    bool operator==(const xml_empty_element<T>& rhs) const {
        return name == rhs.name && attributes == rhs.attributes;
    }

    bool operator!=(const xml_empty_element<T>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename T>
struct xml_tag_content {
    T name;
    std::vector<xml_attribute<T>> attributes;

    bool operator==(const xml_tag_content<T>& rhs) const {
        return name == rhs.name && attributes == rhs.attributes;
    }

    bool operator!=(const xml_tag_content<T>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename T>
struct xml_start_tag {
    T name;
    std::vector<xml_attribute<T>> attributes;

    bool operator==(const xml_start_tag<T>& rhs) const {
        return name == rhs.name && attributes == rhs.attributes;
    }

    bool operator!=(const xml_start_tag<T>& rhs) const {
        return !operator==(rhs);
    }
};


template <typename T>
struct xml_char_data {
    T input;

    bool operator==(const xml_char_data<T>& rhs) const {
        return input == rhs.input;
    }

    bool operator!=(const xml_char_data<T>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename Input>
struct xml_element;

template <typename Input>
using xml_element_type = std::variant<xml_empty_element<Input>, xml_element<Input>>;

template <typename Input>
using xml_content_type = std::variant<xml_empty_element<Input>, xml_element<Input>, xml_char_data<Input>>;

template <typename T>
struct xml_element {
    T name;
    std::vector<xml_attribute<T>> attributes;
    std::vector<xml_content_type<T>> content;

    bool operator==(const xml_element<T>& rhs) const {
        return name == rhs.name && attributes == rhs.attributes && content == rhs.content;
    }

    bool operator!=(const xml_element<T>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, xml_attribute<Input>, Error> parse_attribute(const Input& input) {
    auto parser = parsers::combinator::map<Input, Error>(
        parsers::separated_pair<Input, Error>(
            parse_name<Input, Error>,
            parse_eq<Input, Error>,
            parse_attribute_value<Input, Error>
        ),
        [](auto p) { return xml_attribute<Input>{p.first, p.second}; }
    );

    return parser(input);
}

template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, std::vector<xml_attribute<Input>>, Error> parse_attributes(
    const Input& input
) {
    auto parser = parsers::many0<Input, Error>(
        parsers::preceded<Input, Error>(
            parsers::combinator::opt<Input, Error>(parse_space<Input, Error>),
            parse_attribute<Input, Error>
        )
    );

    return parser(input);
}

// TagContent  ::=  Name (S Attribute)* S?
template <typename Input, typename Error=error::default_error<Input>>
auto parse_tag_content(const Input& input) {
    return parsers::combinator::map<Input, Error>(
        parsers::terminated<Input, Error>(
            parsers::pair<Input, Error>(
                parse_name<Input, Error>,
                parse_attributes<Input, Error>
            ),
            parse_space_optional<Input, Error>
        ),
        [](auto p) { return xml_tag_content<Input>{p.first, p.second}; }
    )(input);
}

// EmptyElemTag  ::=  '<' TagContent '/>'
template <typename Input, typename Error=error::default_error<Input>>
auto parse_empty_element_tag(const Input& input) {
    return parsers::combinator::map<Input, Error>(
        parsers::delimited<Input, Error>(
            parsers::complete::tag<Input, Error>("<"),
            parse_tag_content<Input, Error>,
            parsers::complete::tag<Input, Error>("/>")
        ),
        [](auto p) { return xml_empty_element<Input>{p.name, p.attributes}; }
    )(input);
}

// STag  ::=  '<' TagContent '>'
template <typename Input, typename Error=error::default_error<Input>>
auto parse_start_element_tag(const Input& input) {
    return parsers::combinator::map<Input, Error>(
        parsers::delimited<Input, Error>(
            parsers::complete::tag<Input, Error>("<"),
            parse_tag_content<Input, Error>,
            parsers::complete::tag<Input, Error>(">")
        ),
        [](auto p) { return xml_start_tag<Input>{p.name, p.attributes}; }
    )(input);
}

template <typename Input, typename Error=error::default_error<Input>>
auto parse_end_element_tag(const Input& name, const Input& input) {
    return parsers::combinator::value<Input, Error>(
        name,
        parsers::tuple<Input, Error>(
            parsers::complete::tag<Input, Error>("</"),
            parsers::complete::tag<Input, Error>(name),
            parse_space_optional<Input, Error>,
            parsers::complete::tag<Input, Error>(">")
        )
    )(input);
}

template <typename Input, typename Error=error::default_error<Input>>
auto parse_char_data(const Input& input) {
    return parsers::combinator::map<Input, Error>(
        parsers::complete::take_while1<Input, Error>([](auto c) { return c != '<' && c != '>'; }),
        [](auto c) { return xml_char_data<Input>{c}; }
    )(input);
}

template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, xml_element<Input>, Error> parse_whole_element(const Input& input);

template <typename Input, typename Error=error::default_error<Input>>
auto parse_element(const Input& input);

template <typename Input, typename Error=error::default_error<Input>>
mff::parser_combinator::parser_result<Input, std::vector<xml_content_type<Input>>, Error>
parse_contents(const Input& input) {
    auto parse_optional_char_data = parsers::combinator::opt<Input, Error>(parse_char_data<Input, Error>);

    std::vector<xml_content_type<Input>> content;

    auto char_data_result = TRY(parse_optional_char_data(input));

    if (char_data_result.output != std::nullopt) {
        content.push_back(*char_data_result.output);
    }

    auto i = char_data_result.next_input;

    while (true) {
        auto element = parse_element<Input, Error>(i);

        if (!element) {
            break;
        }

        content.push_back(variant_cast(element->output));

        auto chars = TRY(parse_optional_char_data(element->next_input));

        if (chars.output != std::nullopt) {
            content.push_back(*chars.output);
        }

        i = chars.next_input;
    }

    return mff::parser_combinator::make_parser_result(i, content);
}

template <typename Input, typename Error>
mff::parser_combinator::parser_result<Input, xml_element<Input>, Error> parse_whole_element(const Input& input) {
    auto start_result = TRY(parse_start_element_tag(input));
    auto content_result = TRY(parse_contents(start_result.next_input));
    auto end_result = TRY(parse_end_element_tag(start_result.output.name, content_result.next_input));

    return mff::parser_combinator::make_parser_result(
        end_result.next_input,
        xml_element<Input>{start_result.output.name, start_result.output.attributes, content_result.output}
    );
}

template <typename Input, typename Error>
auto parse_element(const Input& input) {
    return parsers::alt<Input, Error>(
        parsers::combinator::map<Input, Error>(
            parse_empty_element_tag<Input, Error>,
            [](auto p) -> xml_element_type<Input> { return p; }
        ),
        parsers::combinator::map<Input, Error>(
            parse_whole_element<Input, Error>,
            [](auto p) -> xml_element_type<Input> { return p; }
        )
    )(input);
}

SCENARIO("we can build simple XML parser") {
    GIVEN("parse_name parser") {
        WHEN("we try to parse \"asdf\"") {
            auto result = parse_name("asdf"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "asdf"s));
            }
        }

        WHEN("we try to parse \"1asdf\"") {
            auto result = parse_name("1asdf"s);

            THEN("it should fail") {
                REQUIRE(result == mff::parser_combinator::make_parser_result_error<std::string, std::string>(
                    "1asdf"s,
                    error::ErrorKind::User
                ));
            }
        }
    }

    GIVEN("parse_space parser") {
        WHEN("we try to parse \"    \"") {
            auto result = parse_space("    "s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "    "s));
            }
        }

        WHEN("we try to parse \" asdf\"") {
            auto result = parse_space(" asdf"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result("asdf"s, " "s));
            }
        }
    }

    GIVEN("parse_eq parser") {
        WHEN("we try to parse \"  =  \"") {
            auto result = parse_eq("  =  "s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, '='));
            }
        }
    }

    GIVEN("parse_attribute_value parser") {
        WHEN("we try to parse \"\\\"noice\\\"\"") {
            auto result = parse_attribute_value("\"noice\""s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "noice"s));
            }
        }

        WHEN("we try to parse \"'noice'\"") {
            auto result = parse_attribute_value("'noice'"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(""s, "noice"s));
            }
        }
    }

    GIVEN("parse_attribute parser") {
        WHEN("we try to parse 'attr  = \"noice\"") {
            auto result = parse_attribute(R"(attr  = "noice")"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    xml_attribute<std::string>{"attr"s, "noice"s}
                ));
            }
        }
    }

    GIVEN("parse_attributes parser") {
        WHEN("we try to parse 'attr1  = \"noice\" attr2  ='nice'") {
            auto result = parse_attributes(R"(attr1  = "noice" attr2  ='nice')"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    std::vector<xml_attribute<std::string>>{{"attr1", "noice"}, {"attr2", "nice"}}
                ));
            }
        }
    }

    GIVEN("parse_empty_element_tag parser") {
        WHEN("we try to parse \"<elem attr1='content' />\"") {
            auto result = parse_empty_element_tag("<elem attr1='content' />"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    xml_empty_element<std::string>{"elem", {{"attr1", "content"}}}
                ));
            }
        }

        WHEN(R"(we try to parse "<elem attr1='content'  attr2  = "another"    />")") {
            auto result = parse_empty_element_tag(R"(<elem attr1='content'  attr2  = "another"    />)"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    xml_empty_element<std::string>{"elem", {{"attr1", "content"}, {"attr2", "another"}}}
                ));
            }
        }
    }

    GIVEN("parse_start_element_tag parser") {
        WHEN(R"(we try to parse "<elem attr1='content'  attr2  = "another"    >")") {
            auto result = parse_start_element_tag(R"(<elem attr1='content'  attr2  = "another"    >)"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    xml_start_tag<std::string>{"elem", {{"attr1", "content"}, {"attr2", "another"}}}
                ));
            }
        }
    }

    GIVEN("parse_whole_element parser") {
        WHEN(R"(we try to parse "<elem attr1='content'  >NICE</elem>")") {
            auto result = parse_whole_element(R"(<elem attr1='content'  >NICE</elem>)"s);

            THEN("it should succeed") {
                REQUIRE(result == mff::parser_combinator::make_parser_result(
                    ""s,
                    xml_element<std::string>{"elem", {{"attr1", "content"}}, {xml_char_data<std::string>{"NICE"}}}
                ));
            }
        }

        WHEN(R"(we try to parse simple example)") {
            auto result = parse_whole_element(
                R"(<elem attr1='content'  >
  <another-sample />
  <nested attr=  "hooray"  >COMPLEX</nested  >
  NICE
</elem>)"sv
            );

            THEN("it should succeed") {
                REQUIRE(result.has_value());
            }
        }
    }
}