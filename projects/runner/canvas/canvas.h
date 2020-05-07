#pragma once

#include <range/v3/all.hpp>

#include "./path.h"
#include "../third_party/earcut.hpp"
#include "../renderer/renderer.h"

namespace canvas {

class Canvas {
public:
    Canvas(Renderer* renderer);

    void fill(canvas::Path2D path, mff::Vector4f color);

    struct PrerenderedPathFill {
        struct Record {
            std::vector<Vertex> vertices = {};
            std::vector<std::uint32_t> indices = {};
            PushConstants constants = {};

            Record(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, PushConstants constants);
        };

        std::vector<Record> records = {};
    };

    static PrerenderedPathFill prerenderFill(canvas::Path2D path, mff::Vector4f color);
    void drawPrerendered(const PrerenderedPathFill& item);

private:
    Renderer* renderer_;
};

}