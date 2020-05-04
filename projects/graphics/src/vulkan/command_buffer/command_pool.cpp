#include <mff/graphics/vulkan/command_buffer/command_pool.h>

#include <range/v3/all.hpp>
#include <mff/graphics/utils.h>
#include <mff/utils.h>

namespace mff::vulkan {

boost::leaf::result<UniqueCommandPool> CommandPool::build(const Device* device, const QueueFamily* queue_family) {
    struct enable_CommandPtr : public CommandPool {};
    UniqueCommandPool result = std::make_unique<enable_CommandPtr>();

    result->device_ = device;
    result->queue_family_ = queue_family;

    vk::CommandPoolCreateInfo info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family->get_index());
    LEAF_AUTO_TO(result->handle_, to_result(device->get_handle().createCommandPoolUnique(info)));

    auto make_factory = [&](bool secondary) {
        return [&]() -> boost::leaf::result<std::unique_ptr<CommandPoolAllocation>> {
            LEAF_AUTO(allocations, result->get_allocations(1, secondary));
            auto allocation = std::move(allocations.back());

            return allocation;
        };
    };

    result->primary_pool_ = std::make_unique<ObjectPool<CommandPoolAllocation>>(make_factory(false));
    result->secondary_pool_ = std::make_unique<ObjectPool<CommandPoolAllocation>>(make_factory(true));

    return result;
}

boost::leaf::result<std::vector<UniqueCommandPoolAllocation>> CommandPool::allocate(
    std::uint32_t count,
    bool secondary
) {
    auto pool_to_use = !secondary ? primary_pool_.get() : secondary_pool_.get();
    auto to_allocate = count - pool_to_use->unused_resources();

    if (to_allocate > 0) {
        LEAF_AUTO(newly_allocated, get_allocations(to_allocate, secondary));
        pool_to_use->add(std::move(newly_allocated));
    }

    return std::move(pool_to_use->acquire(count));
}

boost::leaf::result<std::vector<std::unique_ptr<CommandPoolAllocation>>> CommandPool::get_allocations(
    std::uint32_t count,
    bool secondary
) {
    auto level = secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;

    vk::CommandBufferAllocateInfo info(
        handle_.get(),
        level,
        count
    );

    LEAF_AUTO(command_buffers, to_result(device_->get_handle().allocateCommandBuffersUnique(info)));

    return command_buffers
        | ranges::views::move
        | ranges::views::transform(
            [&](auto handle) {
                struct enable_CommandPoolAllocation : public CommandPoolAllocation {};
                std::unique_ptr<CommandPoolAllocation> result = std::make_unique<enable_CommandPoolAllocation>();

                result->pool_ = this;
                result->handle_ = std::move(handle);
                result->secondary_ = level == vk::CommandBufferLevel::eSecondary;

                return std::move(result);
            }
        )
        | ranges::to<std::vector>();
}

const Device* CommandPool::get_device() const {
    return device_;
}

vk::CommandBuffer CommandPoolAllocation::get_handle() const {
    return handle_.get();
}

const CommandPool* CommandPoolAllocation::get_pool() const {
    return pool_;
}

}