#include "instance.h"

#include <mff/algorithms.h>
#include <set>
#include <string>
#include <vector>

#include "../../constants.h"
#include "../../window/context.h"
#include "./debug.h"
#include "./dispatcher.h"
#include "./layers.h"
#include "../utils.h"

namespace mff::internal::renderer::vulkan {

std::vector<std::string> get_required_glfw_instance_extensions() {
    auto context = window::detail::create_glfw_context();

    unsigned int glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<std::string>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}

std::vector<std::string> get_required_mff_instance_extensions() {
    std::vector<std::string> validation_extensions = {};

    if (constants::kVULKAN_DEBUG) {
        validation_extensions = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    }

    std::vector<std::string> extensions = {};

    extensions.insert(
        std::end(extensions),
        std::begin(validation_extensions),
        std::end(validation_extensions)
    );

    return extensions;
}

std::set<std::string> get_required_instance_extensions() {
    auto glfw_extensions = get_required_glfw_instance_extensions();
    auto mff_extensions = get_required_mff_instance_extensions();

    std::set<std::string> extensions;

    extensions.insert(std::begin(glfw_extensions), std::end(glfw_extensions));
    extensions.insert(std::begin(mff_extensions), std::end(mff_extensions));

    return extensions;
}

Version::Version(std::uint8_t maj, std::uint8_t min, std::uint8_t pat)
    : major(maj), minor(min), patch(pat) {
}

std::uint32_t Version::to_int() {
    return VK_MAKE_VERSION(major, minor, patch);
}

boost::leaf::result<Instance> Instance::build(
    std::optional<ApplicationInfo> info,
    const std::vector<std::string>& extensions,
    const std::vector<std::string>& layers
) {
    init_dispatcher();

    LEAF_AUTO(instance_extensions, to_result(vk::enumerateInstanceExtensionProperties()));

    auto has_extension = [&](std::string extension) -> bool {
        return mff::contains_if(
            instance_extensions,
            [&](auto item) { return item.extensionName == extension; }
        );
    };

    // check whether we can use the extensions
    for (const auto& extension: extensions) {
        if (!has_extension(extension)) {
            return boost::leaf::new_error(
                create_instance_error_code::extension_not_found_error,
                e_extension_not_found{extension}
            );
        }
    }

    LEAF_AUTO(instance_layers, to_result(vk::enumerateInstanceLayerProperties()));

    auto has_layer = [&](std::string layer) -> bool {
        return mff::contains_if(
            instance_layers,
            [&](auto item) { return item.layerName == layer; }
        );
    };

    // check whether we can use the layers
    for (const auto& layer: layers) {
        if (!has_layer(layer)) {
            return boost::leaf::new_error(
                create_instance_error_code::layer_not_found_error,
                e_layer_not_found{layer}
            );
        }
    }

    auto extensions_c = utils::to_pointer_char_data(extensions);
    auto layers_c = utils::to_pointer_char_data(layers);

    std::optional<vk::ApplicationInfo> application_info = std::nullopt;

    if (info.has_value()) {
        application_info = vk::ApplicationInfo(
            info->application_name.c_str(),
            info->application_version.to_int(),
            info->engine_name.c_str(),
            info->engine_version.to_int(),
            VK_API_VERSION_1_1
        );
    }

    vk::InstanceCreateInfo create_instance_info(
        {},
        application_info.has_value() ? &application_info.value() : nullptr,
        layers_c.size(),
        layers_c.data(),
        extensions_c.size(),
        extensions_c.data()
    );

    Instance instance;

    LEAF_AUTO_TO(instance.handle_, to_result(vk::createInstanceUnique(create_instance_info)));

    init_dispatcher(*instance.handle_);

    instance.extensions_ = extensions;
    instance.layers_ = layers;
    instance.info_ = info;

    LEAF_AUTO(physical_devices, to_result(instance.handle_->enumeratePhysicalDevices()));

    for (auto physical_device: physical_devices) {
        auto properties = physical_device.getProperties();
        auto family_properties = physical_device.getQueueFamilyProperties();
        auto memory_properties = physical_device.getMemoryProperties();
        auto available_features = physical_device.getFeatures();

        instance.physical_devices_.emplace_back(
            physical_device,
            properties,
            family_properties,
            memory_properties,
            available_features
        );
    }

    return std::move(instance);
}

const std::vector<PhysicalDeviceInfo>& Instance::get_physical_devices() const {
    return physical_devices_;
}

vk::Instance& Instance::get_handle() {
    return handle_.get();
}

PhysicalDeviceInfo::PhysicalDeviceInfo(
    vk::PhysicalDevice device,
    vk::PhysicalDeviceProperties properties,
    std::vector<vk::QueueFamilyProperties> queue_families,
    vk::PhysicalDeviceMemoryProperties memory,
    vk::PhysicalDeviceFeatures features
)
    : device_(std::move(device))
    , properties_(std::move(properties))
    , queue_families_(std::move(queue_families))
    , memory_(std::move(memory))
    , features_(std::move(features)) {
}

void PhysicalDeviceInfo::create_logical_device() {
//    if ()
}

}
