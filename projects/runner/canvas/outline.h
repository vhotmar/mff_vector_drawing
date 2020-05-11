#pragma once

#include <vector>

#include "./math.h"
#include "./contour.h"

namespace canvas {

class Outline {
public:
    void add_contour(const Contour& contour);
    const std::vector<Contour>& get_contours() const;

    void transform(const Transform2f& transform);

private:
    std::vector<Contour> contours_;
};

}