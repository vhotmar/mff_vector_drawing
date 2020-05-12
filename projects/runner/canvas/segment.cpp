#include "./segment.h"

namespace canvas {

// Legendre-Gauss coefficients
// https://pomax.github.io/bezierinfo/legendre-gauss.html
std::array<std::double_t, 24> LG_times = {
    -0.0640568928626056260850430826247450385909,
    0.0640568928626056260850430826247450385909,
    -0.1911188674736163091586398207570696318404,
    0.1911188674736163091586398207570696318404,
    -0.3150426796961633743867932913198102407864,
    0.3150426796961633743867932913198102407864,
    -0.4337935076260451384870842319133497124524,
    0.4337935076260451384870842319133497124524,
    -0.5454214713888395356583756172183723700107,
    0.5454214713888395356583756172183723700107,
    -0.6480936519369755692524957869107476266696,
    0.6480936519369755692524957869107476266696,
    -0.7401241915785543642438281030999784255232,
    0.7401241915785543642438281030999784255232,
    -0.8200019859739029219539498726697452080761,
    0.8200019859739029219539498726697452080761,
    -0.8864155270044010342131543419821967550873,
    0.8864155270044010342131543419821967550873,
    -0.9382745520027327585236490017087214496548,
    0.9382745520027327585236490017087214496548,
    -0.9747285559713094981983919930081690617411,
    0.9747285559713094981983919930081690617411,
    -0.9951872199970213601799974097007368118745,
    0.9951872199970213601799974097007368118745
};

std::array<std::double_t, 24> LG_weights = {
    0.1279381953467521569740561652246953718517,
    0.1279381953467521569740561652246953718517,
    0.1258374563468282961213753825111836887264,
    0.1258374563468282961213753825111836887264,
    0.121670472927803391204463153476262425607,
    0.121670472927803391204463153476262425607,
    0.1155056680537256013533444839067835598622,
    0.1155056680537256013533444839067835598622,
    0.1074442701159656347825773424466062227946,
    0.1074442701159656347825773424466062227946,
    0.0976186521041138882698806644642471544279,
    0.0976186521041138882698806644642471544279,
    0.086190161531953275917185202983742667185,
    0.086190161531953275917185202983742667185,
    0.0733464814110803057340336152531165181193,
    0.0733464814110803057340336152531165181193,
    0.0592985849154367807463677585001085845412,
    0.0592985849154367807463677585001085845412,
    0.0442774388174198061686027482113382288593,
    0.0442774388174198061686027482113382288593,
    0.0285313886289336631813078159518782864491,
    0.0285313886289336631813078159518782864491,
    0.0123412297999871995468056670700372915759,
    0.0123412297999871995468056670700372915759
};

LineSegment2f Segment::get_baseline() const {
    return std::visit([](const auto& i) { return i.baseline; }, data);
}

mff::Vector2f Segment::get_last_control() const {
    return std::visit(
        mff::overloaded{
            [](const Kind_::Line& line) -> mff::Vector2f {
                return line.baseline.to;
            },
            [&](const Kind_::Quadratic& quad) -> mff::Vector2f {
                return quad.control;
            },
            [&](const Kind_::Cubic& cubic) -> mff::Vector2f {
                return cubic.control.to;
            }
        },
        data
    );
}

mff::Vector2f Segment::derivative(std::float_t t) const {
    auto dt = (1 - t);

    return std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> mff::Vector2f {
                return (line.baseline.to - line.baseline.from) * dt;
            },
            [&](const Kind_::Quadratic& quad) -> mff::Vector2f {
                return (2 * dt * (quad.control - quad.baseline.from)
                    + 2 * t * (quad.baseline.to - quad.control));
            },
            [&](const Kind_::Cubic& cubic) -> mff::Vector2f {
                return 3 * (dt * dt) * (cubic.control.from - cubic.baseline.from)
                    + 6 * dt * t * (cubic.control.to - cubic.control.from)
                    + 3 * t * t * (cubic.baseline.to - cubic.control.to);
            }
        },
        data
    );
}

std::float_t Segment::length() const {
    if (is_line()) return get_baseline().vector().norm();

    // Using Legendre-Gauss quadrature
    // https://pomax.github.io/bezierinfo/#arclength

    std::double_t z = 0.5f;
    std::double_t sum = 0.0f;

    for (int i = 0; i < 24; i++) {
        std::double_t t = z * LG_times[i] + z;
        sum += LG_weights[i] * (derivative(t).norm());
    }

    return sum;
}

std::float_t Segment::time_for_distance(std::float_t dist) const {
    return dist / length();
}

Segment Segment::transform(const Transform2f& t) const {
    auto new_data = std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> Kind {
                return Kind_::Line{t.apply(line.baseline)};
            },
            [&](const Kind_::Quadratic& quad) -> Kind {
                return Kind_::Quadratic{t.apply(quad.baseline), t.apply(quad.control)};
            },
            [&](const Kind_::Cubic& cubic) -> Kind {
                return Kind_::Cubic{t.apply(cubic.baseline), t.apply(cubic.control)};
            }
        },
        data
    );

    return Segment{new_data};
}

mff::Vector2f Segment::normal(std::float_t t) const {
    auto normalized_derivative = derivative(t).normalized();

    return {-normalized_derivative[1], normalized_derivative[0]};
}

mff::Vector2f Segment::evaluate(std::float_t t) const {
    return std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> mff::Vector2f {
                return mff::lerp(line.baseline.from, line.baseline.to, t);
            },
            [&](const Kind_::Quadratic& quad) -> mff::Vector2f {
                return quad.hull(t).p123;
            },
            [&](const Kind_::Cubic& cubic) -> mff::Vector2f {
                return cubic.hull(t).p1234;
            }
        },
        data
    );
}

std::pair<Segment, Segment> Segment::split(std::float_t t) const {
    using result_t = std::pair<Segment, Segment>;

    return std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> result_t {
                auto p = evaluate(t);

                return {Segment::line({line.baseline.from, p}), Segment::line({p, line.baseline.to})};
            },
            [&](const Kind_::Quadratic& quad) -> result_t {
                auto hull = quad.hull(t);

                return {
                    Segment::quadratic({hull.p1, hull.p123}, hull.p12),
                    Segment::quadratic({hull.p123, hull.p3}, hull.p23)
                };
            },
            [&](const Kind_::Cubic& cubic) -> result_t {
                auto hull = cubic.hull(t);

                return {
                    Segment::cubic({hull.p1, hull.p1234}, {hull.p12, hull.p123}),
                    Segment::cubic({hull.p1234, hull.p4}, {hull.p234, hull.p34})
                };
            }
        },
        data
    );
}

std::vector<mff::Vector2f> Segment::flatten(FlattenOptions options) const {
    using result_t = std::vector<mff::Vector2f>;

    return std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> result_t {
                return {line.baseline.from, line.baseline.to};
            },
            [&](const auto& other) -> result_t {
                // super simple, just split the curve into multiple steps
                std::float_t step = 1.0f / options.steps;

                std::vector<mff::Vector2f> result(options.steps + 1);

                for (int i = 0; i <= options.steps; i++) {
                    result[i] = evaluate(step * ((std::float_t) i));
                }

                return result;
            }
        },
        data
    );
}

bool Segment::is_line() const {
    return std::holds_alternative<Kind_::Line>(data);
}

bool Segment::is_quadratic() const {
    return std::holds_alternative<Kind_::Quadratic>(data);
}

bool Segment::is_cubic() const {
    return std::holds_alternative<Kind_::Cubic>(data);
}

Segment Segment::to_cubic() const {
    return std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> Segment {
                auto cp = (line.baseline.from + line.baseline.to) / 2;
                return Segment::cubic(line.baseline, {cp, cp});
            },
            [&](const Kind_::Quadratic& quad) -> Segment {
                return Segment::cubic(
                    quad.baseline,
                    {
                        quad.baseline.from + (2.0f / 3.0f) * (quad.control - quad.baseline.from),
                        quad.baseline.to + (2.0f / 3.0f) * (quad.control - quad.baseline.to)
                    }
                );
            },
            [&](const Kind_::Cubic& cubic) -> Segment {
                return Segment{cubic};
            }
        },
        data
    );
}

void Segment::offset(std::float_t dist, const Segment::SegmentHandler& handler) const {
    auto from_triangle = [](
        const mff::Vector2f& p1,
        const mff::Vector2f& p2,
        const mff::Vector2f& p3,
        std::float_t dist
    ) -> std::tuple<mff::Vector2f, mff::Vector2f, mff::Vector2f> {
        auto l1 = LineSegment2f{p1, p2};
        auto l2 = LineSegment2f{p2, p3};

        l1 = l1.offset(dist);
        l2 = l2.offset(dist);

        mff::Vector2f cp = (l1.to + l2.from) / 2.0f;

        return std::make_tuple(l1.from, cp, l2.to);
    };

    // for now provide simple one time split (could be done to arbitary precision)

    auto result = std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> Segment {
                return Segment::line(line.baseline.offset(dist));
            },
            [&](const Kind_::Quadratic& quad) -> Segment {
                auto[from, cp, to] = from_triangle(
                    quad.baseline.from,
                    quad.control,
                    quad.baseline.to,
                    dist
                );

                return Segment::quadratic({from, to}, cp);
            },
            [&](const Kind_::Cubic& cubic) -> Segment {
                if (cubic.baseline.from == cubic.control.from) {
                    auto[from, cp, to] = from_triangle(
                        cubic.baseline.from,
                        cubic.control.to,
                        cubic.baseline.to,
                        dist
                    );

                    return Segment::cubic({from, to}, {from, cp});
                }

                if (cubic.baseline.to == cubic.control.to) {
                    auto[from, cp, to] = from_triangle(
                        cubic.baseline.from,
                        cubic.control.from,
                        cubic.baseline.to,
                        dist
                    );

                    return Segment::cubic(LineSegment2f{from, to}, LineSegment2f{cp, to});
                }

                auto l1 = LineSegment2f{cubic.baseline.from, cubic.control.from};
                auto l2 = LineSegment2f(cubic.control);
                auto l3 = LineSegment2f{cubic.control.to, cubic.baseline.to};

                l1 = l1.offset(dist);
                l2 = l2.offset(dist);
                l3 = l3.offset(dist);

                mff::Vector2f cp1 = (l1.to + l2.from) / 2.0f;
                mff::Vector2f cp2 = (l2.to + l3.from) / 2.0f;

                return Segment::cubic(LineSegment2f{l1.from, l3.to}, LineSegment2f{cp1, cp2});
            }
        },
        data
    );

    handler(result);
}

Segment Segment::line(const LineSegment2f& line) {
    return Segment{Kind_::Line{
        line
    }};
}

Segment Segment::quadratic(const LineSegment2f& line, const mff::Vector2f& ctrl) {
    return Segment{Kind_::Quadratic{
        line,
        ctrl
    }};
}

Segment Segment::cubic(const LineSegment2f& line, const LineSegment2f& ctrl) {
    return Segment{Kind_::Cubic{line, ctrl}};
}

Segment Segment::arc(std::float_t phi) {
    // https://pomax.github.io/bezierinfo/#circles_cubic
    auto f = std::tan(phi / 4.0f) * (4.0f / 3.0f);
    auto sin_phi = std::sin(phi);
    auto cos_phi = std::cos(phi);

    auto s = mff::Vector2f(1.0f, 0.0f);
    auto e = mff::Vector2f(cos_phi, sin_phi);

    auto c1 = mff::Vector2f(1.0f, f);
    auto c2 = mff::Vector2f(cos_phi + f * sin_phi, sin_phi - f * cos_phi);

    return Segment::cubic({s, e}, {c1, c2});
}

Segment Segment::quarter_circle_arc() {
    auto base = arc(M_PI_2);

    return base;
}

Segment Segment::reversed() const {
    return std::visit(
        mff::overloaded{
            [&](const Kind_::Line& line) -> Segment {
                return Segment::line(line.baseline.reversed());
            },
            [&](const Kind_::Quadratic& quad) -> Segment {
                return Segment::quadratic(
                    quad.baseline.reversed(),
                    quad.control
                );
            },
            [&](const Kind_::Cubic& cubic) -> Segment {
                return Segment::cubic(cubic.baseline.reversed(), cubic.control.reversed());
            }
        },
        data
    );
}


Kind_::Quadratic::Hull Kind_::Quadratic::hull(std::float_t t) const {
    auto p1 = baseline.from;
    auto p2 = control;
    auto p3 = baseline.to;

    auto p12 = mff::lerp(p1, p2, t);
    auto p23 = mff::lerp(p2, p3, t);

    auto p123 = mff::lerp(p12, p23, t);

    return {p1, p2, p3, p12, p23, p123};
}

Kind_::Cubic::Hull Kind_::Cubic::hull(std::float_t t) const {
    auto p1 = baseline.from;
    auto p2 = control.from;
    auto p3 = control.to;
    auto p4 = baseline.to;

    auto p12 = mff::lerp(p1, p2, t);
    auto p23 = mff::lerp(p2, p3, t);
    auto p34 = mff::lerp(p3, p4, t);

    auto p123 = mff::lerp(p12, p23, t);
    auto p234 = mff::lerp(p23, p34, t);

    auto p1234 = mff::lerp(p123, p234, t);
    return {p1, p2, p3, p4, p12, p23, p34, p123, p234, p1234};
}

}
