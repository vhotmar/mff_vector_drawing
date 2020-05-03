#pragma once

#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <mff/object_pool.h>
#include <mff/leaf.h>
#include <mff/graphics/memory.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/command_buffer/command_pool.h>
#include <mff/graphics/vulkan/sync.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace vma {

class Allocator;
using UniqueAllocator = std::unique_ptr<Allocator>;

}

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

public:
    const vk::Queue& get_handle() const;
    const QueueFamily* get_queue_family() const;

private:
    std::vector<vk::Queue> queues_;
    const Device* device_;
    const QueueFamily* queue_family_;
};

enum class create_device_error_code {
    priority_out_of_range_error,
    too_many_queues_for_family_error
};

using SharedQueue = std::shared_ptr<Queue>;
using UniqueDevice = std::unique_ptr<Device>;

class CommandPool;
class Semaphore;
using UniqueCommandPool = std::unique_ptr<CommandPool>;
using UniqueSemaphore = std::unique_ptr<Semaphore>;
using UniquePooledSemaphore = ObjectPool<Semaphore>::pool_ptr;

/**
 * Represents Vulkan context specific for instance and physical device.
 *
 * Note: ignoring groups for physical devices now
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap4.html#devsandqueues-devices
 */
class Device {
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

    const vma::Allocator* get_allocator() const;

    mff::ObjectPool<mff::vulkan::Semaphore>* get_semaphore_pool() const;

    /**
     * Get command pool for this device and specified queue family (non-const because can allocate)
     * @param queue_family
     * @return
     */
    boost::leaf::result<CommandPool*> get_command_pool(const QueueFamily* queue_family);

    /**
     * Builds new vulkan Device for specified PhysicalDevice.
     * @param physical_device PhysicalDevice on which to create the Device
     * @param queue_families Vector of QueueFamily for which we will create corresponding Queue
     *                       objects. Ignoring priorities because there is no guarantee by
     *                       specification that they will be used.
     * @param extensions A list of vulkan extensions to enable for new Device
     * @return
     */
    static boost::leaf::result<std::tuple<UniqueDevice, std::vector<SharedQueue>>> build(
        const PhysicalDevice* physical_device,
        const std::vector<const QueueFamily*>& queue_families,
        const std::vector<std::string>& extensions
    );

private:
    Device() = default;

    const Instance* instance_ = nullptr;
    const PhysicalDevice* physical_device_ = nullptr;
    vk::UniqueDevice handle_ = {};
    std::vector<std::string> layers_ = {};
    std::vector<std::string> extensions_ = {};
    std::unordered_map<std::uint32_t, UniqueCommandPool> command_pools_ = {};
    vma::UniqueAllocator allocator_ = nullptr;
    mff::UniqueObjectPool<mff::vulkan::Semaphore> semaphores_pool_ = nullptr;
};

}

namespace boost::leaf {

template <>
struct is_e_type<mff::vulkan::create_device_error_code> : public std::true_type {};

}