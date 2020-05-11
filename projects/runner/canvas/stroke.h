#pragma once

#include <variant>

#include "./contour.h"

namespace canvas {

namespace LineCap_ {

struct Butt {};
struct Square {};
struct Round {};

}

using LineCap = std::variant<LineCap_::Butt, LineCap_::Square, LineCap_::Round>;

namespace LineJoin_ {

struct Miter { float value; };
struct Bevel {};
struct Round {};

}

using LineJoin = std::variant<LineJoin_::Miter, LineJoin_::Bevel, LineJoin_::Round>;

struct StrokeStyle {
    std::float_t line_width = 1.0f;
    LineCap line_cap = LineCap_::Butt{};
    LineJoin line_join = LineJoin_::Bevel{};
};

std::vector<Contour> stroke(const Contour& to_stroke, const StrokeStyle& style);

struct StrokeResult {
    std::vector<mff::Vector2f> vertices;
    std::vector<std::uint32_t> indices;
};

StrokeResult get_stroke(const std::vector<mff::Vector2f>& flattened, const StrokeStyle& style, bool loop = false);

}