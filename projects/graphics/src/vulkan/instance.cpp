#include <mff/graphics/vulkan/instance.h>

#include <set>
#include <string>
#include <utility>
#include <vector>

#include <mff/algorithms.h>
#include <mff/graphics/constants.h>
#include <mff/graphics/vulkan/dispatcher.h>
#include <mff/graphics/vulkan/debug.h>
#include <mff/optional.h>

#include "../utils.h"

namespace mff::vulkan {

boost::leaf::result<Instance> Instance::build(
    std::optional<ApplicationInfo> info,
    std::vector<std::string> extensions,
    std::vector<std::string> layers
) {
    // we need to initialize the dispatcher to call vulkan functions
    init_dispatcher();

    // get supported instance extensions
    LEAF_AUTO(instance_extensions, to_result(vk::enumerateInstanceExtensionProperties()));

    // check whether extension exists in instance
    auto has_extension = [&](std::string extension) -> bool {
        return mff::contains_if(
            instance_extensions,
            [&](auto item) { return item.extensionName == extension; }
        );
    };

    // add extension and returns true if is available / we are already requiring it
    auto add_extension = [&](const std::string& extension) -> bool {
        if (mff::contains(extensions, extension)) return true;
        if (!has_extension(extension)) return false;

        extensions.push_back(extension);

        return true;
    };

    if (constants::kVULKAN_DEBUG) {
        bool has_debug_extension = false;

        has_debug_extension = add_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) || has_debug_extension;
        has_debug_extension = add_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) || has_debug_extension;

        if (!has_debug_extension) {
            // we are in debug but extension not found
            return boost::leaf::new_error(create_instance_error_code::debug_extension_not_found);
        }
    }

    // check whether we can use the extensions
    for (const auto& extension: extensions) {
        if (!has_extension(extension)) {
            return boost::leaf::new_error(
                create_instance_error_code::extension_not_found_error,
                e_extension_not_found{extension}
            );
        }
    }

    // get supported instance layers
    LEAF_AUTO(instance_layers, to_result(vk::enumerateInstanceLayerProperties()));

    // check if is layer supported
    auto has_layer = [&](std::string layer) -> bool {
        return mff::contains_if(
            instance_layers,
            [&](auto item) { return item.layerName == layer; }
        );
    };

    // add layer and return true if is layer required by user or supported by instance
    auto add_layer = [&](const std::string& layer) -> bool {
        if (mff::contains(layers, layer)) return true;
        if (!has_layer(layer)) return false;

        layers.push_back(layer);

        return true;
    };

    if (constants::kVULKAN_DEBUG) {
        bool has_debug_layer = false;

        has_debug_layer = has_debug_layer || add_layer("VK_LAYER_LUNARG_standard_validation");

        if (!has_debug_layer) {
            // we are in debug but debug layer not found
            return boost::leaf::new_error(create_instance_error_code::debug_layer_not_found);
        }
    }

    // check whether we can use the layers
    for (const auto& layer: layers) {
        if (!has_layer(layer)) {
            return boost::leaf::new_error(
                create_instance_error_code::layer_not_found_error,
                e_layer_not_found{layer}
            );
        }
    }

    // vulkan needs strings as utf-8 char pointers
    auto extensions_c = utils::to_pointer_char_data(extensions);
    auto layers_c = utils::to_pointer_char_data(layers);

    std::optional<vk::ApplicationInfo> application_info = std::nullopt;

    if (info.has_value()) {
        application_info = info->to_vulkan();
    }

    vk::InstanceCreateInfo create_instance_info(
        {},
        application_info.has_value() ? &application_info.value() : nullptr,
        layers_c.size(),
        layers_c.data(),
        extensions_c.size(),
        extensions_c.data()
    );

    // we need this so std::make_shared works
    Instance instance;

    if (constants::kVULKAN_DEBUG) {
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain = {
            create_instance_info,
            get_debug_utils_create_info()
        };

        LEAF_AUTO_TO(instance.handle_, to_result(vk::createInstanceUnique(chain.get<vk::InstanceCreateInfo>())));
    } else {
        LEAF_AUTO_TO(instance.handle_, to_result(vk::createInstanceUnique(create_instance_info)));
    }

    // add instance functions to dispatcher
    init_dispatcher(instance.handle_.get());

    if (constants::kVULKAN_DEBUG) {
        auto create_info = get_debug_utils_create_info();
        LEAF_AUTO_TO(
            instance.debug_utils_messenger_,
            to_result(instance.handle_->createDebugUtilsMessengerEXTUnique(create_info)));
    }

    instance.extensions_ = extensions;
    instance.layers_ = layers;
    instance.info_ = info;

    // list all physical devices
    LEAF_AUTO(physical_devices, to_result(instance.handle_->enumeratePhysicalDevices()));
    instance.physical_devices_.reserve(physical_devices.size());

    // initialize physical devices
    for (auto physical_device: physical_devices) {
        // we need this so std::make_shared works
        struct enable_PhysicalDevice : PhysicalDevice {};
        auto result = std::make_shared<enable_PhysicalDevice>();

        result->instance_ = instance;
        result->device_ = physical_device;
        result->properties_ = physical_device.getProperties();
        result->memory_ = physical_device.getMemoryProperties();
        result->features_ = physical_device.getFeatures();

        LEAF_AUTO_TO(result->extensions_, to_result(physical_device.enumerateDeviceExtensionProperties()));

        auto family_properties = physical_device.getQueueFamilyProperties();
        result->queue_families_.reserve(family_properties.size());

        int index = 0;
        for (const auto& properties: family_properties) {
            QueueFamily family;

            family.physical_device_ = result;
            family.properties_ = properties;
            family.index_ = index++;

            result->queue_families_.push_back(family);
        }

        instance->physical_devices_.push_back(std::move(result));
    }

    return std::move(instance);
}

const std::vector<std::shared_ptr<PhysicalDevice>>& Instance::get_physical_devices() const {
    return physical_devices_;
}

const vk::Instance& Instance::get_handle() const {
    return handle_.get();
}

const std::vector<std::string>& Instance::get_loaded_layers() {
    return layers_;
}

vk::PhysicalDeviceType PhysicalDevice::get_type() const {
    return properties_.deviceType;
}

vk::PhysicalDevice PhysicalDevice::get_handle() const {
    return device_;
}

std::shared_ptr<Instance> PhysicalDevice::get_instance() {
    return instance_;
}

std::vector<QueueFamily> PhysicalDevice::get_queue_families() const {
    return queue_families_;
}

bool PhysicalDevice::operator==(const PhysicalDevice& rhs) {
    return device_ == rhs.device_ && instance_ == rhs.instance_;
}

const std::vector<vk::ExtensionProperties>& PhysicalDevice::get_extensions() const {
    return extensions_;
}

std::uint32_t QueueFamily::get_queues_count() const {
    return properties_.queueCount;
}

bool QueueFamily::supports_graphics() const {
    return (bool) (properties_.queueFlags & vk::QueueFlagBits::eGraphics);
}

std::uint32_t QueueFamily::get_index() const {
    return index_;
}

std::shared_ptr<PhysicalDevice> QueueFamily::get_physical_device() const {
    return physical_device_;
}

bool QueueFamily::operator==(const QueueFamily& rhs) const {
    return index_ == rhs.index_ && physical_device_ == rhs.physical_device_;
}

vk::ApplicationInfo ApplicationInfo::to_vulkan() {
    auto get_ptr = [](const std::optional<std::string>& data) -> const char* {
        if (data.has_value()) return data->c_str();
        return nullptr;
    };

    auto get_version = [](const std::optional<Version>& version) -> std::uint32_t {
        if (version.has_value()) return version->to_vulkan();
        return 0;
    };

    return vk::ApplicationInfo(
        get_ptr(application_name),
        get_version(application_version),
        get_ptr(engine_name),
        get_version(engine_version),
        VK_API_VERSION_1_1
    );
}

}

