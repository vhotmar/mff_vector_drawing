#include "physical_device.h"

#include <set>

#include "queue_family.h"
#include "swapchain.h"

namespace mff::internal::renderer::vulkan {

std::set<std::string> get_required_physical_device_extension() {
    return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

tl::expected<bool, std::string> check_physical_device_extension_support(vk::PhysicalDevice device) {
    auto available_extensions = VK_TRY(device.enumerateDeviceExtensionProperties());
    auto required_extensions = get_required_physical_device_extension();

    for (const auto& extension: available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

tl::expected<bool, std::string> is_physical_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    auto properties = device.getProperties();
    auto features = device.getFeatures();

    bool is_discrete = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = TRY(find_queue_families(device, surface)).is_complete();
    bool required_extensions_supported = TRY(check_physical_device_extension_support(device));

    bool swapchain_adequate = false;

    // we can't query for swapchain support if the swapchain extension is not supported
    if (required_extensions_supported) {
        auto details = TRY(query_swapchain_support(device, surface));
        swapchain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
}

tl::expected<vk::PhysicalDevice, std::string> get_physical_device(vk::Instance instance, vk::SurfaceKHR surface) {
    auto devices = VK_TRY(instance.enumeratePhysicalDevices());
    vk::PhysicalDevice physical_device;

    bool found = false;

    for (const auto& device: devices) {
        if (is_physical_device_suitable(device, surface)) {
            physical_device = device;

            found = true;

            break;
        }
    }

    if (!found) return tl::make_unexpected("physical device not found");

    return physical_device;
}

}
