#include "./path.h"

#include <mff/utils.h>

namespace canvas {

void Path2D::close_path() {
    current_contour_.close();
}

void Path2D::move_to(const mff::Vector2f& point) {
    end_current_contour();
    current_contour_.add_endpoint(point);
}

void Path2D::line_to(const mff::Vector2f& point) {
    current_contour_.add_endpoint(point);
}

void Path2D::quad_to(const mff::Vector2f& control, const mff::Vector2f& point) {
    current_contour_.add_quadratic(control, point);
}

void Path2D::bezier_to(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point) {
    current_contour_.add_cubic(control0, control1, point);
}

void Path2D::rect(const Rectf& r) {
    end_current_contour();
    current_contour_.add_endpoint(r.top_left());
    current_contour_.add_endpoint(r.top_right());
    current_contour_.add_endpoint(r.bottom_right());
    current_contour_.add_endpoint(r.bottom_left());
    current_contour_.close();
}

void Path2D::ellipse(const mff::Vector2f& center, const mff::Vector2f& axes) {
    end_current_contour();

    Transform2f transform = Transform2f::from_scale(axes).translate(center);
    current_contour_.add_ellipse(transform);

    end_current_contour();
}

Outline Path2D::get_outline() {
    end_current_contour();
    return outline_;
}

void Path2D::end_current_contour() {
    if (!current_contour_.empty()) {
        outline_.add_contour(current_contour_);
        current_contour_ = Contour{};
    }
}

void Path2D::transform(const Transform2f& transform) {
    end_current_contour();
    outline_.transform(transform);
}

Path2D Path2D::from_svg_commands(std::vector<svg::Command> commands) {
    mff::Vector2f last_control_point = {0.0f, 0.0f};
    mff::Vector2f last_point = {0.0f, 0.0f};

    Path2D result = {};

    auto get_pos = [&](svg::Position pos_rel, const mff::Vector2f& pos) {
        mff::Vector2f result;

        if (pos_rel == svg::Position::Absolute) {
            result = pos;
        } else {
            result = last_point + pos;
        }

        return result;
    };

    for (const auto& command: commands) {
        std::visit(
            mff::overloaded{
                [&](const svg::Commands_::Moveto& move_to) -> void {
                    bool first = true;

                    for (const auto& pos: move_to.coordinates) {
                        if (first) {
                            auto p = get_pos(move_to.position, pos);
                            result.move_to(p);
                            last_point = p;

                            first = false;

                            continue;
                        }

                        result.line_to(get_pos(move_to.position, pos));
                    }

                    last_control_point = last_point;
                },
                [&](const svg::Commands_::Lineto& line_to) -> void {
                    for (const auto& pos: line_to.coordinates) {
                        auto p = get_pos(line_to.position, pos);
                        last_point = p;
                        result.line_to(p);
                    }

                    last_control_point = last_point;
                },
                [&](svg::Commands_::VerticalLineto vertical_line_to) -> void {
                    for (const auto& pos: vertical_line_to.coordinates) {
                        auto y = vertical_line_to.position == svg::Position::Absolute ? pos : last_point.y() + pos;
                        mff::Vector2f p = {last_point.x(), y};
                        last_point = p;
                        result.line_to(p);
                    }

                    last_control_point = last_point;
                },
                [&](svg::Commands_::HorizontalLineto horizontal_line_to) -> void {
                    for (const auto& pos: horizontal_line_to.coordinates) {
                        auto x = horizontal_line_to.position == svg::Position::Absolute ? pos : last_point.x() + pos;
                        mff::Vector2f p = {x, last_point.y()};
                        last_point = p;
                        result.line_to(p);
                    }

                    last_control_point = last_point;
                },
                [&](svg::Commands_::Curveto curve_to) -> void {
                    for (const auto& pos: curve_to.coordinates) {
                        auto[c1, c2, p] = pos;

                        auto rc2 = get_pos(curve_to.position, c2);
                        auto rp = get_pos(curve_to.position, p);
                        result.bezier_to(
                            get_pos(curve_to.position, c1),
                            rc2,
                            rp
                        );

                        last_point = rp;
                        last_control_point = rc2;
                    }
                },
                [&](svg::Commands_::SmoothCurveto smooth_curve_to) -> void {
                    for (const auto& pos: smooth_curve_to.coordinates) {
                        auto[c2, p] = pos;

                        auto rc1 = last_point + (last_point - last_control_point);
                        auto rc2 = get_pos(smooth_curve_to.position, c2);
                        auto rp = get_pos(smooth_curve_to.position, p);
                        result.bezier_to(rc1, rc2, rp);

                        last_point = rp;
                        last_control_point = rc2;
                    }
                },
                [&](svg::Commands_::Closepath close) -> void {
                    result.close_path();
                }
            },
            command
        );
    }

    return result;
}

}