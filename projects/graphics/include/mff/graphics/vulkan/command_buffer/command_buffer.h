#pragma once

#include <variant>
#include <vector>

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/image/image.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {


namespace Kind_ {

struct Primary {};
struct Secondary {
    // TODO: missing... everything
};

}

using Kind = std::variant<Kind_::Primary, Kind_::Secondary>;

}