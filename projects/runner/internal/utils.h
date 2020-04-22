#pragma once

#include "eigen.h"
#include "vulkan.h"

namespace mff {

vk::Extent2D to_extent(Vector2ui v) {
    return vk::Extent2D(v[0], v[1]);
}

vk::Offset2D to_offset(Vector2ui v) {
    return vk::Offset2D(v[0], v[1]);
}

std::array<float, 4> to_array(Vector4f v) {
    return {v[0], v[1], v[2], v[3]};
}

}