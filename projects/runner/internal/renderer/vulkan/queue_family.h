#pragma once

#include <optional>
#include <string>

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

struct queue_family_indices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};

queue_family_indices find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface);

}
