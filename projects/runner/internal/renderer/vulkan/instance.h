#pragma once

#include <string>

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::internal::renderer::vulkan {

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

class PhysicalDeviceInfo {
private:
    vk::PhysicalDevice device_;
    vk::PhysicalDeviceProperties properties_;
    std::vector<vk::QueueFamilyProperties> queue_families_;
    vk::PhysicalDeviceMemoryProperties memory_;
    vk::PhysicalDeviceFeatures features_;

public:
    PhysicalDeviceInfo(
        vk::PhysicalDevice device,
        vk::PhysicalDeviceProperties properties,
        std::vector<vk::QueueFamilyProperties> queue_families,
        vk::PhysicalDeviceMemoryProperties memory,
        vk::PhysicalDeviceFeatures features
    );

    void create_logical_device();
};

class Instance {
private:
    std::vector<std::string> extensions_;
    std::vector<std::string> layers_;
    std::optional<ApplicationInfo> info_;
    vk::UniqueInstance handle_;
    std::vector<PhysicalDeviceInfo> physical_devices_;

    Instance() = default;

public:
    const std::vector<PhysicalDeviceInfo>& get_physical_devices() const;

    vk::Instance& get_handle();

    static boost::leaf::result<Instance> build(
        std::optional<ApplicationInfo> info,
        const std::vector<std::string>& extensions,
        const std::vector<std::string>& layers
    );
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::internal::renderer::vulkan::create_instance_error_code> : public std::true_type {};

}
