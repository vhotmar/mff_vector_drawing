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
class Semaphore;
using UniqueSemaphore = std::unique_ptr<Semaphore>;
using UniquePooledSemaphore = ObjectPool<Semaphore>::pool_ptr;

class Semaphore {
public:
    vk::Semaphore get_handle() const;

    static boost::leaf::result<UniqueSemaphore> build(const Device* device);
    static boost::leaf::result<UniquePooledSemaphore> from_pool(const Device* device);

private:
    Semaphore() = default;

    const Device* device_ = nullptr;
    vk::UniqueSemaphore handle_ = {};
};

}