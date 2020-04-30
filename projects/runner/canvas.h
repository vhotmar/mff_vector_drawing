#include <iostream>
#include <vector>
#include <variant>

#include <range/v3/all.hpp>
#include <mff/graphics/math.h>

namespace canvas {

struct LineSegment2f {
    mff::Vector2f from;
    mff::Vector2f to;

    mff::Vector2f vector() const {
        return to - from;
    }

    static LineSegment2f zero() {
        return LineSegment2f{mff::Vector2f::Zero(), mff::Vector2f::Zero()};
    }
};

struct Transform2f {
    mff::Matrix2f transform;
    mff::Vector2f transpose;

    mff::Vector2f apply(const mff::Vector2f& v) const {
        return mff::Vector2f(transform * v + transpose);
    }

    Transform2f apply(const Transform2f& t) const {
        return Transform2f{
            transform * t.transform,
            apply(t.transpose)
        };
    }

    LineSegment2f apply(const LineSegment2f& l) const {
        return LineSegment2f{apply(l.from), apply(l.to)};
    }
};

enum class SegmentKind {
    Line,
    Quadratic,
    Cubic
};

struct FlattenOptions {
    std::size_t steps = 10;
};

/**
 * Basic unit of one contour, it is either Line, Quadratic or Bezier
 */
struct Segment {
    LineSegment2f baseline;
    LineSegment2f control;
    SegmentKind kind;

    Segment transform(const Transform2f& t) {
        return Segment{
            t.apply(baseline),
            t.apply(control),
            kind
        };
    }

    std::float_t length() const {
        return baseline.vector().norm();
    }

    std::float_t time_for_distance(std::float_t dist) const {
        return dist / length();
    }

    mff::Vector2f evaluate(std::float_t t) const {
        auto get_point = [t](const mff::Vector2f& a, const mff::Vector2f& b) -> mff::Vector2f {
            return a + (b - a) * t;
        };

        if (kind == SegmentKind::Line) return get_point(baseline.from, baseline.to);
        if (kind == SegmentKind::Quadratic) {
            auto a = baseline.from;
            auto b = control.from;
            auto c = baseline.to;

            auto m = get_point(a, b);
            auto n = get_point(b, c);

            return get_point(m, n);
        };
        if (kind == SegmentKind::Cubic) {
            auto a = baseline.from;
            auto b = control.from;
            auto c = control.to;
            auto d = baseline.to;

            auto m = get_point(a, b);
            auto n = get_point(b, c);
            auto o = get_point(c, d);

            auto x = get_point(m, n);
            auto y = get_point(n, o);

            return get_point(x, y);
        }
    }

    std::vector<mff::Vector2f> flatten(FlattenOptions options = {}) {
        if (kind == SegmentKind::Line) return {baseline.from, baseline.to};

        // super simple, just split the curve into multiple steps
        std::float_t step = 1.0f / options.steps;

        std::vector<mff::Vector2f> result(options.steps + 1);

        for (int i = 0; i <= options.steps; i++) {
            result[i] = evaluate(step * i);
        }

        return result;
    }

    Segment to_cubic() const {
        if (kind == SegmentKind::Cubic) return *this;
        if (kind == SegmentKind::Quadratic) {
            return Segment{
                baseline,
                {
                    baseline.from + (2.0f / 3.0f) * (control.from - baseline.from),
                    baseline.to + (2.0f / 3.0f) * (control.from - baseline.to)
                },
                SegmentKind::Cubic
            };
        }
        if (kind == SegmentKind::Line) {
            auto cp = (baseline.from + baseline.to) / 2;
            return Segment{
                baseline,
                {cp, cp},
                SegmentKind::Cubic
            };
        }
    }

    static Segment line(const LineSegment2f& line) {
        return Segment{
            line,
            LineSegment2f::zero(),
            SegmentKind::Line
        };
    }

    static Segment quadratic(const LineSegment2f& line, const mff::Vector2f& ctrl) {
        return Segment{
            line,
            LineSegment2f{ctrl, mff::Vector2f::Zero()},
            SegmentKind::Quadratic
        };
    }

    static Segment cubic(const LineSegment2f& line, const LineSegment2f& ctrl) {
        return Segment{
            line,
            ctrl,
            SegmentKind::Cubic
        };
    }

    // https://pomax.github.io/bezierinfo/#circles_cubic
    static Segment arc(std::float_t phi) {
        auto f = std::tan(phi / 4.0f) * (4.0f / 3.0f);
        auto sin_phi = std::sin(phi);
        auto cos_phi = std::cos(phi);

        auto s = mff::Vector2f(1.0f, 0.0f);
        auto e = mff::Vector2f(cos_phi, sin_phi);

        auto c1 = mff::Vector2f(1.0f, f);
        auto c2 = mff::Vector2f(cos_phi + f * sin_phi, sin_phi - f * cos_phi);

        return Segment::cubic({s, e}, {c1, c2});
    }
};

enum class PointFlag : std::uint8_t {
    CONCRETE = 0,
    CONTROL_POINT_0 = 1,
    CONTROL_POINT_1 = 2
};

/**
 * One concrete path
 */
struct Contour {
    std::vector<mff::Vector2f> points;
    std::vector<PointFlag> point_flags;
    // bounds?
    bool closed = false;

    void close() {
        closed = true;
    }

    bool empty() const {
        return points.empty();
    }

    void add_point(const mff::Vector2f& point, PointFlag flag) {
        points.push_back(point);
        point_flags.push_back(flag);
    }

    void add_endpoint(const mff::Vector2f& point) {
        add_point(point, PointFlag::CONCRETE);
    }

    void add_quadratic(const mff::Vector2f& control, const mff::Vector2f& point) {
        add_point(control, PointFlag::CONTROL_POINT_0);
        add_point(point, PointFlag::CONCRETE);
    }

    void add_cubic(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point) {
        add_point(control0, PointFlag::CONTROL_POINT_0);
        add_point(control1, PointFlag::CONTROL_POINT_1);
        add_point(point, PointFlag::CONCRETE);
    }

    std::size_t size() const {
        return points.size();
    }

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

        const Segment& read() const {
            return current_segment_;
        }

        bool equal(ranges::default_sentinel_t) const {
            bool include_close_segment = contour_->closed && !ignore_close_segment_;

            if (!include_close_segment) return index_ == (contour_->size() + 1);
            return index_ == (contour_->size() + 2);
        }

        void next() {
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

    public:
        ContourSegmentView() = default;

        ContourSegmentView(const Contour* contour_, bool ignore_close_segment = false)
            : contour_(contour_)
            , ignore_close_segment_(ignore_close_segment) {
            next();
        }
    };

    ContourSegmentView segment_view(bool ignore_close_segment = false) const {
        return ContourSegmentView(this, ignore_close_segment);
    }

    std::vector<mff::Vector2f> flatten() {
        auto segments = segment_view();

        std::vector<mff::Vector2f> result;

        for (auto segment: segments) {
            auto flattened = segment.flatten();

            auto start = std::begin(flattened);

            if (!result.empty() && (result.back() == *start)) {
                start++;
            }

            result.insert(std::end(result), start, std::end(flattened));
        }

        return result;
    }
};

class Outline {
public:
    void add_contour(const Contour& contour) {
        if (contour.empty()) return;

        contours_.push_back(contour);
    }

    const std::vector<Contour>& get_contours() {
        return contours_;
    }

private:
    std::vector<Contour> contours_;
};

class Path2D {
public:
    void close_path() {
        current_contour_.close();
    }

    void move_to(const mff::Vector2f& point) {
        end_current_contour();
        current_contour_.add_endpoint(point);
    }

    void line_to(const mff::Vector2f& point) {
        current_contour_.add_endpoint(point);
    }

    void quad_to(const mff::Vector2f& control, const mff::Vector2f& point) {
        current_contour_.add_quadratic(control, point);
    }

    void bezier_to(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point) {
        current_contour_.add_cubic(control0, control1, point);
    }

    Outline get_outline() {
        end_current_contour();
        return outline_;
    }

private:
    Outline outline_;
    Contour current_contour_;

    void end_current_contour() {
        if (!current_contour_.empty()) {
            outline_.add_contour(current_contour_);
            current_contour_ = Contour{};
        }
    }
};

struct Coloru {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
};

struct Colorf {
    std::float_t r = 0.0f;
    std::float_t g = 0.0f;
    std::float_t b = 0.0f;
    std::float_t a = 0.0f;
};

namespace LineCap {

struct Butt {};
struct Square {};
struct Round {};

using type = std::variant<Butt, Square, Round>;

}

namespace LineJoin {

struct Miter { float value; };
struct Bevel {};
struct Round {};

using type = std::variant<Miter, Bevel, Round>;

}

struct StrokeStyle {
    std::float_t line_width;
    LineCap::type line_cap;
    LineJoin::type line_join;
};

struct State {
    // transform?
    std::float_t line_width;
    std::float_t miter_limit;
    LineCap::type line_cap;
    LineJoin::type line_join;
    Coloru stroke_color;
    Coloru fill_color;
};

class Canvas {

};

}

/*int main() {
    canvas::Path2D path;

    path.move_to({0, 0});
    path.line_to({0, 10});
    path.quad_to({5, 5}, {10, 10});
    path.line_to({10, 0});
    path.close_path();

    Eigen::IOFormat base_format(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", ", ", "", "", "{", "}");

    std::cout << "MAIN" << std::endl;
    auto cs = path.get_outline().get_contours();
    std::cout << "SI " << cs.size() << std::endl;
    for (auto contour: cs) {
        auto flattened = contour.flatten();

        std::cout << "Points { ";
        std::string comma;
        for (const auto& point: flattened) {
            std::cout << comma << point.format(base_format);
            comma = ", ";
        }

        std::cout << " }" << std::endl;
    }

    return 0;
}*/