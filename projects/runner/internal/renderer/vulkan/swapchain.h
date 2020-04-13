#pragma once

#include <string>
#include <vector>

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

struct swapchain_support_details {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

boost::leaf::result<swapchain_support_details>
query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface);

class Swapchain {
public:

};

}