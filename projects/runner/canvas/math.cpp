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

mff::Vector2f LineSegment2f::vector() const {
    return to - from;
}

LineSegment2f LineSegment2f::zero() {
    return LineSegment2f{mff::Vector2f::Zero(), mff::Vector2f::Zero()};
}

}