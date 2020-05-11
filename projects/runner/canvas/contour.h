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
 * One concrete path
 */
struct Contour {
    std::vector<mff::Vector2f> points = {};
    std::vector<PointFlag> point_flags = {};
    // bounds?
    bool closed = false;

    void close();

    bool empty() const;

    void add_point(const mff::Vector2f& point, PointFlag flag);

    void add_endpoint(const mff::Vector2f& point);
    void add_quadratic(const mff::Vector2f& control, const mff::Vector2f& point);
    void add_cubic(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point);
    void add_segment(const Segment& point);
    void add_ellipse(const Transform2f& transform);

    void transform(const Transform2f& transform);

    std::size_t size() const;

    std::vector<mff::Vector2f> flatten() const;

    std::optional<LineSegment2f> last_tangent() const;

    /**
     * View which converts Contour into segments
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

    ContourSegmentView segment_view(const SegmentViewOptions& options = {false}) const;
};

}