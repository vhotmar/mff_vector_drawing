#include "debug.h"

#include "../../../utils/logger.h"

namespace mff::internal::renderer::vulkan {

vk::DebugUtilsMessengerCreateInfoEXT get_debug_utils_create_info() {
    return vk::DebugUtilsMessengerCreateInfoEXT(
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        &debug_callback,
        nullptr);
}

VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT flags,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* p_user_data
) {
    auto level = spdlog::level::level_enum::debug;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        level = spdlog::level::level_enum::err;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        level = spdlog::level::level_enum::warn;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        level = spdlog::level::level_enum::info;
    }

    logger::vulkan->log(level, "Code {0}: {1}", data->pMessageIdName, data->pMessage);

    return false;
}

}