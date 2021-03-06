#pragma once

#include <optional>
#include <string>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/version.h>

namespace mff::vulkan {

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
 * Class representing QueueFamily in physical device (collection of queues with same capabilities).
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap8.html#shaders-scope-queue-family
 */
class QueueFamily {
    friend Instance;
    friend Queue;
    friend PhysicalDevice;

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
    const PhysicalDevice* get_physical_device() const;

    /**
     * @return true if queues of this family supports graphics operations
     */
    bool supports_graphics() const;

    /**
     * @return true if queues of this family supports compute operations
     */
    bool supports_compute() const;

    /**
     * @return true if queues of this family supports transform operations
     */
    bool supports_transfer() const;

    /**
     * Equality operator
     */
    bool operator==(const QueueFamily& rhs) const;

private:
    QueueFamily() = default;

    std::size_t index_ = -1;
    const PhysicalDevice* physical_device_ = nullptr;
    vk::QueueFamilyProperties properties_ = {};
};

/**
 * Represents one of the physical devices which are available on current machine
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#devsandqueues-physical-device-enumeration
 */
class PhysicalDevice {
    friend class Instance;

private:
    const Instance* instance_ = nullptr;
    std::size_t index_ = -1;
    vk::PhysicalDevice handle_ = nullptr;
    vk::PhysicalDeviceProperties properties_ = {};
    std::vector<std::unique_ptr<QueueFamily>> queue_families_ = {};
    vk::PhysicalDeviceMemoryProperties memory_ = {};
    vk::PhysicalDeviceFeatures features_ = {};
    std::vector<vk::ExtensionProperties> extensions_ = {};

    /**
     * This is implementation specific (user of this API should not be able to create this)
     */

public:
    PhysicalDevice() = default;
    /**
     * @return queue families available for this physical device
     */
    std::vector<const QueueFamily*> get_queue_families() const;

    /**
     * @return the actual vulkan handle
     */
    vk::PhysicalDevice get_handle() const;

    /**
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#VkPhysicalDeviceType
     * @return device type (integrated / discrete / ...)
     */
    vk::PhysicalDeviceType get_type() const;

    /**
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap36.html#_device_extensions
     * @return extensions supported by this physical device
     */
    const std::vector<vk::ExtensionProperties>& get_extensions() const;

    /**
     * Check if extension is supported
     * @param name the name of the extension
     * @return is this extension supported?
     */
    bool is_extension_supported(const std::string& name) const;

    /**
     * Check if all provided extensions are supported
     * @param names the names of the extensions
     * @return are those extensions supported?
     */
    bool are_extensions_supported(const std::vector<std::string>& names) const;

    /**
     * @return instance this physical device belongs to
     */
    const Instance* get_instance() const;

    /**
     * Get format from candidates which has specified features
     * @param candidates candidates to consider
     * @param features features which we require from the format
     * @param tiling image tiling used with format (eOptimal is the one
     * @return the supported format or std::nullopt
     */
    std::optional<vk::Format> find_supported_format(
        const std::vector<vk::Format>& candidates,
        vk::FormatFeatureFlags features,
        vk::ImageTiling tiling = vk::ImageTiling::eOptimal
    ) const;

    /**
     * Equality operator
     */
    inline bool operator==(const PhysicalDevice& rhs) const;
};

using UniqueInstance = std::unique_ptr<Instance>;
using UniquePhysicalDevice = std::unique_ptr<PhysicalDevice>;

/**
 * Vulkan instance (representing context instead of having global one) it is somewhat the
 * entrypoint to using vulkan (it contains state etc.)
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap3.html#initialization-instances
 */
class Instance {
    friend class PhysicalDevice;
private:
    std::vector<std::string> extensions_ = {};
    std::vector<std::string> layers_ = {};
    std::optional<ApplicationInfo> info_ = std::nullopt;
    vk::UniqueInstance handle_ = {};
    vk::UniqueDebugUtilsMessengerEXT debug_utils_messenger_ = {};
    std::vector<UniquePhysicalDevice> physical_devices_ = {};

    Instance() = default;

public:
    /**
     * @return available physical devices on current machine (for current instance)
     */
    std::vector<const PhysicalDevice*> get_physical_devices() const;

    /**
     * @return the actual vk::Instance
     */
    const vk::Instance& get_handle() const;

    /**
     * @return layers that were loaded for this specific Instance
     */
    const std::vector<std::string>& get_loaded_layers() const;

    /**
     * Equality operator
     */
    inline bool operator==(const Instance& rhs) const;

    /**
     * Build the Instance from specified arguments
     *
     * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap3.html#vkCreateInstance
     * @param info ApplicationInfo gives information to vulkan about our application
     * @param extensions extension to enable for the created instance
     * @param layers layers to enable for the created instance (they are loaded in the specified order)
     * @return
     */
    static boost::leaf::result<UniqueInstance> build(
        std::optional<ApplicationInfo> info,
        const std::vector<std::string>& extensions_,
        const std::vector<std::string>& layers_
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