#pragma once

#include <unordered_map>
#include <variant>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/math.h>
#include <mff/parser_combinator/parsers.h>

#include "../path.h"

namespace canvas::svg {

namespace XmlContent_ {

struct EmptyElementTag {
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
};

struct StartTag {
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
};
struct EndTag {
    std::string name;
};
struct CharData {
};

}

using XmlContent = std::variant<
    XmlContent_::EmptyElementTag,
    XmlContent_::StartTag,
    XmlContent_::EndTag,
    XmlContent_::CharData
>;

struct DrawState {
    mff::Vector4f fill_color = {};
    mff::Vector4f stroke_color = {};
    std::float_t stroke_width = {};
    bool stroke = false;
    bool fill = false;
};

std::vector<std::tuple<Path2D, DrawState>> to_paths(const std::string& data);

}