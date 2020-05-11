#include "./math.h"

namespace canvas {

Transform2f::Transform2f()
    : transform(mff::from_array({1, 0, 0, 1})), transpose({0, 0}) {
}

Transform2f::Transform2f(mff::Matrix2f transform, mff::Vector2f transpose)
    : transform(transform), transpose(transpose) {

}

mff::Vector2f Transform2f::apply(const mff::Vector2f& v) const {
    return mff::Vector2f(transform * v + transpose);
}

Transform2f Transform2f::apply(const Transform2f& t) const {
    return Transform2f(
        transform * t.transform,
        apply(t.transpose)
    );
}

LineSegment2f Transform2f::apply(const LineSegment2f& l) const {
    return LineSegment2f{apply(l.from), apply(l.to)};
}

Transform2f Transform2f::identity() {
    return Transform2f(mff::from_array({1, 0, 0, 1}), {0, 0});
}

Transform2f Transform2f::from_scale(mff::Vector2f scale) {
    return Transform2f(mff::from_array({scale[0], 0, 0, scale[1]}), {0, 0});
}

Transform2f Transform2f::from_transpose(mff::Vector2f transpose) {
    return Transform2f(mff::from_array({1, 0, 0, 1}), transpose);
}

Transform2f Transform2f::inverse() const {
    Transform2f result = {};

    result.transform = transform.inverse();
    result.transpose = -(result.transform * transpose);

    return result;
}


Transform2f Transform2f::operator*(const Transform2f& rhs) const {
    return apply(rhs);
}

mff::Vector2f LineSegment2f::vector() const {
    return to - from;
}

LineSegment2f LineSegment2f::zero() {
    return LineSegment2f{mff::Vector2f::Zero(), mff::Vector2f::Zero()};
}

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
// https://stackoverflow.com/questions/385305/efficient-maths-algorithm-to-calculate-intersections
std::optional<std::float_t> LineSegment2f::intersection(const LineSegment2f& other) const {
    mff::Matrix2f m;

    m << vector(), (-other.vector());

    // numerical stability not best
    if (std::abs(m.determinant()) < 0.001f) {
        return std::nullopt;
    }

    return (m.inverse() * (from - other.from))[1];
}

std::float_t LineSegment2f::inner_angle_between(const LineSegment2f& other) const {
    auto v1 = vector();
    auto v2 = other.vector();

    return std::acos(v1.dot(v2) / (v1.norm() * v2.norm()));
}


LineSegment2f LineSegment2f::offset(std::float_t dist) const {
    if (vector().isZero()) return *this;

    auto normalized = vector().normalized() * dist;

    return *this + mff::Vector2f(-normalized[1], normalized[0]);
}

LineSegment2f LineSegment2f::operator+(const mff::Vector2f& translation) const {
    return LineSegment2f{from + translation, to + translation};
}

LineSegment2f LineSegment2f::reversed() const {
    return LineSegment2f{to, from};
}

mff::Vector2f LineSegment2f::evaluate(std::float_t t) const {
    return from + vector() * t;
}

}