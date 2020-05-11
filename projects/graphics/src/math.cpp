#include <mff/graphics/math.h>

namespace mff {

Vector2f lerp(const Vector2f& a, const Vector2f& b, std::float_t t) {
    return a + (b - a) * t;
}

std::array<float, 4> to_array(Vector4f v) {
    return {v[0], v[1], v[2], v[3]};
}

std::array<std::uint32_t, 2> to_array(Vector2ui v) {
    return {v[0], v[1]};
}

Matrix2f from_array(std::array<std::float_t, 4> arr) {
    Matrix2f m;

    m << arr[0], arr[1], arr[2], arr[3];

    return m;
}


bool is_approx_zero(const mff::Vector2f& a) {
    return a.squaredNorm() < (kEPSILON * kEPSILON);
}

bool are_approx_same(const mff::Vector2f& a, const mff::Vector2f& b) {
    return is_approx_zero(b - a);
}

}