#pragma once

#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

class Device;
class Fence;
using UniqueFence = std::unique_ptr<Fence>;
using UniquePooledFence = ObjectPool<Fence>::pool_ptr;

class Fence {
public:
    vk::Fence get_handle() const;

    static boost::leaf::result<UniqueFence> build(const Device* device, bool signaled);
    static boost::leaf::result<UniquePooledFence> from_pool(Device* device);

private:
    Fence() = default;

    const Device* device_ = nullptr;
    vk::UniqueFence handle_ = {};
};

}