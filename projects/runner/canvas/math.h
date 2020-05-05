#pragma once

#include <mff/graphics/utils.h>

namespace canvas {

struct LineSegment2f {
    mff::Vector2f from;
    mff::Vector2f to;

    mff::Vector2f vector() const;

    static LineSegment2f zero();
};

struct Transform2f {
    mff::Matrix2f transform;
    mff::Vector2f transpose;

    Transform2f();
    Transform2f(mff::Matrix2f transform, mff::Vector2f transpose);

    static Transform2f from_transpose(mff::Vector2f transpose);
    static Transform2f from_scale(mff::Vector2f scale);
    static Transform2f identity();

    Transform2f inverse() const;
    mff::Vector2f apply(const mff::Vector2f& v) const;
    Transform2f apply(const Transform2f& t) const;
    LineSegment2f apply(const LineSegment2f& l) const;

    Transform2f operator*(const Transform2f& rhs) const {
        return apply(rhs);
    }
};

}