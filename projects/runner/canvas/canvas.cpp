#include "./canvas.h"

namespace mapbox::util {

template <>
struct nth<0, ::mff::Vector2f> {
    inline static auto get(const ::mff::Vector2f& t) {
        return t[0];
    };
};
template <>
struct nth<1, ::mff::Vector2f> {
    inline static auto get(const ::mff::Vector2f& t) {
        return t[1];
    };
};

}

namespace canvas {

Canvas::Canvas(Renderer* renderer)
    : renderer_(renderer) {
}

void Canvas::fill(canvas::Path2D path, mff::Vector4f color) {
    auto cs = path.get_outline().get_contours();

    for (auto contour: cs) {
        auto flattened = contour.flatten();
        auto indices = ::mapbox::earcut<std::uint32_t>(std::vector<std::vector<mff::Vector2f>>{flattened});
        auto vertices = flattened
            | ranges::views::transform([](const auto& pos) { return Vertex{pos}; })
            | ranges::to<std::vector>();

        renderer_->draw(vertices, indices, PushConstants{1.0f, color});
    }
}

}
