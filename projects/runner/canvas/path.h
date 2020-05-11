#pragma once

#include "./contour.h"
#include "./math.h"
#include "./outline.h"
#include "./svg/path.h"

namespace canvas {

class Path2D {
public:
    void close_path();
    void move_to(const mff::Vector2f& point);
    void line_to(const mff::Vector2f& point);
    void quad_to(const mff::Vector2f& control, const mff::Vector2f& point);
    void bezier_to(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point);
    void rect(const Rectf& point);
    void ellipse(const mff::Vector2f& center, const mff::Vector2f& axes);

    Outline get_outline();

    static Path2D from_svg_commands(std::vector<svg::Command> commands);

    void transform(const Transform2f& transform);

private:
    Outline outline_;
    Contour current_contour_;

    void end_current_contour();
};

}