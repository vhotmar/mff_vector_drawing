#pragma once

#include <range/v3/all.hpp>

#include "./path.h"
#include "../third_party/earcut.hpp"
#include "../renderer/renderer.h"
#include "./stroke.h"

namespace canvas {

/**
 * This class provides us with helper functions to render Path2D objects (which represents vector
 * graphics primitives)
 */
class Canvas {
public:
    Canvas(Renderer* renderer);

    /**
     * Information how to fill the shape
     */
    struct FillInfo {
        mff::Vector4f color = mff::Vector4f::Ones();
        Transform2f transform = Transform2f::identity();
    };

    /**
     * Fill the specified path
     * @param path
     * @param info
     */
    void fill(canvas::Path2D& path, const FillInfo& info);

    struct StrokeInfo {
        mff::Vector4f color = mff::Vector4f::Ones();
        StrokeStyle style = {};
        Transform2f transform = Transform2f::identity();
    };

    /**
     * Stroke the specified path
     * @param path
     * @param info
     */
    void stroke(canvas::Path2D& path, const StrokeInfo& info);

    /**
     * Path consists of multiple records (multiple contours / closed paths)
     */
    struct PrerenderedPath {
        struct Record {
            std::vector<Vertex> vertices = {};
            std::vector<std::uint32_t> indices = {};
            PushConstants constants = {};

            Record(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, PushConstants constants);
        };

        std::vector<Record> records = {};
    };

    /**
     * Prerender stroke of path (to be reused)
     * @param path
     * @param info
     * @return
     */
    static PrerenderedPath prerenderStroke(canvas::Path2D& path, const StrokeInfo& info);

    /**
     * Prerender fill of path (to be reused)
     * @param path
     * @param info
     * @return
     */
    static PrerenderedPath prerenderFill(canvas::Path2D& path, const FillInfo& info);

    /**
     * Draw the prerendered primitives
     * @param item
     */
    void drawPrerendered(const PrerenderedPath& item);

private:
    Renderer* renderer_;
};

}