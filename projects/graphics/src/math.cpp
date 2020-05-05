#include <mff/graphics/math.h>

namespace mff {


std::array<float, 4> to_array(Vector4f v) {
    return {v[0], v[1], v[2], v[3]};
}

std::array<std::uint32_t, 2> to_array(Vector2ui v) {
    return {v[0], v[1]};
}

}