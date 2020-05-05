#pragma once

#include <variant>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/math.h>

namespace canvas::svg {

enum class Position { Absolute, Relative };

namespace Commands_ {

struct Moveto {
    Position position;
    std::vector<mff::Vector2f> coordinates;
};

struct Closepath {};

struct Lineto {
    Position position;
    std::vector<mff::Vector2f> coordinates;
};

struct HorizontalLineto {
    Position position;
    std::vector<std::float_t> coordinates;
};

struct VerticalLineto {
    Position position;
    std::vector<std::float_t> coordinates;
};

struct Curveto {
    Position position;
    std::vector<std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f>> coordinates;
};

struct SmoothCurveto {
    Position position;
    std::vector<std::tuple<mff::Vector2f, mff::Vector2f>> coordinates;
};

}

using Command = std::variant<
    Commands_::Moveto,
    Commands_::Closepath,
    Commands_::Lineto,
    Commands_::VerticalLineto,
    Commands_::HorizontalLineto,
    Commands_::Curveto,
    Commands_::SmoothCurveto
>;

boost::leaf::result<std::vector<Command>> parse_path(const std::string& input);

}