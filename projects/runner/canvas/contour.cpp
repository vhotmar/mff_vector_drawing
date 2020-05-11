#define _USE_MATH_DEFINES
#include <cmath>

#include "./contour.h"
#include "../renderer2.h"

namespace canvas {

void Contour::close() {
    closed = true;
}

bool Contour::empty() const {
    return points.empty();
}

void Contour::add_point(const mff::Vector2f& point, PointFlag flag) {
    points.push_back(point);
    point_flags.push_back(flag);
}

void Contour::add_endpoint(const mff::Vector2f& point) {
    add_point(point, PointFlag::CONCRETE);
}

void Contour::add_quadratic(const mff::Vector2f& control, const mff::Vector2f& point) {
    add_point(control, PointFlag::CONTROL_POINT_0);
    add_point(point, PointFlag::CONCRETE);
}

void Contour::add_cubic(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point) {
    add_point(control0, PointFlag::CONTROL_POINT_0);
    add_point(control1, PointFlag::CONTROL_POINT_1);
    add_point(point, PointFlag::CONCRETE);
}

void Contour::add_segment(const Segment& segment) {
    add_point(segment.get_baseline().from, PointFlag::CONCRETE);

    std::visit(
        mff::overloaded{
            [&](const Kind_::Quadratic& quad) {
                add_point(quad.control, PointFlag::CONTROL_POINT_0);
            },
            [&](const Kind_::Cubic& quad) {
                add_point(quad.control.from, PointFlag::CONTROL_POINT_0);
                add_point(quad.control.to, PointFlag::CONTROL_POINT_1);
            },
            [](const auto& d) {}
        },
        segment.data
    );

    add_point(segment.get_baseline().to, PointFlag::CONCRETE);
}


void Contour::add_ellipse(const Transform2f& transform) {
    auto seg = Segment::quarter_circle_arc();
    add_segment(seg.transform(transform));
    add_segment(seg.transform(transform * Transform2f::from_rotation(M_PI_2)));
    add_segment(seg.transform(transform * Transform2f::from_rotation(2 * M_PI_2)));
    add_segment(seg.transform(transform * Transform2f::from_rotation(3 * M_PI_2)));
}

std::size_t Contour::size() const {
    return points.size();
}

void Contour::transform(const Transform2f& transform) {
    for (auto& point: points) {
        point = transform.apply(point);
    }
}

Contour::ContourSegmentView Contour::segment_view(const SegmentViewOptions& options) const {
    return ContourSegmentView(this, options.ignore_close_segment);
}

std::vector<mff::Vector2f> Contour::flatten() const {
    auto segments = segment_view();

    std::vector<mff::Vector2f> result;

    for (const auto& segment: segments) {
        auto flattened = segment.flatten();

        auto start = std::begin(flattened);

        if (!result.empty() && (result.back() == *start)) {
            start++;
        }

        result.insert(std::end(result), start, std::end(flattened));
    }

    return result;
}

std::optional<LineSegment2f> Contour::last_tangent() const {
    if (points.size() < 2) return std::nullopt;

    return LineSegment2f{points[points.size() - 2], points[points.size() - 1]};
}

const Segment& Contour::ContourSegmentView::read() const {
    return current_segment_;
}

bool Contour::ContourSegmentView::equal(ranges::default_sentinel_t) const {
    bool include_close_segment = contour_->closed && !ignore_close_segment_;

    return ((!include_close_segment && index_ > (contour_->size() - 1)) || index_ > contour_->size());
}

void Contour::ContourSegmentView::next() {
    auto from_point = contour_->points[index_ - 1];

    // closing part of contour
    if (index_ == contour_->size()) {
        auto to_point = contour_->points[0];
        current_segment_ = Segment::line({from_point, to_point});
        index_++;
        return;
    }

    auto get_next = [&]() -> std::tuple<bool, mff::Vector2f> {
        auto index = index_;
        index_++;
        return std::make_tuple(contour_->point_flags[index] == PointFlag::CONCRETE, contour_->points[index]);
    };

    auto[p1_end, p1] = get_next();
    if (p1_end) {
        current_segment_ = Segment::line({from_point, p1});
        return;
    }

    auto[p2_end, p2] = get_next();
    if (p2_end) {
        current_segment_ = Segment::quadratic({from_point, p2}, p1);
        return;
    }

    auto[p3_end, p3] = get_next();

    current_segment_ = Segment::cubic({from_point, p3}, {p1, p2});
}

Contour::ContourSegmentView::ContourSegmentView(const Contour* contour_, bool ignore_close_segment)
    : contour_(contour_)
    , ignore_close_segment_(ignore_close_segment) {
    next();
}

}