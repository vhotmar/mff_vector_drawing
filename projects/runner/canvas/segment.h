#pragma once

#include <mff/graphics/utils.h>

#include "./math.h"

namespace canvas {

enum class SegmentKind {
    Line,
    Quadratic,
    Cubic
};

struct FlattenOptions {
    std::size_t steps = 15;
};

/**
 * Basic unit of one contour, it is either Line, Quadratic or Bezier
 */
struct Segment {
    LineSegment2f baseline;
    LineSegment2f control;
    SegmentKind kind;

    Segment transform(const Transform2f& t);

    std::float_t length() const;
    std::float_t time_for_distance(std::float_t dist) const;
    mff::Vector2f evaluate(std::float_t t) const;
    std::vector<mff::Vector2f> flatten(FlattenOptions options = {});
    Segment to_cubic() const;

    static Segment line(const LineSegment2f& line);
    static Segment quadratic(const LineSegment2f& line, const mff::Vector2f& ctrl);
    static Segment cubic(const LineSegment2f& line, const LineSegment2f& ctrl);
    // https://pomax.github.io/bezierinfo/#circles_cubic
    static Segment arc(std::float_t phi);
};

}