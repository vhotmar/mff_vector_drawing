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

void Canvas::fill(canvas::Path2D& path, const Canvas::FillInfo& info) {
    auto prerendered = prerenderFill(path, info);
    drawPrerendered(prerendered);
}


void Canvas::stroke(canvas::Path2D& path, const Canvas::StrokeInfo& info) {
    auto prerendered = prerenderStroke(path, info);
    drawPrerendered(prerendered);
}

Canvas::PrerenderedPath::Record::Record(
    std::vector<Vertex> vertices,
    std::vector<std::uint32_t> indices,
    PushConstants constants
)
    : vertices(std::move(vertices))
    , indices(std::move(indices))
    , constants(std::move(constants)) {
}

Canvas::PrerenderedPath Canvas::prerenderFill(canvas::Path2D& path, const Canvas::FillInfo& info) {
    PrerenderedPath result = {};

    auto cs = path.get_outline().get_contours();

    for (const auto& contour: cs) {
        auto flattened = contour.flatten();
        auto indices = ::mapbox::earcut<std::uint32_t>(std::vector<std::vector<mff::Vector2f>>{flattened});
        auto vertices = flattened
            | ranges::views::transform([](const auto& pos) { return Vertex{pos}; })
            | ranges::to<std::vector>();

        result.records
            .emplace_back(
                std::move(vertices),
                std::move(indices),
                PushConstants{info.color, info.transform.transform, info.transform.transpose}
            );
    }

    return std::move(result);
}

void Canvas::drawPrerendered(const Canvas::PrerenderedPath& prerendered) {
    for (const auto& item: prerendered.records) {
        renderer_->draw(item.vertices, item.indices, item.constants);
    }
}

Canvas::PrerenderedPath Canvas::prerenderStroke(canvas::Path2D& path, const Canvas::StrokeInfo& info) {
    PrerenderedPath result = {};

    auto cs = path.get_outline().get_contours();

    for (const auto& contour: cs) {
        auto flattened = contour.flatten();
        auto points = get_stroke(flattened, info.style, contour.closed);
        auto vertices = points.vertices
            | ranges::views::transform([](const auto& pos) { return Vertex{pos}; })
            | ranges::to<std::vector>();

        result.records
            .emplace_back(
                std::move(vertices),
                std::move(points.indices),
                PushConstants{info.color, info.transform.transform, info.transform.transpose}
            );
    }

    return result;
}

//Canvas::PrerenderedPath Canvas::prerenderStroke(canvas::Path2D& path, const Canvas::StrokeInfo& info) {
//    PrerenderedPath result = {};
//
//    auto cs = path.get_outline().get_contours();
//
//    for (const auto& contour: cs) {
//        auto stroken = ::canvas::stroke(contour, info.style);
//        auto flattened = stroken
//            | ranges::views::transform([](const auto& contour) { return contour.flatten(); })
//            | ranges::to<std::vector>();
//        auto indices = ::mapbox::earcut<std::uint32_t>(flattened);
//        auto vertices = flattened[0]
//            | ranges::views::transform([](const auto& pos) { return Vertex{pos}; })
//            | ranges::to<std::vector>();
//
//        result.records
//            .emplace_back(
//                std::move(vertices),
//                std::move(indices),
//                PushConstants{info.color, info.transform.transform, info.transform.transpose}
//            );
//    }
//
//    return result;
//}

}
