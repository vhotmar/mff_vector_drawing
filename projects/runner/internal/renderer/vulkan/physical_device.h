#pragma once

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

enum class get_physical_device_error_code {
    device_not_found_error,
};

boost::leaf::result<vk::PhysicalDevice> get_physical_device(vk::Instance instance, vk::SurfaceKHR surface);

}

namespace boost::leaf {

template <>
struct is_e_type<mff::internal::renderer::vulkan::get_physical_device_error_code> : public std::true_type {};

}
