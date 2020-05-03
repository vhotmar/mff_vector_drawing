#pragma once

#include <variant>
#include <vector>

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/image/image.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class Device;
class CommandPool;
class CommandPoolAllocation;

using UniqueCommandPool = std::unique_ptr<CommandPool>;
using UniqueCommandPoolAllocation = mff::ObjectPool<CommandPoolAllocation>::pool_ptr;

class CommandPoolAllocation {
    friend class CommandPool;

public:
    vk::CommandBuffer get_handle() const;

    const CommandPool* get_pool() const;

private:
    CommandPoolAllocation() = default; // unnecessary but we should keep the style

    const CommandPool* pool_ = nullptr;
    vk::UniqueCommandBuffer handle_ = {};
    bool secondary_ = false;
};

class CommandPool {
public:
    boost::leaf::result<std::vector<UniqueCommandPoolAllocation>> allocate(
        std::uint32_t count,
        bool secondary = false
    );

    static boost::leaf::result<UniqueCommandPool> build(const Device* device, const QueueFamily* queue_family);

    const Device* get_device() const;

private:
    CommandPool() = default;

    boost::leaf::result<std::vector<std::unique_ptr<CommandPoolAllocation>>> get_allocations(
        std::uint32_t count,
        bool secondary = false
    );

    const Device* device_ = nullptr;
    const QueueFamily* queue_family_ = nullptr;
    vk::UniqueCommandPool handle_ = {};
    mff::UniqueObjectPool<CommandPoolAllocation> primary_pool_;
    mff::UniqueObjectPool<CommandPoolAllocation> secondary_pool_;
};

}