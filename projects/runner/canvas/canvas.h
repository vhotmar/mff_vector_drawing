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

private:
    Renderer* renderer_;
};

}