#pragma once

#include <memory>
#include <tuple>
#include <vector>

#include <mff/leaf.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/vulkan/instance.h>

namespace mff::vulkan {

// Forward definitions
class Device;

/**
 * Represent Queue where commands are executed. In specification you are grabbing individual
 * queues, but we will take all queues for given family and then use strategy like round-robin
 * to use them.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#devsandqueues-queues
 */
class Queue {
    friend class Device;

private:
    std::vector<vk::Queue> queues_;
    const Device* device_;
    QueueFamily queue_family_;

    Queue(QueueFamily family);
};

enum class create_device_error_code {
    priority_out_of_range_error,
    too_many_queues_for_family_error
};

/**
 * Represents Vulkan context specific for instance and physical device.
 *
 * Note: ignoring groups for physical devices now
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#devsandqueues-devices
 */
class Device {
private:
    const Instance* instance_;
    const PhysicalDevice* physical_device_;
    vk::UniqueDevice handle_;
    std::vector<std::string> layers_;
    std::vector<std::string> extensions_;

    Device() = default;

public:
    /**
     * @return corresponding physical device
     */
    const PhysicalDevice* get_physical_device() const;

    /**
     * @return layers loaded for this device
     */
    const std::vector<std::string>& get_layers() const;

    /**
     * @return extensions loaded for this device
     */
    const std::vector<std::string>& get_extensions() const;

    /**
     * @return concrete vulkan Device
     */
    vk::Device get_handle() const;

    /**
     * Builds new vulkan Device for specified PhysicalDevice.
     * @param physical_device PhysicalDevice on which to create the Device
     * @param queue_families Vector of QueueFamily for which we will create corresponding Queue
     *                       objects. Ignoring priorities because there is no guarantee by
     *                       specification that they will be used.
     * @param extensions A list of vulkan extensions to enable for new Device
     * @return
     */
    static boost::leaf::result<std::tuple<std::unique_ptr<Device>, std::vector<std::shared_ptr<Queue>>>> build(
        const PhysicalDevice* physical_device,
        const std::vector<QueueFamily>& queue_families,
        const std::vector<std::string>& extensions
    );
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::create_device_error_code> : public std::true_type {};

}