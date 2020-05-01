#pragma once

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class Device;
class CommandPool;

using UniqueCommandPool = std::unique_ptr<CommandPool>;

class CommandPool {
public:
    boost::leaf::result<std::vector<vk::UniqueCommandBuffer>> allocate(
        std::uint32_t count,
        vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
    ) const;

    static boost::leaf::result<UniqueCommandPool> build(const Device* device, const QueueFamily* queue_family);

private:
    CommandPool() = default;

    const Device* device_ = nullptr;
    const QueueFamily* queue_family_ = nullptr;
    vk::UniqueCommandPool pool_ = {};
};

}