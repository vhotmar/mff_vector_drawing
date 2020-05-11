#include "./stroke.h"

#include "./contour.h"
#include "./segment.h"
#include "./outline.h"

namespace canvas {

// https://www.w3.org/TR/SVG2/painting.html#LineJoin
void add_join_to_contour(
    Contour& contour,
    std::float_t dist,
    const LineJoin& join,
    const mff::Vector2f& join_to,
    const LineSegment2f& next_tangent
) {
    auto tangent_to_continue = contour.last_tangent();

    if (!tangent_to_continue) return;

    std::visit(
        mff::overloaded{
            [](const LineJoin_::Bevel b) {
                // do nothing... chain segments
            },
            [&](const LineJoin_::Miter m) {
                // check limit per specification
                auto miter_length = std::abs(
                    dist / std::sin(next_tangent.inner_angle_between(tangent_to_continue.value())));
                if (miter_length > ((std::float_t) m.value)) return;

                // as per specification prolong the outer edges by tangents (until miterlimit is hit)
                auto intersection_t = tangent_to_continue->intersection(next_tangent);
                if (!intersection_t) return; // if no intersection then there is some numerical error (skip)

                auto intersection_point = tangent_to_continue->evaluate(intersection_t.value());

                contour.add_endpoint(intersection_point);
            },
            [](const LineJoin_::Round r) {
                // TODO: implement round join
            },
        }, join
    );
}

void add_segment_to_contour(
    Contour& contour,
    const Segment& segment,
    std::float_t dist,
    const LineJoin& join,
    const mff::Vector2f& join_to
) {
    add_join_to_contour(
        contour,
        dist,
        join,
        join_to,
        LineSegment2f{segment.get_last_control(), segment.get_baseline().from}
    );

    contour.add_segment(segment);
}

void offset_contour_forward(const Contour& to_offset, Contour& result, std::float_t radius, const LineJoin& join) {
    std::size_t index = 0;

    for (const auto& segment: to_offset.segment_view()) {
        segment.offset(
            -radius, [&](const Segment& segment) {
                add_segment_to_contour(
                    result,
                    segment,
                    -radius,
                    index == 0 ? LineJoin_::Bevel{} : join,
                    segment.get_baseline().from
                );
            }
        );

        index++;
    }
}

void offset_contour_backward(const Contour& to_offset, Contour& result, std::float_t radius, const LineJoin& join) {
    std::size_t index = 0;

    // TODO: investigate problem with ranges::views::reverse here
    auto view = to_offset.segment_view() | ranges::to<std::vector>();

    for (const auto& segment: view | ranges::views::reverse) {
        segment.reversed().offset(
            -radius, [&](const Segment& segment) {
                add_segment_to_contour(
                    result,
                    segment,
                    -radius,
                    index == 0 ? LineJoin_::Bevel{} : join,
                    segment.get_baseline().from
                );
            }
        );

        index++;
    }
}

std::vector<Contour> stroke(const Contour& to_stroke, const StrokeStyle& style) {
    std::vector<Contour> result;

    Contour output = {};

    offset_contour_forward(to_stroke, output, style.line_width / 2.0f, style.line_join);

    if (to_stroke.closed) {
        add_join_to_contour(
            output,
            style.line_width / 2.0f,
            style.line_join,
            to_stroke.points[0],
            LineSegment2f{output.points[1], output.points[0]}
        );
    }

    auto offset = output.points.size();

    offset_contour_backward(to_stroke, output, style.line_width / 2.0f, style.line_join);

    if (to_stroke.closed) {
        add_join_to_contour(
            output,
            style.line_width / 2.0f,
            style.line_join,
            to_stroke.points[0],
            LineSegment2f{output.points[offset + 1], output.points[offset]}
        );
    }

    result.push_back(std::move(output));

    return result;
}

/*Outline offset(const Outline& to_offset) {
    Outline result = {};
    std::vector<Contour>

    auto contours = to_offset.get_contours();

    for (const auto& contour: contours) {

    }
}*/



StrokeResult get_stroke(
    const std::vector<mff::Vector2f>& flattened,
    const StrokeStyle& style,
    bool loop
) {
    if (flattened.size() < 2) return {};

    std::vector<mff::Vector2f> result_vertices;
    std::vector<std::uint32_t> result_indices;

    auto half_width = style.line_width / 2.0f;

    auto get_perpendicular = [](const mff::Vector2f& a) -> mff::Vector2f { return (mff::Vector2f{a[1], -a[0]}); };

    bool skip_last = loop && ((flattened[0] - flattened[flattened.size() - 1]).norm() < 0.001f);
    auto flattened_size = (skip_last ? flattened.size() - 1 : flattened.size());
    auto get_index = [&](std::int32_t index) -> std::size_t {
        if (index < 0) return (flattened_size - std::abs(index % ((std::int32_t) flattened_size)));
        return index % flattened_size;
    };

    auto get_point = [&](std::int32_t index) -> mff::Vector2f { return flattened[get_index(index)]; };
    auto is_approx_zero = [](const mff::Vector2f& a) -> bool { return a.norm() < 0.001; };
    auto add_vert = [&](const mff::Vector2f& a) { result_vertices.push_back(a); };

    auto add_indices_rectangle = [&](std::size_t from) {
        result_indices.push_back(from);
        result_indices.push_back(from + 2);
        result_indices.push_back(from + 1);
        result_indices.push_back(from + 1);
        result_indices.push_back(from + 2);
        result_indices.push_back(from + 3);
    };

    auto add_indices_triangle = [&](std::size_t p1, std::size_t p2, std::size_t p3) {
        result_indices.push_back(p1);
        result_indices.push_back(p2);
        result_indices.push_back(p3);
    };

    auto add_cap = [&](const mff::Vector2f& p0, const mff::Vector2f& p1, bool start) {
        auto curr_vertices_ix = result_vertices.size();

        if (start) {
            auto normal = (p1 - p0).normalized();
            mff::Vector2f extrusion = normal * half_width;

            auto base_point = p0;

            if (std::holds_alternative<LineCap_::Square>(style.line_cap)) {
                // if square cap move start point back
                base_point = p0 - extrusion;
            }

            extrusion = get_perpendicular(extrusion);

            add_vert(base_point + extrusion);
            add_vert(base_point - extrusion);

            add_indices_rectangle(curr_vertices_ix);
        } else {
            auto normal = (p0 - p1).normalized();
            mff::Vector2f extrusion = normal * half_width;

            auto base_point = p0;

            if (std::holds_alternative<LineCap_::Square>(style.line_cap)) {
                // if square cap move start point forward
                base_point = p0 + extrusion;
            }

            extrusion = get_perpendicular(extrusion);

            add_vert(base_point + extrusion);
            add_vert(base_point - extrusion);
        }
    };

    auto add_step = [&](const mff::Vector2f& pL, const mff::Vector2f& p0, const mff::Vector2f& pR) {
        mff::Vector2f dL = (p0 - pL); // left connection
        mff::Vector2f dR = (pR - p0); // right connection

        dL = dL.normalized(); // normalize
        dR = dR.normalized();

        auto dLp = get_perpendicular(dL);
        auto dRp = get_perpendicular(dR);

        // helpers
        auto dot = dL.dot(dR);
        auto alpha = std::acos(dot) / 2.0f; // normalized length
        auto cross = dL[0] * dR[1] - dL[1] * dR[0];

        if (cross < 0) alpha = -alpha;

        // this vector is directed in the "join"
        mff::Vector2f bisec = get_perpendicular((dL + dR).normalized()) * (half_width / std::cos(alpha));

        auto curr_vertices_ix = result_vertices.size();

        std::visit(
            mff::overloaded{
                [&](const LineJoin_::Bevel b) {
                    if (cross < 0) {
                        add_vert(p0 + bisec);
                        add_vert(p0 - (dLp * half_width));

                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 2, curr_vertices_ix + 1);
                        add_indices_triangle(curr_vertices_ix + 2, curr_vertices_ix + 4, curr_vertices_ix);
                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 3, curr_vertices_ix + 4);

                        add_vert(p0 - (dRp * half_width));
                    } else {
                        add_vert(p0 + (dLp * half_width));
                        add_vert(p0 - bisec);

                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 2, curr_vertices_ix + 1);
                        add_indices_triangle(curr_vertices_ix + 2, curr_vertices_ix + 3, curr_vertices_ix + 1);
                        add_indices_triangle(curr_vertices_ix + 1, curr_vertices_ix + 3, curr_vertices_ix + 4);

                        add_vert(p0 + (dRp * half_width));
                    }
                },
                [&](const LineJoin_::Miter m) {
                    add_vert(p0 + bisec);
                    add_vert(p0 - bisec);
                    add_indices_rectangle(curr_vertices_ix);
                },
                [&](const LineJoin_::Round r) {
                    // TODO: implement round join

                    if (cross < 0) {
                        add_vert(p0 - (dRp * half_width));
                    } else {
                        add_vert(p0 + (dRp * half_width));
                    }
                },
            }, style.line_join
        );

        return cross;
    };


    auto pL = get_point(-1);
    auto p0 = get_point(0);
    auto pR = get_point(1);

    std::size_t start = 0;
    std::size_t end = flattened.size();

    if (!loop) {
        start = 1;
        end = flattened.size() - 1;

        add_cap(flattened[0], flattened[1], true);

        pL = p0;
        p0 = pR;
        pR = get_point(2);
    }

    for (auto i = start; i < end; ++i) {
        if ((p0 - pL).norm() < 0.0001 || (pR - p0).norm() < 0.0001) {
            p0 = pR;
            pR = get_point(i + 2);
        }

        add_step(pL, p0, pR);

        pL = p0;
        p0 = pR;
        pR = get_point(i + 2);
    }

    if (!loop) {
        add_cap(get_point(-1), get_point(-2), false);
    } else {
        auto c = add_step(get_point(-2), get_point(-1), get_point(0));
        auto bi = result_indices.size() - 6;

        if (c < 0 && !std::holds_alternative<LineJoin_::Miter>(style.line_join)) {
            result_indices[bi + 1] = 1;
            result_indices[bi + 4] = 1;
            result_indices[bi + 5] = 0;
        } else {
            result_indices[bi + 1] = 0;
            result_indices[bi + 4] = 0;
            result_indices[bi + 5] = 1;
        }
    }

    return {result_vertices, result_indices};
}


}
