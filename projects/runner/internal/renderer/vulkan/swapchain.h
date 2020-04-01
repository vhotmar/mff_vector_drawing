#pragma once

#include <string>
#include <vector>

#include "../../expected.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

struct swapchain_support_details {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

tl::expected<swapchain_support_details, std::string> query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface);

}