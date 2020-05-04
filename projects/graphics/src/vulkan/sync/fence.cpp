#include <mff/graphics/vulkan/sync/fence.h>

#include <mff/graphics/utils.h>
#include <mff/graphics/vulkan/instance.h>

namespace mff::vulkan {

boost::leaf::result<UniqueFence> Fence::build(const Device* device, bool signaled) {
    struct enable_Fence : public Fence {};
    UniqueFence result = std::make_unique<enable_Fence>();

    result->device_ = device;

    vk::FenceCreateInfo info(signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags{});

    LEAF_AUTO_TO(
        result->handle_,
        to_result(device->get_handle().createFenceUnique(info)));

    return result;
}

vk::Fence Fence::get_handle() const {
    return handle_.get();
}

boost::leaf::result<UniquePooledFence> Fence::from_pool(Device* device) {
    LEAF_AUTO(result, device->get_fence_pool()->acquire());

    return std::move(result);
}

}