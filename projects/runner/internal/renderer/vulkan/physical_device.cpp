#include "physical_device.h"

#include <set>

#include "queue_family.h"
#include "swapchain.h"

#include <boost/outcome/try.hpp>

namespace mff::internal::renderer::vulkan {

std::set<std::string> get_required_physical_device_extension() {
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

boost::leaf::result<bool> check_physical_device_extension_support(vk::PhysicalDevice device) {
    LEAF_AUTO(available_extensions, to_result(device.enumerateDeviceExtensionProperties()));
    auto required_extensions = get_required_physical_device_extension();

    for (const auto& extension: available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

boost::leaf::result<bool> is_physical_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    auto queue_families = find_queue_families(device, surface);
    auto properties = device.getProperties();
    auto features = device.getFeatures();

    bool is_discrete = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    bool has_queue_families = queue_families.is_complete();
    LEAF_AUTO(required_extensions_supported, check_physical_device_extension_support(device));

    bool swapchain_adequate = false;

    // we can't query for swapchain support if the swapchain extension is not supported
    if (required_extensions_supported) {
        LEAF_AUTO(details, query_swapchain_support(device, surface));
        swapchain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }

    return is_discrete
        && has_queue_families
        && required_extensions_supported
        && swapchain_adequate;
}

boost::leaf::result<vk::PhysicalDevice> get_physical_device(vk::Instance instance, vk::SurfaceKHR surface) {
    LEAF_AUTO(devices, to_result(instance.enumeratePhysicalDevices()));
    vk::PhysicalDevice physical_device;

    bool found = false;

    for (const auto& device: devices) {
        if (is_physical_device_suitable(device, surface)) {
            physical_device = device;

            found = true;

            break;
        }
    }

    if (!found) return boost::leaf::new_error(get_physical_device_error_code::device_not_found_error);

    return physical_device;
}

}
