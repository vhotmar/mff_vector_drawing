#pragma once

#include <unordered_map>
#include <variant>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/math.h>
#include <mff/parser_combinator/parsers.h>

#include "../path.h"
#include "../stroke.h"

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

enum class DrawStatePaintFirst { Stroke, Fill };

struct DrawState {
    mff::Vector4f fill_color = {0.0f, 0.0f, 0.0f, 1.0f};
    mff::Vector4f stroke_color = {0.0f, 0.0f, 0.0f, 1.0f};
    std::float_t stroke_width = {};
    LineJoin line_join = LineJoin_::Bevel{};
    LineCap line_cap = LineCap_::Butt{};
    std::float_t stroke_miterlimit = 15.0f;
    bool stroke = false;
    bool fill = false;
    bool hide = false;
    DrawStatePaintFirst paint_first = DrawStatePaintFirst::Fill;
};

/**
 * Parse SVG string to paths
 * @param data
 * @return
 */
std::vector<std::tuple<Path2D, DrawState>> to_paths(const std::string& data);

}