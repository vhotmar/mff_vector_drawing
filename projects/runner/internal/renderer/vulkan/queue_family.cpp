#include "queue_family.h"

namespace mff::internal::renderer::vulkan {

queue_family_indices find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    queue_family_indices indices;

    auto queue_families = device.getQueueFamilyProperties();

    int i = 0;

    for (const auto& family: queue_families) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }

        LEAF_DEFAULT(surface_supported, false, to_result(device.getSurfaceSupportKHR(i, surface)));
        if (surface_supported) {
            indices.present_family = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

}
