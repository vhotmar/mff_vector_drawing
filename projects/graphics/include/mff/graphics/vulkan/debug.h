#pragma once

#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT flags,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* pUserData
);

vk::DebugUtilsMessengerCreateInfoEXT get_debug_utils_create_info();

}
