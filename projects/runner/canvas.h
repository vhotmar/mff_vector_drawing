#include <iostream>
#include <vector>
#include <variant>

#include <range/v3/all.hpp>
#include <mff/graphics/math.h>
#include "./renderer/renderer.h"
#include "./third_party/earcut.hpp"

namespace canvas {

struct Coloru {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
};

struct Colorf {
    std::float_t r = 0.0f;
    std::float_t g = 0.0f;
    std::float_t b = 0.0f;
    std::float_t a = 0.0f;
};

struct State {
    // transform?
    std::float_t line_width;
    std::float_t miter_limit;
    //LineCap::type line_cap;
    //LineJoin::type line_join;
    //Coloru stroke_color;
    Coloru fill_color;
};

}
