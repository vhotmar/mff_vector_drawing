#include "instance.h"

#include <mff/algorithms.h>
#include <set>
#include <string>
#include <utility>
#include <utility>
#include <vector>

#include "../../constants.h"
#include "../../window/context.h"
#include "./debug.h"
#include "./dispatcher.h"
#include "../utils.h"

namespace mff::vulkan {

Version::Version(std::uint8_t maj, std::uint8_t min, std::uint8_t pat)
    : major(maj), minor(min), patch(pat) {
}

std::uint32_t Version::to_int() {
    return VK_MAKE_VERSION(major, minor, patch);
}

boost::leaf::result<std::shared_ptr<Instance>> Instance::build(
    std::optional<ApplicationInfo> info,
    std::vector<std::string> extensions,
    std::vector<std::string> layers
) {
    init_dispatcher();

    auto add_extension = [&](const std::string& extension) -> bool {
        if (mff::contains(extensions, extension)) return false;

        extensions.push_back(extension);

        return true;
    };

    if (constants::kVULKAN_DEBUG) {
        add_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        add_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

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

    auto add_layer = [&](const std::string& layer) -> bool {
        if (mff::contains(layers, layer)) return false;

        layers.push_back(layer);

        return true;
    };

    if (constants::kVULKAN_DEBUG) {
        add_layer("VK_LAYER_LUNARG_standard_validation");
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

    struct enable_Instance : public Instance {};

    std::shared_ptr<Instance> instance = std::make_shared<enable_Instance>();

    if (constants::kVULKAN_DEBUG) {
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain = {
            create_instance_info,
            get_debug_utils_create_info()
        };

        LEAF_AUTO_TO(instance->handle_, to_result(vk::createInstanceUnique(chain.get<vk::InstanceCreateInfo>())));
    } else {
        LEAF_AUTO_TO(instance->handle_, to_result(vk::createInstanceUnique(create_instance_info)));
    }

    init_dispatcher(instance->handle_.get());

    if (constants::kVULKAN_DEBUG) {
        auto create_info = get_debug_utils_create_info();
        LEAF_AUTO_TO(
            instance->debug_utils_messenger_,
            to_result(instance->handle_->createDebugUtilsMessengerEXTUnique(create_info)));
    }

    instance->extensions_ = extensions;
    instance->layers_ = layers;
    instance->info_ = info;

    LEAF_AUTO(physical_devices, to_result(instance->handle_->enumeratePhysicalDevices()));
    instance->physical_devices_.reserve(physical_devices.size());

    for (auto physical_device: physical_devices) {
        auto result = std::make_shared<PhysicalDevice>(instance, physical_device);
        result->properties_ = physical_device.getProperties();
        result->memory_ = physical_device.getMemoryProperties();
        result->features_ = physical_device.getFeatures();
        LEAF_AUTO_TO(result->extensions_, to_result(physical_device.enumerateDeviceExtensionProperties()));

        auto family_properties = physical_device.getQueueFamilyProperties();
        int index = 0;
        std::vector<QueueFamily> queue_families = mff::map(
            [&](auto properties) {
                return QueueFamily(result, properties, index++);
            },
            family_properties
        );
        result->queue_families_ = std::move(queue_families);

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

PhysicalDevice::PhysicalDevice(
    std::shared_ptr<Instance> instance,
    vk::PhysicalDevice device
)
    : instance_(instance)
    , device_(device) {
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

QueueFamily::QueueFamily(
    std::shared_ptr<PhysicalDevice> physical_device,
    vk::QueueFamilyProperties properties,
    std::size_t index
)
    : physical_device_(std::move(physical_device)), properties_(std::move(properties)), index_(index) {
}

std::uint32_t QueueFamily::get_queues_count() const {
    return properties_.queueCount;
}

bool QueueFamily::supports_graphics() const {
    return (bool) (get_properties().queueFlags & vk::QueueFlagBits::eGraphics);
}

bool QueueFamily::supports_compute() const {
    return (bool) (get_properties().queueFlags & vk::QueueFlagBits::eCompute);
}

std::uint32_t QueueFamily::get_index() const {
    return index_;
}

std::shared_ptr<PhysicalDevice> QueueFamily::get_physical_device() const {
    return physical_device_;
}

vk::QueueFamilyProperties QueueFamily::get_properties() const {
    return properties_;
}

bool QueueFamily::operator==(const QueueFamily& rhs) const {
    return index_ == rhs.index_ && physical_device_ == rhs.physical_device_;
}

}
