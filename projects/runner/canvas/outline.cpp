#include "./outline.h"

namespace canvas {

void Outline::add_contour(const Contour& contour) {
    if (contour.empty()) return;

    contours_.push_back(contour);
}

const std::vector<Contour>& Outline::get_contours() const {
    return contours_;
}

void Outline::transform(const Transform2f& transform) {
    for (auto& contour: contours_) {
        contour.transform(transform);
    }
}

}