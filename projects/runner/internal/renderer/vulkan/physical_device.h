#pragma once

#include "../../expected.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

tl::expected<vk::PhysicalDevice, std::string> get_physical_device(vk::Instance instance, vk::SurfaceKHR surface);

}
