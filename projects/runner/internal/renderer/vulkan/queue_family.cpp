#include "queue_family.h"

namespace mff::internal::renderer::vulkan {

tl::expected<queue_family_indices, std::string> find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    queue_family_indices indices;

    auto queue_families = device.getQueueFamilyProperties();

    int i = 0;

    for (const auto& family: queue_families) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }

        if (VK_TRY(device.getSurfaceSupportKHR(i, surface))) {
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
