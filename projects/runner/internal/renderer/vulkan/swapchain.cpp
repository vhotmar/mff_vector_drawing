#include "swapchain.h"

namespace mff::internal::renderer::vulkan {

tl::expected<swapchain_support_details, std::string> query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    swapchain_support_details details;

    details.capabilities = VK_TRY(device.getSurfaceCapabilitiesKHR(surface));
    details.formats = VK_TRY(device.getSurfaceFormatsKHR(surface));
    details.present_modes = VK_TRY(device.getSurfacePresentModesKHR(surface));

    return details;
}

}
