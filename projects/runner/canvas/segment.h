#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <variant>

#include <mff/graphics/utils.h>
#include <mff/utils.h>

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

namespace Kind_ {

struct Line {
    LineSegment2f baseline;
};

struct Quadratic {
    LineSegment2f baseline;
    mff::Vector2f control;

    struct Hull {
        mff::Vector2f p1;
        mff::Vector2f p2;
        mff::Vector2f p3;

        mff::Vector2f p12;
        mff::Vector2f p23;

        mff::Vector2f p123;
    };

    Hull hull(std::float_t t) const;
};

struct Cubic {
    LineSegment2f baseline;
    LineSegment2f control;

    struct Hull {
        mff::Vector2f p1;
        mff::Vector2f p2;
        mff::Vector2f p3;
        mff::Vector2f p4;

        mff::Vector2f p12;
        mff::Vector2f p23;
        mff::Vector2f p34;

        mff::Vector2f p123;
        mff::Vector2f p234;

        mff::Vector2f p1234;
    };

    Hull hull(std::float_t t) const;
};

}

using Kind = std::variant<Kind_::Line, Kind_::Quadratic, Kind_::Cubic>;

struct Segment {
    Kind data;

    LineSegment2f get_baseline() const;
    mff::Vector2f get_last_control() const;

    mff::Vector2f derivative(std::float_t t) const;
    std::float_t length() const;
    std::float_t time_for_distance(std::float_t dist) const;
    Segment transform(const Transform2f& t) const;
    mff::Vector2f normal(std::float_t t) const;
    mff::Vector2f evaluate(std::float_t t) const;
    std::pair<Segment, Segment> split(std::float_t t) const;
    std::vector<mff::Vector2f> flatten(FlattenOptions options = {}) const;

    using SegmentHandler = std::function<void(const Segment&)>;

    // Reason why not return segment is future-proofing (we may do some splitting in future)
    void offset(std::float_t dist, const SegmentHandler& handler) const;
    Segment reversed() const;

    bool is_line() const;
    bool is_quadratic() const;
    bool is_cubic() const;

    Segment to_cubic() const;

    static Segment line(const LineSegment2f& line);
    static Segment quadratic(const LineSegment2f& line, const mff::Vector2f& ctrl);
    static Segment cubic(const LineSegment2f& line, const LineSegment2f& ctrl);
    static Segment arc(std::float_t phi);

    static Segment quarter_circle_arc();
};

}