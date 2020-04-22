#pragma once

#include <string>

#include "../../leaf.h"
#include "../../vulkan.h"

namespace mff::vulkan {

/**
 * Vulkan API version
 */
class Version {
public:
    Version(std::uint8_t maj, std::uint8_t min, std::uint8_t pat);

    std::uint8_t major = 0;
    std::uint8_t minor = 0;
    std::uint8_t patch = 0;

    /**
     * Convert the version to int usable by vulkan
     * @return int casted version
     */
    std::uint32_t to_int() const;
};

/**
 * Application info representation (could be used by driver to optimalize for this engine...
 * Not gonna happen
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap3.html#VkApplicationInfo
 */
struct ApplicationInfo {
    std::optional<std::string> application_name = std::nullopt;
    std::optional<Version> application_version = std::nullopt;
    std::optional<std::string> engine_name = std::nullopt;
    std::optional<Version> engine_version = std::nullopt;

    /**
     * Convert this struct to vulkan representation
     * @return vulkan ApplicationInfo
     */
    vk::ApplicationInfo to_vulkan();
};

// Forward definitions
class Instance;

class PhysicalDevice;

class Queue;

/**
 * Class representing QueueFamily in physical device
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap8.html#shaders-scope-queue-family
 */
class QueueFamily {
private:
    std::size_t index_ = -1;
    vk::QueueFamilyProperties properties_ = {};
    std::shared_ptr<PhysicalDevice> physical_device_ = nullptr;

    /**
     * This is implementation specific (user of this API should not be able to create this)
     */
    QueueFamily() = default;

    friend Instance;
    friend Queue;
public:
    /**
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#devsandqueues-index
     * @return Index of this QueueFamily within physical device
     */
    std::uint32_t get_index() const;

    /**
     * @return Queue count provided by this QueueFamily
     */
    std::uint32_t get_queues_count() const;

    /**
     * @return the physical device which owns this QueueFamily
     */
    std::shared_ptr<PhysicalDevice> get_physical_device() const;

    /**
     * @return true if queues of this family supports graphics operations
     */
    bool supports_graphics() const;

    /**
     * Equality operator
     */
    bool operator==(const QueueFamily& rhs) const;
};

/**
 * Represents one of the physical devices which are available on current machine
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#devsandqueues-physical-device-enumeration
 */
class PhysicalDevice {
    friend class Instance;

private:
    std::shared_ptr<Instance> instance_ = nullptr;
    vk::PhysicalDevice device_ = nullptr;
    vk::PhysicalDeviceProperties properties_ = {};
    std::vector<QueueFamily> queue_families_ = {};
    vk::PhysicalDeviceMemoryProperties memory_ = {};
    vk::PhysicalDeviceFeatures features_ = {};
    std::vector<vk::ExtensionProperties> extensions_ = {};

    /**
     * This is implementation specific (user of this API should not be able to create this)
     */
    PhysicalDevice() = default;

public:
    /**
     * @return queue families available for this physical device
     */
    std::vector<QueueFamily> get_queue_families() const;

    /**
     * @return the actual vulkan handle
     */
    vk::PhysicalDevice get_handle() const;

    /**
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#VkPhysicalDeviceType
     * @return device type (integreated / discrete / ...)
     */
    vk::PhysicalDeviceType get_type() const;

    /**
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap36.html#_device_extensions
     * @return extensions supported by this physical device
     */
    const std::vector<vk::ExtensionProperties>& get_extensions() const;

    /**
     * @return instance this physical device belongs to
     */
    std::shared_ptr<Instance> get_instance();

    /**
     * Equality operator
     */
    inline bool operator==(const PhysicalDevice& rhs);
};

/**
 * Vulkan instance (representing context instead of having global one) it is somewhat the
 * entrypoint to using vulkan (it contains state etc.)
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap3.html#initialization-instances
 */
class Instance {
private:
    std::vector<std::string> extensions_ = {};
    std::vector<std::string> layers_ = {};
    std::optional<ApplicationInfo> info_ = std::nullopt;
    vk::UniqueInstance handle_ = {};
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_ = {};
    std::vector<std::shared_ptr<PhysicalDevice>> physical_devices_ = {};

    Instance() = default;

public:
    /**
     * @return available physical devices on current machine (for current instance)
     */
    const std::vector<std::shared_ptr<PhysicalDevice>>& get_physical_devices() const;

    /**
     * @return the actual vk::Instance
     */
    const vk::Instance& get_handle() const;

    /**
     * @return layers that were loaded for this specific Instance
     */
    const std::vector<std::string>& get_loaded_layers();

    /**
     * Build the Instance from specified arguments
     *
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap3.html#vkCreateInstance
     * @param info ApplicationInfo gives information to vulkan about our application
     * @param extensions extension to enable for the created instance
     * @param layers layers to enable for the created instance (they are loaded in the specified order)
     * @return
     */
    static boost::leaf::result<std::shared_ptr<Instance>> build(
        std::optional<ApplicationInfo> info,
        std::vector<std::string> extensions,
        std::vector<std::string> layers
    );
};


struct e_extension_not_found {
    std::string value;
};

struct e_layer_not_found {
    std::string value;
};

/**
 * Errors that can happen during instance creation
 */
enum class create_instance_error_code {
    /**
     * Specified extension was not found in instance
     */
        extension_not_found_error,

    /**
     * Specified layer was not found in instance
     */
        layer_not_found_error,

    /**
     * Debug extension was not found, but we are in debug mode
     */
        debug_extension_not_found,


    /**
     * Debug layer was not found, but we are in debug mode
     */
        debug_layer_not_found
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::create_instance_error_code> : public std::true_type {};

}
