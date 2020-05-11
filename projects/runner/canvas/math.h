#pragma once

#include <mff/graphics/utils.h>

namespace canvas {

struct LineSegment2f {
    mff::Vector2f from;
    mff::Vector2f to;

    /**
     * Get the intersection of this line and some other line
     * @param other the other line
     * @return the time at which they intersects (may be outside of the 0..1 range)
     */
    std::optional<std::float_t> intersection(const LineSegment2f& other) const;

    /**
     * Get the angle between this line and some other line
     * @param other the other line
     * @return angle between them
     */
    std::float_t inner_angle_between(const LineSegment2f& other) const;

    /**
     * Get the angle specified by (B - A)
     * @return
     */
    mff::Vector2f vector() const;

    /**
     * Move the line in its normal direction by distance
     * @param dist distance to move the line by
     * @returns the moved line
     */
    LineSegment2f offset(std::float_t dist) const;

    /**
     * Move the line by specified translation (vector addition)
     * @param translation
     * @return moved line
     */
    LineSegment2f operator+(const mff::Vector2f& translation) const;

    /**
     * Return the line segment reversed (exchange from, to points)
     * @return reversed line segment
     */
    LineSegment2f reversed() const;

    /**
     * Get the position at line  at time
     * @param t the time
     * @return
     */
    mff::Vector2f evaluate(std::float_t t) const;

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

    // Chain transformations
    Transform2f operator*(const Transform2f& rhs) const;
};

}