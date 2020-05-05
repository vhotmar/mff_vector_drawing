#include "./segment.h"

namespace canvas {

Segment Segment::transform(const Transform2f& t) {
    return Segment{
        t.apply(baseline),
        t.apply(control),
        kind
    };
}

std::float_t Segment::length() const {
    return baseline.vector().norm();
}

std::float_t Segment::time_for_distance(std::float_t dist) const {
    return dist / length();
}

mff::Vector2f Segment::evaluate(std::float_t t) const {
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

std::vector<mff::Vector2f> Segment::flatten(FlattenOptions options) {
    if (kind == SegmentKind::Line) return {baseline.from, baseline.to};

    // super simple, just split the curve into multiple steps
    std::float_t step = 1.0f / options.steps;

    std::vector<mff::Vector2f> result(options.steps + 1);

    for (int i = 0; i <= options.steps; i++) {
        result[i] = evaluate(step * i);
    }

    return result;
}

Segment Segment::to_cubic() const {
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

Segment Segment::line(const LineSegment2f& line) {
    return Segment{
        line,
        LineSegment2f::zero(),
        SegmentKind::Line
    };
}

Segment Segment::quadratic(const LineSegment2f& line, const mff::Vector2f& ctrl) {
    return Segment{
        line,
        LineSegment2f{ctrl, mff::Vector2f::Zero()},
        SegmentKind::Quadratic
    };
}

Segment Segment::cubic(const LineSegment2f& line, const LineSegment2f& ctrl) {
    return Segment{
        line,
        ctrl,
        SegmentKind::Cubic
    };
}

// https://pomax.github.io/bezierinfo/#circles_cubic
Segment Segment::arc(std::float_t phi) {
    auto f = std::tan(phi / 4.0f) * (4.0f / 3.0f);
    auto sin_phi = std::sin(phi);
    auto cos_phi = std::cos(phi);

    auto s = mff::Vector2f(1.0f, 0.0f);
    auto e = mff::Vector2f(cos_phi, sin_phi);

    auto c1 = mff::Vector2f(1.0f, f);
    auto c2 = mff::Vector2f(cos_phi + f * sin_phi, sin_phi - f * cos_phi);

    return Segment::cubic({s, e}, {c1, c2});
}

}
