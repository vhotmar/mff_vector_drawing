#include "./stroke.h"

#include "./outline.h"
#include "../renderer2.h"

namespace canvas {

// we do stroking manually (better way would be to use already existing splitting into contours and
// segments)
StrokeResult get_stroke(
    const std::vector<mff::Vector2f>& flattened,
    const StrokeStyle& style,
    bool loop
) {
    if (flattened.size() < 2) return {};

    std::vector<mff::Vector2f> result_vertices;
    std::vector<std::uint32_t> result_indices;

    auto half_width = style.line_width / 2.0f;

    // get left perpendicular vector
    auto get_perpendicular = [](const mff::Vector2f& a) -> mff::Vector2f { return (mff::Vector2f{a[1], -a[0]}); };

    // if the first and last element coincide then skip the last element
    bool skip_last = loop && mff::is_approx_zero(flattened.front() - flattened.back());
    auto flattened_size = (skip_last ? flattened.size() - 1 : flattened.size());
    // helper to get the correct index (including negatie indices)
    auto get_index = [&](std::int32_t index) -> std::size_t {
        if (index < 0) return (flattened_size - std::abs(index % ((std::int32_t) flattened_size)));
        return index % flattened_size;
    };

    // get the point with correct index
    auto get_point = [&](std::int32_t index) -> mff::Vector2f { return flattened[get_index(index)]; };
    auto add_vert = [&](const mff::Vector2f& a) { result_vertices.push_back(a); };

    // add triangle_indices
    auto add_indices_triangle = [&](std::size_t p1, std::size_t p2, std::size_t p3) {
        result_indices.push_back(p1);
        result_indices.push_back(p2);
        result_indices.push_back(p3);
    };

    // add rectangle indices starting with specified index
    auto add_indices_rectangle = [&](std::size_t from) {
        add_indices_triangle(from, from + 2, from + 1);
        add_indices_triangle(from + 1, from + 2, from + 3);
    };

    // helper method to add line end caps + doc paper
    auto add_cap = [&](const mff::Vector2f& p0, const mff::Vector2f& p1, bool start) {
        auto curr_vertices_ix = result_vertices.size();

        if (start) {
            auto direction = (p1 - p0).normalized();
            mff::Vector2f extrusion = direction * half_width;

            auto base_point = p0;

            if (std::holds_alternative<LineCap_::Square>(style.line_cap)) {
                // if square cap move start point back
                base_point = p0 - extrusion;
            }

            if (std::holds_alternative<LineCap_::Round>(style.line_cap)) {
                std::float_t step = M_PI / (2.0 * half_width);
                std::float_t beta = std::acos(direction[0]) + M_PI_2;
                if (direction[1] < 0.0f) beta = M_PI - beta;
                std::float_t beta_to = beta + M_PI; // half circl

                beta += step;
                while (beta < beta_to) {
                    add_vert({std::cos(beta) * half_width + p0[0], std::sin(beta) * half_width + p0[1]});
                    beta += step;
                }

                std::uint32_t after_add_vertices_ix = result_vertices.size();

                for (std::uint32_t indice = curr_vertices_ix; indice < after_add_vertices_ix; indice++) {
                    add_indices_triangle(after_add_vertices_ix + 1, indice, indice + 1);
                }

                curr_vertices_ix = after_add_vertices_ix;
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

            curr_vertices_ix = result_vertices.size();

            if (std::holds_alternative<LineCap_::Round>(style.line_cap)) {
                std::float_t step = M_PI / (2.0 * half_width);
                std::float_t beta = std::acos(normal[0]) + M_PI_2;
                if (normal[1] < 0.0f) beta = M_PI - beta;
                std::float_t beta_to = beta - M_PI; // half circl

                beta -= step;
                while (beta > beta_to) {
                    add_vert({std::cos(beta) * half_width + p0[0], std::sin(beta) * half_width + p0[1]});
                    beta -= step;
                }

                std::uint32_t after_add_vertices_ix = result_vertices.size();

                for (std::uint32_t indice = curr_vertices_ix - 1; indice < (after_add_vertices_ix - 1); indice++) {
                    add_indices_triangle(indice + 1, indice, curr_vertices_ix - 2);
                }

                curr_vertices_ix = after_add_vertices_ix;
            }
        }
    };

    auto add_step = [&](const mff::Vector2f& pL, const mff::Vector2f& p0, const mff::Vector2f& pR) {
        mff::Vector2f dL = (p0 - pL); // left connection
        mff::Vector2f dR = (pR - p0); // right connection

        dL = dL.normalized(); // normalize
        dR = dR.normalized();

        auto dLp = get_perpendicular(dL); // get perpendiculars to left and right parts
        auto dRp = get_perpendicular(dR);

        // helpers
        auto dot = dL.dot(dR);
        auto alpha = std::acos(dot) / 2.0f; // angle from left part to "normal" on bisection
        auto cross = dL[0] * dR[1] - dL[1] * dR[0]; // to check orientation

        if (cross < 0) alpha = -alpha; // change alpha so we always have thecorrect orientation

        // set the bisection vector
        mff::Vector2f bisec;
        if (std::cos(alpha) > 0.1f) {
            // if alpha not too small we can use
            bisec = get_perpendicular((dL + dR).normalized()) * (half_width / std::cos(alpha));
        } else {
            // TODO: check if we can use this everytime
            std::float_t sign = cross < 0 ? 1.0f : -1.0f;
            bisec = (mff::lerp(p0 + sign * dLp * half_width, p0 + sign * dRp * half_width, 0.5f)) - p0;
        }

        // first index
        auto curr_vertices_ix = result_vertices.size();

        // check whether miter is incorrect to use (miter limit)
        auto join_to_use = std::visit(
            mff::overloaded{
                [&](const LineJoin_::Miter& m) -> LineJoin {
                    if ((half_width / std::cos(alpha)) > m.value) {
                        return LineJoin_::Bevel{};
                    }

                    return m;
                },
                [](const auto& other) -> LineJoin { return other; }
            }, style.line_join
        );

        // add joins (order dependent on curve orientation)
        std::visit(
            mff::overloaded{
                [&](const LineJoin_::Bevel b) {
                    if (cross < 0) {
                        add_vert(p0 + bisec);
                        add_vert(p0 - (dLp * half_width));
                        add_vert(p0 - (dRp * half_width));

                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 2, curr_vertices_ix + 1);
                        add_indices_triangle(curr_vertices_ix + 2, curr_vertices_ix + 4, curr_vertices_ix);
                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 3, curr_vertices_ix + 4);
                    } else {
                        add_vert(p0 + (dLp * half_width));
                        add_vert(p0 - bisec);
                        add_vert(p0 + (dRp * half_width));

                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 2, curr_vertices_ix + 1);
                        add_indices_triangle(curr_vertices_ix + 2, curr_vertices_ix + 3, curr_vertices_ix + 1);
                        add_indices_triangle(curr_vertices_ix + 1, curr_vertices_ix + 3, curr_vertices_ix + 4);
                    }
                },
                [&](const LineJoin_::Miter m) {
                    add_vert(p0 + bisec);
                    add_vert(p0 - bisec);
                    add_indices_rectangle(curr_vertices_ix);
                },
                [&](const LineJoin_::Round r) {
                    std::float_t step = M_PI / (half_width);
                    std::float_t beta = std::acos(dLp[0]); // angle of left normal
                    if (dLp[1] < 0.0f) beta = -beta;

                    if (cross < 0) {
                        // add end of current segment
                        add_vert(p0 + bisec);
                        add_vert(p0 - (dLp * half_width));

                        beta += M_PI; // we are adding the "bottom" part
                        std::float_t beta_to = beta + alpha * 2;
                        beta -= step;

                        while (beta > beta_to) {
                            add_vert({std::cos(beta) * half_width + p0[0], std::sin(beta) * half_width + p0[1]});
                            beta -= step;
                        }

                        std::uint32_t after_add_vertices_ix = result_vertices.size();

                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 2, curr_vertices_ix + 1);

                        for (std::uint32_t indice = curr_vertices_ix + 2; indice < after_add_vertices_ix; indice++) {
                            add_indices_triangle(indice, indice + 1, curr_vertices_ix);
                        }

                        add_indices_triangle(after_add_vertices_ix, after_add_vertices_ix + 2, curr_vertices_ix);
                        add_indices_triangle(curr_vertices_ix, after_add_vertices_ix + 1, after_add_vertices_ix + 2);

                        add_vert(p0 - (dRp * half_width));
                    } else {
                        // add end of current segment
                        add_vert(p0 + (dLp * half_width));
                        add_vert(p0 - bisec);

                        std::float_t beta_to = beta + alpha * 2;
                        beta += step;

                        while (beta < beta_to) {
                            add_vert({std::cos(beta) * half_width + p0[0], std::sin(beta) * half_width + p0[1]});
                            beta += step;
                        }

                        std::uint32_t after_add_vertices_ix = result_vertices.size();

                        add_indices_triangle(curr_vertices_ix, curr_vertices_ix + 2, curr_vertices_ix + 1);

                        for (std::uint32_t indice = curr_vertices_ix + 2; indice < after_add_vertices_ix; indice++) {
                            add_indices_triangle(indice, indice + 1, curr_vertices_ix + 1);
                        }

                        add_indices_triangle(after_add_vertices_ix, after_add_vertices_ix + 1, curr_vertices_ix + 1);
                        add_indices_triangle(
                            curr_vertices_ix + 1,
                            after_add_vertices_ix + 1,
                            after_add_vertices_ix + 2
                        );

                        add_vert(p0 + (dRp * half_width));
                    }
                },
            },
            join_to_use
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
        if (mff::are_approx_same(p0, pL) || mff::are_approx_same(p0, pR)) {
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
