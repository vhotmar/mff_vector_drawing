#pragma once

#include <string>

#include "../../expected.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

struct InstanceRaw {
    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT utils;
};

tl::expected<InstanceRaw, std::string> create_instance(const std::string& name);

}