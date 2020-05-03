#pragma once

#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include <mff/graphics/vulkan/device.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/vulkan.h>

namespace mff::vulkan {

namespace SharingMode_ {

/**
 * Resource is owned by one QueueFamily and just this QueueFamily has access to it
 */
struct Exclusive {};

/**
 * Resource is not owned by any concrete QueueFamily.
 */
struct Concurrent {
    std::vector<std::uint32_t> queue_families;
};

}

/**
 * SharingMode specifies how different resources can be accessed from different queues.
 *
 * @see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap11.html#VkSharingMode
 */
using SharingMode = std::variant<SharingMode_::Exclusive, SharingMode_::Concurrent>;

SharingMode get_sharing_mode(const std::vector<const QueueFamily*>& queue_families = {});

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