#pragma once

#include <array>

#include <Eigen/Dense>

namespace mff {

typedef Eigen::Vector2i Vector2i;
typedef Eigen::Vector2f Vector2f;
typedef Eigen::Matrix<std::uint32_t, 2, 1> Vector2ui;

typedef Eigen::Vector4f Vector4f;
typedef Eigen::Matrix2f Matrix2f;

std::array<std::uint32_t, 2> to_array(Vector2ui v);
std::array<float, 4> to_array(Vector4f v);

}
