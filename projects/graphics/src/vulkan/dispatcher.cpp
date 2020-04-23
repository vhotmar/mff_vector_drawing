#include <mff/graphics/vulkan/vulkan.h>

#include <mutex>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace mff::vulkan {

void init_dispatcher() {
    static std::once_flag flag;

    std::call_once(
        flag, []() {
            vk::DynamicLoader dl;
            auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }
    );
}

void init_dispatcher(vk::Instance instance) {
    static std::once_flag flag;

    std::call_once(flag, [&]() {
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    });
}

void init_dispatcher(vk::Device device) {
    static std::once_flag flag;

    std::call_once(flag, [&]() {
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
    });
}

}
