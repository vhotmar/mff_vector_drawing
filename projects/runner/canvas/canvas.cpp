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
    auto prerendered = prerenderFill(path, color);
    drawPrerendered(prerendered);
}

Canvas::PrerenderedPathFill::Record::Record(
    std::vector<Vertex> vertices,
    std::vector<std::uint32_t> indices,
    PushConstants constants
)
    : vertices(std::move(vertices))
    , indices(std::move(indices))
    , constants(std::move(constants)) {
}

Canvas::PrerenderedPathFill Canvas::prerenderFill(canvas::Path2D path, mff::Vector4f color) {
    PrerenderedPathFill result = {};

    auto cs = path.get_outline().get_contours();

    for (auto contour: cs) {
        auto flattened = contour.flatten();
        auto indices = ::mapbox::earcut<std::uint32_t>(std::vector<std::vector<mff::Vector2f>>{flattened});
        auto vertices = flattened
            | ranges::views::transform([](const auto& pos) { return Vertex{pos}; })
            | ranges::to<std::vector>();

        result.records.emplace_back(std::move(vertices), std::move(indices), PushConstants{1.0f, color});
    }

    return std::move(result);
}

void Canvas::drawPrerendered(const Canvas::PrerenderedPathFill& prerendered) {
    for (const auto& item: prerendered.records) {
        renderer_->draw(item.vertices, item.indices, item.constants);
    }
}

}
