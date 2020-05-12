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

/**
 * Segment representing the rendering primitive of vector graphics
 * - Line
 * - Quadratic Curve
 * - Cubic Curve
 *
 * All other primitives can be transformed to these primitives with some error (which can be
 * controlled
 */
struct Segment {
    Kind data;

    /**
     * Get the baseling (from point and to point) of segment
     * @return the baseline
     */
    LineSegment2f get_baseline() const;

    /**
     * Get the last control point (used for tangents)
     * - Line (from point)
     * - Quadratic (the control point)
     * - Cubic (last control point)
     * @return the last control point
     */
    mff::Vector2f get_last_control() const;

    /**
     * Get the derivative of segment at time
     * @param t
     * @return the derivative
     */
    mff::Vector2f derivative(std::float_t t) const;

    /**
     * Get the length of the segment
     * @return
     */
    std::float_t length() const;

    /**
     * Get approx t for distance
     * @param dist
     * @return
     */
    std::float_t time_for_distance(std::float_t dist) const;

    /**
     * Transform this segment
     * @param t
     * @return the transformed segment
     */
    Segment transform(const Transform2f& t) const;

    /**
     * Get normal to this segment at time
     * @param t
     * @return the normal
     */
    mff::Vector2f normal(std::float_t t) const;

    /**
     * Evaluate this segment at time
     * @param t
     * @return point of segment(t)
     */
    mff::Vector2f evaluate(std::float_t t) const;

    /**
     * Split this segment at time
     * @param t
     * @return
     */
    std::pair<Segment, Segment> split(std::float_t t) const;

    /**
     * Flatten this segment to points
     * @param options
     * @return
     */
    std::vector<mff::Vector2f> flatten(FlattenOptions options = {}) const;

    using SegmentHandler = std::function<void(const Segment&)>;

    // Reason why not return segment is future-proofing (we may do some splitting in future)
    void offset(std::float_t dist, const SegmentHandler& handler) const;

    /**
     * Get this segment in reverse
     * @return
     */
    Segment reversed() const;

    // Helpers so we do not have to use std::holds_alternative when we do not need to
    bool is_line() const;
    bool is_quadratic() const;
    bool is_cubic() const;

    /**
     * Convert all basic types to cubic type
     * @return
     */
    Segment to_cubic() const;

    /**
     * Create Line segment
     * @param line
     * @return
     */
    static Segment line(const LineSegment2f& line);

    /**
     * Create quadratic segment
     * @param line
     * @param ctrl
     * @return
     */
    static Segment quadratic(const LineSegment2f& line, const mff::Vector2f& ctrl);

    /**
     * Create cubic segment
     * @param line
     * @param ctrl
     * @return
     */
    static Segment cubic(const LineSegment2f& line, const LineSegment2f& ctrl);

    /**
     * Create arc segment from specified angle
     * @param phi
     * @return
     */
    static Segment arc(std::float_t phi);

    /**
     * Get me quarter circle segment
     * @return
     */
    static Segment quarter_circle_arc();
};

}