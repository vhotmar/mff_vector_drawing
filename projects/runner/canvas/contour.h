#pragma once

#include <vector>

#include <range/v3/all.hpp>
#include <mff/graphics/math.h>

#include "./segment.h"

namespace canvas {

enum class PointFlag : std::uint8_t {
    CONCRETE = 0,
    CONTROL_POINT_0 = 1,
    CONTROL_POINT_1 = 2
};

/**
 * One concrete simple path (that means this path has no "interruptions" / "blank" spaces
 */
struct Contour {
    // The points of the path
    std::vector<mff::Vector2f> points = {};
    // Is the point start/end point, or control point?
    std::vector<PointFlag> point_flags = {};
    // is the contour closed?
    bool closed = false;

    /**
     * Close the contour
     */
    void close();

    /**
     * Is the contour empty? (are there no points)
     * @return
     */
    bool empty() const;

    /**
     * Helper function to add point and flag
     * @param point
     * @param flag
     */
    void add_point(const mff::Vector2f& point, PointFlag flag);

    /**
     * Add next endpoint (end of line)
     * @param point
     */
    void add_endpoint(const mff::Vector2f& point);

    /**
     * Add quadratic bezier curve from last point to next specified point with specified control
     * point
     * https://pomax.github.io/bezierinfo/#control
     * @param control the control point
     * @param point quadratic curve endpoint
     */
    void add_quadratic(const mff::Vector2f& control, const mff::Vector2f& point);

    /**
     * Add cubic bezier curve from last point to next specified point with specified control point
     * https://pomax.github.io/bezierinfo/#control
     * @param control0 the first control point
     * @param control1 the second control point
     * @param point
     */
    void add_cubic(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point);

    /**
     * Add segment (primitive) to this contour
     * @param point
     */
    void add_segment(const Segment& point);

    /**
     * Add ellipse to this contour
     * @param transform this transform decides how the ellipse will look like (where it will be
     *                  and the scale
     */
    void add_ellipse(const Transform2f& transform);

    /**
     * Transform this contour (transform all the points from which this contour consists of)
     * @param transform
     */
    void transform(const Transform2f& transform);

    /**
     * Number of points this contour contains
     * @return
     */
    std::size_t size() const;

    /**
     * Flatten this contour into points
     * @return
     */
    std::vector<mff::Vector2f> flatten() const; // TODO: add flatten options

    /**
     * Get the last tangent
     * @return
     */
    std::optional<LineSegment2f> last_tangent() const;

    /**
     * View which converts Contour into segments (from array of points get concrete segments
     * representing this contour
     */
    class ContourSegmentView : public ranges::view_facade<ContourSegmentView, ranges::unknown> {
        friend ranges::range_access;

    private:
        const Contour* contour_ = nullptr;
        std::size_t index_ = 1;
        Segment current_segment_;
        bool ignore_close_segment_ = false;
        bool end_ = false;

        const Segment& read() const;

        bool equal(ranges::default_sentinel_t) const;

        void next();

    public:
        ContourSegmentView() = default;

        ContourSegmentView(const Contour* contour_, bool ignore_close_segment = false);
    };

    struct SegmentViewOptions {
        bool ignore_close_segment = false;
    };

    /**
     * Get segments representing this contour
     * @param options
     * @return
     */
    ContourSegmentView segment_view(const SegmentViewOptions& options = {false}) const;
};

}