#pragma once

#include <string>

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::vulkan {

enum class create_instance_error_code {
    extension_not_found_error,
    layer_not_found_error,
};

struct e_extension_not_found {
    std::string value;
};

struct e_layer_not_found {
    std::string value;
};

class Version {
public:
    Version(std::uint8_t maj, std::uint8_t min, std::uint8_t pat);

    std::uint8_t major;
    std::uint8_t minor;
    std::uint8_t patch;

    std::uint32_t to_int();
};

struct ApplicationInfo {
    std::string application_name;
    Version application_version;
    std::string engine_name;
    Version engine_version;
    bool debug = false;
};

class Instance;

class PhysicalDevice;

class QueueFamily {
private:
    std::size_t index_;
    vk::QueueFamilyProperties properties_;
    std::shared_ptr<PhysicalDevice> physical_device_;

public:
    QueueFamily(
        std::shared_ptr<PhysicalDevice> physical_device,
        vk::QueueFamilyProperties properties,
        std::size_t index
    );

    vk::QueueFamilyProperties get_properties() const;

    std::uint32_t get_index() const;

    std::uint32_t get_queues_count() const;

    std::shared_ptr<PhysicalDevice> get_physical_device() const;

    bool supports_graphics() const;

    bool supports_compute() const;

    bool operator==(const QueueFamily& rhs) const;
};

class PhysicalDevice {
    friend class Instance;

private:
    std::shared_ptr<Instance> instance_;
    vk::PhysicalDevice device_;
    vk::PhysicalDeviceProperties properties_;
    std::vector<QueueFamily> queue_families_;
    vk::PhysicalDeviceMemoryProperties memory_;
    vk::PhysicalDeviceFeatures features_;
    std::vector<vk::ExtensionProperties> extensions_;

public:
    PhysicalDevice(
        std::shared_ptr<Instance> instance,
        vk::PhysicalDevice device
    );

    std::vector<QueueFamily> get_queue_families() const;

    vk::PhysicalDevice get_handle() const;

    vk::PhysicalDeviceType get_type() const;

    const std::vector<vk::ExtensionProperties>& get_extensions() const;

    std::shared_ptr<Instance> get_instance();

    inline bool operator==(const PhysicalDevice& rhs);
};

class Instance {
private:
    std::vector<std::string> extensions_;
    std::vector<std::string> layers_;
    std::optional<ApplicationInfo> info_;
    vk::UniqueInstance handle_;
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_;
    std::vector<std::shared_ptr<PhysicalDevice>> physical_devices_;

    Instance() = default;

public:
    const std::vector<std::shared_ptr<PhysicalDevice>>& get_physical_devices() const;

    const vk::Instance& get_handle() const;

    const std::vector<std::string>& get_loaded_layers();

    static boost::leaf::result<std::shared_ptr<Instance>> build(
        std::optional<ApplicationInfo> info,
        std::vector<std::string> extensions,
        std::vector<std::string> layers
    );
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::create_instance_error_code> : public std::true_type {};

}
