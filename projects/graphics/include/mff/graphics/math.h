#pragma once

#include <array>

#include <Eigen/Dense>

namespace mff {

typedef Eigen::Vector2i Vector2i;
typedef Eigen::Vector2f Vector2f;
typedef Eigen::Matrix<std::uint32_t, 2, 1> Vector2ui;

typedef Eigen::Vector3f Vector3f;

typedef Eigen::Vector4f Vector4f;
typedef Eigen::Matrix2f Matrix2f;

std::array<std::uint32_t, 2> to_array(Vector2ui v);
std::array<float, 4> to_array(Vector4f v);

Matrix2f from_array(std::array<std::float_t, 4> arr);

Vector2f lerp(const Vector2f& a, const Vector2f& b, std::float_t t);

const std::float_t kEPSILON = 0.001;

bool is_approx_zero(const mff::Vector2f& a);
bool are_approx_same(const mff::Vector2f& a, const mff::Vector2f& b);

}
