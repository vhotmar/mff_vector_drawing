#include <mff/graphics/vulkan/command_buffer/command_pool.h>

#include <mff/graphics/utils.h>

namespace mff::vulkan {

boost::leaf::result<UniqueCommandPool> CommandPool::build(const Device* device, const QueueFamily* queue_family) {
    struct enable_CommandPtr: public CommandPool {};
    UniqueCommandPool result = std::make_unique<enable_CommandPtr>();

    result->device_ = device;
    result->queue_family_ = queue_family;

    vk::CommandPoolCreateInfo info({}, queue_family->get_index());
    LEAF_AUTO_TO(result->pool_, to_result(device->get_handle().createCommandPoolUnique(info)));

    return result;
}

boost::leaf::result<std::vector<vk::UniqueCommandBuffer>> CommandPool::allocate(
    std::uint32_t count,
    vk::CommandBufferLevel level
) const {
    vk::CommandBufferAllocateInfo info(
        pool_.get(),
        level,
        count
        );

    LEAF_AUTO(result, to_result(device_->get_handle().allocateCommandBuffersUnique(info)));

    return std::move(result);
}

}