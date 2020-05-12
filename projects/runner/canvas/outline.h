#pragma once

#include <vector>

#include "./math.h"
#include "./contour.h"

namespace canvas {

/**
 * The concrete object describing SVG path (or multiple contours)
 */
class Outline {
public:
    /**
     * Add new contour
     * @param contour
     */
    void add_contour(const Contour& contour);

    /**
     * Get all contours
     * @return
     */
    const std::vector<Contour>& get_contours() const;

    /**
     * Transform all contained contours
     * @param transform
     */
    void transform(const Transform2f& transform);

private:
    std::vector<Contour> contours_ = {};
};

}