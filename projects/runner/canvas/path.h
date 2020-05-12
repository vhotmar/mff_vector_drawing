#pragma once

#include "./contour.h"
#include "./math.h"
#include "./outline.h"
#include "./svg/path.h"

namespace canvas {

/**
 * This path represents the SVG paths (<path />, <ellipse />, <circle /> etc.)
 */
class Path2D {
public:
    /**
     * Close the current path
     */
    void close_path();

    /**
     * Move to new point (and close current contour)
     * @param point
     */
    void move_to(const mff::Vector2f& point);

    /**
     * Create line to next point
     * @param point
     */
    void line_to(const mff::Vector2f& point);

    /**
     * Create quadratic curve to specified point (with specified control point)
     * @param control
     * @param point
     */
    void quad_to(const mff::Vector2f& control, const mff::Vector2f& point);

    /**
     * Create cubic curve to specified point (with specified control point)
     * @param control0
     * @param control1
     * @param point
     */
    void bezier_to(const mff::Vector2f& control0, const mff::Vector2f& control1, const mff::Vector2f& point);

    /**
     * Add rectangle (and close current contour)
     * @param rect
     */
    void rect(const Rectf& rect);

    /**
     * Add ellipse with center and specified axes
     * @param center
     * @param axes
     */
    void ellipse(const mff::Vector2f& center, const mff::Vector2f& axes);

    /**
     * Get the resulting outline (non const, because we have to end the current contour)
     * @return
     */
    Outline get_outline();

    /**
     * Build Path2D from SVG commands
     * @param commands
     * @return
     */
    static Path2D from_svg_commands(std::vector<svg::Command> commands);

    /**
     * Transform all contours contained in this path
     * @param transform
     */
    void transform(const Transform2f& transform);

private:
    Outline outline_ = {};
    Contour current_contour_ = {};

    void end_current_contour();
};

}