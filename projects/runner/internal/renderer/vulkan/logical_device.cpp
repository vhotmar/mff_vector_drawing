#include "logical_device.h"

#include <mff/algorithms.h>
#include <set>
#include <vector>

#include "../utils.h"
#include "dispatcher.h"
#include "layers.h"
#include "queue_family.h"

namespace mff::internal::renderer::vulkan {

std::set<std::string> get_logical_device_required_extensions() {
    return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

tl::expected<DeviceRaw, std::string> create_logical_device(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
    auto indices = TRY(find_queue_families(physical_device, surface));

    auto priority = 1.0f;

    std::set<uint32_t> unique_queue_families = {
        indices.graphics_family.value(),
        indices.present_family.value()
    };

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos = mff::map(
        [&](auto id) {
            return vk::DeviceQueueCreateInfo({}, id, 1, &priority);
        },
        unique_queue_families);

    vk::PhysicalDeviceFeatures device_features;

    auto layers = get_required_mff_layers();
    auto layers_c = utils::to_pointer_char_data(layers);

    auto extensions = get_logical_device_required_extensions();
    auto extensions_c = utils::to_pointer_char_data(extensions);

    auto device_create_info = vk::DeviceCreateInfo(
        {},
        queue_create_infos.size(),
        queue_create_infos.data(),
        layers_c.size(),
        layers_c.data(),
        extensions_c.size(),
        extensions_c.data());

    auto device = VK_TRY(physical_device.createDeviceUnique(device_create_info));

    init_dispatcher(device.get());

    auto graphics_queue = device->getQueue(indices.graphics_family.value(), 0);
    auto present_queue = device->getQueue(indices.present_family.value(), 0);

    return DeviceRaw{std::move(device), {graphics_queue, present_queue}};
}

}