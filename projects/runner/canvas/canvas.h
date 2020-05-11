#pragma once

#include <range/v3/all.hpp>

#include "./path.h"
#include "../third_party/earcut.hpp"
#include "../renderer/renderer.h"
#include "./stroke.h"

namespace canvas {

class Canvas {
public:
    Canvas(Renderer* renderer);

    struct FillInfo {
        mff::Vector4f color = mff::Vector4f::Ones();
        Transform2f transform = Transform2f::identity();
    };

    void fill(canvas::Path2D& path, const FillInfo& info);

    struct StrokeInfo {
        mff::Vector4f color = mff::Vector4f::Ones();
        StrokeStyle style = {};
        Transform2f transform = Transform2f::identity();
    };

    void stroke(canvas::Path2D& path, const StrokeInfo& info);

    struct PrerenderedPath {
        struct Record {
            std::vector<Vertex> vertices = {};
            std::vector<std::uint32_t> indices = {};
            PushConstants constants = {};

            Record(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, PushConstants constants);
        };

        std::vector<Record> records = {};
    };

    static PrerenderedPath prerenderStroke(canvas::Path2D& path, const StrokeInfo& info);
    static PrerenderedPath prerenderFill(canvas::Path2D& path, const FillInfo& info);
    void drawPrerendered(const PrerenderedPath& item);

private:
    Renderer* renderer_;
};

}