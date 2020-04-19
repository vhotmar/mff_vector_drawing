#pragma once

#include "eigen.h"
#include "vulkan.h"

namespace mff {

vk::Extent2D to_extent(Vector2ui v) {
    return vk::Extent2D(v[0], v[1]);
}

}